/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2008 Jeff Zaroyko
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
#include <stdio.h>

#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "wincon.h"
#include "fileapi.h"
#include "shlwapi.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"
#include "ioringapi.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

/* info structure for FindFirstFile handle */
typedef struct
{
    DWORD             magic;       /* magic number */
    HANDLE            handle;      /* handle to directory */
    CRITICAL_SECTION  cs;          /* crit section protecting this structure */
    FINDEX_SEARCH_OPS search_op;   /* Flags passed to FindFirst.  */
    FINDEX_INFO_LEVELS level;      /* Level passed to FindFirst */
    UNICODE_STRING    path;        /* NT path used to open the directory */
    BOOL              is_root;     /* is directory the root of the drive? */
    UINT              data_pos;    /* current position in dir data */
    UINT              data_len;    /* length of dir data */
    UINT              data_size;   /* size of data buffer, or 0 when everything has been read */
    BYTE              data[1];     /* directory data */
} FIND_FIRST_INFO;

#define FIND_FIRST_MAGIC  0xc0ffee11

static const UINT max_entry_size = offsetof( FILE_BOTH_DIRECTORY_INFORMATION, FileName[256] );

const WCHAR windows_dir[] = L"C:\\windows";
const WCHAR system_dir[] = L"C:\\windows\\system32";

static BOOL oem_file_apis;


static void WINAPI read_write_apc( void *apc_user, PIO_STATUS_BLOCK io, ULONG reserved )
{
    LPOVERLAPPED_COMPLETION_ROUTINE func = apc_user;
    func( RtlNtStatusToDosError( io->Status ), io->Information, (LPOVERLAPPED)io );
}

static const WCHAR *get_machine_wow64_dir( WORD machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_TARGET_HOST: return system_dir;
    case IMAGE_FILE_MACHINE_I386:        return L"C:\\windows\\syswow64";
    case IMAGE_FILE_MACHINE_ARMNT:       return L"C:\\windows\\sysarm32";
    default: return NULL;
    }
}


/***********************************************************************
 * Operations on file names
 ***********************************************************************/


/***********************************************************************
 *	contains_path
 *
 * Check if the file name contains a path; helper for SearchPathW.
 * A relative path is not considered a path unless it starts with ./ or ../
 */
static inline BOOL contains_path( const WCHAR *name )
{
    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH) return TRUE;
    if (name[0] != '.') return FALSE;
    if (name[1] == '/' || name[1] == '\\') return TRUE;
    return (name[1] == '.' && (name[2] == '/' || name[2] == '\\'));
}


/***********************************************************************
 *      add_boot_rename_entry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registry key is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL add_boot_rename_entry( LPCWSTR source, LPCWSTR dest, DWORD flags )
{
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING session_manager = RTL_CONSTANT_STRING( L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );
    UNICODE_STRING pending_file_rename_operations = RTL_CONSTANT_STRING( L"PendingFileRenameOperations" );
    UNICODE_STRING source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE key = 0;
    DWORD len1, len2;
    DWORD size = 0;
    BYTE *buffer = NULL;
    WCHAR *p;

    if (!RtlDosPathNameToNtPathName_U( source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (dest && !RtlDosPathNameToNtPathName_U( dest, &dest_name, NULL, NULL ))
    {
        RtlFreeUnicodeString( &source_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &session_manager;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (NtCreateKey( &key, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        RtlFreeUnicodeString( &source_name );
        RtlFreeUnicodeString( &dest_name );
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (dest)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( key, &pending_file_rename_operations, KeyValuePartialInformation,
                         NULL, 0, &size ) == STATUS_BUFFER_TOO_SMALL)
    {
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size + len1 + len2 + sizeof(WCHAR) ))) goto done;
        if (NtQueryValueKey( key, &pending_file_rename_operations, KeyValuePartialInformation, buffer, size, &size )) goto done;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
        if (info->Type != REG_MULTI_SZ) goto done;
        if (size > sizeof(info)) size -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        size = info_size;
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, size + len1 + len2 + sizeof(WCHAR) ))) goto done;
    }

    memcpy( buffer + size, source_name.Buffer, len1 );
    size += len1;
    p = (WCHAR *)(buffer + size);
    if (dest)
    {
        if (flags & MOVEFILE_REPLACE_EXISTING) *p++ = '!';
        memcpy( p, dest_name.Buffer, len2 );
        size += len2;
    }
    else
    {
        *p = 0;
        size += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(buffer + size);
    *p = 0;
    size += sizeof(WCHAR);
    rc = !NtSetValueKey( key, &pending_file_rename_operations, 0, REG_MULTI_SZ, buffer + info_size, size - info_size );

 done:
    RtlFreeUnicodeString( &source_name );
    RtlFreeUnicodeString( &dest_name );
    if (key) NtClose(key);
    HeapFree( GetProcessHeap(), 0, buffer );
    return rc;
}


/***********************************************************************
 *	append_ext
 */
static WCHAR *append_ext( const WCHAR *name, const WCHAR *ext )
{
    const WCHAR *p;
    WCHAR *ret;
    DWORD len;

    if (!ext) return NULL;
    p = wcsrchr( name, '.' );
    if (p && !wcschr( p, '/' ) && !wcschr( p, '\\' )) return NULL;

    len = lstrlenW( name ) + lstrlenW( ext );
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
    {
        lstrcpyW( ret, name );
        lstrcatW( ret, ext );
    }
    return ret;
}


/***********************************************************************
 *	find_actctx_dllpath
 *
 * Find the path (if any) of the dll from the activation context.
 * Returned path doesn't include a name.
 */
