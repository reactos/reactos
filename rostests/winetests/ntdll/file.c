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

static NTSTATUS (WINAPI *pNtCreateIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI *pNtOpenIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtQueryIoCompletion)(HANDLE, IO_COMPLETION_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtRemoveIoCompletion)(HANDLE, PULONG_PTR, PULONG_PTR, PIO_STATUS_BLOCK, PLARGE_INTEGER);
static NTSTATUS (WINAPI *pNtSetIoCompletion)(HANDLE, ULONG_PTR, ULONG_PTR, NTSTATUS, ULONG);
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtQueryDirectoryFile)(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,
                                                PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
static NTSTATUS (WINAPI *pNtQueryVolumeInformationFile)(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);

static inline BOOL is_signaled( HANDLE obj )
{
    return WaitForSingleObject( obj, 0 ) == 0;
}

#define PIPENAME "\\\\.\\pipe\\ntdll_tests_file.c"
#define TEST_BUF_LEN 3

static BOOL create_pipe( HANDLE *read, HANDLE *write, ULONG flags, ULONG size )
{
    *read = CreateNamedPipe(PIPENAME, PIPE_ACCESS_INBOUND | flags, PIPE_TYPE_BYTE | PIPE_WAIT,
                            1, size, size, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(*read != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    *write = CreateFileA(PIPENAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(*write != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    return TRUE;
}

static HANDLE create_temp_file( ULONG flags )
{
    char buffer[MAX_PATH];
    HANDLE handle;

    GetTempFileNameA( ".", "foo", 0, buffer );
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
    static const WCHAR systemrootW[] = {'\\','S','y','s','t','e','m','R','o','o','t',
                                        '\\','f','a','i','l','i','n','g',0};
    static const WCHAR questionmarkInvalidNameW[] = {'a','f','i','l','e','?',0};
    static const WCHAR pipeInvalidNameW[]  = {'a','|','b',0};
    NTSTATUS status;
    HANDLE dir, file;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;

    GetCurrentDirectoryW( MAX_PATH, path );
    pRtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    /* try various open modes and options on directories */
    status = pNtCreateFile( &dir, GENERIC_READ, &attr, &io, NULL, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN, FILE_DIRECTORY_FILE, NULL, 0 );
    ok( !status, "open %s failed %x\n", wine_dbgstr_w(nameW.Buffer), status );
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
}

static void open_file_test(void)
{
    NTSTATUS status;
    HANDLE dir, root, handle;
    WCHAR path[MAX_PATH];
    BYTE data[1024];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    UINT i, len;
    BOOL restart = TRUE;

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
    HANDLE handle, read, write;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb, iosb2;
    DWORD written;
    int apc_count = 0;
    char buffer[128];
    LARGE_INTEGER offset;
    HANDLE event = CreateEventA( NULL, TRUE, FALSE, NULL );
    BOOL ret;

    buffer[0] = 1;

    if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;

    /* try read with no data */
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( is_signaled( read ), "read handle is not signaled\n" );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* iosb updated here by async i/o */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    apc_count = 0;
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* with no event, the pipe handle itself gets signaled */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( !is_signaled( read ), "read handle is not signaled\n" );
    status = pNtReadFile( read, 0, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( read ), "read handle is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* iosb updated here by async i/o */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( read ), "read handle is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    apc_count = 0;
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* now read with data ready */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ResetEvent( event );
    WriteFile( write, buffer, 1, &written, NULL );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_SUCCESS, "wrong status %x\n", status );
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, FALSE ); /* non-alertable sleep */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc not called\n" );

    /* try read with no data */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    ok( is_signaled( event ), "event is not signaled\n" ); /* check that read resets the event */
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    WriteFile( write, buffer, 1, &written, NULL );
    /* partial read is good enough */
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 1, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read from disconnected pipe */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    CloseHandle( write );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_PIPE_BROKEN, "wrong status %x\n", status );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );
    CloseHandle( read );

    /* read from closed handle */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    SetEvent( event );
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 1, NULL, NULL );
    ok( status == STATUS_INVALID_HANDLE, "wrong status %x\n", status );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );  /* not reset on invalid handle */
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    /* disconnect while async read is in progress */
    if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    CloseHandle( write );
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == STATUS_PIPE_BROKEN, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );
    CloseHandle( read );

    if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;
    ret = DuplicateHandle(GetCurrentProcess(), read, GetCurrentProcess(), &handle, 0, TRUE, DUPLICATE_SAME_ACCESS);
    ok(ret, "Failed to duplicate handle: %d\n", GetLastError());

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    /* Cancel by other handle */
    status = pNtCancelIoFile( read, &iosb2 );
    ok(status == STATUS_SUCCESS, "failed to cancel by different handle: %x\n", status);
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == STATUS_CANCELLED, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    todo_wine ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
    ok( !is_signaled( event ), "event is signaled\n" );
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    ok( !apc_count, "apc was called\n" );
    /* Close queued handle */
    CloseHandle( read );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
    status = pNtCancelIoFile( read, &iosb2 );
    ok(status == STATUS_INVALID_HANDLE, "cancelled by closed handle?\n");
    status = pNtCancelIoFile( handle, &iosb2 );
    ok(status == STATUS_SUCCESS, "failed to cancel: %x\n", status);
    Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
    ok( U(iosb).Status == STATUS_CANCELLED, "wrong status %x\n", U(iosb).Status );
    ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    ok( is_signaled( event ), "event is signaled\n" );
    todo_wine ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );
    CloseHandle( handle );
    CloseHandle( write );

    if (pNtCancelIoFileEx)
    {
        /* Basic Cancel Ex */
        if (!create_pipe( &read, &write, FILE_FLAG_OVERLAPPED, 4096 )) return;

        apc_count = 0;
        U(iosb).Status = 0xdeadbabe;
        iosb.Information = 0xdeadbeef;
        status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
        ok( status == STATUS_PENDING, "wrong status %x\n", status );
        ok( !is_signaled( event ), "event is signaled\n" );
        ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
        ok( !apc_count, "apc was called\n" );
        status = pNtCancelIoFileEx( read, &iosb, &iosb2 );
        ok(status == STATUS_SUCCESS, "Failed to cancel I/O\n");
        Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
        ok( U(iosb).Status == STATUS_CANCELLED, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
        ok( is_signaled( event ), "event is signaled\n" );
        todo_wine ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( apc_count == 1, "apc was not called\n" );

        /* Duplicate iosb */
        apc_count = 0;
        U(iosb).Status = 0xdeadbabe;
        iosb.Information = 0xdeadbeef;
        status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
        ok( status == STATUS_PENDING, "wrong status %x\n", status );
        ok( !is_signaled( event ), "event is signaled\n" );
        ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
        ok( !apc_count, "apc was called\n" );
        status = pNtReadFile( read, event, apc, &apc_count, &iosb, buffer, 2, NULL, NULL );
        ok( status == STATUS_PENDING, "wrong status %x\n", status );
        ok( !is_signaled( event ), "event is signaled\n" );
        ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
        ok( !apc_count, "apc was called\n" );
        status = pNtCancelIoFileEx( read, &iosb, &iosb2 );
        ok(status == STATUS_SUCCESS, "Failed to cancel I/O\n");
        Sleep(1);  /* FIXME: needed for wine to run the i/o apc  */
        ok( U(iosb).Status == STATUS_CANCELLED, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
        ok( is_signaled( event ), "event is signaled\n" );
        todo_wine ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( apc_count == 2, "apc was not called\n" );

        CloseHandle( read );
        CloseHandle( write );
    }

    /* now try a real file */
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
    ok( is_signaled( event ), "event is signaled\n" );
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
    ok( is_signaled( event ), "event is signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( apc_count == 1, "apc was not called\n" );

    /* read beyond eof */
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = strlen(text) + 2;
    status = pNtReadFile( handle, event, apc, &apc_count, &iosb, buffer, 2, &offset, NULL );
    if (status == STATUS_PENDING)  /* vista */
    {
        WaitForSingleObject( event, 1000 );
        ok( U(iosb).Status == STATUS_END_OF_FILE, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
        ok( is_signaled( event ), "event is signaled\n" );
        ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( apc_count == 1, "apc was not called\n" );
    }
    else
    {
        ok( status == STATUS_END_OF_FILE, "wrong status %x\n", status );
        ok( U(iosb).Status == 0xdeadbabe, "wrong status %x\n", U(iosb).Status );
        ok( iosb.Information == 0xdeadbeef, "wrong info %lu\n", iosb.Information );
        ok( !is_signaled( event ), "event is signaled\n" );
        ok( !apc_count, "apc was called\n" );
        SleepEx( 1, TRUE ); /* alertable sleep */
        ok( !apc_count, "apc was called\n" );
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
    ok( is_signaled( event ), "event is signaled\n" );
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
    ok( is_signaled( event ), "event is signaled\n" );
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
    todo_wine ok( U(iosb).Status == STATUS_END_OF_FILE, "wrong status %x\n", U(iosb).Status );
    todo_wine ok( iosb.Information == 0, "wrong info %lu\n", iosb.Information );
    todo_wine ok( is_signaled( event ), "event is not signaled\n" );
    ok( !apc_count, "apc was called\n" );
    SleepEx( 1, TRUE ); /* alertable sleep */
    ok( !apc_count, "apc was called\n" );

    CloseHandle( handle );

    CloseHandle( event );
}

static void append_file_test(void)
{
    const char text[] = "foobar";
    HANDLE handle;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    DWORD written;
    char buffer[128];

    GetTempFileNameA( ".", "foo", 0, buffer );
    /* It is possible to open a file with only FILE_APPEND_DATA access flags.
       It matches the O_WRONLY|O_APPEND open() posix behavior */
    handle = CreateFileA(buffer, FILE_APPEND_DATA, 0, NULL, CREATE_ALWAYS,
                         FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok( handle != INVALID_HANDLE_VALUE, "Failed to create a temp file in FILE_APPEND_DATA mode.\n" );
    if(handle == INVALID_HANDLE_VALUE)
    {
        skip("Couldn't create a temporary file, skipping FILE_APPEND_DATA test\n");
        return;
    }

    U(iosb).Status = STATUS_PENDING;
    iosb.Information = 0;

    status = pNtWriteFile(handle, NULL, NULL, NULL, &iosb,
                          text, sizeof(text), NULL, NULL);

    if (status == STATUS_PENDING)
    {
        WaitForSingleObject( handle, 1000 );
        status = U(iosb).Status;
    }
    written = iosb.Information;

    todo_wine
    ok(status == STATUS_SUCCESS && written == sizeof(text), "FILE_APPEND_DATA NtWriteFile failed\n");

    CloseHandle(handle);
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
     * Test that the length field is checked properly
     */
    attr.Length = 0;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    todo_wine ok( rc == STATUS_INVALID_PARAMETER, "rc = %x not c000000d STATUS_INVALID_PARAMETER\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

    attr.Length = sizeof(OBJECT_ATTRIBUTES)+1;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    todo_wine ok( rc == STATUS_INVALID_PARAMETER, "rc = %x not c000000d STATUS_INVALID_PARAMETER\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

    /*
     * Test handling of a NULL unicode string in ObjectName
     */
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    attr.ObjectName = NULL;
    rc = pNtCreateMailslotFile(&hslot, DesiredAccess,
         &attr, &IoStatusBlock, CreateOptions, MailslotQuota, MaxMessageSize,
         &TimeOut);
    ok( rc == STATUS_OBJECT_PATH_SYNTAX_BAD ||
        rc == STATUS_INVALID_PARAMETER,
        "rc = %x not STATUS_OBJECT_PATH_SYNTAX_BAD or STATUS_INVALID_PARAMETER\n", rc);

    if  (rc == STATUS_SUCCESS) pNtClose(hslot);

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

    res = pNtSetIoCompletion( h, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, 3 );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %x\n", res );

    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %d\n", count );

    if (get_msg(h))
    {
        ok( completionKey == CKEY_FIRST, "Invalid completion key: %lx\n", completionKey );
        ok( ioSb.Information == 3, "Invalid ioSb.Information: %ld\n", ioSb.Information );
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
            todo_wine ok( U(ioSb).Status == STATUS_PIPE_BROKEN, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
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
    }

    CloseHandle( hPipeSrv );
    CloseHandle( hPipeClt );
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
    todo_wine ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %x not FILE_ATTRIBUTE_SYSTEM\n", fbi.FileAttributes );

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
    todo_wine ok ( (fbi.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %x not FILE_ATTRIBUTE_HIDDEN\n", fbi.FileAttributes );

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
    todo_wine ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_SYSTEM, "attribute %x not FILE_ATTRIBUTE_SYSTEM\n", fai_buf.fai.BasicInformation.FileAttributes );

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
    todo_wine ok ( (fai_buf.fai.BasicInformation.FileAttributes & attrib_mask) == FILE_ATTRIBUTE_HIDDEN, "attribute %x not FILE_ATTRIBUTE_HIDDEN\n", fai_buf.fai.BasicInformation.FileAttributes );

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

todo_wine
{
    ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", status);
    ok(U(io).Status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %d\n", U(io).Status);

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
    static const WCHAR dotW[] = {'.',0};
    NTSTATUS status;
    HANDLE handle;
    WCHAR path[MAX_PATH];
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nameW;
    DWORD ret, i;

    GetTempFileNameW(dotW, fooW, 0, path);
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

    SetFileAttributesW(path, FILE_ATTRIBUTE_ARCHIVE);
    DeleteFileW( path );
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
    pNtCreateIoCompletion   = (void *)GetProcAddress(hntdll, "NtCreateIoCompletion");
    pNtOpenIoCompletion     = (void *)GetProcAddress(hntdll, "NtOpenIoCompletion");
    pNtQueryIoCompletion    = (void *)GetProcAddress(hntdll, "NtQueryIoCompletion");
    pNtRemoveIoCompletion   = (void *)GetProcAddress(hntdll, "NtRemoveIoCompletion");
    pNtSetIoCompletion      = (void *)GetProcAddress(hntdll, "NtSetIoCompletion");
    pNtSetInformationFile   = (void *)GetProcAddress(hntdll, "NtSetInformationFile");
    pNtQueryInformationFile = (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtQueryDirectoryFile   = (void *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
    pNtQueryVolumeInformationFile = (void *)GetProcAddress(hntdll, "NtQueryVolumeInformationFile");

    test_NtCreateFile();
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
    test_file_all_name_information();
    test_query_volume_information_file();
}
