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
#ifndef __REACTOS__
#include "ntifs.h"
#else
/* FIXME: Inspect */
typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  _ANONYMOUS_UNION union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#endif

#ifndef IO_COMPLETION_ALL_ACCESS
#define IO_COMPLETION_ALL_ACCESS 0x001F0003
#endif

static BOOL     (WINAPI * pGetVolumePathNameW)(LPCWSTR, LPWSTR, DWORD);
static UINT     (WINAPI *pGetSystemWow64DirectoryW)( LPWSTR, UINT );

static VOID     (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static NTSTATUS (WINAPI *pRtlWow64EnableFsRedirectionEx)( ULONG, ULONG * );

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
static NTSTATUS (WINAPI *pNtSetIoCompletion)(HANDLE, ULONG_PTR, ULONG_PTR, NTSTATUS, SIZE_T);
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static NTSTATUS (WINAPI *pNtQueryVolumeInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryFullAttributesFile)(const OBJECT_ATTRIBUTES*, FILE_NETWORK_OPEN_INFORMATION*);
static NTSTATUS (WINAPI *pNtFlushBuffersFile)(HANDLE, IO_STATUS_BLOCK*);
static NTSTATUS (WINAPI *pNtQueryEaFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,BOOLEAN,PVOID,ULONG,PULONG,BOOLEAN);

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

static ULONG_PTR completionKey;
static IO_STATUS_BLOCK ioSb;
static ULONG_PTR completionValue;

static ULONG get_pending_msgs(HANDLE h)
{
    NTSTATUS res;
    ULONG a, req;

    res = pNtQueryIoCompletion( h, IoCompletionBasicInformation, &a, sizeof(a), &req );
    ok( res == STATUS_SUCCESS, "NtQueryIoCompletion failed: %x\n", res );
    if (res != STATUS_SUCCESS) return -1;
    ok( req == sizeof(a), "Unexpected response size: %x\n", req );
    return a;
}

static BOOL get_msg(HANDLE h)
{
    LARGE_INTEGER timeout = {{-10000000*3}};
    DWORD res = pNtRemoveIoCompletion( h, &completionKey, &completionValue, &ioSb, &timeout);
    ok( res == STATUS_SUCCESS, "NtRemoveIoCompletion failed: %x\n", res );
    if (res != STATUS_SUCCESS)
    {
        completionKey = completionValue = 0;
        memset(&ioSb, 0, sizeof(ioSb));
        return FALSE;
    }
    return TRUE;
}


static void WINAPI apc( void *arg, IO_STATUS_BLOCK *iosb, ULONG reserved )
{
    int *count = arg;

    trace( "apc called block %p iosb.status %x iosb.info %lu\n",
           iosb, U(*iosb).Status, iosb->Information );
    (*count)++;
    ok( !reserved, "reserved is not 0: %x\n", reserved );
}

static void create_file_test(void)
{
    static const WCHAR notepadW[] = {'n','o','t','e','p','a','d','.','e','x','e',0};
    static const WCHAR systemrootW[] = {'\\','S','y','s','t','e','m','R','o','o','t',
                                        '\\','f','a','i','l','i','n','g',0};
    static const WCHAR systemrootExplorerW[] = {'\\','S','y','s','t','e','m','R','o','o','t',
                                               '\\','e','x','p','l','o','r','e','r','.','e','x','e',0};
    static const WCHAR questionmarkInvalidNameW[] = {'a','f','i','l','e','?',0};
    static const WCHAR pipeInvalidNameW[]  = {'a','|','b',0};
    static const WCHAR pathInvalidNtW[] = {'\\','\\','?','\\',0};
    static const WCHAR pathInvalidNt2W[] = {'\\','?','?','\\',0};
    static const WCHAR pathInvalidDosW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
    static const char testdata[] = "Hello World";
    static const WCHAR sepW[] = {'\\',0};
    FILE_NETWORK_OPEN_INFORMATION info;
    NTSTATUS status;
    HANDLE dir, file;
    WCHAR path[MAX_PATH], temp[MAX_PATH];
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
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    U(io).Status = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtReadFile( dir, NULL, NULL, NULL, &io, buf, sizeof(buf), &offset, NULL );
    ok( status == STATUS_INVALID_DEVICE_REQUEST || status == STATUS_PENDING, "NtReadFile error %08x\n", status );
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject( dir, 1000 );
        ok( ret == WAIT_OBJECT_0, "WaitForSingleObject error %u\n", ret );
        ok( U(io).Status == STATUS_INVALID_DEVICE_REQUEST,
            "expected STATUS_INVALID_DEVICE_REQUEST, got %08x\n", U(io).Status );
    }

    U(io).Status = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtWriteFile( dir, NULL, NULL, NULL, &io, testdata, sizeof(testdata), &offset, NULL);
    todo_wine
    ok( status == STATUS_INVALID_DEVICE_REQUEST || status == STATUS_PENDING, "NtWriteFile error %08x\n", status );
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject( dir, 1000 );
        ok( ret == WAIT_OBJECT_0, "WaitForSingleObject error %u\n", ret );
        ok( U(io).Status == STATUS_INVALID_DEVICE_REQUEST,
            "expected STATUS_INVALID_DEVICE_REQUEST, got %08x\n", U(io).Status );
    }

    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_CREATE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN_IF, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_SUPERSEDE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE_IF, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_INVALID_PARAMETER, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN, 0, NULL, 0 );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_CREATE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN_IF, 0, NULL, 0 );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( dir );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_SUPERSEDE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OVERWRITE_IF, 0, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_ACCESS_DENIED,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

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
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    /* Invalid chars in file/dirnames */
    pRtlDosPathNameToNtPathName_U(questionmarkInvalidNameW, &nameW, NULL, NULL);
    attr.ObjectName = &nameW;
    status = pNtCreateFile(&dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status);

    status = pNtCreateFile(&file, GENERIC_WRITE|SYNCHRONIZE, &attr, &io, NULL, 0,
                           0, FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status);
    pRtlFreeUnicodeString(&nameW);

    pRtlDosPathNameToNtPathName_U(pipeInvalidNameW, &nameW, NULL, NULL);
    attr.ObjectName = &nameW;
    status = pNtCreateFile(&dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status);

    status = pNtCreateFile(&file, GENERIC_WRITE|SYNCHRONIZE, &attr, &io, NULL, 0,
                           0, FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok(status == STATUS_OBJECT_NAME_INVALID,
       "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status);
    pRtlFreeUnicodeString(&nameW);

    pRtlInitUnicodeString( &nameW, pathInvalidNtW );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    todo_wine ok( status == STATUS_OBJECT_NAME_INVALID,
                  "query %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlInitUnicodeString( &nameW, pathInvalidNt2W );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "query %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlInitUnicodeString( &nameW, pathInvalidDosW );
    status = pNtCreateFile( &dir, GENERIC_READ|SYNCHRONIZE, &attr, &io, NULL, 0,
                            FILE_SHARE_READ, FILE_CREATE,
                            FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_OBJECT_NAME_INVALID,
        "query %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    GetWindowsDirectoryW( path, MAX_PATH );
    path[2] = 0;
    ok( QueryDosDeviceW( path, temp, MAX_PATH ),
        "QueryDosDeviceW failed with error %u\n", GetLastError() );
    lstrcatW( temp, sepW );
    lstrcatW( temp, path+3 );
    lstrcatW( temp, sepW );
    lstrcatW( temp, notepadW );

    pRtlInitUnicodeString( &nameW, temp );
    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_SUCCESS,
        "query %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );

    pRtlInitUnicodeString( &nameW, systemrootExplorerW );
    status = pNtQueryFullAttributesFile( &attr, &info );
    ok( status == STATUS_SUCCESS,
        "query %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
}

static void open_file_test(void)
{
    static const char testdata[] = "Hello World";
    static WCHAR fooW[] = {'f','o','o',0};
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
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = pNtOpenFile( &dir, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    path[3] = 0;  /* root of the drive */
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    status = pNtOpenFile( &root, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    /* test opening system dir with RootDirectory set to windows dir */
    GetSystemDirectoryW( path, MAX_PATH );
    while (path[len] == '\\') len++;
    nameW.Buffer = path + len;
    nameW.Length = lstrlenW(path + len) * sizeof(WCHAR);
    attr.RootDirectory = dir;
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );

    /* try uppercase name */
    for (i = len; path[i]; i++) if (path[i] >= 'a' && path[i] <= 'z') path[i] -= 'a' - 'A';
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );

    /* try with leading backslash */
    nameW.Buffer--;
    nameW.Length += sizeof(WCHAR);
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( status == STATUS_INVALID_PARAMETER ||
        status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    if (!status) CloseHandle( handle );

    /* try with empty name */
    nameW.Length = 0;
    status = pNtOpenFile( &handle, GENERIC_READ, &attr, &io,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    CloseHandle( handle );

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
        ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED || status == STATUS_NOT_IMPLEMENTED || status == STATUS_SHARING_VIOLATION,
            "open %s failed %x\n", wine_dbgstr_w(info->FileName), status );
        if (status == STATUS_NOT_IMPLEMENTED)
        {
            win_skip( "FILE_OPEN_BY_FILE_ID not supported\n" );
            break;
        }
        if (status == STATUS_SHARING_VIOLATION)
            trace( "%s is currently open\n", wine_dbgstr_w(info->FileName) );
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
                "open %s failed %x\n", wine_dbgstr_w(info->FileName), status );
            if (!status) CloseHandle( handle );
        }
    }

    CloseHandle( dir );
    CloseHandle( root );

    GetTempPathW( MAX_PATH, path );
    GetTempFileNameW( path, fooW, 0, tmpfile );
    pRtlDosPathNameToNtPathName_U( tmpfile, &nameW, NULL, NULL );

    file = CreateFileW( tmpfile, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError() );
    numbytes = 0xdeadbeef;
    ret = WriteFile( file, testdata, sizeof(testdata) - 1, &numbytes, NULL );
    ok( ret, "WriteFile failed with error %u\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to write all data\n" );
    CloseHandle( file );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = pNtOpenFile( &file, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                         FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    numbytes = 0xdeadbeef;
    memset( data, 0, sizeof(data) );
    ret = ReadFile( file, data, sizeof(data), &numbytes, NULL );
    ok( ret, "ReadFile failed with error %u\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to read all data\n" );
    ok( !memcmp( data, testdata, sizeof(testdata) - 1 ), "testdata doesn't match\n" );

    nameW.Length = sizeof(fooW) - sizeof(WCHAR);
    nameW.Buffer = fooW;
    attr.RootDirectory = file;
    attr.ObjectName = &nameW;
    status = pNtOpenFile( &root, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                         FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND,
        "expected STATUS_OBJECT_PATH_NOT_FOUND, got %08x\n", status );

    nameW.Length = 0;
    nameW.Buffer = NULL;
    attr.RootDirectory = file;
    attr.ObjectName = &nameW;
    status = pNtOpenFile( &root, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &io,
                          FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(tmpfile), status );

    numbytes = SetFilePointer( file, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %u\n", numbytes );
    numbytes = SetFilePointer( root, 0, 0, FILE_CURRENT );
    ok( numbytes == 0, "SetFilePointer returned %u\n", numbytes );

    numbytes = 0xdeadbeef;
    memset( data, 0, sizeof(data) );
    ret = ReadFile( root, data, sizeof(data), &numbytes, NULL );
    ok( ret, "ReadFile failed with error %u\n", GetLastError() );
    ok( numbytes == sizeof(testdata) - 1, "failed to read all data\n" );
    ok( !memcmp( data, testdata, sizeof(testdata) - 1 ), "testdata doesn't match\n" );

    numbytes = SetFilePointer( file, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %u\n", numbytes );
    numbytes = SetFilePointer( root, 0, 0, FILE_CURRENT );
    ok( numbytes == sizeof(testdata) - 1, "SetFilePointer returned %u\n", numbytes );

    CloseHandle( file );
    CloseHandle( root );
    DeleteFileW( tmpfile );
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
    if (ret + sizeof(testdirW)/sizeof(WCHAR)-1 + sizeof(subdirW)/sizeof(WCHAR)-1 >= MAX_PATH)
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

static void read_file_test(void)
{
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
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_SUCCESS || status == STATUS_PENDING, "wrong status %x\n", status );
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok( status == STATUS_SUCCESS ||
        status == STATUS_PENDING, /* vista */
        "wrong status %x\n", status );
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read beyond eof */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok(status == STATUS_PENDING || status == STATUS_END_OF_FILE /* before Vista */, "expected STATUS_PENDING or STATUS_END_OF_FILE, got %#x\n", status);
    if (status == STATUS_PENDING)  /* vista */
    {
        WaitForSingleObject( event, 1000 );
        ok( U(iosb).Status == STATUS_END_OF_FILE, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
        ok( is_signaled( event ), "event is not signaled\n" );
        ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( apc_count == 1, "apc was not called\n" );
    }
    CloseHandle( handle );

    /* now a non-overlapped file */
    if (!(handle = create_temp_file(0))) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    status = pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_END_OF_FILE ||
        status == STATUS_SUCCESS ||
        status == STATUS_PENDING,  /* vista */
        "wrong status %x\n", status );
    if (status == STATUS_PENDING) WaitForSingleObject( event, 1000 );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, strlen(text) + 10, &offset, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_SUCCESS, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == strlen(text), "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    todo_wine ok( !apc_count, "apc was called\n" );

    /* read beyond eof */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    ResetEvent( event );
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    ok( status == STATUS_END_OF_FILE, "wrong status %x\n", status );
    ok( U(iosb).Status == STATUS_END_OF_FILE, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    CloseHandle( handle );

    CloseHandle( event );
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
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    U(iosb).Status = -1;
    iosb.Information = -1;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text, 2, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 2, "expected 2, got %lu\n", iosb.Information);

    CloseHandle(handle);

    /* It is possible to open a file with only FILE_APPEND_DATA access flags.
       It matches the O_WRONLY|O_APPEND open() posix behavior */
    handle = CreateFileA(buffer, FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    U(iosb).Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 1;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 2, 2, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 2, "expected 2, got %lu\n", iosb.Information);

    ret = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
    ok(ret == 4, "expected 4, got %u\n", ret);

    U(iosb).Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 3;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 4, 2, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 2, "expected 2, got %lu\n", iosb.Information);

    ret = SetFilePointer(handle, 0, NULL, FILE_CURRENT);
    ok(ret == 6, "expected 6, got %u\n", ret);

    CloseHandle(handle);

    handle = CreateFileA(buffer, FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    memset(buf, 0, sizeof(buf));
    U(iosb).Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(handle, 0, NULL, NULL, &iosb, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 6, "expected 6, got %lu\n", iosb.Information);
    buf[6] = 0;
    ok(memcmp(buf, text, 6) == 0, "wrong file contents: %s\n", buf);

    U(iosb).Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb, text + 3, 3, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 3, "expected 3, got %lu\n", iosb.Information);

    memset(buf, 0, sizeof(buf));
    U(iosb).Status = -1;
    iosb.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(handle, 0, NULL, NULL, &iosb, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iosb).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iosb).Status);
    ok(iosb.Information == 6, "expected 6, got %lu\n", iosb.Information);
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
        "rc = %x not STATUS_ACCESS_VIOLATION or STATUS_INVALID_PARAMETER\n", rc);

    /*
     * Test to see if the Timeout can be NULL
     */
    hslot = (HANDLE)0xdeadbeef;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         NULL);
    ok( rc == STATUS_SUCCESS ||
        rc == STATUS_INVALID_PARAMETER, /* win2k3 */
        "rc = %x not STATUS_SUCCESS or STATUS_INVALID_PARAMETER\n", rc);
    ok( hslot != 0, "Handle is invalid\n");

    if  ( rc == STATUS_SUCCESS ) pNtClose(hslot);

    /*
     * Test a valid call
     */
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_SUCCESS, "Create MailslotFile failed rc = %x\n", rc);
    ok( hslot != 0, "Handle is invalid\n");

    rc = pNtClose(hslot);
    ok( rc == STATUS_SUCCESS, "NtClose failed\n");
}