static NTSTATUS find_actctx_dllpath( const WCHAR *name, WCHAR **path )
{
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    RtlInitUnicodeString( &nameW, name );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    for (;;)
    {
        if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( GetProcessHeap(), 0, info );
        size = needed;
        /* restart with larger buffer */
    }

    if (!info->lpAssemblyManifestPath)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    if ((p = wcsrchr( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);

        p++;
        if (!dirlen ||
            CompareStringOrdinal( p, dirlen, info->lpAssemblyDirectoryName, dirlen, TRUE ) != CSTR_EQUAL ||
            wcsicmp( p + dirlen, L".manifest" ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR);
            if (!(*path = p = HeapAlloc( GetProcessHeap(), 0, needed )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            *(p + dirlen) = 0;
            goto done;
        }
    }

    if (!info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    needed = sizeof(L"C:\\windows\\winsxs\\") + info->ulAssemblyDirectoryNameLength + sizeof(WCHAR);

    if (!(*path = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    lstrcpyW( p, L"C:\\windows\\winsxs\\" );
    p += lstrlenW(p);
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = '\\';
    *p = 0;
done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    return status;
}


/***********************************************************************
 *           copy_filename
 */
static DWORD copy_filename( const WCHAR *name, WCHAR *buffer, DWORD len )
{
    UINT ret = lstrlenW( name ) + 1;
    if (buffer && len >= ret)
    {
        lstrcpyW( buffer, name );
        ret--;
    }
    return ret;
}


/***********************************************************************
 *           copy_filename_WtoA
 *
 * copy a file name back to OEM/Ansi, but only if the buffer is large enough
 */
static DWORD copy_filename_WtoA( LPCWSTR nameW, LPSTR buffer, DWORD len )
{
    UNICODE_STRING strW;
    DWORD ret;

    RtlInitUnicodeString( &strW, nameW );

    ret = oem_file_apis ? RtlUnicodeStringToOemSize( &strW ) : RtlUnicodeStringToAnsiSize( &strW );
    if (buffer && ret <= len)
    {
        ANSI_STRING str;

        str.Buffer = buffer;
        str.MaximumLength = min( len, UNICODE_STRING_MAX_CHARS );
        if (oem_file_apis)
            RtlUnicodeStringToOemString( &str, &strW, FALSE );
        else
            RtlUnicodeStringToAnsiString( &str, &strW, FALSE );
        ret = str.Length;  /* length without terminating 0 */
    }
    return ret;
}


/***********************************************************************
 *           file_name_AtoW
 *
 * Convert a file name to Unicode, taking into account the OEM/Ansi API mode.
 *
 * If alloc is FALSE uses the TEB static buffer, so it can only be used when
 * there is no possibility for the function to do that twice, taking into
 * account any called function.
 */
WCHAR *file_name_AtoW( LPCSTR name, BOOL alloc )
{
    ANSI_STRING str;
    UNICODE_STRING strW, *pstrW;
    NTSTATUS status;

    RtlInitAnsiString( &str, name );
    pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;
    if (oem_file_apis)
        status = RtlOemStringToUnicodeString( pstrW, &str, alloc );
    else
        status = RtlAnsiStringToUnicodeString( pstrW, &str, alloc );
    if (status == STATUS_SUCCESS) return pstrW->Buffer;

    if (status == STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return NULL;
}


/***********************************************************************
 *           file_name_WtoA
 *
 * Convert a file name back to OEM/Ansi. Returns number of bytes copied.
 */
DWORD file_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen )
{
    DWORD ret;

    if (srclen < 0) srclen = lstrlenW( src ) + 1;
    if (!destlen)
    {
        if (oem_file_apis)
        {
            UNICODE_STRING strW;
            strW.Buffer = (WCHAR *)src;
            strW.Length = srclen * sizeof(WCHAR);
            ret = RtlUnicodeStringToOemSize( &strW ) - 1;
        }
        else
            RtlUnicodeToMultiByteSize( &ret, src, srclen * sizeof(WCHAR) );
    }
    else
    {
        if (oem_file_apis)
            RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
        else
            RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    }
    return ret;
}


/***********************************************************************
 *           is_same_file
 */
static BOOL is_same_file( HANDLE h1, HANDLE h2 )
{
    FILE_OBJECTID_BUFFER id1, id2;
    IO_STATUS_BLOCK io;

    return (!NtFsControlFile( h1, 0, NULL, NULL, &io, FSCTL_GET_OBJECT_ID, NULL, 0, &id1, sizeof(id1) ) &&
            !NtFsControlFile( h2, 0, NULL, NULL, &io, FSCTL_GET_OBJECT_ID, NULL, 0, &id2, sizeof(id2) ) &&
            !memcmp( &id1.ObjectId, &id2.ObjectId, sizeof(id1.ObjectId) ));
}


/******************************************************************************
 *	AreFileApisANSI   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AreFileApisANSI(void)
{
    return !oem_file_apis;
}

/******************************************************************************
 *  copy_file
 */
static BOOL copy_file( const WCHAR *source, const WCHAR *dest, COPYFILE2_EXTENDED_PARAMETERS *params )
{
    DWORD flags = params ? params->dwCopyFlags : 0;
    BOOL *cancel_ptr = params ? params->pfCancel : NULL;
    PCOPYFILE2_PROGRESS_ROUTINE progress = params ? params->pProgressRoutine : NULL;

    static const int buffer_size = 65536;
    HANDLE h1, h2;
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;
    DWORD count;
    BOOL ret = FALSE;
    char *buffer;

    if (cancel_ptr)
        FIXME("pfCancel is not supported\n");
    if (progress)
        FIXME("PCOPYFILE2_PROGRESS_ROUTINE is not supported\n");

    if (!source || !dest)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_size )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    TRACE("%s -> %s, %lx\n", debugstr_w(source), debugstr_w(dest), flags);

    if (flags & COPY_FILE_RESTARTABLE)
        FIXME("COPY_FILE_RESTARTABLE is not supported\n");
    if (flags & COPY_FILE_COPY_SYMLINK)
        FIXME("COPY_FILE_COPY_SYMLINK is not supported\n");
    if (flags & COPY_FILE_OPEN_SOURCE_FOR_WRITE)
        FIXME("COPY_FILE_OPEN_SOURCE_FOR_WRITE is not supported\n");

    if ((h1 = CreateFileW( source, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           NULL, OPEN_EXISTING, 0, 0 )) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open source %s\n", debugstr_w(source));
        HeapFree( GetProcessHeap(), 0, buffer );
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationFile( h1, &io, &info, sizeof(info), FileBasicInformation )))
    {
        WARN("GetFileInformationByHandle returned error for %s\n", debugstr_w(source));
        HeapFree( GetProcessHeap(), 0, buffer );
        CloseHandle( h1 );
        return FALSE;
    }

    if (!(flags & COPY_FILE_FAIL_IF_EXISTS))
    {
        BOOL same_file = FALSE;
        h2 = CreateFileW( dest, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (h2 != INVALID_HANDLE_VALUE)
        {
            same_file = is_same_file( h1, h2 );
            CloseHandle( h2 );
        }
        if (same_file)
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            CloseHandle( h1 );
            SetLastError( ERROR_SHARING_VIOLATION );
            return FALSE;
        }
    }

    if ((h2 = CreateFileW( dest, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           (flags & COPY_FILE_FAIL_IF_EXISTS) ? CREATE_NEW : CREATE_ALWAYS,
                           info.FileAttributes, h1 )) == INVALID_HANDLE_VALUE)
    {
        WARN("Unable to open dest %s\n", debugstr_w(dest));
        HeapFree( GetProcessHeap(), 0, buffer );
        CloseHandle( h1 );
        return FALSE;
    }

    while (ReadFile( h1, buffer, buffer_size, &count, NULL ) && count)
    {
        char *p = buffer;
        while (count != 0)
        {
            DWORD res;
            if (!WriteFile( h2, p, count, &res, NULL ) || !res) goto done;
            p += res;
            count -= res;
        }
    }
    ret = TRUE;
done:
    /* Maintain the timestamp of source file to destination file and read-only attribute */
    info.FileAttributes &= FILE_ATTRIBUTE_READONLY;
    NtSetInformationFile( h2, &io, &info, sizeof(info), FileBasicInformation );
    HeapFree( GetProcessHeap(), 0, buffer );
    CloseHandle( h1 );
    CloseHandle( h2 );
    if (ret) SetLastError( 0 );
    return ret;
}

/***********************************************************************
 *	CopyFile2   (kernelbase.@)
 */
HRESULT WINAPI CopyFile2( const WCHAR *source, const WCHAR *dest, COPYFILE2_EXTENDED_PARAMETERS *params )
{
    return copy_file(source, dest, params) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}


/***********************************************************************
 *	CopyFileExW   (kernelbase.@)
 */
BOOL WINAPI CopyFileExW( const WCHAR *source, const WCHAR *dest, LPPROGRESS_ROUTINE progress,
                         void *param, BOOL *cancel_ptr, DWORD flags )
{
    COPYFILE2_EXTENDED_PARAMETERS params;

    if (progress)
        FIXME("LPPROGRESS_ROUTINE is not supported\n");
    if (cancel_ptr)
        FIXME("cancel_ptr is not supported\n");

    params.dwSize = sizeof(params);
    params.dwCopyFlags = flags;
    params.pProgressRoutine = NULL;
    params.pvCallbackContext = NULL;
    params.pfCancel = NULL;

    return copy_file( source, dest, &params );
}


/**************************************************************************
 *	CopyFileW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CopyFileW( const WCHAR *source, const WCHAR *dest, BOOL fail_if_exists )
{
    return CopyFileExW( source, dest, NULL, NULL, NULL, fail_if_exists ? COPY_FILE_FAIL_IF_EXISTS : 0 );
}


/***********************************************************************
 *	CreateDirectoryA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryA( LPCSTR path, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return FALSE;
    return CreateDirectoryW( pathW, sa );
}


/***********************************************************************
 *	CreateDirectoryW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryW( LPCWSTR path, LPSECURITY_ATTRIBUTES sa )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io, NULL,
                           FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (status == STATUS_SUCCESS) NtClose( handle );

    RtlFreeUnicodeString( &nt_name );
    return set_ntstatus( status );
}


/***********************************************************************
 *	CreateDirectoryEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryExW( LPCWSTR template, LPCWSTR path,
                                                  LPSECURITY_ATTRIBUTES sa )
{
    return CreateDirectoryW( path, sa );
}


/*************************************************************************
 *	CreateFile2   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFile2( LPCWSTR name, DWORD access, DWORD sharing, DWORD creation,
                                             CREATEFILE2_EXTENDED_PARAMETERS *params )
{
    static const DWORD attributes_mask = FILE_ATTRIBUTE_READONLY |
                                         FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_ARCHIVE |
                                         FILE_ATTRIBUTE_NORMAL |
                                         FILE_ATTRIBUTE_TEMPORARY |
                                         FILE_ATTRIBUTE_OFFLINE |
                                         FILE_ATTRIBUTE_ENCRYPTED |
                                         FILE_ATTRIBUTE_INTEGRITY_STREAM;
    static const DWORD flags_mask = FILE_FLAG_BACKUP_SEMANTICS |
                                    FILE_FLAG_DELETE_ON_CLOSE |
                                    FILE_FLAG_NO_BUFFERING |
                                    FILE_FLAG_OPEN_NO_RECALL |
                                    FILE_FLAG_OPEN_REPARSE_POINT |
                                    FILE_FLAG_OVERLAPPED |
                                    FILE_FLAG_POSIX_SEMANTICS |
                                    FILE_FLAG_RANDOM_ACCESS |
                                    FILE_FLAG_SEQUENTIAL_SCAN |
                                    FILE_FLAG_WRITE_THROUGH;

    LPSECURITY_ATTRIBUTES sa = params ? params->lpSecurityAttributes : NULL;
    HANDLE template = params ? params->hTemplateFile : NULL;
    DWORD attributes = params ? params->dwFileAttributes : 0;
    DWORD flags = params ? params->dwFileFlags : 0;

    TRACE( "%s %#lx %#lx %#lx %p", debugstr_w(name), access, sharing, creation, params );
    if (params) FIXME( "Ignoring extended parameters %p\n", params );

    if (attributes & ~attributes_mask) FIXME( "unsupported attributes %#lx\n", attributes );
    if (flags & ~flags_mask) FIXME( "unsupported flags %#lx\n", flags );
    attributes &= attributes_mask;
    flags &= flags_mask;

    return CreateFileW( name, access, sharing, sa, creation, flags | attributes, template );
}


/*************************************************************************
 *	CreateFileA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileA( LPCSTR name, DWORD access, DWORD sharing,
                                             LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                             DWORD attributes, HANDLE template)
{
    WCHAR *nameW;

    if ((GetVersion() & 0x80000000) && IsBadStringPtrA( name, -1 )) return INVALID_HANDLE_VALUE;
    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_HANDLE_VALUE;
    return CreateFileW( nameW, access, sharing, sa, creation, attributes, template );
}

static UINT get_nt_file_options( DWORD attributes )
{
    UINT options = 0;

    if (attributes & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
        options |= FILE_DELETE_ON_CLOSE;
    if (attributes & FILE_FLAG_NO_BUFFERING)
        options |= FILE_NO_INTERMEDIATE_BUFFERING;
    if (!(attributes & FILE_FLAG_OVERLAPPED))
        options |= FILE_SYNCHRONOUS_IO_NONALERT;
    if (attributes & FILE_FLAG_RANDOM_ACCESS)
        options |= FILE_RANDOM_ACCESS;
    if (attributes & FILE_FLAG_SEQUENTIAL_SCAN)
        options |= FILE_SEQUENTIAL_ONLY;
    if (attributes & FILE_FLAG_WRITE_THROUGH)
        options |= FILE_WRITE_THROUGH;
    return options;
}

/*************************************************************************
 *	CreateFileW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                                             LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                             DWORD attributes, HANDLE template )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    HANDLE ret;
    const WCHAR *vxd_name = NULL;
    SECURITY_QUALITY_OF_SERVICE qos;

    static const UINT nt_disposition[5] =
    {
        FILE_CREATE,        /* CREATE_NEW */
        FILE_OVERWRITE_IF,  /* CREATE_ALWAYS */
        FILE_OPEN,          /* OPEN_EXISTING */
        FILE_OPEN_IF,       /* OPEN_ALWAYS */
        FILE_OVERWRITE      /* TRUNCATE_EXISTING */
    };


    /* sanity checks */

    if (!filename || !filename[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    TRACE( "%s %s%s%s%s%s%s%s creation %ld attributes 0x%lx\n", debugstr_w(filename),
           (access & GENERIC_READ) ? "GENERIC_READ " : "",
           (access & GENERIC_WRITE) ? "GENERIC_WRITE " : "",
           (access & GENERIC_EXECUTE) ? "GENERIC_EXECUTE " : "",
           !access ? "QUERY_ACCESS " : "",
           (sharing & FILE_SHARE_READ) ? "FILE_SHARE_READ " : "",
           (sharing & FILE_SHARE_WRITE) ? "FILE_SHARE_WRITE " : "",
           (sharing & FILE_SHARE_DELETE) ? "FILE_SHARE_DELETE " : "",
           creation, attributes);

    if ((GetVersion() & 0x80000000) && !wcsncmp( filename, L"\\\\.\\", 4 ) &&
        !RtlIsDosDeviceName_U( filename + 4 ) &&
        wcsnicmp( filename + 4, L"PIPE\\", 5 ) &&
        wcsnicmp( filename + 4, L"MAILSLOT\\", 9 ))
    {
        vxd_name = filename + 4;
        if (!creation) creation = OPEN_EXISTING;
    }

    if (creation < CREATE_NEW || creation > TRUNCATE_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    /* now call NtCreateFile */

    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
        access |= DELETE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    if (attributes & SECURITY_SQOS_PRESENT)
    {
        qos.Length = sizeof(qos);
        qos.ImpersonationLevel = (attributes >> 16) & 0x3;
        qos.ContextTrackingMode = attributes & SECURITY_CONTEXT_TRACKING ? SECURITY_DYNAMIC_TRACKING : SECURITY_STATIC_TRACKING;
        qos.EffectiveOnly = (attributes & SECURITY_EFFECTIVE_ONLY) != 0;
        attr.SecurityQualityOfService = &qos;
    }
    else
        attr.SecurityQualityOfService = NULL;

    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    status = NtCreateFile( &ret, access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &io,
                           NULL, attributes & FILE_ATTRIBUTE_VALID_FLAGS, sharing,
                           nt_disposition[creation - CREATE_NEW],
                           get_nt_file_options( attributes ), NULL, 0 );
    if (status)
    {
        if (vxd_name && vxd_name[0])
        {
            static HANDLE (*vxd_open)(LPCWSTR,DWORD,SECURITY_ATTRIBUTES*);
            if (!vxd_open) vxd_open = (void *)GetProcAddress( GetModuleHandleW(L"krnl386.exe16"),
                                                              "__wine_vxd_open" );
            if (vxd_open && (ret = vxd_open( vxd_name, access, sa ))) goto done;
        }

        WARN("Unable to create file %s (status %lx)\n", debugstr_w(filename), status);
        ret = INVALID_HANDLE_VALUE;

        /* In the case file creation was rejected due to CREATE_NEW flag
         * was specified and file with that name already exists, correct
         * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
         * Note: RtlNtStatusToDosError is not the subject to blame here.
         */
        if (status == STATUS_OBJECT_NAME_COLLISION)
            SetLastError( ERROR_FILE_EXISTS );
        else
            SetLastError( RtlNtStatusToDosError(status) );
    }
    else
    {
        if ((creation == CREATE_ALWAYS && io.Information == FILE_OVERWRITTEN) ||
            (creation == OPEN_ALWAYS && io.Information == FILE_OPENED))
            SetLastError( ERROR_ALREADY_EXISTS );
        else
            SetLastError( 0 );
    }
    RtlFreeUnicodeString( &nameW );

 done:
    if (!ret) ret = INVALID_HANDLE_VALUE;
    TRACE("returning %p\n", ret);
    return ret;
}


/*************************************************************************
 *	CreateHardLinkA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateHardLinkA( const char *dest, const char *source,
                                               SECURITY_ATTRIBUTES *attr )
{
    WCHAR *sourceW, *destW;
    BOOL res;

    if (!(sourceW = file_name_AtoW( source, TRUE ))) return FALSE;
    if (!(destW = file_name_AtoW( dest, TRUE )))
    {
        HeapFree( GetProcessHeap(), 0, sourceW );
        return FALSE;
    }
    res = CreateHardLinkW( destW, sourceW, attr );
    HeapFree( GetProcessHeap(), 0, sourceW );
    HeapFree( GetProcessHeap(), 0, destW );
    return res;
}


/*************************************************************************
 *	CreateHardLinkW   (kernelbase.@)
 */
BOOL WINAPI CreateHardLinkW( LPCWSTR dest, LPCWSTR source, SECURITY_ATTRIBUTES *sec_attr )
{
    UNICODE_STRING ntDest, ntSource;
    FILE_LINK_INFORMATION *info = NULL;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    BOOL ret = FALSE;
    HANDLE file;
    ULONG size;

    TRACE( "(%s, %s, %p)\n", debugstr_w(dest), debugstr_w(source), sec_attr );

    ntDest.Buffer = ntSource.Buffer = NULL;
    if (!RtlDosPathNameToNtPathName_U( dest, &ntDest, NULL, NULL ) ||
        !RtlDosPathNameToNtPathName_U( source, &ntSource, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto done;
    }

    size = offsetof( FILE_LINK_INFORMATION, FileName ) + ntDest.Length;
    if (!(info = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto done;
    }

    InitializeObjectAttributes( &attr, &ntSource, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!(ret = set_ntstatus( NtOpenFile( &file, SYNCHRONIZE, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_SYNCHRONOUS_IO_NONALERT ) )))
        goto done;

    info->ReplaceIfExists = FALSE;
    info->RootDirectory = NULL;
    info->FileNameLength = ntDest.Length;
    memcpy( info->FileName, ntDest.Buffer, ntDest.Length );
    ret = set_ntstatus( NtSetInformationFile( file, &io, info, size, FileLinkInformation ) );
    NtClose( file );

done:
    RtlFreeUnicodeString( &ntSource );
    RtlFreeUnicodeString( &ntDest );
    HeapFree( GetProcessHeap(), 0, info );
    return ret;
}


/*************************************************************************
 *	CreateSymbolicLinkW   (kernelbase.@)
 */
BOOLEAN WINAPI /* DECLSPEC_HOTPATCH */ CreateSymbolicLinkW( LPCWSTR link, LPCWSTR target, DWORD flags )
{
    FIXME( "(%s %s %ld): stub\n", debugstr_w(link), debugstr_w(target), flags );
    return TRUE;
}


/***********************************************************************
 *	DeleteFileA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileA( LPCSTR path )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return FALSE;
    return DeleteFileW( pathW );
}


/***********************************************************************
 *	DeleteFileW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileW( LPCWSTR path )
{
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE hFile;
    IO_STATUS_BLOCK io;

    TRACE( "%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile(&hFile, SYNCHRONIZE | DELETE, &attr, &io, NULL, 0,
			  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			  FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0);
    if (status == STATUS_SUCCESS) status = NtClose(hFile);

    RtlFreeUnicodeString( &nameW );
    return set_ntstatus( status );
}


/****************************************************************************
 *	FindCloseChangeNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindCloseChangeNotification( HANDLE handle )
{
    return CloseHandle( handle );
}


/****************************************************************************
 *	FindFirstChangeNotificationA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstChangeNotificationA( LPCSTR path, BOOL subtree, DWORD filter )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return INVALID_HANDLE_VALUE;
    return FindFirstChangeNotificationW( pathW, subtree, filter );
}


/*
 * NtNotifyChangeDirectoryFile may write back to the IO_STATUS_BLOCK
 * asynchronously.  We don't care about the contents, but it can't
 * be placed on the stack since it will go out of scope when we return.
 */
static IO_STATUS_BLOCK dummy_iosb;

/****************************************************************************
 *	FindFirstChangeNotificationW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstChangeNotificationW( LPCWSTR path, BOOL subtree, DWORD filter )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE handle = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %lx\n", debugstr_w(path), subtree, filter );

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return handle;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attr, &dummy_iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (!set_ntstatus( status )) return INVALID_HANDLE_VALUE;

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &dummy_iosb, NULL, 0, filter, subtree );
    if (status != STATUS_PENDING)
    {
        NtClose( handle );
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}


/****************************************************************************
 *	FindNextChangeNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindNextChangeNotification( HANDLE handle )
{
    NTSTATUS status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &dummy_iosb,
                                                   NULL, 0, FILE_NOTIFY_CHANGE_SIZE, 0 );
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/******************************************************************************
 *	FindFirstFileExA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileExA( const char *filename, FINDEX_INFO_LEVELS level,
                                                  void *data, FINDEX_SEARCH_OPS search_op,
                                                  void *filter, DWORD flags )
{
    HANDLE handle;
    WIN32_FIND_DATAA *dataA = data;
    WIN32_FIND_DATAW dataW;
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( filename, FALSE ))) return INVALID_HANDLE_VALUE;

    handle = FindFirstFileExW( nameW, level, &dataW, search_op, filter, flags );
    if (handle == INVALID_HANDLE_VALUE) return handle;

    dataA->dwFileAttributes = dataW.dwFileAttributes;
    dataA->ftCreationTime   = dataW.ftCreationTime;
    dataA->ftLastAccessTime = dataW.ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW.ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW.nFileSizeHigh;
    dataA->nFileSizeLow     = dataW.nFileSizeLow;
    file_name_WtoA( dataW.cFileName, -1, dataA->cFileName, sizeof(dataA->cFileName) );
    file_name_WtoA( dataW.cAlternateFileName, -1, dataA->cAlternateFileName,
                    sizeof(dataA->cAlternateFileName) );
    return handle;
}


/***********************************************************************
 *     fixup_mask
 *
 * Fixup mask with wildcards for use with NtQueryDirectoryFile().
 */
static WCHAR *fixup_mask( const WCHAR *mask )
{
    unsigned int len = lstrlenW( mask ), i;
    BOOL no_ext;
    WCHAR *ret;

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(*mask) ))) return NULL;
    memcpy( ret, mask, (len + 1) * sizeof(*mask) );
    if (!len) return ret;
    no_ext = ret[len - 1] == '.';
    while (len && (ret[len - 1] == '.' || ret[len - 1] == ' ')) --len;

    for (i = 0; i < len; ++i)
    {
        if (ret[i] == '.' && (ret[i + 1] == '*' || ret[i + 1] == '?')) ret[i] = '\"';
        else if (ret[i] == '?')                                        ret[i] = '>';
    }
    ret[len] = 0;
    if (no_ext && len && ret[len - 1] == '*') ret[len - 1] = '<';
    return ret;
}


/******************************************************************************
 *	FindFirstFileExW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileExW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                                  LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                  LPVOID filter, DWORD flags )
{
    WCHAR *mask;
    BOOL has_wildcard = FALSE;
    FIND_FIRST_INFO *info = NULL;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD size, device = 0;

    TRACE( "%s %d %p %d %p %lx\n", debugstr_w(filename), level, data, search_op, filter, flags );

    if (flags & ~FIND_FIRST_EX_LARGE_FETCH)
    {
        FIXME("flags not implemented 0x%08lx\n", flags );
    }
    if (search_op != FindExSearchNameMatch && search_op != FindExSearchLimitToDirectories)
    {
        FIXME( "search_op not implemented 0x%08x\n", search_op );
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }
    if (level != FindExInfoStandard && level != FindExInfoBasic)
    {
        FIXME("info level %d not implemented\n", level );
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nt_name, &mask, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    if (!mask && (device = RtlIsDosDeviceName_U( filename )))
    {
        WCHAR *dir = NULL;

        /* we still need to check that the directory can be opened */

        if (HIWORD(device))
        {
            if (!(dir = HeapAlloc( GetProcessHeap(), 0, HIWORD(device) + sizeof(WCHAR) )))
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                goto error;
            }
            memcpy( dir, filename, HIWORD(device) );
            dir[HIWORD(device)/sizeof(WCHAR)] = 0;
        }
        RtlFreeUnicodeString( &nt_name );
        if (!RtlDosPathNameToNtPathName_U( dir ? dir : L".", &nt_name, &mask, NULL ))
        {
            HeapFree( GetProcessHeap(), 0, dir );
            SetLastError( ERROR_PATH_NOT_FOUND );
            goto error;
        }
        HeapFree( GetProcessHeap(), 0, dir );
        size = 0;
    }
    else if (!mask || !*mask)
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        goto error;
    }
    else
    {
        nt_name.Length = (mask - nt_name.Buffer) * sizeof(WCHAR);
        has_wildcard = wcspbrk( mask, L"*?<>" ) != NULL;
        if (has_wildcard)
        {
            size = 8192;
            mask = PathFindFileNameW( filename );
        }
        else size = max_entry_size;
    }

    if (!(info = HeapAlloc( GetProcessHeap(), 0, offsetof( FIND_FIRST_INFO, data[size] ))))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto error;
    }

    /* check if path is the root of the drive, skipping the \??\ prefix */
    info->is_root = FALSE;
    if (nt_name.Length >= 6 * sizeof(WCHAR) && nt_name.Buffer[5] == ':')
    {
        DWORD pos = 6;
        while (pos * sizeof(WCHAR) < nt_name.Length && nt_name.Buffer[pos] == '\\') pos++;
        info->is_root = (pos * sizeof(WCHAR) >= nt_name.Length);
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &info->handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );
    if (status != STATUS_SUCCESS)
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            SetLastError( ERROR_PATH_NOT_FOUND );
        else
            SetLastError( RtlNtStatusToDosError(status) );
        goto error;
    }

    RtlInitializeCriticalSectionEx( &info->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    info->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FIND_FIRST_INFO.cs");
    info->path      = nt_name;
    info->magic     = FIND_FIRST_MAGIC;
    info->data_pos  = 0;
    info->data_len  = 0;
    info->data_size = size;
    info->search_op = search_op;
    info->level     = level;

    if (device)
    {
        WIN32_FIND_DATAW *wfd = data;

        memset( wfd, 0, sizeof(*wfd) );
        memcpy( wfd->cFileName, filename + HIWORD(device)/sizeof(WCHAR), LOWORD(device) );
        wfd->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
        CloseHandle( info->handle );
        info->handle = 0;
    }
    else
    {
        WCHAR *fixedup_mask = mask;
        UNICODE_STRING mask_str;

        if (has_wildcard && !(fixedup_mask = fixup_mask( mask ))) status = STATUS_NO_MEMORY;
        else
        {
            RtlInitUnicodeString( &mask_str, fixedup_mask );
            status = NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, info->data_size,
                                           FileBothDirectoryInformation, FALSE, &mask_str, TRUE );
        }
        if (fixedup_mask != mask) HeapFree( GetProcessHeap(), 0, fixedup_mask );
        if (status)
        {
            FindClose( info );
            SetLastError( RtlNtStatusToDosError( status ) );
            return INVALID_HANDLE_VALUE;
        }

        info->data_len = io.Information;
        if (!has_wildcard) info->data_size = 0;  /* we read everything */

        if (!FindNextFileW( info, data ))
        {
            TRACE( "%s not found\n", debugstr_w(filename) );
            FindClose( info );
            SetLastError( ERROR_FILE_NOT_FOUND );
            return INVALID_HANDLE_VALUE;
        }
        if (!has_wildcard)  /* we can't find two files with the same name */
        {
            CloseHandle( info->handle );
            info->handle = 0;
        }
    }
    return info;

