/* Unit test suite for Ntdll file functions
 *
 * Copyright 2007 Jeff Latimer
 * Copyright 2007 Andrey Turkin
 * Copyright 2008 Jeff Zaroyko
 * Copyright 2011 Dmitry Timoshkov
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
 * NOTES
 * We use function pointers here as there is no import library for NTDLL on
 * windows.
 */

#include <stdio.h>
#include <stdarg.h>

#include "ntstatus.h"
/* Define WIN32_NO_STATUS so MSVC does not give us duplicate macro
 * definition errors when we get to winnt.h
 */
#define WIN32_NO_STATUS

#include "wine/test.h"
#include "winternl.h"
#include "winuser.h"
#include "winioctl.h"
#include "winnls.h"

#ifndef IO_COMPLETION_ALL_ACCESS
#define IO_COMPLETION_ALL_ACCESS 0x001F0003
#endif

static BOOL     (WINAPI * pGetVolumePathNameW)(LPCWSTR, LPWSTR, DWORD);
static UINT     (WINAPI *pGetSystemWow64DirectoryW)( LPWSTR, UINT );

static VOID     (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirectionEx)( ULONG, ULONG * );

static NTSTATUS (WINAPI *pNtAllocateReserveObject)( HANDLE *, const OBJECT_ATTRIBUTES *, MEMORY_RESERVE_OBJECT_TYPE );
static NTSTATUS (WINAPI *pNtCreateMailslotFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtCreateFile)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
static NTSTATUS (WINAPI *pNtOpenFile)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
static NTSTATUS (WINAPI *pNtDeleteFile)(POBJECT_ATTRIBUTES ObjectAttributes);
static NTSTATUS (WINAPI *pNtReadFile)(HANDLE hFile, HANDLE hEvent,
                                      PIO_APC_ROUTINE apc, void* apc_user,
                                      PIO_STATUS_BLOCK io_status, void* buffer, ULONG length,
                                      PLARGE_INTEGER offset, PULONG key);
static NTSTATUS (WINAPI *pNtWriteFile)(HANDLE hFile, HANDLE hEvent,
                                       PIO_APC_ROUTINE apc, void* apc_user,
                                       PIO_STATUS_BLOCK io_status,
                                       const void* buffer, ULONG length,
                                       PLARGE_INTEGER offset, PULONG key);
static NTSTATUS (WINAPI *pNtCancelIoFile)(HANDLE hFile, PIO_STATUS_BLOCK io_status);
static NTSTATUS (WINAPI *pNtCancelIoFileEx)(HANDLE hFile, PIO_STATUS_BLOCK iosb, PIO_STATUS_BLOCK io_status);
static NTSTATUS (WINAPI *pNtClose)( PHANDLE );
static NTSTATUS (WINAPI *pNtFsControlFile) (HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, PVOID apc_context, PIO_STATUS_BLOCK io, ULONG code, PVOID in_buffer, ULONG in_size, PVOID out_buffer, ULONG out_size);

static NTSTATUS (WINAPI *pNtCreateIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI *pNtOpenIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtQueryIoCompletion)(HANDLE, IO_COMPLETION_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtRemoveIoCompletion)(HANDLE, PULONG_PTR, PULONG_PTR, PIO_STATUS_BLOCK, PLARGE_INTEGER);
static NTSTATUS (WINAPI *pNtRemoveIoCompletionEx)(HANDLE,FILE_IO_COMPLETION_INFORMATION*,ULONG,ULONG*,LARGE_INTEGER*,BOOLEAN);
static NTSTATUS (WINAPI *pNtSetIoCompletion)(HANDLE, ULONG_PTR, ULONG_PTR, NTSTATUS, SIZE_T);
static NTSTATUS (WINAPI *pNtSetIoCompletionEx)(HANDLE, HANDLE, ULONG_PTR, ULONG_PTR, NTSTATUS, SIZE_T);
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryAttributesFile)(const OBJECT_ATTRIBUTES*,FILE_BASIC_INFORMATION*);
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static NTSTATUS (WINAPI *pNtQueryVolumeInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryFullAttributesFile)(const OBJECT_ATTRIBUTES*, FILE_NETWORK_OPEN_INFORMATION*);
static NTSTATUS (WINAPI *pNtFlushBuffersFile)(HANDLE, IO_STATUS_BLOCK*);
static NTSTATUS (WINAPI *pNtQueryEaFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,BOOLEAN,PVOID,ULONG,PULONG,BOOLEAN);

static WCHAR fooW[] = {'f','o','o',0};

static inline BOOL is_signaled( HANDLE obj )
{
    return WaitForSingleObject( obj, 0 ) == WAIT_OBJECT_0;
}

#define TEST_BUF_LEN 3