static void test_iocp_setcompletion(HANDLE h)
{
    NTSTATUS res;
    ULONG count;
    SIZE_T size = 3;

    if (sizeof(size) > 4) size |= (ULONGLONG)0x12345678 << 32;

    res = pNtSetIoCompletion( h, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, size );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %x\n", res );

    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %d\n", count );

    if (get_msg(h))
    {
        ok( completionKey == CKEY_FIRST, "Invalid completion key: %lx\n", completionKey );
        ok( ioSb.Information == size, "Invalid ioSb.Information: %lu\n", ioSb.Information );
        ok( U(ioSb).Status == STATUS_INVALID_DEVICE_REQUEST, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
        ok( completionValue == CVALUE_FIRST, "Invalid completion value: %lx\n", completionValue );
    }

    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %d\n", count );
}

static void test_iocp_fileio(HANDLE h)
{
    static const char pipe_name[] = "\\\\.\\pipe\\iocompletiontestnamedpipe";

    IO_STATUS_BLOCK iosb;
    FILE_COMPLETION_INFORMATION fci = {h, CKEY_SECOND};
    HANDLE hPipeSrv, hPipeClt;
    NTSTATUS res;

    hPipeSrv = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 4, 1024, 1024, 1000, NULL );
    ok( hPipeSrv != INVALID_HANDLE_VALUE, "Cannot create named pipe\n" );
    if (hPipeSrv != INVALID_HANDLE_VALUE )
    {
        hPipeClt = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
        ok( hPipeClt != INVALID_HANDLE_VALUE, "Cannot connect to pipe\n" );
        if (hPipeClt != INVALID_HANDLE_VALUE)
        {
            U(iosb).Status = 0xdeadbeef;
            res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
            ok( res == STATUS_INVALID_PARAMETER, "Unexpected NtSetInformationFile on non-overlapped handle: %x\n", res );
            ok( U(iosb).Status == STATUS_INVALID_PARAMETER /* 98 */ || U(iosb).Status == 0xdeadbeef /* NT4+ */,
                "Unexpected iosb.Status on non-overlapped handle: %x\n", U(iosb).Status );
            CloseHandle(hPipeClt);
        }
        CloseHandle( hPipeSrv );
    }

    hPipeSrv = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 4, 1024, 1024, 1000, NULL );
    ok( hPipeSrv != INVALID_HANDLE_VALUE, "Cannot create named pipe\n" );
    if (hPipeSrv == INVALID_HANDLE_VALUE )
        return;

    hPipeClt = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( hPipeClt != INVALID_HANDLE_VALUE, "Cannot connect to pipe\n" );
    if (hPipeClt != INVALID_HANDLE_VALUE)
    {
        OVERLAPPED o = {0,};
        BYTE send_buf[TEST_BUF_LEN], recv_buf[TEST_BUF_LEN];
        DWORD read;
        long count;

        U(iosb).Status = 0xdeadbeef;
        res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
        ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %x\n", res );
        ok( U(iosb).Status == STATUS_SUCCESS, "iosb.Status invalid: %x\n", U(iosb).Status );

        memset( send_buf, 0, TEST_BUF_LEN );
        memset( recv_buf, 0xde, TEST_BUF_LEN );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );
        ReadFile( hPipeSrv, recv_buf, TEST_BUF_LEN, &read, &o);
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );
        WriteFile( hPipeClt, send_buf, TEST_BUF_LEN, &read, NULL );

        if (get_msg(h))
        {
            ok( completionKey == CKEY_SECOND, "Invalid completion key: %lx\n", completionKey );
            ok( ioSb.Information == 3, "Invalid ioSb.Information: %ld\n", ioSb.Information );
            ok( U(ioSb).Status == STATUS_SUCCESS, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
            ok( completionValue == (ULONG_PTR)&o, "Invalid completion value: %lx\n", completionValue );
            ok( !memcmp( send_buf, recv_buf, TEST_BUF_LEN ), "Receive buffer (%x %x %x) did not match send buffer (%x %x %x)\n", recv_buf[0], recv_buf[1], recv_buf[2], send_buf[0], send_buf[1], send_buf[2] );
        }
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        memset( send_buf, 0, TEST_BUF_LEN );
        memset( recv_buf, 0xde, TEST_BUF_LEN );
        WriteFile( hPipeClt, send_buf, 2, &read, NULL );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );
        ReadFile( hPipeSrv, recv_buf, 2, &read, &o);
        count = get_pending_msgs(h);
        ok( count == 1, "Unexpected msg count: %ld\n", count );
        if (get_msg(h))
        {
            ok( completionKey == CKEY_SECOND, "Invalid completion key: %lx\n", completionKey );
            ok( ioSb.Information == 2, "Invalid ioSb.Information: %ld\n", ioSb.Information );
            ok( U(ioSb).Status == STATUS_SUCCESS, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
            ok( completionValue == (ULONG_PTR)&o, "Invalid completion value: %lx\n", completionValue );
            ok( !memcmp( send_buf, recv_buf, 2 ), "Receive buffer (%x %x) did not match send buffer (%x %x)\n", recv_buf[0], recv_buf[1], send_buf[0], send_buf[1] );
        }

        ReadFile( hPipeSrv, recv_buf, TEST_BUF_LEN, &read, &o);
        CloseHandle( hPipeSrv );
        count = get_pending_msgs(h);
        ok( count == 1, "Unexpected msg count: %ld\n", count );
        if (get_msg(h))
        {
            ok( completionKey == CKEY_SECOND, "Invalid completion key: %lx\n", completionKey );
            ok( ioSb.Information == 0, "Invalid ioSb.Information: %ld\n", ioSb.Information );
            /* wine sends wrong status here */
            ok( U(ioSb).Status == STATUS_PIPE_BROKEN, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
            ok( completionValue == (ULONG_PTR)&o, "Invalid completion value: %lx\n", completionValue );
        }
    }

    CloseHandle( hPipeClt );

    /* test associating a completion port with a handle after an async is queued */
    hPipeSrv = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 4, 1024, 1024, 1000, NULL );
    ok( hPipeSrv != INVALID_HANDLE_VALUE, "Cannot create named pipe\n" );
    if (hPipeSrv == INVALID_HANDLE_VALUE )
        return;
    hPipeClt = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( hPipeClt != INVALID_HANDLE_VALUE, "Cannot connect to pipe\n" );
    if (hPipeClt != INVALID_HANDLE_VALUE)
    {
        OVERLAPPED o = {0,};
        BYTE send_buf[TEST_BUF_LEN], recv_buf[TEST_BUF_LEN];
        int apc_count = 0;
        DWORD read;
        long count;

        memset( send_buf, 0, TEST_BUF_LEN );
        memset( recv_buf, 0xde, TEST_BUF_LEN );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );
        ReadFile( hPipeSrv, recv_buf, TEST_BUF_LEN, &read, &o);

        U(iosb).Status = 0xdeadbeef;
        res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
        ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %x\n", res );
        ok( U(iosb).Status == STATUS_SUCCESS, "iosb.Status invalid: %x\n", U(iosb).Status );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        WriteFile( hPipeClt, send_buf, TEST_BUF_LEN, &read, NULL );

        if (get_msg(h))
        {
            ok( completionKey == CKEY_SECOND, "Invalid completion key: %lx\n", completionKey );
            ok( ioSb.Information == 3, "Invalid ioSb.Information: %ld\n", ioSb.Information );
            ok( U(ioSb).Status == STATUS_SUCCESS, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
            ok( completionValue == (ULONG_PTR)&o, "Invalid completion value: %lx\n", completionValue );
            ok( !memcmp( send_buf, recv_buf, TEST_BUF_LEN ), "Receive buffer (%x %x %x) did not match send buffer (%x %x %x)\n", recv_buf[0], recv_buf[1], recv_buf[2], send_buf[0], send_buf[1], send_buf[2] );
        }
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        /* using APCs on handle with associated completion port is not allowed */
        res = NtReadFile( hPipeSrv, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
        ok(res == STATUS_INVALID_PARAMETER, "NtReadFile returned %x\n", res);
    }

    CloseHandle( hPipeSrv );
    CloseHandle( hPipeClt );

    /* test associating a completion port with a handle after an async using APC is queued */
    hPipeSrv = CreateNamedPipeA( pipe_name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 4, 1024, 1024, 1000, NULL );
    ok( hPipeSrv != INVALID_HANDLE_VALUE, "Cannot create named pipe\n" );
    if (hPipeSrv == INVALID_HANDLE_VALUE )
        return;
    hPipeClt = CreateFileA( pipe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL );
    ok( hPipeClt != INVALID_HANDLE_VALUE, "Cannot connect to pipe\n" );
    if (hPipeClt != INVALID_HANDLE_VALUE)
    {
        BYTE send_buf[TEST_BUF_LEN], recv_buf[TEST_BUF_LEN];
        int apc_count = 0;
        DWORD read;
        long count;

        memset( send_buf, 0, TEST_BUF_LEN );
        memset( recv_buf, 0xde, TEST_BUF_LEN );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        res = NtReadFile( hPipeSrv, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
        ok(res == STATUS_PENDING, "NtReadFile returned %x\n", res);

        U(iosb).Status = 0xdeadbeef;
        res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
        ok( res == STATUS_SUCCESS, "NtSetInformationFile failed: %x\n", res );
        ok( U(iosb).Status == STATUS_SUCCESS, "iosb.Status invalid: %x\n", U(iosb).Status );
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        WriteFile( hPipeClt, send_buf, TEST_BUF_LEN, &read, NULL );

        ok(!apc_count, "apc_count = %u\n", apc_count);
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        SleepEx(1, TRUE); /* alertable sleep */
        ok(apc_count == 1, "apc was not called\n");
        count = get_pending_msgs(h);
        ok( !count, "Unexpected msg count: %ld\n", count );

        /* using APCs on handle with associated completion port is not allowed */
        res = NtReadFile( hPipeSrv, NULL, apc, &apc_count, &iosb, recv_buf, sizeof(recv_buf), NULL, NULL );
        ok(res == STATUS_INVALID_PARAMETER, "NtReadFile returned %x\n", res);
    }

    CloseHandle( hPipeSrv );
    CloseHandle( hPipeClt );
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
    ok(res == STATUS_SUCCESS, "cannot get attributes, res %x\n", res);
    res = pNtQueryVolumeInformationFile(h, &io, &fsi, sizeof fsi, FileFsSizeInformation);
    ok(res == STATUS_SUCCESS, "cannot get attributes, res %x\n", res);

    /* Test for FileFsSizeInformation */
    ok(fsi.TotalAllocationUnits.QuadPart > 0,
        "[fsi] TotalAllocationUnits expected positive, got 0x%s\n",
        wine_dbgstr_longlong(fsi.TotalAllocationUnits.QuadPart));
    ok(fsi.AvailableAllocationUnits.QuadPart > 0,
        "[fsi] AvailableAllocationUnits expected positive, got 0x%s\n",
        wine_dbgstr_longlong(fsi.AvailableAllocationUnits.QuadPart));

    /* Assume file system is NTFS */
    ok(fsi.BytesPerSector == 512, "[fsi] BytesPerSector expected 512, got %d\n",fsi.BytesPerSector);
    ok(fsi.SectorsPerAllocationUnit == 8, "[fsi] SectorsPerAllocationUnit expected 8, got %d\n",fsi.SectorsPerAllocationUnit);

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
    ok(ffsi.CallerAvailableAllocationUnits.QuadPart == fsi.AvailableAllocationUnits.QuadPart,
        "[ffsi] CallerAvailableAllocationUnits error fsi:0x%s, ffsi: 0x%s\n",
        wine_dbgstr_longlong(fsi.AvailableAllocationUnits.QuadPart),
        wine_dbgstr_longlong(ffsi.CallerAvailableAllocationUnits.QuadPart));

    /* Assume file system is NTFS */
    ok(ffsi.BytesPerSector == 512, "[ffsi] BytesPerSector expected 512, got %d\n",ffsi.BytesPerSector);
    ok(ffsi.SectorsPerAllocationUnit == 8, "[ffsi] SectorsPerAllocationUnit expected 8, got %d\n",ffsi.SectorsPerAllocationUnit);

    CloseHandle( h );
}

static void test_file_basic_information(void)
{
    IO_STATUS_BLOCK io;
    FILE_BASIC_INFORMATION fbi;
    HANDLE h;
    int res;
    int attrib_mask = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL;

    if (!(h = create_temp_file(0))) return;

    /* Check default first */
    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes, res %x\n", res);
    ok ( (fbi.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) == FILE_ATTRIBUTE_ARCHIVE,
         "attribute %x not expected\n", fbi.FileAttributes );

    /* Then SYSTEM */
    /* Clear fbi to avoid setting times */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, NtSetInformationFile returned %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set system attribute, io.Status is %x\n", U(io).Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %x not FILE_ATTRIBUTE_SYSTEM (ok in old linux without xattr)\n", fbi.FileAttributes );

    /* Then HIDDEN */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, NtSetInformationFile returned %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set system attribute, io.Status is %x\n", U(io).Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %x not FILE_ATTRIBUTE_HIDDEN (ok in old linux without xattr)\n", fbi.FileAttributes );

    /* Check NORMAL last of all (to make sure we can clear attributes) */
    memset(&fbi, 0, sizeof(fbi));
    fbi.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set normal attribute, NtSetInformationFile returned %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set normal attribute, io.Status is %x\n", U(io).Status );

    memset(&fbi, 0, sizeof(fbi));
    res = pNtQueryInformationFile(h, &io, &fbi, sizeof fbi, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    todo_wine ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_NORMAL, "attribute %x not 0\n", fbi.FileAttributes );

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
         "attribute %x not expected\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Then SYSTEM */
    /* Clear fbi to avoid setting times */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_SYSTEM;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_INVALID_INFO_CLASS || broken(res == STATUS_NOT_IMPLEMENTED), "shouldn't be able to set FileAllInformation, res %x\n", res);
    todo_wine ok ( U(io).Status == 0xdeadbeef, "shouldn't be able to set FileAllInformation, io.Status is %x\n", U(io).Status);
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %x\n", U(io).Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes, res %x\n", res);
    ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %x not FILE_ATTRIBUTE_SYSTEM (ok in old linux without xattr)\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Then HIDDEN */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_HIDDEN;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %x\n", U(io).Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %x not FILE_ATTRIBUTE_HIDDEN (ok in old linux without xattr)\n", fai_buf.fai.BasicInformation.FileAttributes );

    /* Check NORMAL last of all (to make sure we can clear attributes) */
    memset(&fai_buf.fai.BasicInformation, 0, sizeof(fai_buf.fai.BasicInformation));
    fai_buf.fai.BasicInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile(h, &io, &fai_buf.fai.BasicInformation, sizeof fai_buf.fai.BasicInformation, FileBasicInformation);
    ok ( res == STATUS_SUCCESS, "can't set system attribute, res: %x\n", res );
    ok ( U(io).Status == STATUS_SUCCESS, "can't set system attribute, io.Status: %x\n", U(io).Status );

    memset(&fai_buf.fai, 0, sizeof(fai_buf.fai));
    res = pNtQueryInformationFile(h, &io, &fai_buf.fai, sizeof fai_buf, FileAllInformation);
    ok ( res == STATUS_SUCCESS, "can't get attributes\n");
    todo_wine ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_NORMAL, "attribute %x not FILE_ATTRIBUTE_NORMAL\n", fai_buf.fai.BasicInformation.FileAttributes );

    CloseHandle( h );
}