error:
    HeapFree( GetProcessHeap(), 0, info );
    RtlFreeUnicodeString( &nt_name );
    return INVALID_HANDLE_VALUE;
}


/******************************************************************************
 *	FindFirstFileA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileA( const char *filename, WIN32_FIND_DATAA *data )
{
    return FindFirstFileExA( filename, FindExInfoStandard, data, FindExSearchNameMatch, NULL, 0 );
}


/******************************************************************************
 *	FindFirstFileW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileW( const WCHAR *filename, WIN32_FIND_DATAW *data )
{
    return FindFirstFileExW( filename, FindExInfoStandard, data, FindExSearchNameMatch, NULL, 0 );
}

/******************************************************************************
 *     FindFirstFileNameW   (kernelbase.@)
 */
HANDLE WINAPI FindFirstFileNameW( const WCHAR *file_name, DWORD flags, DWORD *len, WCHAR *link_name )
{
    FIXME( "(%s, %lu, %p, %p): stub!\n", debugstr_w(file_name), flags, len, link_name );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return INVALID_HANDLE_VALUE;
}

/**************************************************************************
 *	FindFirstStreamW   (kernelbase.@)
 */
HANDLE WINAPI FindFirstStreamW( const WCHAR *filename, STREAM_INFO_LEVELS level, void *data, DWORD flags )
{
    FIXME("(%s, %d, %p, %lx): stub!\n", debugstr_w(filename), level, data, flags);
    SetLastError( ERROR_HANDLE_EOF );
    return INVALID_HANDLE_VALUE;
}


/******************************************************************************
 *	FindNextFileA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindNextFileA( HANDLE handle, WIN32_FIND_DATAA *data )
{
    WIN32_FIND_DATAW dataW;

    if (!FindNextFileW( handle, &dataW )) return FALSE;
    data->dwFileAttributes = dataW.dwFileAttributes;
    data->ftCreationTime   = dataW.ftCreationTime;
    data->ftLastAccessTime = dataW.ftLastAccessTime;
    data->ftLastWriteTime  = dataW.ftLastWriteTime;
    data->nFileSizeHigh    = dataW.nFileSizeHigh;
    data->nFileSizeLow     = dataW.nFileSizeLow;
    file_name_WtoA( dataW.cFileName, -1, data->cFileName, sizeof(data->cFileName) );
    file_name_WtoA( dataW.cAlternateFileName, -1, data->cAlternateFileName,
                    sizeof(data->cAlternateFileName) );
    return TRUE;
}


/******************************************************************************
 *	FindNextFileW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindNextFileW( HANDLE handle, WIN32_FIND_DATAW *data )
{
    FIND_FIRST_INFO *info = handle;
    FILE_BOTH_DIR_INFORMATION *dir_info;
    BOOL ret = FALSE;
    NTSTATUS status;

    TRACE( "%p %p\n", handle, data );

    if (!handle || handle == INVALID_HANDLE_VALUE || info->magic != FIND_FIRST_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }

    RtlEnterCriticalSection( &info->cs );

    if (!info->handle) SetLastError( ERROR_NO_MORE_FILES );
    else for (;;)
    {
        if (info->data_pos >= info->data_len)  /* need to read some more data */
        {
            IO_STATUS_BLOCK io;

            if (info->data_size)
                status = NtQueryDirectoryFile( info->handle, 0, NULL, NULL, &io, info->data, info->data_size,
                                               FileBothDirectoryInformation, FALSE, NULL, FALSE );
            else
                status = STATUS_NO_MORE_FILES;

            if (!set_ntstatus( status ))
            {
                if (status == STATUS_NO_MORE_FILES)
                {
                    CloseHandle( info->handle );
                    info->handle = 0;
                }
                break;
            }
            info->data_len = io.Information;
            info->data_pos = 0;
        }

        dir_info = (FILE_BOTH_DIR_INFORMATION *)(info->data + info->data_pos);

        if (dir_info->NextEntryOffset) info->data_pos += dir_info->NextEntryOffset;
        else info->data_pos = info->data_len;

        /* don't return '.' and '..' in the root of the drive */
        if (info->is_root)
        {
            const WCHAR *file_name = dir_info->FileName;
            if (dir_info->FileNameLength == sizeof(WCHAR) && file_name[0] == '.') continue;
            if (dir_info->FileNameLength == 2 * sizeof(WCHAR) &&
                file_name[0] == '.' && file_name[1] == '.') continue;
        }

        data->dwFileAttributes = dir_info->FileAttributes;
        data->ftCreationTime   = *(FILETIME *)&dir_info->CreationTime;
        data->ftLastAccessTime = *(FILETIME *)&dir_info->LastAccessTime;
        data->ftLastWriteTime  = *(FILETIME *)&dir_info->LastWriteTime;
        data->nFileSizeHigh    = dir_info->EndOfFile.QuadPart >> 32;
        data->nFileSizeLow     = (DWORD)dir_info->EndOfFile.QuadPart;
        data->dwReserved0      = 0;
        data->dwReserved1      = 0;

        memcpy( data->cFileName, dir_info->FileName, dir_info->FileNameLength );
        data->cFileName[dir_info->FileNameLength/sizeof(WCHAR)] = 0;

        if (info->level != FindExInfoBasic)
        {
            memcpy( data->cAlternateFileName, dir_info->ShortName, dir_info->ShortNameLength );
            data->cAlternateFileName[dir_info->ShortNameLength/sizeof(WCHAR)] = 0;
        }
        else
            data->cAlternateFileName[0] = 0;

        TRACE( "returning %s (%s)\n",
               debugstr_w(data->cFileName), debugstr_w(data->cAlternateFileName) );

        ret = TRUE;
        break;
    }

    RtlLeaveCriticalSection( &info->cs );
    return ret;
}


/**************************************************************************
 *	FindNextStreamW   (kernelbase.@)
 */
BOOL WINAPI FindNextStreamW( HANDLE handle, void *data )
{
    FIXME( "(%p, %p): stub!\n", handle, data );
    SetLastError( ERROR_HANDLE_EOF );
    return FALSE;
}


/******************************************************************************
 *	FindClose   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindClose( HANDLE handle )
{
    FIND_FIRST_INFO *info = handle;

    if (!handle || handle == INVALID_HANDLE_VALUE)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    __TRY
    {
        if (info->magic == FIND_FIRST_MAGIC)
        {
            RtlEnterCriticalSection( &info->cs );
            if (info->magic == FIND_FIRST_MAGIC)  /* in case someone else freed it in the meantime */
            {
                info->magic = 0;
                if (info->handle) CloseHandle( info->handle );
                info->handle = 0;
                RtlFreeUnicodeString( &info->path );
                info->data_pos = 0;
                info->data_len = 0;
                RtlLeaveCriticalSection( &info->cs );
                info->cs.DebugInfo->Spare[0] = 0;
                RtlDeleteCriticalSection( &info->cs );
                HeapFree( GetProcessHeap(), 0, info );
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        WARN( "illegal handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    __ENDTRY

    return TRUE;
}


/******************************************************************************
 *	GetCompressedFileSizeA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetCompressedFileSizeA( LPCSTR name, LPDWORD size_high )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_FILE_SIZE;
    return GetCompressedFileSizeW( nameW, size_high );
}


/******************************************************************************
 *	GetCompressedFileSizeW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetCompressedFileSizeW( LPCWSTR name, LPDWORD size_high )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    DWORD ret;

    TRACE("%s %p\n", debugstr_w(name), size_high);

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_SIZE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );
    if (!set_ntstatus( status )) return INVALID_FILE_SIZE;

    /* we don't support compressed files, simply return the file size */
    ret = GetFileSize( handle, size_high );
    NtClose( handle );
    return ret;
}


/***********************************************************************
 *           GetCurrentDirectoryA    (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (buflen && buf && ((ULONG_PTR)buf >> 16) == 0)
    {
        /* Win9x catches access violations here, returning zero.
         * This behaviour resulted in some people not noticing
         * that they got the argument order wrong. So let's be
         * nice and fail gracefully if buf is invalid and looks
         * more like a buflen. */
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    ret = RtlGetCurrentDirectory_U( sizeof(bufferW), bufferW );
    if (!ret) return 0;
    if (ret > sizeof(bufferW))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( bufferW, buf, buflen );
}


/***********************************************************************
 *           GetCurrentDirectoryW    (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    return RtlGetCurrentDirectory_U( buflen * sizeof(WCHAR), buf ) / sizeof(WCHAR);
}


/**************************************************************************
 *	GetFileAttributesA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_FILE_ATTRIBUTES;
    return GetFileAttributesW( nameW );
}


/**************************************************************************
 *	GetFileAttributesW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesW( LPCWSTR name )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE( "%s\n", debugstr_w(name) );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_ATTRIBUTES;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS) return info.FileAttributes;

    /* NtQueryAttributesFile fails on devices, but GetFileAttributesW succeeds */
    if (RtlIsDosDeviceName_U( name )) return FILE_ATTRIBUTE_ARCHIVE;

    SetLastError( RtlNtStatusToDosError(status) );
    return INVALID_FILE_ATTRIBUTES;
}