static HANDLE create_temp_file( ULONG flags )
{
    char path[MAX_PATH], buffer[MAX_PATH];
    HANDLE handle;

    GetTempPathA( MAX_PATH, path );
    GetTempFileNameA( path, "foo", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         flags | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    return (handle == INVALID_HANDLE_VALUE) ? 0 : handle;
}

#define CVALUE_FIRST 0xfffabbcc
#define CKEY_FIRST 0x1030341
#define CKEY_SECOND 0x132E46

static ULONG get_pending_msgs(HANDLE h)
{
    NTSTATUS res;
    ULONG a, req;

    res = pNtQueryIoCompletion( h, IoCompletionBasicInformation, &a, sizeof(a), &req );
    ok( res == STATUS_SUCCESS, "NtQueryIoCompletion failed: %lx\n", res );
    if (res != STATUS_SUCCESS) return -1;
    ok( req == sizeof(a), "Unexpected response size: %lx\n", req );
    return a;
}

static void WINAPI apc( void *arg, IO_STATUS_BLOCK *iosb, ULONG reserved )
{
    int *count = arg;

    trace( "apc called block %p iosb.status %lx iosb.info %Iu\n",
           iosb, iosb->Status, iosb->Information );
    (*count)++;
    ok( !reserved, "reserved is not 0: %lx\n", reserved );
}

static void create_file_test(void)
{
    static const WCHAR systemrootW[] = {'\\','S','y','s','t','e','m','R','o','o','t',
                                        '\\','f','a','i','l','i','n','g',0};
    static const WCHAR questionmarkInvalidNameW[] = {'a','f','i','l','e','?',0};
    static const WCHAR pipeInvalidNameW[]  = {'a','|','b',0};
    static const WCHAR pathInvalidNtW[] = {'\\','\\','?','\\',0};
    static const WCHAR pathInvalidNt2W[] = {'\\','?','?','\\',0};
    static const WCHAR pathInvalidDosW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
    static const char testdata[] = "Hello World";
    FILE_NETWORK_OPEN_INFORMATION info;
    NTSTATUS status;
    HANDLE dir, file;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    LARGE_INTEGER offset;
    char buf[32];
    DWORD ret;

    GetCurrentDirectoryW( MAX_PATH, path );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* try various open modes and options on directories */
    status = pNtCreateFile( &dir, GENERIC_READ|GENERIC_WRITE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    io.Status = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtReadFile( dir, NULL, NULL, NULL, &io, buf, sizeof(buf), &offset, NULL );
    ok( status == STATUS_INVALID_DEVICE_REQUEST || status == STATUS_PENDING, "NtReadFile error %08lx\n", status );
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject( dir, 1000 );
        ok( ret == WAIT_OBJECT_0, "WaitForSingleObject error %lu\n", ret );
        ok( io.Status == STATUS_INVALID_DEVICE_REQUEST,
            "expected STATUS_INVALID_DEVICE_REQUEST, got %08lx\n", io.Status );
    }

    io.Status = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtWriteFile( dir, NULL, NULL, NULL, &io, testdata, sizeof(testdata), &offset, NULL);
    todo_wine
    ok( status == STATUS_INVALID_DEVICE_REQUEST || status == STATUS_PENDING, "NtWriteFile error %08lx\n", status );
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject( dir, 1000 );
        ok( ret == WAIT_OBJECT_0, "WaitForSingleObject error %lu\n", ret );
        ok( io.Status == STATUS_INVALID_DEVICE_REQUEST,
            "expected STATUS_INVALID_DEVICE_REQUEST, got %08lx\n", io.Status );
    }

    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_CREATE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN_IF, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_SUPERSEDE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE_IF, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN, 0, NULL, 0 );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_CREATE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN_IF, 0, NULL, 0 );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_SUPERSEDE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE_IF, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlFreeUnicodeString( &nameW );

    pRtlInitUnicodeString( &nameW, systemrootW );
    attr.Length = sizeof(attr);
    attr.RootDirectory = NULL;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    dir = NULL;
    status = pNtCreateFile( &dir, FILE_APPEND_DATA, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, 0,
                            FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    todo_wine
    ok( status == STATUS_INVALID_PARAMETER,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    /* Invalid chars in file/dirnames */
    pRtlDosPathNameToNtPathName_U(questionmarkInvalidNameW, &nameW, NULL, NULL);
    attr.ObjectName = &nameW;
    status = pNtCreateFile(&dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status);

    status = pNtCreateFile(&file, GENERIC_WRITE|SYNCHRONIZE, &attr, &io, NULL, 0,
                           0, FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status);
    pRtlFreeUnicodeString(&nameW);

    pRtlDosPathNameToNtPathName_U(pipeInvalidNameW, &nameW, NULL, NULL);
    attr.ObjectName = &nameW;
    status = pNtCreateFile(&dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status);

    status = pNtCreateFile(&file, GENERIC_WRITE|SYNCHRONIZE, &attr, &io, NULL, 0,
                           0, FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status);
    pRtlFreeUnicodeString(&nameW);

    pRtlInitUnicodeString( &nameW, pathInvalidNtW );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    todo_wine ok( status == STATUS_OBJECT_NAME_INVALID,
                  "query %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlInitUnicodeString( &nameW, pathInvalidNt2W );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "query %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlInitUnicodeString( &nameW, pathInvalidDosW );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "query %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
}

static void open_file_test(void)
{
    static const WCHAR testdirW[] = {'o','p','e','n','f','i','l','e','t','e','s','t',0};
    static const char testdata[] = "Hello World";
    NTSTATUS status;
    HANDLE dir, root, handle, file;
    WCHAR path[MAX_PATH], tmpfile[MAX_PATH];
    BYTE data[1024];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    UINT i, len;
    BOOL ret, restart = TRUE;
    DWORD numbytes;

    len = GetWindowsDirectoryW( path, MAX_PATH );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = pNtOpenFile( &dir, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    path[3] = 0;  /* root of the drive */
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    status = pNtOpenFile( &root, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    /* test opening system dir with RootDirectory set to windows dir */
    GetSystemDirectoryW( path, MAX_PATH );
    while (path[len] == '\\') len++;
    nameW.Buffer = path + len;
    nameW.Length = lstrlenW(path + len) * sizeof(WCHAR);
    attr.RootDirectory = dir;
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );

    /* try uppercase name */
    for (i = len; path[i]; i++) if (path[i] >= 'a' && path[i] <= 'z') path[i] -= 'a' - 'A';
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );

    /* try with leading backslash */
    nameW.Buffer--;
    nameW.Length += sizeof(WCHAR);
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( status == STATUS_INVALID_PARAMETER ||
        status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    if (!status) CloseHandle( handle );

    /* try with empty name */
    nameW.Length = 0;
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );
    CloseHandle( dir );

    attr.RootDirectory = 0;
    wcscat( path, L"\\cmd.exe" );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( status == STATUS_NOT_A_DIRECTORY, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );
    pRtlFreeUnicodeString( &nameW );

    wcscat( path, L"\\" );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    status = NtOpenFile( &handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attr, &io,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT );
    ok( status == STATUS_NOT_A_DIRECTORY, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );
    pRtlFreeUnicodeString( &nameW );

    wcscat( path, L"\\cmd.exe" );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    GetTempPathW( MAX_PATH, path );
    lstrcatW( path, testdirW );
    CreateDirectoryW( path, NULL );

    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.RootDirectory = NULL;
    status = pNtOpenFile( &dir, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    GetTempFileNameW( path, fooW, 0, tmpfile );
    file = CreateFileW( tmpfile, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError() );
    numbytes = 0xdeadbeef;
    ret = WriteFile( file, testdata, sizeof(testdata) - 1, &numbytes, NULL );
    ok( ret, "WriteFile failed with error %lu\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to write all data\n" );
    CloseHandle( file );

    /* try open by file id */

    while (!pNtQueryDirectoryFile( dir, NULL, NULL, NULL, &io, data, sizeof(data),
                                   FileIdBothDirectoryInformation, TRUE, NULL, restart ))
    {
        FILE_ID_BOTH_DIRECTORY_INFORMATION *info = (FILE_ID_BOTH_DIRECTORY_INFORMATION *)data;

        restart = FALSE;

        if (!info->FileId.QuadPart) continue;

        nameW.Buffer = (WCHAR *)&info->FileId;
        nameW.Length = sizeof(info->FileId);
        info->FileName[info->FileNameLength/sizeof(WCHAR)] = 0;
        attr.RootDirectory = dir;
        /* We skip 'open' files by not specifying FILE_SHARE_WRITE */
        status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                              FILE_SHARE_READ,
                              FILE_OPEN_BY_FILE_ID |
                              ((info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_DIRECTORY_FILE : 0) );
        ok( status == STATUS_SUCCESS, "open %s failed %lx\n", wine_dbgstr_w(info->FileName), status );
        if (!status)
        {
            BYTE buf[sizeof(FILE_ALL_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

            if (!pNtQueryInformationFile( handle, &io, buf, sizeof(buf), FileAllInformation ))
            {
                FILE_ALL_INFORMATION *fai = (FILE_ALL_INFORMATION *)buf;

                /* check that it's the same file/directory */

                /* don't check the size for directories */
                if (!(info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    ok( info->EndOfFile.QuadPart == fai->StandardInformation.EndOfFile.QuadPart,
                        "mismatched file size for %s\n", wine_dbgstr_w(info->FileName));

                ok( info->CreationTime.QuadPart == fai->BasicInformation.CreationTime.QuadPart,
                    "mismatched creation time for %s\n", wine_dbgstr_w(info->FileName));
            }
            CloseHandle( handle );

            /* try same thing from drive root */
            attr.RootDirectory = root;
            status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  FILE_OPEN_BY_FILE_ID |
                                  ((info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_DIRECTORY_FILE : 0) );
            ok( status == STATUS_SUCCESS || status == STATUS_NOT_IMPLEMENTED,
                "open %s failed %lx\n", wine_dbgstr_w(info->FileName), status );
            if (!status) CloseHandle( handle );
        }
    }

    CloseHandle( dir );
    CloseHandle( root );

    pRtlDosPathNameToNtPathName_U( tmpfile, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = pNtOpenFile( &file, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                         FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    numbytes = 0xdeadbeef;
    memset( data, 0, sizeof(data) );
    ret = ReadFile( file, data, sizeof(data), &numbytes, NULL );
    ok( ret, "ReadFile failed with error %lu\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to read all data\n" );
    ok( !memcmp( data, testdata, sizeof(testdata) - 1 ), "testdata doesn't match\n" );

    nameW.Length = sizeof(fooW) - sizeof(WCHAR);
    nameW.Buffer = fooW;
    attr.RootDirectory = file;
    attr.ObjectName = &nameW;
    status = pNtOpenFile( &root, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                         FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND,
        "expected STATUS_OBJECT_PATH_NOT_FOUND, got %08lx\n", status );

    nameW.Length = 0;
    nameW.Buffer = NULL;
    attr.RootDirectory = file;
    attr.ObjectName = &nameW;
    status = pNtOpenFile( &root, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(tmpfile), status );

    numbytes = SetFilePointer( file, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %lu\n", numbytes );
    numbytes = SetFilePointer( root, 0, 0, FILE_CURRENT );
    ok( numbytes == 0, "SetFilePointer returned %lu\n", numbytes );

    numbytes = 0xdeadbeef;
    memset( data, 0, sizeof(data) );
    ret = ReadFile( root, data, sizeof(data), &numbytes, NULL );
    ok( ret, "ReadFile failed with error %lu\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to read all data\n" );
    ok( !memcmp( data, testdata, sizeof(testdata) - 1 ), "testdata doesn't match\n" );

    numbytes = SetFilePointer( file, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %lu\n", numbytes );
    numbytes = SetFilePointer( root, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %lu\n", numbytes );

    CloseHandle( file );
    CloseHandle( root );
    DeleteFileW( tmpfile );
    RemoveDirectoryW( path );
}

static void delete_file_test(void)
{
    NTSTATUS ret;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR pathW[MAX_PATH];
    WCHAR pathsubW[MAX_PATH];
    static const WCHAR testdirW[] = {'n','t','d','e','l','e','t','e','f','i','l','e',0};
    static const WCHAR subdirW[]  = {'\\','s','u','b',0};

    ret = GetTempPathW(MAX_PATH, pathW);
    if (!ret)
    {
	ok(0, "couldn't get temp dir\n");
	return;
    }
    if (ret + ARRAY_SIZE(testdirW)-1 + ARRAY_SIZE(subdirW)-1 >= MAX_PATH)
    {
	ok(0, "MAX_PATH exceeded in constructing paths\n");
	return;
    }

    lstrcatW(pathW, testdirW);
    lstrcpyW(pathsubW, pathW);
    lstrcatW(pathsubW, subdirW);

    ret = CreateDirectoryW(pathW, NULL);
    ok(ret == TRUE, "couldn't create directory ntdeletefile\n");
    if (!pRtlDosPathNameToNtPathName_U(pathW, &nameW, NULL, NULL))
    {
	ok(0,"RtlDosPathNametoNtPathName_U failed\n");
	return;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* test NtDeleteFile on an empty directory */
    ret = pNtDeleteFile(&attr);
    ok(ret == STATUS_SUCCESS, "NtDeleteFile should succeed in removing an empty directory\n");
    ret = RemoveDirectoryW(pathW);
    ok(ret == FALSE, "expected to fail removing directory, NtDeleteFile should have removed it\n");

    /* test NtDeleteFile on a non-empty directory */
    ret = CreateDirectoryW(pathW, NULL);
    ok(ret == TRUE, "couldn't create directory ntdeletefile ?!\n");
    ret = CreateDirectoryW(pathsubW, NULL);
    ok(ret == TRUE, "couldn't create directory subdir\n");
    ret = pNtDeleteFile(&attr);
    ok(ret == STATUS_SUCCESS, "expected NtDeleteFile to ret STATUS_SUCCESS\n");
    ret = RemoveDirectoryW(pathsubW);
    ok(ret == TRUE, "expected to remove directory ntdeletefile\\sub\n");
    ret = RemoveDirectoryW(pathW);
    ok(ret == TRUE, "expected to remove directory ntdeletefile, NtDeleteFile failed.\n");

    pRtlFreeUnicodeString( &nameW );
}

#define TEST_OVERLAPPED_READ_SIZE 4096

static void read_file_test(void)
{
    DECLSPEC_ALIGN(TEST_OVERLAPPED_READ_SIZE) static unsigned char aligned_buffer[TEST_OVERLAPPED_READ_SIZE];
    const char text[] = "foobar";
    HANDLE handle;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    int apc_count = 0;
    char buffer[128];
    LARGE_INTEGER offset;
    HANDLE event = CreateEventA( NULL, TRUE, FALSE, NULL );

    if (!(handle = create_temp_file( FILE_FLAG_OVERLAPPED ))) return;
    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_PENDING || broken(status == STATUS_SUCCESS) /* before Vista */,
            "wrong status %lx.\n", status );
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %lx\n", iosb.Status );
    ok( iosb.Information == strlen(text), "wrong info %Iu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok(status == STATUS_PENDING
            || broken(status == STATUS_SUCCESS) /* before Vista */,
            "wrong status %lx.\n", status);
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %lx\n", iosb.Status );
    ok( iosb.Information == strlen(text), "wrong info %Iu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read beyond eof */
    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok(status == STATUS_PENDING || broken(status == STATUS_END_OF_FILE) /* before Vista */,
            "expected STATUS_PENDING, got %#lx\n", status);
    if (status == STATUS_PENDING)  /* vista */
    {
        WaitForSingleObject( event, 1000 );
        ok( iosb.Status == STATUS_END_OF_FILE, "wrong status %lx\n", iosb.Status );
        ok( iosb.Information == 0, "wrong info %Iu\n", iosb.Information );
        ok( is_signaled( event ), "event is not signaled\n" );
        ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( apc_count == 1, "apc was not called\n" );
    }
    CloseHandle( handle );

    /* now a non-overlapped file */
    if (!(handle = create_temp_file(0))) return;
    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_END_OF_FILE ||
        status == STATUS_SUCCESS ||
        status == STATUS_PENDING,  /* vista */
        "wrong status %lx\n", status );
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %lx\n", iosb.Status );
    ok( iosb.Information == strlen(text), "wrong info %Iu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %lx\n", iosb.Status );
    ok( iosb.Information == strlen(text), "wrong info %Iu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    todo_wine ok( !apc_count, "apc was called\n" );

    /* read beyond eof */
    apc_count = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok( status == STATUS_END_OF_FILE, "wrong status %lx\n", status );
    ok( iosb.Status == STATUS_END_OF_FILE, "wrong status %lx\n", iosb.Status );
    ok( iosb.Information == 0, "wrong info %Iu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    CloseHandle( handle );

    if (!(handle = create_temp_file(FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING)))
        return;

    apc_count = 0;
    offset.QuadPart = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent(event);
    status = pNtWriteFile(handle, event, apc, &apc_count, &iosb,
            aligned_buffer, sizeof(aligned_buffer), &offset, NULL);
    ok(status == STATUS_END_OF_FILE || status == STATUS_PENDING
            || broken(status == STATUS_SUCCESS) /* before Vista */,
            "Wrong status %lx.\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "Wrong status %lx.\n", iosb.Status);
    ok(iosb.Information == sizeof(aligned_buffer), "Wrong info %Iu.\n", iosb.Information);
    ok(is_signaled(event), "event is not signaled.\n");
    ok(!apc_count, "apc was called.\n");
    SleepEx(1, TRUE); /* alertable sleep */
    ok(apc_count == 1, "apc was not called.\n");

    apc_count = 0;
    offset.QuadPart = 0;
    iosb.Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent(event);
    status = pNtReadFile(handle, event, apc, &apc_count, &iosb,
            aligned_buffer, sizeof(aligned_buffer), &offset, NULL);
    ok(status == STATUS_PENDING, "Wrong status %lx.\n", status);
    WaitForSingleObject(event, 1000);
    ok(iosb.Status == STATUS_SUCCESS, "Wrong status %lx.\n", iosb.Status);
    ok(iosb.Information == sizeof(aligned_buffer), "Wrong info %Iu.\n", iosb.Information);
    ok(is_signaled(event), "event is not signaled.\n");
    ok(!apc_count, "apc was called.\n");
    SleepEx(1, TRUE); /* alertable sleep */
    ok(apc_count == 1, "apc was not called.\n");

    CloseHandle(handle);
    CloseHandle(event);
}

static void append_file_test(void)
{
    static const char text[6] = "foobar";
    HANDLE handle;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER offset;
    char path[MAX_PATH], buffer[MAX_PATH], buf[16];
    DWORD ret;

    GetTempPathA( MAX_PATH, path );
    GetTempFileNameA( path, "foo", 0, buffer );

    handle = CreateFileA(buffer, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    iosb.Status = -1;
    iosb.Information = -1;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text, 2, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 2, "expected 2, got %Iu\n", iosb.Information);

    CloseHandle(handle);

    /* It is possible to open a file with only FILE_APPEND_DATA access flags.
       It matches the O_WRONLY|O_APPEND open() posix behavior */
    handle = CreateFileA(buffer, FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    iosb.Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 1;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 2, 2, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 2, "expected 2, got %Iu\n", iosb.Information);

    ret = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
    ok(ret == 4, "expected 4, got %lu\n", ret);

    iosb.Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 3;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 4, 2, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 2, "expected 2, got %Iu\n", iosb.Information);

    ret = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
    ok(ret == 6, "expected 6, got %lu\n", ret);

    CloseHandle(handle);

    handle = CreateFileA(buffer, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %ld\n", GetLastError());

    memset(buf, 0, sizeof(buf));
    iosb.Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(handle, 0, NULL, NULL, &iosb, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 6, "expected 6, got %Iu\n", iosb.Information);
    buf[6] = 0;
    ok(memcmp(buf, text, 6) == 0, "wrong file contents: %s\n", buf);

    iosb.Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 3, 3, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 3, "expected 3, got %Iu\n", iosb.Information);

    memset(buf, 0, sizeof(buf));
    iosb.Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(handle, 0, NULL, NULL, &iosb, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iosb.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iosb.Status);
    ok(iosb.Information == 6, "expected 6, got %Iu\n", iosb.Information);
    buf[6] = 0;
    ok(memcmp(buf, "barbar", 6) == 0, "wrong file contents: %s\n", buf);

    CloseHandle(handle);
    DeleteFileA(buffer);
}

static void nt_mailslot_test(void)
{
    HANDLE hslot;
    ACCESS_MASK DesiredAccess;
    OBJECT_ATTRIBUTES attr;

    ULONG CreateOptions;
    ULONG MailslotQuota;
    ULONG MaxMessageSize;
    LARGE_INTEGER TimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS rc;
    UNICODE_STRING str;
    WCHAR buffer1[] = { '\\','?','?','\\','M','A','I','L','S','L','O','T','\\',
                        'R',':','\\','F','R','E','D','\0' };

    TimeOut.QuadPart = -1;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    CreateOptions = MailslotQuota = MaxMessageSize = 0;
    DesiredAccess = GENERIC_READ;

    /*
     * Check for NULL pointer handling
     */
    rc = pNtCreateMailslotFile(NULL, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_ACCESS_VIOLATION ||
        rc == STATUS_INVALID_PARAMETER, /* win2k3 */
        "rc = %lx not STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER\n", rc);

    /*
     * Test to see if the Timeout can be NULL
     */
    hslot = (HANDLE)0xdeadbeef;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         NULL);
    ok( rc == STATUS_SUCCESS ||
        rc == STATUS_INVALID_PARAMETER, /* win2k3 */
        "rc = %lx not STATUS_SUCCESS or STATUS_INVALID_PARAMETER\n", rc);
    ok( hslot != 0, "Handle is invalid\n");

    if  ( rc == STATUS_SUCCESS ) pNtClose(hslot);

    /*
     * Test a valid call
     */
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_SUCCESS, "Create MailslotFile failed rc = %lx\n", rc);
    ok( hslot != 0, "Handle is invalid\n");

    rc = pNtClose(hslot);
    ok( rc == STATUS_SUCCESS, "NtClose failed\n");
}

static void WINAPI user_apc_proc(ULONG_PTR arg)
{
    unsigned int *apc_count = (unsigned int *)arg;
    ++*apc_count;
}

static void test_set_io_completion(void)
{
    FILE_IO_COMPLETION_INFORMATION info[2] = {{0}};
    LARGE_INTEGER timeout = {{0}};
    unsigned int apc_count;
    IO_STATUS_BLOCK iosb;
    ULONG_PTR key, value;
    NTSTATUS res;
    ULONG count;
    SIZE_T size = 3;
    HANDLE h, h2;

    if (sizeof(size) > 4) size |= (ULONGLONG)0x12345678 << 32;

    res = pNtCreateIoCompletion( &h2, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
    ok( res == STATUS_SUCCESS, "NtCreateIoCompletion failed: %#lx\n", res );
    ok( h2 && h2 != INVALID_HANDLE_VALUE, "got invalid handle %p\n", h2 );
    res = pNtSetIoCompletion( h2, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );
    res = pNtRemoveIoCompletionEx( h2, info, 2, &count, &timeout, TRUE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );

    res = pNtCreateIoCompletion( &h, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
    ok( res == STATUS_SUCCESS, "NtCreateIoCompletion failed: %#lx\n", res );
    ok( h && h != INVALID_HANDLE_VALUE, "got invalid handle %p\n", h );

    apc_count = 0;
    QueueUserAPC( user_apc_proc, GetCurrentThread(), (ULONG_PTR)&apc_count );
    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    /* APC goes first associated with completion port APC takes priority over pending completion.
     * Even if the thread is associated with some other completion port. */
    ok( res == STATUS_USER_APC, "NtRemoveIoCompletionEx unexpected status %#lx\n", res );
    ok( apc_count == 1, "wrong apc count %u\n", apc_count );

    CloseHandle( h2 );

    apc_count = 0;
    QueueUserAPC( user_apc_proc, GetCurrentThread(), (ULONG_PTR)&apc_count );
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    /* Previous call resulted in STATUS_USER_APC did not associate the thread with the port. */
    ok( res == STATUS_USER_APC, "NtRemoveIoCompletion unexpected status %#lx\n", res );
    ok( apc_count == 1, "wrong apc count %u\n", apc_count );

    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    /* Now the thread is associated. */
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );

    apc_count = 0;
    QueueUserAPC( user_apc_proc, GetCurrentThread(), (ULONG_PTR)&apc_count );
    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    /* After a thread is associated with completion port existing completion is returned if APC is pending. */
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );
    ok( apc_count == 0, "wrong apc count %u\n", apc_count );
    SleepEx( 0, TRUE);
    ok( apc_count == 1, "wrong apc count %u\n", apc_count );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_TIMEOUT, "NtRemoveIoCompletion failed: %#lx\n", res );

    res = pNtSetIoCompletion( h, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %lx\n", res );

    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %ld\n", count );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( key == CKEY_FIRST, "Invalid completion key: %#Ix\n", key );
    ok( iosb.Information == size, "Invalid iosb.Information: %Iu\n", iosb.Information );
    ok( iosb.Status == STATUS_INVALID_DEVICE_REQUEST, "Invalid iosb.Status: %#lx\n", iosb.Status );
    ok( value == CVALUE_FIRST, "Invalid completion value: %#Ix\n", value );

    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    if (!pNtRemoveIoCompletionEx)
    {
        skip("NtRemoveIoCompletionEx() not present\n");
        pNtClose( h );
        return;
    }

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, FALSE );
    ok( res == STATUS_TIMEOUT, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count <= 1, "wrong count %lu\n", count );

    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, FALSE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );
    ok( info[0].CompletionKey == 123, "wrong key %#Ix\n", info[0].CompletionKey );
    ok( info[0].CompletionValue == 456, "wrong value %#Ix\n", info[0].CompletionValue );
    ok( info[0].IoStatusBlock.Information == size, "wrong information %#Ix\n",
        info[0].IoStatusBlock.Information );
    ok( info[0].IoStatusBlock.Status == 789, "wrong status %#lx\n", info[0].IoStatusBlock.Status);

    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    res = pNtSetIoCompletion( h, 12, 34, 56, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, FALSE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 2, "wrong count %lu\n", count );
    ok( info[0].CompletionKey == 123, "wrong key %#Ix\n", info[0].CompletionKey );
    ok( info[0].CompletionValue == 456, "wrong value %#Ix\n", info[0].CompletionValue );
    ok( info[0].IoStatusBlock.Information == size, "wrong information %#Ix\n",
        info[0].IoStatusBlock.Information );
    ok( info[0].IoStatusBlock.Status == 789, "wrong status %#lx\n", info[0].IoStatusBlock.Status);
    ok( info[1].CompletionKey == 12, "wrong key %#Ix\n", info[1].CompletionKey );
    ok( info[1].CompletionValue == 34, "wrong value %#Ix\n", info[1].CompletionValue );
    ok( info[1].IoStatusBlock.Information == size, "wrong information %#Ix\n",
        info[1].IoStatusBlock.Information );
    ok( info[1].IoStatusBlock.Status == 56, "wrong status %#lx\n", info[1].IoStatusBlock.Status);

    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    res = pNtSetIoCompletion( h, 12, 34, 56, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 1, &count, NULL, FALSE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );
    ok( info[0].CompletionKey == 123, "wrong key %#Ix\n", info[0].CompletionKey );
    ok( info[0].CompletionValue == 456, "wrong value %#Ix\n", info[0].CompletionValue );
    ok( info[0].IoStatusBlock.Information == size, "wrong information %#Ix\n",
        info[0].IoStatusBlock.Information );
    ok( info[0].IoStatusBlock.Status == 789, "wrong status %#lx\n", info[0].IoStatusBlock.Status);

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 1, &count, NULL, FALSE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );
    ok( info[0].CompletionKey == 12, "wrong key %#Ix\n", info[0].CompletionKey );
    ok( info[0].CompletionValue == 34, "wrong value %#Ix\n", info[0].CompletionValue );
    ok( info[0].IoStatusBlock.Information == size, "wrong information %#Ix\n",
        info[0].IoStatusBlock.Information );
    ok( info[0].IoStatusBlock.Status == 56, "wrong status %#lx\n", info[0].IoStatusBlock.Status);

    apc_count = 0;
    QueueUserAPC( user_apc_proc, GetCurrentThread(), (ULONG_PTR)&apc_count );

    count = 0xdeadbeef;
    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, FALSE );
    ok( res == STATUS_TIMEOUT, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count <= 1, "wrong count %lu\n", count );
    ok( !apc_count, "wrong apc count %d\n", apc_count );

    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    ok( res == STATUS_USER_APC, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count <= 1, "wrong count %lu\n", count );
    ok( apc_count == 1, "wrong apc count %u\n", apc_count );

    apc_count = 0;
    QueueUserAPC( user_apc_proc, GetCurrentThread(), (ULONG_PTR)&apc_count );

    res = pNtSetIoCompletion( h, 123, 456, 789, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %#lx\n", res );

    res = pNtRemoveIoCompletionEx( h, info, 2, &count, &timeout, TRUE );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletionEx failed: %#lx\n", res );
    ok( count == 1, "wrong count %lu\n", count );
    ok( !apc_count, "wrong apc count %u\n", apc_count );

    SleepEx( 1, TRUE );

    pNtClose( h );
}

static void test_file_io_completion(void)
{
    static const char pipe_name[] = "\\\\.\\pipe\\iocompletiontestnamedpipe";

    IO_STATUS_BLOCK iosb;
    BYTE send_buf[TEST_BUF_LEN], recv_buf[TEST_BUF_LEN];
    FILE_COMPLETION_INFORMATION fci;
    LARGE_INTEGER timeout = {{0}};
    HANDLE server, client;
    ULONG_PTR key, value;
    OVERLAPPED o = {0};
    int apc_count = 0;
    NTSTATUS res;
    DWORD read;
    long count;
    HANDLE h;

    res = pNtCreateIoCompletion( &h, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
    ok( res == STATUS_SUCCESS, "NtCreateIoCompletion failed: %#lx\n", res );
    ok( h && h != INVALID_HANDLE_VALUE, "got invalid handle %p\n", h );
    fci.CompletionPort = h;
    fci.CompletionKey = CKEY_SECOND;

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );
    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    iosb.Status = 0xdeadbeef;
    res = pNtSetInformationFile( server, &iosb, &fci, sizeof(fci), FileCompletionInformation );
    ok( res == STATUS_INVALID_PARAMETER, "NtSetInformationFile failed: %#lx\n", res );
    todo_wine
    ok( iosb.Status == 0xdeadbeef, "wrong status %#lx\n", iosb.Status );
    CloseHandle( client );
    CloseHandle( server );

    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );
    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    iosb.Status = 0xdeadbeef;
    res = pNtSetInformationFile( server, &iosb, &fci, sizeof(fci), FileCompletionInformation );
    ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %#lx\n", res );
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %#lx\n", iosb.Status );

    memset( send_buf, 0, TEST_BUF_LEN );
    memset( recv_buf, 0xde, TEST_BUF_LEN );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );
    ReadFile( server, recv_buf, TEST_BUF_LEN, &read, &o);
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );
    WriteFile( client, send_buf, TEST_BUF_LEN, &read, NULL );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( key == CKEY_SECOND, "Invalid completion key: %#Ix\n", key );
    ok( iosb.Information == 3, "Invalid iosb.Information: %Id\n", iosb.Information );
    ok( iosb.Status == STATUS_SUCCESS, "Invalid iosb.Status: %#lx\n", iosb.Status );
    ok( value == (ULONG_PTR)&o, "Invalid completion value: %#Ix\n", value );
    ok( !memcmp( send_buf, recv_buf, TEST_BUF_LEN ),
            "Receive buffer (%02x %02x %02x) did not match send buffer (%02x %02x %02x)\n",
            recv_buf[0], recv_buf[1], recv_buf[2], send_buf[0], send_buf[1], send_buf[2] );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    memset( send_buf, 0, TEST_BUF_LEN );
    memset( recv_buf, 0xde, TEST_BUF_LEN );
    WriteFile( client, send_buf, 2, &read, NULL );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );
    ReadFile( server, recv_buf, 2, &read, &o);
    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %ld\n", count );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( key == CKEY_SECOND, "Invalid completion key: %#Ix\n", key );
    ok( iosb.Information == 2, "Invalid iosb.Information: %Id\n", iosb.Information );
    ok( iosb.Status == STATUS_SUCCESS, "Invalid iosb.Status: %#lx\n", iosb.Status );
    ok( value == (ULONG_PTR)&o, "Invalid completion value: %#Ix\n", value );
    ok( !memcmp( send_buf, recv_buf, 2 ),
            "Receive buffer (%02x %02x) did not match send buffer (%02x %02x)\n",
            recv_buf[0], recv_buf[1], send_buf[0], send_buf[1] );

    ReadFile( server, recv_buf, TEST_BUF_LEN, &read, &o);
    CloseHandle( server );
    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %ld\n", count );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( key == CKEY_SECOND, "Invalid completion key: %Ix\n", key );
    ok( iosb.Information == 0, "Invalid iosb.Information: %Id\n", iosb.Information );
    ok( iosb.Status == STATUS_PIPE_BROKEN, "Invalid iosb.Status: %lx\n", iosb.Status );
    ok( value == (ULONG_PTR)&o, "Invalid completion value: %Ix\n", value );

    CloseHandle( client );

    /* test associating a completion port with a handle after an async is queued */
    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );
    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    memset( send_buf, 0, TEST_BUF_LEN );
    memset( recv_buf, 0xde, TEST_BUF_LEN );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );
    ReadFile( server, recv_buf, TEST_BUF_LEN, &read, &o);

    iosb.Status = 0xdeadbeef;
    res = pNtSetInformationFile( server, &iosb, &fci, sizeof(fci), FileCompletionInformation );
    ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %lx\n", res );
    ok( iosb.Status == STATUS_SUCCESS, "iosb.Status invalid: %lx\n", iosb.Status );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    WriteFile( client, send_buf, TEST_BUF_LEN, &read, NULL );

    res = pNtRemoveIoCompletion( h, &key, &value, &iosb, &timeout );
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %#lx\n", res );
    ok( key == CKEY_SECOND, "Invalid completion key: %#Ix\n", key );
    ok( iosb.Information == 3, "Invalid iosb.Information: %Id\n", iosb.Information );
    ok( iosb.Status == STATUS_SUCCESS, "Invalid iosb.Status: %#lx\n", iosb.Status );
    ok( value == (ULONG_PTR)&o, "Invalid completion value: %#Ix\n", value );
    ok( !memcmp( send_buf, recv_buf, TEST_BUF_LEN ),
            "Receive buffer (%02x %02x %02x) did not match send buffer (%02x %02x %02x)\n",
            recv_buf[0], recv_buf[1], recv_buf[2], send_buf[0], send_buf[1], send_buf[2] );

    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    /* using APCs on handle with associated completion port is not allowed */
    res = pNtReadFile( server, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
    ok(res == STATUS_INVALID_PARAMETER, "NtReadFile returned %lx\n", res);

    CloseHandle( server );
    CloseHandle( client );

    /* test associating a completion port with a handle after an async using APC is queued */
    server = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               4, 1024, 1024, 1000, NULL );
    ok( server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %lu\n", GetLastError() );
    client = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError() );

    apc_count = 0;
    memset( send_buf, 0, TEST_BUF_LEN );
    memset( recv_buf, 0xde, TEST_BUF_LEN );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    res = pNtReadFile( server, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
    ok(res == STATUS_PENDING, "NtReadFile returned %lx\n", res);

    iosb.Status = 0xdeadbeef;
    res = pNtSetInformationFile( server, &iosb, &fci, sizeof(fci), FileCompletionInformation );
    ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %lx\n", res );
    ok( iosb.Status == STATUS_SUCCESS, "iosb.Status invalid: %lx\n", iosb.Status );
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    WriteFile( client, send_buf, TEST_BUF_LEN, &read, NULL );

    ok(!apc_count, "apc_count = %u\n", apc_count);
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    SleepEx(1, TRUE); /* alertable sleep */
    ok(apc_count == 1, "apc was not called\n");
    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );

    /* using APCs on handle with associated completion port is not allowed */
    res = pNtReadFile( server, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
    ok(res == STATUS_INVALID_PARAMETER, "NtReadFile returned %lx\n", res);

    CloseHandle( server );
    CloseHandle( client );
    pNtClose( h );
}

static void test_file_full_size_information(void)
{
    IO_STATUS_BLOCK io;
    FILE_FS_FULL_SIZE_INFORMATION ffsi;
    FILE_FS_SIZE_INFORMATION fsi;
    HANDLE h;
    NTSTATUS res;

    if(!(h = create_temp_file(0))) return ;

    memset(&ffsi,0,sizeof(ffsi));
    memset(&fsi,0,sizeof(fsi));

    /* Assume No Quota Settings configured on Wine Testbot */
    res = pNtQueryVolumeInformationFile(h, &io, &ffsi, sizeof ffsi, FileFsFullSizeInformation);
    ok(res == STATUS_SUCCESS, "cannot get attributes, res %lx\n", res);
    res = pNtQueryVolumeInformationFile(h, &io, &fsi, sizeof fsi, FileFsSizeInformation);
    ok(res == STATUS_SUCCESS, "cannot get attributes, res %lx\n", res);

    /* Test for FileFsSizeInformation */
    ok(fsi.TotalAllocationUnits.QuadPart > 0,
        "[fsi] TotalAllocationUnits expected positive, got 0x%s\n",
        wine_dbgstr_longlong(fsi.TotalAllocationUnits.QuadPart));
    ok(fsi.AvailableAllocationUnits.QuadPart > 0,
        "[fsi] AvailableAllocationUnits expected positive, got 0x%s\n",
        wine_dbgstr_longlong(fsi.AvailableAllocationUnits.QuadPart));

    /* Assume file system is NTFS */
    ok(fsi.BytesPerSector == 512, "[fsi] BytesPerSector expected 512, got %ld\n",fsi.BytesPerSector);
    ok(fsi.SectorsPerAllocationUnit == 8, "[fsi] SectorsPerAllocationUnit expected 8, got %ld\n",fsi.SectorsPerAllocationUnit);

    ok(ffsi.TotalAllocationUnits.QuadPart > 0,
        "[ffsi] TotalAllocationUnits expected positive, got negative value 0x%s\n",
        wine_dbgstr_longlong(ffsi.TotalAllocationUnits.QuadPart));
    ok(ffsi.CallerAvailableAllocationUnits.QuadPart > 0,
        "[ffsi] CallerAvailableAllocationUnits expected positive, got negative value 0x%s\n",
        wine_dbgstr_longlong(ffsi.CallerAvailableAllocationUnits.QuadPart));
    ok(ffsi.ActualAvailableAllocationUnits.QuadPart > 0,
        "[ffsi] ActualAvailableAllocationUnits expected positive, got negative value 0x%s\n",
        wine_dbgstr_longlong(ffsi.ActualAvailableAllocationUnits.QuadPart));
    ok(ffsi.TotalAllocationUnits.QuadPart == fsi.TotalAllocationUnits.QuadPart,
        "[ffsi] TotalAllocationUnits error fsi:0x%s, ffsi:0x%s\n",
        wine_dbgstr_longlong(fsi.TotalAllocationUnits.QuadPart),
        wine_dbgstr_longlong(ffsi.TotalAllocationUnits.QuadPart));

    /* Assume file system is NTFS */
    ok(ffsi.BytesPerSector == 512, "[ffsi] BytesPerSector expected 512, got %ld\n",ffsi.BytesPerSector);
    ok(ffsi.SectorsPerAllocationUnit == 8, "[ffsi] SectorsPerAllocationUnit expected 8, got %ld\n",ffsi.SectorsPerAllocationUnit);

    CloseHandle( h );
}

static void test_file_basic_information(void)
{
    FILE_BASIC_INFORMATION fbi, fbi2;
    IO_STATUS_BLOCK io;
    HANDLE h;
    int res;
    int attrib_mask = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL;

    if (!(h = create_temp_file(0))) return;

    /* Check default first */
    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes, res %x\n", res);
    ok ( (fbi.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) == FILE_ATTRIBUTE_ARCHIVE,
         "attribute %lx not expected\n", fbi.FileAttributes );

    memset(&fbi2, 0, sizeof(fbi2));
    fbi2.LastWriteTime.QuadPart = -1;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi2, sizeof fbi2, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set -1 write time, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set -1 write time, io.Status is %lx\n", io.Status );

    memset(&fbi2, 0, sizeof(fbi2));
    fbi2.LastAccessTime.QuadPart = 0x200deadcafebeef;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi2, sizeof(fbi2), FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set access time, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set access time, io.Status is %lx\n", io.Status );
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof(fbi), FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get access time, NtQueryInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't get access time, io.Status is %lx\n", io.Status );
    ok ( fbi2.LastAccessTime.QuadPart == fbi.LastAccessTime.QuadPart,
         "access time mismatch, set: %s get: %s\n",
         wine_dbgstr_longlong(fbi2.LastAccessTime.QuadPart),
         wine_dbgstr_longlong(fbi.LastAccessTime.QuadPart) );

    memset(&fbi2, 0, sizeof(fbi2));
    res = pNtQueryInformationFile(h, &io, &fbi2, sizeof fbi2, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get write time 1, res %x\n", res);
    ok ( fbi2.LastWriteTime.QuadPart == fbi.LastWriteTime.QuadPart, "write time mismatch, %s != %s\n",
         wine_dbgstr_longlong(fbi2.LastWriteTime.QuadPart),
         wine_dbgstr_longlong(fbi.LastWriteTime.QuadPart) );

    memset(&fbi2, 0, sizeof(fbi2));
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi2, sizeof fbi2, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set nothing, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set nothing, io.Status is %lx\n", io.Status );

    memset(&fbi2, 0, sizeof(fbi2));
    res = pNtQueryInformationFile(h, &io, &fbi2, sizeof fbi2, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get write time 2, res %x\n", res);
    ok ( fbi2.LastWriteTime.QuadPart == fbi.LastWriteTime.QuadPart, "write time changed, %s != %s\n",
         wine_dbgstr_longlong(fbi2.LastWriteTime.QuadPart),
         wine_dbgstr_longlong(fbi.LastWriteTime.QuadPart) );

    /* Then SYSTEM */
    /* Clear fbi to avoid setting times */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set system attribute, io.Status is %lx\n", io.Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get system attribute\n");
    ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %lx not FILE_ATTRIBUTE_SYSTEM\n", fbi.FileAttributes );

    /* Then HIDDEN */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set hidden attribute, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set hidden attribute, io.Status is %lx\n", io.Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get hidden attribute\n");
    ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %lx not FILE_ATTRIBUTE_HIDDEN\n", fbi.FileAttributes );

    /* Check NORMAL last of all (to make sure we can clear attributes) */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set normal attribute, NtSetInformationFile returned %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set normal attribute, io.Status is %lx\n", io.Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get normal attribute\n");
    todo_wine ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_NORMAL, "attribute %lx not 0\n", fbi.FileAttributes );

    CloseHandle( h );
}

static void test_file_all_information(void)
{
    IO_STATUS_BLOCK io;
    /* FileAllInformation, like FileNameInformation, has a variable-length pathname
     * buffer at the end.  Vista objects with STATUS_BUFFER_OVERFLOW if you
     * don't leave enough room there.
     */
    struct {
      FILE_ALL_INFORMATION fai;
      WCHAR buf[256];
    } fai_buf;
    HANDLE h;
    int res;
    int attrib_mask = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL;

    if (!(h = create_temp_file(0))) return;

    /* Check default first */
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes, res %x\n", res);
    ok ( (fai_buf.fai.BasicInformation.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) == FILE_ATTRIBUTE_ARCHIVE,
         "attribute %lx not expected\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Then SYSTEM */
    /* Clear fbi to avoid setting times */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_INVALID_INFO_CLASS || broken(res == STATUS_NOT_IMPLEMENTED), "shouldn't be able to set FileAllInformation, res %x\n", res);
    todo_wine ok ( io.Status == 0xdeadbeef, "shouldn't be able to set FileAllInformation, io.Status is %lx\n", io.Status);
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %lx\n", io.Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes, res %x\n", res);
    ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %lx not FILE_ATTRIBUTE_SYSTEM\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Then HIDDEN */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %lx\n", io.Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %lx not FILE_ATTRIBUTE_HIDDEN\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Check NORMAL last of all (to make sure we can clear attributes) */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( io.Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %lx\n", io.Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    todo_wine ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_NORMAL, "attribute %lx not FILE_ATTRIBUTE_NORMAL\n", fai_buf.fai.BasicInformation.FileAttributes );

    CloseHandle( h );
}