static void delete_object( WCHAR *path )
{
    BOOL ret = DeleteFileW( path );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_ACCESS_DENIED,
        "DeleteFileW failed with %u\n", GetLastError() );
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        ret = RemoveDirectoryW( path );
        ok( ret, "RemoveDirectoryW failed with %u\n", GetLastError() );
    }
}

static void test_file_rename_information(void)
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    todo_wine ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists= TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fri->ReplaceIfExists = TRUE;
    fri->RootDirectory = NULL;
    fri->FileNameLength = name_str.Length;
    memcpy( fri->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fri->ReplaceIfExists = FALSE;
    fri->RootDirectory = handle2;
    fri->FileNameLength = lstrlenW(filename) * sizeof(WCHAR);
    memcpy( fri->FileName, filename, fri->FileNameLength );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fri, sizeof(FILE_RENAME_INFORMATION) + fri->FileNameLength, FileRenameInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, newpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(newpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    CloseHandle( handle2 );
    HeapFree( GetProcessHeap(), 0, fri );
    delete_object( oldpath );
    delete_object( newpath );
}

static void test_file_link_information(void)
{
    static const WCHAR foo_txtW[] = {'\\','f','o','o','.','t','x','t',0};
    static const WCHAR fooW[] = {'f','o','o',0};
    WCHAR tmp_path[MAX_PATH], oldpath[MAX_PATH + 16], newpath[MAX_PATH + 16], *filename, *p;
    FILE_LINK_INFORMATION *fli;
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
    fli = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_LINK_INFORMATION) + name_str.Length );
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    CloseHandle( handle );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef , "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "file should not exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION || res == STATUS_FILE_IS_A_DIRECTORY /* > Win XP */,
        "res expected STATUS_OBJECT_NAME_COLLISION or STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_FILE_IS_A_DIRECTORY, "res expected STATUS_FILE_IS_A_DIRECTORY, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_OBJECT_NAME_COLLISION, "res expected STATUS_OBJECT_NAME_COLLISION, got %x\n", res );
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
    fli->ReplaceIfExists = TRUE;
    fli->RootDirectory = NULL;
    fli->FileNameLength = name_str.Length;
    memcpy( fli->FileName, name_str.Buffer, name_str.Length );
    pRtlFreeUnicodeString( &name_str );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    todo_wine ok( U(io).Status == 0xdeadbeef, "io.Status expected 0xdeadbeef, got %x\n", U(io).Status );
    ok( res == STATUS_ACCESS_DENIED, "res expected STATUS_ACCESS_DENIED, got %x\n", res );
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
    fli->ReplaceIfExists = FALSE;
    fli->RootDirectory = handle2;
    fli->FileNameLength = lstrlenW(filename) * sizeof(WCHAR);
    memcpy( fli->FileName, filename, fli->FileNameLength );

    U(io).Status = 0xdeadbeef;
    res = pNtSetInformationFile( handle, &io, fli, sizeof(FILE_LINK_INFORMATION) + fli->FileNameLength, FileLinkInformation );
    ok( U(io).Status == STATUS_SUCCESS, "io.Status expected STATUS_SUCCESS, got %x\n", U(io).Status );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fileDeleted = GetFileAttributesW( oldpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );
    fileDeleted = GetFileAttributesW( newpath ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "file should exist\n" );

    fni = HeapAlloc( GetProcessHeap(), 0, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR) );
    res = pNtQueryInformationFile( handle, &io, fni, sizeof(FILE_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), FileNameInformation );
    ok( res == STATUS_SUCCESS, "res expected STATUS_SUCCESS, got %x\n", res );
    fni->FileName[ fni->FileNameLength / sizeof(WCHAR) ] = 0;
    ok( !lstrcmpiW(fni->FileName, oldpath + 2), "FileName expected %s, got %s\n",
        wine_dbgstr_w(oldpath + 2), wine_dbgstr_w(fni->FileName) );
    HeapFree( GetProcessHeap(), 0, fni );

    CloseHandle( handle );
    CloseHandle( handle2 );
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