/**************************************************************************
 *	GetFileAttributesExA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileAttributesExA( LPCSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return FALSE;
    return GetFileAttributesExW( nameW, level, ptr );
}


/**************************************************************************
 *	GetFileAttributesExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileAttributesExW( LPCWSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr )
{
    FILE_NETWORK_OPEN_INFORMATION info;
    WIN32_FILE_ATTRIBUTE_DATA *data = ptr;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE("%s %d %p\n", debugstr_w(name), level, ptr);

    if (level != GetFileExInfoStandard)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryFullAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );
    if (!set_ntstatus( status )) return FALSE;

    data->dwFileAttributes                = info.FileAttributes;
    data->ftCreationTime.dwLowDateTime    = info.CreationTime.u.LowPart;
    data->ftCreationTime.dwHighDateTime   = info.CreationTime.u.HighPart;
    data->ftLastAccessTime.dwLowDateTime  = info.LastAccessTime.u.LowPart;
    data->ftLastAccessTime.dwHighDateTime = info.LastAccessTime.u.HighPart;
    data->ftLastWriteTime.dwLowDateTime   = info.LastWriteTime.u.LowPart;
    data->ftLastWriteTime.dwHighDateTime  = info.LastWriteTime.u.HighPart;
    data->nFileSizeLow                    = info.EndOfFile.u.LowPart;
    data->nFileSizeHigh                   = info.EndOfFile.u.HighPart;
    return TRUE;
}


/***********************************************************************
 *	GetFinalPathNameByHandleA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFinalPathNameByHandleA( HANDLE file, LPSTR path,
                                                          DWORD count, DWORD flags )
{
    WCHAR *str;
    DWORD result, len;

    TRACE( "(%p,%p,%ld,%lx)\n", file, path, count, flags);

    len = GetFinalPathNameByHandleW(file, NULL, 0, flags);
    if (len == 0) return 0;

    str = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!str)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    result = GetFinalPathNameByHandleW(file, str, len, flags);
    if (result != len - 1)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return 0;
    }

    len = file_name_WtoA( str, -1, NULL, 0 );
    if (count < len)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return len - 1;
    }
    file_name_WtoA( str, -1, path, count );
    HeapFree(GetProcessHeap(), 0, str);
    return len - 1;
}


/***********************************************************************
 *	GetFinalPathNameByHandleW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFinalPathNameByHandleW( HANDLE file, LPWSTR path,
                                                          DWORD count, DWORD flags )
{
    WCHAR buffer[sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH + 1];
    OBJECT_NAME_INFORMATION *info = (OBJECT_NAME_INFORMATION*)&buffer;
    WCHAR drive_part[MAX_PATH];
    DWORD drive_part_len = 0;
    NTSTATUS status;
    DWORD result = 0;
    ULONG dummy;
    WCHAR *ptr;

    TRACE( "(%p,%p,%ld,%lx)\n", file, path, count, flags );

    if (flags & ~(FILE_NAME_OPENED | VOLUME_NAME_GUID | VOLUME_NAME_NONE | VOLUME_NAME_NT))
    {
        WARN("Unknown flags: %lx\n", flags);
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* get object name */
    status = NtQueryObject( file, ObjectNameInformation, &buffer, sizeof(buffer) - sizeof(WCHAR), &dummy );
    if (!set_ntstatus( status )) return 0;

    if (!info->Name.Buffer)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }
    if (info->Name.Length < 4 * sizeof(WCHAR) || info->Name.Buffer[0] != '\\' ||
        info->Name.Buffer[1] != '?' || info->Name.Buffer[2] != '?' || info->Name.Buffer[3] != '\\' )
    {
        FIXME("Unexpected object name: %s\n", debugstr_wn(info->Name.Buffer, info->Name.Length / sizeof(WCHAR)));
        SetLastError( ERROR_GEN_FAILURE );
        return 0;
    }

    /* add terminating null character, remove "\\??\\" */
    info->Name.Buffer[info->Name.Length / sizeof(WCHAR)] = 0;
    info->Name.Length -= 4 * sizeof(WCHAR);
    info->Name.Buffer += 4;

    /* FILE_NAME_OPENED is not supported yet, and would require Wineserver changes */
    if (flags & FILE_NAME_OPENED)
    {
        FIXME("FILE_NAME_OPENED not supported\n");
        flags &= ~FILE_NAME_OPENED;
    }

    /* Get information required for VOLUME_NAME_NONE, VOLUME_NAME_GUID and VOLUME_NAME_NT */
    if (flags == VOLUME_NAME_NONE || flags == VOLUME_NAME_GUID || flags == VOLUME_NAME_NT)
    {
        if (!GetVolumePathNameW( info->Name.Buffer, drive_part, MAX_PATH )) return 0;
        drive_part_len = lstrlenW(drive_part);
        if (!drive_part_len || drive_part_len > lstrlenW(info->Name.Buffer) ||
            drive_part[drive_part_len-1] != '\\' ||
            CompareStringOrdinal( info->Name.Buffer, drive_part_len, drive_part, drive_part_len, TRUE ) != CSTR_EQUAL)
        {
            FIXME( "Path %s returned by GetVolumePathNameW does not match file path %s\n",
                   debugstr_w(drive_part), debugstr_w(info->Name.Buffer) );
            SetLastError( ERROR_GEN_FAILURE );
            return 0;
        }
    }

    if (flags == VOLUME_NAME_NONE)
    {
        ptr = info->Name.Buffer + drive_part_len - 1;
        result = lstrlenW(ptr);
        if (result < count) memcpy(path, ptr, (result + 1) * sizeof(WCHAR));
        else result++;
    }
    else if (flags == VOLUME_NAME_GUID)
    {
        WCHAR volume_prefix[51];

        /* GetVolumeNameForVolumeMountPointW sets error code on failure */
        if (!GetVolumeNameForVolumeMountPointW( drive_part, volume_prefix, 50 )) return 0;
        ptr = info->Name.Buffer + drive_part_len;
        result = lstrlenW(volume_prefix) + lstrlenW(ptr);
        if (result < count)
        {
            lstrcpyW(path, volume_prefix);
            lstrcatW(path, ptr);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else if (flags == VOLUME_NAME_NT)
    {
        WCHAR nt_prefix[MAX_PATH];

        /* QueryDosDeviceW sets error code on failure */
        drive_part[drive_part_len - 1] = 0;
        if (!QueryDosDeviceW( drive_part, nt_prefix, MAX_PATH )) return 0;
        ptr = info->Name.Buffer + drive_part_len - 1;
        result = lstrlenW(nt_prefix) + lstrlenW(ptr);
        if (result < count)
        {
            lstrcpyW(path, nt_prefix);
            lstrcatW(path, ptr);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else if (flags == VOLUME_NAME_DOS)
    {
        result = 4 + lstrlenW(info->Name.Buffer);
        if (result < count)
        {
            lstrcpyW(path, L"\\\\?\\");
            lstrcatW(path, info->Name.Buffer);
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result++;
        }
    }
    else
    {
        /* Windows crashes here, but we prefer returning ERROR_INVALID_PARAMETER */
        WARN("Invalid combination of flags: %lx\n", flags);
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    return result;
}


/***********************************************************************
 *	GetFullPathNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFullPathNameA( LPCSTR name, DWORD len, LPSTR buffer, LPSTR *lastpart )
{
    WCHAR *nameW;
    WCHAR bufferW[MAX_PATH], *lastpartW = NULL;
    DWORD ret;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return 0;

    ret = GetFullPathNameW( nameW, MAX_PATH, bufferW, &lastpartW );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, len );
    if (ret < len && lastpart)
    {
        if (lastpartW)
            *lastpart = buffer + file_name_WtoA( bufferW, lastpartW - bufferW, NULL, 0 );
        else
            *lastpart = NULL;
    }
    return ret;
}


/***********************************************************************
 *	GetFullPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFullPathNameW( LPCWSTR name, DWORD len, LPWSTR buffer, LPWSTR *lastpart )
{
    return RtlGetFullPathName_U( name, len * sizeof(WCHAR), buffer, lastpart ) / sizeof(WCHAR);
}


/***********************************************************************
 *	GetLongPathNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetLongPathNameA( LPCSTR shortpath, LPSTR longpath, DWORD longlen )
{
    WCHAR *shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret;

    TRACE( "%s\n", debugstr_a( shortpath ));

    if (!(shortpathW = file_name_AtoW( shortpath, FALSE ))) return 0;

    ret = GetLongPathNameW( shortpathW, longpathW, MAX_PATH );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( longpathW, longpath, longlen );
}


/***********************************************************************
 *	GetLongPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
    WCHAR tmplongpath[1024];
    DWORD sp = 0, lp = 0, tmplen;
    WIN32_FIND_DATAW wfd;
    UNICODE_STRING nameW;
    LPCWSTR p;
    HANDLE handle;

    TRACE("%s,%p,%lu\n", debugstr_w(shortpath), longpath, longlen);

    if (!shortpath)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!shortpath[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return 0;
    }

    if (shortpath[0] == '\\' && shortpath[1] == '\\')
    {
        FIXME( "UNC pathname %s\n", debugstr_w(shortpath) );
        tmplen = lstrlenW( shortpath );
        if (tmplen < longlen)
        {
            if (longpath != shortpath) lstrcpyW( longpath, shortpath );
            return tmplen;
        }
        return tmplen + 1;
    }

    /* check for drive letter */
    if (shortpath[0] != '/' && shortpath[1] == ':' )
    {
        tmplongpath[0] = shortpath[0];
        tmplongpath[1] = ':';
        lp = sp = 2;
    }

    if (wcspbrk( shortpath + sp, L"*?" ))
    {
        SetLastError( ERROR_INVALID_NAME );
        return 0;
    }

    while (shortpath[sp])
    {
        /* check for path delimiters and reproduce them */
        if (shortpath[sp] == '\\' || shortpath[sp] == '/')
        {
            tmplongpath[lp++] = shortpath[sp++];
            tmplongpath[lp] = 0; /* terminate string */
            continue;
        }

        for (p = shortpath + sp; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (shortpath + sp);
        lstrcpynW( tmplongpath + lp, shortpath + sp, tmplen + 1 );

        if (tmplongpath[lp] == '.')
        {
            if (tmplen == 1 || (tmplen == 2 && tmplongpath[lp + 1] == '.'))
            {
                lp += tmplen;
                sp += tmplen;
                continue;
            }
        }

        /* Check if the file exists */
        handle = FindFirstFileW( tmplongpath, &wfd );
        if (handle == INVALID_HANDLE_VALUE)
        {
            TRACE( "not found %s\n", debugstr_w( tmplongpath ));
            SetLastError ( ERROR_FILE_NOT_FOUND );
            return 0;
        }
        FindClose( handle );

        /* Use the existing file name if it's a short name */
        RtlInitUnicodeString( &nameW, tmplongpath + lp );
        if (RtlIsNameLegalDOS8Dot3( &nameW, NULL, NULL )) lstrcpyW( tmplongpath + lp, wfd.cFileName );
        lp += lstrlenW( tmplongpath + lp );
        sp += tmplen;
    }
    tmplen = lstrlenW( shortpath ) - 1;
    if ((shortpath[tmplen] == '/' || shortpath[tmplen] == '\\') &&
        (tmplongpath[lp - 1] != '/' && tmplongpath[lp - 1] != '\\'))
        tmplongpath[lp++] = shortpath[tmplen];
    tmplongpath[lp] = 0;

    tmplen = lstrlenW( tmplongpath ) + 1;
    if (tmplen <= longlen)
    {
        lstrcpyW( longpath, tmplongpath );
        TRACE("returning %s\n", debugstr_w( longpath ));
        tmplen--; /* length without 0 */
    }
    return tmplen;
}


/***********************************************************************
 *	GetShortPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetShortPathNameW( LPCWSTR longpath, LPWSTR shortpath, DWORD shortlen )
{
    WIN32_FIND_DATAW wfd;
    WCHAR *tmpshortpath;
    HANDLE handle;
    LPCWSTR p;
    DWORD sp = 0, lp = 0, tmplen, buf_len;

    TRACE( "%s,%p,%lu\n", debugstr_w(longpath), shortpath, shortlen );

    if (!longpath)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!longpath[0])
    {
        SetLastError( ERROR_BAD_PATHNAME );
        return 0;
    }

    /* code below only removes characters from string, never adds, so this is
     * the largest buffer that tmpshortpath will need to have */
    buf_len = lstrlenW(longpath) + 1;
    tmpshortpath = HeapAlloc( GetProcessHeap(), 0, buf_len * sizeof(WCHAR) );
    if (!tmpshortpath)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    if (longpath[0] == '\\' && longpath[1] == '\\' && longpath[2] == '?' && longpath[3] == '\\')
    {
        memcpy( tmpshortpath, longpath, 4 * sizeof(WCHAR) );
        sp = lp = 4;
    }

    if (wcspbrk( longpath + lp, L"*?" ))
    {
        HeapFree( GetProcessHeap(), 0, tmpshortpath );
        SetLastError( ERROR_INVALID_NAME );
        return 0;
    }

    /* check for drive letter */
    if (longpath[lp] != '/' && longpath[lp + 1] == ':' )
    {
        tmpshortpath[sp] = longpath[lp];
        tmpshortpath[sp + 1] = ':';
        sp += 2;
        lp += 2;
    }

    while (longpath[lp])
    {
        /* check for path delimiters and reproduce them */
        if (longpath[lp] == '\\' || longpath[lp] == '/')
        {
            tmpshortpath[sp++] = longpath[lp++];
            tmpshortpath[sp] = 0; /* terminate string */
            continue;
        }

        p = longpath + lp;
        for (; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (longpath + lp);
        lstrcpynW( tmpshortpath + sp, longpath + lp, tmplen + 1 );

        if (tmpshortpath[sp] == '.')
        {
            if (tmplen == 1 || (tmplen == 2 && tmpshortpath[sp + 1] == '.'))
            {
                sp += tmplen;
                lp += tmplen;
                continue;
            }
        }

        /* Check if the file exists and use the existing short file name */
        handle = FindFirstFileW( tmpshortpath, &wfd );
        if (handle == INVALID_HANDLE_VALUE) goto notfound;
        FindClose( handle );

        /* In rare cases (like "a.abcd") short path may be longer than original path.
         * Make sure we have enough space in temp buffer. */
        if (wfd.cAlternateFileName[0] && tmplen < lstrlenW(wfd.cAlternateFileName))
        {
            WCHAR *new_buf;
            buf_len += lstrlenW( wfd.cAlternateFileName ) - tmplen;
            new_buf = HeapReAlloc( GetProcessHeap(), 0, tmpshortpath, buf_len * sizeof(WCHAR) );
            if(!new_buf)
            {
                HeapFree( GetProcessHeap(), 0, tmpshortpath );
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
            tmpshortpath = new_buf;
        }

        lstrcpyW( tmpshortpath + sp, wfd.cAlternateFileName[0] ? wfd.cAlternateFileName : wfd.cFileName );
        sp += lstrlenW( tmpshortpath + sp );
        lp += tmplen;
    }
    tmpshortpath[sp] = 0;

    tmplen = lstrlenW( tmpshortpath ) + 1;
    if (tmplen <= shortlen)
    {
        lstrcpyW( shortpath, tmpshortpath );
        TRACE( "returning %s\n", debugstr_w( shortpath ));
        tmplen--; /* length without 0 */
    }

    HeapFree( GetProcessHeap(), 0, tmpshortpath );
    return tmplen;

 notfound:
    HeapFree( GetProcessHeap(), 0, tmpshortpath );
    TRACE( "not found\n" );
    SetLastError( ERROR_FILE_NOT_FOUND );
    return 0;
}


/***********************************************************************
 *	GetSystemDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( system_dir, path, count );
}


/***********************************************************************
 *	GetSystemDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemDirectoryW( LPWSTR path, UINT count )
{
    return copy_filename( system_dir, path, count );
}


/***********************************************************************
 *	GetSystemWindowsDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWindowsDirectoryA( LPSTR path, UINT count )
{
    return GetWindowsDirectoryA( path, count );
}


/***********************************************************************
 *	GetSystemWindowsDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWindowsDirectoryW( LPWSTR path, UINT count )
{
    return GetWindowsDirectoryW( path, count );
}


/***********************************************************************
 *	GetSystemWow64DirectoryA   (kernelbase.@)
 */
UINT WINAPI /* DECLSPEC_HOTPATCH */ GetSystemWow64DirectoryA( LPSTR path, UINT count )
{
    if (!is_win64 && !is_wow64)
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }
    return copy_filename_WtoA( get_machine_wow64_dir( IMAGE_FILE_MACHINE_I386 ), path, count );
}


/***********************************************************************
 *	GetSystemWow64DirectoryW   (kernelbase.@)
 */
UINT WINAPI /* DECLSPEC_HOTPATCH */ GetSystemWow64DirectoryW( LPWSTR path, UINT count )
{
    if (!is_win64 && !is_wow64)
    {
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    }
    return copy_filename( get_machine_wow64_dir( IMAGE_FILE_MACHINE_I386 ), path, count );
}


/***********************************************************************
 *	GetSystemWow64Directory2A   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWow64Directory2A( LPSTR path, UINT count, WORD machine )
{
    const WCHAR *dir = get_machine_wow64_dir( machine );

    return dir ? copy_filename_WtoA( dir, path, count ) : 0;
}


/***********************************************************************
 *	GetSystemWow64Directory2W   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWow64Directory2W( LPWSTR path, UINT count, WORD machine )
{
    const WCHAR *dir = get_machine_wow64_dir( machine );

    return dir ? copy_filename( dir, path, count ) : 0;
}


/***********************************************************************
 *	GetTempFileNameA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique, LPSTR buffer )
{
    WCHAR *pathW, *prefixW = NULL;
    WCHAR bufferW[MAX_PATH];
    UINT ret;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return 0;
    if (prefix && !(prefixW = file_name_AtoW( prefix, TRUE ))) return 0;

    ret = GetTempFileNameW( pathW, prefixW, unique, bufferW );
    if (ret) file_name_WtoA( bufferW, -1, buffer, MAX_PATH );

    HeapFree( GetProcessHeap(), 0, prefixW );
    return ret;
}


/***********************************************************************
 *	GetTempFileNameW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer )
{
    int i;
    LPWSTR p;
    DWORD attr;

    if (!path || !buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* ensure that the provided directory exists */
    attr = GetFileAttributesW( path );
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        TRACE( "path not found %s\n", debugstr_w( path ));
        SetLastError( ERROR_DIRECTORY );
        return 0;
    }

    lstrcpyW( buffer, path );
    p = buffer + lstrlenW(buffer);

    /* add a \, if there isn't one  */
    if ((p == buffer) || (p[-1] != '\\')) *p++ = '\\';

    if (prefix) for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    unique &= 0xffff;
    if (unique) swprintf( p, MAX_PATH - (p - buffer), L"%x.tmp", unique );
    else
    {
        /* get a "random" unique number and try to create the file */
        HANDLE handle;
        UINT num = NtGetTickCount() & 0xffff;
        static UINT last;

        /* avoid using the same name twice in a short interval */
        if (last - num < 10) num = last + 1;
        if (!num) num = 1;
        unique = num;
        do
        {
            swprintf( p, MAX_PATH - (p - buffer), L"%x.tmp", unique );
            handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                CloseHandle( handle );
                last = unique;
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION)
                break;  /* No need to go on */
            if (!(++unique & 0xffff)) unique = 1;
        } while (unique != num);
    }
    TRACE( "returning %s\n", debugstr_w( buffer ));
    return unique;
}


/***********************************************************************
 *	GetTempPathA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPathA( DWORD count, LPSTR path )
{
    WCHAR pathW[MAX_PATH];
    UINT ret;

    if (!(ret = GetTempPathW( MAX_PATH, pathW ))) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( pathW, path, count );
}


/***********************************************************************
 *	GetTempPathW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPathW( DWORD count, LPWSTR path )
{
    WCHAR tmp_path[MAX_PATH];
    UINT ret;

    if (!(ret = GetEnvironmentVariableW( L"TMP", tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( L"TEMP", tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( L"USERPROFILE", tmp_path, MAX_PATH )) &&
        !(ret = GetWindowsDirectoryW( tmp_path, MAX_PATH )))
        return 0;

    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = GetFullPathNameW( tmp_path, MAX_PATH, tmp_path, NULL );
    if (!ret) return 0;

    if (ret > MAX_PATH - 2)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    if (tmp_path[ret-1] != '\\')
    {
        tmp_path[ret++] = '\\';
        tmp_path[ret]   = '\0';
    }

    ret++; /* add space for terminating 0 */
    if (count >= ret)
    {
        lstrcpynW( path, tmp_path, count );
        /* the remaining buffer must be zeroed up to 32766 bytes in XP or 32767
         * bytes after it, we will assume the > XP behavior for now */
        memset( path + ret, 0, (min(count, 32767) - ret) * sizeof(WCHAR) );
        ret--; /* return length without 0 */
    }
    else if (count)
    {
        /* the buffer must be cleared if contents will not fit */
        memset( path, 0, count * sizeof(WCHAR) );
    }

    TRACE( "returning %u, %s\n", ret, debugstr_w( path ));
    return ret;
}