static void delete_object( WCHAR *path )
{
    BOOL ret = SetFileAttributesW( path, FILE_ATTRIBUTE_NORMAL );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "SetFileAttribute failed with %lu\n", GetLastError() );
    ret = DeleteFileW( path );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_ACCESS_DENIED,
        "DeleteFileW failed with %lu\n", GetLastError() );
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        ret = RemoveDirectoryW( path );
        ok( ret, "RemoveDirectoryW failed with %lu\n", GetLastError() );
    }
}

static void test_file_rename_information(FILE_INFORMATION_CLASS class)
{
    static const WCHAR foo_txtW[] = {'\\','f','o','o','.','t','x','t',0};
    static const WCHAR fooW[] = {'f','o','o',0};
    WCHAR tmp_path[MAX_PATH], oldpath[MAX_PATH + 16], newpath[MAX_PATH + 16], *filename, *p;
    FILE_RENAME_INFORMATION *fri;
    FILE_NAME_INFORMATION *fni;
    BOOL success, fileDeleted;
    UNICODE_STRING name_str;
    HANDLE handle, handle2;
    IO_STATUS_BLOCK io;
    NTSTATUS res;

    GetTempPathW( MAX_PATH, tmp_path );

    /* oldpath is a file, newpath doesn't exist */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );

    if (class == FileRenameInformationEx && (res == STATUS_NOT_IMPLEMENTED || res == STATUS_INVALID_INFO_CLASS))
    {
        win_skip( "FileRenameInformationEx not supported\n" );
        CloseHandle( handle );
        HeapFree( GetProcessHeap(), 0, fri );
        delete_object( oldpath );
        return;
    }

    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, newpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(newpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, Replace = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, Replace = FALSE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, Replace = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath doesn't exist, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, newpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(newpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory (but child object opened), newpath doesn't exist, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    lstrcpyW( newpath, oldpath );
    lstrcatW( newpath, foo_txtW );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef || io.Status == STATUS_ACCESS_DENIED, "io.Status got %lx\n", io.Status );
    todo_wine ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    todo_wine ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    todo_wine ok( fileDeleted, "file should not exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    if (res == STATUS_SUCCESS) /* remove when Wine is fixed */
    {
        lstrcpyW( oldpath, newpath );
        lstrcatW( oldpath, foo_txtW );
        delete_object( oldpath );
    }
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_OBJECT_NAME_COLLISION, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, Replace = FALSE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_OBJECT_NAME_COLLISION, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, Replace = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, Replace = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_ACCESS_DENIED, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_OBJECT_NAME_COLLISION, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, Replace = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_ACCESS_DENIED, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, Replace = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_ACCESS_DENIED, "io.Status got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a directory, Replace = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a directory, Replace = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath doesn't exist, test with RootDir != NULL */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    for (filename = newpath, p = newpath; *p; p++)
        if (*p == '\\') filename = p + 1;
    handle2 = CreateFileW( tmp_path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + lstrlenW(filename) * sizeof(WCHAR) );
    fri->Flags = 0;
    fri->RootDirectory = handle2;
    fri->FileNameLength = lstrlenW(filename) * sizeof(WCHAR);
    memcpy( fri->FileName, filename, fri->FileNameLength );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, newpath + 2), "FileName expected %s, got %s\n",
                  wine_dbgstr_w(newpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath == newpath */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( oldpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = 0;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "got io status %#lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "got status %lx\n", res );
    ok( GetFileAttributesW( oldpath ) != INVALID_FILE_ATTRIBUTES, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
}

static void test_file_rename_information_ex(void)
{
    static const WCHAR fooW[] = {'f','o','o',0};
    WCHAR tmp_path[MAX_PATH], oldpath[MAX_PATH + 16], newpath[MAX_PATH + 16];
    FILE_RENAME_INFORMATION *fri;
    BOOL fileDeleted;
    UNICODE_STRING name_str;
    HANDLE handle, handle2;
    IO_STATUS_BLOCK io;
    NTSTATUS res;

    GetTempPathW( MAX_PATH, tmp_path );

    /* oldpath is a file, newpath is a read-only file, with FILE_RENAME_REPLACE_IF_EXISTS */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    handle2 = CreateFileW( newpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );
    CloseHandle( handle2 );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformationEx );

    if (res == STATUS_NOT_IMPLEMENTED || res == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "FileRenameInformationEx not supported\n" );
        CloseHandle( handle );
        HeapFree( GetProcessHeap(), 0, fri );
        delete_object( oldpath );
        return;
    }

    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a read-only file, with FILE_RENAME_REPLACE_IF_EXISTS and FILE_RENAME_IGNORE_READONLY_ATTRIBUTE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    handle2 = CreateFileW( newpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );
    CloseHandle( handle2 );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fri = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fri->Flags = FILE_RENAME_REPLACE_IF_EXISTS | FILE_RENAME_IGNORE_READONLY_ATTRIBUTE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformationEx );
    ok( io.Status == STATUS_SUCCESS || io.Status == 0xdeadbeef,
        "io.Status expected STATUS_SUCCESS or 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS || res == STATUS_NOT_SUPPORTED,
        "res expected STATUS_SUCCESS or STATUS_NOT_SUPPORTED, got %lx\n", res );

    if (res == STATUS_SUCCESS)
    {
        fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( fileDeleted, "file should not exist\n" );
        fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( !fileDeleted, "file should exist\n" );
    }

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );
}

