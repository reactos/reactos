/*
 * Unit test suite for object manager functions
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2005 Vitaliy Margolen
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#ifndef __REACTOS__
#include "ddk/wdm.h"
#endif
#include "wine/test.h"
#ifdef __REACTOS__
/* These aren't in the Windows SDK but Wine needs them. */
#define DIRECTORY_QUERY                 0x0001
#define DIRECTORY_TRAVERSE              0x0002
#define DIRECTORY_CREATE_OBJECT         0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY   0x0008
#define DIRECTORY_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED | 0xF)
#define SYMBOLIC_LINK_QUERY             0x0001
#define SYMBOLIC_LINK_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | 0x1)
#define EVENT_QUERY_STATE               0x0001
#define SEMAPHORE_QUERY_STATE           0x0001

/* These are here because Wine's headers aren't compatible */
#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof(s[0]), sizeof(s), (void*)s }
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_ERROR(Status)                ((((ULONG)(Status)) >> 30) == 3)
#endif

static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static NTSTATUS (WINAPI *pNtAllocateReserveObject)( HANDLE *, const OBJECT_ATTRIBUTES *, MEMORY_RESERVE_OBJECT_TYPE );
static NTSTATUS (WINAPI *pNtCreateEvent) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, EVENT_TYPE, BOOLEAN);
static NTSTATUS (WINAPI *pNtOpenEvent)   ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateJobObject)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtOpenJobObject)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateKey)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG,
                                        const UNICODE_STRING *, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtOpenKey)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtDeleteKey)( HANDLE );
static NTSTATUS (WINAPI *pNtCreateMailslotFile)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                                 ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtCreateMutant)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN );
static NTSTATUS (WINAPI *pNtOpenMutant)  ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateSemaphore)( PHANDLE, ACCESS_MASK,const POBJECT_ATTRIBUTES,LONG,LONG );
static NTSTATUS (WINAPI *pNtOpenSemaphore)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateTimer) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, TIMER_TYPE );
static NTSTATUS (WINAPI *pNtOpenTimer)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateSection)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, const PLARGE_INTEGER,
                                            ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtOpenSection)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtOpenFile)    ( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG );
static NTSTATUS (WINAPI *pNtClose)       ( HANDLE );
static NTSTATUS (WINAPI *pNtCreateNamedPipeFile)( PHANDLE, ULONG, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                       ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, PLARGE_INTEGER );
static NTSTATUS (WINAPI *pNtOpenDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateDirectoryObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtOpenSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtCreateSymbolicLinkObject)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PUNICODE_STRING);
static NTSTATUS (WINAPI *pNtQuerySymbolicLinkObject)(HANDLE,PUNICODE_STRING,PULONG);
static NTSTATUS (WINAPI *pNtQueryObject)(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
static NTSTATUS (WINAPI *pNtReleaseSemaphore)(HANDLE, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtCreateKeyedEvent)( HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *, ULONG );
static NTSTATUS (WINAPI *pNtOpenKeyedEvent)( HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES * );
static NTSTATUS (WINAPI *pNtCreateIoCompletion)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG);
static NTSTATUS (WINAPI *pNtOpenIoCompletion)( PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, void *, ULONG, FILE_INFORMATION_CLASS);
static NTSTATUS (WINAPI *pNtOpenProcess)( HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *, const CLIENT_ID * );
static NTSTATUS (WINAPI *pNtCreateDebugObject)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *, ULONG );
static NTSTATUS (WINAPI *pNtGetNextThread)(HANDLE process, HANDLE thread, ACCESS_MASK access, ULONG attributes,
                                            ULONG flags, HANDLE *handle);
static NTSTATUS (WINAPI *pNtOpenProcessToken)(HANDLE,DWORD,HANDLE*);
static NTSTATUS (WINAPI *pNtOpenThread)(HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *, const CLIENT_ID * );
static NTSTATUS (WINAPI *pNtOpenThreadToken)(HANDLE,DWORD,BOOLEAN,HANDLE*);
static NTSTATUS (WINAPI *pNtDuplicateToken)(HANDLE,ACCESS_MASK,OBJECT_ATTRIBUTES*,BOOLEAN,TOKEN_TYPE,HANDLE*);
static NTSTATUS (WINAPI *pNtDuplicateObject)(HANDLE,HANDLE,HANDLE,HANDLE*,ACCESS_MASK,ULONG,ULONG);
static NTSTATUS (WINAPI *pNtCompareObjects)(HANDLE,HANDLE);

#define KEYEDEVENT_WAIT       0x0001
#define KEYEDEVENT_WAKE       0x0002
#define KEYEDEVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x0003)
#define DESKTOP_ALL_ACCESS    0x01ff

#define check_unicode_string(a, b) check_unicode_string_(__LINE__, a, b)
static void check_unicode_string_( int line, const UNICODE_STRING *string, const WCHAR *expect )
{
    size_t len = wcslen( expect ) * sizeof(WCHAR);

    ok_(__FILE__, line)( !wcscmp( string->Buffer, expect ), "got string %s\n", debugstr_w( string->Buffer ));
    ok_(__FILE__, line)( string->Length == len, "got length %u\n", string->Length );
    ok_(__FILE__, line)( string->MaximumLength == len + sizeof(WCHAR), "got max length %u\n", string->MaximumLength );
}

static void test_case_sensitive (void)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE Event, Mutant, h;

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\test");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08lx)\n", status);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, NotificationEvent, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH /* Vista+ */, "got %#lx\n", status);

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Test");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, NotificationEvent, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Event(%08lx)\n", status);

    pRtlInitUnicodeString(&str, L"\\BaseNamedObjects\\TEst");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenMutant(&h, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtOpenMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);

    pNtClose(Mutant);

    pRtlInitUnicodeString(&str, L"\\BASENamedObjects\\test");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH /* Vista+ */, "got %#lx\n", status);

    status = pNtCreateEvent(&h, GENERIC_ALL, &attr, NotificationEvent, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION got(%08lx)\n", status);

    attr.Attributes = 0;
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);

    pNtClose(Event);
}

static void test_namespace_pipe(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    HANDLE pipe, h;

    timeout.QuadPart = -10000;

    pRtlInitUnicodeString(&str, L"\\??\\PIPE\\test\\pipe");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile((HANDLE *)0xdeadbee0, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_ACCESS_VIOLATION, "Failed to create NamedPipe(%08lx)\n", status);

    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_SUCCESS, "Failed to create NamedPipe(%08lx)\n", status);

    h = (HANDLE)0xdeadbeef;
    status = pNtCreateNamedPipeFile(&h, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08lx)\n", status);
    ok( !h || broken(h == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", h );

    pRtlInitUnicodeString(&str, L"\\??\\PIPE\\TEST\\PIPE");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    h = (HANDLE)0xdeadbeef;
    status = pNtCreateNamedPipeFile(&h, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08lx)\n", status);
    ok( !h || broken(h == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", h );

    h = CreateFileA("\\\\.\\pipe\\test\\pipe", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, 0 );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open NamedPipe (%lu)\n", GetLastError());
    pNtClose(h);

    pRtlInitUnicodeString(&str, L"\\??\\pipe\\test\\pipe");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    h = (HANDLE)0xdeadbeef;
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND ||
       status == STATUS_PIPE_NOT_AVAILABLE ||
       status == STATUS_OBJECT_NAME_INVALID || /* vista */
       status == STATUS_OBJECT_NAME_NOT_FOUND, /* win8 */
        "NtOpenFile should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);
    ok( !h || broken(h == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", h );

    pRtlInitUnicodeString(&str, L"\\??\\pipe\\test");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    h = (HANDLE)0xdeadbeef;
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND ||
       status == STATUS_OBJECT_NAME_INVALID, /* vista */
        "NtOpenFile should have failed with STATUS_OBJECT_NAME_NOT_FOUND got(%08lx)\n", status);
    ok( !h || broken(h == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", h );

    str.Length -= 4 * sizeof(WCHAR);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "NtOpenFile should have succeeded got %08lx\n", status);
    pNtClose( h );

    str.Length -= sizeof(WCHAR);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "NtOpenFile should have succeeded got %08lx\n", status);
    pNtClose( h );

    pNtClose(pipe);
}

#define check_create_open_dir(parent, name, status) check_create_open_dir_(__LINE__, parent, name, status)
static void check_create_open_dir_( int line, HANDLE parent, const WCHAR *name, NTSTATUS expect )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    NTSTATUS status;
    HANDLE h;

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes( &attr, &str, 0, parent, NULL );
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok_(__FILE__, line)( status == expect, "NtCreateDirectoryObject(%s) got %08lx\n", debugstr_w(name), status );
    if (!status) pNtClose( h );

    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok_(__FILE__, line)( status == expect, "NtOpenDirectoryObject(%s) got %08lx\n", debugstr_w(name), status );
    if (!status) pNtClose( h );
}

static BOOL is_correct_dir( HANDLE dir, const WCHAR *name )
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE h = 0;

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    if (h) pNtClose( h );
    return (status == STATUS_OBJECT_NAME_EXISTS);
}

/* return a handle to the BaseNamedObjects dir where kernel32 objects get created */
static HANDLE get_base_dir(void)
{
    static const WCHAR objname[] = L"om.c_get_base_dir_obj";
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h;
    WCHAR name[40];

    h = CreateMutexW( NULL, FALSE, objname );
    ok(h != 0, "CreateMutexA failed got ret=%p (%ld)\n", h, GetLastError());
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    swprintf( name, ARRAY_SIZE(name), L"\\BaseNamedObjects\\Session\\%u", NtCurrentTeb()->Peb->SessionId );
    RtlInitUnicodeString( &str, name );
    status = pNtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
    ok(!status, "got %#lx\n", status);
    ok(is_correct_dir( dir, objname ), "wrong dir\n");

    pNtClose( h );
    return dir;
}

static void test_name_collisions(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h, h1, h2;
    DWORD winerr;
    LARGE_INTEGER size, timeout;
    IO_STATUS_BLOCK iosb;

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_COLLISION, "NtCreateDirectoryObject got %08lx\n", status );
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "NtCreateDirectoryObject got %08lx\n", status );
    pNtClose(h);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);

    RtlInitUnicodeString(&str, L"\\??\\PIPE\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    todo_wine ok(status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#lx\n", status);

    dir = get_base_dir();
    RtlInitUnicodeString(&str, L"om.c-test");
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    h = CreateMutexA(NULL, FALSE, "om.c-test");
    ok(h != 0, "CreateMutexA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateMutant(&h1, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateMutant should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateMutexA(NULL, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateMutexA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    ok(h != 0, "CreateEventA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateEvent(&h1, GENERIC_ALL, &attr, NotificationEvent, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateEvent should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateEventA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    ok(h != 0, "CreateSemaphoreA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateSemaphore(&h1, GENERIC_ALL, &attr, 1, 2);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSemaphore should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateSemaphoreA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    ok(h != 0, "CreateWaitableTimerA failed got ret=%p (%ld)\n", h, GetLastError());
    status = pNtCreateTimer(&h1, GENERIC_ALL, &attr, NotificationTimer);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateTimer should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateWaitableTimerA(NULL, TRUE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateWaitableTimerA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    ok(h != 0, "CreateFileMappingA failed got ret=%p (%ld)\n", h, GetLastError());
    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection(&h1, SECTION_MAP_WRITE, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSection should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08lx)\n", status);
    h2 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateFileMappingA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%ld)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    pNtClose(dir);

    RtlInitUnicodeString(&str, L"\\??\\PIPE\\named_pipe");
    attr.RootDirectory = 0;
    timeout.QuadPart = -10000;
    status = pNtCreateNamedPipeFile( &h, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OPEN, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, &timeout );
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "failed to create pipe %08lx\n", status);

    memset( &iosb, 0xcc, sizeof(iosb) );
    status = pNtCreateNamedPipeFile( &h, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OPEN_IF, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, &timeout );
    ok(status == STATUS_SUCCESS, "failed to create pipe %08lx\n", status);
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %08lx\n", status);
    ok( iosb.Information == FILE_CREATED, "wrong info %Ix\n", iosb.Information );
    pNtClose( h );

    memset( &iosb, 0xcc, sizeof(iosb) );
    status = pNtCreateNamedPipeFile( &h, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, &timeout );
    ok(status == STATUS_SUCCESS, "failed to create pipe %08lx\n", status);
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %08lx\n", status);
    ok( iosb.Information == FILE_CREATED, "wrong info %Ix\n", iosb.Information );

    memset( &iosb, 0xcc, sizeof(iosb) );
    status = pNtCreateNamedPipeFile( &h1, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OPEN, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, &timeout );
    ok(status == STATUS_SUCCESS, "failed to create pipe %08lx\n", status);
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %08lx\n", status);
    ok( iosb.Information == FILE_OPENED, "wrong info %Ix\n", iosb.Information );
    pNtClose(h1);

    memset( &iosb, 0xcc, sizeof(iosb) );
    status = pNtCreateNamedPipeFile( &h1, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OPEN_IF, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, &timeout );
    ok(status == STATUS_SUCCESS, "failed to create pipe %08lx\n", status);
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %08lx\n", status);
    ok( iosb.Information == FILE_OPENED, "wrong info %Ix\n", iosb.Information );
    pNtClose(h1);

    memset( &iosb, 0xcc, sizeof(iosb) );
    status = pNtCreateNamedPipeFile( &h1, GENERIC_READ|GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_OPEN_IF, FILE_PIPE_FULL_DUPLEX,
                                     FALSE, FALSE, FALSE, 10, 256, 256, NULL );
    ok(status == STATUS_SUCCESS, "failed to create pipe %08lx\n", status);
    ok( iosb.Status == STATUS_SUCCESS, "wrong status %08lx\n", status);
    ok( iosb.Information == FILE_OPENED, "wrong info %Ix\n", iosb.Information );
    pNtClose(h1);

    h1 = CreateNamedPipeA( "\\\\.\\pipe\\named_pipe", PIPE_ACCESS_DUPLEX,
                          PIPE_READMODE_BYTE, 10, 256, 256, 1000, NULL );
    winerr = GetLastError();
    ok(h1 != 0 && winerr == ERROR_ALREADY_EXISTS, "CreateNamedPipeA got ret=%p (%ld)\n", h1, winerr);
    pNtClose(h1);
    pNtClose(h);
}

static void test_all_kernel_objects( UINT line, OBJECT_ATTRIBUTES *attr,
                                     NTSTATUS create_expect, NTSTATUS open_expect )
{
    UNICODE_STRING target;
    LARGE_INTEGER size;
    NTSTATUS status, status2;
    HANDLE ret, ret2;

    RtlInitUnicodeString( &target, L"\\DosDevices" );
    size.QuadPart = 4096;

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateMutant( &ret, GENERIC_ALL, attr, FALSE );
    ok( status == create_expect, "%u: NtCreateMutant failed %lx\n", line, status );
    status2 = pNtOpenMutant( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenMutant failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateMutant handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenMutant handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, attr, 1, 2 );
    ok( status == create_expect, "%u: NtCreateSemaphore failed %lx\n", line, status );
    status2 = pNtOpenSemaphore( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenSemaphore failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateSemaphore handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenSemaphore handle %p\n", line, ret );
    ret = (HANDLE)0xdeadbeef;
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, attr, 2, 1 );
    ok( status == STATUS_INVALID_PARAMETER ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateSemaphore failed %lx\n", line, status );
    ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateSemaphore handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateEvent( &ret, GENERIC_ALL, attr, SynchronizationEvent, 0 );
    ok( status == create_expect, "%u: NtCreateEvent failed %lx\n", line, status );
    status2 = pNtOpenEvent( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenEvent failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateEvent handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenEvent handle %p\n", line, ret );
    ret = (HANDLE)0xdeadbeef;
    status = pNtCreateEvent( &ret, GENERIC_ALL, attr, 2, 0 );
    ok( status == STATUS_INVALID_PARAMETER ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateEvent failed %lx\n", line, status );
    ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateEvent handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, attr, 0 );
    ok( status == create_expect, "%u: NtCreateKeyedEvent failed %lx\n", line, status );
    status2 = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenKeyedEvent failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateKeyedEvent handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenKeyedEvent handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateTimer( &ret, GENERIC_ALL, attr, NotificationTimer );
    ok( status == create_expect, "%u: NtCreateTimer failed %lx\n", line, status );
    status2 = pNtOpenTimer( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenTimer failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateTimer handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenTimer handle %p\n", line, ret );
    ret = (HANDLE)0xdeadbeef;
    status = pNtCreateTimer( &ret, GENERIC_ALL, attr, 2 );
    ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_4 ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateTimer failed %lx\n", line, status );
    ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateTimer handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, attr, 0 );
    ok( status == create_expect, "%u: NtCreateIoCompletion failed %lx\n", line, status );
    status2 = pNtOpenIoCompletion( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenIoCompletion failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateIoCompletion handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenIoCompletion handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateJobObject( &ret, GENERIC_ALL, attr );
    ok( status == create_expect, "%u: NtCreateJobObject failed %lx\n", line, status );
    status2 = pNtOpenJobObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenJobObject failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateJobObject handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenJobObject handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, attr );
    ok( status == create_expect, "%u: NtCreateDirectoryObject failed %lx\n", line, status );
    status2 = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenDirectoryObject failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateDirectoryObject handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenDirectoryObject handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, attr, &target );
    ok( status == create_expect, "%u: NtCreateSymbolicLinkObject failed %lx\n", line, status );
    status2 = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, attr );
    ok( status2 == open_expect, "%u: NtOpenSymbolicLinkObject failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateSymbolicLinkObject handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenSymbolicLinkObject handle %p\n", line, ret );
    ret = (HANDLE)0xdeadbeef;
    target.MaximumLength = 0;
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, attr, &target );
    ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER_4 ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateSymbolicLinkObject failed %lx\n", line, status );
    ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateSymbolicLinkObject handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == create_expect, "%u: NtCreateSection failed %lx\n", line, status );
    status2 = pNtOpenSection( &ret2, SECTION_MAP_WRITE, attr );
    ok( status2 == open_expect, "%u: NtOpenSection failed %lx\n", line, status2 );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateSection handle %p\n", line, ret );
    if (!status2) pNtClose( ret2 );
    else ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtOpenSection handle %p\n", line, ret );
    ret = (HANDLE)0xdeadbeef;
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, attr, &size, 0x1234, SEC_COMMIT, 0 );
    ok( status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PAGE_PROTECTION ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateSection failed %lx\n", line, status );
    ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateSection handle %p\n", line, ret );

    ret = ret2 = (HANDLE)0xdeadbeef;
    status = pNtCreateDebugObject( &ret, DEBUG_ALL_ACCESS, attr, 0 );
    ok( status == create_expect, "%u: NtCreateDebugObject failed %lx\n", line, status );
    if (!status) pNtClose( ret );
    else ok( !ret || broken( ret == (HANDLE)0xdeadbeef ) /* vista */,
             "%u: NtCreateDebugObject handle %p\n", line, ret );
    status = pNtCreateDebugObject( &ret2, DEBUG_ALL_ACCESS, attr, 0xdead );
    ok( status == STATUS_INVALID_PARAMETER ||
        (status == STATUS_ACCESS_VIOLATION && create_expect == STATUS_ACCESS_VIOLATION),
        "%u: NtCreateDebugObject failed %lx\n", line, status );
    ok( !ret2 || broken( ret2 == (HANDLE)0xdeadbeef ) /* vista */,
        "%u: NtCreateDebugObject handle %p\n", line, ret );
}