/***********************************************************************
 *	GetTempPath2A   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPath2A(DWORD count, LPSTR path)
{
    /* TODO: Set temp path to C:\Windows\SystemTemp\ when a SYSTEM process calls this function */
    FIXME("(%lu, %p) semi-stub\n", count, path);
    return GetTempPathA(count, path);
}


/***********************************************************************
 *	GetTempPath2W   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPath2W(DWORD count, LPWSTR path)
{
    /* TODO: Set temp path to C:\Windows\SystemTemp\ when a SYSTEM process calls this function */
    FIXME("(%lu, %p) semi-stub\n", count, path);
    return GetTempPathW(count, path);
}


/***********************************************************************
 *	GetWindowsDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWindowsDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( windows_dir, path, count );
}


/***********************************************************************
 *	GetWindowsDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    return copy_filename( windows_dir, path, count );
}


/**************************************************************************
 *	MoveFileExW   (kernelbase.@)
 */
BOOL WINAPI MoveFileExW( const WCHAR *source, const WCHAR *dest, DWORD flag )
{
    return MoveFileWithProgressW( source, dest, NULL, NULL, flag );
}


/**************************************************************************
 *	MoveFileWithProgressW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH MoveFileWithProgressW( const WCHAR *source, const WCHAR *dest,
                                                     LPPROGRESS_ROUTINE progress,
                                                     void *param, DWORD flag )
{
    FILE_RENAME_INFORMATION *rename_info;
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE source_handle = 0;
    ULONG size;

    TRACE( "(%s,%s,%p,%p,%04lx)\n", debugstr_w(source), debugstr_w(dest), progress, param, flag );

    if (flag & MOVEFILE_DELAY_UNTIL_REBOOT) return add_boot_rename_entry( source, dest, flag );

    if (!dest) return DeleteFileW( source );

    /* check if we are allowed to rename the source */

    if (!RtlDosPathNameToNtPathName_U( source, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &source_handle, DELETE | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );
    if (!set_ntstatus( status )) goto error;

    status = NtQueryInformationFile( source_handle, &io, &info, sizeof(info), FileBasicInformation );
    if (!set_ntstatus( status )) goto error;

    if (!RtlDosPathNameToNtPathName_U( dest, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto error;
    }

    size = offsetof( FILE_RENAME_INFORMATION, FileName ) + nt_name.Length;
    if (!(rename_info = HeapAlloc( GetProcessHeap(), 0, size ))) goto error;

    rename_info->ReplaceIfExists = !!(flag & MOVEFILE_REPLACE_EXISTING);
    rename_info->RootDirectory = NULL;
    rename_info->FileNameLength = nt_name.Length;
    memcpy( rename_info->FileName, nt_name.Buffer, nt_name.Length );
    RtlFreeUnicodeString( &nt_name );
    status = NtSetInformationFile( source_handle, &io, rename_info, size, FileRenameInformation );
    HeapFree( GetProcessHeap(), 0, rename_info );
    if (status == STATUS_NOT_SAME_DEVICE && (flag & MOVEFILE_COPY_ALLOWED))
    {
        NtClose( source_handle );
        if (!CopyFileExW( source, dest, progress, param, NULL,
                          flag & MOVEFILE_REPLACE_EXISTING ? 0 : COPY_FILE_FAIL_IF_EXISTS ))
            return FALSE;
        return DeleteFileW( source );
    }

    NtClose( source_handle );
    return set_ntstatus( status );

error:
    if (source_handle) NtClose( source_handle );
    return FALSE;
}


/***********************************************************************
 *	NeedCurrentDirectoryForExePathA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH NeedCurrentDirectoryForExePathA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return TRUE;
    return NeedCurrentDirectoryForExePathW( nameW );
}


/***********************************************************************
 *	NeedCurrentDirectoryForExePathW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH NeedCurrentDirectoryForExePathW( LPCWSTR name )
{
    WCHAR env_val;

    if (wcschr( name, '\\' )) return TRUE;
    /* check the existence of the variable, not value */
    return !GetEnvironmentVariableW( L"NoDefaultCurrentDirectoryInExePath", &env_val, 1 );
}