static void test_file_disposition_information(void)
{
    char tmp_path[MAX_PATH], buffer[MAX_PATH + 16];
    DWORD dirpos;
    HANDLE handle, handle2, mapping;
    NTSTATUS res;
    IO_STATUS_BLOCK io;
    FILE_DISPOSITION_INFORMATION fdi;
    BOOL fileDeleted;
    DWORD fdi2;
    void *ptr;

    GetTempPathA( MAX_PATH, tmp_path );

    /* tests for info struct size */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA( buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = pNtSetInformationFile( handle, &io, &fdi, 0, FileDispositionInformation );
    todo_wine
    ok( res == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %x\n", res );
    fdi2 = 0x100;
    res = pNtSetInformationFile( handle, &io, &fdi2, sizeof(fdi2), FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %x\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* cannot set disposition on file not opened with delete access */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    res = pNtQueryInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_INVALID_INFO_CLASS || res == STATUS_NOT_IMPLEMENTED, "Unexpected NtQueryInformationFile result (expected STATUS_INVALID_INFO_CLASS, got %x)\n", res );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_ACCESS_DENIED, "unexpected FileDispositionInformation result (expected STATUS_ACCESS_DENIED, got %x)\n", res );
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
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );
    DeleteFileA( buffer );

    /* cannot set disposition on readonly file */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_READONLY, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %x)\n", res );
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
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    todo_wine
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    SetFileAttributesA( buffer, FILE_ATTRIBUTE_NORMAL );
    DeleteFileA( buffer );

    /* can set disposition on file and then reset it */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    DeleteFileA( buffer );

    /* Delete-on-close flag doesn't change file disposition until a handle is closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );
    DeleteFileA( buffer );

    /* Delete-on-close flag sets disposition when a handle is closed and then it could be changed back */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    ok( DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &handle2, 0, FALSE, DUPLICATE_SAME_ACCESS ), "DuplicateHandle failed\n" );
    CloseHandle( handle );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle2, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle2 );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );
    DeleteFileA( buffer );

    /* can set disposition on a directory opened with proper access */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "Directory should have been deleted\n" );
    RemoveDirectoryA( buffer );

    /* RemoveDirectory sets directory disposition and it can be undone */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    DeleteFileA( buffer );
    ok( CreateDirectoryA( buffer, NULL ), "CreateDirectory failed\n" );
    handle = CreateFileA(buffer, DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to open a directory\n" );
    RemoveDirectoryA( buffer );
    fdi.DoDeleteFile = FALSE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    RemoveDirectoryA( buffer );

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
    todo_wine
    ok( res == STATUS_DIRECTORY_NOT_EMPTY, "unexpected FileDispositionInformation result (expected STATUS_DIRECTORY_NOT_EMPTY, got %x)\n", res );
    DeleteFileA( buffer );
    buffer[dirpos] = '\0';
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    todo_wine
    ok( !fileDeleted, "Directory shouldn't have been deleted\n" );
    RemoveDirectoryA( buffer );

    /* cannot set disposition on file with file mapping opened */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    mapping = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 64 * 1024, "DelFileTest" );
    ok( mapping != NULL, "failed to create file mapping\n");
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    CloseHandle( mapping );
    DeleteFileA( buffer );

    /* can set disposition on file with file mapping closed */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    mapping = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 64 * 1024, "DelFileTest" );
    ok( mapping != NULL, "failed to create file mapping\n");
    CloseHandle( mapping );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );
    DeleteFileA( buffer );

    /* cannot set disposition on file which is mapped to memory */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    mapping = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 64 * 1024, "DelFileTest" );
    ok( mapping != NULL, "failed to create file mapping\n");
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile failed\n");
    CloseHandle( mapping );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_CANNOT_DELETE, "unexpected FileDispositionInformation result (expected STATUS_CANNOT_DELETE, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( !fileDeleted, "File shouldn't have been deleted\n" );
    UnmapViewOfFile( ptr );
    DeleteFileA( buffer );

    /* can set disposition on file which is mapped to memory and unmapped again */
    GetTempFileNameA( tmp_path, "dis", 0, buffer );
    handle = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( handle != INVALID_HANDLE_VALUE, "failed to create temp file\n" );
    mapping = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 64 * 1024, "DelFileTest" );
    ok( mapping != NULL, "failed to create file mapping\n");
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile failed\n");
    CloseHandle( mapping );
    UnmapViewOfFile( ptr );
    fdi.DoDeleteFile = TRUE;
    res = pNtSetInformationFile( handle, &io, &fdi, sizeof fdi, FileDispositionInformation );
    ok( res == STATUS_SUCCESS, "unexpected FileDispositionInformation result (expected STATUS_SUCCESS, got %x)\n", res );
    CloseHandle( handle );
    fileDeleted = GetFileAttributesA( buffer ) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    ok( fileDeleted, "File should have been deleted\n" );
    DeleteFileA( buffer );
}