static void test_file_link_information(FILE_INFORMATION_CLASS class)
{
    static const WCHAR foo_txtW[] = {'\\','f','o','o','.','t','x','t',0};
    static const WCHAR fooW[] = {'f','o','o',0};
    WCHAR tmp_path[MAX_PATH], oldpath[MAX_PATH + 16], newpath[MAX_PATH + 16], *filename, *p;
    FILE_LINK_INFORMATION *fli;
    FILE_NAME_INFORMATION *fni;
    WIN32_FIND_DATAW find_data;
    BOOL success, fileDeleted;
    UNICODE_STRING name_str;
    HANDLE handle, handle2;
    IO_STATUS_BLOCK io;
    NTSTATUS res;

    GetTempPathW( MAX_PATH, tmp_path );

    /* oldpath is a file, newpath doesn't exist */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );

    if (class == FileLinkInformationEx && (res == STATUS_NOT_IMPLEMENTED || res == STATUS_INVALID_INFO_CLASS))
    {
        win_skip( "FileLinkInformationEx not supported\n" );
        CloseHandle( handle );
        HeapFree( GetProcessHeap(), 0, fli );
        delete_object( oldpath );
        return;
    }

    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, oldpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(oldpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, ReplaceIfExists = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, ReplaceIfExists = TRUE, different casing on link */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    wcsrchr( newpath, '\\' )[1] = 'F';
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    handle = FindFirstFileW( newpath, &find_data );
    ok(handle != INVALID_HANDLE_VALUE, "FindFirstFileW: failed, error %ld\n", GetLastError());
    if (handle != INVALID_HANDLE_VALUE)
    {
        todo_wine ok(!lstrcmpW(wcsrchr(newpath, '\\') + 1, find_data.cFileName),
           "Link did not change casing on existing target file: got %s\n", wine_dbgstr_w(find_data.cFileName));
    }

    FindClose( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, ReplaceIfExists = FALSE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a file, ReplaceIfExists = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath doesn't exist, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY ,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, oldpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(oldpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory (but child object opened), newpath doesn't exist, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    lstrcpyW( newpath, oldpath );
    lstrcatW( newpath, foo_txtW );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    DeleteFileW( newpath );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, ReplaceIfExists = FALSE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, ReplaceIfExists = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a file, ReplaceIfExists = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, ReplaceIfExists = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a directory, newpath is a directory, ReplaceIfExists = TRUE, target file opened */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( oldpath );
    success = CreateDirectoryW( oldpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    handle2 = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == 0xdeadbeef || io.Status == STATUS_FILE_IS_A_DIRECTORY,
        "io.Status expected 0xdeadbeef or STATUS_FILE_IS_A_DIRECTORY, got %lx\n", io.Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a directory, ReplaceIfExists = FALSE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a directory, ReplaceIfExists = TRUE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    success = CreateDirectoryW( newpath, NULL );
    ok( success != 0, "failed to create temp directory\n" );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath doesn't exist, test with RootDirectory != NULL */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    for (filename = newpath, p = newpath; *p; p++)
        if (*p == '\\') filename = p + 1;
    handle2 = CreateFileW( tmp_path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + lstrlenW(filename) * sizeof(WCHAR) );
    fli->Flags = 0;
    fli->RootDirectory = handle2;
    fli->FileNameLength = lstrlenW(filename) * sizeof(WCHAR);
    memcpy( fli->FileName, filename, fli->FileNameLength );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, oldpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(oldpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath == newpath */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    pRtlDosPathNameToNtPathName_U( oldpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "got io status %#lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "got status %lx\n", res );

    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "got io status %#lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "got status %lx\n", res );
    ok( GetFileAttributesW( oldpath ) != INVALID_FILE_ATTRIBUTES, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );

    /* oldpath == newpath, different casing on link */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    wcsrchr( oldpath, '\\' )[1] = 'F';
    pRtlDosPathNameToNtPathName_U( oldpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_RENAME_INFORMATION) + name_str.Length );
    fli->Flags = 0;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    todo_wine ok( io.Status == 0xdeadbeef, "got io status %#lx\n", io.Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "got status %lx\n", res );

    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, class );
    ok( io.Status == STATUS_SUCCESS, "got io status %#lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "got status %lx\n", res );
    ok( GetFileAttributesW( oldpath ) != INVALID_FILE_ATTRIBUTES, "file should exist\n" );

    CloseHandle( handle );
    handle = FindFirstFileW( oldpath, &find_data );
    ok(handle != INVALID_HANDLE_VALUE, "FindFirstFileW: failed, error %ld\n", GetLastError());
    if (handle != INVALID_HANDLE_VALUE)
    {
        todo_wine ok(!lstrcmpW(wcsrchr(oldpath, '\\') + 1, find_data.cFileName),
           "Link did not change casing on same file: got %s\n", wine_dbgstr_w(find_data.cFileName));
    }

    FindClose( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
}

static void test_file_link_information_ex(void)
{
    static const WCHAR fooW[] = {'f','o','o',0};
    WCHAR tmp_path[MAX_PATH], oldpath[MAX_PATH + 16], newpath[MAX_PATH + 16];
    FILE_LINK_INFORMATION *fli;
    BOOL fileDeleted;
    UNICODE_STRING name_str;
    HANDLE handle, handle2;
    IO_STATUS_BLOCK io;
    NTSTATUS res;

    GetTempPathW( MAX_PATH, tmp_path );

    /* oldpath is a file, newpath is a read-only file, with FILE_LINK_REPLACE_IF_EXISTS */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    handle2 = CreateFileW( newpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );
    CloseHandle( handle2 );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformationEx );

    if (res == STATUS_NOT_IMPLEMENTED || res == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "FileLinkInformationEx not supported\n" );
        CloseHandle( handle );
        HeapFree( GetProcessHeap(), 0, fli );
        delete_object( oldpath );
        return;
    }

    todo_wine ok( io.Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %lx\n", io.Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );

    /* oldpath is a file, newpath is a read-only file, with FILE_LINK_REPLACE_IF_EXISTS and FILE_LINK_IGNORE_READONLY_ATTRIBUTE */
    res = GetTempFileNameW( tmp_path, fooW, 0, oldpath );
    ok( res != 0, "failed to create temp file\n" );
    handle = CreateFileW( oldpath, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );

    res = GetTempFileNameW( tmp_path, fooW, 0, newpath );
    ok( res != 0, "failed to create temp file\n" );
    DeleteFileW( newpath );
    handle2 = CreateFileW( newpath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0 );
    ok( handle2 != INVALID_HANDLE_VALUE, "CreateFileW failed\n" );
    CloseHandle( handle2 );
    pRtlDosPathNameToNtPathName_U( newpath, &name_str, NULL, NULL );
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->Flags = FILE_LINK_REPLACE_IF_EXISTS | FILE_LINK_IGNORE_READONLY_ATTRIBUTE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    io.Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformationEx );
    ok( io.Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %lx\n", io.Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %lx\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
    HeapFree( GetProcessHeap(), 0, fli );
    delete_object( oldpath );
    delete_object( newpath );
}

static void test_file_both_information(void)
{
    IO_STATUS_BLOCK io;
    FILE_BOTH_DIR_INFORMATION fbi;
    HANDLE h;
    int res;

    if (!(h = create_temp_file(0))) return;

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBothDirectoryInformation);
    ok ( res == STATUS_INVALID_INFO_CLASS || res == STATUS_NOT_IMPLEMENTED, "shouldn't be able to query FileBothDirectoryInformation, res %x\n", res);

    CloseHandle( h );
}

static NTSTATUS nt_get_file_attrs(const char *name, DWORD *attrs)
{
    WCHAR nameW[MAX_PATH];
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, MAX_PATH );

    *attrs = INVALID_FILE_ATTRIBUTES;

    if (!pRtlDosPathNameToNtPathName_U( nameW, &nt_name, NULL, NULL ))
        return STATUS_UNSUCCESSFUL;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtQueryAttributesFile( &attr, &info );
    pRtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS)
        *attrs = info.FileAttributes;

    return status;
}