static void test_name_limits(void)
{
    static const WCHAR pipeW[]     = L"\\Device\\NamedPipe\\";
    static const WCHAR mailslotW[] = L"\\Device\\MailSlot\\";
    static const WCHAR registryW[] = L"\\REGISTRY\\Machine\\SOFTWARE\\Microsoft\\";
    OBJECT_ATTRIBUTES attr, attr2, attr3;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER size, timeout;
    UNICODE_STRING str, str2, target;
    NTSTATUS status;
    HANDLE ret, ret2;
    DWORD i;

    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    InitializeObjectAttributes( &attr2, &str, 0, (HANDLE)0xdeadbeef, NULL );
    InitializeObjectAttributes( &attr3, &str, 0, 0, NULL );
    str.Buffer = HeapAlloc( GetProcessHeap(), 0, 65536 + sizeof(registryW));
    str.MaximumLength = 65534;
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i] = 'a';
    size.QuadPart = 4096;
    RtlInitUnicodeString( &target, L"\\DosDevices" );

    attr.RootDirectory = get_base_dir();
    str.Length = 0;
    status = pNtCreateMutant( &ret, GENERIC_ALL, &attr2, FALSE );
    ok( status == STATUS_SUCCESS, "%u: NtCreateMutant failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenMutant( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenMutant failed %lx\n", str.Length, status);
    status = pNtOpenMutant( &ret2, GENERIC_ALL, &attr3 );
    ok( status == (__REACTOS__WinVer_lt(_WIN32_WINNT_WIN7) ? STATUS_INVALID_HANDLE : STATUS_OBJECT_TYPE_MISMATCH), "%u: NtOpenMutant failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateSemaphore( &ret, GENERIC_ALL, &attr2, 1, 2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSemaphore failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSemaphore( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSemaphore failed %lx\n", str.Length, status );
    status = pNtOpenSemaphore( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenSemaphore failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateEvent( &ret, GENERIC_ALL, &attr2, SynchronizationEvent, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateEvent failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenEvent( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenEvent failed %lx\n", str.Length, status );
    status = pNtOpenEvent( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenEvent failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, &attr2, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateKeyedEvent failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenKeyedEvent failed %lx\n", str.Length, status );
    status = pNtOpenKeyedEvent( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenKeyedEvent failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateTimer( &ret, GENERIC_ALL, &attr2, NotificationTimer );
    ok( status == STATUS_SUCCESS, "%u: NtCreateTimer failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenTimer( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenTimer failed %lx\n", str.Length, status );
    status = pNtOpenTimer( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenTimer failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, &attr2, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateCompletion failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenIoCompletion( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenCompletion failed %lx\n", str.Length, status );
    status = pNtOpenIoCompletion( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenCompletion failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateJobObject( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateJobObject failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenJobObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenJobObject failed %lx\n", str.Length, status );
    status = pNtOpenJobObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenJobObject failed %lx\n", str.Length, status );
    pNtClose( ret );
    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateDirectoryObject failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_SUCCESS, "%u: NtOpenDirectoryObject failed %lx\n", str.Length, status );
    if (!status) pNtClose( ret2 );
    status = pNtOpenDirectoryObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_SUCCESS, "%u: NtOpenDirectoryObject failed %lx\n", str.Length, status );
    pNtClose( ret2 );
    pNtClose( ret );
    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, &attr2, &target );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSymbolicLinkObject failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSymbolicLinkObject failed %lx\n", str.Length, status );
    status = pNtOpenSymbolicLinkObject( &ret2, GENERIC_ALL, &attr3 );
    ok( status == STATUS_SUCCESS, "%u: NtOpenSymbolicLinkObject failed %lx\n", str.Length, status );
    pNtClose( ret2 );
    pNtClose( ret );
    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, &attr2, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS, "%u: NtCreateSection failed %lx\n", str.Length, status );
    attr3.RootDirectory = ret;
    status = pNtOpenSection( &ret2, SECTION_MAP_WRITE, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "%u: NtOpenSection failed %lx\n", str.Length, status );
    status = pNtOpenSection( &ret2, SECTION_MAP_WRITE, &attr3 );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH || /* __REACTOS__ */ broken(status == STATUS_INVALID_HANDLE), "%u: NtOpenSection failed %lx\n", str.Length, status );
    pNtClose( ret );

    str.Length = 67;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );

    str.Length = 65532;
    test_all_kernel_objects( __LINE__, &attr, STATUS_SUCCESS, STATUS_SUCCESS );

    str.Length = 65534;
    test_all_kernel_objects( __LINE__, &attr, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );

    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        if (attr.Length == sizeof(attr))
            test_all_kernel_objects( __LINE__, &attr, STATUS_SUCCESS, STATUS_SUCCESS );
        else
            test_all_kernel_objects( __LINE__, &attr, STATUS_INVALID_PARAMETER, STATUS_INVALID_PARAMETER );
    }
    attr.Length = sizeof(attr);

    /* null attributes or ObjectName, with or without RootDirectory */
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_NAME_INVALID );
    test_all_kernel_objects( __LINE__, &attr3, STATUS_SUCCESS, STATUS_OBJECT_PATH_SYNTAX_BAD );
    attr2.ObjectName = attr3.ObjectName = (void *)0xdeadbeef;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_ACCESS_VIOLATION, STATUS_ACCESS_VIOLATION );
    test_all_kernel_objects( __LINE__, &attr3, STATUS_ACCESS_VIOLATION, STATUS_ACCESS_VIOLATION );
    attr2.ObjectName = attr3.ObjectName = &str2;
    str2.Buffer = (WCHAR *)((char *)pipeW + 1); /* misaligned buffer */
    str2.Length = 3;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_DATATYPE_MISALIGNMENT, STATUS_DATATYPE_MISALIGNMENT );
    test_all_kernel_objects( __LINE__, &attr3, STATUS_DATATYPE_MISALIGNMENT, STATUS_DATATYPE_MISALIGNMENT );
    str2.Buffer = (WCHAR *)0xdeadbee0;
    str2.Length = 2;
    test_all_kernel_objects( __LINE__, &attr2, STATUS_ACCESS_VIOLATION, STATUS_ACCESS_VIOLATION );
    test_all_kernel_objects( __LINE__, &attr3, STATUS_ACCESS_VIOLATION, STATUS_ACCESS_VIOLATION );

    attr3.ObjectName = &str2;
    pRtlInitUnicodeString( &str2, L"\\BaseNamedObjects\\Local" );
    status = pNtOpenSymbolicLinkObject( &ret, SYMBOLIC_LINK_QUERY, &attr3 );
    ok( status == STATUS_SUCCESS, "can't open BaseNamedObjects\\Local %lx\n", status );
    attr3.ObjectName = &str;
    attr3.RootDirectory = ret;
    test_all_kernel_objects( __LINE__, &attr3, STATUS_OBJECT_TYPE_MISMATCH, STATUS_OBJECT_TYPE_MISMATCH );
    pNtClose( attr3.RootDirectory );

    status = pNtCreateMutant( &ret, GENERIC_ALL, NULL, FALSE );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateMutant failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateMutant( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, FALSE );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateMutant failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenMutant( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenMutant failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenMutant( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenMutant failed %lx\n", status );

    status = pNtCreateSemaphore( &ret, GENERIC_ALL, NULL, 1, 2 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateSemaphore failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateSemaphore( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, 1, 2 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateSemaphore failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenSemaphore( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSemaphore failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenSemaphore( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenSemaphore failed %lx\n", status );

    status = pNtCreateEvent( &ret, GENERIC_ALL, NULL, SynchronizationEvent, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateEvent failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateEvent( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, SynchronizationEvent, 0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateEvent failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenEvent( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenEvent failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenEvent( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenEvent failed %lx\n", status );

    status = pNtCreateKeyedEvent( &ret, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateKeyedEvent failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateKeyedEvent( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKeyedEvent failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenKeyedEvent( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenKeyedEvent failed %lx\n", status );
    ok( !ret, "handle set %p\n", ret );
    status = pNtOpenKeyedEvent( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKeyedEvent failed %lx\n", status );

    status = pNtCreateTimer( &ret, GENERIC_ALL, NULL, NotificationTimer );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateTimer failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateTimer( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, NotificationTimer );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateTimer failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenTimer( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenTimer failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenTimer( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenTimer failed %lx\n", status );

    status = pNtCreateIoCompletion( &ret, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateCompletion failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateIoCompletion( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, 0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateCompletion failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenIoCompletion( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenCompletion failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenIoCompletion( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenCompletion failed %lx\n", status );

    status = pNtCreateJobObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateJobObject failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateJobObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateJobObject failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenJobObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenJobObject failed %lx\n", status );
    ok( !ret || broken(ret == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", ret );
    status = pNtOpenJobObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenJobObject failed %lx\n", status );

    status = pNtCreateDirectoryObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateDirectoryObject failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateDirectoryObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateDirectoryObject failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenDirectoryObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenDirectoryObject failed %lx\n", status );
    ok( !ret, "handle set %p\n", ret );
    status = pNtOpenDirectoryObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenDirectoryObject failed %lx\n", status );

    status = pNtCreateSymbolicLinkObject( &ret, GENERIC_ALL, NULL, &target );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateSymbolicLinkObject failed %lx\n", status );
    if (!status) pNtClose( ret );
    status = pNtCreateSymbolicLinkObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL, &target );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateSymbolicLinkObject failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenSymbolicLinkObject( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSymbolicLinkObject failed %lx\n", status );
    ok( !ret, "handle set %p\n", ret );
    status = pNtOpenSymbolicLinkObject( (HANDLE *)0xdeadbee0, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenSymbolicLinkObject failed %lx\n", status );

    status = pNtCreateSection( &ret, SECTION_MAP_WRITE, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS, "NULL: NtCreateSection failed %lx\n", status );
    pNtClose( ret );
    status = pNtCreateSection( (HANDLE *)0xdeadbee0, SECTION_MAP_WRITE, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateSection failed %lx\n", status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenSection( &ret, SECTION_MAP_WRITE, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtOpenSection failed %lx\n", status );
    ok( !ret, "handle set %p\n", ret );
    status = pNtOpenSection( (HANDLE *)0xdeadbee0, SECTION_MAP_WRITE, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenSection failed %lx\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* named pipes */
    wcscpy( str.Buffer, pipeW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen( pipeW )] = 'a';
    str.Length = 0;
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    timeout.QuadPart = -10000;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    str.Length = 67;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                      FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
        if (attr.Length == sizeof(attr))
        {
            ok( status == STATUS_SUCCESS, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
            pNtClose( ret );
        }
        else ok( status == STATUS_INVALID_PARAMETER,
                 "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    }
    attr.Length = sizeof(attr);
    str.Length = 65532;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_SUCCESS, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    pNtClose( ret );
    str.Length = 65534;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateNamedPipeFile failed %lx\n", str.Length, status );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr2, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NULL: NtCreateNamedPipeFile failed %lx\n", status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, &attr3, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NULL: NtCreateNamedPipeFile failed %lx\n", status );
    status = pNtCreateNamedPipeFile( &ret, GENERIC_ALL, NULL, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                     FILE_CREATE, FILE_PIPE_FULL_DUPLEX, 0, 0, 0, 1, 256, 256, &timeout );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtCreateNamedPipeFile failed %lx\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* mailslots */
    wcscpy( str.Buffer, mailslotW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen( mailslotW )] = 'a';
    str.Length = 0;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    str.Length = 67;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    str.Length = 128;
    for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
    {
        status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
        if (attr.Length == sizeof(attr))
        {
            ok( status == STATUS_SUCCESS, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
            pNtClose( ret );
        }
        else ok( status == STATUS_INVALID_PARAMETER,
                 "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    }
    attr.Length = sizeof(attr);
    str.Length = 65532;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    pNtClose( ret );
    str.Length = 65534;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "%u: NtCreateMailslotFile failed %lx\n", str.Length, status );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr2, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NULL: NtCreateMailslotFile failed %lx\n", status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, &attr3, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NULL: NtCreateMailslotFile failed %lx\n", status );
    status = pNtCreateMailslotFile( &ret, GENERIC_ALL, NULL, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "NULL: NtCreateMailslotFile failed %lx\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    /* registry keys */
    wcscpy( str.Buffer, registryW );
    for (i = 0; i < 65536 / sizeof(WCHAR); i++) str.Buffer[i + wcslen(registryW)] = 'a';
    str.Length = 0;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "%u: NtCreateKey failed %lx\n", str.Length, status );
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr2, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtCreateKey failed %lx\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_INVALID_HANDLE, "%u: NtOpenKey failed %lx\n", str.Length, status );
    str.Length = (wcslen( registryW ) + 250) * sizeof(WCHAR) + 1;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_SUCCESS ), /* wow64 */
        "%u: NtCreateKey failed %lx\n", str.Length, status );
    if (!status)
    {
        pNtDeleteKey( ret );
        pNtClose( ret );
    }
    str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR);
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED,
        "%u: NtCreateKey failed %lx\n", str.Length, status );
    status = pNtCreateKey( (HANDLE *)0xdeadbee0, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "%u: NtCreateKey failed %lx\n", str.Length, status );
    if (!status)
    {
        status = pNtOpenKey( &ret2, KEY_READ, &attr );
        ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %lx\n", str.Length, status );
        pNtClose( ret2 );
        status = pNtOpenKey( (HANDLE *)0xdeadbee0, KEY_READ, &attr );
        ok( status == STATUS_ACCESS_VIOLATION, "%u: NtOpenKey failed %lx\n", str.Length, status );
        attr3.RootDirectory = ret;
        str.Length = 0;
        status = pNtOpenKey( &ret2, KEY_READ, &attr3 );
        ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %lx\n", str.Length, status );
        pNtClose( ret2 );
        pNtDeleteKey( ret );
        pNtClose( ret );

        str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR);
        for (attr.Length = 0; attr.Length <= 2 * sizeof(attr); attr.Length++)
        {
            if (attr.Length == sizeof(attr))
            {
                status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
                ok( status == STATUS_SUCCESS, "%u: NtCreateKey failed %lx\n", str.Length, status );
                status = pNtOpenKey( &ret2, KEY_READ, &attr );
                ok( status == STATUS_SUCCESS, "%u: NtOpenKey failed %lx\n", str.Length, status );
                pNtClose( ret2 );
                pNtDeleteKey( ret );
                pNtClose( ret );
            }
            else
            {
                status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
                ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %lx\n", str.Length, status );
                status = pNtOpenKey( &ret2, KEY_READ, &attr );
                ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %lx\n", str.Length, status );
            }
        }
        attr.Length = sizeof(attr);
    }
    str.Length = (wcslen( registryW ) + 256) * sizeof(WCHAR) + 1;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_SUCCESS ), /* win7 */
        "%u: NtCreateKey failed %lx\n", str.Length, status );
    if (!status)
    {
        pNtDeleteKey( ret );
        pNtClose( ret );
    }
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_INVALID_PARAMETER ||
        broken( status == STATUS_OBJECT_NAME_NOT_FOUND ), /* wow64 */
        "%u: NtOpenKey failed %lx\n", str.Length, status );
    str.Length++;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %lx\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %lx\n", str.Length, status );
    str.Length = 2000;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtCreateKey failed %lx\n", str.Length, status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_INVALID_PARAMETER, "%u: NtOpenKey failed %lx\n", str.Length, status );
    /* some Windows versions change the error past 2050 chars, others past 4066 chars, some don't */
    str.Length = 5000;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL ||
        status == STATUS_INVALID_PARAMETER,
        "%u: NtCreateKey failed %lx\n", str.Length, status );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL ||
        status == STATUS_INVALID_PARAMETER,
         "%u: NtOpenKey failed %lx\n", str.Length, status );
    ok( !ret, "handle set %p\n", ret );
    str.Length = 65534;
    ret = (HANDLE)0xdeadbeef;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr, 0, NULL, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL,
        "%u: NtCreateKey failed %lx\n", str.Length, status );
    ok( !ret, "handle set %p\n", ret );
    ret = (HANDLE)0xdeadbeef;
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID ||
        status == STATUS_BUFFER_OVERFLOW ||
        status == STATUS_BUFFER_TOO_SMALL,
         "%u: NtOpenKey failed %lx\n", str.Length, status );
    ok( !ret, "handle set %p\n", ret );
    attr3.RootDirectory = 0;
    attr2.ObjectName = attr3.ObjectName = NULL;
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr2, 0, NULL, 0, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKey failed %lx\n", status );
    status = pNtCreateKey( &ret, GENERIC_ALL, &attr3, 0, NULL, 0, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKey failed %lx\n", status );
    status = pNtCreateKey( &ret, GENERIC_ALL, NULL, 0, NULL, 0, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtCreateKey failed %lx\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr2 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKey failed %lx\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, &attr3 );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKey failed %lx\n", status );
    status = pNtOpenKey( &ret, GENERIC_ALL, NULL );
    ok( status == STATUS_ACCESS_VIOLATION, "NULL: NtOpenKey failed %lx\n", status );
    attr2.ObjectName = attr3.ObjectName = &str;

    HeapFree( GetProcessHeap(), 0, str.Buffer );
}

static void test_directory(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, dir1, h, h2;
    WCHAR buffer[256];
    ULONG len, full_len;

    /* No name and/or no attributes */
    status = pNtCreateDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* wow64 */, "got %#lx\n", status);
    status = pNtOpenDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER /* wow64 */, "got %#lx\n", status);

    status = pNtCreateDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_SUCCESS, "Failed to create Directory without attributes(%08lx)\n", status);
    pNtClose(h);
    status = pNtOpenDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenDirectoryObject should have failed with STATUS_INVALID_PARAMETER got(%08lx)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08lx\n", status );

    /* Bad name */
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    RtlInitUnicodeString(&str, L"");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    pNtClose(h);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08lx\n", status );
    pNtClose(dir);

    check_create_open_dir( NULL, L"BaseNamedObjects", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\\\BaseNamedObjects", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\\\om.c-test", STATUS_OBJECT_NAME_INVALID );
    check_create_open_dir( NULL, L"\\BaseNamedObjects\\om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND );

    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    status = pNtOpenDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08lx\n", status );
    pNtClose(h);
    pNtClose(dir1);


    /* Use of root directory */

    /* Can't use symlinks as a directory */
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Local");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&dir, SYMBOLIC_LINK_QUERY, &attr);

    ok(status == STATUS_SUCCESS, "Failed to open SymbolicLink(%08lx)\n", status);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtCreateDirectoryObject got %08lx\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\Local\\om.c-test" );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = pNtCreateDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    RtlInitUnicodeString( &str, L"om.c-test" );
    InitializeObjectAttributes( &attr, &str, 0, dir, NULL );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "Failed to open directory %08lx\n", status );

    RtlInitUnicodeString( &str, L"om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, dir1, NULL );
    status = pNtCreateEvent( &h, GENERIC_ALL, &attr, SynchronizationEvent, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateEvent failed %lx\n", status );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_SUCCESS, "NtOpenEvent failed %lx\n", status );
    pNtClose( h2 );
    RtlInitUnicodeString( &str, L"om.c-test\\om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, dir, NULL );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtOpenEvent failed %lx\n", status );
    RtlInitUnicodeString( &str, L"\\BasedNamedObjects\\Local\\om.c-test\\om.c-event" );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = pNtOpenEvent( &h2, GENERIC_ALL, &attr );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "NtOpenEvent failed %lx\n", status );
    pNtClose( h );
    pNtClose( dir1 );

    str.Buffer = buffer;
    str.MaximumLength = sizeof(buffer);
    len = 0xdeadbeef;
    memset( buffer, 0xaa, sizeof(buffer) );
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08lx\n", status );
    full_len = str.Length + sizeof(WCHAR);
    ok( len == full_len, "bad length %lu/%lu\n", len, full_len );
    ok( buffer[len / sizeof(WCHAR) - 1] == 0, "no terminating null\n" );

    str.MaximumLength = str.Length;
    str.Length = 0x4444;
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08lx\n", status );
    ok( len == full_len, "bad length %lu/%lu\n", len, full_len );
    ok( str.Length == 0x4444, "len set to %x\n", str.Length );

    str.MaximumLength = 0;
    str.Length = 0x4444;
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08lx\n", status );
    ok( len == full_len, "bad length %lu/%lu\n", len, full_len );
    ok( str.Length == 0x4444, "len set to %x\n", str.Length );

    str.MaximumLength = full_len;
    str.Length = 0x4444;
    len = 0xdeadbeef;
    status = pNtQuerySymbolicLinkObject( dir, &str, &len );
    ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08lx\n", status );
    ok( len == full_len, "bad length %lu/%lu\n", len, full_len );
    ok( str.Length == full_len - sizeof(WCHAR), "len set to %x\n", str.Length );

    pNtClose(dir);

    RtlInitUnicodeString(&str, L"\\BaseNamedObjects");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08lx\n", status );

    InitializeObjectAttributes(&attr, NULL, 0, dir, NULL);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenDirectoryObject got %08lx\n", status );

    check_create_open_dir( dir, L"", STATUS_SUCCESS );
    check_create_open_dir( dir, L"\\", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"\\om.c-test", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"\\om.c-test\\", STATUS_OBJECT_PATH_SYNTAX_BAD );
    check_create_open_dir( dir, L"om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND );

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"om.c-test");
    status = pNtCreateDirectoryObject( &dir1, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08lx\n", status );

    pNtClose(h);
    pNtClose(dir1);
    pNtClose(dir);

    /* Nested directories */
    RtlInitUnicodeString(&str, L"\\");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08lx\n", status );
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    status = pNtOpenDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenDirectoryObject got %08lx\n", status );
    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test");
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test\\one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    pNtClose(h);

    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Global\\om.c-test");
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\Local\\om.c-test\\one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"one more level");
    status = pNtCreateDirectoryObject( &h, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to create directory %08lx\n", status );
    pNtClose(h);
    pNtClose(dir);

    /* Create other objects using RootDirectory */

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects");
    status = pNtOpenDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( status == STATUS_SUCCESS, "Failed to open directory %08lx\n", status );
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);

    /* Test invalid paths */
    RtlInitUnicodeString(&str, L"\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);
    RtlInitUnicodeString(&str, L"\\om.c-mutant\\");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);

    RtlInitUnicodeString(&str, L"om.c\\-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08lx)\n", status);

    RtlInitUnicodeString(&str, L"om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08lx)\n", status);
    pNtClose(h);

    pNtClose(dir);
}

static void test_symboliclink(void)
{
    NTSTATUS status;
    UNICODE_STRING str, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, link, h, h2;
    IO_STATUS_BLOCK iosb;

    /* No name and/or no attributes */
    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    RtlInitUnicodeString(&target, L"\\DosDevices");
    status = pNtCreateSymbolicLinkObject( NULL, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok(status == STATUS_ACCESS_VIOLATION, "got %#lx\n", status);
    status = pNtOpenSymbolicLinkObject( NULL, SYMBOLIC_LINK_QUERY, &attr );
    ok(status == STATUS_ACCESS_VIOLATION, "got %#lx\n", status);

    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION,
        "NtCreateSymbolicLinkObject should have failed with STATUS_ACCESS_VIOLATION got(%08lx)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenSymbolicLinkObject should have failed with STATUS_INVALID_PARAMETER got(%08lx)\n", status);

    /* No attributes */
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, &target);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_VIOLATION, /* nt4 */
       "NtCreateSymbolicLinkObject failed(%08lx)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    memset(&target, 0, sizeof(target));
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);

    /* Bad name */
    RtlInitUnicodeString(&target, L"anywhere");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    RtlInitUnicodeString(&str, L"");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08lx)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08lx)\n", status);
    pNtClose(link);

    RtlInitUnicodeString(&str, L"\\");
    attr.Attributes = OBJ_OPENIF;
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
       "NtCreateSymbolicLinkObject should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);
    attr.Attributes = 0;
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr, &target);
    todo_wine
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
       "NtCreateSymbolicLinkObject should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08lx)\n", status);

    RtlInitUnicodeString( &target, L"->Somewhere");

    RtlInitUnicodeString( &str, L"BaseNamedObjects" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtCreateSymbolicLinkObject got %08lx\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_SYNTAX_BAD, "NtOpenSymbolicLinkObject got %08lx\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08lx\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08lx\n", status );

    RtlInitUnicodeString( &str, L"\\\\BaseNamedObjects" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08lx\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08lx\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\\\om.c-test" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtCreateSymbolicLinkObject got %08lx\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_NAME_INVALID, "NtOpenSymbolicLinkObject got %08lx\n", status );

    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\om.c-test\\" );
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#lx\n", status );
    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok( status == STATUS_OBJECT_PATH_NOT_FOUND, "got %#lx\n", status );

    /* Compound test */
    dir = get_base_dir();
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString(&str, L"test-link");
    RtlInitUnicodeString(&target, L"\\DosDevices");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08lx)\n", status);

    RtlInitUnicodeString(&str, L"test-link\\NUL");
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS, "Failed to open NUL device(%08lx)\n", status);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE);
    ok(status == STATUS_SUCCESS, "Failed to open NUL device(%08lx)\n", status);

    pNtClose(h);
    pNtClose(link);
    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString(&str, L"\\BaseNamedObjects\\om.c-test");
    status = pNtCreateDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    RtlInitUnicodeString(&str, L"\\DosDevices\\test_link");
    RtlInitUnicodeString(&target, L"\\BaseNamedObjects");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS && !!link, "Got unexpected status %#lx.\n", status);

    status = NtCreateFile(&h, GENERIC_READ | SYNCHRONIZE, &attr, &iosb, NULL, 0,
            FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    pNtClose(h);

    attr.Attributes = OBJ_OPENIF;
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok(status == STATUS_SUCCESS || broken( status == STATUS_OBJECT_NAME_EXISTS ), /* <= win10 1507 */
       "Got unexpected status %#lx.\n", status);
    pNtClose(h);
    attr.Attributes = 0;
    status = pNtCreateSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr, &target );
    ok(status == STATUS_OBJECT_NAME_COLLISION, "Got unexpected status %#lx.\n", status);
    pNtClose(h);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\om.c-test\\" );
    status = NtCreateFile(&h, GENERIC_READ | SYNCHRONIZE, &attr, &iosb, NULL, 0,
            FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    ok(status == STATUS_OBJECT_NAME_INVALID, "Got unexpected status %#lx.\n", status);

    InitializeObjectAttributes(&attr, &str, 0, link, NULL);
    RtlInitUnicodeString( &str, L"om.c-test\\test_object" );
    status = pNtCreateMutant( &h, GENERIC_ALL, &attr, FALSE );
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    RtlInitUnicodeString( &str, L"\\DosDevices\\test_link\\om.c-test\\test_object" );
    status = pNtCreateMutant( &h, GENERIC_ALL, &attr, FALSE );
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = pNtOpenMutant( &h2, GENERIC_ALL, &attr );
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    pNtClose(h2);
    RtlInitUnicodeString( &str, L"\\BaseNamedObjects\\om.c-test\\test_object" );
    status = pNtCreateMutant( &h2, GENERIC_ALL, &attr, FALSE );
    ok(status == STATUS_OBJECT_NAME_COLLISION, "Got unexpected status %#lx.\n", status);

    InitializeObjectAttributes(&attr, &str, 0, link, NULL);
    RtlInitUnicodeString( &str, L"om.c-test\\test_object" );
    status = pNtOpenMutant( &h2, GENERIC_ALL, &attr );
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    pNtClose(h);

    status = pNtOpenMutant( &h, GENERIC_ALL, &attr );
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Got unexpected status %#lx.\n", status);

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    RtlInitUnicodeString( &str, L"test_object" );
    status = pNtCreateMutant( &h, GENERIC_ALL, &attr, FALSE );
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = pNtOpenMutant( &h2, GENERIC_ALL, &attr );
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    pNtClose(h);
    pNtClose(h2);

    pNtClose(link);
    pNtClose(dir);
}

#define test_file_info(a) _test_file_info(__LINE__,a)
static void _test_file_info(unsigned line, HANDLE handle)
{
    IO_STATUS_BLOCK io;
    char buf[256];
    NTSTATUS status;

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), 0xdeadbeef);
    ok_(__FILE__,line)(status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* Vista+ */,
                       "expected STATUS_NOT_IMPLEMENTED, got %lx\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), FileAccessInformation);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "FileAccessInformation returned %lx\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf),
                                    FileIoCompletionNotificationInformation);
    ok_(__FILE__,line)(status == STATUS_SUCCESS,
                       "FileIoCompletionNotificationInformation returned %lx\n", status);
}

#define test_no_file_info(a) _test_no_file_info(__LINE__,a)
static void _test_no_file_info(unsigned line, HANDLE handle)
{
    IO_STATUS_BLOCK io;
    char buf[256];
    NTSTATUS status;

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), 0xdeadbeef);
    ok_(__FILE__,line)(status == STATUS_INVALID_INFO_CLASS || status == STATUS_NOT_IMPLEMENTED /* Vista+ */,
                       "expected STATUS_NOT_IMPLEMENTED, got %lx\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf), FileAccessInformation);
    ok_(__FILE__,line)(status == STATUS_OBJECT_TYPE_MISMATCH,
                       "FileAccessInformation returned %lx\n", status);

    status = pNtQueryInformationFile(handle, &io, buf, sizeof(buf),
                                    FileIoCompletionNotificationInformation);
    ok_(__FILE__,line)(status == STATUS_OBJECT_TYPE_MISMATCH,
                       "FileIoCompletionNotificationInformation returned %lx\n", status);
}