/***********************************************************************
 *	ReplaceFileW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReplaceFileW( const WCHAR *replaced, const WCHAR *replacement,
                                            const WCHAR *backup, DWORD flags,
                                            void *exclude, void *reserved )
{
    UNICODE_STRING nt_replaced_name, nt_replacement_name;
    HANDLE hReplacement = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK io;
    OBJECT_ATTRIBUTES attr;
    FILE_BASIC_INFORMATION info;

    TRACE( "%s %s %s 0x%08lx %p %p\n", debugstr_w(replaced), debugstr_w(replacement), debugstr_w(backup),
           flags, exclude, reserved );

    if (flags) FIXME("Ignoring flags %lx\n", flags);

    /* First two arguments are mandatory */
    if (!replaced || !replacement)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = NULL;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* Open the "replaced" file for reading */
    if (!RtlDosPathNameToNtPathName_U( replaced, &nt_replaced_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.ObjectName = &nt_replaced_name;

    /* Replacement should fail if replaced is READ_ONLY */
    status = NtQueryAttributesFile(&attr, &info);
    RtlFreeUnicodeString(&nt_replaced_name);
    if (!set_ntstatus( status )) return FALSE;

    if (info.FileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    /*
     * Open the replacement file for reading, writing, and deleting
     * (writing and deleting are needed when finished)
     */
    if (!RtlDosPathNameToNtPathName_U( replacement, &nt_replacement_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.ObjectName = &nt_replacement_name;
    status = NtOpenFile( &hReplacement, GENERIC_READ | GENERIC_WRITE | DELETE | WRITE_DAC | SYNCHRONIZE,
                         &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE );
    RtlFreeUnicodeString(&nt_replacement_name);
    if (!set_ntstatus( status )) return FALSE;
    NtClose( hReplacement );

    /* If the user wants a backup then that needs to be performed first */
    if (backup)
    {
        if (!MoveFileExW( replaced, backup, MOVEFILE_REPLACE_EXISTING )) return FALSE;
    }
    else
    {
        /* ReplaceFile() can replace an open target. To do this, we need to move
         * it out of the way first. */
        WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];

        lstrcpynW( temp_path, replaced, ARRAY_SIZE( temp_path ) );
        PathRemoveFileSpecW( temp_path );
        if (!GetTempFileNameW( temp_path, L"rf", 0, temp_file ) ||
            !MoveFileExW( replaced, temp_file, MOVEFILE_REPLACE_EXISTING ))
            return FALSE;

        DeleteFileW( temp_file );
    }

    /*
     * Now that the backup has been performed (if requested), copy the replacement
     * into place
     */
    if (!MoveFileExW( replacement, replaced, 0 ))
    {
        /* on failure we need to indicate whether a backup was made */
        if (!backup)
            SetLastError( ERROR_UNABLE_TO_MOVE_REPLACEMENT );
        else
            SetLastError( ERROR_UNABLE_TO_MOVE_REPLACEMENT_2 );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	SearchPathA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SearchPathA( LPCSTR path, LPCSTR name, LPCSTR ext,
                                            DWORD buflen, LPSTR buffer, LPSTR *lastpart )
{
    WCHAR *pathW = NULL, *nameW, *extW = NULL;
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(nameW = file_name_AtoW( name, FALSE ))) return 0;
    if (path && !(pathW = file_name_AtoW( path, TRUE ))) return 0;
    if (ext && !(extW = file_name_AtoW( ext, TRUE )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, pathW );
        return 0;
    }

    ret = SearchPathW( pathW, nameW, extW, MAX_PATH, bufferW, NULL );

    RtlFreeHeap( GetProcessHeap(), 0, pathW );
    RtlFreeHeap( GetProcessHeap(), 0, extW );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, buflen );
    if (buflen > ret && lastpart) *lastpart = strrchr(buffer, '\\') + 1;
    return ret;
}


/***********************************************************************
 *	SearchPathW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SearchPathW( LPCWSTR path, LPCWSTR name, LPCWSTR ext, DWORD buflen,
                                            LPWSTR buffer, LPWSTR *lastpart )
{
    DWORD ret = 0;
    WCHAR *name_ext;

    if (!name || !name[0])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* If the name contains an explicit path, ignore the path */

    if (contains_path( name ))
    {
        /* try first without extension */
        if (RtlDoesFileExists_U( name )) return GetFullPathNameW( name, buflen, buffer, lastpart );

        if ((name_ext = append_ext( name, ext )))
        {
            if (RtlDoesFileExists_U( name_ext ))
                ret = GetFullPathNameW( name_ext, buflen, buffer, lastpart );
            RtlFreeHeap( GetProcessHeap(), 0, name_ext );
        }
    }
    else if (path && path[0])  /* search in the specified path */
    {
        ret = RtlDosSearchPath_U( path, name, ext, buflen * sizeof(WCHAR),
                                  buffer, lastpart ) / sizeof(WCHAR);
    }
    else  /* search in active context and default path */
    {
        WCHAR *dll_path = NULL, *name_ext = append_ext( name, ext );

        if (name_ext) name = name_ext;

        /* When file is found with activation context no attempt is made
          to check if it's really exist, path is returned only basing on context info. */
        if (find_actctx_dllpath( name, &dll_path ) == STATUS_SUCCESS)
        {
            ret = lstrlenW( dll_path ) + lstrlenW( name ) + 1;

            /* count null termination char too */
            if (ret <= buflen)
            {
                lstrcpyW( buffer, dll_path );
                lstrcatW( buffer, name );
                if (lastpart) *lastpart = buffer + lstrlenW( dll_path );
                ret--;
            }
            else if (lastpart) *lastpart = NULL;
            RtlFreeHeap( GetProcessHeap(), 0, dll_path );
        }
        else if (!RtlGetSearchPath( &dll_path ))
        {
            ret = RtlDosSearchPath_U( dll_path, name, NULL, buflen * sizeof(WCHAR),
                                      buffer, lastpart ) / sizeof(WCHAR);
            RtlReleasePath( dll_path );
        }
        RtlFreeHeap( GetProcessHeap(), 0, name_ext );
    }

    if (!ret) SetLastError( ERROR_FILE_NOT_FOUND );
    else TRACE( "found %s\n", debugstr_w(buffer) );
    return ret;
}


/***********************************************************************
 *	SetCurrentDirectoryA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCurrentDirectoryA( LPCSTR dir )
{
    WCHAR *dirW;
    UNICODE_STRING strW;

    if (!(dirW = file_name_AtoW( dir, FALSE ))) return FALSE;
    RtlInitUnicodeString( &strW, dirW );
    return set_ntstatus( RtlSetCurrentDirectory_U( &strW ));
}


/***********************************************************************
 *	SetCurrentDirectoryW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCurrentDirectoryW( LPCWSTR dir )
{
    UNICODE_STRING dirW;

    RtlInitUnicodeString( &dirW, dir );
    return set_ntstatus( RtlSetCurrentDirectory_U( &dirW ));
}


/**************************************************************************
 *	SetFileApisToANSI   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToANSI(void)
{
    oem_file_apis = FALSE;
}


/**************************************************************************
 *	SetFileApisToOEM   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToOEM(void)
{
    oem_file_apis = TRUE;
}


/**************************************************************************
 *	SetFileAttributesA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileAttributesA( LPCSTR name, DWORD attributes )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return FALSE;
    return SetFileAttributesW( nameW, attributes );
}


/**************************************************************************
 *	SetFileAttributesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileAttributesW( LPCWSTR name, DWORD attributes )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "%s %lx\n", debugstr_w(name), attributes );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS)
    {
        FILE_BASIC_INFORMATION info;

        memset( &info, 0, sizeof(info) );
        info.FileAttributes = attributes | FILE_ATTRIBUTE_NORMAL;  /* make sure it's not zero */
        status = NtSetInformationFile( handle, &io, &info, sizeof(info), FileBasicInformation );
        NtClose( handle );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	Wow64DisableWow64FsRedirection   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Wow64DisableWow64FsRedirection( PVOID *old_value )
{
    return set_ntstatus( RtlWow64EnableFsRedirectionEx( TRUE, (ULONG *)old_value ));
}


/***********************************************************************
 *	Wow64EnableWow64FsRedirection   (kernelbase.@)
 *
 * Microsoft C++ Redistributable installers are depending on all %eax bits being set.
 */
DWORD /*BOOLEAN*/ WINAPI kernelbase_Wow64EnableWow64FsRedirection( BOOLEAN enable )
{
    return set_ntstatus( RtlWow64EnableFsRedirection( enable ));
}


/***********************************************************************
 *	Wow64RevertWow64FsRedirection   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Wow64RevertWow64FsRedirection( PVOID old_value )
{
    return set_ntstatus( RtlWow64EnableFsRedirection( !old_value ));
}


/***********************************************************************
 * Operations on file handles
 ***********************************************************************/


/***********************************************************************
 *	CancelIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIo( HANDLE handle )
{
    IO_STATUS_BLOCK io;

    return set_ntstatus( NtCancelIoFile( handle, &io ) );
}


/***********************************************************************
 *	CancelIoEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIoEx( HANDLE handle, LPOVERLAPPED overlapped )
{
    IO_STATUS_BLOCK io;

    return set_ntstatus( NtCancelIoFileEx( handle, (PIO_STATUS_BLOCK)overlapped, &io ) );
}


/***********************************************************************
 *	CancelSynchronousIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelSynchronousIo( HANDLE thread )
{
    IO_STATUS_BLOCK io;

    return set_ntstatus( NtCancelSynchronousIoFile( thread, NULL, &io ));
}


/***********************************************************************
 *	FlushFileBuffers   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushFileBuffers( HANDLE file )
{
    IO_STATUS_BLOCK iosb;

    return set_ntstatus( NtFlushBuffersFile( file, &iosb ));
}


/***********************************************************************
 *	GetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandle( HANDLE file, BY_HANDLE_FILE_INFORMATION *info )
{
    FILE_FS_VOLUME_INFORMATION volume_info;
    FILE_STAT_INFORMATION stat_info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( file, &io, &stat_info, sizeof(stat_info), FileStatInformation );
    if (!set_ntstatus( status )) return FALSE;

    info->dwFileAttributes                = stat_info.FileAttributes;
    info->ftCreationTime.dwHighDateTime   = stat_info.CreationTime.u.HighPart;
    info->ftCreationTime.dwLowDateTime    = stat_info.CreationTime.u.LowPart;
    info->ftLastAccessTime.dwHighDateTime = stat_info.LastAccessTime.u.HighPart;
    info->ftLastAccessTime.dwLowDateTime  = stat_info.LastAccessTime.u.LowPart;
    info->ftLastWriteTime.dwHighDateTime  = stat_info.LastWriteTime.u.HighPart;
    info->ftLastWriteTime.dwLowDateTime   = stat_info.LastWriteTime.u.LowPart;
    info->dwVolumeSerialNumber            = 0;
    info->nFileSizeHigh                   = stat_info.EndOfFile.u.HighPart;
    info->nFileSizeLow                    = stat_info.EndOfFile.u.LowPart;
    info->nNumberOfLinks                  = stat_info.NumberOfLinks;
    info->nFileIndexHigh                  = stat_info.FileId.u.HighPart;
    info->nFileIndexLow                   = stat_info.FileId.u.LowPart;

    status = NtQueryVolumeInformationFile( file, &io, &volume_info, sizeof(volume_info), FileFsVolumeInformation );
    if (status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW)
        info->dwVolumeSerialNumber = volume_info.VolumeSerialNumber;

    return TRUE;
}


/***********************************************************************
 *	GetFileInformationByHandleEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandleEx( HANDLE handle, FILE_INFO_BY_HANDLE_CLASS class,
                                                            LPVOID info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    switch (class)
    {
    case FileRemoteProtocolInfo:
    case FileStorageInfo:
    case FileDispositionInfoEx:
    case FileRenameInfoEx:
    case FileCaseSensitiveInfo:
    case FileNormalizedNameInfo:
        FIXME( "%p, %u, %p, %lu\n", handle, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileStreamInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileStreamInformation );
        break;

    case FileCompressionInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileCompressionInformation );
        break;

    case FileAlignmentInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileAlignmentInformation );
        break;

    case FileAttributeTagInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileAttributeTagInformation );
        break;

    case FileBasicInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileBasicInformation );
        break;

    case FileStandardInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileStandardInformation );
        break;

    case FileNameInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileNameInformation );
        break;

    case FileIdInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileIdInformation );
        break;

    case FileIdBothDirectoryRestartInfo:
    case FileIdBothDirectoryInfo:
        status = NtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, info, size,
                                       FileIdBothDirectoryInformation, FALSE, NULL,
                                       (class == FileIdBothDirectoryRestartInfo) );
        break;

    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
        status = NtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, info, size,
                                       FileFullDirectoryInformation, FALSE, NULL,
                                       (class == FileFullDirectoryRestartInfo) );
        break;

    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        status = NtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, info, size,
                                       FileIdExtdDirectoryInformation, FALSE, NULL,
                                       (class == FileIdExtdDirectoryRestartInfo) );
        break;

    case FileRenameInfo:
    case FileDispositionInfo:
    case FileAllocationInfo:
    case FileIoPriorityHintInfo:
    case FileEndOfFileInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	GetFileSize   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileSize( HANDLE file, LPDWORD size_high )
{
    LARGE_INTEGER size;

    if (!GetFileSizeEx( file, &size )) return INVALID_FILE_SIZE;
    if (size_high) *size_high = size.u.HighPart;
    if (size.u.LowPart == INVALID_FILE_SIZE) SetLastError( 0 );
    return size.u.LowPart;
}


/***********************************************************************
 *	GetFileSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileSizeEx( HANDLE file, PLARGE_INTEGER size )
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileStandardInformation )))
        return FALSE;

    *size = info.EndOfFile;
    return TRUE;
}


/***********************************************************************
 *	GetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileTime( HANDLE file, FILETIME *creation,
                                           FILETIME *access, FILETIME *write )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileBasicInformation )))
        return FALSE;

    if (creation)
    {
        creation->dwHighDateTime = info.CreationTime.u.HighPart;
        creation->dwLowDateTime  = info.CreationTime.u.LowPart;
    }
    if (access)
    {
        access->dwHighDateTime = info.LastAccessTime.u.HighPart;
        access->dwLowDateTime  = info.LastAccessTime.u.LowPart;
    }
    if (write)
    {
        write->dwHighDateTime = info.LastWriteTime.u.HighPart;
        write->dwLowDateTime  = info.LastWriteTime.u.LowPart;
    }
    return TRUE;
}


/***********************************************************************
 *	GetFileType   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileType( HANDLE file )
{
    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (file == (HANDLE)STD_INPUT_HANDLE ||
        file == (HANDLE)STD_OUTPUT_HANDLE ||
        file == (HANDLE)STD_ERROR_HANDLE)
        file = GetStdHandle( (DWORD_PTR)file );

    if (!set_ntstatus( NtQueryVolumeInformationFile( file, &io, &info, sizeof(info),
                                                     FileFsDeviceInformation )))
        return FILE_TYPE_UNKNOWN;

    switch (info.DeviceType)
    {
    case FILE_DEVICE_NULL:
    case FILE_DEVICE_CONSOLE:
    case FILE_DEVICE_SERIAL_PORT:
    case FILE_DEVICE_PARALLEL_PORT:
    case FILE_DEVICE_TAPE:
    case FILE_DEVICE_UNKNOWN:
        return FILE_TYPE_CHAR;
    case FILE_DEVICE_NAMED_PIPE:
        return FILE_TYPE_PIPE;
    default:
        return FILE_TYPE_DISK;
    }
}


/***********************************************************************
 *	GetOverlappedResult   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetOverlappedResult( HANDLE file, LPOVERLAPPED overlapped,
                                                   LPDWORD result, BOOL wait )
{
    return GetOverlappedResultEx( file, overlapped, result, wait ? INFINITE : 0, FALSE );
}


/***********************************************************************
 *	GetOverlappedResultEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetOverlappedResultEx( HANDLE file, OVERLAPPED *overlapped,
                                                     DWORD *result, DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    DWORD ret;

    TRACE( "(%p %p %p %lu %d)\n", file, overlapped, result, timeout, alertable );

    /* Paired with the write-release in set_async_iosb() in ntdll; see the
     * latter for details. */
    status = ReadAcquire( (LONG *)&overlapped->Internal );
    if (status == STATUS_PENDING)
    {
        if (!timeout)
        {
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
        }
        ret = WaitForSingleObjectEx( overlapped->hEvent ? overlapped->hEvent : file, timeout, alertable );
        if (ret == WAIT_FAILED)
            return FALSE;
        else if (ret)
        {
            SetLastError( ret );
            return FALSE;
        }

        /* We don't need to give this load acquire semantics; the wait above
         * already guarantees that the IOSB and output buffer are filled. */
        status = overlapped->Internal;
        if (status == STATUS_PENDING) status = STATUS_SUCCESS;
    }

    *result = overlapped->InternalHigh;
    return set_ntstatus( status );
}


/**************************************************************************
 *	LockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                        DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    TRACE( "%p %lx%08lx %lx%08lx\n", file, offset_high, offset_low, count_high, count_low );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtLockFile( file, 0, NULL, NULL, NULL, &offset, &count, NULL, TRUE, TRUE ));
}


/**************************************************************************
 *	LockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFileEx( HANDLE file, DWORD flags, DWORD reserved,
                                          DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER count, offset;
    LPVOID cvalue = NULL;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "%p %lx%08lx %lx%08lx flags %lx\n",
           file, overlapped->OffsetHigh, overlapped->Offset, count_high, count_low, flags );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;

    if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;

    return set_ntstatus( NtLockFile( file, overlapped->hEvent, NULL, cvalue,
                                     NULL, &offset, &count, NULL,
                                     flags & LOCKFILE_FAIL_IMMEDIATELY,
                                     flags & LOCKFILE_EXCLUSIVE_LOCK ));
}


/***********************************************************************
 *	OpenFileById   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenFileById( HANDLE handle, LPFILE_ID_DESCRIPTOR id, DWORD access,
                                              DWORD share, LPSECURITY_ATTRIBUTES sec_attr, DWORD flags )
{
    UINT options;
    HANDLE result;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING objectName;

    if (!id)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    options = FILE_OPEN_BY_FILE_ID;
    if (flags & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (flags & FILE_FLAG_NO_BUFFERING) options |= FILE_NO_INTERMEDIATE_BUFFERING;
    if (!(flags & FILE_FLAG_OVERLAPPED)) options |= FILE_SYNCHRONOUS_IO_NONALERT;
    if (flags & FILE_FLAG_RANDOM_ACCESS) options |= FILE_RANDOM_ACCESS;
    if (flags & FILE_FLAG_SEQUENTIAL_SCAN) options |= FILE_SEQUENTIAL_ONLY;
    flags &= FILE_ATTRIBUTE_VALID_FLAGS;

    objectName.Length             = sizeof(ULONGLONG);
    objectName.Buffer             = (WCHAR *)&id->FileId;
    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = handle;
    attr.Attributes               = 0;
    attr.ObjectName               = &objectName;
    attr.SecurityDescriptor       = sec_attr ? sec_attr->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (sec_attr && sec_attr->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    if (!set_ntstatus( NtCreateFile( &result, access | SYNCHRONIZE, &attr, &io, NULL, flags,
                                     share, OPEN_EXISTING, options, NULL, 0 )))
        return INVALID_HANDLE_VALUE;
    return result;
}


/***********************************************************************
 *	ReOpenFile   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH ReOpenFile( HANDLE handle, DWORD access, DWORD sharing, DWORD attributes )
{
    SECURITY_QUALITY_OF_SERVICE qos;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING empty = { 0 };
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE file;

    TRACE("handle %p, access %#lx, sharing %#lx, attributes %#lx.\n", handle, access, sharing, attributes);

    if (attributes & 0x7ffff) /* FILE_ATTRIBUTE_* flags are invalid */
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
        access |= DELETE;

    InitializeObjectAttributes( &attr, &empty, OBJ_CASE_INSENSITIVE, handle, NULL );
    if (attributes & SECURITY_SQOS_PRESENT)
    {
        qos.Length = sizeof(qos);
        qos.ImpersonationLevel = (attributes >> 16) & 0x3;
        qos.ContextTrackingMode = attributes & SECURITY_CONTEXT_TRACKING ? SECURITY_DYNAMIC_TRACKING : SECURITY_STATIC_TRACKING;
        qos.EffectiveOnly = (attributes & SECURITY_EFFECTIVE_ONLY) != 0;
        attr.SecurityQualityOfService = &qos;
    }

    status = NtCreateFile( &file, access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &io, NULL,
                           0, sharing, FILE_OPEN, get_nt_file_options( attributes ), NULL, 0 );
    if (!set_ntstatus( status ))
        return INVALID_HANDLE_VALUE;
    return file;
}


static void WINAPI invoke_completion( void *context, IO_STATUS_BLOCK *io, ULONG res )
{
    LPOVERLAPPED_COMPLETION_ROUTINE completion = context;
    completion( RtlNtStatusToDosError( io->Status ), io->Information, (LPOVERLAPPED)io );
}

