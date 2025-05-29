/*
 * File handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2003 Eric Pouech
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
 *
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);


/**************************************************************************
 *           MoveFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedA(const char *source, const char *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_a(source), debugstr_a(dest), progress, data, flags, handle);

    return MoveFileWithProgressA(source, dest, progress, data, flags);
}

/**************************************************************************
 *           MoveFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI MoveFileTransactedW(const WCHAR *source, const WCHAR *dest, LPPROGRESS_ROUTINE progress, void *data, DWORD flags, HANDLE handle)
{
    FIXME("(%s, %s, %p, %p, %ld, %p) semi-stub\n", debugstr_w(source), debugstr_w(dest), progress, data, flags, handle);

    return MoveFileWithProgressW(source, dest, progress, data, flags);
}

/*************************************************************************
 *           CreateFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedA( LPCSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template,
                                                       HANDLE transaction, PUSHORT version,
                                                       PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_a(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileA(name, access, sharing, sa, creation, attributes, template);
}

/*************************************************************************
 *           CreateFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileTransactedW( LPCWSTR name, DWORD access, DWORD sharing,
                                                       LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                                       DWORD attributes, HANDLE template, HANDLE transaction,
                                                       PUSHORT version, PVOID param )
{
    FIXME("(%s %lx %lx %p %lx %lx %p %p %p %p): semi-stub\n", debugstr_w(name), access, sharing, sa,
           creation, attributes, template, transaction, version, param);
    return CreateFileW(name, access, sharing, sa, creation, attributes, template);
}

/***********************************************************************
 *           CreateDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedA(LPCSTR template, LPCSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_a(template), debugstr_a(path), sa, transaction);
    return CreateDirectoryExA(template, path, sa);
}

/***********************************************************************
 *           CreateDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryTransactedW(LPCWSTR template, LPCWSTR path, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): semi-stub\n", debugstr_w(template), debugstr_w(path), sa, transaction);
    return CreateDirectoryExW(template, path, sa);
}

/***********************************************************************
 *           DeleteFileTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return DeleteFileA(path);
}

/***********************************************************************
 *           DeleteFileTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return DeleteFileW(path);
}

/******************************************************************************
 *           FindFirstFileTransactedA   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedA( LPCSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_a(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExA(filename, level, data, search_op, filter, flags);
}

/******************************************************************************
 *           FindFirstFileTransactedW   (KERNEL32.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstFileTransactedW( LPCWSTR filename, FINDEX_INFO_LEVELS level,
                                                          LPVOID data, FINDEX_SEARCH_OPS search_op,
                                                          LPVOID filter, DWORD flags, HANDLE transaction )
{
    FIXME("(%s %d %p %d %p %lx %p): semi-stub\n", debugstr_w(filename), level, data, search_op, filter, flags, transaction);
    return FindFirstFileExW(filename, level, data, search_op, filter, flags);
}


/**************************************************************************
 *           GetFileAttributesTransactedA   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedA(LPCSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(name), transaction);
    return GetFileAttributesExA(name, level, ptr);
}

/**************************************************************************
 *           GetFileAttributesTransactedW   (KERNEL32.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesTransactedW(LPCWSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(name), transaction);
    return GetFileAttributesExW(name, level, ptr);
}

/***********************************************************************
 *           RemoveDirectoryTransactedA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedA(LPCSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_a(path), transaction);
    return RemoveDirectoryA(path);
}

/***********************************************************************
 *           RemoveDirectoryTransactedW   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDirectoryTransactedW(LPCWSTR path, HANDLE transaction)
{
    FIXME("(%s %p): semi-stub\n", debugstr_w(path), transaction);
    return RemoveDirectoryW(path);
}

/*************************************************************************
 *           CreateHardLinkTransactedA   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedA(LPCSTR link, LPCSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_a(link), debugstr_a(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*************************************************************************
 *           CreateHardLinkTransactedW   (KERNEL32.@)
 */
BOOL WINAPI CreateHardLinkTransactedW(LPCWSTR link, LPCWSTR target, LPSECURITY_ATTRIBUTES sa, HANDLE transaction)
{
    FIXME("(%s %s %p %p): stub\n", debugstr_w(link), debugstr_w(target), sa, transaction);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