static void test_file_disposition_information(void)
{
    char tmp_path[MAX_PATH], buffer[MAX_PATH + 16];
    DWORD dirpos;
    HANDLE handle, handle2, handle3, mapping;
    NTSTATUS res;
    IO_STATUS_BLOCK io;
    FILE_DISPOSITION_INFORMATION fdi;
    FILE_DISPOSITION_INFORMATION_EX fdie;
    FILE_STANDARD_INFORMATION fsi;
    BOOL fileDeleted;
    DWORD fdi2, size;
    void *view;

    GetTempPathA( MAX_PATH, tmp_path );

    /* tests for info struct size */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = pNtSetInformationFile( handle, &io, &fdi, 0, FileDispositionInformation );
    todo_wine
    ok( res == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %lx\n", res );
    fdi2 = 0x100;
    res = pNtSetInformationFile( handle, &io, &fdi2, sizeof(fdi2), FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %lx\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* cannot set disposition on file not opened with delete access */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = pNtQueryInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_INVALID_INFO_CLASS || res == STATUS_NOT_IMPLEMENTED, "Unexpected NtQueryInformationFile result (expected STATUS_INVALID_INFO_CLASS, got %lx)\n", res );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_ACCESS_DENIED, "unexpected FileDispositionInformation result (expected STATUS_ACCESS_DENIED, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* can set disposition on file opened with proper access */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    todo_wine
    ok(fsi.DeletePending, "Handle should be marked for deletion\n");
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* file exists until all handles to it get closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    res = nt_get_file_attrs( buffer, &fdi2 );
    todo_wine
    ok( res == STATUS_DELETE_PENDING, "got %#lx\n", res );
    /* can't open the deleted file */
    handle3 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    todo_wine
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    if (handle3 != INVALID_HANDLE_VALUE)
        CloseHandle( handle3 );
    todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got %lu\n", GetLastError());
    /* can't open the deleted file (wrong sharing mode) */
    handle3 = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got %lu\n", GetLastError());
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* file exists until all handles to it get closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    /* can open the marked for delete file (proper sharing mode) */
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open temp file\n" );
    res = nt_get_file_attrs( buffer, &fdi2 );
    ok( res == STATUS_SUCCESS, "got %#lx\n", res );
    /* can't open the marked for delete file (wrong sharing mode) */
    handle3 = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "got %lu\n", GetLastError());
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* file is deleted after handle with FILE_DISPOSITION_POSIX_SEMANTICS is closed */
    /* FileDispositionInformationEx is only supported on Windows 10 build 1809 and later */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open temp file\n" );
    fdie.Flags = FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS;
    res = pNtSetInformationFile( handle, &io, &fdie, sizeof fdie, FileDispositionInformationEx );
    ok( res == STATUS_INVALID_INFO_CLASS || res == STATUS_SUCCESS,
        "unexpected FileDispositionInformationEx result (expected STATUS_SUCCESS or SSTATUS_INVALID_INFO_CLASS, got %lx)\n", res );
    CloseHandle( handle );
    if ( res == STATUS_SUCCESS )
    {
        fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( fileDeleted, "File should have been deleted\n" );
    }
    CloseHandle( handle2 );

    /* file is deleted after handle with FILE_DISPOSITION_POSIX_SEMANTICS is closed */
    /* FileDispositionInformationEx is only supported on Windows 10 build 1809 and later */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open temp file\n" );
    fdie.Flags = FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS;
    res = pNtSetInformationFile( handle, &io, &fdie, sizeof fdie, FileDispositionInformationEx );
    ok( res == STATUS_INVALID_INFO_CLASS || res == STATUS_SUCCESS,
        "unexpected FileDispositionInformationEx result (expected STATUS_SUCCESS or SSTATUS_INVALID_INFO_CLASS, got %lx)\n", res );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    CloseHandle( handle );
    if ( res == STATUS_SUCCESS )
    {
        fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( fileDeleted, "File should have been deleted\n" );
    }

    /* cannot set disposition on readonly file */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    SetFileAttributesA( buffer, FILE_ATTRIBUTE_NORMAL );
    DeleteFileA( buffer );

    /* cannot set disposition on readonly file */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    todo_wine
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    todo_wine
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    SetFileAttributesA( buffer, FILE_ATTRIBUTE_NORMAL );
    DeleteFileA( buffer );

    /* set disposition on readonly file ignoring readonly attribute */
    /* FileDispositionInformationEx is only supported on Windows 10 build 1809 and later */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdie.Flags = FILE_DISPOSITION_DELETE | FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE;
    res = pNtSetInformationFile( handle, &io, &fdie, sizeof fdie, FileDispositionInformationEx );
    ok( res == STATUS_SUCCESS
        || broken(res == STATUS_INVALID_INFO_CLASS) /* win10 1507 & 32-bit 1607 */
        || broken(res == STATUS_NOT_SUPPORTED), /* win10 1709 & 64-bit 1607 */
        "unexpected FileDispositionInformationEx result (expected STATUS_SUCCESS or SSTATUS_INVALID_INFO_CLASS, got %lx)\n", res );
    CloseHandle( handle );
    if ( res == STATUS_SUCCESS )
    {
        fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( fileDeleted, "File should have been deleted\n" );
    }
    SetFileAttributesA( buffer, FILE_ATTRIBUTE_NORMAL );
    DeleteFileA( buffer );

    /* can set disposition on file and then reset it */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* can't reset disposition if delete-on-close flag is specified */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* can't reset disposition on duplicated handle if delete-on-close flag is specified */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    ok( DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &handle2, 0, FALSE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle2, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* can reset delete-on-close flag through FileDispositionInformationEx */
    /* FileDispositionInformationEx is only supported on Windows 10 build 1809 and later */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdie.Flags = FILE_DISPOSITION_ON_CLOSE;
    res = pNtSetInformationFile( handle, &io, &fdie, sizeof fdie, FileDispositionInformationEx );
    ok( res == STATUS_INVALID_INFO_CLASS || res == STATUS_SUCCESS,
        "unexpected FileDispositionInformationEx result (expected STATUS_SUCCESS or SSTATUS_INVALID_INFO_CLASS, got %lx)\n", res );
    CloseHandle( handle );
    if ( res == STATUS_SUCCESS )
    {
        fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
        ok( !fileDeleted, "File shouldn't have been deleted\n" );
        DeleteFileA( buffer );
    }

    /* DeleteFile fails for wrong sharing mode */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fileDeleted = DeleteFileA( buffer );
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "got %lu\n", GetLastError());
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* DeleteFile succeeds for proper sharing mode */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    fileDeleted = DeleteFileA( buffer );
    ok( fileDeleted, "File should have been deleted\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    todo_wine
    ok(fsi.DeletePending, "Handle should be marked for deletion\n");
    res = nt_get_file_attrs( buffer, &fdi2 );
    todo_wine
    ok( res == STATUS_OBJECT_NAME_NOT_FOUND || broken(res == STATUS_DELETE_PENDING), "got %#lx\n", res );
    /* can't open the deleted file */
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    todo_wine
    ok( handle2 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    todo_wine
    ok( GetLastError() == ERROR_FILE_NOT_FOUND || broken(GetLastError() == ERROR_ACCESS_DENIED), "got %lu\n", GetLastError());
    if (handle2 != INVALID_HANDLE_VALUE)
        CloseHandle( handle2);
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );

    /* can set disposition on a directory opened with proper access */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* RemoveDirectory fails for wrong sharing mode */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    fileDeleted = RemoveDirectoryA( buffer );
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "got %lu\n", GetLastError());
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    RemoveDirectoryA( buffer );

    /* RemoveDirectory succeeds for proper sharing mode */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    fileDeleted = RemoveDirectoryA( buffer );
    ok( fileDeleted, "Directory should have been deleted\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    todo_wine
    ok(fsi.DeletePending, "Handle should be marked for deletion\n");
    res = nt_get_file_attrs( buffer, &fdi2 );
    todo_wine
    ok( res == STATUS_OBJECT_NAME_NOT_FOUND || broken(res == STATUS_DELETE_PENDING), "got %#lx\n", res );
    /* can't open the deleted directory */
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    todo_wine
    ok( handle2 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    todo_wine
    ok(GetLastError() == ERROR_FILE_NOT_FOUND || broken(GetLastError() == ERROR_ACCESS_DENIED), "got %lu\n", GetLastError());
    if (handle2 != INVALID_HANDLE_VALUE) CloseHandle( handle2 );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* directory exists until all handles to it get closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle2, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %lx)\n", res );
    res = nt_get_file_attrs( buffer, &fdi2 );
    todo_wine
    ok( res == STATUS_DELETE_PENDING, "got %#lx\n", res );
    /* can't open the deleted directory */
    handle3 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    todo_wine
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    if (handle3 != INVALID_HANDLE_VALUE)
        CloseHandle( handle3 );
    todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got %lu\n", GetLastError());
    /* can't open the deleted directory (wrong sharing mode) */
    handle3 = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "got %lu\n", GetLastError());
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* directory exists until all handles to it get closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    /* can open the marked for delete directory (proper sharing mode) */
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    /* can't open the marked for delete file (wrong sharing mode) */
    handle3 = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle3 == INVALID_HANDLE_VALUE, "CreateFile should fail\n" );
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "got %lu\n", GetLastError());
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* can open a non-empty directory with FILE_FLAG_DELETE_ON_CLOSE */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    dirpos = lstrlenA( buffer );
    lstrcpyA( buffer + dirpos, "\\tst" );
    handle2 = CreateFileA(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    CloseHandle( handle2 );
    buffer[dirpos] = '\0';
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    SetLastError(0xdeadbeef);
    CloseHandle( handle );
    ok(GetLastError() == 0xdeadbeef, "got %lu\n", GetLastError());
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    buffer[dirpos] = '\\';
    fileDeleted = DeleteFileA( buffer );
    ok( fileDeleted, "File should have been deleted\n" );
    buffer[dirpos] = '\0';
    fileDeleted = RemoveDirectoryA( buffer );
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* cannot set disposition on a non-empty directory */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    dirpos = lstrlenA( buffer );
    lstrcpyA( buffer + dirpos, "\\tst" );
    handle2 = CreateFileA(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    CloseHandle( handle2 );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_DIRECTORY_NOT_EMPTY, "unexpected FileDispositionInformation result (expected STATUS_DIRECTORY_NOT_EMPTY, got %lx)\n", res );
    fileDeleted = DeleteFileA( buffer );
    ok( fileDeleted, "File should have been deleted\n" );
    buffer[dirpos] = '\0';
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    fileDeleted = RemoveDirectoryA( buffer );
    ok( fileDeleted, "Directory should have been deleted\n" );

    /* a file with an open mapping handle cannot be deleted */

    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );
    WriteFile(handle, "data", 4, &size, NULL);
    mapping = CreateFileMappingA( handle, NULL, PAGE_READONLY, 0, 4, NULL );
    ok( !!mapping, "failed to create mapping, error %lu\n", GetLastError() );

    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( !res, "got %#lx\n", res );

    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "got %#lx\n", res );
    res = GetFileAttributesA( buffer );
    ok( res != INVALID_FILE_ATTRIBUTES, "expected file to exist\n" );

    CloseHandle( mapping );
    CloseHandle( handle );
    res = DeleteFileA( buffer );
    ok( res, "got error %lu\n", GetLastError() );

    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );
    WriteFile(handle, "data", 4, &size, NULL);
    mapping = CreateFileMappingA( handle, NULL, PAGE_READONLY, 0, 4, NULL );
    ok( !!mapping, "failed to create mapping, error %lu\n", GetLastError() );
    CloseHandle( mapping );

    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( !res, "got %#lx\n", res );

    CloseHandle( handle );
    res = DeleteFileA( buffer );
    ok( !res, "expected failure\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError() );

    /* a file with an open view cannot be deleted */

    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );
    WriteFile(handle, "data", 4, &size, NULL);
    mapping = CreateFileMappingA( handle, NULL, PAGE_READONLY, 0, 4, NULL );
    ok( !!mapping, "failed to create mapping, error %lu\n", GetLastError() );
    view = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4 );
    ok( !!view, "failed to map view, error %lu\n", GetLastError() );
    CloseHandle( mapping );

    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( !res, "got %#lx\n", res );

    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "got %#lx\n", res );
    res = GetFileAttributesA( buffer );
    ok( res != INVALID_FILE_ATTRIBUTES, "expected file to exist\n" );

    UnmapViewOfFile( view );
    CloseHandle( handle );
    res = DeleteFileA( buffer );
    ok( res, "got error %lu\n", GetLastError() );

    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );
    WriteFile(handle, "data", 4, &size, NULL);
    mapping = CreateFileMappingA( handle, NULL, PAGE_READONLY, 0, 4, NULL );
    ok( !!mapping, "failed to create mapping, error %lu\n", GetLastError() );
    view = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4 );
    ok( !!view, "failed to map view, error %lu\n", GetLastError() );
    CloseHandle( mapping );
    UnmapViewOfFile( view );

    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof(fdi), FileDispositionInformation );
    ok( !res, "got %#lx\n", res );

    CloseHandle( handle );
    res = DeleteFileA( buffer );
    ok( !res, "expected failure\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError() );

    /* pending delete flag is shared across handles */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = NtQueryInformationFile(handle2, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    fdi.DoDeleteFile = TRUE;
    res = NtSetInformationFile(handle, &io, &fdi, sizeof(fdi), FileDispositionInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    res = NtQueryInformationFile(handle2, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    todo_wine
    ok(fsi.DeletePending, "Handle should be marked for deletion\n");
    fdi.DoDeleteFile = FALSE;
    res = NtSetInformationFile(handle2, &io, &fdi, sizeof(fdi), FileDispositionInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    CloseHandle(handle);
    CloseHandle(handle2);
    res = GetFileAttributesA( buffer );
    todo_wine
    ok( res != INVALID_FILE_ATTRIBUTES, "expected file to exist\n" );

    /* pending delete flag is shared across handles (even after closing) */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    handle2 = CreateFileA(buffer, DELETE, FILE_SHARE_DELETE | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok( handle2 != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = NtQueryInformationFile(handle2, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    fdi.DoDeleteFile = TRUE;
    res = NtSetInformationFile(handle, &io, &fdi, sizeof(fdi), FileDispositionInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    res = NtQueryInformationFile(handle2, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    todo_wine
    ok(fsi.DeletePending, "Handle should be marked for deletion\n");
    fdi.DoDeleteFile = FALSE;
    res = NtSetInformationFile(handle2, &io, &fdi, sizeof(fdi), FileDispositionInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    CloseHandle(handle2);
    res = NtQueryInformationFile(handle, &io, &fsi, sizeof(fsi), FileStandardInformation);
    ok(res == STATUS_SUCCESS, "NtQueryInformationFile failed %lx\n", res);
    ok(!fsi.DeletePending, "Handle shouldn't be marked for deletion\n");
    CloseHandle(handle);
    res = GetFileAttributesA( buffer );
    todo_wine
    ok( res != INVALID_FILE_ATTRIBUTES, "expected file to exist\n" );
}

static void test_file_name_information(void)
{
    WCHAR *file_name, *volume_prefix, *expected;
    FILE_NAME_INFORMATION *info;
    ULONG old_redir = 1, tmp;
    UINT file_name_size;
    IO_STATUS_BLOCK io;
    UINT info_size;
    HRESULT hr;
    HANDLE h;
    UINT len;

    /* GetVolumePathName is not present before w2k */
    if (!pGetVolumePathNameW) {
        win_skip("GetVolumePathNameW not found\n");
        return;
    }

    file_name_size = GetSystemDirectoryW( NULL, 0 );
    file_name = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*file_name) );
    volume_prefix = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );
    expected = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );

    len = GetSystemDirectoryW( file_name, file_name_size );
    ok(len == file_name_size - 1,
            "GetSystemDirectoryW returned %u, expected %u.\n",
            len, file_name_size - 1);

    len = pGetVolumePathNameW( file_name, volume_prefix, file_name_size );
    ok(len, "GetVolumePathNameW failed.\n");

    len = lstrlenW( volume_prefix );
    if (len && volume_prefix[len - 1] == '\\') --len;
    memcpy( expected, file_name + len, (file_name_size - len - 1) * sizeof(WCHAR) );
    expected[file_name_size - len - 1] = '\0';

    /* A bit more than we actually need, but it keeps the calculation simple. */
    info_size = sizeof(*info) + (file_name_size * sizeof(WCHAR));
    info = HeapAlloc( GetProcessHeap(), 0, info_size );

    if (pRtlWow64EnableFsRedirectionEx) pRtlWow64EnableFsRedirectionEx( TRUE, &old_redir );
    h = CreateFileW( file_name, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    if (pRtlWow64EnableFsRedirectionEx) pRtlWow64EnableFsRedirectionEx( old_redir, &tmp );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open file.\n");

    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info) - 1, FileNameInformation );
    ok(hr == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationFile returned %#lx.\n", hr);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info), FileNameInformation );
    ok(hr == STATUS_BUFFER_OVERFLOW, "NtQueryInformationFile returned %#lx, expected %#lx.\n",
            hr, STATUS_BUFFER_OVERFLOW);
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "io.Status is %#lx, expected %#lx.\n",
            io.Status, STATUS_BUFFER_OVERFLOW);
    ok(info->FileNameLength == lstrlenW( expected ) * sizeof(WCHAR), "info->FileNameLength is %lu\n", info->FileNameLength);
    ok(info->FileName[2] == 0xcccc, "info->FileName[2] is %#x, expected 0xcccc.\n", info->FileName[2]);
    ok(CharLowerW((LPWSTR)(UINT_PTR)info->FileName[1]) == CharLowerW((LPWSTR)(UINT_PTR)expected[1]),
            "info->FileName[1] is %p, expected %p.\n",
            CharLowerW((LPWSTR)(UINT_PTR)info->FileName[1]), CharLowerW((LPWSTR)(UINT_PTR)expected[1]));
    ok(io.Information == sizeof(*info), "io.Information is %Iu\n", io.Information);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileNameInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#lx, expected %#lx.\n", hr, STATUS_SUCCESS);
    ok(io.Status == STATUS_SUCCESS, "io.Status is %#lx, expected %#lx.\n", io.Status, STATUS_SUCCESS);
    ok(info->FileNameLength == lstrlenW( expected ) * sizeof(WCHAR), "info->FileNameLength is %lu\n", info->FileNameLength);
    ok(info->FileName[info->FileNameLength / sizeof(WCHAR)] == 0xcccc, "info->FileName[len] is %#x, expected 0xcccc.\n",
       info->FileName[info->FileNameLength / sizeof(WCHAR)]);
    info->FileName[info->FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->FileName, expected ), "info->FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->FileName ), wine_dbgstr_w( expected ));
    ok(io.Information == FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + info->FileNameLength,
            "io.Information is %Iu, expected %lu.\n",
            io.Information, FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + info->FileNameLength);

    CloseHandle( h );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, expected );
    HeapFree( GetProcessHeap(), 0, volume_prefix );

    if (old_redir || !pGetSystemWow64DirectoryW || !(file_name_size = pGetSystemWow64DirectoryW( NULL, 0 )))
    {
        skip("Not running on WoW64, skipping test.\n");
        HeapFree( GetProcessHeap(), 0, file_name );
        return;
    }

    h = CreateFileW( file_name, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open file.\n");
    HeapFree( GetProcessHeap(), 0, file_name );

    file_name = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*file_name) );
    volume_prefix = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );
    expected = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*expected) );

    len = pGetSystemWow64DirectoryW( file_name, file_name_size );
    ok(len == file_name_size - 1,
            "GetSystemWow64DirectoryW returned %u, expected %u.\n",
            len, file_name_size - 1);

    len = pGetVolumePathNameW( file_name, volume_prefix, file_name_size );
    ok(len, "GetVolumePathNameW failed.\n");

    len = lstrlenW( volume_prefix );
    if (len && volume_prefix[len - 1] == '\\') --len;
    memcpy( expected, file_name + len, (file_name_size - len - 1) * sizeof(WCHAR) );
    expected[file_name_size - len - 1] = '\0';

    info_size = sizeof(*info) + (file_name_size * sizeof(WCHAR));
    info = HeapAlloc( GetProcessHeap(), 0, info_size );

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileNameInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#lx, expected %#lx.\n", hr, STATUS_SUCCESS);
    info->FileName[info->FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->FileName, expected ), "info->FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->FileName ), wine_dbgstr_w( expected ));

    CloseHandle( h );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, expected );
    HeapFree( GetProcessHeap(), 0, volume_prefix );
    HeapFree( GetProcessHeap(), 0, file_name );
}

static void test_file_all_name_information(void)
{
    WCHAR *file_name, *volume_prefix, *expected;
    FILE_ALL_INFORMATION *info;
    ULONG old_redir = 1, tmp;
    UINT file_name_size;
    IO_STATUS_BLOCK io;
    UINT info_size;
    HRESULT hr;
    HANDLE h;
    UINT len;

    /* GetVolumePathName is not present before w2k */
    if (!pGetVolumePathNameW) {
        win_skip("GetVolumePathNameW not found\n");
        return;
    }

    file_name_size = GetSystemDirectoryW( NULL, 0 );
    file_name = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*file_name) );
    volume_prefix = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );
    expected = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );

    len = GetSystemDirectoryW( file_name, file_name_size );
    ok(len == file_name_size - 1,
            "GetSystemDirectoryW returned %u, expected %u.\n",
            len, file_name_size - 1);

    len = pGetVolumePathNameW( file_name, volume_prefix, file_name_size );
    ok(len, "GetVolumePathNameW failed.\n");

    len = lstrlenW( volume_prefix );
    if (len && volume_prefix[len - 1] == '\\') --len;
    memcpy( expected, file_name + len, (file_name_size - len - 1) * sizeof(WCHAR) );
    expected[file_name_size - len - 1] = '\0';

    /* A bit more than we actually need, but it keeps the calculation simple. */
    info_size = sizeof(*info) + (file_name_size * sizeof(WCHAR));
    info = HeapAlloc( GetProcessHeap(), 0, info_size );

    if (pRtlWow64EnableFsRedirectionEx) pRtlWow64EnableFsRedirectionEx( TRUE, &old_redir );
    h = CreateFileW( file_name, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    if (pRtlWow64EnableFsRedirectionEx) pRtlWow64EnableFsRedirectionEx( old_redir, &tmp );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open file.\n");

    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info) - 1, FileAllInformation );
    ok(hr == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationFile returned %#lx, expected %#lx.\n",
            hr, STATUS_INFO_LENGTH_MISMATCH);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info), FileAllInformation );
    ok(hr == STATUS_BUFFER_OVERFLOW, "NtQueryInformationFile returned %#lx, expected %#lx.\n",
            hr, STATUS_BUFFER_OVERFLOW);
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "io.Status is %#lx, expected %#lx.\n",
            io.Status, STATUS_BUFFER_OVERFLOW);
    ok(info->NameInformation.FileNameLength == lstrlenW( expected ) * sizeof(WCHAR),
       "info->NameInformation.FileNameLength is %lu\n", info->NameInformation.FileNameLength );
    ok(info->NameInformation.FileName[2] == 0xcccc,
            "info->NameInformation.FileName[2] is %#x, expected 0xcccc.\n", info->NameInformation.FileName[2]);
    ok(CharLowerW((LPWSTR)(UINT_PTR)info->NameInformation.FileName[1]) == CharLowerW((LPWSTR)(UINT_PTR)expected[1]),
            "info->NameInformation.FileName[1] is %p, expected %p.\n",
            CharLowerW((LPWSTR)(UINT_PTR)info->NameInformation.FileName[1]), CharLowerW((LPWSTR)(UINT_PTR)expected[1]));
    ok(io.Information == sizeof(*info), "io.Information is %Iu\n", io.Information);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileAllInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#lx, expected %#lx.\n", hr, STATUS_SUCCESS);
    ok(io.Status == STATUS_SUCCESS, "io.Status is %#lx, expected %#lx.\n", io.Status, STATUS_SUCCESS);
    ok(info->NameInformation.FileNameLength == lstrlenW( expected ) * sizeof(WCHAR),
       "info->NameInformation.FileNameLength is %lu\n", info->NameInformation.FileNameLength );
    ok(info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] == 0xcccc,
       "info->NameInformation.FileName[len] is %#x, expected 0xcccc.\n",
       info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)]);
    info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->NameInformation.FileName, expected ),
            "info->NameInformation.FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->NameInformation.FileName ), wine_dbgstr_w( expected ));
    ok(io.Information == FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName)
            + info->NameInformation.FileNameLength,
            "io.Information is %Iu\n", io.Information );

    CloseHandle( h );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, expected );
    HeapFree( GetProcessHeap(), 0, volume_prefix );

    if (old_redir || !pGetSystemWow64DirectoryW || !(file_name_size = pGetSystemWow64DirectoryW( NULL, 0 )))
    {
        skip("Not running on WoW64, skipping test.\n");
        HeapFree( GetProcessHeap(), 0, file_name );
        return;
    }

    h = CreateFileW( file_name, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open file.\n");
    HeapFree( GetProcessHeap(), 0, file_name );

    file_name = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*file_name) );
    volume_prefix = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*volume_prefix) );
    expected = HeapAlloc( GetProcessHeap(), 0, file_name_size * sizeof(*expected) );

    len = pGetSystemWow64DirectoryW( file_name, file_name_size );
    ok(len == file_name_size - 1,
            "GetSystemWow64DirectoryW returned %u, expected %u.\n",
            len, file_name_size - 1);

    len = pGetVolumePathNameW( file_name, volume_prefix, file_name_size );
    ok(len, "GetVolumePathNameW failed.\n");

    len = lstrlenW( volume_prefix );
    if (len && volume_prefix[len - 1] == '\\') --len;
    memcpy( expected, file_name + len, (file_name_size - len - 1) * sizeof(WCHAR) );
    expected[file_name_size - len - 1] = '\0';

    info_size = sizeof(*info) + (file_name_size * sizeof(WCHAR));
    info = HeapAlloc( GetProcessHeap(), 0, info_size );

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileAllInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#lx, expected %#lx.\n", hr, STATUS_SUCCESS);
    info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->NameInformation.FileName, expected ), "info->NameInformation.FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->NameInformation.FileName ), wine_dbgstr_w( expected ));

    CloseHandle( h );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, expected );
    HeapFree( GetProcessHeap(), 0, volume_prefix );
    HeapFree( GetProcessHeap(), 0, file_name );
}

#define test_completion_flags(a,b) _test_completion_flags(__LINE__,a,b)
static void _test_completion_flags(unsigned line, HANDLE handle, DWORD expected_flags)
{
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    info.Flags = 0xdeadbeef;
    status = pNtQueryInformationFile(handle, &io, &info, sizeof(info),
                                     FileIoCompletionNotificationInformation);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    ok_(__FILE__,line)(io.Status == STATUS_SUCCESS, "Status = %lx\n", io.Status);
    ok_(__FILE__,line)(io.Information == sizeof(info), "Information = %Iu\n", io.Information);
    ok_(__FILE__,line)((info.Flags & expected_flags) == expected_flags, "got %08lx\n", info.Flags);
}