static OBJECT_TYPE_INFORMATION all_types[256];

static void add_object_type( OBJECT_TYPE_INFORMATION *info )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(all_types); i++)
    {
        if (!all_types[i].TypeName.Buffer) break;
        if (!RtlCompareUnicodeString( &all_types[i].TypeName, &info->TypeName, FALSE )) break;
    }
    ok( i < ARRAY_SIZE(all_types), "too many types\n" );

    if (all_types[i].TypeName.Buffer)  /* existing type */
    {
        ok( !memcmp( &all_types[i].GenericMapping, &info->GenericMapping, sizeof(GENERIC_MAPPING) ),
            "%u: mismatched mappings %08lx,%08lx,%08lx,%08lx / %08lx,%08lx,%08lx,%08lx\n", i,
            all_types[i].GenericMapping.GenericRead, all_types[i].GenericMapping.GenericWrite,
            all_types[i].GenericMapping.GenericExecute, all_types[i].GenericMapping.GenericAll,
            info->GenericMapping.GenericRead, info->GenericMapping.GenericWrite,
            info->GenericMapping.GenericExecute, info->GenericMapping.GenericAll );
        ok( all_types[i].ValidAccessMask == info->ValidAccessMask,
            "%u: mismatched access mask %08lx / %08lx\n", i,
            all_types[i].ValidAccessMask, info->ValidAccessMask );
    }
    else  /* add it */
    {
        all_types[i] = *info;
        RtlDuplicateUnicodeString( 1, &info->TypeName, &all_types[i].TypeName );
    }
    ok( info->TotalNumberOfObjects <= info->HighWaterNumberOfObjects, "%s: wrong object counts %lu/%lu\n",
        debugstr_w( all_types[i].TypeName.Buffer ),
        info->TotalNumberOfObjects, info->HighWaterNumberOfObjects );
    ok( info->TotalNumberOfHandles <= info->HighWaterNumberOfHandles, "%s: wrong handle counts %lu/%lu\n",
        debugstr_w( all_types[i].TypeName.Buffer ),
        info->TotalNumberOfHandles, info->HighWaterNumberOfHandles );
}

static BOOL compare_unicode_string( const UNICODE_STRING *string, const WCHAR *expect )
{
    return string->Length == wcslen( expect ) * sizeof(WCHAR)
            && !wcsnicmp( string->Buffer, expect, string->Length / sizeof(WCHAR) );
}

#define test_object_type(a,b) _test_object_type(__LINE__,a,b)
static void _test_object_type( unsigned line, HANDLE handle, const WCHAR *expected_name )
{
    char buffer[1024];
    OBJECT_TYPE_INFORMATION *type = (OBJECT_TYPE_INFORMATION *)buffer;
    UNICODE_STRING expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__,line)( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok_(__FILE__,line)( len > sizeof(UNICODE_STRING), "unexpected len %lu\n", len );
    ok_(__FILE__,line)( len >= sizeof(*type) + type->TypeName.Length, "unexpected len %lu\n", len );
    ok_(__FILE__,line)(compare_unicode_string( &type->TypeName, expected_name ), "wrong name %s\n",
                       debugstr_w( type->TypeName.Buffer ));
    add_object_type( type );
}