/****************************************************************************
 *	ReadDirectoryChangesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadDirectoryChangesW( HANDLE handle, LPVOID buffer, DWORD len,
                                                     BOOL subtree, DWORD filter, LPDWORD returned,
                                                     LPOVERLAPPED overlapped,
                                                     LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    OVERLAPPED ov, *pov;
    IO_STATUS_BLOCK *ios;
    NTSTATUS status;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %08lx %d %08lx %p %p %p\n",
           handle, buffer, len, subtree, filter, returned, overlapped, completion );

    if (!overlapped)
    {
        memset( &ov, 0, sizeof ov );
        ov.hEvent = CreateEventW( NULL, 0, 0, NULL );
        pov = &ov;
    }
    else
    {
        pov = overlapped;
        if (completion) cvalue = completion;
        else if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;
    }

    ios = (PIO_STATUS_BLOCK)pov;
    ios->Status = STATUS_PENDING;

    status = NtNotifyChangeDirectoryFile( handle, completion && overlapped ? NULL : pov->hEvent,
                                          completion && overlapped ? invoke_completion : NULL,
                                          cvalue, ios, buffer, len, filter, subtree );
    if (status == STATUS_PENDING)
    {
        if (overlapped) return TRUE;
        WaitForSingleObjectEx( ov.hEvent, INFINITE, TRUE );
        if (returned) *returned = ios->Information;
        status = ios->Status;
    }
    if (!overlapped) CloseHandle( ov.hEvent );
    return set_ntstatus( status );
}


/***********************************************************************
 *	ReadFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFile( HANDLE file, LPVOID buffer, DWORD count,
                                        LPDWORD result, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK io_status = &iosb;
    HANDLE event = 0;
    NTSTATUS status;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %ld %p %p\n", file, buffer, count, result, overlapped );

    if (result) *result = 0;

    if (overlapped)
    {
        offset.u.LowPart = overlapped->Offset;
        offset.u.HighPart = overlapped->OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        io_status = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else io_status->Information = 0;
    io_status->Status = STATUS_PENDING;

    status = NtReadFile( file, event, NULL, cvalue, io_status, buffer, count, poffset, NULL);

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = io_status->Status;
    }

    if (result) *result = overlapped && status ? 0 : io_status->Information;

    if (status == STATUS_END_OF_FILE)
    {
        if (overlapped != NULL)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }
    }
    else if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	ReadFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileEx( HANDLE file, LPVOID buffer, DWORD count, LPOVERLAPPED overlapped,
                                          LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    NTSTATUS status;

    TRACE( "(file=%p, buffer=%p, bytes=%lu, ovl=%p, ovl_fn=%p)\n",
           file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->Status = STATUS_PENDING;
    io->Information = 0;

    status = NtReadFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL);
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	ReadFileScatter   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileScatter( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "(%p %p %lu %p)\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtReadFileScatter( file, overlapped->hEvent, NULL, cvalue, io,
                                            segments, count, &offset, NULL ));
}


/***********************************************************************
 *	RemoveDirectoryA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryA( LPCSTR path )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return FALSE;
    return RemoveDirectoryW( pathW );
}


/***********************************************************************
 *	RemoveDirectoryW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryW( LPCWSTR path )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "%s\n", debugstr_w(path) );

    status = RtlDosPathNameToNtPathName_U_WithStatus( path, &nt_name, NULL, NULL );
    if (!set_ntstatus( status )) return FALSE;

    InitializeObjectAttributes( &attr, &nt_name, OBJ_CASE_INSENSITIVE, 0, NULL );
    status = NtOpenFile( &handle, DELETE | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (!status)
    {
        FILE_DISPOSITION_INFORMATION info = { TRUE };
        status = NtSetInformationFile( handle, &io, &info, sizeof(info), FileDispositionInformation );
        NtClose( handle );
    }
    return set_ntstatus( status );
}


/**************************************************************************
 *	SetEndOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEndOfFile( HANDLE file )
{
    FILE_POSITION_INFORMATION pos;
    FILE_END_OF_FILE_INFORMATION eof;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (!(status = NtQueryInformationFile( file, &io, &pos, sizeof(pos), FilePositionInformation )))
    {
        eof.EndOfFile = pos.CurrentByteOffset;
        status = NtSetInformationFile( file, &io, &eof, sizeof(eof), FileEndOfFileInformation );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileInformationByHandle( HANDLE file, FILE_INFO_BY_HANDLE_CLASS class,
                                                          void *info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    TRACE( "%p %u %p %lu\n", file, class, info, size );

    switch (class)
    {
    case FileNameInfo:
    case FileAllocationInfo:
    case FileStreamInfo:
    case FileIdBothDirectoryInfo:
    case FileIdBothDirectoryRestartInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        FIXME( "%p, %u, %p, %lu\n", file, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileEndOfFileInfo:
        status = NtSetInformationFile( file, &io, info, size, FileEndOfFileInformation );
        break;
    case FileBasicInfo:
        status = NtSetInformationFile( file, &io, info, size, FileBasicInformation );
        break;
    case FileDispositionInfo:
        status = NtSetInformationFile( file, &io, info, size, FileDispositionInformation );
        break;
    case FileDispositionInfoEx:
        status = NtSetInformationFile( file, &io, info, size, FileDispositionInformationEx );
        break;
    case FileIoPriorityHintInfo:
        status = NtSetInformationFile( file, &io, info, size, FileIoPriorityHintInformation );
        break;
    case FileRenameInfo:
        {
            FILE_RENAME_INFORMATION *rename_info;
            UNICODE_STRING nt_name;
            ULONG size;

            if ((status = RtlDosPathNameToNtPathName_U_WithStatus( ((FILE_RENAME_INFORMATION *)info)->FileName,
                                                                   &nt_name, NULL, NULL )))
                break;

            size = sizeof(*rename_info) + nt_name.Length;
            if ((rename_info = HeapAlloc( GetProcessHeap(), 0, size )))
            {
                memcpy( rename_info, info, sizeof(*rename_info) );
                memcpy( rename_info->FileName, nt_name.Buffer, nt_name.Length + sizeof(WCHAR) );
                rename_info->FileNameLength = nt_name.Length;
                status = NtSetInformationFile( file, &io, rename_info, size, FileRenameInformation );
                HeapFree( GetProcessHeap(), 0, rename_info );
            }
            RtlFreeUnicodeString( &nt_name );
            break;
        }
    case FileStandardInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFilePointer   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SetFilePointer( HANDLE file, LONG distance, LONG *highword, DWORD method )
{
    LARGE_INTEGER dist, newpos;

    if (highword)
    {
        dist.u.LowPart  = distance;
        dist.u.HighPart = *highword;
    }
    else dist.QuadPart = distance;

    if (!SetFilePointerEx( file, dist, &newpos, method )) return INVALID_SET_FILE_POINTER;

    if (highword) *highword = newpos.u.HighPart;
    if (newpos.u.LowPart == INVALID_SET_FILE_POINTER) SetLastError( 0 );
    return newpos.u.LowPart;
}


/***********************************************************************
 *	SetFilePointerEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFilePointerEx( HANDLE file, LARGE_INTEGER distance,
                                                LARGE_INTEGER *newpos, DWORD method )
{
    LONGLONG pos;
    IO_STATUS_BLOCK io;
    FILE_POSITION_INFORMATION info;
    FILE_STANDARD_INFORMATION eof;

    switch(method)
    {
    case FILE_BEGIN:
        pos = distance.QuadPart;
        break;
    case FILE_CURRENT:
        if (NtQueryInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
            goto error;
        pos = info.CurrentByteOffset.QuadPart + distance.QuadPart;
        break;
    case FILE_END:
        if (NtQueryInformationFile( file, &io, &eof, sizeof(eof), FileStandardInformation ))
            goto error;
        pos = eof.EndOfFile.QuadPart + distance.QuadPart;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (pos < 0)
    {
        SetLastError( ERROR_NEGATIVE_SEEK );
        return FALSE;
    }

    info.CurrentByteOffset.QuadPart = pos;
    if (!NtSetInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
    {
        if (newpos) newpos->QuadPart = pos;
        return TRUE;
    }

error:
    return set_ntstatus( io.Status );
}


/***********************************************************************
 *	SetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileTime( HANDLE file, const FILETIME *ctime,
                                           const FILETIME *atime, const FILETIME *mtime )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

    memset( &info, 0, sizeof(info) );
    if (ctime)
    {
        info.CreationTime.u.HighPart = ctime->dwHighDateTime;
        info.CreationTime.u.LowPart  = ctime->dwLowDateTime;
    }
    if (atime)
    {
        info.LastAccessTime.u.HighPart = atime->dwHighDateTime;
        info.LastAccessTime.u.LowPart  = atime->dwLowDateTime;
    }
    if (mtime)
    {
        info.LastWriteTime.u.HighPart = mtime->dwHighDateTime;
        info.LastWriteTime.u.LowPart  = mtime->dwLowDateTime;
    }

    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info), FileBasicInformation ));
}


/***********************************************************************
 *	SetFileValidData   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileValidData( HANDLE file, LONGLONG length )
{
    FILE_VALID_DATA_LENGTH_INFORMATION info;
    IO_STATUS_BLOCK io;

    info.ValidDataLength.QuadPart = length;
    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info),
                                               FileValidDataLengthInformation ));
}


/**************************************************************************
 *	UnlockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                          DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtUnlockFile( file, NULL, &offset, &count, NULL ));
}


/**************************************************************************
 *	UnlockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFileEx( HANDLE file, DWORD reserved,
                                            DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (overlapped->hEvent) FIXME("Unimplemented overlapped operation\n");

    return UnlockFile( file, overlapped->Offset, overlapped->OffsetHigh, count_low, count_high );
}


/***********************************************************************
 *	WriteFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFile( HANDLE file, LPCVOID buffer, DWORD count,
                                         LPDWORD result, LPOVERLAPPED overlapped )
{
    HANDLE event = NULL;
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK piosb = &iosb;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %ld %p %p\n", file, buffer, count, result, overlapped );

    if (overlapped)
    {
        offset.u.LowPart = overlapped->Offset;
        offset.u.HighPart = overlapped->OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        piosb = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else piosb->Information = 0;
    piosb->Status = STATUS_PENDING;

    status = NtWriteFile( file, event, NULL, cvalue, piosb, buffer, count, poffset, NULL );

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = piosb->Status;
    }

    if (result) *result = overlapped && status ? 0 : piosb->Information;

    if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	WriteFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileEx( HANDLE file, LPCVOID buffer,
                                           DWORD count, LPOVERLAPPED overlapped,
                                           LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    LARGE_INTEGER offset;
    NTSTATUS status;
    PIO_STATUS_BLOCK io;

    TRACE( "%p %p %ld %p %p\n", file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;

    io = (PIO_STATUS_BLOCK)overlapped;
    io->Status = STATUS_PENDING;
    io->Information = 0;

    status = NtWriteFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL );
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	WriteFileGather   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileGather( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "%p %p %lu %p\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->Offset;
    offset.u.HighPart = overlapped->OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtWriteFileGather( file, overlapped->hEvent, NULL, cvalue,
                                            io, segments, count, &offset, NULL ));
}


/***********************************************************************
 * Operations on file times
 ***********************************************************************/


/*********************************************************************
 *	CompareFileTime   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareFileTime( const FILETIME *x, const FILETIME *y )
{
    if (!x || !y) return -1;
    if (x->dwHighDateTime > y->dwHighDateTime) return 1;
    if (x->dwHighDateTime < y->dwHighDateTime) return -1;
    if (x->dwLowDateTime > y->dwLowDateTime) return 1;
    if (x->dwLowDateTime < y->dwLowDateTime) return -1;
    return 0;
}


/*********************************************************************
 *	FileTimeToLocalFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FileTimeToLocalFileTime( const FILETIME *utc, FILETIME *local )
{
    return set_ntstatus( RtlSystemTimeToLocalTime( (const LARGE_INTEGER *)utc, (LARGE_INTEGER *)local ));
}


/*********************************************************************
 *	FileTimeToSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FileTimeToSystemTime( const FILETIME *ft, SYSTEMTIME *systime )
{
    TIME_FIELDS tf;
    const LARGE_INTEGER *li = (const LARGE_INTEGER *)ft;

    if (li->QuadPart < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlTimeToTimeFields( li, &tf );
    systime->wYear = tf.Year;
    systime->wMonth = tf.Month;
    systime->wDay = tf.Day;
    systime->wHour = tf.Hour;
    systime->wMinute = tf.Minute;
    systime->wSecond = tf.Second;
    systime->wMilliseconds = tf.Milliseconds;
    systime->wDayOfWeek = tf.Weekday;
    return TRUE;
}


/*********************************************************************
 *	GetLocalTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetLocalTime( SYSTEMTIME *systime )
{
    LARGE_INTEGER ft, ft2;

    NtQuerySystemTime( &ft );
    RtlSystemTimeToLocalTime( &ft, &ft2 );
    FileTimeToSystemTime( (FILETIME *)&ft2, systime );
}


/*********************************************************************
 *	GetSystemTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTime( SYSTEMTIME *systime )
{
    LARGE_INTEGER ft;

    NtQuerySystemTime( &ft );
    FileTimeToSystemTime( (FILETIME *)&ft, systime );
}


/***********************************************************************
 *	GetSystemTimeAdjustment   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetSystemTimeAdjustment( DWORD *adjust, DWORD *increment, BOOL *disabled )
{
    SYSTEM_TIME_ADJUSTMENT_QUERY st;
    ULONG len;

    if (!set_ntstatus( NtQuerySystemInformation( SystemTimeAdjustmentInformation, &st, sizeof(st), &len )))
        return FALSE;
    *adjust    = st.TimeAdjustment;
    *increment = st.TimeIncrement;
    *disabled  = st.TimeAdjustmentDisabled;
    return TRUE;
}


/***********************************************************************
 *	GetSystemTimeAsFileTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTimeAsFileTime( FILETIME *time )
{
    NtQuerySystemTime( (LARGE_INTEGER *)time );
}


/***********************************************************************
 *	GetSystemTimePreciseAsFileTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTimePreciseAsFileTime( FILETIME *time )
{
    LARGE_INTEGER t;

    t.QuadPart = RtlGetSystemTimePrecise();
    time->dwLowDateTime = t.u.LowPart;
    time->dwHighDateTime = t.u.HighPart;
}


/*********************************************************************
 *	LocalFileTimeToFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LocalFileTimeToFileTime( const FILETIME *local, FILETIME *utc )
{
    return set_ntstatus( RtlLocalTimeToSystemTime( (const LARGE_INTEGER *)local, (LARGE_INTEGER *)utc ));
}


/***********************************************************************
 *	SetLocalTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetLocalTime( const SYSTEMTIME *systime )
{
    FILETIME ft;
    LARGE_INTEGER st;

    if (!SystemTimeToFileTime( systime, &ft )) return FALSE;
    RtlLocalTimeToSystemTime( (LARGE_INTEGER *)&ft, &st );
    return set_ntstatus( NtSetSystemTime( &st, NULL ));
}


/***********************************************************************
 *	SetSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetSystemTime( const SYSTEMTIME *systime )
{
    FILETIME ft;

    if (!SystemTimeToFileTime( systime, &ft )) return FALSE;
    return set_ntstatus( NtSetSystemTime( (LARGE_INTEGER *)&ft, NULL ));
}


/***********************************************************************
 *	SetSystemTimeAdjustment   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetSystemTimeAdjustment( DWORD adjust, BOOL disabled )
{
    SYSTEM_TIME_ADJUSTMENT st;

    st.TimeAdjustment = adjust;
    st.TimeAdjustmentDisabled = disabled;
    return set_ntstatus( NtSetSystemInformation( SystemTimeAdjustmentInformation, &st, sizeof(st) ));
}


/*********************************************************************
 *	SystemTimeToFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SystemTimeToFileTime( const SYSTEMTIME *systime, FILETIME *ft )
{
    TIME_FIELDS tf;

    tf.Year = systime->wYear;
    tf.Month = systime->wMonth;
    tf.Day = systime->wDay;
    tf.Hour = systime->wHour;
    tf.Minute = systime->wMinute;
    tf.Second = systime->wSecond;
    tf.Milliseconds = systime->wMilliseconds;
    if (RtlTimeFieldsToTime( &tf, (LARGE_INTEGER *)ft )) return TRUE;
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 * I/O controls
 ***********************************************************************/