static void test_file_completion_information(void)
{
    DECLSPEC_ALIGN(TEST_OVERLAPPED_READ_SIZE) static unsigned char aligned_buf[TEST_OVERLAPPED_READ_SIZE];
    static const char pipe_name[] = "\\\\.\\pipe\\test_file_completion_information";
    static const char buf[] = "testdata";
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION info;
    HANDLE port, h, completion, server, client;
    FILE_COMPLETION_INFORMATION fci;
    BYTE recv_buf[TEST_BUF_LEN];
    DWORD num_bytes, flag;
    OVERLAPPED ov, *pov;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    ULONG_PTR key;
    BOOL ret;
    int i;

    if (!(h = create_temp_file(0))) return;

    status = pNtSetInformationFile(h, &io, &info, sizeof(info) - 1, FileIoCompletionNotificationInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH || broken(status == STATUS_INVALID_INFO_CLASS /* XP */),
       "expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
    {
        win_skip("FileIoCompletionNotificationInformation class not supported\n");
        CloseHandle(h);
        return;
    }

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %08lx\n", status);

    CloseHandle(h);
    if (!(h = create_temp_file(FILE_FLAG_OVERLAPPED))) return;

    info.Flags = FILE_SKIP_SET_EVENT_ON_HANDLE;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_SET_EVENT_ON_HANDLE);

    info.Flags = FILE_SKIP_SET_USER_EVENT_ON_FAST_IO;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_SET_EVENT_ON_HANDLE);

    info.Flags = 0;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_SET_EVENT_ON_HANDLE);

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_SET_EVENT_ON_HANDLE | FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    info.Flags = 0xdeadbeef;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_SET_EVENT_ON_HANDLE | FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    CloseHandle(h);
    if (!(h = create_temp_file(FILE_FLAG_OVERLAPPED))) return;
    test_completion_flags(h, 0);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    port = CreateIoCompletionPort(h, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %lu\n", GetLastError());

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* Before Vista */,
                "Unexpected result %#x, GetLastError() %lu.\n", ret, GetLastError());
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %lu\n", GetLastError());
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %lu\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %lu\n", num_bytes);

        key = 0;
        pov = NULL;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %lu\n", GetLastError());
        ok(key == 0xdeadbeef, "expected 0xdeadbeef, got %Ix\n", key);
        ok(pov == &ov, "expected %p, got %p\n", &ov, pov);
    }

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* Before Vista */,
                "Unexpected result %#x, GetLastError() %lu.\n", ret, GetLastError());
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %lu\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %lu\n", num_bytes);

        pov = (void *)0xdeadbeef;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 500);
        ok(!ret, "GetQueuedCompletionStatus succeeded\n");
        ok(pov == NULL, "expected NULL, got %p\n", pov);
    }

    info.Flags = 0;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status);
    test_completion_flags(h, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* Before Vista */,
                "Unexpected result %#x, GetLastError() %lu.\n", ret, GetLastError());
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %lu\n", GetLastError());
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %lu\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %lu\n", num_bytes);

        pov = (void *)0xdeadbeef;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(!ret, "GetQueuedCompletionStatus succeeded\n");
        ok(pov == NULL, "expected NULL, got %p\n", pov);
    }

    CloseHandle(port);
    CloseHandle(h);

    if (!(h = create_temp_file(FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING)))
        return;

    port = CreateIoCompletionPort(h, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %lu.\n", GetLastError());

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %#lx.\n", status);
    test_completion_flags(h, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    ret = WriteFile(h, aligned_buf, sizeof(aligned_buf), &num_bytes, &ov);
    if (!ret && GetLastError() == ERROR_IO_PENDING)
    {
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %lu.\n", GetLastError());
        ok(num_bytes == sizeof(aligned_buf), "expected sizeof(aligned_buf), got %lu.\n", num_bytes);
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %lu.\n", GetLastError());
    }
    ok(num_bytes == sizeof(aligned_buf), "expected sizeof(buf), got %lu.\n", num_bytes);

    SetLastError(0xdeadbeef);
    ret = ReadFile(h, aligned_buf, sizeof(aligned_buf), &num_bytes, &ov);
    ok(!ret && GetLastError() == ERROR_IO_PENDING, "Unexpected result, ret %#x, error %lu.\n",
            ret, GetLastError());
    ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
    ok(ret, "GetOverlappedResult failed, error %lu.\n", GetLastError());
    ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
    ok(ret, "GetQueuedCompletionStatus failed, error %lu.\n", GetLastError());

    CloseHandle(ov.hEvent);
    CloseHandle(port);
    CloseHandle(h);

    /* Test that setting FileCompletionInformation makes an overlapped file signaled unless FILE_SKIP_SET_EVENT_ON_HANDLE is set */
    for (flag = 0; flag <= FILE_SKIP_SET_USER_EVENT_ON_FAST_IO; flag = flag ? flag << 1 : 1)
    {
        winetest_push_context("%#lx", flag);

        status = pNtCreateIoCompletion(&completion, IO_COMPLETION_ALL_ACCESS, NULL, 0);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

        server = CreateNamedPipeA(pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 4, 1024, 1024,
                                  1000, NULL);
        ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed, error %lu.\n", GetLastError());
        client = CreateFileA(pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
        ok(client != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu.\n", GetLastError());

        memset(&ov, 0, sizeof(ov));
        ReadFile(server, recv_buf, TEST_BUF_LEN, &num_bytes, &ov);
        ok(!is_signaled(server), "Expected not signaled.\n");

        info.Flags = flag;
        status = pNtSetInformationFile(server, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        test_completion_flags(server, flag);

        fci.CompletionPort = completion;
        fci.CompletionKey = CKEY_FIRST;
        io.Status = 0xdeadbeef;
        status = pNtSetInformationFile(server, &io, &fci, sizeof(fci), FileCompletionInformation);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        ok(io.Status == STATUS_SUCCESS, "Got unexpected iosb.Status %#lx.\n", io.Status);
        if (flag == FILE_SKIP_SET_EVENT_ON_HANDLE)
            ok(!is_signaled(server), "Expected not signaled.\n");
        else
            ok(is_signaled(server), "Expected signaled.\n");

        status = pNtClose(client);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        status = pNtClose(server);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        status = pNtClose(completion);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        winetest_pop_context();
    }
}

static void test_file_id_information(void)
{
    BY_HANDLE_FILE_INFORMATION info;
    FILE_ID_INFORMATION fid;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD *dwords;
    HANDLE h;
    BOOL ret;

    if (!(h = create_temp_file(0))) return;

    memset( &fid, 0x11, sizeof(fid) );
    status = pNtQueryInformationFile( h, &io, &fid, sizeof(fid), FileIdInformation );
    if (status == STATUS_NOT_IMPLEMENTED || status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "FileIdInformation not supported\n" );
        CloseHandle( h );
        return;
    }

    memset( &info, 0x22, sizeof(info) );
    ret = GetFileInformationByHandle( h, &info );
    ok( ret, "GetFileInformationByHandle failed\n" );

    dwords = (DWORD *)&fid.VolumeSerialNumber;
    ok( dwords[0] == info.dwVolumeSerialNumber, "expected %08lx, got %08lx\n",
        info.dwVolumeSerialNumber, dwords[0] );
    ok( dwords[1] != 0x11111111, "expected != 0x11111111\n" );

    dwords = (DWORD *)&fid.FileId;
    ok( dwords[0] == info.nFileIndexLow, "expected %08lx, got %08lx\n", info.nFileIndexLow, dwords[0] );
    ok( dwords[1] == info.nFileIndexHigh, "expected %08lx, got %08lx\n", info.nFileIndexHigh, dwords[1] );
    ok( dwords[2] == 0, "expected 0, got %08lx\n", dwords[2] );
    ok( dwords[3] == 0, "expected 0, got %08lx\n", dwords[3] );

    CloseHandle( h );
}

static void test_file_access_information(void)
{
    FILE_ACCESS_INFORMATION info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE h;

    if (!(h = create_temp_file(0))) return;

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info) - 1, FileAccessInformation );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %08lx\n", status );

    status = pNtQueryInformationFile( (HANDLE)0xdeadbeef, &io, &info, sizeof(info), FileAccessInformation );
    ok( status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %08lx\n", status );

    memset(&info, 0x11, sizeof(info));
    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileAccessInformation );
    ok( status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", status );
    ok( info.AccessFlags == 0x13019f, "got %08lx\n", info.AccessFlags );

    CloseHandle( h );
}

static void test_file_attribute_tag_information(void)
{
    FILE_ATTRIBUTE_TAG_INFORMATION info;
    FILE_BASIC_INFORMATION fbi = {};
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE h;

    if (!(h = create_temp_file(0))) return;

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info) - 1, FileAttributeTagInformation );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status );

    status = pNtQueryInformationFile( (HANDLE)0xdeadbeef, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx\n", status );

    memset(&info, 0x11, sizeof(info));
    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    info.FileAttributes &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
    ok( info.FileAttributes == FILE_ATTRIBUTE_ARCHIVE, "got attributes %#lx\n", info.FileAttributes );
    ok( !info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag );

    fbi.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    status = pNtSetInformationFile(h, &io, &fbi, sizeof(fbi), FileBasicInformation);
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );

    memset(&info, 0x11, sizeof(info));
    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    todo_wine ok( info.FileAttributes == FILE_ATTRIBUTE_SYSTEM, "got attributes %#lx\n", info.FileAttributes );
    ok( !info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag );

    fbi.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
    status = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );

    memset(&info, 0x11, sizeof(info));
    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    todo_wine ok( info.FileAttributes == FILE_ATTRIBUTE_HIDDEN, "got attributes %#lx\n", info.FileAttributes );
    ok( !info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag );

    CloseHandle( h );
}

static void test_file_stat_information(void)
{
    BY_HANDLE_FILE_INFORMATION info;
    FILE_STAT_INFORMATION fsd;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE h;
    BOOL ret;

    if (!(h = create_temp_file(0))) return;

    memset( &fsd, 0x11, sizeof(fsd) );
    status = pNtQueryInformationFile( h, &io, &fsd, sizeof(fsd), FileStatInformation );
    if (status == STATUS_NOT_IMPLEMENTED || status == STATUS_INVALID_INFO_CLASS)
    {
        win_skip( "FileStatInformation not supported\n" );
        CloseHandle( h );
        return;
    }
    ok( status == STATUS_SUCCESS, "query FileStatInformation returned %#lx\n", status );

    memset( &info, 0x22, sizeof(info) );
    ret = GetFileInformationByHandle( h, &info );
    ok( ret, "GetFileInformationByHandle failed\n" );

    ok( fsd.FileId.u.LowPart == info.nFileIndexLow, "expected %08lx, got %08lx\n", info.nFileIndexLow, fsd.FileId.u.LowPart );
    ok( fsd.FileId.u.HighPart == info.nFileIndexHigh, "expected %08lx, got %08lx\n", info.nFileIndexHigh, fsd.FileId.u.HighPart );
    ok( fsd.CreationTime.u.LowPart == info.ftCreationTime.dwLowDateTime, "expected %08lx, got %08lx\n",
        info.ftCreationTime.dwLowDateTime, fsd.CreationTime.u.LowPart );
    ok( fsd.CreationTime.u.HighPart == info.ftCreationTime.dwHighDateTime, "expected %08lx, got %08lx\n",
        info.ftCreationTime.dwHighDateTime, fsd.CreationTime.u.HighPart );
    ok( fsd.LastAccessTime.u.LowPart == info.ftLastAccessTime.dwLowDateTime, "expected %08lx, got %08lx\n",
        info.ftLastAccessTime.dwLowDateTime, fsd.LastAccessTime.u.LowPart );
    ok( fsd.LastAccessTime.u.HighPart == info.ftLastAccessTime.dwHighDateTime, "expected %08lx, got %08lx\n",
        info.ftLastAccessTime.dwHighDateTime, fsd.LastAccessTime.u.HighPart );
    ok( fsd.LastWriteTime.u.LowPart == info.ftLastWriteTime.dwLowDateTime, "expected %08lx, got %08lx\n",
        info.ftLastWriteTime.dwLowDateTime, fsd.LastWriteTime.u.LowPart );
    ok( fsd.LastWriteTime.u.HighPart == info.ftLastWriteTime.dwHighDateTime, "expected %08lx, got %08lx\n",
        info.ftLastWriteTime.dwHighDateTime, fsd.LastWriteTime.u.HighPart );
    /* TODO: ChangeTime */
    /* TODO: AllocationSize */
    ok( fsd.EndOfFile.u.LowPart == info.nFileSizeLow, "expected %08lx, got %08lx\n", info.nFileSizeLow, fsd.EndOfFile.u.LowPart );
    ok( fsd.EndOfFile.u.HighPart == info.nFileSizeHigh, "expected %08lx, got %08lx\n", info.nFileSizeHigh, fsd.EndOfFile.u.HighPart );
    ok( fsd.FileAttributes == info.dwFileAttributes, "expected %08lx, got %08lx\n", info.dwFileAttributes, fsd.FileAttributes );
    ok( !fsd.ReparseTag, "got reparse tag %#lx\n", fsd.ReparseTag );
    ok( fsd.NumberOfLinks == info.nNumberOfLinks, "expected %08lx, got %08lx\n", info.nNumberOfLinks, fsd.NumberOfLinks );
    ok( fsd.EffectiveAccess == FILE_ALL_ACCESS, "got %08lx\n", fsd.EffectiveAccess );

    CloseHandle( h );
}

static void rename_file( HANDLE h, const WCHAR *filename )
{
    FILE_RENAME_INFORMATION *fri;
    UNICODE_STRING ntpath;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    BOOLEAN ret;
    ULONG size;

    ret = pRtlDosPathNameToNtPathName_U( filename, &ntpath, NULL, NULL );
    ok( ret, "RtlDosPathNameToNtPathName_U failed\n" );

    size = offsetof( FILE_RENAME_INFORMATION, FileName ) + ntpath.Length;
    fri = HeapAlloc( GetProcessHeap(), 0, size );
    ok( fri != NULL, "HeapAlloc failed\n" );
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = ntpath.Length;
    memcpy( fri->FileName, ntpath.Buffer, ntpath.Length );
    pRtlFreeUnicodeString( &ntpath );

    status = pNtSetInformationFile( h, &io, fri, size, FileRenameInformation );
    HeapFree( GetProcessHeap(), 0, fri );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
}