static void test_iocompletion(void)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    NTSTATUS res;

    res = pNtCreateIoCompletion( &h, IO_COMPLETION_ALL_ACCESS, NULL, 0);

    ok( res == 0, "NtCreateIoCompletion anonymous failed: %x\n", res );
    ok( h && h != INVALID_HANDLE_VALUE, "Invalid handle returned\n" );

    if ( h && h != INVALID_HANDLE_VALUE)
    {
        test_iocp_setcompletion(h);
        test_iocp_fileio(h);
        pNtClose(h);
    }
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
    ok(hr == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationFile returned %#x.\n", hr);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info), FileNameInformation );
    ok(hr == STATUS_BUFFER_OVERFLOW, "NtQueryInformationFile returned %#x, expected %#x.\n",
            hr, STATUS_BUFFER_OVERFLOW);
    ok(U(io).Status == STATUS_BUFFER_OVERFLOW, "io.Status is %#x, expected %#x.\n",
            U(io).Status, STATUS_BUFFER_OVERFLOW);
    ok(info->FileNameLength == lstrlenW( expected ) * sizeof(WCHAR), "info->FileNameLength is %u\n", info->FileNameLength);
    ok(info->FileName[2] == 0xcccc, "info->FileName[2] is %#x, expected 0xcccc.\n", info->FileName[2]);
    ok(CharLowerW((LPWSTR)(UINT_PTR)info->FileName[1]) == CharLowerW((LPWSTR)(UINT_PTR)expected[1]),
            "info->FileName[1] is %p, expected %p.\n",
            CharLowerW((LPWSTR)(UINT_PTR)info->FileName[1]), CharLowerW((LPWSTR)(UINT_PTR)expected[1]));
    ok(io.Information == sizeof(*info), "io.Information is %lu\n", io.Information);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileNameInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#x, expected %#x.\n", hr, STATUS_SUCCESS);
    ok(U(io).Status == STATUS_SUCCESS, "io.Status is %#x, expected %#x.\n", U(io).Status, STATUS_SUCCESS);
    ok(info->FileNameLength == lstrlenW( expected ) * sizeof(WCHAR), "info->FileNameLength is %u\n", info->FileNameLength);
    ok(info->FileName[info->FileNameLength / sizeof(WCHAR)] == 0xcccc, "info->FileName[len] is %#x, expected 0xcccc.\n",
       info->FileName[info->FileNameLength / sizeof(WCHAR)]);
    info->FileName[info->FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->FileName, expected ), "info->FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->FileName ), wine_dbgstr_w( expected ));
    ok(io.Information == FIELD_OFFSET(FILE_NAME_INFORMATION, FileName) + info->FileNameLength,
            "io.Information is %lu, expected %u.\n",
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
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#x, expected %#x.\n", hr, STATUS_SUCCESS);
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
    ok(hr == STATUS_INFO_LENGTH_MISMATCH, "NtQueryInformationFile returned %#x, expected %#x.\n",
            hr, STATUS_INFO_LENGTH_MISMATCH);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, sizeof(*info), FileAllInformation );
    ok(hr == STATUS_BUFFER_OVERFLOW, "NtQueryInformationFile returned %#x, expected %#x.\n",
            hr, STATUS_BUFFER_OVERFLOW);
    ok(U(io).Status == STATUS_BUFFER_OVERFLOW, "io.Status is %#x, expected %#x.\n",
            U(io).Status, STATUS_BUFFER_OVERFLOW);
    ok(info->NameInformation.FileNameLength == lstrlenW( expected ) * sizeof(WCHAR),
       "info->NameInformation.FileNameLength is %u\n", info->NameInformation.FileNameLength );
    ok(info->NameInformation.FileName[2] == 0xcccc,
            "info->NameInformation.FileName[2] is %#x, expected 0xcccc.\n", info->NameInformation.FileName[2]);
    ok(CharLowerW((LPWSTR)(UINT_PTR)info->NameInformation.FileName[1]) == CharLowerW((LPWSTR)(UINT_PTR)expected[1]),
            "info->NameInformation.FileName[1] is %p, expected %p.\n",
            CharLowerW((LPWSTR)(UINT_PTR)info->NameInformation.FileName[1]), CharLowerW((LPWSTR)(UINT_PTR)expected[1]));
    ok(io.Information == sizeof(*info), "io.Information is %lu\n", io.Information);

    memset( info, 0xcc, info_size );
    hr = pNtQueryInformationFile( h, &io, info, info_size, FileAllInformation );
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#x, expected %#x.\n", hr, STATUS_SUCCESS);
    ok(U(io).Status == STATUS_SUCCESS, "io.Status is %#x, expected %#x.\n", U(io).Status, STATUS_SUCCESS);
    ok(info->NameInformation.FileNameLength == lstrlenW( expected ) * sizeof(WCHAR),
       "info->NameInformation.FileNameLength is %u\n", info->NameInformation.FileNameLength );
    ok(info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] == 0xcccc,
       "info->NameInformation.FileName[len] is %#x, expected 0xcccc.\n",
       info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)]);
    info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->NameInformation.FileName, expected ),
            "info->NameInformation.FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->NameInformation.FileName ), wine_dbgstr_w( expected ));
    ok(io.Information == FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName)
            + info->NameInformation.FileNameLength,
            "io.Information is %lu\n", io.Information );

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
    ok(hr == STATUS_SUCCESS, "NtQueryInformationFile returned %#x, expected %#x.\n", hr, STATUS_SUCCESS);
    info->NameInformation.FileName[info->NameInformation.FileNameLength / sizeof(WCHAR)] = '\0';
    ok(!lstrcmpiW( info->NameInformation.FileName, expected ), "info->NameInformation.FileName is %s, expected %s.\n",
            wine_dbgstr_w( info->NameInformation.FileName ), wine_dbgstr_w( expected ));

    CloseHandle( h );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, expected );
    HeapFree( GetProcessHeap(), 0, volume_prefix );
    HeapFree( GetProcessHeap(), 0, file_name );
}