static void dump_dcb( const DCB *dcb )
{
    TRACE( "size=%d rate=%ld fParity=%d Parity=%d stopbits=%d %sIXON %sIXOFF CTS=%d RTS=%d DSR=%d DTR=%d %sCRTSCTS\n",
           dcb->ByteSize, dcb->BaudRate, dcb->fParity, dcb->Parity,
           (dcb->StopBits == ONESTOPBIT) ? 1 : (dcb->StopBits == TWOSTOPBITS) ? 2 : 0,
           dcb->fOutX ? "" : "~", dcb->fInX ? "" : "~",
           dcb->fOutxCtsFlow, dcb->fRtsControl, dcb->fOutxDsrFlow, dcb->fDtrControl,
           (dcb->fOutxCtsFlow || dcb->fRtsControl == RTS_CONTROL_HANDSHAKE) ? "" : "~" );
}

/*****************************************************************************
 *	ClearCommBreak   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ClearCommBreak( HANDLE handle )
{
    return EscapeCommFunction( handle, CLRBREAK );
}


/*****************************************************************************
 *	ClearCommError   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ClearCommError( HANDLE handle, DWORD *errors, COMSTAT *stat )
{
    SERIAL_STATUS ss;

    if (!DeviceIoControl( handle, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0, &ss, sizeof(ss), NULL, NULL ))
        return FALSE;

    TRACE( "status %#lx,%#lx, in %lu, out %lu, eof %d, wait %d\n", ss.Errors, ss.HoldReasons,
           ss.AmountInInQueue, ss.AmountInOutQueue, ss.EofReceived, ss.WaitForImmediate );

    if (errors)
    {
        *errors = 0;
        if (ss.Errors & SERIAL_ERROR_BREAK)        *errors |= CE_BREAK;
        if (ss.Errors & SERIAL_ERROR_FRAMING)      *errors |= CE_FRAME;
        if (ss.Errors & SERIAL_ERROR_OVERRUN)      *errors |= CE_OVERRUN;
        if (ss.Errors & SERIAL_ERROR_QUEUEOVERRUN) *errors |= CE_RXOVER;
        if (ss.Errors & SERIAL_ERROR_PARITY)       *errors |= CE_RXPARITY;
    }
    if (stat)
    {
        stat->fCtsHold  = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_CTS);
        stat->fDsrHold  = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_DSR);
        stat->fRlsdHold = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_DCD);
        stat->fXoffHold = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_XON);
        stat->fXoffSent = !!(ss.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT);
        stat->fEof      = !!ss.EofReceived;
        stat->fTxim     = !!ss.WaitForImmediate;
        stat->cbInQue   = ss.AmountInInQueue;
        stat->cbOutQue  = ss.AmountInOutQueue;
    }
    return TRUE;
}


/****************************************************************************
 *	DeviceIoControl   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeviceIoControl( HANDLE handle, DWORD code, void *in_buff, DWORD in_count,
                                               void *out_buff, DWORD out_count, DWORD *returned,
                                               OVERLAPPED *overlapped )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    void *cvalue = NULL;
    HANDLE event = 0;
    NTSTATUS status;

    TRACE( "(%p,%lx,%p,%ld,%p,%ld,%p,%p)\n",
           handle, code, in_buff, in_count, out_buff, out_count, returned, overlapped );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
    }

    if (HIWORD(code) == FILE_DEVICE_FILE_SYSTEM)
        status = NtFsControlFile( handle, event, NULL, cvalue, piosb, code,
                                  in_buff, in_count, out_buff, out_count );
    else
        status = NtDeviceIoControlFile( handle, event, NULL, cvalue, piosb, code,
                                        in_buff, in_count, out_buff, out_count );

    if (returned && !NT_ERROR(status)) *returned = piosb->Information;
    if (status == STATUS_PENDING || !NT_SUCCESS( status )) return set_ntstatus( status );
    return TRUE;
}


/*****************************************************************************
 *	EscapeCommFunction   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EscapeCommFunction( HANDLE handle, DWORD func )
{
    static const DWORD ioctls[] =
    {
        0,
        IOCTL_SERIAL_SET_XOFF,      /* SETXOFF */
        IOCTL_SERIAL_SET_XON,       /* SETXON */
        IOCTL_SERIAL_SET_RTS,       /* SETRTS */
        IOCTL_SERIAL_CLR_RTS,       /* CLRRTS */
        IOCTL_SERIAL_SET_DTR,       /* SETDTR */
        IOCTL_SERIAL_CLR_DTR,       /* CLRDTR */
        IOCTL_SERIAL_RESET_DEVICE,  /* RESETDEV */
        IOCTL_SERIAL_SET_BREAK_ON,  /* SETBREAK */
        IOCTL_SERIAL_SET_BREAK_OFF  /* CLRBREAK */
    };

    if (func >= ARRAY_SIZE(ioctls) || !ioctls[func])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, ioctls[func], NULL, 0, NULL, 0, NULL, NULL );
}


/***********************************************************************
 *	GetCommConfig   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommConfig( HANDLE handle, COMMCONFIG *config, DWORD *size )
{
    if (!config) return FALSE;

    TRACE( "(%p, %p, %p %lu)\n", handle, config, size, *size );

    if (*size < sizeof(COMMCONFIG))
    {
        *size = sizeof(COMMCONFIG);
        return FALSE;
    }
    *size = sizeof(COMMCONFIG);
    config->dwSize = sizeof(COMMCONFIG);
    config->wVersion = 1;
    config->wReserved = 0;
    config->dwProviderSubType = PST_RS232;
    config->dwProviderOffset = 0;
    config->dwProviderSize = 0;
    return GetCommState( handle, &config->dcb );
}


/*****************************************************************************
 *	GetCommMask   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommMask( HANDLE handle, DWORD *mask )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_WAIT_MASK, NULL, 0, mask, sizeof(*mask),
                            NULL, NULL );
}


/***********************************************************************
 *	GetCommModemStatus   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommModemStatus( HANDLE handle, DWORD *status )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_MODEMSTATUS, NULL, 0, status, sizeof(*status),
                            NULL, NULL );
}


/***********************************************************************
 *	GetCommProperties   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommProperties( HANDLE handle, COMMPROP *prop )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_PROPERTIES, NULL, 0, prop, sizeof(*prop), NULL, NULL );
}


/*****************************************************************************
 *	GetCommState   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommState( HANDLE handle, DCB *dcb )
{
    SERIAL_BAUD_RATE sbr;
    SERIAL_LINE_CONTROL slc;
    SERIAL_HANDFLOW shf;
    SERIAL_CHARS sc;

    if (!dcb)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!DeviceIoControl(handle, IOCTL_SERIAL_GET_BAUD_RATE, NULL, 0, &sbr, sizeof(sbr), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_LINE_CONTROL, NULL, 0, &slc, sizeof(slc), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_HANDFLOW, NULL, 0, &shf, sizeof(shf), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_CHARS, NULL, 0, &sc, sizeof(sc), NULL, NULL))
        return FALSE;

    dcb->DCBlength         = sizeof(*dcb);
    dcb->BaudRate          = sbr.BaudRate;
    /* yes, they seem no never be (re)set on NT */
    dcb->fBinary           = 1;
    dcb->fParity           = 0;
    dcb->fOutxCtsFlow      = !!(shf.ControlHandShake & SERIAL_CTS_HANDSHAKE);
    dcb->fOutxDsrFlow      = !!(shf.ControlHandShake & SERIAL_DSR_HANDSHAKE);
    dcb->fDsrSensitivity   = !!(shf.ControlHandShake & SERIAL_DSR_SENSITIVITY);
    dcb->fTXContinueOnXoff = !!(shf.FlowReplace & SERIAL_XOFF_CONTINUE);
    dcb->fOutX             = !!(shf.FlowReplace & SERIAL_AUTO_TRANSMIT);
    dcb->fInX              = !!(shf.FlowReplace & SERIAL_AUTO_RECEIVE);
    dcb->fErrorChar        = !!(shf.FlowReplace & SERIAL_ERROR_CHAR);
    dcb->fNull             = !!(shf.FlowReplace & SERIAL_NULL_STRIPPING);
    dcb->fAbortOnError     = !!(shf.ControlHandShake & SERIAL_ERROR_ABORT);
    dcb->XonLim            = shf.XonLimit;
    dcb->XoffLim           = shf.XoffLimit;
    dcb->ByteSize          = slc.WordLength;
    dcb->Parity            = slc.Parity;
    dcb->StopBits          = slc.StopBits;
    dcb->XonChar           = sc.XonChar;
    dcb->XoffChar          = sc.XoffChar;
    dcb->ErrorChar         = sc.ErrorChar;
    dcb->EofChar           = sc.EofChar;
    dcb->EvtChar           = sc.EventChar;

    switch (shf.ControlHandShake & (SERIAL_DTR_CONTROL | SERIAL_DTR_HANDSHAKE))
    {
    case SERIAL_DTR_CONTROL:    dcb->fDtrControl = DTR_CONTROL_ENABLE; break;
    case SERIAL_DTR_HANDSHAKE:  dcb->fDtrControl = DTR_CONTROL_HANDSHAKE; break;
    default:                    dcb->fDtrControl = DTR_CONTROL_DISABLE; break;
    }
    switch (shf.FlowReplace & (SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE))
    {
    case SERIAL_RTS_CONTROL:    dcb->fRtsControl = RTS_CONTROL_ENABLE; break;
    case SERIAL_RTS_HANDSHAKE:  dcb->fRtsControl = RTS_CONTROL_HANDSHAKE; break;
    case SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE:
                                dcb->fRtsControl = RTS_CONTROL_TOGGLE; break;
    default:                    dcb->fRtsControl = RTS_CONTROL_DISABLE; break;
    }
    dump_dcb( dcb );
    return TRUE;
}


/*****************************************************************************
 *	GetCommTimeouts   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommTimeouts( HANDLE handle, COMMTIMEOUTS *timeouts )
{
    if (!timeouts)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, timeouts, sizeof(*timeouts),
                            NULL, NULL );
}

/********************************************************************
 *	PurgeComm   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PurgeComm(HANDLE handle, DWORD flags)
{
    return DeviceIoControl( handle, IOCTL_SERIAL_PURGE, &flags, sizeof(flags),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	SetCommBreak   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommBreak( HANDLE handle )
{
    return EscapeCommFunction( handle, SETBREAK );
}


/***********************************************************************
 *	SetCommConfig   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommConfig( HANDLE handle, COMMCONFIG *config, DWORD size )
{
    TRACE( "(%p, %p, %lu)\n", handle, config, size );
    return SetCommState( handle, &config->dcb );
}


/*****************************************************************************
 *	SetCommMask   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommMask( HANDLE handle, DWORD mask )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_WAIT_MASK, &mask, sizeof(mask),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	SetCommState   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommState( HANDLE handle, DCB *dcb )
{
    SERIAL_BAUD_RATE sbr;
    SERIAL_LINE_CONTROL slc;
    SERIAL_HANDFLOW shf;
    SERIAL_CHARS sc;

    if (!dcb)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    dump_dcb( dcb );

    sbr.BaudRate   = dcb->BaudRate;
    slc.StopBits   = dcb->StopBits;
    slc.Parity     = dcb->Parity;
    slc.WordLength = dcb->ByteSize;
    shf.ControlHandShake = 0;
    shf.FlowReplace = 0;
    if (dcb->fOutxCtsFlow) shf.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    if (dcb->fOutxDsrFlow) shf.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
    switch (dcb->fDtrControl)
    {
    case DTR_CONTROL_DISABLE:   break;
    case DTR_CONTROL_ENABLE:    shf.ControlHandShake |= SERIAL_DTR_CONTROL; break;
    case DTR_CONTROL_HANDSHAKE: shf.ControlHandShake |= SERIAL_DTR_HANDSHAKE; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (dcb->fRtsControl)
    {
    case RTS_CONTROL_DISABLE:   break;
    case RTS_CONTROL_ENABLE:    shf.FlowReplace |= SERIAL_RTS_CONTROL; break;
    case RTS_CONTROL_HANDSHAKE: shf.FlowReplace |= SERIAL_RTS_HANDSHAKE; break;
    case RTS_CONTROL_TOGGLE:    shf.FlowReplace |= SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (dcb->fDsrSensitivity)   shf.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
    if (dcb->fAbortOnError)     shf.ControlHandShake |= SERIAL_ERROR_ABORT;
    if (dcb->fErrorChar)        shf.FlowReplace |= SERIAL_ERROR_CHAR;
    if (dcb->fNull)             shf.FlowReplace |= SERIAL_NULL_STRIPPING;
    if (dcb->fTXContinueOnXoff) shf.FlowReplace |= SERIAL_XOFF_CONTINUE;
    if (dcb->fOutX)             shf.FlowReplace |= SERIAL_AUTO_TRANSMIT;
    if (dcb->fInX)              shf.FlowReplace |= SERIAL_AUTO_RECEIVE;
    shf.XonLimit  = dcb->XonLim;
    shf.XoffLimit = dcb->XoffLim;
    sc.EofChar    = dcb->EofChar;
    sc.ErrorChar  = dcb->ErrorChar;
    sc.BreakChar  = 0;
    sc.EventChar  = dcb->EvtChar;
    sc.XonChar    = dcb->XonChar;
    sc.XoffChar   = dcb->XoffChar;

    /* note: change DTR/RTS lines after setting the comm attributes,
     * so flow control does not interfere.
     */
    return (DeviceIoControl( handle, IOCTL_SERIAL_SET_BAUD_RATE, &sbr, sizeof(sbr), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_LINE_CONTROL, &slc, sizeof(slc), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_HANDFLOW, &shf, sizeof(shf), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_CHARS, &sc, sizeof(sc), NULL, 0, NULL, NULL ));
}


/*****************************************************************************
 *	SetCommTimeouts   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommTimeouts( HANDLE handle, COMMTIMEOUTS *timeouts )
{
    if (!timeouts)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_TIMEOUTS, timeouts, sizeof(*timeouts),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *      SetupComm   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetupComm( HANDLE handle, DWORD in_size, DWORD out_size )
{
    SERIAL_QUEUE_SIZE sqs;

    sqs.InSize = in_size;
    sqs.OutSize = out_size;
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_QUEUE_SIZE, &sqs, sizeof(sqs), NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	TransmitCommChar   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TransmitCommChar( HANDLE handle, CHAR ch )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_IMMEDIATE_CHAR, &ch, sizeof(ch), NULL, 0, NULL, NULL );
}


/***********************************************************************
 *	WaitCommEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitCommEvent( HANDLE handle, DWORD *events, OVERLAPPED *overlapped )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_WAIT_ON_MASK, NULL, 0, events, sizeof(*events),
                            NULL, overlapped );
}


/***********************************************************************
 *	QueryIoRingCapabilities   (kernelbase.@)
 */
HRESULT WINAPI QueryIoRingCapabilities(IORING_CAPABILITIES *caps)
{
    FIXME( "caps %p stub.\n", caps );

    return E_NOTIMPL;
}