#define test_object_name(a,b) _test_object_name(__LINE__,a,b)
static void _test_object_name( unsigned line, HANDLE handle, const WCHAR *expected_name )
{
    char buffer[1024];
    UNICODE_STRING *str = (UNICODE_STRING *)buffer, expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__,line)( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok_(__FILE__,line)( len >= sizeof(OBJECT_NAME_INFORMATION) + str->Length, "unexpected len %lu\n", len );
    ok_(__FILE__,line)( compare_unicode_string( str, expected_name ), "got %s, expected %s\n",
                        debugstr_w(str->Buffer), debugstr_w(expected_name) );
}

static void test_query_object(void)
{
    static const WCHAR name[] = L"\\BaseNamedObjects\\test_event";
    HANDLE handle, client;
    char buffer[1024];
    NTSTATUS status;
    ULONG len, expected_len;
    OBJECT_BASIC_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING path, target, *str;
    char dir[MAX_PATH], tmp_path[MAX_PATH], file1[MAX_PATH + 16];
    WCHAR expect[100];
    LARGE_INTEGER size;

    InitializeObjectAttributes( &attr, &path, 0, 0, 0 );

    handle = CreateEventA( NULL, FALSE, FALSE, "test_event" );

    status = pNtQueryObject( handle, ObjectBasicInformation, NULL, 0, NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );

    status = pNtQueryObject( handle, ObjectBasicInformation, &info, 0, NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );

    status = pNtQueryObject( handle, ObjectBasicInformation, NULL, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );

    len = 0;
    status = pNtQueryObject( handle, ObjectBasicInformation, &info, sizeof(OBJECT_BASIC_INFORMATION), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(OBJECT_BASIC_INFORMATION), "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(OBJECT_TYPE_INFORMATION), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %lu\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR) == len, "unexpected len %lu\n", len );
    ok( str->Length >= sizeof(name) - sizeof(WCHAR), "unexpected len %u\n", str->Length );
    ok( len > sizeof(UNICODE_STRING) + sizeof("\\test_event") * sizeof(WCHAR),
        "name too short %s\n", wine_dbgstr_w(str->Buffer) );
    /* check for \\Sessions prefix in the name */
    swprintf( expect, ARRAY_SIZE(expect), L"\\Sessions\\%u%s", NtCurrentTeb()->Peb->SessionId, name );
    ok( (str->Length == wcslen( expect ) * sizeof(WCHAR) && !wcscmp( str->Buffer, expect )),
        "wrong name %s\n", wine_dbgstr_w(str->Buffer) );
    trace( "got %s len %lu\n", wine_dbgstr_w(str->Buffer), len );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, len, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name), "unexpected len %lu\n", len );

    test_object_type( handle, L"Event" );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, len, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(L"Event"), "unexpected len %lu\n", len );

    test_no_file_info( handle );
    pNtClose( handle );

    handle = CreateEventA( NULL, FALSE, FALSE, NULL );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok( len == sizeof(UNICODE_STRING), "unexpected len %lu\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( str->Length == 0, "unexpected len %lu\n", len );
    ok( str->Buffer == NULL, "unexpected ptr %p\n", str->Buffer );
    test_no_file_info( handle );
    pNtClose( handle );

    GetWindowsDirectoryA( dir, MAX_PATH );
    handle = CreateFileA( dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS, 0 );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %lu\n", len );
    str = (UNICODE_STRING *)buffer;
    expected_len = sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR);
    ok( len == expected_len, "unexpected len %lu\n", len );
    trace( "got %s len %lu\n", wine_dbgstr_w(str->Buffer), len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status );
    ok( len == expected_len, "unexpected len %lu\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", status);
    ok( len == expected_len, "unexpected len %lu\n", len );

    test_object_type( handle, L"File" );

    pNtClose( handle );

    GetTempPathA(MAX_PATH, tmp_path);
    GetTempFileNameA(tmp_path, "foo", 0, file1);
    handle = CreateFileA(file1, GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, 0);
    test_object_type(handle, L"File");
    test_file_info( handle );
    pNtClose( handle );
    DeleteFileA( file1 );

    status = pNtCreateIoCompletion( &handle, IO_COMPLETION_ALL_ACCESS, NULL, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateIoCompletion failed %lx\n", status);

    test_object_type( handle, L"IoCompletion" );
    test_no_file_info( handle );

    pNtClose( handle );

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_debug" );
    status = pNtCreateDebugObject( &handle, DEBUG_ALL_ACCESS, &attr, 0 );
    ok(!status, "NtCreateDebugObject failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_debug" );
    test_object_type( handle, L"DebugObject" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_mutant" );
    status = pNtCreateMutant( &handle, MUTANT_ALL_ACCESS, &attr, 0 );
    ok(!status, "NtCreateMutant failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_mutant" );
    test_object_type( handle, L"Mutant" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_sem" );
    status = pNtCreateSemaphore( &handle, SEMAPHORE_ALL_ACCESS, &attr, 1, 2 );
    ok(!status, "NtCreateSemaphore failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_sem" );
    test_object_type( handle, L"Semaphore" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_keyed" );
    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS, &attr, 0 );
    ok(!status, "NtCreateKeyedEvent failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_keyed" );
    test_object_type( handle, L"KeyedEvent" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_compl" );
    status = pNtCreateIoCompletion( &handle, IO_COMPLETION_ALL_ACCESS, &attr, 0 );
    ok(!status, "NtCreateIoCompletion failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_compl" );
    test_object_type( handle, L"IoCompletion" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_job" );
    status = pNtCreateJobObject( &handle, JOB_OBJECT_ALL_ACCESS, &attr );
    ok(!status, "NtCreateJobObject failed: %lx\n", status);
    test_object_name( handle, L"\\BaseNamedObjects\\test_job" );
    test_object_type( handle, L"Job" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\BaseNamedObjects\\test_timer" );
    status = pNtCreateTimer( &handle, TIMER_ALL_ACCESS, &attr, NotificationTimer );
    ok(!status, "NtCreateTimer failed: %lx\n", status);
    test_object_type( handle, L"Timer" );
    test_no_file_info( handle );
    pNtClose(handle);

    RtlInitUnicodeString( &path, L"\\DosDevices\\test_link" );
    RtlInitUnicodeString( &target, L"\\DosDevices" );
    status = pNtCreateSymbolicLinkObject( &handle, SYMBOLIC_LINK_ALL_ACCESS, &attr, &target );
    ok(!status, "NtCreateSymbolicLinkObject failed: %lx\n", status);
    test_object_type( handle, L"SymbolicLink" );
    test_no_file_info( handle );
    pNtClose(handle);

    handle = GetProcessWindowStation();
    swprintf( expect, ARRAY_SIZE(expect), L"\\Sessions\\%u\\Windows\\WindowStations\\WinSta0", NtCurrentTeb()->Peb->SessionId );
    test_object_name( handle, expect );
    test_object_type( handle, L"WindowStation" );
    test_no_file_info( handle );

    handle = GetThreadDesktop( GetCurrentThreadId() );
    test_object_name( handle, L"\\Default" );
    test_object_type( handle, L"Desktop" );
    test_no_file_info( handle );

    status = pNtCreateDirectoryObject( &handle, DIRECTORY_QUERY, NULL );
    ok(status == STATUS_SUCCESS, "Failed to create Directory %08lx\n", status);

    test_object_type( handle, L"Directory" );
    test_no_file_info( handle );

    pNtClose( handle );

    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection( &handle, SECTION_MAP_WRITE, NULL, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_SUCCESS , "NtCreateSection returned %lx\n", status );

    test_object_type( handle, L"Section" );
    test_no_file_info( handle );

    pNtClose( handle );

    handle = CreateMailslotA( "\\\\.\\mailslot\\test_mailslot", 100, 1000, NULL );
    ok( handle != INVALID_HANDLE_VALUE, "CreateMailslot failed err %lu\n", GetLastError() );

    test_object_name( handle, L"\\Device\\Mailslot\\test_mailslot" );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    client = CreateFileA( "\\\\.\\mailslot\\test_mailslot", 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    len = 0;
    status = pNtQueryObject( client, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    str = (UNICODE_STRING *)buffer;
    ok( len == sizeof(UNICODE_STRING) + str->MaximumLength, "unexpected len %lu\n", len );
    todo_wine
        ok( compare_unicode_string( str, L"\\Device\\Mailslot" ) ||
            compare_unicode_string( str, L"\\Device\\Mailslot\\test_mailslot" ) /* win8+ */,
            "wrong name %s\n", debugstr_w( str->Buffer ));

    test_object_type( client, L"File" );
    test_file_info( client );

    pNtClose( client );
    pNtClose( handle );

    handle = CreateFileA( "\\\\.\\mailslot", 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    test_object_name( handle, L"\\Device\\Mailslot" );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    pNtClose( handle );

    handle = CreateNamedPipeA( "\\\\.\\pipe\\test_pipe", PIPE_ACCESS_DUPLEX, PIPE_READMODE_BYTE,
                               1, 1000, 1000, 1000, NULL );
    ok( handle != INVALID_HANDLE_VALUE, "CreateNamedPipe failed err %lu\n", GetLastError() );

    test_object_name( handle, L"\\Device\\NamedPipe\\test_pipe" );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    client = CreateFileA( "\\\\.\\pipe\\test_pipe", GENERIC_READ | GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING, 0, 0 );
    ok( client != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    test_object_type( client, L"File" );
    test_file_info( client );

    pNtClose( client );
    pNtClose( handle );

    handle = CreateFileA( "\\\\.\\pipe", 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    test_object_name( handle, L"\\Device\\NamedPipe" );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    pNtClose( handle );

    handle = CreateFileA( "\\\\.\\pipe\\", 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%lu)\n", GetLastError() );

    test_object_name( handle, L"\\Device\\NamedPipe\\" );
    test_object_type( handle, L"File" );
    test_file_info( handle );

    pNtClose( handle );

    RtlInitUnicodeString( &path, L"\\REGISTRY\\Machine" );
    status = pNtCreateKey( &handle, KEY_READ, &attr, 0, 0, 0, 0 );
    ok( status == STATUS_SUCCESS, "NtCreateKey failed status %lx\n", status );

    test_object_name( handle, L"\\REGISTRY\\MACHINE" );
    test_object_type( handle, L"Key" );

    pNtClose( handle );

    test_object_name( GetCurrentProcess(), L"" );
    test_object_type( GetCurrentProcess(), L"Process" );
    test_no_file_info( GetCurrentProcess() );

    test_object_name( GetCurrentThread(), L"" );
    test_object_type( GetCurrentThread(), L"Thread" );
    test_no_file_info( GetCurrentThread() );

    status = pNtOpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &handle);
    ok(!status, "OpenProcessToken failed: %lx\n", status);

    test_object_name( handle, L"" );
    test_object_type( handle, L"Token" );
    test_no_file_info( handle );

    pNtClose(handle);

    handle = CreateFileA( "nul", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );
    test_object_name( handle, L"\\Device\\Null" );
    test_object_type( handle, L"File" );
    test_file_info( handle );
    pNtClose( handle );
}

static void test_type_mismatch(void)
{
    HANDLE h;
    NTSTATUS res;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    res = pNtCreateEvent( &h, 0, &attr, NotificationEvent, 0 );
    ok(!res, "can't create event: %lx\n", res);

    res = pNtReleaseSemaphore( h, 30, NULL );
    ok(res == STATUS_OBJECT_TYPE_MISMATCH, "expected 0xc0000024, got %lx\n", res);

    pNtClose( h );
}

static void test_null_device(void)
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING str;
    NTSTATUS status;
    DWORD num_bytes;
    OVERLAPPED ov;
    char buf[64];
    HANDLE null;
    BOOL ret;

    memset(buf, 0xAA, sizeof(buf));
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    RtlInitUnicodeString(&str, L"\\Device\\Null");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&null, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
       "expected STATUS_OBJECT_TYPE_MISMATCH, got %08lx\n", status);

    status = pNtOpenFile(&null, GENERIC_READ | GENERIC_WRITE, &attr, &iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS,
       "expected STATUS_SUCCESS, got %08lx\n", status);

    test_object_type(null, L"File");

    SetLastError(0xdeadbeef);
    ret = WriteFile(null, buf, sizeof(buf), &num_bytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(null, buf, sizeof(buf), &num_bytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = WriteFile(null, buf, sizeof(buf), &num_bytes, &ov);
    ok(ret, "got error %lu\n", GetLastError());
    ok(num_bytes == sizeof(buf), "expected num_bytes = %lu, got %lu\n",
       (DWORD)sizeof(buf), num_bytes);

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(null, buf, sizeof(buf), &num_bytes, &ov);
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_HANDLE_EOF, "got error %lu\n", GetLastError());

    pNtClose(null);

    null = CreateFileA("\\\\.\\Null", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(null == INVALID_HANDLE_VALUE, "CreateFileA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "expected ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    null = CreateFileA("\\\\.\\Device\\Null", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(null == INVALID_HANDLE_VALUE, "CreateFileA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND,
       "expected ERROR_PATH_NOT_FOUND, got %lu\n", GetLastError());

    CloseHandle(ov.hEvent);
}

static void test_process(void)
{
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;
    NTSTATUS status;
    HANDLE process;

    if (!pNtOpenProcess)
    {
        win_skip( "NtOpenProcess not supported, skipping test\n" );
        return;
    }

    InitializeObjectAttributes( &attr, NULL, 0, 0, NULL );

    cid.UniqueProcess = 0;
    cid.UniqueThread = 0;
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, NULL, &cid );
    todo_wine ok( status == STATUS_ACCESS_VIOLATION, "NtOpenProcess returned %lx\n", status );
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, NULL );
    todo_wine ok( status == STATUS_INVALID_PARAMETER_MIX, "NtOpenProcess returned %lx\n", status );

    cid.UniqueProcess = 0;
    cid.UniqueThread = 0;
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( status == STATUS_INVALID_CID, "NtOpenProcess returned %lx\n", status );

    cid.UniqueProcess = ULongToHandle( 0xdeadbeef );
    cid.UniqueThread = ULongToHandle( 0xdeadbeef );
    process = (HANDLE)0xdeadbeef;
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( status == STATUS_INVALID_CID, "NtOpenProcess returned %lx\n", status );
    ok( !process || broken(process == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", process );

    cid.UniqueProcess = ULongToHandle( GetCurrentThreadId() );
    cid.UniqueThread = 0;
    process = (HANDLE)0xdeadbeef;
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( status == STATUS_INVALID_CID, "NtOpenProcess returned %lx\n", status );
    ok( !process || broken(process == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", process );

    cid.UniqueProcess = ULongToHandle( GetCurrentProcessId() );
    cid.UniqueThread = 0;
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( !status, "NtOpenProcess returned %lx\n", status );
    pNtClose( process );

    cid.UniqueProcess = ULongToHandle( GetCurrentProcessId() );
    cid.UniqueThread = ULongToHandle( GetCurrentThreadId() );
    status = pNtOpenProcess( &process, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( !status, "NtOpenProcess returned %lx\n", status );
    pNtClose( process );
    status = pNtOpenProcess( (HANDLE *)0xdeadbee0, PROCESS_QUERY_LIMITED_INFORMATION, &attr, &cid );
    ok( status == STATUS_ACCESS_VIOLATION, "NtOpenProcess returned %lx\n", status );
}

static void test_token(void)
{
    NTSTATUS status;
    HANDLE handle, handle2;

    status = pNtOpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS, (HANDLE *)0xdeadbee0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NtOpenProcessToken failed: %lx\n", status);
    status = pNtOpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, (HANDLE *)0xdeadbee0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NtOpenProcessToken failed: %lx\n", status);
    handle = (HANDLE)0xdeadbeef;
    status = pNtOpenProcessToken( (HANDLE)0xdead, TOKEN_ALL_ACCESS, &handle );
    ok( status == STATUS_INVALID_HANDLE, "NtOpenProcessToken failed: %lx\n", status);
    ok( !handle || broken(handle == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", handle );
    handle = (HANDLE)0xdeadbeef;
    status = pNtOpenThreadToken( (HANDLE)0xdead, TOKEN_ALL_ACCESS, TRUE, &handle );
    ok( status == STATUS_INVALID_HANDLE, "NtOpenThreadToken failed: %lx\n", status);
    ok( !handle || broken(handle == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", handle );

    status = pNtOpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS, &handle );
    ok( status == STATUS_SUCCESS, "NtOpenProcessToken failed: %lx\n", status);
    status = pNtDuplicateToken( handle, TOKEN_ALL_ACCESS, NULL, FALSE, TokenPrimary, &handle2 );
    ok( status == STATUS_SUCCESS, "NtOpenProcessToken failed: %lx\n", status);
    pNtClose( handle2 );
    status = pNtDuplicateToken( handle, TOKEN_ALL_ACCESS, NULL, FALSE, TokenPrimary, (HANDLE *)0xdeadbee0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NtOpenProcessToken failed: %lx\n", status);
    handle2 = (HANDLE)0xdeadbeef;
    status = pNtDuplicateToken( (HANDLE)0xdead, TOKEN_ALL_ACCESS, NULL, FALSE, TokenPrimary, &handle2 );
    ok( status == STATUS_INVALID_HANDLE, "NtOpenProcessToken failed: %lx\n", status);
    ok( !handle2 || broken(handle2 == (HANDLE)0xdeadbeef) /* vista */, "handle set %p\n", handle2 );
    pNtClose( handle );
}

#define DEBUG_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define DEBUG_GENERIC_READ            (STANDARD_RIGHTS_READ|DEBUG_READ_EVENT)
#define DEBUG_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE|DEBUG_PROCESS_ASSIGN)
#define DESKTOP_GENERIC_EXECUTE       (STANDARD_RIGHTS_EXECUTE|DESKTOP_SWITCHDESKTOP)
#define DESKTOP_GENERIC_READ          (STANDARD_RIGHTS_READ|DESKTOP_ENUMERATE|DESKTOP_READOBJECTS)
#define DESKTOP_GENERIC_WRITE         (STANDARD_RIGHTS_WRITE|DESKTOP_WRITEOBJECTS|DESKTOP_JOURNALPLAYBACK|\
                                       DESKTOP_JOURNALRECORD|DESKTOP_HOOKCONTROL|DESKTOP_CREATEMENU| \
                                       DESKTOP_CREATEWINDOW)
#define DIRECTORY_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE|DIRECTORY_TRAVERSE|DIRECTORY_QUERY)
#define DIRECTORY_GENERIC_READ        (STANDARD_RIGHTS_READ|DIRECTORY_TRAVERSE|DIRECTORY_QUERY)
#define DIRECTORY_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE|DIRECTORY_CREATE_SUBDIRECTORY|\
                                       DIRECTORY_CREATE_OBJECT)
#define EVENT_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define EVENT_GENERIC_READ            (STANDARD_RIGHTS_READ|EVENT_QUERY_STATE)
#define EVENT_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE|EVENT_MODIFY_STATE)
#define IO_COMPLETION_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define IO_COMPLETION_GENERIC_READ    (STANDARD_RIGHTS_READ|IO_COMPLETION_QUERY_STATE)
#define IO_COMPLETION_GENERIC_WRITE   (STANDARD_RIGHTS_WRITE|IO_COMPLETION_MODIFY_STATE)
#define IO_COMPLETION_RESERVE_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE)
#define IO_COMPLETION_RESERVE_GENERIC_READ    (STANDARD_RIGHTS_READ|0x1)
#define IO_COMPLETION_RESERVE_GENERIC_WRITE   (STANDARD_RIGHTS_WRITE|0x2)
#define IO_COMPLETION_RESERVE_ALL_ACCESS      (STANDARD_RIGHTS_REQUIRED|0x3)
#define JOB_OBJECT_GENERIC_EXECUTE    (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define JOB_OBJECT_GENERIC_READ       (STANDARD_RIGHTS_READ|JOB_OBJECT_QUERY)
#define JOB_OBJECT_GENERIC_WRITE      (STANDARD_RIGHTS_WRITE|JOB_OBJECT_TERMINATE|\
                                       JOB_OBJECT_SET_ATTRIBUTES|JOB_OBJECT_ASSIGN_PROCESS)
#define KEY_GENERIC_EXECUTE           (STANDARD_RIGHTS_EXECUTE|KEY_CREATE_LINK|KEY_NOTIFY|\
                                       KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE)
#define KEY_GENERIC_READ              (STANDARD_RIGHTS_READ|KEY_NOTIFY|KEY_ENUMERATE_SUB_KEYS|\
                                       KEY_QUERY_VALUE)
#define KEY_GENERIC_WRITE             (STANDARD_RIGHTS_WRITE|KEY_CREATE_SUB_KEY|KEY_SET_VALUE)
#define KEYEDEVENT_GENERIC_EXECUTE    (STANDARD_RIGHTS_EXECUTE)
#define KEYEDEVENT_GENERIC_READ       (STANDARD_RIGHTS_READ|KEYEDEVENT_WAIT)
#define KEYEDEVENT_GENERIC_WRITE      (STANDARD_RIGHTS_WRITE|KEYEDEVENT_WAKE)
#define MUTANT_GENERIC_EXECUTE        (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define MUTANT_GENERIC_READ           (STANDARD_RIGHTS_READ|MUTANT_QUERY_STATE)
#define MUTANT_GENERIC_WRITE          (STANDARD_RIGHTS_WRITE)
#define PROCESS_GENERIC_EXECUTE       (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE|\
                                       PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_TERMINATE)
#define PROCESS_GENERIC_READ          (STANDARD_RIGHTS_READ|PROCESS_VM_READ|PROCESS_QUERY_INFORMATION)
#define PROCESS_GENERIC_WRITE         (STANDARD_RIGHTS_WRITE|PROCESS_SUSPEND_RESUME|\
                                       PROCESS_SET_INFORMATION|PROCESS_SET_QUOTA|PROCESS_CREATE_PROCESS|\
                                       PROCESS_DUP_HANDLE|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|\
                                       PROCESS_CREATE_THREAD)
#define SECTION_GENERIC_EXECUTE       (STANDARD_RIGHTS_EXECUTE|SECTION_MAP_EXECUTE)
#define SECTION_GENERIC_READ          (STANDARD_RIGHTS_READ|SECTION_QUERY|SECTION_MAP_READ)
#define SECTION_GENERIC_WRITE         (STANDARD_RIGHTS_WRITE|SECTION_MAP_WRITE)
#define SEMAPHORE_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define SEMAPHORE_GENERIC_READ        (STANDARD_RIGHTS_READ|SEMAPHORE_QUERY_STATE)
#define SEMAPHORE_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE|SEMAPHORE_MODIFY_STATE)
#define SYMBOLIC_LINK_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE|SYMBOLIC_LINK_QUERY)
#define SYMBOLIC_LINK_GENERIC_READ    (STANDARD_RIGHTS_READ|SYMBOLIC_LINK_QUERY)
#define SYMBOLIC_LINK_GENERIC_WRITE   (STANDARD_RIGHTS_WRITE)
#define THREAD_GENERIC_EXECUTE        (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE|THREAD_RESUME|\
                                       THREAD_QUERY_LIMITED_INFORMATION)
#define THREAD_GENERIC_READ           (STANDARD_RIGHTS_READ|THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT)
#define THREAD_GENERIC_WRITE          (STANDARD_RIGHTS_WRITE|THREAD_SET_LIMITED_INFORMATION|\
                                       THREAD_SET_INFORMATION|THREAD_SET_CONTEXT|THREAD_SUSPEND_RESUME|\
                                       THREAD_TERMINATE|0x04)
#define TIMER_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE|SYNCHRONIZE)
#define TIMER_GENERIC_READ            (STANDARD_RIGHTS_READ|TIMER_QUERY_STATE)
#define TIMER_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE|TIMER_MODIFY_STATE)
#define TOKEN_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE|TOKEN_IMPERSONATE|TOKEN_ASSIGN_PRIMARY)
#define TOKEN_GENERIC_READ            (STANDARD_RIGHTS_READ|TOKEN_QUERY_SOURCE|TOKEN_QUERY|TOKEN_DUPLICATE)
#define TOKEN_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE|TOKEN_ADJUST_SESSIONID|TOKEN_ADJUST_DEFAULT|\
                                       TOKEN_ADJUST_GROUPS|TOKEN_ADJUST_PRIVILEGES)
#define TYPE_GENERIC_EXECUTE          (STANDARD_RIGHTS_EXECUTE)
#define TYPE_GENERIC_READ             (STANDARD_RIGHTS_READ)
#define TYPE_GENERIC_WRITE            (STANDARD_RIGHTS_WRITE)
#define USER_APC_RESERVE_GENERIC_EXECUTE (STANDARD_RIGHTS_EXECUTE)
#define USER_APC_RESERVE_GENERIC_READ    (STANDARD_RIGHTS_READ|0x1)
#define USER_APC_RESERVE_GENERIC_WRITE   (STANDARD_RIGHTS_WRITE|0x2)
#define USER_APC_RESERVE_ALL_ACCESS      (STANDARD_RIGHTS_REQUIRED|0x3)
#define WINSTA_GENERIC_EXECUTE        (STANDARD_RIGHTS_EXECUTE|WINSTA_EXITWINDOWS|WINSTA_ACCESSGLOBALATOMS)
#define WINSTA_GENERIC_READ           (STANDARD_RIGHTS_READ|WINSTA_READSCREEN|WINSTA_ENUMERATE|\
                                       WINSTA_READATTRIBUTES|WINSTA_ENUMDESKTOPS)
#define WINSTA_GENERIC_WRITE          (STANDARD_RIGHTS_WRITE|WINSTA_WRITEATTRIBUTES|WINSTA_CREATEDESKTOP|\
                                       WINSTA_ACCESSCLIPBOARD)

#undef WINSTA_ALL_ACCESS
#undef DESKTOP_ALL_ACCESS
#define WINSTA_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED|0x37f)
#define DESKTOP_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|0x1ff)
#define DEVICE_ALL_ACCESS  (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x1ff)
#define TYPE_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED|0x1)

static void *align_ptr( void *ptr )
{
    ULONG_PTR align = sizeof(DWORD_PTR) - 1;
    return (void *)(((DWORD_PTR)ptr + align) & ~align);
}

static void test_duplicate_object(void)
{
    NTSTATUS status;
    HANDLE handle;

    status = pNtDuplicateObject( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                                 &handle, PROCESS_ALL_ACCESS, 0, 0 );
    ok( !status, "NtDuplicateObject failed %lx\n", status );
    pNtClose( handle );
    status = pNtDuplicateObject( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                                 NULL, PROCESS_ALL_ACCESS, 0, 0 );
    ok( !status, "NtDuplicateObject failed %lx\n", status );

    status = pNtDuplicateObject( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                                 (HANDLE *)0xdeadbee0, PROCESS_ALL_ACCESS, 0, 0 );
    ok( status == STATUS_ACCESS_VIOLATION, "NtDuplicateObject failed %lx\n", status );

    handle = (HANDLE)0xdeadbeef;
    status = pNtDuplicateObject( GetCurrentProcess(), (HANDLE)0xdead, GetCurrentProcess(),
                                 &handle, PROCESS_ALL_ACCESS, 0, 0 );
    ok( status == STATUS_INVALID_HANDLE, "NtDuplicateObject failed %lx\n", status );
    ok( !handle, "handle set %p\n", handle );

    handle = (HANDLE)0xdeadbeef;
    status = pNtDuplicateObject( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                                 &handle, ~0u, 0, 0 );
    todo_wine
    ok( status == STATUS_ACCESS_DENIED, "NtDuplicateObject failed %lx\n", status );
    todo_wine
    ok( !handle, "handle set %p\n", handle );
    if (!status) pNtClose( handle );
}

static void test_object_types(void)
{
    static const struct { const WCHAR *name; GENERIC_MAPPING mapping; ULONG mask, broken; } tests[] =
    {
#define TYPE(name,gen,extra,broken) { name, { gen ## _GENERIC_READ, gen ## _GENERIC_WRITE, \
                gen ## _GENERIC_EXECUTE, gen ## _ALL_ACCESS }, gen ## _ALL_ACCESS | extra, broken }
        TYPE( L"DebugObject",   DEBUG, 0, 0 ),
        TYPE( L"Desktop",       DESKTOP, 0, 0 ),
        TYPE( L"Device",        FILE, 0, 0 ),
        TYPE( L"Directory",     DIRECTORY, 0, 0 ),
        TYPE( L"Event",         EVENT, 0, 0 ),
        TYPE( L"File",          FILE, 0, 0 ),
        TYPE( L"IoCompletion",  IO_COMPLETION, 0, 0 ),
        TYPE( L"IoCompletionReserve", IO_COMPLETION_RESERVE, 0, 0 ),
#ifdef __REACTOS__
        TYPE( L"Job",           JOB_OBJECT, 0, JOB_OBJECT_IMPERSONATE ),
        TYPE( L"Key",           KEY, SYNCHRONIZE, KEY_CREATE_LINK ),
#else
        TYPE( L"Job",           JOB_OBJECT, 0, JOB_OBJECT_IMPERSONATE | 0x3C0 ),
        TYPE( L"Key",           KEY, SYNCHRONIZE, 0 ),
#endif
        TYPE( L"KeyedEvent",    KEYEDEVENT, SYNCHRONIZE, 0 ),
        TYPE( L"Mutant",        MUTANT, 0, 0 ),
#ifdef __REACTOS__
        TYPE( L"Process",       PROCESS, 0, PROCESS_TERMINATE ),
#else
        TYPE( L"Process",       PROCESS, 0, 0 ),
#endif
        TYPE( L"Section",       SECTION, SYNCHRONIZE, 0 ),
        TYPE( L"Semaphore",     SEMAPHORE, 0, 0 ),
        TYPE( L"SymbolicLink",  SYMBOLIC_LINK, 0, 0xfffe ),
#ifdef __REACTOS__
        TYPE( L"Thread",        THREAD, 0, THREAD_RESUME | THREAD_QUERY_LIMITED_INFORMATION ),
#else
        TYPE( L"Thread",        THREAD, 0, THREAD_RESUME ),
#endif
        TYPE( L"Timer",         TIMER, 0, 0 ),
#ifdef __REACTOS__
        TYPE( L"Token",         TOKEN, SYNCHRONIZE, TOKEN_QUERY_SOURCE | TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY),
#else
        TYPE( L"Token",         TOKEN, SYNCHRONIZE, 0 ),
#endif
        TYPE( L"Type",          TYPE, SYNCHRONIZE, 0 ),
        TYPE( L"UserApcReserve", USER_APC_RESERVE, 0, 0 ),
        TYPE( L"WindowStation", WINSTA, 0, 0 ),
#undef TYPE
    };
    unsigned int i, j;
    BOOLEAN tested[ARRAY_SIZE(all_types)] = { 0 };
    char buffer[256];
    OBJECT_TYPES_INFORMATION *info = (OBJECT_TYPES_INFORMATION *)buffer;
    GENERIC_MAPPING map;
    NTSTATUS status;
    ULONG len, retlen;

    memset( buffer, 0xcc, sizeof(buffer) );
    status = pNtQueryObject( NULL, ObjectTypesInformation, info, sizeof(buffer), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %lx\n", status );
    ok( info->NumberOfTypes < 100 || info->NumberOfTypes == 0xcccccccc, /* wow64 */
        "wrong number of types %lu\n", info->NumberOfTypes );

    info = malloc( len + 16 );  /* Windows gets the length wrong on WoW64 and overflows the buffer */
    memset( info, 0xcc, sizeof(*info) );
    status = pNtQueryObject( NULL, ObjectTypesInformation, info, len, &retlen );
    ok( retlen <= len + 16, "wrong len %lx/%lx\n", len, retlen );
    ok( len == retlen || broken( retlen >= len - 32 && retlen <= len + 32 ),  /* wow64 */
        "wrong len %lx/%lx\n", len, retlen );
    ok( !status, "NtQueryObject failed %lx\n", status );
    if (!status)
    {
        OBJECT_TYPE_INFORMATION *type = align_ptr( info + 1 );
        for (i = 0; i < info->NumberOfTypes; i++)
        {
            add_object_type( type );
            type = align_ptr( (char *)type->TypeName.Buffer + type->TypeName.MaximumLength );
        }
    }
    free( info );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
#ifdef __REACTOS__
        if (GetNTVersion() < _WIN32_WINNT_WIN7)
        {
            if (!wcscmp(tests[i].name, L"IoCompletionReserve") ||
                !wcscmp(tests[i].name, L"UserApcReserve"))
            {
                skip("%s not implemented before Win7\n", tests[i].name);
                break;
            }
        }
#endif
        for (j = 0; j < ARRAY_SIZE(all_types); j++)
        {
            if (!all_types[j].TypeName.Buffer) continue;
            if (wcscmp( tests[i].name, all_types[j].TypeName.Buffer )) continue;
            map = all_types[j].GenericMapping;
            ok( !memcmp( &map, &tests[i].mapping, sizeof(GENERIC_MAPPING) ) ||
                broken( !((map.GenericRead ^ tests[i].mapping.GenericRead) & ~tests[i].broken) &&
                        !((map.GenericWrite ^ tests[i].mapping.GenericWrite) & ~tests[i].broken) &&
                        !((map.GenericExecute ^ tests[i].mapping.GenericExecute) & ~tests[i].broken) &&
                        !((map.GenericAll ^ tests[i].mapping.GenericAll) & ~tests[i].broken) ),
                "%s: mismatched mappings %08lx,%08lx,%08lx,%08lx / %08lx,%08lx,%08lx,%08lx\n",
                debugstr_w( tests[i].name ),
                all_types[j].GenericMapping.GenericRead, all_types[j].GenericMapping.GenericWrite,
                all_types[j].GenericMapping.GenericExecute, all_types[j].GenericMapping.GenericAll,
                tests[i].mapping.GenericRead, tests[i].mapping.GenericWrite,
                tests[i].mapping.GenericExecute, tests[i].mapping.GenericAll );
            ok( all_types[j].ValidAccessMask == tests[i].mask ||
                broken( !((all_types[j].ValidAccessMask ^ tests[i].mask) & ~tests[i].broken) ),
                "%s: mismatched access mask %08lx / %08lx\n", debugstr_w( tests[i].name ),
                all_types[j].ValidAccessMask, tests[i].mask );
            tested[j] = TRUE;
            break;
        }

        ok( j < ARRAY_SIZE(all_types), "type %s not found\n", debugstr_w(tests[i].name) );
    }
    for (j = 0; j < ARRAY_SIZE(all_types); j++)
    {
        if (!all_types[j].TypeName.Buffer) continue;
        if (tested[j]) continue;
        trace( "not tested: %s\n", debugstr_w(all_types[j].TypeName.Buffer ));
    }
}

static DWORD WINAPI test_get_next_thread_proc( void *arg )
{
    HANDLE event = (HANDLE)arg;

    WaitForSingleObject(event, INFINITE);
    return 0;
}

static void test_get_next_thread(void)
{
    HANDLE hprocess = GetCurrentProcess();
    HANDLE handle, thread, event, prev;
    NTSTATUS status;
    DWORD thread_id;
    BOOL found;

    if (!pNtGetNextThread)
    {
        win_skip("NtGetNextThread is not available.\n");
        return;
    }

    event = CreateEventA(NULL, FALSE, FALSE, NULL);

    thread = CreateThread( NULL, 0, test_get_next_thread_proc, event, 0, &thread_id );

    status = pNtGetNextThread(hprocess, NULL, THREAD_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 0, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "Got unexpected status %#lx.\n", status);

    found = FALSE;
    prev = NULL;
    while (!(status = pNtGetNextThread(hprocess, prev, THREAD_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 0, &handle)))
    {
        if (prev)
        {
            if (GetThreadId(handle) == thread_id)
                found = TRUE;
            pNtClose(prev);
        }
        else
        {
            ok(GetThreadId(handle) == GetCurrentThreadId(), "Got unexpected thread id %04lx, current %04lx.\n",
                    GetThreadId(handle), GetCurrentThreadId());
        }
        prev = handle;
        handle = (HANDLE)0xdeadbeef;
    }
    pNtClose(prev);
    ok(!handle, "Got unexpected handle %p.\n", handle);
    ok(status == STATUS_NO_MORE_ENTRIES, "Unexpected status %#lx.\n", status);
    ok(found, "Thread not found.\n");

    handle = (HANDLE)0xdeadbeef;
    status = pNtGetNextThread((void *)0xdeadbeef, 0, PROCESS_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 0, &handle);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected status %#lx.\n", status);
    ok(!handle, "Got unexpected handle %p.\n", handle);
    handle = (HANDLE)0xdeadbeef;
    status = pNtGetNextThread(hprocess, (void *)0xdeadbeef, PROCESS_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 0, &handle);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected status %#lx.\n", status);
    ok(!handle, "Got unexpected handle %p.\n", handle);

    /* Reversed search is only supported on recent enough Win10. */
    status = pNtGetNextThread(hprocess, 0, PROCESS_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 1, &handle);
    ok(!status || broken(status == STATUS_INVALID_PARAMETER), "Unexpected status %#lx.\n", status);
    if (!status)
        pNtClose(handle);

    status = pNtGetNextThread(hprocess, 0, PROCESS_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 2, &handle);
    ok(status == STATUS_INVALID_PARAMETER, "Unexpected status %#lx.\n", status);

    SetEvent(event);
    WaitForSingleObject(thread, INFINITE);

    found = FALSE;
    prev = NULL;
    while (!(status = pNtGetNextThread(hprocess, prev, THREAD_QUERY_LIMITED_INFORMATION, OBJ_INHERIT, 0, &handle)))
    {
        if (prev)
            pNtClose(prev);
        if (GetThreadId(handle) == thread_id)
            found = TRUE;
        prev = handle;
    }
    pNtClose(prev);
    ok(found, "Thread not found.\n");

    CloseHandle(thread);
}

static void test_globalroot(void)
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE h;
    WCHAR buffer[256];
    ULONG len, full_len, i;
    static const struct { const WCHAR *name, *target; } symlinks[] = {
        { L"\\??\\GLOBALROOT", L"" },
        { L"\\??\\GLOBALROOT\\??\\GLOBALROOT", L"" },
        { L"\\??\\GLOBALROOT\\??\\GLOBALROOT\\??\\GLOBALROOT", L"" },
        { L"\\??\\GLOBALROOT\\DosDevices", L"\\??" },
        { L"\\??\\GLOBALROOT\\BaseNamedObjects\\Global", NULL },
    };

    for (i = 0; i < ARRAY_SIZE(symlinks); i++)
    {
        pRtlInitUnicodeString(&str, symlinks[i].name);
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        status = pNtOpenSymbolicLinkObject( &h, SYMBOLIC_LINK_QUERY, &attr );
        ok(status == STATUS_SUCCESS, "NtOpenSymbolicLinkObject failed %08lx\n", status);

        str.Buffer = buffer;
        str.MaximumLength = sizeof(buffer);
        len = 0xdeadbeef;
        memset( buffer, 0xaa, sizeof(buffer) );
        status = pNtQuerySymbolicLinkObject( h, &str, &len);
        ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08lx\n", status );
        full_len = str.Length + sizeof(WCHAR);
        ok( len == full_len, "bad length %lu (expected %lu)\n", len, full_len );
        ok( buffer[len / sizeof(WCHAR) - 1] == 0, "no terminating null\n" );

        if (symlinks[i].target)
        {
            ok( compare_unicode_string( &str, symlinks[i].target ),
                "symlink %s: target expected %s, got %s\n",
                debugstr_w( symlinks[i].name ),
                debugstr_w( symlinks[i].target ),
                debugstr_w( str.Buffer ) );
        }

        pNtClose(h);
    }

    pRtlInitUnicodeString(&str, L"\\??\\GLOBALROOT\\Device\\Null");
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ | GENERIC_WRITE, &attr, &iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
    ok(status == STATUS_SUCCESS,
       "expected STATUS_SUCCESS, got %08lx\n", status);

    test_object_type(h, L"File");

    pNtClose(h);
}

static void test_object_identity(void)
{
    NTSTATUS status;
    HANDLE h1, h2;

    if (!pNtCompareObjects)
    {
        win_skip("NtCompareObjects is not available.\n");
        return;
    }

    status = pNtCompareObjects( GetCurrentProcess(), GetCurrentProcess() );
    ok( status == STATUS_SUCCESS, "comparing GetCurrentProcess() to self failed with %08lx\n", status );

    status = pNtCompareObjects( GetCurrentThread(), GetCurrentThread() );
    ok( status == STATUS_SUCCESS, "comparing GetCurrentThread() to self failed with %08lx\n", status );

    status = pNtCompareObjects( GetCurrentProcess(), GetCurrentThread() );
    ok( status == STATUS_NOT_SAME_OBJECT, "comparing GetCurrentProcess() to GetCurrentThread() returned %08lx\n", status );

    h1 = NULL;
    status = pNtDuplicateObject( GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
                                 &h1, 0, 0, DUPLICATE_SAME_ACCESS );
    ok( status == STATUS_SUCCESS, "failed to duplicate current process handle: %08lx\n", status);

    status = pNtCompareObjects( GetCurrentProcess(), h1 );
    ok( status == STATUS_SUCCESS, "comparing GetCurrentProcess() with %p failed with %08lx\n", h1, status );

    pNtClose( h1 );

    h1 = CreateFileA( "\\\\.\\NUL", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h1 != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    h2 = NULL;
    status = pNtDuplicateObject( GetCurrentProcess(), h1, GetCurrentProcess(),
                                  &h2, 0, 0, DUPLICATE_SAME_ACCESS );
    ok( status == STATUS_SUCCESS, "failed to duplicate handle %p: %08lx\n", h1, status);

    status = pNtCompareObjects( h1, h2 );
    ok( status == STATUS_SUCCESS, "comparing %p with %p failed with %08lx\n", h1, h2, status );

    pNtClose( h2 );

    h2 = CreateFileA( "\\\\.\\NUL", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h2 != INVALID_HANDLE_VALUE, "CreateFile failed (%ld)\n", GetLastError() );

    status = pNtCompareObjects( h1, h2 );
    ok( status == STATUS_NOT_SAME_OBJECT, "comparing %p with %p returned %08lx\n", h1, h2, status );

    pNtClose( h2 );
    pNtClose( h1 );
}

static void test_query_directory(void)
{
    static const DIRECTORY_BASIC_INFORMATION empty_info;
    char buffer[200];
    DIRECTORY_BASIC_INFORMATION *info = (void *)buffer;
    ULONG context, size, needed_size;
    const WCHAR *name1, *name2;
    HANDLE dir, child1, child2;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    NTSTATUS status;
    BOOL is_wow64 = FALSE;

#ifndef _WIN64
    if (!IsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;
#endif

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest" );
    InitializeObjectAttributes( &attr, &string, 0, 0, NULL );
    status = pNtCreateDirectoryObject( &dir, DIRECTORY_QUERY, &attr );
    ok( !status, "got %#lx\n", status );

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, 0, TRUE, TRUE, &context, &size );
    ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == sizeof(*info) || (is_wow64 && !size), "got size %lu\n", size );

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, 0, FALSE, TRUE, &context, &size );
    ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == sizeof(*info) || (is_wow64 && !size), "got size %lu\n", size );

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, TRUE, &context, &size );
    ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == sizeof(*info) || (is_wow64 && !size), "got size %lu\n", size );
    if (size == sizeof(*info))
        ok( !memcmp( &info[0], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), FALSE, TRUE, &context, &size );
    ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == sizeof(*info) || (is_wow64 && !size), "got size %lu\n", size );
    if (size == sizeof(*info))
        ok( !memcmp( &info[0], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest\\Telamon" );
    status = NtCreateMutant( &child1, GENERIC_ALL, &attr, FALSE );
    ok( !status, "got %#lx\n", status );

    RtlInitUnicodeString( &string, L"\\BaseNamedObjects\\winetest\\Oileus" );
    status = NtCreateMutant( &child2, GENERIC_ALL, &attr, FALSE );
    ok( !status, "got %#lx\n", status );

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    status = NtQueryDirectoryObject( NULL, info, sizeof(buffer), TRUE, TRUE, &context, &size );
    ok( status == STATUS_INVALID_HANDLE, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == 0xdeadbeef || broken(!size) /* WoW64 */, "got size %lu\n", size);

    size = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, TRUE, NULL, &size );
    ok( status == STATUS_ACCESS_VIOLATION, "got %#lx\n", status );
    ok( size == 0xdeadbeef, "got size %lu\n", size);

    context = 0xdeadbeef;
    size = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, TRUE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 1, "got context %#lx\n", context );
    ok( size && size < sizeof(buffer), "got size %lu\n", size );
    if (!wcscmp( info[0].ObjectName.Buffer, L"Oileus" ))
    {
        name1 = L"Oileus";
        name2 = L"Telamon";
    }
    else
    {
        name1 = L"Telamon";
        name2 = L"Oileus";
    }
    check_unicode_string( &info[0].ObjectName, name1 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, FALSE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 2, "got context %#lx\n", context );
    check_unicode_string( &info[0].ObjectName, name2 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    size = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, FALSE, &context, &size );
    ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 2, "got context %#lx\n", context );
    ok( size == sizeof(*info) || (is_wow64 && !size), "got size %lu\n", size );

    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, TRUE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 1, "got context %#lx\n", context );
    check_unicode_string( &info[0].ObjectName, name1 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    needed_size = size;

    size = 0xdeadbeef;
    context = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, 0, TRUE, TRUE, &context, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", status );
    ok( context == 0xdeadbeef, "got context %#lx\n", context );
    ok( size == needed_size, "expected size %lu, got %lu\n", needed_size, size );

    size = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, needed_size - 1, TRUE, TRUE, &context, &size );
    ok( status == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", status );
    ok( size == needed_size, "expected size %lu, got %lu\n", needed_size, size );

    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, TRUE, &context, NULL );
    ok( !status, "got %#lx\n", status );

    context = 0;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), TRUE, FALSE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 1, "got context %#lx\n", context );
    check_unicode_string( &info[0].ObjectName, name1 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), FALSE, TRUE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 2, "got context %#lx\n", context );
    check_unicode_string( &info[0].ObjectName, name1 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    check_unicode_string( &info[1].ObjectName, name2 );
    check_unicode_string( &info[1].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[2], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    needed_size = size;
    size = 0xdeadbeef;
    context = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, needed_size - 1, FALSE, TRUE, &context, &size );
    ok( status == STATUS_MORE_ENTRIES, "got %#lx\n", status );
    ok( context == 1, "got context %#lx\n", context );
    ok( size > 0 && size < needed_size, "got size %lu\n", size );
    check_unicode_string( &info[0].ObjectName, name1 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    size = 0xdeadbeef;
    context = 0xdeadbeef;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(*info), FALSE, TRUE, &context, &size );
    ok( status == STATUS_MORE_ENTRIES
            || broken(status == STATUS_BUFFER_TOO_SMALL) /* wow64 */, "got %#lx\n", status );
    if (status == STATUS_MORE_ENTRIES)
    {
        ok( !context, "got context %#lx\n", context );
        ok( size == sizeof(*info), "got size %lu\n", size );
        ok( !memcmp( &info[0], &empty_info, sizeof(*info) ), "entry was not cleared\n" );
    }

    size = 0xdeadbeef;
    context = 0xdeadbeef;
    status = NtQueryDirectoryObject( dir, info, 0, FALSE, TRUE, &context, &size );
    ok( status == STATUS_MORE_ENTRIES
            || broken(status == STATUS_BUFFER_TOO_SMALL) /* wow64 */, "got %#lx\n", status );
    if (status == STATUS_MORE_ENTRIES)
    {
        ok( !context, "got context %#lx\n", context );
        ok( size == sizeof(*info), "got size %lu\n", size );
    }

    context = 1;
    memset( buffer, 0xcc, sizeof(buffer) );
    status = NtQueryDirectoryObject( dir, info, sizeof(buffer), FALSE, FALSE, &context, &size );
    ok( !status, "got %#lx\n", status );
    ok( context == 2, "got context %#lx\n", context );
    check_unicode_string( &info[0].ObjectName, name2 );
    check_unicode_string( &info[0].ObjectTypeName, L"Mutant" );
    ok( !memcmp( &info[1], &empty_info, sizeof(*info) ), "entry was not cleared\n" );

    pNtClose( child1 );
    pNtClose( child2 );
    pNtClose( dir );
}