static void test_file_completion_information(void)
{
    static const char buf[] = "testdata";
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION info;
    OVERLAPPED ov, *pov;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    DWORD num_bytes;
    HANDLE port, h;
    ULONG_PTR key;
    BOOL ret;
    int i;

    if (!(h = create_temp_file(0))) return;

    status = pNtSetInformationFile(h, &io, &info, sizeof(info) - 1, FileIoCompletionNotificationInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_INVALID_INFO_CLASS /* XP */,
       "expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
    if (status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED)
    {
        win_skip("FileIoCompletionNotificationInformation class not supported\n");
        CloseHandle(h);
        return;
    }

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %08x\n", status);

    CloseHandle(h);
    if (!(h = create_temp_file(FILE_FLAG_OVERLAPPED))) return;

    info.Flags = FILE_SKIP_SET_EVENT_ON_HANDLE;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);

    info.Flags = FILE_SKIP_SET_USER_EVENT_ON_FAST_IO;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);

    CloseHandle(h);
    if (!(h = create_temp_file(FILE_FLAG_OVERLAPPED))) return;

    info.Flags = ~0U;
    status = pNtQueryInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    ok(!(info.Flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS), "got %08x\n", info.Flags);

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    port = CreateIoCompletionPort(h, NULL, 0xdeadbeef, 0);
    ok(port != NULL, "CreateIoCompletionPort failed, error %u\n", GetLastError());

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %u\n", GetLastError());
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %u\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %u\n", num_bytes);

        key = 0;
        pov = NULL;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %u\n", GetLastError());
        ok(key == 0xdeadbeef, "expected 0xdeadbeef, got %lx\n", key);
        ok(pov == &ov, "expected %p, got %p\n", &ov, pov);
    }
    else
        win_skip("WriteFile never returned TRUE\n");

    info.Flags = FILE_SKIP_COMPLETION_PORT_ON_SUCCESS;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);

    info.Flags = 0;
    status = pNtQueryInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    ok((info.Flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) != 0, "got %08x\n", info.Flags);

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %u\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %u\n", num_bytes);

        pov = (void *)0xdeadbeef;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 500);
        ok(!ret, "GetQueuedCompletionStatus succeeded\n");
        ok(pov == NULL, "expected NULL, got %p\n", pov);
    }
    else
        win_skip("WriteFile never returned TRUE\n");

    info.Flags = 0;
    status = pNtSetInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);

    info.Flags = 0;
    status = pNtQueryInformationFile(h, &io, &info, sizeof(info), FileIoCompletionNotificationInformation);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
    ok((info.Flags & FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) != 0, "got %08x\n", info.Flags);

    for (i = 0; i < 10; i++)
    {
        SetLastError(0xdeadbeef);
        ret = WriteFile(h, buf, sizeof(buf), &num_bytes, &ov);
        if (ret || GetLastError() != ERROR_IO_PENDING) break;
        ret = GetOverlappedResult(h, &ov, &num_bytes, TRUE);
        ok(ret, "GetOverlappedResult failed, error %u\n", GetLastError());
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(ret, "GetQueuedCompletionStatus failed, error %u\n", GetLastError());
        ret = FALSE;
    }
    if (ret)
    {
        ok(num_bytes == sizeof(buf), "expected sizeof(buf), got %u\n", num_bytes);

        pov = (void *)0xdeadbeef;
        ret = GetQueuedCompletionStatus(port, &num_bytes, &key, &pov, 1000);
        ok(!ret, "GetQueuedCompletionStatus succeeded\n");
        ok(pov == NULL, "expected NULL, got %p\n", pov);
    }
    else
        win_skip("WriteFile never returned TRUE\n");

    CloseHandle(ov.hEvent);
    CloseHandle(port);
    CloseHandle(h);
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
    ok( dwords[0] == info.dwVolumeSerialNumber, "expected %08x, got %08x\n",
        info.dwVolumeSerialNumber, dwords[0] );
    ok( dwords[1] != 0x11111111, "expected != 0x11111111\n" );

    dwords = (DWORD *)&fid.FileId;
    ok( dwords[0] == info.nFileIndexLow, "expected %08x, got %08x\n", info.nFileIndexLow, dwords[0] );
    ok( dwords[1] == info.nFileIndexHigh, "expected %08x, got %08x\n", info.nFileIndexHigh, dwords[1] );
    ok( dwords[2] == 0, "expected 0, got %08x\n", dwords[2] );
    ok( dwords[3] == 0, "expected 0, got %08x\n", dwords[3] );

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
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status );

    status = pNtQueryInformationFile( (HANDLE)0xdeadbeef, &io, &info, sizeof(info), FileAccessInformation );
    ok( status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %08x\n", status );

    memset(&info, 0x11, sizeof(info));
    status = pNtQueryInformationFile( h, &io, &info, sizeof(info), FileAccessInformation );
    ok( status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status );
    ok( info.AccessFlags == 0x13019f, "got %08x\n", info.AccessFlags );

    CloseHandle( h );
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
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    ZeroMemory( buf, sizeof(buf) );
    U(io).Status = 0xdadadada;
    io.Information = 0xcacacaca;

    status = pNtQueryVolumeInformationFile( dir, &io, buf, sizeof(buf), FileFsVolumeInformation );

    ffvi = (FILE_FS_VOLUME_INFORMATION *)buf;

    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", status);
    ok(U(io).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", U(io).Status);

todo_wine
{
    ok(io.Information == (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + ffvi->VolumeLabelLength),
    "expected %d, got %lu\n", (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel) + ffvi->VolumeLabelLength),
     io.Information);

    ok(ffvi->VolumeCreationTime.QuadPart != 0, "Missing VolumeCreationTime\n");
    ok(ffvi->VolumeSerialNumber != 0, "Missing VolumeSerialNumber\n");
    ok(ffvi->SupportsObjects == 1,"expected 1, got %d\n", ffvi->SupportsObjects);
}
    ok(ffvi->VolumeLabelLength == lstrlenW(ffvi->VolumeLabel) * sizeof(WCHAR), "got %d\n", ffvi->VolumeLabelLength);

    trace("VolumeSerialNumber: %x VolumeLabelName: %s\n", ffvi->VolumeSerialNumber, wine_dbgstr_w(ffvi->VolumeLabel));

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
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
    pRtlFreeUnicodeString( &nameW );

    ZeroMemory( buf, sizeof(buf) );
    U(io).Status = 0xdadadada;
    io.Information = 0xcacacaca;

    status = pNtQueryVolumeInformationFile( dir, &io, buf, sizeof(buf), FileFsAttributeInformation );

    ffai = (FILE_FS_ATTRIBUTE_INFORMATION *)buf;

    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", status);
    ok(U(io).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", U(io).Status);
    ok(ffai->FileSystemAttributes != 0, "Missing FileSystemAttributes\n");
    ok(ffai->MaximumComponentNameLength != 0, "Missing MaximumComponentNameLength\n");
    ok(ffai->FileSystemNameLength != 0, "Missing FileSystemNameLength\n");

    trace("FileSystemAttributes: %x MaximumComponentNameLength: %x FileSystemName: %s\n",
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

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        status = pNtCreateFile(&handle, GENERIC_READ, &attr, &io, NULL,
                               td[i].attrib_in, FILE_SHARE_READ|FILE_SHARE_WRITE,
                               td[i].disposition, 0, NULL, 0);

        ok(status == td[i].status, "%d: expected %#x got %#x\n", i, td[i].status, status);

        if (!status)
        {
            ok(io.Information == td[i].result,"%d: expected %#x got %#lx\n", i, td[i].result, io.Information);

            ret = GetFileAttributesW(path);
            ret &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
            /* FIXME: leave only 'else' case below once Wine is fixed */
            if (ret != td[i].attrib_out)
            {
            todo_wine
                ok(ret == td[i].attrib_out, "%d: expected %#x got %#x\n", i, td[i].attrib_out, ret);
                SetFileAttributesW(path, td[i].attrib_out);
            }
            else
                ok(ret == td[i].attrib_out, "%d: expected %#x got %#x\n", i, td[i].attrib_out, ret);

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
}

static void test_readonly(void)
{
    static const WCHAR fooW[] = {'f','o','o',0};
    NTSTATUS status;
    HANDLE handle;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;

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

    status = pNtCreateFile(&handle, GENERIC_READ, &attr, &io, NULL, FILE_ATTRIBUTE_READONLY,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_CREATE, 0, NULL, 0);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, GENERIC_WRITE,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_ACCESS_DENIED, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, GENERIC_READ,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, FILE_READ_ATTRIBUTES,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, FILE_WRITE_ATTRIBUTES,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, DELETE,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, READ_CONTROL,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, WRITE_DAC,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, WRITE_OWNER,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle(handle);

    status = pNtOpenFile(&handle, SYNCHRONIZE,  &attr, &io,
                         FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN_FOR_BACKUP_INTENT);
    ok(status == STATUS_SUCCESS, "got %#x\n", status);
    CloseHandle( handle );

    pRtlFreeUnicodeString(&nameW);
    SetFileAttributesW(path, FILE_ATTRIBUTE_ARCHIVE);
    DeleteFileW(path);
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

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, NULL, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(INVALID_HANDLE_VALUE, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_INVALID_HANDLE, "expected STATUS_OBJECT_TYPE_MISMATCH, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    hfile = create_temp_file(0);
    if (!hfile) return;

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_USER_BUFFER, "expected STATUS_INVALID_USER_BUFFER, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtWriteFile(hfile, event, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_USER_BUFFER, "expected STATUS_INVALID_USER_BUFFER, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);
    ok(!is_signaled(event), "event is not signaled\n");

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtReadFile(hfile, event, NULL, NULL, &iob, NULL, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);
    ok(is_signaled(event), "event is not signaled\n");

    U(iob).Status = -1;
    iob.Information = -1;
    SetEvent(event);
    status = pNtReadFile(hfile, event, NULL, NULL, &iob, (void*)0xdeadbeef, sizeof(contents), NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);
    ok(is_signaled(event), "event is not signaled\n");

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, 7, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 7, "expected 7, got %lu\n", iob.Information);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-1 /* FILE_WRITE_TO_END_OF_FILE */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents + 7, sizeof(contents) - 7, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == sizeof(contents) - 7, "expected sizeof(contents)-7, got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, buf, 0, &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    for (i = -20; i < -1; i++)
    {
        if (i == -2) continue;

        U(iob).Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%d: expected STATUS_INVALID_PARAMETER, got %#x\n", i, status);
        ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
        ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);
    }

    SetFilePointer(hfile, sizeof(contents) - 4, NULL, FILE_BEGIN);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 4, "expected 4, got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", status);
    ok(U(iob).Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, contents, sizeof(contents), &bytes, NULL);
    ok(ret, "WriteFile error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    /* test reading beyond EOF */
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, NULL, 0, &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    S(U(ovl)).Offset = sizeof(contents);
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_HANDLE_EOF, "expected ERROR_HANDLE_EOF, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);

    S(U(ovl)).Offset = sizeof(contents);
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = -1;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, &ovl);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", status);
    ok(U(iob).Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, NULL, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", status);
    ok(U(iob).Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", status);
    ok(U(iob).Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-2 /* FILE_USE_FILE_POINTER_POSITION */;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);

    for (i = -20; i < 0; i++)
    {
        if (i == -2) continue;

        U(iob).Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%d: expected STATUS_INVALID_PARAMETER, got %#x\n", i, status);
        ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
        ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);
    }

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %lu\n", iob.Information);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents) - 4;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtWriteFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 4, "expected 4, got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_SUCCESS, "NtReadFile error %#x\n", status);
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %lu\n", iob.Information);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    S(U(ovl)).Offset = sizeof(contents) - 4;
    S(U(ovl)).OffsetHigh = 0;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, "ABCD", 4, &bytes, &ovl);
    ok(ret, "WriteFile error %d\n", GetLastError());
    ok(bytes == 4, "bytes %u\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    S(U(ovl)).Offset = 0;
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(ret, "ReadFile error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %lu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "ABCD", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == sizeof(contents), "expected sizeof(contents), got %u\n", off);

    CloseHandle(hfile);

    hfile = create_temp_file(FILE_FLAG_OVERLAPPED);
    if (!hfile) return;

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, buf, 0, &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    S(U(ovl)).Offset = 0;
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    ReadFile(hfile, buf, 0, &bytes, &ovl);
    ok(bytes == 0, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, contents, sizeof(contents), &bytes, NULL);
    ok(!ret, "WriteFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);

    for (i = -20; i < -1; i++)
    {
        U(iob).Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%d: expected STATUS_INVALID_PARAMETER, got %#x\n", i, status);
        ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
        ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);
    }

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, contents, sizeof(contents), &offset, NULL);
    ok(status == STATUS_PENDING || status == STATUS_SUCCESS /* before Vista */, "expected STATUS_PENDING or STATUS_SUCCESS, got %#x\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
    }
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ok(bytes == 0, "bytes %u\n", bytes);

    U(iob).Status = -1;
    iob.Information = -1;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), NULL, NULL);
    ok(status == STATUS_INVALID_PARAMETER, "expected STATUS_INVALID_PARAMETER, got %#x\n", status);
    ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
    ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);

    for (i = -20; i < 0; i++)
    {
        U(iob).Status = -1;
        iob.Information = -1;
        offset.QuadPart = (LONGLONG)i;
        status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
        ok(status == STATUS_INVALID_PARAMETER, "%d: expected STATUS_INVALID_PARAMETER, got %#x\n", i, status);
        ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
        ok(iob.Information == -1, "expected -1, got %ld\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    /* test reading beyond EOF */
    offset.QuadPart = sizeof(contents);
    S(U(ovl)).Offset = offset.u.LowPart;
    S(U(ovl)).OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    ok(!ret, "ReadFile should fail\n");
    ret = GetLastError();
    ok(ret == ERROR_IO_PENDING || ret == ERROR_HANDLE_EOF /* before Vista */, "expected ERROR_IO_PENDING or ERROR_HANDLE_EOF, got %d\n", ret);
    ok(bytes == 0, "bytes %u\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(!ret, "GetOverlappedResult should report FALSE\n");
        ok(GetLastError() == ERROR_HANDLE_EOF, "expected ERROR_HANDLE_EOF, got %d\n", GetLastError());
        ok(bytes == 0, "expected 0, read %u\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#lx\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    offset.QuadPart = sizeof(contents);
    S(U(ovl)).Offset = offset.u.LowPart;
    S(U(ovl)).OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, 0, &bytes, &ovl);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    if (!ret)
        ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
    ret = GetLastError();
    ok(bytes == 0, "bytes %u\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(ret, "GetOverlappedResult should report TRUE\n");
        ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
        ok(bytes == 0, "expected 0, read %u\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);
    }

    offset.QuadPart = sizeof(contents);
    S(U(ovl)).Offset = offset.u.LowPart;
    S(U(ovl)).OffsetHigh = offset.u.HighPart;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, NULL, 0, &bytes, &ovl);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    if (!ret)
        ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
    ret = GetLastError();
    ok(bytes == 0, "bytes %u\n", bytes);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    if (ret == ERROR_IO_PENDING)
    {
        bytes = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
        ok(ret, "GetOverlappedResult should report TRUE\n");
        ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
        ok(bytes == 0, "expected 0, read %u\n", bytes);
        ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
        ok(ovl.InternalHigh == 0, "expected 0, got %lu\n", ovl.InternalHigh);
    }

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
        ok(U(iob).Status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", U(iob).Status);
        ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);
    }
    else
    {
        ok(status == STATUS_END_OF_FILE, "expected STATUS_END_OF_FILE, got %#x\n", status);
        ok(U(iob).Status == -1, "expected -1, got %#x\n", U(iob).Status);
        ok(iob.Information == -1, "expected -1, got %lu\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = sizeof(contents);
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, 0, &offset, NULL);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
        ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
        ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);
    }
    else
    {
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", status);
        ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
        ok(iob.Information == 0, "expected 0, got %lu\n", iob.Information);
    }

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    S(U(ovl)).Offset = 0;
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    if (!ret)
    {
        ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
        ok(bytes == 0, "bytes %u\n", bytes);
    }
    else ok(bytes == 14, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %lu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %lu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents)), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    SetFilePointer(hfile, sizeof(contents) - 4, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);
    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = (LONGLONG)-1 /* FILE_WRITE_TO_END_OF_FILE */;
    status = pNtWriteFile(hfile, 0, NULL, NULL, &iob, "DCBA", 4, &offset, NULL);
    ok(status == STATUS_PENDING || status == STATUS_SUCCESS /* before Vista */, "expected STATUS_PENDING or STATUS_SUCCESS, got %#x\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
    }
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == 4, "expected 4, got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    U(iob).Status = -1;
    iob.Information = -1;
    offset.QuadPart = 0;
    status = pNtReadFile(hfile, 0, NULL, NULL, &iob, buf, sizeof(buf), &offset, NULL);
    ok(status == STATUS_PENDING || status == STATUS_SUCCESS, "expected STATUS_PENDING or STATUS_SUCCESS, got %#x\n", status);
    if (status == STATUS_PENDING)
    {
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
    }
    ok(U(iob).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x\n", U(iob).Status);
    ok(iob.Information == sizeof(contents), "expected sizeof(contents), got %lu\n", iob.Information);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "DCBA", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    S(U(ovl)).Offset = sizeof(contents) - 4;
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, "ABCD", 4, &bytes, &ovl);
    /* WriteFile return value depends on Windows version and testing it is not practical */
    if (!ret)
    {
        ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
        ok(bytes == 0, "bytes %u\n", bytes);
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
    }
    else ok(bytes == 4, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == 4, "expected 4, got %lu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %d\n", GetLastError());
    ok(bytes == 4, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == 4, "expected 4, got %lu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    S(U(ovl)).Offset = 0;
    S(U(ovl)).OffsetHigh = 0;
    ovl.Internal = -1;
    ovl.InternalHigh = -1;
    ovl.hEvent = 0;
    bytes = 0;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buf, sizeof(buf), &bytes, &ovl);
    /* ReadFile return value depends on Windows version and testing it is not practical */
    if (!ret)
    {
        ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
        ok(bytes == 0, "bytes %u\n", bytes);
        ret = WaitForSingleObject(hfile, 3000);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObject error %d\n", ret);
    }
    else ok(bytes == 14, "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %lu\n", ovl.InternalHigh);

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

    bytes = 0xdeadbeef;
    ret = GetOverlappedResult(hfile, &ovl, &bytes, TRUE);
    ok(ret, "GetOverlappedResult error %d\n", GetLastError());
    ok(bytes == sizeof(contents), "bytes %u\n", bytes);
    ok((NTSTATUS)ovl.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", ovl.Internal);
    ok(ovl.InternalHigh == sizeof(contents), "expected sizeof(contents), got %lu\n", ovl.InternalHigh);
    ok(!memcmp(contents, buf, sizeof(contents) - 4), "file contents mismatch\n");
    ok(!memcmp(buf + sizeof(contents) - 4, "ABCD", 4), "file contents mismatch\n");

    off = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
    ok(off == 0, "expected 0, got %u\n", off);

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
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtFsControlFile returned %x\n", status);
    ok(!is_signaled(event), "event is signaled\n");

    status = pNtFsControlFile(file, (HANDLE)0xdeadbeef, NULL, NULL, &iosb, 0xdeadbeef, 0, 0, 0, 0);
    ok(status == STATUS_INVALID_HANDLE, "NtFsControlFile returned %x\n", status);

    memset(&iosb, 0x55, sizeof(iosb));
    status = NtFsControlFile(file, NULL, NULL, NULL, &iosb, FSCTL_PIPE_PEEK, NULL, 0,
                             &peek_buf, sizeof(peek_buf));
    todo_wine
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtFsControlFile failed: %x\n", status);
    ok(iosb.Status == 0x55555555, "iosb.Status = %x\n", iosb.Status);

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
    ok(hfileread != INVALID_HANDLE_VALUE, "could not open temp file, error %d.\n", GetLastError());

    status = pNtFlushBuffersFile(hfile, NULL);
    todo_wine
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#x.\n", status);

    status = pNtFlushBuffersFile(hfile, (IO_STATUS_BLOCK *)0xdeadbeaf);
    todo_wine
    ok(status == STATUS_ACCESS_VIOLATION, "expected STATUS_ACCESS_VIOLATION, got %#x.\n", status);

    status = pNtFlushBuffersFile(hfile, &io_status_block);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x.\n", status);

    status = pNtFlushBuffersFile(hfileread, &io_status_block);
    ok(status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %#x.\n", status);

    status = pNtFlushBuffersFile(NULL, &io_status_block);
    ok(status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %#x.\n", status);

    CloseHandle(hfileread);
    CloseHandle(hfile);
    hfile = CreateFileA(buffer, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, 0, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "could not open temp file, error %d.\n", GetLastError());

    status = pNtFlushBuffersFile(hfile, &io_status_block);
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#x.\n", status);

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
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile(INVALID_HANDLE_VALUE, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "expected STATUS_OBJECT_TYPE_MISMATCH, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);
    ok(buffer[0] == 0xcc, "data at position 0 overwritten\n");

    /* test with 0xdeadbeef */
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile((void *)0xdeadbeef, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_INVALID_HANDLE, "expected STATUS_INVALID_HANDLE, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);
    ok(buffer[0] == 0xcc, "data at position 0 overwritten\n");

    /* test without buffer */
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryEaFile(handle, &io, NULL, 0, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);

    /* test with zero buffer */
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    status = pNtQueryEaFile(handle, &io, buffer, 0, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);

    /* test with very small buffer */
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = 4;
    status = pNtQueryEaFile(handle, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);
    for (i = 0; i < buffer_len && !buffer[i]; i++);
    ok(i == buffer_len,  "expected %u bytes filled with 0x00, got %u bytes\n", buffer_len, i);
    ok(buffer[i] == 0xcc, "data at position %u overwritten\n", buffer[i]);

    /* test with very big buffer */
    U(io).Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset(buffer, 0xcc, EA_BUFFER_SIZE);
    buffer_len = EA_BUFFER_SIZE - 1;
    status = pNtQueryEaFile(handle, &io, buffer, buffer_len, TRUE, NULL, 0, NULL, FALSE);
    ok(status == STATUS_NO_EAS_ON_FILE, "expected STATUS_NO_EAS_ON_FILE, got %x\n", status);
    ok(U(io).Status == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", U(io).Status);
    ok(io.Information == 0xdeadbeef, "expected 0xdeadbeef, got %lu\n", io.Information);
    for (i = 0; i < buffer_len && !buffer[i]; i++);
    ok(i == buffer_len,  "expected %u bytes filled with 0x00, got %u bytes\n", buffer_len, i);
    ok(buffer[i] == 0xcc, "data at position %u overwritten\n", buffer[i]);

    CloseHandle(handle);
    #undef EA_BUFFER_SIZE
}

static INT build_reparse_buffer(WCHAR *filename, REPARSE_DATA_BUFFER **pbuffer)
{
    REPARSE_DATA_BUFFER *buffer;
    INT buffer_len, string_len;
    WCHAR *dest;

    string_len = (lstrlenW(filename)+1)*sizeof(WCHAR);
    buffer_len = FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer[1]) + string_len;
    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_len);
    buffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    buffer->ReparseDataLength = sizeof(buffer->MountPointReparseBuffer) + string_len;
    buffer->MountPointReparseBuffer.SubstituteNameLength = string_len - sizeof(WCHAR);
    buffer->MountPointReparseBuffer.PrintNameOffset = string_len;
    dest = &buffer->MountPointReparseBuffer.PathBuffer[0];
    memcpy(dest, filename, string_len);
    *pbuffer = buffer;
    return buffer_len;
}

static void test_junction_points(void)
{
    static const WCHAR junctionW[] = {'\\','j','u','n','c','t','i','o','n',0};
    WCHAR path[MAX_PATH], junction_path[MAX_PATH], target_path[MAX_PATH];
    static const WCHAR targetW[] = {'\\','t','a','r','g','e','t',0};
    FILE_BASIC_INFORMATION old_attrib, new_attrib;
    static const WCHAR fooW[] = {'f','o','o',0};
    static WCHAR volW[] = {'c',':','\\',0};
    REPARSE_GUID_DATA_BUFFER guid_buffer;
    static const WCHAR dotW[] = {'.',0};
    REPARSE_DATA_BUFFER *buffer = NULL;
    DWORD dwret, dwLen, dwFlags, err;
    INT buffer_len, string_len;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING nameW;
    HANDLE hJunction;
    WCHAR *dest;
    BOOL bret;

    /* Create a temporary folder for the junction point tests */
    GetTempFileNameW(dotW, fooW, 0, path);
    DeleteFileW(path);
    if (!CreateDirectoryW(path, NULL))
    {
        win_skip("Unable to create a temporary junction point directory.\n");
        return;
    }

    /* Check that the volume this folder is located on supports junction points */
    pRtlDosPathNameToNtPathName_U(path, &nameW, NULL, NULL);
    volW[0] = nameW.Buffer[4];
    pRtlFreeUnicodeString( &nameW );
    GetVolumeInformationW(volW, 0, 0, 0, &dwLen, &dwFlags, 0, 0);
    if (!(dwFlags & FILE_SUPPORTS_REPARSE_POINTS))
    {
        skip("File system does not support junction points.\n");
        RemoveDirectoryW(path);
        return;
    }

    /* Create the folder to be replaced by a junction point */
    lstrcpyW(junction_path, path);
    lstrcatW(junction_path, junctionW);
    bret = CreateDirectoryW(junction_path, NULL);
    ok(bret, "Failed to create junction point directory.\n");

    /* Create a destination folder for the junction point to target */
    lstrcpyW(target_path, path);
    lstrcatW(target_path, targetW);
    bret = CreateDirectoryW(target_path, NULL);
    ok(bret, "Failed to create junction point target directory.\n");
    pRtlDosPathNameToNtPathName_U(target_path, &nameW, NULL, NULL);

    /* Create the junction point */
    hJunction = CreateFileW(junction_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0);
    if (hJunction == INVALID_HANDLE_VALUE)
    {
        win_skip("Failed to open junction point directory handle (0x%x).\n", GetLastError());
        goto cleanup;
    }
    dwret = NtQueryInformationFile(hJunction, &iosb, &old_attrib, sizeof(old_attrib), FileBasicInformation);
    ok(dwret == STATUS_SUCCESS, "Failed to get junction point folder's attributes (0x%x).\n", dwret);
    buffer_len = build_reparse_buffer(nameW.Buffer, &buffer);
    bret = DeviceIoControl(hJunction, FSCTL_SET_REPARSE_POINT, (LPVOID)buffer, buffer_len, NULL, 0, &dwret, 0);
    ok(bret, "Failed to create junction point! (0x%x)\n", GetLastError());

    /* Check the file attributes of the junction point */
    dwret = GetFileAttributesW(junction_path);
    ok(dwret != (DWORD)~0, "Junction point doesn't exist (attributes: 0x%x)!\n", dwret);
    ok(dwret & FILE_ATTRIBUTE_REPARSE_POINT, "File is not a junction point! (attributes: %d)\n", dwret);

    /* Read back the junction point */
    HeapFree(GetProcessHeap(), 0, buffer);
    buffer_len = sizeof(*buffer) + MAX_PATH*sizeof(WCHAR);
    buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_len);
    bret = DeviceIoControl(hJunction, FSCTL_GET_REPARSE_POINT, NULL, 0, (LPVOID)buffer, buffer_len, &dwret, 0);
    string_len = buffer->MountPointReparseBuffer.SubstituteNameLength;
    dest = &buffer->MountPointReparseBuffer.PathBuffer[buffer->MountPointReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)];
    ok(bret, "Failed to read junction point!\n");
    ok((memcmp(dest, nameW.Buffer, string_len) == 0), "Junction point destination does not match ('%s' != '%s')!\n",
                                                      wine_dbgstr_w(dest), wine_dbgstr_w(nameW.Buffer));

    /* Delete the junction point */
    memset(&old_attrib, 0x00, sizeof(old_attrib));
    old_attrib.LastAccessTime.QuadPart = 0x200deadcafebeef;
    dwret = NtSetInformationFile(hJunction, &iosb, &old_attrib, sizeof(old_attrib), FileBasicInformation);
    ok(dwret == STATUS_SUCCESS, "Failed to set junction point folder's attributes (0x%x).\n", dwret);
    memset(&guid_buffer, 0x00, sizeof(guid_buffer));
    guid_buffer.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    bret = DeviceIoControl(hJunction, FSCTL_DELETE_REPARSE_POINT, (LPVOID)&guid_buffer,
                           REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0, &dwret, 0);
    ok(bret, "Failed to delete junction point! (0x%x)\n", GetLastError());
    memset(&new_attrib, 0x00, sizeof(new_attrib));
    dwret = NtQueryInformationFile(hJunction, &iosb, &new_attrib, sizeof(new_attrib), FileBasicInformation);
    ok(dwret == STATUS_SUCCESS, "Failed to get junction point folder's attributes (0x%x).\n", dwret);
    ok(old_attrib.LastAccessTime.QuadPart == new_attrib.LastAccessTime.QuadPart,
       "Junction point folder's access time does not match.\n");
    CloseHandle(hJunction);

    /* Check deleting a junction point as if it were a directory */
    HeapFree(GetProcessHeap(), 0, buffer);
    hJunction = CreateFileW(junction_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0);
    buffer_len = build_reparse_buffer(nameW.Buffer, &buffer);
    bret = DeviceIoControl(hJunction, FSCTL_SET_REPARSE_POINT, (LPVOID)buffer, buffer_len, NULL, 0, &dwret, 0);
    ok(bret, "Failed to create junction point! (0x%x)\n", GetLastError());
    CloseHandle(hJunction);
    bret = RemoveDirectoryW(junction_path);
    ok(bret, "Failed to delete junction point as directory!\n");
    dwret = GetFileAttributesW(junction_path);
    ok(dwret == (DWORD)~0, "Junction point still exists (attributes: 0x%x)!\n", dwret);

    /* Check deleting a junction point as if it were a file */
    HeapFree(GetProcessHeap(), 0, buffer);
    bret = CreateDirectoryW(junction_path, NULL);
    ok(bret, "Failed to create junction point target directory.\n");
    hJunction = CreateFileW(junction_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0);
    buffer_len = build_reparse_buffer(nameW.Buffer, &buffer);
    bret = DeviceIoControl(hJunction, FSCTL_SET_REPARSE_POINT, (LPVOID)buffer, buffer_len, NULL, 0, &dwret, 0);
    ok(bret, "Failed to create junction point! (0x%x)\n", GetLastError());
    CloseHandle(hJunction);
    bret = DeleteFileW(junction_path);
    ok(!bret, "Succeeded in deleting junction point as file!\n");
    err = GetLastError();
    ok(err == ERROR_ACCESS_DENIED, "Expected last error 0x%x for DeleteFile on junction point (actually 0x%x)!\n",
                                   ERROR_ACCESS_DENIED, err);
    dwret = GetFileAttributesW(junction_path);
    ok(dwret != (DWORD)~0, "Junction point doesn't exist (attributes: 0x%x)!\n", dwret);
    ok(dwret & FILE_ATTRIBUTE_REPARSE_POINT, "File is not a junction point! (attributes: 0x%x)\n", dwret);

    /* Test deleting a junction point's target */
    dwret = GetFileAttributesW(junction_path);
    ok(dwret == 0x410 || broken(dwret == 0x430) /* win2k */,
       "Unexpected junction point attributes (0x%x != 0x410)!\n", dwret);
    bret = RemoveDirectoryW(target_path);
    ok(bret, "Failed to delete junction point target!\n");
    bret = CreateDirectoryW(target_path, NULL);
    ok(bret, "Failed to create junction point target directory.\n");

cleanup:
    /* Cleanup */
    pRtlFreeUnicodeString( &nameW );
    HeapFree(GetProcessHeap(), 0, buffer);
    bret = RemoveDirectoryW(junction_path);
    ok(bret, "Failed to remove temporary junction point directory!\n");
    bret = RemoveDirectoryW(target_path);
    ok(bret, "Failed to remove temporary target directory!\n");
    RemoveDirectoryW(path);
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
    pNtSetIoCompletion      = (void *)GetProcAddress(hntdll, "NtSetIoCompletion");
    pNtSetInformationFile   = (void *)GetProcAddress(hntdll, "NtSetInformationFile");
    pNtQueryInformationFile = (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pNtQueryVolumeInformationFile = (void *)GetProcAddress(hntdll, "NtQueryVolumeInformationFile");
    pNtQueryFullAttributesFile = (void *)GetProcAddress(hntdll, "NtQueryFullAttributesFile");
    pNtFlushBuffersFile = (void *)GetProcAddress(hntdll, "NtFlushBuffersFile");
    pNtQueryEaFile          = (void *)GetProcAddress(hntdll, "NtQueryEaFile");

    test_read_write();
    test_NtCreateFile();
    test_readonly();
    create_file_test();
    open_file_test();
    delete_file_test();
    read_file_test();
    append_file_test();
    nt_mailslot_test();
    test_iocompletion();
    test_file_basic_information();
    test_file_all_information();
    test_file_both_information();
    test_file_name_information();
    test_file_full_size_information();
    test_file_all_name_information();
    test_file_rename_information();
    test_file_link_information();
    test_file_disposition_information();
    test_file_completion_information();
    test_file_id_information();
    test_file_access_information();
    test_query_volume_information_file();
    test_query_attribute_information_file();
    test_ioctl();
    test_flush_buffers_file();
    test_query_ea();
    test_junction_points();
}
