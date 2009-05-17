/* Unit test suite for Ntdll file functions
 *
 * Copyright 2007 Jeff Latimer
 * Copyright 2007 Andrey Turkin
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

#ifndef IO_COMPLETION_ALL_ACCESS
#define IO_COMPLETION_ALL_ACCESS 0x001F0003
#endif

static NTSTATUS (WINAPI *pRtlFreeUnicodeString)( PUNICODE_STRING );
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static BOOL     (WINAPI *pRtlDosPathNameToNtPathName_U)( LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR* );
static NTSTATUS (WINAPI *pNtCreateMailslotFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, PLARGE_INTEGER );
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
static NTSTATUS (WINAPI *pNtClose)( PHANDLE );

static NTSTATUS (WINAPI *pNtCreateIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI *pNtOpenIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtQueryIoCompletion)(HANDLE, IO_COMPLETION_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtRemoveIoCompletion)(HANDLE, PULONG_PTR, PULONG_PTR, PIO_STATUS_BLOCK, PLARGE_INTEGER);
static NTSTATUS (WINAPI *pNtSetIoCompletion)(HANDLE, ULONG_PTR, ULONG_PTR, NTSTATUS, ULONG);
static NTSTATUS (WINAPI *pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

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

ULONG_PTR completionKey;
IO_STATUS_BLOCK ioSb;
ULONG_PTR completionValue;

static long get_pending_msgs(HANDLE h)
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
    IO_STATUS_BLOCK iosb;
    DWORD written;
    int apc_count = 0;
    char buffer[128];
    LARGE_INTEGER offset;
    HANDLE event = CreateEventA( NULL, TRUE, FALSE, NULL );

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

    /* now try a real file */
    if (!(handle = create_temp_file( FILE_FLAG_OVERLAPPED ))) return;
    apc_count = 0;
    U(iosb).Status = 0xdeadbabe;
    iosb.Information = 0xdeadbeef;
    offset.QuadPart = 0;
    ResetEvent( event );
    pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_PENDING, "wrong status %x\n", status );
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
    pNtWriteFile( handle, event, apc, &apc_count, &iosb, text, strlen(text), &offset, NULL );
    ok( status == STATUS_END_OF_FILE ||
        status == STATUS_PENDING,  /* vista */
        "wrong status %x\n", status );
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

    if  ( rc == STATUS_SUCCESS ) rc = pNtClose(hslot);

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
    long count;

    res = pNtSetIoCompletion( h, CKEY_FIRST, CVALUE_FIRST, STATUS_INVALID_DEVICE_REQUEST, 3 );
    ok( res == STATUS_SUCCESS, "NtSetIoCompletion failed: %x\n", res );

    count = get_pending_msgs(h);
    ok( count == 1, "Unexpected msg count: %ld\n", count );

    if (get_msg(h))
    {
        ok( completionKey == CKEY_FIRST, "Invalid completion key: %lx\n", completionKey );
        ok( ioSb.Information == 3, "Invalid ioSb.Information: %ld\n", ioSb.Information );
        ok( U(ioSb).Status == STATUS_INVALID_DEVICE_REQUEST, "Invalid ioSb.Status: %x\n", U(ioSb).Status);
        ok( completionValue == CVALUE_FIRST, "Invalid completion value: %lx\n", completionValue );
    }

    count = get_pending_msgs(h);
    ok( !count, "Unexpected msg count: %ld\n", count );
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
            res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
            ok( res == STATUS_INVALID_PARAMETER, "Unexpected NtSetInformationFile on non-overlapped handle: %x\n", res );
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

        NTSTATUS res = pNtSetInformationFile( hPipeSrv, &iosb, &fci, sizeof(fci), FileCompletionInformation );
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

START_TEST(file)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pNtCreateMailslotFile   = (void *)GetProcAddress(hntdll, "NtCreateMailslotFile");
    pNtDeleteFile           = (void *)GetProcAddress(hntdll, "NtDeleteFile");
    pNtReadFile             = (void *)GetProcAddress(hntdll, "NtReadFile");
    pNtWriteFile            = (void *)GetProcAddress(hntdll, "NtWriteFile");
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pNtCreateIoCompletion   = (void *)GetProcAddress(hntdll, "NtCreateIoCompletion");
    pNtOpenIoCompletion     = (void *)GetProcAddress(hntdll, "NtOpenIoCompletion");
    pNtQueryIoCompletion    = (void *)GetProcAddress(hntdll, "NtQueryIoCompletion");
    pNtRemoveIoCompletion   = (void *)GetProcAddress(hntdll, "NtRemoveIoCompletion");
    pNtSetIoCompletion      = (void *)GetProcAddress(hntdll, "NtSetIoCompletion");
    pNtSetInformationFile   = (void *)GetProcAddress(hntdll, "NtSetInformationFile");

    delete_file_test();
    read_file_test();
    nt_mailslot_test();
    test_iocompletion();
}