#define test_object_name_with_null(a,b) _test_object_name_with_null(__LINE__,a,b)
static void _test_object_name_with_null(unsigned line, HANDLE handle, UNICODE_STRING *expect)
{
    char buffer[1024];
    UNICODE_STRING *str = (UNICODE_STRING *)buffer;
    ULONG len = 0;
    NTSTATUS status;

    memset(buffer, 0, sizeof(buffer));
    status = pNtQueryObject(handle, ObjectNameInformation, buffer, sizeof(buffer), &len);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "got %08lx\n", status);
    ok_(__FILE__,line)(len >= sizeof(OBJECT_NAME_INFORMATION) + str->Length, "got %lu\n", len);
    ok_(__FILE__,line)(str->Length == expect->Length, "got %u, expected %u\n", str->Length, expect->Length);
    ok_(__FILE__,line)(!wcsnicmp(str->Buffer, expect->Buffer, str->Length/sizeof(WCHAR)), "got %s, expected %s\n",
        debugstr_w(str->Buffer), debugstr_w(expect->Buffer));
}

static void test_null_in_object_name(void)
{
    WCHAR name[256], name3[256], *p, *name_exp, *name3_exp;
    HANDLE handle, handle2;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr, attr2, attr3;
    UNICODE_STRING nameU, name2U, name3U, name2U_exp, name3U_exp;
    LARGE_INTEGER size;
#ifndef _WIN64
    BOOL is_wow64 = FALSE;
#endif

    trace("running as %d bit\n", (int)sizeof(void *) * 8);

    swprintf(name, ARRAY_SIZE(name), L"\\Sessions\\%u\\BaseNamedObjects\\wine_test", NtCurrentTeb()->Peb->SessionId);
    swprintf(name3, ARRAY_SIZE(name3), L"\\Sessions\\%u\\BaseNamedObjects\\wine_test", NtCurrentTeb()->Peb->SessionId);
    p = wcsrchr(name3, '\\');
    p[5] = 0; /* => \\wine\0test */

    RtlInitUnicodeString(&nameU, name);
    InitializeObjectAttributes(&attr, &nameU, 0, 0, NULL);

    name2U = nameU;
    name2U.Length += sizeof(WCHAR); /* add terminating \0 to string length */
    InitializeObjectAttributes(&attr2, &name2U, 0, 0, NULL);

    name3U = nameU;
    name3U.Buffer = name3;
    InitializeObjectAttributes(&attr3, &name3U, 0, 0, NULL);

    status = pNtCreateEvent(&handle, GENERIC_ALL, &attr, NotificationEvent, FALSE);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenEvent(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    status = pNtOpenEvent(&handle2, GENERIC_ALL, &attr2);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateEvent(&handle, GENERIC_ALL, &attr2, NotificationEvent, FALSE);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenEvent(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    status = pNtOpenEvent(&handle2, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateEvent(&handle, GENERIC_ALL, &attr3, NotificationEvent, FALSE);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenEvent(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateDebugObject(&handle, GENERIC_ALL, &attr, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    pNtClose(handle);
    status = pNtCreateDebugObject(&handle, GENERIC_ALL, &attr2, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    pNtClose(handle);
    status = pNtCreateDebugObject(&handle, GENERIC_ALL, &attr3, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    pNtClose(handle);

    status = pNtCreateMutant(&handle, GENERIC_ALL, &attr, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenMutant(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    status = pNtOpenMutant(&handle2, GENERIC_ALL, &attr2);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateMutant(&handle, GENERIC_ALL, &attr2, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenMutant(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    status = pNtOpenMutant(&handle2, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateMutant(&handle, GENERIC_ALL, &attr3, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenMutant(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateSemaphore(&handle, GENERIC_ALL, &attr, 1, 2);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenSemaphore(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    status = pNtOpenSemaphore(&handle2, GENERIC_ALL, &attr2);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateSemaphore(&handle, GENERIC_ALL, &attr2, 1, 2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenSemaphore(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    status = pNtOpenSemaphore(&handle2, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateSemaphore(&handle, GENERIC_ALL, &attr3, 1, 2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenSemaphore(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateKeyedEvent(&handle, GENERIC_ALL, &attr, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenKeyedEvent(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    status = pNtOpenKeyedEvent(&handle2, GENERIC_ALL, &attr2);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateKeyedEvent(&handle, GENERIC_ALL, &attr2, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenKeyedEvent(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    status = pNtOpenKeyedEvent(&handle2, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
    pNtClose(handle);
    status = pNtCreateKeyedEvent(&handle, GENERIC_ALL, &attr3, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenKeyedEvent(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateIoCompletion(&handle, GENERIC_ALL, &attr, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenIoCompletion(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateIoCompletion(&handle, GENERIC_ALL, &attr2, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenIoCompletion(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateIoCompletion(&handle, GENERIC_ALL, &attr3, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenIoCompletion(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateJobObject(&handle, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenJobObject(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateJobObject(&handle, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenJobObject(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateJobObject(&handle, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenJobObject(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    status = pNtCreateTimer(&handle, GENERIC_ALL, &attr, NotificationTimer);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenTimer(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateTimer(&handle, GENERIC_ALL, &attr2, NotificationTimer);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenTimer(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateTimer(&handle, GENERIC_ALL, &attr3, NotificationTimer);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenTimer(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    size.QuadPart = 4096;
    status = pNtCreateSection(&handle, GENERIC_ALL, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle, name);
    status = pNtOpenSection(&handle2, GENERIC_ALL, &attr);
    ok(!status, "got %08lx\n", status);
    test_object_name(handle2, name);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateSection(&handle, GENERIC_ALL, &attr2, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name2U);
    status = pNtOpenSection(&handle2, GENERIC_ALL, &attr2);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name2U);
    pNtClose(handle2);
    pNtClose(handle);
    status = pNtCreateSection(&handle, GENERIC_ALL, &attr3, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle, &name3U);
    status = pNtOpenSection(&handle2, GENERIC_ALL, &attr3);
    ok(!status, "got %08lx\n", status);
    test_object_name_with_null(handle2, &name3U);
    pNtClose(handle2);
    pNtClose(handle);

    wcscpy(name, L"\\Registry\\Machine\\Software\\wine_test");
    wcscpy(name3, L"\\Registry\\Machine\\Software\\wine_test");
    p = wcsrchr(name3, '\\');
    p[5] = 0; /* => \\wine\0test */

    RtlInitUnicodeString(&nameU, name);
    name2U = nameU;
    name3U = nameU;
    name3U.Buffer = name3;
#ifdef _WIN64
    name_exp = name;
    name3_exp = name3;
    name2U_exp = name2U;
#else
    if (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)
    {
        name_exp = (WCHAR *)L"\\Registry\\Machine\\Software\\WOW6432Node\\wine_test";
        name3_exp =(WCHAR *) L"\\Registry\\Machine\\Software\\WOW6432Node\\wine\0test";
    }
    else
    {
        name_exp = name;
        name3_exp = name3;
    }
    RtlInitUnicodeString(&name2U_exp, name_exp);
#endif
    name3U_exp = name2U_exp;
    name3U_exp.Buffer = name3_exp;
    name2U.Length += sizeof(WCHAR); /* add terminating \0 to string length */
    name2U_exp.Length += sizeof(WCHAR); /* add terminating \0 to string length */

    status = pNtCreateKey(&handle, GENERIC_ALL, &attr, 0, NULL, 0, NULL);
    ok(!status || status == STATUS_ACCESS_DENIED || broken(status == STATUS_OBJECT_PATH_NOT_FOUND) /* win8 */, "got %08lx\n", status);
    if (!status)
    {
        test_object_name(handle, name_exp);
        status = pNtOpenKey(&handle2, GENERIC_ALL, &attr);
        ok(!status, "got %08lx\n", status);
        test_object_name(handle2, name_exp);
        pNtClose(handle2);
        status = pNtOpenKey(&handle2, GENERIC_ALL, &attr2);
        ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
        pNtDeleteKey(handle);
        pNtClose(handle);
        status = pNtCreateKey(&handle, GENERIC_ALL, &attr2, 0, NULL, 0, NULL);
        ok(!status, "got %08lx\n", status);
        test_object_name_with_null(handle, &name2U_exp);
        status = pNtOpenKey(&handle2, GENERIC_ALL, &attr2);
        ok(!status, "got %08lx\n", status);
        test_object_name_with_null(handle, &name2U_exp);
        pNtClose(handle2);
        status = pNtOpenKey(&handle2, GENERIC_ALL, &attr);
        ok(status == STATUS_OBJECT_NAME_NOT_FOUND, "got %08lx\n", status);
        pNtDeleteKey(handle);
        pNtClose(handle);
        status = pNtCreateKey(&handle, GENERIC_ALL, &attr3, 0, NULL, 0, NULL);
        ok(!status, "got %08lx\n", status);
        test_object_name_with_null(handle, &name3U_exp);
        status = pNtOpenKey(&handle2, GENERIC_ALL, &attr3);
        ok(!status, "got %08lx\n", status);
        test_object_name_with_null(handle, &name3U_exp);
        pNtClose(handle2);
        pNtDeleteKey(handle);
        pNtClose(handle);
    }
    else
        skip("Limited access to \\Registry\\Machine\\Software key, skipping the tests\n");
}

static void test_object_permanence(void)
{
    static const struct object_permanence_test {
        const char *name;
        ULONG initial_attr;
        ACCESS_MASK access;
        BOOLEAN make_temporary;
        BOOLEAN make_permanent;
        NTSTATUS make_temp_status;
    } tests[] = {
        {
            .name = "permanent object persists",
            .initial_attr     = OBJ_PERMANENT,
            .access           = GENERIC_ALL,
        },
        {
            .name = "NtMakeTemporaryObject() succeeds",
            .initial_attr     = OBJ_PERMANENT,
            .access           = GENERIC_ALL,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_SUCCESS,
        },
        {
            .name = "NtMakeTemporaryObject() fails w/o DELETE access",
            .initial_attr     = OBJ_PERMANENT,
            .access           = EVENT_ALL_ACCESS & ~DELETE,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_ACCESS_DENIED,
        },
        {
            .name = "NtMakePermanentObject() succeeds even if already permanent",
            .initial_attr     = OBJ_PERMANENT,
            .access           = EVENT_ALL_ACCESS & ~DELETE,
            .make_permanent   = TRUE,
        },
        {
            .name = "NtMakePermanentObject() reverses effect of NtMakeTemporaryObject()",
            .initial_attr     = OBJ_PERMANENT,
            .access           = GENERIC_ALL,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_SUCCESS,
            .make_permanent   = TRUE,
        },

        {
            .name = "temporary object disappears",
            .initial_attr     = 0,
            .access           = GENERIC_ALL,
        },
        {
            .name = "NtMakeTemporaryObject() succeeds even if already temporary",
            .initial_attr     = 0,
            .access           = GENERIC_ALL,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_SUCCESS,
        },
        {
            .name = "NtMakeTemporaryObject() fails w/o DELETE access even if already temporary",
            .initial_attr     = 0,
            .access           = EVENT_ALL_ACCESS & ~DELETE,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_ACCESS_DENIED,
        },
        {
            .name = "NtMakePermanentObject() makes an object persist",
            .initial_attr     = 0,
            .access           = EVENT_ALL_ACCESS & ~DELETE,
            .make_permanent   = TRUE,
        },
        {
            .name = "NtMakePermanentObject() is not annulled by calling NtMakeTemporaryObject() on an already temporary object",
            .initial_attr     = 0,
            .access           = GENERIC_ALL,
            .make_temporary   = TRUE,
            .make_temp_status = STATUS_SUCCESS,
            .make_permanent   = TRUE,
        },
    };
    const struct object_permanence_test *test;
    HANDLE process_token = NULL, thread_token = NULL;
    SECURITY_QUALITY_OF_SERVICE token_qos = {
        .Length = sizeof(token_qos),
        .ImpersonationLevel = SecurityDelegation,
        .ContextTrackingMode = SECURITY_STATIC_TRACKING,
        .EffectiveOnly = FALSE,
    };
    OBJECT_ATTRIBUTES token_attr = {
        .Length = sizeof(token_attr),
        .SecurityQualityOfService = &token_qos,
    };
    TOKEN_PRIVILEGES new_privs = {
        .PrivilegeCount = 1,
        .Privileges = {
            {
                .Luid = { .LowPart = SE_CREATE_PERMANENT_PRIVILEGE },
                .Attributes = SE_PRIVILEGE_ENABLED,
            },
        },
    };
    NTSTATUS status;
    BOOL creatpermapriv = FALSE;

    status = NtOpenProcessToken( GetCurrentProcess(), TOKEN_DUPLICATE, &process_token );
    ok( status == STATUS_SUCCESS, "NtOpenProcessToken returned %08lx\n", status );

    status = NtDuplicateToken( process_token, TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES,
                               &token_attr, FALSE, TokenImpersonation, &thread_token );
    ok( status == STATUS_SUCCESS, "NtDuplicateToken returned %08lx\n", status );
    NtClose( process_token );

    status = NtAdjustPrivilegesToken( thread_token, FALSE, &new_privs, sizeof(new_privs), NULL, NULL );
    ok( status == STATUS_SUCCESS || status == STATUS_NOT_ALL_ASSIGNED, "NtAdjustPrivilegesToken returned %08lx\n", status );
    creatpermapriv = (status == STATUS_SUCCESS);

    status = NtSetInformationThread( GetCurrentThread(), ThreadImpersonationToken, &thread_token, sizeof(thread_token) );
    ok( status == STATUS_SUCCESS, "NtSetInformationThread returned %08lx\n", status );
    NtClose( thread_token );

    if (!creatpermapriv) skip( "no privileges, tests may be limited\n" );

    for (test = &tests[0]; test != &tests[ARRAY_SIZE(tests)]; test++)
    {
        NTSTATUS make_perma_status = creatpermapriv ? STATUS_SUCCESS : STATUS_PRIVILEGE_NOT_HELD;
        HANDLE handle, handle2;
        OBJECT_BASIC_INFORMATION obi;
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING name;
        BOOL is_permanent;
        ULONG len = 0;

        winetest_push_context( "test#%Iu", test - &tests[0] );
        trace( "(%s)\n", test->name );

        RtlInitUnicodeString( &name, L"\\BaseNamedObjects\\test_object_permanence" );
        InitializeObjectAttributes( &attr, &name, test->initial_attr, 0, NULL );
        status = NtCreateEvent( &handle, test->access, &attr, NotificationEvent, FALSE );
        if (test->initial_attr & OBJ_PERMANENT)
        {
            todo_wine_if(status == STATUS_SUCCESS || status == STATUS_PRIVILEGE_NOT_HELD)
            ok( status == make_perma_status, "NtCreateEvent returned %08lx (expected %08lx)\n", status, make_perma_status );
        }
        else
        {
            ok( status == STATUS_SUCCESS, "NtCreateEvent returned %08lx\n", status );
        }
        if (NT_ERROR(status))
        {
            winetest_pop_context();
            continue;
        }
        is_permanent = (test->initial_attr & OBJ_PERMANENT) != 0;

        status = NtQueryObject( handle, ObjectBasicInformation, &obi, sizeof(obi), &len );
        ok( status == STATUS_SUCCESS, "NtQueryObject returned %08lx\n", status );
        todo_wine_if(test->initial_attr != 0)
        ok( obi.Attributes == test->initial_attr, "expected attr %08lx, got %08lx\n", test->initial_attr, obi.Attributes );

        if (test->make_temporary)
        {
            if (test->make_temp_status == STATUS_ACCESS_DENIED)
                ok( !(obi.GrantedAccess & DELETE), "expected no DELETE access in %08lx\n", obi.GrantedAccess );
            if (test->make_temp_status == STATUS_SUCCESS)
                ok( !!(obi.GrantedAccess & DELETE), "expected DELETE access in %08lx\n", obi.GrantedAccess );

            status = NtMakeTemporaryObject( handle );
            ok( status == test->make_temp_status, "NtMakeTemporaryObject returned %08lx\n", status );
            if (!NT_ERROR(status)) is_permanent = FALSE;
        }

        if (winetest_debug > 1)
            trace( "NOTE: object still has unclosed handle (%p) and shouldn't be deleted", handle );

        winetest_push_context( "first handle (%p) still open", handle );
        status = pNtOpenEvent( &handle2, GENERIC_ALL, &attr );
        ok( status == STATUS_SUCCESS, "NtOpenEvent returned %08lx\n", status );
        if (!NT_ERROR(status))
        {
            ULONG expect_attr = (obi.Attributes & ~OBJ_PERMANENT) | (is_permanent ? OBJ_PERMANENT : 0);
            OBJECT_BASIC_INFORMATION obi2;

            status = NtQueryObject( handle2, ObjectBasicInformation, &obi2, sizeof(obi2), &len );
            ok( status == STATUS_SUCCESS, "NtQueryObject returned %08lx\n", status );
            todo_wine_if(expect_attr != 0)
            ok( obi2.Attributes == expect_attr, "expected attr %08lx, got %08lx\n", expect_attr, obi2.Attributes );

            NtClose( handle2 );
        }
        winetest_pop_context();

        if (test->make_permanent)
        {
            status = NtMakePermanentObject( handle );
            todo_wine_if(status == STATUS_SUCCESS || status == STATUS_PRIVILEGE_NOT_HELD)
            ok( status == make_perma_status, "NtMakePermanentObject returned %08lx expected (%08lx)\n", status, make_perma_status );
            if (!NT_ERROR(status)) is_permanent = TRUE;
        }

        if (winetest_debug > 1)
            trace( "NOTE: about to close earlier handle (%p) which should be the last", handle );
        NtClose( handle );

        winetest_push_context( "first handle closed" );
        status = pNtOpenEvent( &handle, GENERIC_ALL, &attr );
        ok( status == (is_permanent ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND), "NtOpenEvent returned %08lx\n", status );
        if (!NT_ERROR(status))
        {
            ULONG expect_attr = (obi.Attributes & ~OBJ_PERMANENT) | (is_permanent ? OBJ_PERMANENT : 0);
            OBJECT_BASIC_INFORMATION obi_new;

            status = NtQueryObject( handle, ObjectBasicInformation, &obi_new, sizeof(obi_new), &len );
            ok( status == STATUS_SUCCESS, "NtQueryObject returned %08lx\n", status );
            todo_wine_if(expect_attr != 0)
            ok( obi_new.Attributes == expect_attr, "expected attr %08lx, got %08lx\n", expect_attr, obi_new.Attributes );

            /* ensure object is deleted */
            NtMakeTemporaryObject( handle );
            NtClose( handle );
        }
        winetest_pop_context();

        winetest_pop_context();
    }

    thread_token = NULL;
    status = NtSetInformationThread( GetCurrentThread(), ThreadImpersonationToken, &thread_token, sizeof(thread_token) );
    ok( status == STATUS_SUCCESS, "NtSetInformationThread returned %08lx\n", status );
}

static void test_zero_access(void)
{
    UNICODE_STRING target = RTL_CONSTANT_STRING( L"\\DosDevices" );
    OBJECT_BASIC_INFORMATION obj_info;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER size, timeout;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    NTSTATUS status, status2;
    WCHAR name[256];
    CLIENT_ID cid;
    HANDLE h1, h2;
    DWORD err;

#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_VISTA)
    {
        skip("Zero access tests don't work on Windows 2003\n");
        return;
    }
#endif

    size.QuadPart = 4096;
    timeout.QuadPart = -10000;
    swprintf( name, ARRAY_SIZE(name), L"\\Sessions\\%u\\BaseNamedObjects\\test_object", NtCurrentTeb()->Peb->SessionId );
    pRtlInitUnicodeString( &str, name );
    InitializeObjectAttributes( &attr, &str, OBJ_OPENIF, 0, NULL );

    status = pNtCreateEvent( &h1, 0, &attr, NotificationEvent, FALSE );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenEvent( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateEvent( &h1, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateEvent( &h2, EVENT_ALL_ACCESS, &attr, NotificationEvent, FALSE );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateEvent( &h2, 0, &attr, NotificationEvent, FALSE );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenEvent( &h2, EVENT_ALL_ACCESS, &attr );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtOpenEvent( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    InitializeObjectAttributes( &attr, &str, OBJ_INHERIT, 0, NULL );
    status = pNtOpenEvent( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );

    status = pNtDuplicateObject( GetCurrentProcess(), h1, GetCurrentProcess(), &h2, 0, 0, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtDuplicateObject( GetCurrentProcess(), h1, GetCurrentProcess(), &h2, EVENT_ALL_ACCESS, 0, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h2 );
    CloseHandle( h1 );

    InitializeObjectAttributes( &attr, &str, OBJ_OPENIF, 0, NULL );
    status = pNtCreateMutant( &h1, 0, &attr, FALSE );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenMutant( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateMutant( &h1, MUTANT_ALL_ACCESS, &attr, FALSE );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateMutant( &h2, MUTANT_ALL_ACCESS, &attr, FALSE );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateMutant( &h2, 0, &attr, FALSE );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenMutant( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateSemaphore( &h1, 0, &attr, 1, 2 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenSemaphore( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateSemaphore( &h1, SEMAPHORE_ALL_ACCESS, &attr, 1, 2 );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateSemaphore( &h2, SEMAPHORE_ALL_ACCESS, &attr, 1, 2 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateSemaphore( &h2, 0, &attr, 1, 2 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenSemaphore( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateKeyedEvent( &h1, 0, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenKeyedEvent( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateKeyedEvent( &h1, KEYEDEVENT_ALL_ACCESS, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateKeyedEvent( &h2, KEYEDEVENT_ALL_ACCESS, &attr, 0 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateKeyedEvent( &h2, 0, &attr, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenKeyedEvent( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateIoCompletion( &h1, 0, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenIoCompletion( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateIoCompletion( &h1, IO_COMPLETION_ALL_ACCESS, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    status = NtCreateIoCompletion( &h2, IO_COMPLETION_ALL_ACCESS, &attr, 0 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateIoCompletion( &h2, 0, &attr, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenIoCompletion( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateJobObject( &h1, 0, &attr );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenJobObject( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateJobObject( &h1, JOB_OBJECT_ALL_ACCESS, &attr );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateJobObject( &h2, JOB_OBJECT_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_EXISTS || broken( status == STATUS_ACCESS_DENIED), /* <= win10 1507 */
        "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtCreateJobObject( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenJobObject( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateDirectoryObject( &h1, 0, &attr );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenDirectoryObject( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateDirectoryObject( &h1, DIRECTORY_ALL_ACCESS, &attr );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateDirectoryObject( &h2, DIRECTORY_ALL_ACCESS, &attr );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateDirectoryObject( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenDirectoryObject( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateTimer( &h1, 0, &attr, NotificationTimer );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenTimer( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateTimer( &h1, TIMER_ALL_ACCESS, &attr, NotificationTimer );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateTimer( &h2, TIMER_ALL_ACCESS, &attr, NotificationTimer );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateTimer( &h2, 0, &attr, NotificationTimer );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenTimer( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateSymbolicLinkObject( &h1, 0, &attr, &target );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenSymbolicLinkObject( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateSymbolicLinkObject( &h1, SYMBOLIC_LINK_ALL_ACCESS, &attr, &target );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateSymbolicLinkObject( &h2, SYMBOLIC_LINK_ALL_ACCESS, &attr, &target );
    ok( !status || broken( status == STATUS_OBJECT_NAME_EXISTS ), /* <= win10 1507 */
        "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateSymbolicLinkObject( &h2, 0, &attr, &target );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenSymbolicLinkObject( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateSection( &h1, 0, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenSection( &h1, 0, &attr );
    ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "got %#lx.\n", status );
    status = pNtCreateSection( &h1, SECTION_ALL_ACCESS, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateSection( &h2, SECTION_ALL_ACCESS, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateSection( &h2, 0, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    status = pNtOpenSection( &h2, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    status = pNtCreateDebugObject( &h1, 0, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtCreateDebugObject( &h1, DEBUG_ALL_ACCESS, &attr, 0 );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateDebugObject( &h2, DEBUG_ALL_ACCESS, &attr, 0 );
    ok( status == STATUS_OBJECT_NAME_EXISTS, "got %#lx.\n", status );
    CloseHandle( h2 );
    status = pNtCreateDebugObject( &h2, 0, &attr, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\Device\\Null" );
    status = NtCreateFile( &h1, GENERIC_READ | SYNCHRONIZE, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = NtCreateFile( &h1, 0, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\??\\c:\\windows\\system32\\ntdll.dll" );
    status = NtCreateFile( &h1, GENERIC_READ | SYNCHRONIZE, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = NtCreateFile( &h1, 0, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, 0, NULL, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\Device\\MailSlot\\abc" );
    status = pNtCreateMailslotFile( &h1, 0, &attr, &iosb, 0, 0, 0, NULL );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );
    status = pNtCreateMailslotFile( &h1, GENERIC_ALL, &attr, &iosb, 0, 0, 0, NULL );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateMailslotFile( &h2, GENERIC_READ, &attr, &iosb, 0, 0, 0, NULL );
    ok( status == STATUS_OBJECT_NAME_COLLISION, "got %#lx.\n", status );
    status = pNtCreateMailslotFile( &h2, 0, &attr, &iosb, 0, 0, 0, NULL );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\??\\PIPE\\test\\pipe" );
    status = pNtCreateNamedPipeFile( &h1, 0, &attr, &iosb,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE,
                                     FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );
    status = pNtCreateNamedPipeFile( &h1, GENERIC_READ | GENERIC_WRITE, &attr, &iosb,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE,
                                     FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout );
    ok( !status, "got %#lx.\n", status );
    status = pNtCreateNamedPipeFile( &h2, GENERIC_READ, &attr, &iosb,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                     FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout );
    ok( status == STATUS_INSTANCE_NOT_AVAILABLE, "got %#lx.\n", status );
    status = pNtCreateNamedPipeFile( &h2, 0, &attr, &iosb,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN,
                                     FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h2 );
    CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\REGISTRY\\Machine" );
    status = pNtCreateKey( &h1, 0, &attr, 0, 0, 0, 0 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );
    status = pNtCreateKey( &h1, KEY_READ, &attr, 0, 0, 0, 0 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenKey( &h1, KEY_READ, &attr );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenKey( &h1, 0, &attr );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

    pRtlInitUnicodeString( &str, L"\\REGISTRY\\Machine\\Software\\foobar-test" );
    status = pNtCreateKey( &h1, KEY_ALL_ACCESS, &attr, 0, 0, 0, 0 );
    ok( !status || status == STATUS_ACCESS_DENIED || broken(status == STATUS_OBJECT_PATH_NOT_FOUND), /* win7 */
        "got %#lx.\n", status );
    if (!status)
    {
        pNtDeleteKey( h1 );
        CloseHandle( h1 );
    }
    status2 = pNtCreateKey( &h1, 0, &attr, 0, 0, 0, 0 );
    ok( status2 == status, "got %#lx / %#lx.\n", status2, status );
    if (!status2)
    {
        pNtDeleteKey( h1 );
        CloseHandle( h1 );
    }

    status = pNtOpenProcessToken( GetCurrentProcess(), TOKEN_ALL_ACCESS, &h1 );
    ok( !status, "got %#lx.\n", status );
    CloseHandle( h1 );
    status = pNtOpenProcessToken( GetCurrentProcess(), 0, &h1 );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

#if !defined(__REACTOS__) || DLL_EXPORT_VERSION >= 0x600
    status = NtGetNextThread(GetCurrentProcess(), NULL, 0, 0, 0, &h1);
    todo_wine ok( status == STATUS_NO_MORE_ENTRIES, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );
#endif

    InitializeObjectAttributes( &attr, NULL, 0, 0, NULL );
    cid.UniqueProcess = ULongToHandle( GetCurrentProcessId() );
    cid.UniqueThread  = 0;
    status = pNtOpenProcess( &h1, 0, &attr, &cid );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

    InitializeObjectAttributes( &attr, NULL, 0, 0, NULL );
    cid.UniqueProcess = 0;
    cid.UniqueThread  = ULongToHandle( GetCurrentThreadId() );
    status = pNtOpenThread( &h1, 0, &attr, &cid );
    todo_wine ok( status == STATUS_ACCESS_DENIED, "got %#lx.\n", status );
    if (NT_SUCCESS(status)) CloseHandle( h1 );

    h1 = CreateWindowStationA( "WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
    err = GetLastError();
    if (h1)
    {
        CloseWindowStation( h1 );
        h1 = CreateWindowStationA( "WinSta0", 0, WINSTA_ALL_ACCESS, NULL );
        ok( h1 != 0, "CreateWindowStationA failed err %lu\n", GetLastError());
        CloseWindowStation( h1 );
        h1 = CreateWindowStationA( "WinSta0", 0, 0, NULL );
        err = GetLastError();
        todo_wine ok( !h1, "CreateWindowStationA succeeded\n" );
        todo_wine ok( err == ERROR_ACCESS_DENIED, "CreateWindowStationA failed %lu\n", err );
        if (h1) CloseWindowStation( h1 );
    }
    else if (err == ERROR_ACCESS_DENIED) win_skip( "Not enough privileges for CreateWindowStation\n" );

    h1 = OpenWindowStationA( "WinSta0", TRUE, WINSTA_ALL_ACCESS );
    ok( h1 != 0, "OpenWindowStation failed\n" );
    CloseWindowStation( h1 );
    h1 = OpenWindowStationA( "WinSta0", TRUE, 0 );
    err = GetLastError();
    todo_wine ok( !h1, "OpenWindowStation succeeeded\n" );
    todo_wine ok( err == ERROR_ACCESS_DENIED, "OpenWindowStation failed %lu\n", err );
    if (h1) CloseWindowStation( h1 );

    h1 = CreateDesktopA( "default", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
    ok( h1 != 0, "CreateDesktopA failed %lu\n", GetLastError() );
    CloseDesktop( h1 );
    SetLastError( 0xdeadbeef );
    h1 = CreateDesktopA( "default", NULL, NULL, 0, 0, NULL );
    err = GetLastError();
    todo_wine ok( !h1, "CreateDesktopA succeeded\n" );
    todo_wine ok( err == ERROR_ACCESS_DENIED, "CreateDesktopA failed %lu\n", err );
    if (h1) CloseDesktop( h1 );

    h1 = OpenDesktopA( "default", 0, TRUE, DESKTOP_ALL_ACCESS );
    ok( h1 != 0, "OpenDesktopA failed %lu\n", GetLastError() );
    CloseDesktop( h1 );
    h1 = OpenDesktopA( "default", 0, TRUE, 0 );
    ok( h1 != 0, "OpenDesktopA failed %lu\n", GetLastError() );
    NtQueryObject( h1, ObjectBasicInformation, &obj_info, sizeof(obj_info), NULL );
    ok( obj_info.GrantedAccess == (DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS),
        "wrong access %lx\n", obj_info.GrantedAccess );
    CloseDesktop( h1 );
    h1 = OpenDesktopA( "default", 0, TRUE, DESKTOP_CREATEWINDOW );
    ok( h1 != 0, "OpenDesktopA failed %lu\n", GetLastError() );
    NtQueryObject( h1, ObjectBasicInformation, &obj_info, sizeof(obj_info), NULL );
    ok( obj_info.GrantedAccess == (DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_CREATEWINDOW),
        "wrong access %lx\n", obj_info.GrantedAccess );
    CloseDesktop( h1 );

    h1 = OpenInputDesktop( 0, FALSE, DESKTOP_ALL_ACCESS );
    ok( h1 != 0, "OpenInputDesktop failed %lu\n", GetLastError() );
    CloseDesktop( h1 );
    h1 = OpenInputDesktop( 0, FALSE, 0 );
    ok( h1 != 0, "OpenInputDesktop failed %lu\n", GetLastError() );
    NtQueryObject( h1, ObjectBasicInformation, &obj_info, sizeof(obj_info), NULL );
    ok( obj_info.GrantedAccess == (DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS),
        "wrong access %lx\n", obj_info.GrantedAccess );
    CloseDesktop( h1 );
    h1 = OpenInputDesktop( 0, FALSE, DESKTOP_ENUMERATE );
    ok( h1 != 0, "OpenInputDesktop failed %lu\n", GetLastError() );
    NtQueryObject( h1, ObjectBasicInformation, &obj_info, sizeof(obj_info), NULL );
    ok( obj_info.GrantedAccess == (DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE),
        "wrong access %lx\n", obj_info.GrantedAccess );
    CloseDesktop( h1 );
}

static void test_NtAllocateReserveObject(void)
{
    UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\test_NtAllocateReserveObject_name");
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE handle;
    int i;

    static const struct
    {
        MEMORY_RESERVE_OBJECT_TYPE type;
        const WCHAR *type_name;
    }
    tests[] =
    {
        {MemoryReserveObjectTypeUserApc, L"UserApcReserve"},
        {MemoryReserveObjectTypeIoCompletion, L"IoCompletionReserve"},
    };

    if (!pNtAllocateReserveObject)
    {
        win_skip("NtAllocateReserveObject is unavailable.\n");
        return;
    }

    InitializeObjectAttributes(&attr, &name, 0, NULL, NULL);

    /* Parameter checks */
    status = pNtAllocateReserveObject(NULL, &attr, MemoryReserveObjectTypeUserApc);
    ok(status == STATUS_ACCESS_VIOLATION, "Got unexpected status %#lx.\n", status);

    status = pNtAllocateReserveObject(&handle, NULL, MemoryReserveObjectTypeUserApc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = NtClose(handle);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    status = pNtAllocateReserveObject(&handle, NULL, MemoryReserveObjectTypeIoCompletion + 1);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status);

    status = pNtAllocateReserveObject(&handle, &attr, MemoryReserveObjectTypeUserApc);
    ok(status == STATUS_OBJECT_NAME_INVALID, "Got unexpected status %#lx.\n", status);

    attr.ObjectName = NULL;
    status = pNtAllocateReserveObject(&handle, &attr, MemoryReserveObjectTypeUserApc);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    status = NtClose(handle);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

    /* Test creating objects */
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("type %d", tests[i].type);

        status = pNtAllocateReserveObject(&handle, NULL, tests[i].type);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);

        test_object_type(handle, tests[i].type_name);

        status = NtClose(handle);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        winetest_pop_context();
    }
}

START_TEST(om)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");

    pNtAllocateReserveObject= (void *)GetProcAddress(hntdll, "NtAllocateReserveObject");
    pNtCreateEvent          = (void *)GetProcAddress(hntdll, "NtCreateEvent");
    pNtCreateJobObject      = (void *)GetProcAddress(hntdll, "NtCreateJobObject");
    pNtOpenJobObject        = (void *)GetProcAddress(hntdll, "NtOpenJobObject");
    pNtCreateKey            = (void *)GetProcAddress(hntdll, "NtCreateKey");
    pNtOpenKey              = (void *)GetProcAddress(hntdll, "NtOpenKey");
    pNtDeleteKey            = (void *)GetProcAddress(hntdll, "NtDeleteKey");
    pNtCreateMailslotFile   = (void *)GetProcAddress(hntdll, "NtCreateMailslotFile");
    pNtCreateMutant         = (void *)GetProcAddress(hntdll, "NtCreateMutant");
    pNtOpenEvent            = (void *)GetProcAddress(hntdll, "NtOpenEvent");
    pNtOpenMutant           = (void *)GetProcAddress(hntdll, "NtOpenMutant");
    pNtOpenFile             = (void *)GetProcAddress(hntdll, "NtOpenFile");
    pNtClose                = (void *)GetProcAddress(hntdll, "NtClose");
    pRtlInitUnicodeString   = (void *)GetProcAddress(hntdll, "RtlInitUnicodeString");
    pNtCreateNamedPipeFile  = (void *)GetProcAddress(hntdll, "NtCreateNamedPipeFile");
    pNtOpenDirectoryObject  = (void *)GetProcAddress(hntdll, "NtOpenDirectoryObject");
    pNtCreateDirectoryObject= (void *)GetProcAddress(hntdll, "NtCreateDirectoryObject");
    pNtOpenSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtOpenSymbolicLinkObject");
    pNtCreateSymbolicLinkObject = (void *)GetProcAddress(hntdll, "NtCreateSymbolicLinkObject");
    pNtQuerySymbolicLinkObject  = (void *)GetProcAddress(hntdll, "NtQuerySymbolicLinkObject");
    pNtCreateSemaphore      =  (void *)GetProcAddress(hntdll, "NtCreateSemaphore");
    pNtOpenSemaphore        =  (void *)GetProcAddress(hntdll, "NtOpenSemaphore");
    pNtCreateTimer          =  (void *)GetProcAddress(hntdll, "NtCreateTimer");
    pNtOpenTimer            =  (void *)GetProcAddress(hntdll, "NtOpenTimer");
    pNtCreateSection        =  (void *)GetProcAddress(hntdll, "NtCreateSection");
    pNtOpenSection          =  (void *)GetProcAddress(hntdll, "NtOpenSection");
    pNtQueryObject          =  (void *)GetProcAddress(hntdll, "NtQueryObject");
    pNtReleaseSemaphore     =  (void *)GetProcAddress(hntdll, "NtReleaseSemaphore");
    pNtCreateKeyedEvent     =  (void *)GetProcAddress(hntdll, "NtCreateKeyedEvent");
    pNtOpenKeyedEvent       =  (void *)GetProcAddress(hntdll, "NtOpenKeyedEvent");
    pNtCreateIoCompletion   =  (void *)GetProcAddress(hntdll, "NtCreateIoCompletion");
    pNtOpenIoCompletion     =  (void *)GetProcAddress(hntdll, "NtOpenIoCompletion");
    pNtQueryInformationFile =  (void *)GetProcAddress(hntdll, "NtQueryInformationFile");
    pNtOpenProcess          =  (void *)GetProcAddress(hntdll, "NtOpenProcess");
    pNtCreateDebugObject    =  (void *)GetProcAddress(hntdll, "NtCreateDebugObject");
    pNtGetNextThread        =  (void *)GetProcAddress(hntdll, "NtGetNextThread");
    pNtOpenProcessToken     =  (void *)GetProcAddress(hntdll, "NtOpenProcessToken");
    pNtOpenThreadToken      =  (void *)GetProcAddress(hntdll, "NtOpenThreadToken");
    pNtDuplicateToken       =  (void *)GetProcAddress(hntdll, "NtDuplicateToken");
    pNtDuplicateObject      =  (void *)GetProcAddress(hntdll, "NtDuplicateObject");
    pNtCompareObjects       =  (void *)GetProcAddress(hntdll, "NtCompareObjects");
    pNtOpenThread           =  (void *)GetProcAddress(hntdll, "NtOpenThread");

#ifdef __REACTOS__
    if (GetNTVersion() < _WIN32_WINNT_VISTA)
        win_skip("test_null_in_object_name() doesn't work on Windows 2003\n");
    else
#endif
    test_null_in_object_name();
    test_case_sensitive();
    test_namespace_pipe();
    test_name_collisions();
    test_name_limits();
    test_directory();
    test_symboliclink();
    test_query_object();
    test_type_mismatch();
    test_null_device();
    test_process();
    test_token();
    test_duplicate_object();
    test_object_types();
    test_get_next_thread();
    test_globalroot();
    test_object_identity();
    test_query_directory();
    test_object_permanence();
    test_zero_access();
    test_NtAllocateReserveObject();
}