static void test_dotfile_file_attributes(void)
{
    char temppath[MAX_PATH], filename[MAX_PATH];
    WCHAR temppathW[MAX_PATH], filenameW[MAX_PATH];
    FILE_BASIC_INFORMATION info = {};
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD attrs;
    HANDLE h;

    GetTempPathA( MAX_PATH, temppath );
    GetTempFileNameA( temppath, ".foo", 0, filename );
    h = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0 );
    ok( h != INVALID_HANDLE_VALUE, "failed to create temp file\n" );

    status = nt_get_file_attrs(filename, &attrs);
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( !(attrs & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", attrs );

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileBasicInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( !(info.FileAttributes & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", info.FileAttributes );

    info.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    status = pNtSetInformationFile( h, &io, &info, sizeof(info), FileBasicInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );

    status = nt_get_file_attrs(filename, &attrs);
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( attrs & FILE_ATTRIBUTE_SYSTEM, "got attributes %#lx\n", attrs );
    ok( !(attrs & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", attrs );

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileBasicInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( info.FileAttributes & FILE_ATTRIBUTE_SYSTEM, "got attributes %#lx\n", info.FileAttributes );
    ok( !(info.FileAttributes & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", info.FileAttributes );

    CloseHandle( h );

    GetTempPathW( MAX_PATH, temppathW );
    GetTempFileNameW( temppathW, L"foo", 0, filenameW );
    h = CreateFileW( filenameW, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0 );
    ok( h != INVALID_HANDLE_VALUE, "failed to create temp file\n" );

    GetTempFileNameW( temppathW, L".foo", 0, filenameW );
    winetest_push_context("foo -> .foo");
    rename_file( h, filenameW );
    winetest_pop_context();

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileBasicInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( !(info.FileAttributes & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", info.FileAttributes );

    GetTempFileNameW( temppathW, L"foo", 0, filenameW );
    winetest_push_context(".foo -> foo");
    rename_file( h, filenameW );
    winetest_pop_context();

    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileBasicInformation );
    ok( status == STATUS_SUCCESS, "got %#lx\n", status );
    ok( !(info.FileAttributes & FILE_ATTRIBUTE_HIDDEN), "got attributes %#lx\n", info.FileAttributes );

    CloseHandle( h );
}

static void test_file_mode(void)
{
    UNICODE_STRING file_name, pipe_dev_name, mountmgr_dev_name, mailslot_dev_name;
    WCHAR tmp_path[MAX_PATH], dos_file_name[MAX_PATH];
    FILE_MODE_INFORMATION mode;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    HANDLE file;
    unsigned i;
    DWORD res, access;
    NTSTATUS status;

    const struct {
        UNICODE_STRING *file_name;
        ULONG options;
        ULONG mode;
    } option_tests[] = {
        { &file_name, 0, 0 },
        { &file_name, FILE_NON_DIRECTORY_FILE, 0 },
        { &file_name, FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY, FILE_SEQUENTIAL_ONLY },
        { &file_name, FILE_WRITE_THROUGH, FILE_WRITE_THROUGH },
        { &file_name, FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_ALERT },
        { &file_name, FILE_NO_INTERMEDIATE_BUFFERING, FILE_NO_INTERMEDIATE_BUFFERING },
        { &file_name, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE, FILE_SYNCHRONOUS_IO_NONALERT },
        { &file_name, FILE_DELETE_ON_CLOSE, 0 },
        { &file_name, FILE_RANDOM_ACCESS | FILE_NO_COMPRESSION, 0 },
        { &pipe_dev_name, 0, 0 },
        { &pipe_dev_name, FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_ALERT },
        { &mailslot_dev_name, 0, 0 },
        { &mailslot_dev_name, FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_ALERT },
        { &mountmgr_dev_name, 0, 0 },
        { &mountmgr_dev_name, FILE_SYNCHRONOUS_IO_ALERT, FILE_SYNCHRONOUS_IO_ALERT }
    };

    static WCHAR pipe_devW[] = {'\\','?','?','\\','P','I','P','E','\\'};
    static WCHAR mailslot_devW[] = {'\\','?','?','\\','M','A','I','L','S','L','O','T','\\'};
    static WCHAR mountmgr_devW[] =
        {'\\','?','?','\\','M','o','u','n','t','P','o','i','n','t','M','a','n','a','g','e','r'};

    GetTempPathW(MAX_PATH, tmp_path);
    res = GetTempFileNameW(tmp_path, fooW, 0, dos_file_name);
    ok(res, "GetTempFileNameW failed: %lu\n", GetLastError());
    pRtlDosPathNameToNtPathName_U( dos_file_name, &file_name, NULL, NULL );

    pipe_dev_name.Buffer = pipe_devW;
    pipe_dev_name.Length = sizeof(pipe_devW);
    pipe_dev_name.MaximumLength = sizeof(pipe_devW);

    mailslot_dev_name.Buffer = mailslot_devW;
    mailslot_dev_name.Length = sizeof(mailslot_devW);
    mailslot_dev_name.MaximumLength = sizeof(mailslot_devW);

    mountmgr_dev_name.Buffer = mountmgr_devW;
    mountmgr_dev_name.Length = sizeof(mountmgr_devW);
    mountmgr_dev_name.MaximumLength = sizeof(mountmgr_devW);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    for (i = 0; i < ARRAY_SIZE(option_tests); i++)
    {
        attr.ObjectName = option_tests[i].file_name;
        access = SYNCHRONIZE;

        if (option_tests[i].file_name == &file_name)
        {
            file = CreateFileW(dos_file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
            ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
            CloseHandle(file);
            access |= GENERIC_WRITE | DELETE;
        }

        status = pNtOpenFile(&file, access, &attr, &io, 0, option_tests[i].options);
        ok(status == STATUS_SUCCESS, "[%u] NtOpenFile failed: %lx\n", i, status);

        memset(&mode, 0xcc, sizeof(mode));
        status = pNtQueryInformationFile(file, &io, &mode, sizeof(mode), FileModeInformation);
        ok(status == STATUS_SUCCESS, "[%u] can't get FileModeInformation: %lx\n", i, status);
        ok(mode.Mode == option_tests[i].mode, "[%u] Mode = %lx, expected %lx\n",
           i, mode.Mode, option_tests[i].mode);

        pNtClose(file);
        if (option_tests[i].file_name == &file_name)
            DeleteFileW(dos_file_name);
    }

    pRtlFreeUnicodeString(&file_name);
}

static void test_query_volume_information_file(void)
{
    NTSTATUS status;
    HANDLE dir;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    FILE_FS_VOLUME_INFORMATION *ffvi;
    BYTE buf[sizeof(FILE_FS_VOLUME_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    GetWindowsDirectoryW( path, MAX_PATH );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtOpenFile( &dir, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    ZeroMemory( buf, sizeof(buf) );
    io.Status = 0xdadadada;
    io.Information = 0xcacacaca;

    status = pNtQueryVolumeInformationFile( dir, &io, buf, sizeof(buf), FileFsVolumeInformation );

    ffvi = (FILE_FS_VOLUME_INFORMATION *)buf;

    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %ld\n", status);
    ok(io.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %ld\n", io.Status);

    ok(io.Information == (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + ffvi->VolumeLabelLength),
    "expected %ld, got %Iu\n", (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + ffvi->VolumeLabelLength),
     io.Information);

    todo_wine ok(ffvi->VolumeCreationTime.QuadPart != 0, "Missing VolumeCreationTime\n");
    ok(ffvi->VolumeSerialNumber != 0, "Missing VolumeSerialNumber\n");
    ok(ffvi->SupportsObjects == 1,"expected 1, got %d\n", ffvi->SupportsObjects);
    ok(ffvi->VolumeLabelLength == lstrlenW(ffvi->VolumeLabel) * sizeof(WCHAR), "got %ld\n", ffvi->VolumeLabelLength);

    trace("VolumeSerialNumber: %lx VolumeLabelName: %s\n", ffvi->VolumeSerialNumber, wine_dbgstr_w(ffvi->VolumeLabel));

    CloseHandle( dir );
}

static void test_query_attribute_information_file(void)
{
    NTSTATUS status;
    HANDLE dir;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    FILE_FS_ATTRIBUTE_INFORMATION *ffai;
    BYTE buf[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    GetWindowsDirectoryW( path, MAX_PATH );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtOpenFile( &dir, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %lx\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    ZeroMemory( buf, sizeof(buf) );
    io.Status = 0xdadadada;
    io.Information = 0xcacacaca;

    status = pNtQueryVolumeInformationFile( dir, &io, buf, sizeof(buf), FileFsAttributeInformation );

    ffai = (FILE_FS_ATTRIBUTE_INFORMATION *)buf;

    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %ld\n", status);
    ok(io.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %ld\n", io.Status);
    ok(ffai->FileSystemAttributes != 0, "Missing FileSystemAttributes\n");
    ok(ffai->MaximumComponentNameLength != 0, "Missing MaximumComponentNameLength\n");
    ok(ffai->FileSystemNameLength != 0, "Missing FileSystemNameLength\n");

    trace("FileSystemAttributes: %lx MaximumComponentNameLength: %lx FileSystemName: %s\n",
          ffai->FileSystemAttributes, ffai->MaximumComponentNameLength,
          wine_dbgstr_wn(ffai->FileSystemName, ffai->FileSystemNameLength / sizeof(WCHAR)));

    CloseHandle( dir );
}

static void test_NtCreateFile(void)
{
    static const struct test_data
    {
        DWORD disposition, attrib_in, status, result, attrib_out, needs_cleanup;
    } td[] =
    {
    /* 0*/{ FILE_CREATE, FILE_ATTRIBUTE_READONLY, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, FALSE },
    /* 1*/{ FILE_CREATE, 0, STATUS_OBJECT_NAME_COLLISION, 0, 0, TRUE },
    /* 2*/{ FILE_CREATE, 0, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE, FALSE },
    /* 3*/{ FILE_OPEN, FILE_ATTRIBUTE_READONLY, 0, FILE_OPENED, FILE_ATTRIBUTE_ARCHIVE, TRUE },
    /* 4*/{ FILE_OPEN, FILE_ATTRIBUTE_READONLY, STATUS_OBJECT_NAME_NOT_FOUND, 0, 0, FALSE },
    /* 5*/{ FILE_OPEN_IF, 0, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE, FALSE },
    /* 6*/{ FILE_OPEN_IF, FILE_ATTRIBUTE_READONLY, 0, FILE_OPENED, FILE_ATTRIBUTE_ARCHIVE, TRUE },
    /* 7*/{ FILE_OPEN_IF, FILE_ATTRIBUTE_READONLY, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, FALSE },
    /* 8*/{ FILE_OPEN_IF, 0, 0, FILE_OPENED, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, FALSE },
    /* 9*/{ FILE_OVERWRITE, 0, STATUS_ACCESS_DENIED, 0, 0, TRUE },
    /*10*/{ FILE_OVERWRITE, 0, STATUS_OBJECT_NAME_NOT_FOUND, 0, 0, FALSE },
    /*11*/{ FILE_CREATE, 0, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE, FALSE },
    /*12*/{ FILE_OVERWRITE, FILE_ATTRIBUTE_READONLY, 0, FILE_OVERWRITTEN, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, FALSE },
    /*13*/{ FILE_OVERWRITE_IF, 0, STATUS_ACCESS_DENIED, 0, 0, TRUE },
    /*14*/{ FILE_OVERWRITE_IF, 0, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE, FALSE },
    /*15*/{ FILE_OVERWRITE_IF, FILE_ATTRIBUTE_READONLY, 0, FILE_OVERWRITTEN, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, FALSE },
    /*16*/{ FILE_SUPERSEDE, 0, 0, FILE_SUPERSEDED, FILE_ATTRIBUTE_ARCHIVE, FALSE },
    /*17*/{ FILE_SUPERSEDE, FILE_ATTRIBUTE_READONLY, 0, FILE_SUPERSEDED, FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_READONLY, TRUE },
    /*18*/{ FILE_SUPERSEDE, 0, 0, FILE_CREATED, FILE_ATTRIBUTE_ARCHIVE, TRUE }
    };
    static const WCHAR fooW[] = {'f','o','o',0};
    NTSTATUS status;
    HANDLE handle;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    DWORD ret, i;

    GetTempPathW(MAX_PATH, path);
    GetTempFileNameW(path, fooW, 0, path);
    DeleteFileW(path);
    pRtlDosPathNameToNtPathName_U(path, &nameW, NULL, NULL);

    attr.Length = sizeof(attr);
    attr.RootDirectory = NULL;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        status = pNtCreateFile(&handle, GENERIC_READ, &attr, &io, NULL,
                               td[i].attrib_in, FILE_SHARE_READ|FILE_SHARE_WRITE,
                               td[i].disposition, 0, NULL, 0);

        ok(status == td[i].status, "%ld: expected %#lx got %#lx\n", i, td[i].status, status);

        if (!status)
        {
            ok(io.Information == td[i].result,"%ld: expected %#lx got %#Ix\n", i, td[i].result, io.Information);

            ret = GetFileAttributesW(path);
            ret &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
            /* FIXME: leave only 'else' case below once Wine is fixed */
            if (ret != td[i].attrib_out)
            {
                todo_wine
                ok(ret == td[i].attrib_out, "%ld: expected %#lx got %#lx\n", i, td[i].attrib_out, ret);
                SetFileAttributesW(path, td[i].attrib_out);
            }
            else
                ok(ret == td[i].attrib_out, "%ld: expected %#lx got %#lx\n", i, td[i].attrib_out, ret);

            CloseHandle(handle);
        }

        if (td[i].needs_cleanup)
        {
            SetFileAttributesW(path, FILE_ATTRIBUTE_ARCHIVE);
            DeleteFileW(path);
        }
    }

    pRtlFreeUnicodeString( &nameW );
    SetFileAttributesW(path, FILE_ATTRIBUTE_ARCHIVE);
    DeleteFileW( path );

    wcscat( path, L"\\" );
    pRtlDosPathNameToNtPathName_U(path, &nameW, NULL, NULL);

    status = pNtCreateFile( &handle, GENERIC_READ, &attr, &io, NULL,
                            0, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_CREATE, 0, NULL, 0);
    ok( status == STATUS_OBJECT_NAME_INVALID, "failed %s %lx\n", debugstr_w(nameW.Buffer), status );
    status = pNtCreateFile( &handle, GENERIC_READ, &attr, &io, NULL,
                            0, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_CREATE,
                            FILE_DIRECTORY_FILE, NULL, 0);
    ok( !status, "failed %s %lx\n", debugstr_w(nameW.Buffer), status );
    RemoveDirectoryW( path );
}

static void test_read_write(void)
{
    static const char contents[14] = "1234567890abcd";
    char buf[256];
    HANDLE hfile, event;
    OVERLAPPED ovl;
    IO_STATUS_BLOCK iob;
    DWORD ret, bytes, status, off;
    LARGE_INTEGER offset;
    LONG i;

    event = CreateEventA( NULL, TRUE, FALSE, NULL );

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, NULL, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    hfile = create_temp_file(0);
    if (!hfile) return;

    iob.Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_USER_BUFFER, "expected STATUS_INVALID_USER_BUFFER, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtWriteFile(hfile, event, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_USER_BUFFER, "expected STATUS_INVALID_USER_BUFFER, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);
    ok(!is_signaled(event), "event is not signaled\n");

    iob.Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtReadFile(hfile, event, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);
    ok(is_signaled(event), "event is not signaled\n");

    iob.Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtReadFile(hfile, event, NULL, NULL, &iob, (void*)0xdeadbeef, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);
    ok(is_signaled(event), "event is not signaled\n");

    iob.Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, 7, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 7, "expected 7, got %Iu\n", iob.Information);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-1 /* FILE_WRITE_TO_END_OF_FILE */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents + 7, sizeof(contents) - 7, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == sizeof(contents) - 7, "expected sizeof(contents)-7, got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, buf, 0, &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    for (i = -20; i < -1; i++)
    {
        if (i == -2) continue;

        iob.Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%ld: expected STATUS_INVALID_PARAMETER, got %#lx\n", i, status);
        ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
        ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);
    }

    SetFilePointer(hfile, sizeof(contents) - 4, NULL, FILE_BEGIN);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 4, "expected 4, got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", status);
    ok(iob.Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, contents, sizeof(contents), &bytes, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    /* test reading beyond EOF */
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, NULL, 0, &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    ovl.Offset = sizeof(contents);
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_HANDLE_EOF, "expected ERROR_HANDLE_EOF, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);

    ovl.Offset = sizeof(contents);
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, &ovl);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);

    iob.Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", status);
    ok(iob.Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", status);
    ok(iob.Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", status);
    ok(iob.Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);

    for (i = -20; i < 0; i++)
    {
        if (i == -2) continue;

        iob.Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%ld: expected STATUS_INVALID_PARAMETER, got %#lx\n", i, status);
        ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
        ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);
    }

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %Iu\n", iob.Information);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents) - 4;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 4, "expected 4, got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#lx\n", status);
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %Iu\n", iob.Information);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    ovl.Offset = sizeof(contents) - 4;
    ovl.OffsetHigh = 0;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, "ABCD", 4, &bytes, &ovl);
    ok(ret, "WriteFile error %ld\n", GetLastError());
    ok(bytes == 4, "bytes %lu\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(ret, "ReadFile error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %Iu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "ABCD", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %lu\n", off);

    CloseHandle(hfile);

    hfile = create_temp_file(FILE_FLAG_OVERLAPPED);
    if (!hfile) return;

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, buf, 0, &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    ReadFile(hfile, buf, 0, &bytes, &ovl);
    ok(bytes == 0, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, contents, sizeof(contents), &bytes, NULL);
    ok(!ret, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    iob.Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);

    for (i = -20; i < -1; i++)
    {
        iob.Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%ld: expected STATUS_INVALID_PARAMETER, got %#lx\n", i, status);
        ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
        ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);
    }

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
    ok(status == STATUS_PENDING || broken(status == STATUS_SUCCESS) /* before Vista */,
            "expected STATUS_PENDING, got %#lx.\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
    }
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(bytes == 0, "bytes %lu\n", bytes);

    iob.Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %#lx\n", status);
    ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
    ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);

    for (i = -20; i < 0; i++)
    {
        iob.Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%ld: expected STATUS_INVALID_PARAMETER, got %#lx\n", i, status);
        ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
        ok(iob.Information == -1, "expected -1, got %Id\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    /* test reading beyond EOF */
    offset.QuadPart = sizeof(contents);
    ovl.Offset = offset.u.LowPart;
    ovl.OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(!ret, "ReadFile should fail\n");
    ret = GetLastError();
    ok(ret == ERROR_IO_PENDING || broken(ret == ERROR_HANDLE_EOF) /* before Vista */,
            "expected ERROR_IO_PENDING, got %ld\n", ret);
    ok(bytes == 0, "bytes %lu\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(!ret, "GetOverlappedResult should report FALSE\n");
        ok(GetLastError() == ERROR_HANDLE_EOF, "expected ERROR_HANDLE_EOF, got %ld\n", GetLastError());
        ok(bytes == 0, "expected 0, read %lu\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#Ix\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    offset.QuadPart = sizeof(contents);
    ovl.Offset = offset.u.LowPart;
    ovl.OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, &ovl);
    ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* before Vista */,
            "Unexpected result, ret %#lx, GetLastError() %lu.\n", ret, GetLastError());
    ret = GetLastError();
    ok(bytes == 0, "bytes %lu\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(ret, "GetOverlappedResult returned FALSE with %lu (expected TRUE)\n", GetLastError());
        ok(bytes == 0, "expected 0, read %lu\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);
    }

    offset.QuadPart = sizeof(contents);
    ovl.Offset = offset.u.LowPart;
    ovl.OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, NULL, 0, &bytes, &ovl);
    ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* before Vista */,
            "Unexpected result, ret %#lx, GetLastError() %lu.\n", ret, GetLastError());
    ret = GetLastError();
    ok(bytes == 0, "bytes %lu\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(ret, "GetOverlappedResult returned FALSE with %lu (expected TRUE)\n", GetLastError());
        ok(bytes == 0, "expected 0, read %lu\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %Iu\n", ovl.InternalHigh);
    }

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
        ok(iob.Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", iob.Status);
        ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);
    }
    else
    {
        ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", status);
        ok(iob.Status == -1, "expected -1, got %#lx\n", iob.Status);
        ok(iob.Information == -1, "expected -1, got %Iu\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    ok(status == STATUS_PENDING || broken(status == STATUS_SUCCESS) /* before Vista */,
            "expected STATUS_PENDING, got %#lx.\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
        ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
        ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);
    }
    else
    {
        ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
        ok(iob.Information == 0, "expected 0, got %Iu\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* before Vista */,
            "Unexpected result, ret %#lx, GetLastError() %lu.\n", ret, GetLastError());
    if (!ret)
        ok(bytes == 0, "bytes %lu\n", bytes);
    else
        ok(bytes == 14, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %Iu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %Iu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    SetFilePointer(hfile, sizeof(contents) - 4, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);
    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-1 /* FILE_WRITE_TO_END_OF_FILE */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_PENDING || broken(status == STATUS_SUCCESS) /* before Vista */,
            "expected STATUS_PENDING, got %#lx.\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
    }
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == 4, "expected 4, got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    iob.Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_PENDING || broken(status == STATUS_SUCCESS) /* before Vista */,
            "expected STATUS_PENDING, got %#lx.\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
    }
    ok(iob.Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", iob.Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %Iu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    ovl.Offset = sizeof(contents) - 4;
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, "ABCD", 4, &bytes, &ovl);
    ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* before Vista */,
            "Unexpected result %#lx, GetLastError() %lu.\n", ret, GetLastError());
    if (!ret)
    {
        ok(bytes == 0, "bytes %lu\n", bytes);
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
    }
    else ok(bytes == 4, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == 4, "expected 4, got %Iu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %ld\n", GetLastError());
    ok(bytes == 4, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == 4, "expected 4, got %Iu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok((!ret && GetLastError() == ERROR_IO_PENDING) || broken(ret) /* before Vista */,
            "Unexpected result %#lx, GetLastError() %lu.\n", ret, GetLastError());
    if (!ret)
    {
        ok(bytes == 0, "bytes %lu\n", bytes);
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %ld\n", ret);
    }
    else ok(bytes == 14, "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %Iu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %ld\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %lu\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#Ix\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %Iu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "ABCD", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %lu\n", off);

    CloseHandle(event);
    CloseHandle(hfile);
}

static void test_ioctl(void)
{
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);
    FILE_PIPE_PEEK_BUFFER peek_buf;
    IO_STATUS_BLOCK iosb;
    HANDLE file;
    NTSTATUS status;

    file = create_temp_file(FILE_FLAG_OVERLAPPED);
    ok(file != INVALID_HANDLE_VALUE, "could not create temp file\n");

    SetEvent(event);
    status = pNtFsControlFile(file, event, NULL, NULL, &iosb, 0xdeadbeef, 0, 0, 0, 0);
    todo_wine
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtFsControlFile returned %lx\n", status);
    ok(!is_signaled(event), "event is signaled\n");

    status = pNtFsControlFile(file, (HANDLE)0xdeadbeef, NULL, NULL, &iosb, 0xdeadbeef, 0, 0, 0, 0);
    ok(status == STATUS_INVALID_HANDLE, "NtFsControlFile returned %lx\n", status);

    memset(&iosb, 0x55, sizeof(iosb));
    status = pNtFsControlFile(file, NULL, NULL, NULL, &iosb, FSCTL_PIPE_PEEK, NULL, 0,
                             &peek_buf, sizeof(peek_buf));
    todo_wine
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtFsControlFile failed: %lx\n", status);
    ok(iosb.Status == 0x55555555, "iosb.Status = %lx\n", iosb.Status);

    CloseHandle(event);
    CloseHandle(file);
}

static void test_flush_buffers_file(void)
{
    char path[MAX_PATH], buffer[MAX_PATH];
    HANDLE hfile, hfileread;
    NTSTATUS status;
    IO_STATUS_BLOCK io_status_block;

    GetTempPathA(MAX_PATH, path);
    GetTempFileNameA(path, "foo", 0, buffer);
    hfile = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create temp file.\n" );

    hfileread = CreateFileA(buffer, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, 0, NULL);
    ok(hfileread != INVALID_HANDLE_VALUE, "could not open temp file, error %ld.\n", GetLastError());

    status = pNtFlushBuffersFile(hfile, NULL);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER, "got %#lx.\n", status);

    status = pNtFlushBuffersFile(hfile, (IO_STATUS_BLOCK *)0xdeadbeaf);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#lx.\n", status);

    io_status_block.Information = 0xdeadbeef;
    io_status_block.Status = 0xdeadbeef;
    status = pNtFlushBuffersFile(hfile, &io_status_block);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx.\n", status);
    ok(io_status_block.Status == STATUS_SUCCESS, "Got unexpected io_status_block.Status %#lx.\n",
            io_status_block.Status);
    ok(!io_status_block.Information, "Got unexpected io_status_block.Information %#Ix.\n",
            io_status_block.Information);

    status = pNtFlushBuffersFile(hfileread, &io_status_block);
    ok(status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %#lx.\n", status);

    io_status_block.Information = 0xdeadbeef;
    io_status_block.Status = 0xdeadbeef;
    status = pNtFlushBuffersFile(NULL, &io_status_block);
    ok(status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %#lx.\n", status);
    ok(io_status_block.Status == 0xdeadbeef, "Got unexpected io_status_block.Status %#lx.\n",
            io_status_block.Status);
    ok(io_status_block.Information == 0xdeadbeef, "Got unexpected io_status_block.Information %#Ix.\n",
            io_status_block.Information);

    CloseHandle(hfileread);
    CloseHandle(hfile);
    hfile = CreateFileA(buffer, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, 0, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "could not open temp file, error %ld.\n", GetLastError());

    status = pNtFlushBuffersFile(hfile, &io_status_block);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx.\n", status);

    io_status_block.Information = 0xdeadbeef;
    io_status_block.Status = 0xdeadbeef;
    status = pNtFlushBuffersFile((HANDLE)0xdeadbeef, &io_status_block);
    ok(status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %#lx.\n", status);
    ok(io_status_block.Status == 0xdeadbeef, "Got unexpected io_status_block.Status %#lx.\n",
            io_status_block.Status);
    ok(io_status_block.Information == 0xdeadbeef, "Got unexpected io_status_block.Information %#Ix.\n",
            io_status_block.Information);

    CloseHandle(hfile);
    DeleteFileA(buffer);
}

static void test_query_ea(void)
{
#define EA_BUFFER_SIZE 4097
    unsigned char data[EA_BUFFER_SIZE + 8];
    unsigned char *buffer = (void *)(((DWORD_PTR)data + 7) & ~7);
    DWORD buffer_len, i;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    if (!(handle = create_temp_file(0))) return;

    /* test with INVALID_HANDLE_VALUE */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile(INVALID_HANDLE_VALUE, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);
    ok(buffer[0] == 0xcc, "data at position 0 overwritten\n");

    /* test with 0xdeadbeef */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile((void *)0xdeadbeef, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);
    ok(buffer[0] == 0xcc, "data at position 0 overwritten\n");

    /* test without buffer */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryEaFile(handle, &io, NULL, 0, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);

    /* test with zero buffer */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryEaFile(handle, &io, buffer, 0, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);

    /* test with very small buffer */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = 4;
    status = pNtQueryEaFile(handle, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);
    for (i = 0; i < buffer_len && !buffer[i]; i++);
    ok(i == buffer_len,  "expected %lu bytes filled with 0x00, got %lu bytes\n", buffer_len, i);
    ok(buffer[i] == 0xcc, "data at position %u overwritten\n", buffer[i]);

    /* test with very big buffer */
    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile(handle, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %#lx\n", status);
    ok(io.Status == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", io.Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %#Ix\n", io.Information);
    for (i = 0; i < buffer_len && !buffer[i]; i++);
    ok(i == buffer_len,  "expected %lu bytes filled with 0x00, got %lu bytes\n", buffer_len, i);
    ok(buffer[i] == 0xcc, "data at position %u overwritten\n", buffer[i]);

    CloseHandle(handle);
#undef EA_BUFFER_SIZE
}

static void test_file_readonly_access(void)
{
    static const DWORD default_sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    static const WCHAR fooW[] = {'f', 'o', 'o', 0};
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    HANDLE handle;
    NTSTATUS status;
    DWORD ret;

    /* Set up */
    GetTempPathW(MAX_PATH, path);
    GetTempFileNameW(path, fooW, 0, path);
    DeleteFileW(path);
    pRtlDosPathNameToNtPathName_U(path, &nameW, NULL, NULL);

    attr.Length = sizeof(attr);
    attr.RootDirectory = NULL;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = pNtCreateFile(&handle, FILE_GENERIC_WRITE, &attr, &io, NULL, FILE_ATTRIBUTE_READONLY, default_sharing,
                           FILE_CREATE, 0, NULL, 0);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx.\n", status);
    CloseHandle(handle);

    /* NtCreateFile FILE_GENERIC_WRITE */
    status = pNtCreateFile(&handle, FILE_GENERIC_WRITE, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, default_sharing,
                           FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok(status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %#lx.\n", status);

    /* NtCreateFile DELETE without FILE_DELETE_ON_CLOSE */
    status = pNtCreateFile(&handle, DELETE, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, default_sharing, FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx.\n", status);
    CloseHandle(handle);

    /* NtCreateFile DELETE with FILE_DELETE_ON_CLOSE */
    status = pNtCreateFile(&handle, SYNCHRONIZE | DELETE, &attr, &io, NULL, FILE_ATTRIBUTE_NORMAL, default_sharing,
                           FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok(status == STATUS_CANNOT_DELETE, "expected STATUS_CANNOT_DELETE, got %#lx.\n", status);

    /* NtOpenFile GENERIC_WRITE */
    status = pNtOpenFile(&handle, GENERIC_WRITE, &attr, &io, default_sharing, FILE_NON_DIRECTORY_FILE);
    ok(status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %#lx.\n", status);

    /* NtOpenFile DELETE without FILE_DELETE_ON_CLOSE */
    status = pNtOpenFile(&handle, DELETE, &attr, &io, default_sharing, FILE_NON_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx.\n", status);
    CloseHandle(handle);

    /* NtOpenFile DELETE with FILE_DELETE_ON_CLOSE */
    status = pNtOpenFile(&handle, DELETE, &attr, &io, default_sharing, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE);
    ok(status == STATUS_CANNOT_DELETE, "expected STATUS_CANNOT_DELETE, got %#lx.\n", status);

    ret = GetFileAttributesW(path);
    ok(ret & FILE_ATTRIBUTE_READONLY, "got wrong attribute: %#lx.\n", ret);

    /* Clean up */
    pRtlFreeUnicodeString(&nameW);
    SetFileAttributesW(path, FILE_ATTRIBUTE_NORMAL);
    DeleteFileW(path);
}

static void test_mailslot_name(void)
{
    char buffer[1024] = {0};
    const FILE_NAME_INFORMATION *name = (const FILE_NAME_INFORMATION *)buffer;
    HANDLE server, client, device;
    IO_STATUS_BLOCK io;
    NTSTATUS ret;

    server = CreateMailslotA( "\\\\.\\mailslot\\winetest", 100, 1000, NULL );
    ok(server != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = NtQueryInformationFile( server, &io, buffer, 0, FileNameInformation );
    ok(ret == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", ret);

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtQueryInformationFile( server, &io, buffer,
            offsetof(FILE_NAME_INFORMATION, FileName[5]), FileNameInformation );
    todo_wine ok(ret == STATUS_BUFFER_OVERFLOW, "got %#lx\n", ret);
    if (ret == STATUS_BUFFER_OVERFLOW)
    {
        ok(name->FileNameLength == 18, "got length %lu\n", name->FileNameLength);
        ok(!memcmp(name->FileName, L"\\wine", 10), "got %s\n",
                debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));
    }

    memset(buffer, 0xcc, sizeof(buffer));
    ret = NtQueryInformationFile( server, &io, buffer, sizeof(buffer), FileNameInformation );
    todo_wine ok(!ret, "got %#lx\n", ret);
    if (!ret)
    {
        ok(name->FileNameLength == 18, "got length %lu\n", name->FileNameLength);
        ok(!memcmp(name->FileName, L"\\winetest", 18), "got %s\n",
                debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));
    }

    client = CreateFileA( "\\\\.\\mailslot\\winetest", 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    ok(client != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = NtQueryInformationFile( client, &io, buffer, 0, FileNameInformation );
    ok(ret == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", ret);

    ret = NtQueryInformationFile( client, &io, buffer, sizeof(buffer), FileNameInformation );
    todo_wine ok(ret == STATUS_INVALID_PARAMETER || !ret /* win8+ */, "got %#lx\n", ret);
    if (!ret)
    {
        ok(name->FileNameLength == 18, "got length %lu\n", name->FileNameLength);
        ok(!memcmp(name->FileName, L"\\winetest", 18), "got %s\n",
                debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));
    }

    CloseHandle( server );
    CloseHandle( client );

    device = CreateFileA("\\\\.\\mailslot", 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(device != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = NtQueryInformationFile( device, &io, buffer, 0, FileNameInformation );
    ok(ret == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", ret);

    ret = NtQueryInformationFile( device, &io, buffer, sizeof(buffer), FileNameInformation );
    todo_wine ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    CloseHandle( device );
}

static void test_reparse_points(void)
{
    OBJECT_ATTRIBUTES attr;
    HANDLE handle;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    UNICODE_STRING nameW;
    unsigned char reparse_data[1];

    pRtlInitUnicodeString( &nameW, L"\\??\\C:\\" );
    InitializeObjectAttributes( &attr, &nameW, 0, NULL, NULL );

    status = pNtOpenFile( &handle, READ_CONTROL, &attr, &io, 0, 0 );
    ok( !status, "open %s failed %#lx\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtFsControlFile( handle, NULL, NULL, NULL, &io, FSCTL_GET_REPARSE_POINT, NULL, 0, NULL, 0 );
    ok( status == STATUS_INVALID_USER_BUFFER, "expected %#lx, got %#lx\n", STATUS_INVALID_USER_BUFFER, status );

    status = pNtFsControlFile( handle, NULL, NULL, NULL, &io, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse_data, 0 );
    ok( status == STATUS_INVALID_USER_BUFFER, "expected %#lx, got %#lx\n", STATUS_INVALID_USER_BUFFER, status );

    /* a volume cannot be a reparse point by definition */
    status = pNtFsControlFile( handle, NULL, NULL, NULL, &io, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse_data, 1 );
    ok( status == STATUS_NOT_A_REPARSE_POINT, "expected %#lx, got %#lx\n", STATUS_NOT_A_REPARSE_POINT, status );

    CloseHandle( handle );
}

static void test_set_io_completion_ex(void)
{
    HANDLE completion, completion_reserve, apc_reserve;
    LARGE_INTEGER timeout = {{0}};
    IO_STATUS_BLOCK iosb;
    ULONG_PTR key, value;
    NTSTATUS status;
    SIZE_T size = 3;

    if (!pNtSetIoCompletionEx || !pNtAllocateReserveObject)
    {
        win_skip("NtSetIoCompletionEx() or NtAllocateReserveObject() is unavailable.\n");
        return;
    }

    if (sizeof(size) > 4) size |= (ULONGLONG)0x12345678 << 32;

    status = pNtCreateIoCompletion(&completion, IO_COMPLETION_ALL_ACCESS, NULL, 0);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = pNtAllocateReserveObject(&completion_reserve, NULL, MemoryReserveObjectTypeIoCompletion);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = pNtAllocateReserveObject(&apc_reserve, NULL, MemoryReserveObjectTypeUserApc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    /* Parameter checks */
    status = pNtSetIoCompletionEx(NULL, completion_reserve, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected status %#lx.\n", status);

    status = pNtSetIoCompletionEx(INVALID_HANDLE_VALUE, completion_reserve, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    status = pNtSetIoCompletionEx(completion, NULL, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected status %#lx.\n", status);

    status = pNtSetIoCompletionEx(completion, INVALID_HANDLE_VALUE, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    status = pNtSetIoCompletionEx(completion, apc_reserve, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    /* Normal call */
    status = pNtSetIoCompletionEx(completion, completion_reserve, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    status = pNtRemoveIoCompletion(completion, &key, &value, &iosb, &timeout);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(key == CKEY_FIRST, "Invalid completion key: %#Ix\n", key);
    ok(iosb.Information == size, "Invalid iosb.Information: %Iu\n", iosb.Information);
    ok(iosb.Status == STATUS_INVALID_DEVICE_REQUEST, "Invalid iosb.Status: %#lx\n", iosb.Status);
    ok(value == CVALUE_FIRST, "Invalid completion value: %#Ix\n", value);

    CloseHandle(apc_reserve);
    CloseHandle(completion_reserve);
    CloseHandle(completion);
}

START_TEST(file)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pGetVolumePathNameW = (void *)GetProcAddress(hkernel32, "GetVolumePathNameW");
    pGetSystemWow64DirectoryW = (void *)GetProcAddress(hkernel32, "GetSystemWow64DirectoryW");

    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlWow64EnableFsRedirectionEx = (void *)GetProcAddress(hntdll, "RtlWow64EnableFsRedirectionEx");
    pNtAllocateReserveObject= (void *)GetProcAddress(hntdll, "NtAllocateReserveObject");
    pNtCreateMailslotFile   = (void *)GetProcAddress(hntdll, "NtCreateMailslotFile");
    pNtCreateFile           = (void *)GetProcAddress(hntdll, "NtCreateFile");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtDeleteFile           = (void *)GetProcAddress(hntdll, "NtDeleteFile");
    pNtReadFile             = (void *)GetProcAddress(hntdll, "NtReadFile");
    pNtWriteFile            = (void *)GetProcAddress(hntdll, "NtWriteFile");
    pNtCancelIoFile         = (void *)GetProcAddress(hntdll, "NtCancelIoFile");
    pNtCancelIoFileEx       = (void *)GetProcAddress(hntdll, "NtCancelIoFileEx");
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pNtFsControlFile        = (void *)GetProcAddress(hntdll, "NtFsControlFile");
    pNtCreateIoCompletion   = (void *)GetProcAddress(hntdll, "NtCreateIoCompletion");
    pNtOpenIoCompletion     = (void *)GetProcAddress(hntdll, "NtOpenIoCompletion");
    pNtQueryIoCompletion    = (void *)GetProcAddress(hntdll, "NtQueryIoCompletion");
    pNtRemoveIoCompletion   = (void *)GetProcAddress(hntdll, "NtRemoveIoCompletion");
    pNtRemoveIoCompletionEx = (void *)GetProcAddress(hntdll, "NtRemoveIoCompletionEx");
    pNtSetIoCompletion      = (void *)GetProcAddress(hntdll, "NtSetIoCompletion");
    pNtSetIoCompletionEx    = (void *)GetProcAddress(hntdll, "NtSetIoCompletionEx");
    pNtSetInformationFile   = (void *)GetProcAddress(hntdll, "NtSetInformationFile");
    pNtQueryAttributesFile  = (void *)GetProcAddress(hntdll, "NtQueryAttributesFile");
    pNtQueryInformationFile = (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pNtQueryVolumeInformationFile = (void *)GetProcAddress(hntdll, "NtQueryVolumeInformationFile");
    pNtQueryFullAttributesFile = (void *)GetProcAddress(hntdll, "NtQueryFullAttributesFile");
    pNtFlushBuffersFile = (void *)GetProcAddress(hntdll, "NtFlushBuffersFile");
    pNtQueryEaFile          = (void *)GetProcAddress(hntdll, "NtQueryEaFile");

    test_read_write();
    test_NtCreateFile();
    create_file_test();
    open_file_test();
    delete_file_test();
    read_file_test();
    append_file_test();
    nt_mailslot_test();
    test_set_io_completion();
    test_set_io_completion_ex();
    test_file_io_completion();
    test_file_basic_information();
    test_file_all_information();
    test_file_both_information();
    test_file_name_information();
    test_file_full_size_information();
    test_file_all_name_information();
    test_file_rename_information(FileRenameInformation);
    test_file_rename_information(FileRenameInformationEx);
    test_file_rename_information_ex();
    test_file_link_information(FileLinkInformation);
    test_file_link_information(FileLinkInformationEx);
    test_file_link_information_ex();
    test_file_disposition_information();
    test_file_completion_information();
    test_file_id_information();
    test_file_access_information();
    test_file_attribute_tag_information();
    test_file_stat_information();
    test_dotfile_file_attributes();
    test_file_mode();
    test_file_readonly_access();
    test_query_volume_information_file();
    test_query_attribute_information_file();
    test_ioctl();
    test_query_ea();
    test_flush_buffers_file();
    test_mailslot_name();
    test_reparse_points();
}
