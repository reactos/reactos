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

#include "ntdll_test.h"
#include "winternl.h"
#include "stdio.h"
#include "winnt.h"
#include "stdlib.h"

static HANDLE   (WINAPI *pCreateWaitableTimerA)(SECURITY_ATTRIBUTES*, BOOL, LPCSTR);
static BOOLEAN  (WINAPI *pRtlCreateUnicodeStringFromAsciiz)(PUNICODE_STRING, LPCSTR);
static VOID     (WINAPI *pRtlInitUnicodeString)( PUNICODE_STRING, LPCWSTR );
static VOID     (WINAPI *pRtlFreeUnicodeString)(PUNICODE_STRING);
static NTSTATUS (WINAPI *pNtCreateEvent) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN, BOOLEAN);
static NTSTATUS (WINAPI *pNtOpenEvent)   ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES);
static NTSTATUS (WINAPI *pNtPulseEvent)  ( HANDLE, PULONG );
static NTSTATUS (WINAPI *pNtQueryEvent)  ( HANDLE, EVENT_INFORMATION_CLASS, PVOID, ULONG, PULONG );
static NTSTATUS (WINAPI *pNtCreateMutant)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, BOOLEAN );
static NTSTATUS (WINAPI *pNtOpenMutant)  ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES );
static NTSTATUS (WINAPI *pNtCreateSemaphore)( PHANDLE, ACCESS_MASK,const POBJECT_ATTRIBUTES,LONG,LONG );
static NTSTATUS (WINAPI *pNtCreateTimer) ( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, TIMER_TYPE );
static NTSTATUS (WINAPI *pNtCreateSection)( PHANDLE, ACCESS_MASK, const POBJECT_ATTRIBUTES, const PLARGE_INTEGER,
                                            ULONG, ULONG, HANDLE );
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
static NTSTATUS (WINAPI *pNtWaitForKeyedEvent)( HANDLE, const void *, BOOLEAN, const LARGE_INTEGER * );
static NTSTATUS (WINAPI *pNtReleaseKeyedEvent)( HANDLE, const void *, BOOLEAN, const LARGE_INTEGER * );

#define KEYEDEVENT_WAIT       0x0001
#define KEYEDEVENT_WAKE       0x0002
#define KEYEDEVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x0003)

static void test_case_sensitive (void)
{
    static const WCHAR buffer1[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t',0};
    static const WCHAR buffer2[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','T','e','s','t',0};
    static const WCHAR buffer3[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','T','E','s','t',0};
    static const WCHAR buffer4[] = {'\\','B','A','S','E','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t',0};
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE Event, Mutant, h;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08x)\n", status);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION or STATUS_OBJECT_TYPE_MISMATCH got (%08x)\n", status);

    pRtlInitUnicodeString(&str, buffer2);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Event(%08x)\n", status);

    pRtlInitUnicodeString(&str, buffer3);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenMutant(&h, GENERIC_ALL, &attr);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtOpenMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);

    pNtClose(Mutant);

    pRtlInitUnicodeString(&str, buffer4);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_NAME_COLLISION or STATUS_OBJECT_TYPE_MISMATCH got (%08x)\n", status);

    status = pNtCreateEvent(&h, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_COLLISION,
        "NtCreateEvent should have failed with STATUS_OBJECT_NAME_COLLISION got(%08x)\n", status);

    attr.Attributes = 0;
    status = pNtCreateMutant(&Mutant, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);

    pNtClose(Event);
}

static void test_namespace_pipe(void)
{
    static const WCHAR buffer1[] = {'\\','?','?','\\','P','I','P','E','\\','t','e','s','t','\\','p','i','p','e',0};
    static const WCHAR buffer2[] = {'\\','?','?','\\','P','I','P','E','\\','T','E','S','T','\\','P','I','P','E',0};
    static const WCHAR buffer3[] = {'\\','?','?','\\','p','i','p','e','\\','t','e','s','t','\\','p','i','p','e',0};
    static const WCHAR buffer4[] = {'\\','?','?','\\','p','i','p','e','\\','t','e','s','t',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    HANDLE pipe, h;

    timeout.QuadPart = -10000;

    pRtlInitUnicodeString(&str, buffer1);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_SUCCESS, "Failed to create NamedPipe(%08x)\n", status);

    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08x)\n", status);

    pRtlInitUnicodeString(&str, buffer2);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtCreateNamedPipeFile(&pipe, GENERIC_READ|GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    FILE_CREATE, FILE_PIPE_FULL_DUPLEX, FALSE, FALSE, FALSE, 1, 256, 256, &timeout);
    ok(status == STATUS_INSTANCE_NOT_AVAILABLE,
        "NtCreateNamedPipeFile should have failed with STATUS_INSTANCE_NOT_AVAILABLE got(%08x)\n", status);

    h = CreateFileA("\\\\.\\pipe\\test\\pipe", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, 0, 0 );
    ok(h != INVALID_HANDLE_VALUE, "Failed to open NamedPipe (%u)\n", GetLastError());
    pNtClose(h);

    pRtlInitUnicodeString(&str, buffer3);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND ||
       status == STATUS_PIPE_NOT_AVAILABLE ||
       status == STATUS_OBJECT_NAME_INVALID || /* vista */
       status == STATUS_OBJECT_NAME_NOT_FOUND, /* win8 */
        "NtOpenFile should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);

    pRtlInitUnicodeString(&str, buffer4);
    InitializeObjectAttributes(&attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL);
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    ok(status == STATUS_OBJECT_NAME_NOT_FOUND ||
       status == STATUS_OBJECT_NAME_INVALID, /* vista */
        "NtOpenFile should have failed with STATUS_OBJECT_NAME_NOT_FOUND got(%08x)\n", status);

    pNtClose(pipe);
}

#define DIRECTORY_QUERY (0x0001)
#define SYMBOLIC_LINK_QUERY 0x0001

#define DIR_TEST_CREATE_FAILURE(h,e) \
    status = pNtCreateDirectoryObject(h, DIRECTORY_QUERY, &attr);\
    ok(status == e,"NtCreateDirectoryObject should have failed with %s got(%08x)\n", #e, status);
#define DIR_TEST_OPEN_FAILURE(h,e) \
    status = pNtOpenDirectoryObject(h, DIRECTORY_QUERY, &attr);\
    ok(status == e,"NtOpenDirectoryObject should have failed with %s got(%08x)\n", #e, status);
#define DIR_TEST_CREATE_OPEN_FAILURE(h,n,e) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    DIR_TEST_CREATE_FAILURE(h,e) DIR_TEST_OPEN_FAILURE(h,e)\
    pRtlFreeUnicodeString(&str);

#define DIR_TEST_CREATE_SUCCESS(h) \
    status = pNtCreateDirectoryObject(h, DIRECTORY_QUERY, &attr); \
    ok(status == STATUS_SUCCESS, "Failed to create Directory(%08x)\n", status);
#define DIR_TEST_OPEN_SUCCESS(h) \
    status = pNtOpenDirectoryObject(h, DIRECTORY_QUERY, &attr); \
    ok(status == STATUS_SUCCESS, "Failed to open Directory(%08x)\n", status);
#define DIR_TEST_CREATE_OPEN_SUCCESS(h,n) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    DIR_TEST_CREATE_SUCCESS(&h) pNtClose(h); DIR_TEST_OPEN_SUCCESS(&h) pNtClose(h); \
    pRtlFreeUnicodeString(&str);

static BOOL is_correct_dir( HANDLE dir, const char *name )
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE h = 0;

    pRtlCreateUnicodeStringFromAsciiz(&str, name);
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    pRtlFreeUnicodeString(&str);
    if (h) pNtClose( h );
    return (status == STATUS_OBJECT_NAME_EXISTS);
}

/* return a handle to the BaseNamedObjects dir where kernel32 objects get created */
static HANDLE get_base_dir(void)
{
    static const char objname[] = "om.c_get_base_dir_obj";
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, h;
    unsigned int i;

    h = CreateMutexA(NULL, FALSE, objname);
    ok(h != 0, "CreateMutexA failed got ret=%p (%d)\n", h, GetLastError());
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Local");
    status = pNtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
    pRtlFreeUnicodeString(&str);
    if (!status && is_correct_dir( dir, objname )) goto done;
    if (!status) pNtClose( dir );

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    status = pNtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
    pRtlFreeUnicodeString(&str);
    if (!status && is_correct_dir( dir, objname )) goto done;
    if (!status) pNtClose( dir );

    for (i = 0; i < 20; i++)
    {
        char name[40];
        sprintf( name, "\\BaseNamedObjects\\Session\\%u", i );
        pRtlCreateUnicodeStringFromAsciiz(&str, name );
        status = pNtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr);
        pRtlFreeUnicodeString(&str);
        if (!status && is_correct_dir( dir, objname )) goto done;
        if (!status) pNtClose( dir );
    }
    dir = 0;

done:
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
    LARGE_INTEGER size;

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_NAME_COLLISION)
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, 0, NULL);

    DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_NAME_EXISTS)
    pNtClose(h);
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\??\\PIPE\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH || status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);

    if (!(dir = get_base_dir()))
    {
        win_skip( "couldn't find the BaseNamedObjects dir\n" );
        return;
    }
    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-test");
    InitializeObjectAttributes(&attr, &str, OBJ_OPENIF, dir, NULL);
    h = CreateMutexA(NULL, FALSE, "om.c-test");
    ok(h != 0, "CreateMutexA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateMutant(&h1, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateMutant should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateMutexA(NULL, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateMutexA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    ok(h != 0, "CreateEventA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateEvent(&h1, GENERIC_ALL, &attr, FALSE, FALSE);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateEvent should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateEventA(NULL, FALSE, FALSE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateEventA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    ok(h != 0, "CreateSemaphoreA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateSemaphore(&h1, GENERIC_ALL, &attr, 1, 2);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSemaphore should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateSemaphoreA(NULL, 1, 2, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateSemaphoreA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);
    
    h = pCreateWaitableTimerA(NULL, TRUE, "om.c-test");
    ok(h != 0, "CreateWaitableTimerA failed got ret=%p (%d)\n", h, GetLastError());
    status = pNtCreateTimer(&h1, GENERIC_ALL, &attr, NotificationTimer);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateTimer should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = pCreateWaitableTimerA(NULL, TRUE, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateWaitableTimerA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    ok(h != 0, "CreateFileMappingA failed got ret=%p (%d)\n", h, GetLastError());
    size.u.LowPart = 256;
    size.u.HighPart = 0;
    status = pNtCreateSection(&h1, SECTION_MAP_WRITE, &attr, &size, PAGE_READWRITE, SEC_COMMIT, 0);
    ok(status == STATUS_OBJECT_NAME_EXISTS && h1 != NULL,
        "NtCreateSection should have succeeded with STATUS_OBJECT_NAME_EXISTS got(%08x)\n", status);
    h2 = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "om.c-test");
    winerr = GetLastError();
    ok(h2 != 0 && winerr == ERROR_ALREADY_EXISTS,
        "CreateFileMappingA should have succeeded with ERROR_ALREADY_EXISTS got ret=%p (%d)\n", h2, winerr);
    pNtClose(h);
    pNtClose(h1);
    pNtClose(h2);

    pRtlFreeUnicodeString(&str);
    pNtClose(dir);
}

static void test_directory(void)
{
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, dir1, h;
    BOOL is_nt4;

    /* No name and/or no attributes */
    status = pNtCreateDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
        "NtCreateDirectoryObject should have failed with STATUS_ACCESS_VIOLATION got(%08x)\n", status);
    status = pNtOpenDirectoryObject(NULL, DIRECTORY_QUERY, &attr);
    ok(status == STATUS_ACCESS_VIOLATION || status == STATUS_INVALID_PARAMETER,
        "NtOpenDirectoryObject should have failed with STATUS_ACCESS_VIOLATION got(%08x)\n", status);

    status = pNtCreateDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_SUCCESS, "Failed to create Directory without attributes(%08x)\n", status);
    pNtClose(h);
    status = pNtOpenDirectoryObject(&h, DIRECTORY_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenDirectoryObject should have failed with STATUS_INVALID_PARAMETER got(%08x)\n", status);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    DIR_TEST_CREATE_SUCCESS(&dir)
    DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD)

    /* Bad name */
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    pRtlCreateUnicodeStringFromAsciiz(&str, "");
    DIR_TEST_CREATE_SUCCESS(&h)
    pNtClose(h);
    DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD)
    pRtlFreeUnicodeString(&str);
    pNtClose(dir);

    DIR_TEST_CREATE_OPEN_FAILURE(&h, "BaseNamedObjects", STATUS_OBJECT_PATH_SYNTAX_BAD)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\", STATUS_OBJECT_NAME_INVALID)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\\\BaseNamedObjects", STATUS_OBJECT_NAME_INVALID)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\\\om.c-test", STATUS_OBJECT_NAME_INVALID)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND)

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test");
    DIR_TEST_CREATE_SUCCESS(&h)
    DIR_TEST_OPEN_SUCCESS(&dir1)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    pNtClose(dir1);


    /* Use of root directory */

    /* Can't use symlinks as a directory */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Local");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    status = pNtOpenSymbolicLinkObject(&dir, SYMBOLIC_LINK_QUERY, &attr);
    is_nt4 = (status == STATUS_OBJECT_NAME_NOT_FOUND);  /* nt4 doesn't have Local\\ symlink */
    if (!is_nt4)
    {
        WCHAR buffer[256];
        ULONG len, full_len;

        ok(status == STATUS_SUCCESS, "Failed to open SymbolicLink(%08x)\n", status);
        pRtlFreeUnicodeString(&str);
        InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
        pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
        DIR_TEST_CREATE_FAILURE(&h, STATUS_OBJECT_TYPE_MISMATCH)
        pRtlFreeUnicodeString(&str);

        str.Buffer = buffer;
        str.MaximumLength = sizeof(buffer);
        len = 0xdeadbeef;
        memset( buffer, 0xaa, sizeof(buffer) );
        status = pNtQuerySymbolicLinkObject( dir, &str, &len );
        ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08x\n", status );
        if (status != STATUS_SUCCESS)
            goto error;
        full_len = str.Length + sizeof(WCHAR);
        ok( len == full_len, "bad length %u/%u\n", len, full_len );
        if (len == full_len)
            ok( buffer[len / sizeof(WCHAR) - 1] == 0, "no terminating null\n" );

        str.MaximumLength = str.Length;
        len = 0xdeadbeef;
        status = pNtQuerySymbolicLinkObject( dir, &str, &len );
        ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08x\n", status );
        ok( len == full_len, "bad length %u/%u\n", len, full_len );

        str.MaximumLength = 0;
        len = 0xdeadbeef;
        status = pNtQuerySymbolicLinkObject( dir, &str, &len );
        ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySymbolicLinkObject failed %08x\n", status );
        ok( len == full_len, "bad length %u/%u\n", len, full_len );

        str.MaximumLength = str.Length + sizeof(WCHAR);
        len = 0xdeadbeef;
        status = pNtQuerySymbolicLinkObject( dir, &str, &len );
        ok( status == STATUS_SUCCESS, "NtQuerySymbolicLinkObject failed %08x\n", status );
        ok( len == full_len, "bad length %u/%u\n", len, full_len );

error:
        pNtClose(dir);
    }

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);

    InitializeObjectAttributes(&attr, NULL, 0, dir, NULL);
    DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_NAME_INVALID)

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    DIR_TEST_CREATE_OPEN_SUCCESS(h, "")
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\", STATUS_OBJECT_PATH_SYNTAX_BAD)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\om.c-test", STATUS_OBJECT_PATH_SYNTAX_BAD)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "\\om.c-test\\", STATUS_OBJECT_PATH_SYNTAX_BAD)
    DIR_TEST_CREATE_OPEN_FAILURE(&h, "om.c-test\\", STATUS_OBJECT_PATH_NOT_FOUND)

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-test");
    DIR_TEST_CREATE_SUCCESS(&dir1)
    DIR_TEST_OPEN_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);

    pNtClose(h);
    pNtClose(dir1);
    pNtClose(dir);

    /* Nested directories */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    DIR_TEST_OPEN_SUCCESS(&dir)
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    DIR_TEST_OPEN_FAILURE(&h, STATUS_OBJECT_PATH_SYNTAX_BAD)
    pRtlFreeUnicodeString(&str);
    pNtClose(dir);

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test");
    DIR_TEST_CREATE_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\om.c-test\\one more level");
    DIR_TEST_CREATE_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
    DIR_TEST_CREATE_SUCCESS(&h)
    pRtlFreeUnicodeString(&str);
    pNtClose(h);

    pNtClose(dir);

    if (!is_nt4)
    {
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Global\\om.c-test");
        DIR_TEST_CREATE_SUCCESS(&dir)
        pRtlFreeUnicodeString(&str);
        pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects\\Local\\om.c-test\\one more level");
        DIR_TEST_CREATE_SUCCESS(&h)
        pRtlFreeUnicodeString(&str);
        pNtClose(h);
        InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
        pRtlCreateUnicodeStringFromAsciiz(&str, "one more level");
        DIR_TEST_CREATE_SUCCESS(&dir)
        pRtlFreeUnicodeString(&str);
        pNtClose(h);
        pNtClose(dir);
    }

    /* Create other objects using RootDirectory */

    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\BaseNamedObjects");
    DIR_TEST_OPEN_SUCCESS(&dir)
    pRtlFreeUnicodeString(&str);
    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);

    /* Test invalid paths */
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);
    pRtlCreateUnicodeStringFromAsciiz(&str, "\\om.c-mutant\\");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c\\-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_OBJECT_PATH_NOT_FOUND,
        "NtCreateMutant should have failed with STATUS_OBJECT_PATH_NOT_FOUND got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "om.c-mutant");
    status = pNtCreateMutant(&h, GENERIC_ALL, &attr, FALSE);
    ok(status == STATUS_SUCCESS, "Failed to create Mutant(%08x)\n", status);
    pRtlFreeUnicodeString(&str);
    pNtClose(h);

    pNtClose(dir);
}

#define SYMLNK_TEST_CREATE_OPEN_FAILURE2(h,n,t,e,e2) \
    pRtlCreateUnicodeStringFromAsciiz(&str, n);\
    pRtlCreateUnicodeStringFromAsciiz(&target, t);\
    status = pNtCreateSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr, &target);\
    ok(status == e || status == e2, \
       "NtCreateSymbolicLinkObject should have failed with %s or %s got(%08x)\n", #e, #e2, status);\
    status = pNtOpenSymbolicLinkObject(h, SYMBOLIC_LINK_QUERY, &attr);\
    ok(status == e || status == e2, \
       "NtOpenSymbolicLinkObject should have failed with %s or %s got(%08x)\n", #e, #e2, status);\
    pRtlFreeUnicodeString(&target);\
    pRtlFreeUnicodeString(&str);

#define SYMLNK_TEST_CREATE_OPEN_FAILURE(h,n,t,e) SYMLNK_TEST_CREATE_OPEN_FAILURE2(h,n,t,e,e)

static void test_symboliclink(void)
{
    NTSTATUS status;
    UNICODE_STRING str, target;
    OBJECT_ATTRIBUTES attr;
    HANDLE dir, link, h;
    IO_STATUS_BLOCK iosb;

    /* No name and/or no attributes */
    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    SYMLNK_TEST_CREATE_OPEN_FAILURE2(NULL, "", "", STATUS_ACCESS_VIOLATION, STATUS_INVALID_PARAMETER)

    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, NULL);
    ok(status == STATUS_ACCESS_VIOLATION,
        "NtCreateSymbolicLinkObject should have failed with STATUS_ACCESS_VIOLATION got(%08x)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL);
    ok(status == STATUS_INVALID_PARAMETER,
        "NtOpenSymbolicLinkObject should have failed with STATUS_INVALID_PARAMETER got(%08x)\n", status);

    /* No attributes */
    pRtlCreateUnicodeStringFromAsciiz(&target, "\\DosDevices");
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, NULL, &target);
    ok(status == STATUS_SUCCESS || status == STATUS_ACCESS_VIOLATION, /* nt4 */
       "NtCreateSymbolicLinkObject failed(%08x)\n", status);
    pRtlFreeUnicodeString(&target);
    if (!status) pNtClose(h);

    InitializeObjectAttributes(&attr, NULL, 0, 0, NULL);
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_INVALID_PARAMETER ||
       broken(status == STATUS_SUCCESS),  /* nt4 */
       "NtCreateSymbolicLinkObject should have failed with STATUS_INVALID_PARAMETER got(%08x)\n", status);
    if (!status) pNtClose(h);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);

    /* Bad name */
    pRtlCreateUnicodeStringFromAsciiz(&target, "anywhere");
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    pRtlCreateUnicodeStringFromAsciiz(&str, "");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08x)\n", status);
    status = pNtOpenSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr);
    ok(status == STATUS_OBJECT_PATH_SYNTAX_BAD,
       "NtOpenSymbolicLinkObject should have failed with STATUS_OBJECT_PATH_SYNTAX_BAD got(%08x)\n", status);
    pNtClose(link);
    pRtlFreeUnicodeString(&str);

    pRtlCreateUnicodeStringFromAsciiz(&str, "\\");
    status = pNtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_QUERY, &attr, &target);
    todo_wine ok(status == STATUS_OBJECT_TYPE_MISMATCH,
                 "NtCreateSymbolicLinkObject should have failed with STATUS_OBJECT_TYPE_MISMATCH got(%08x)\n", status);
    pRtlFreeUnicodeString(&str);
    pRtlFreeUnicodeString(&target);

    SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "BaseNamedObjects", "->Somewhere", STATUS_OBJECT_PATH_SYNTAX_BAD)
    SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\", "->Somewhere", STATUS_OBJECT_NAME_INVALID)
    SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\\\BaseNamedObjects", "->Somewhere", STATUS_OBJECT_NAME_INVALID)
    SYMLNK_TEST_CREATE_OPEN_FAILURE(&h, "\\BaseNamedObjects\\\\om.c-test", "->Somewhere", STATUS_OBJECT_NAME_INVALID)
    SYMLNK_TEST_CREATE_OPEN_FAILURE2(&h, "\\BaseNamedObjects\\om.c-test\\", "->Somewhere",
                                     STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_PATH_NOT_FOUND)


    /* Compound test */
    if (!(dir = get_base_dir()))
    {
        win_skip( "couldn't find the BaseNamedObjects dir\n" );
        return;
    }

    InitializeObjectAttributes(&attr, &str, 0, dir, NULL);
    pRtlCreateUnicodeStringFromAsciiz(&str, "test-link");
    pRtlCreateUnicodeStringFromAsciiz(&target, "\\DosDevices");
    status = pNtCreateSymbolicLinkObject(&link, SYMBOLIC_LINK_QUERY, &attr, &target);
    ok(status == STATUS_SUCCESS, "Failed to create SymbolicLink(%08x)\n", status);
    pRtlFreeUnicodeString(&str);
    pRtlFreeUnicodeString(&target);

    pRtlCreateUnicodeStringFromAsciiz(&str, "test-link\\NUL");
    status = pNtOpenFile(&h, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN);
    todo_wine ok(status == STATUS_SUCCESS, "Failed to open NUL device(%08x)\n", status);
    pRtlFreeUnicodeString(&str);

    pNtClose(h);
    pNtClose(link);
    pNtClose(dir);
}

static void test_query_object(void)
{
    static const WCHAR name[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',
                                 '\\','t','e','s','t','_','e','v','e','n','t'};
    static const WCHAR type_event[] = {'E','v','e','n','t'};
    static const WCHAR type_file[] = {'F','i','l','e'};
    HANDLE handle;
    char buffer[1024];
    NTSTATUS status;
    ULONG len, expected_len;
    UNICODE_STRING *str;
    char dir[MAX_PATH];

    handle = CreateEventA( NULL, FALSE, FALSE, "test_event" );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name) + sizeof(WCHAR), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, 0, &len );
    todo_wine ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    todo_wine ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(type_event) + sizeof(WCHAR), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name) + sizeof(WCHAR), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(OBJECT_TYPE_INFORMATION), &len );
    todo_wine ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    todo_wine ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(type_event) + sizeof(WCHAR), "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR) == len, "unexpected len %u\n", len );
    ok( str->Length >= sizeof(name), "unexpected len %u\n", str->Length );
    /* there can be a \\Sessions prefix in the name */
    ok( !memcmp( str->Buffer + (str->Length - sizeof(name)) / sizeof(WCHAR), name, sizeof(name) ),
        "wrong name %s\n", wine_dbgstr_w(str->Buffer) );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, len, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    ok( len >= sizeof(UNICODE_STRING) + sizeof(name) + sizeof(WCHAR), "unexpected len %u\n", len );

    len = 0;
    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(buffer), &len );
    todo_wine ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    todo_wine ok( len > sizeof(OBJECT_TYPE_INFORMATION), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    todo_wine ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + str->Length + sizeof(WCHAR), "unexpected len %u\n", len );
    todo_wine ok( str->Buffer && !memcmp( str->Buffer, type_event, sizeof(type_file) ),
                  "wrong/bad type name %s (%p)\n", wine_dbgstr_w(str->Buffer), str->Buffer );

    len -= sizeof(WCHAR);
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, len, &len );
    todo_wine ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQueryObject failed %x\n", status );
    todo_wine ok( len >= sizeof(OBJECT_TYPE_INFORMATION) + sizeof(type_event) + sizeof(WCHAR), "unexpected len %u\n", len );

    pNtClose( handle );

    handle = CreateEventA( NULL, FALSE, FALSE, NULL );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len == sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    ok( str->Length == 0, "unexpected len %u\n", len );
    ok( str->Buffer == NULL, "unexpected ptr %p\n", str->Buffer );
    pNtClose( handle );

    GetWindowsDirectoryA( dir, MAX_PATH );
    handle = CreateFileA( dir, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS, 0 );
    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    ok( len > sizeof(UNICODE_STRING), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    expected_len = sizeof(UNICODE_STRING) + str->Length + sizeof(WCHAR);
    ok( len == expected_len || broken(len == expected_len - sizeof(WCHAR)), /* NT4 */
        "unexpected len %u\n", len );
    trace( "got %s len %u\n", wine_dbgstr_w(str->Buffer), len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, 0, &len );
    ok( status == STATUS_INFO_LENGTH_MISMATCH || broken(status == STATUS_INSUFFICIENT_RESOURCES),
        "NtQueryObject failed %x\n", status );
    ok( len == expected_len || broken(!len || len == sizeof(UNICODE_STRING)),
        "unexpected len %u\n", len );

    len = 0;
    status = pNtQueryObject( handle, ObjectNameInformation, buffer, sizeof(UNICODE_STRING), &len );
    ok( status == STATUS_BUFFER_OVERFLOW || broken(status == STATUS_INSUFFICIENT_RESOURCES
            || status == STATUS_INFO_LENGTH_MISMATCH),
        "NtQueryObject failed %x\n", status );
    ok( len == expected_len || broken(!len),
        "unexpected len %u\n", len );

    len = 0;
    memset( buffer, 0, sizeof(buffer) );
    status = pNtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(buffer), &len );
    todo_wine ok( status == STATUS_SUCCESS, "NtQueryObject failed %x\n", status );
    todo_wine ok( len > sizeof(OBJECT_TYPE_INFORMATION), "unexpected len %u\n", len );
    str = (UNICODE_STRING *)buffer;
    expected_len = sizeof(OBJECT_TYPE_INFORMATION) + str->Length + sizeof(WCHAR);
    todo_wine ok( len >= expected_len, "unexpected len %u\n", len );
    todo_wine ok( str->Buffer && !memcmp( str->Buffer, type_file, sizeof(type_file) ),
                  "wrong/bad type name %s (%p)\n", wine_dbgstr_w(str->Buffer), str->Buffer );

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

    res = pNtCreateEvent( &h, 0, &attr, 0, 0 );
    ok(!res, "can't create event: %x\n", res);

    res = pNtReleaseSemaphore( h, 30, NULL );
    ok(res == STATUS_OBJECT_TYPE_MISMATCH, "expected 0xc0000024, got %x\n", res);

    pNtClose( h );
}

static void test_event(void)
{
    HANDLE Event;
    HANDLE Event2;
    NTSTATUS status;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    EVENT_BASIC_INFORMATION info;
    static const WCHAR eventName[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s','\\','t','e','s','t','E','v','e','n','t',0};

    pRtlInitUnicodeString(&str, eventName);
    InitializeObjectAttributes(&attr, &str, 0, 0, NULL);

    status = pNtCreateEvent(&Event, GENERIC_ALL, &attr, 1, 0);
    ok( status == STATUS_SUCCESS, "NtCreateEvent failed %08x\n", status );

    status = pNtPulseEvent(Event, NULL);
    ok( status == STATUS_SUCCESS, "NtPulseEvent failed %08x\n", status );

    status = pNtQueryEvent(Event, EventBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryEvent failed %08x\n", status );
    ok( info.EventType == 1 && info.EventState == 0,
        "NtQueryEvent failed, expected 1 0, got %d %d\n", info.EventType, info.EventState );

    status = pNtOpenEvent(&Event2, GENERIC_ALL, &attr);
    ok( status == STATUS_SUCCESS, "NtOpenEvent failed %08x\n", status );

    status = pNtClose(Event);

    status = pNtQueryEvent(Event2, EventBasicInformation, &info, sizeof(info), NULL);
    ok( status == STATUS_SUCCESS, "NtQueryEvent failed %08x\n", status );
    ok( info.EventType == 1 && info.EventState == 0,
        "NtQueryEvent failed, expected 1 0, got %d %d\n", info.EventType, info.EventState );

    status = pNtClose(Event2);
}

static const WCHAR keyed_nameW[] = {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',
                                    '\\','W','i','n','e','T','e','s','t','E','v','e','n','t',0};

static DWORD WINAPI keyed_event_thread( void *arg )
{
    HANDLE handle;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    ULONG_PTR i;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &str;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &str, keyed_nameW );

    status = pNtOpenKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS, &attr );
    ok( !status, "NtOpenKeyedEvent failed %x\n", status );

    for (i = 0; i < 20; i++)
    {
        if (i & 1)
            status = pNtWaitForKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        else
            status = pNtReleaseKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        ok( status == STATUS_SUCCESS, "%li: failed %x\n", i, status );
        Sleep( 20 - i );
    }

    status = pNtReleaseKeyedEvent( handle, (void *)0x1234, 0, NULL );
    ok( status == STATUS_SUCCESS, "NtReleaseKeyedEvent %x\n", status );

    timeout.QuadPart = -10000;
    status = pNtWaitForKeyedEvent( handle, (void *)0x5678, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)0x9abc, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    NtClose( handle );
    return 0;
}

static void test_keyed_events(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    HANDLE handle, event, thread;
    NTSTATUS status;
    LARGE_INTEGER timeout;
    ULONG_PTR i;

    if (!pNtCreateKeyedEvent)
    {
        win_skip( "Keyed events not supported\n" );
        return;
    }

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = &str;
    attr.Attributes               = 0;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &str, keyed_nameW );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS | SYNCHRONIZE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );

    status = WaitForSingleObject( handle, 1000 );
    ok( status == 0, "WaitForSingleObject %x\n", status );

    timeout.QuadPart = -100000;
    status = pNtWaitForKeyedEvent( handle, (void *)255, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)255, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( handle, (void *)254, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)254, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( handle, NULL, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, NULL, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( (HANDLE)0xdeadbeef, (void *)9, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( (HANDLE)0xdeadbeef, (void *)9, 0, &timeout );
    ok( status == STATUS_INVALID_PARAMETER_1, "NtReleaseKeyedEvent %x\n", status );

    status = pNtWaitForKeyedEvent( (HANDLE)0xdeadbeef, (void *)8, 0, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( (HANDLE)0xdeadbeef, (void *)8, 0, &timeout );
    ok( status == STATUS_INVALID_HANDLE, "NtReleaseKeyedEvent %x\n", status );

    thread = CreateThread( NULL, 0, keyed_event_thread, 0, 0, NULL );
    for (i = 0; i < 20; i++)
    {
        if (i & 1)
            status = pNtReleaseKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        else
            status = pNtWaitForKeyedEvent( handle, (void *)(i * 2), 0, NULL );
        ok( status == STATUS_SUCCESS, "%li: failed %x\n", i, status );
        Sleep( i );
    }
    status = pNtWaitForKeyedEvent( handle, (void *)0x1234, 0, &timeout );
    ok( status == STATUS_SUCCESS, "NtWaitForKeyedEvent %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)0x5678, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)0x9abc, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    ok( WaitForSingleObject( thread, 30000 ) == 0, "wait failed\n" );

    NtClose( handle );

    /* test access rights */

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_WAIT, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_WAKE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    status = pNtCreateKeyedEvent( &handle, KEYEDEVENT_ALL_ACCESS, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = WaitForSingleObject( handle, 1000 );
    ok( status == WAIT_FAILED && GetLastError() == ERROR_ACCESS_DENIED,
        "WaitForSingleObject %x err %u\n", status, GetLastError() );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    /* GENERIC_READ gives wait access */
    status = pNtCreateKeyedEvent( &handle, GENERIC_READ, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtReleaseKeyedEvent %x\n", status );
    NtClose( handle );

    /* GENERIC_WRITE gives wake access */
    status = pNtCreateKeyedEvent( &handle, GENERIC_WRITE, &attr, 0 );
    ok( !status, "NtCreateKeyedEvent failed %x\n", status );
    status = pNtWaitForKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_ACCESS_DENIED, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( handle, (void *)8, 0, &timeout );
    ok( status == STATUS_TIMEOUT, "NtReleaseKeyedEvent %x\n", status );

    /* it's not an event */
    status = pNtPulseEvent( handle, NULL );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtPulseEvent %x\n", status );

    status = pNtCreateEvent( &event, GENERIC_ALL, &attr, FALSE, FALSE );
    ok( status == STATUS_OBJECT_NAME_COLLISION || status == STATUS_OBJECT_TYPE_MISMATCH,
        "CreateEvent %x\n", status );

    NtClose( handle );

    status = pNtCreateEvent( &event, GENERIC_ALL, &attr, FALSE, FALSE );
    ok( status == 0, "CreateEvent %x\n", status );
    status = pNtWaitForKeyedEvent( event, (void *)8, 0, &timeout );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtWaitForKeyedEvent %x\n", status );
    status = pNtReleaseKeyedEvent( event, (void *)8, 0, &timeout );
    ok( status == STATUS_OBJECT_TYPE_MISMATCH, "NtReleaseKeyedEvent %x\n", status );
    NtClose( event );
}

START_TEST(om)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");

    if (!hntdll)
    {
        skip("not running on NT, skipping test\n");
        return;
    }

    pCreateWaitableTimerA = (void *)GetProcAddress(hkernel32, "CreateWaitableTimerA");

    pRtlCreateUnicodeStringFromAsciiz = (void *)GetProcAddress(hntdll, "RtlCreateUnicodeStringFromAsciiz");
    pRtlFreeUnicodeString   = (void *)GetProcAddress(hntdll, "RtlFreeUnicodeString");
    pNtCreateEvent          = (void *)GetProcAddress(hntdll, "NtCreateEvent");
    pNtCreateMutant         = (void *)GetProcAddress(hntdll, "NtCreateMutant");
    pNtOpenEvent            = (void *)GetProcAddress(hntdll, "NtOpenEvent");
    pNtQueryEvent           = (void *)GetProcAddress(hntdll, "NtQueryEvent");
    pNtPulseEvent           = (void *)GetProcAddress(hntdll, "NtPulseEvent");
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
    pNtCreateTimer          =  (void *)GetProcAddress(hntdll, "NtCreateTimer");
    pNtCreateSection        =  (void *)GetProcAddress(hntdll, "NtCreateSection");
    pNtQueryObject          =  (void *)GetProcAddress(hntdll, "NtQueryObject");
    pNtReleaseSemaphore     =  (void *)GetProcAddress(hntdll, "NtReleaseSemaphore");
    pNtCreateKeyedEvent     =  (void *)GetProcAddress(hntdll, "NtCreateKeyedEvent");
    pNtOpenKeyedEvent       =  (void *)GetProcAddress(hntdll, "NtOpenKeyedEvent");
    pNtWaitForKeyedEvent    =  (void *)GetProcAddress(hntdll, "NtWaitForKeyedEvent");
    pNtReleaseKeyedEvent    =  (void *)GetProcAddress(hntdll, "NtReleaseKeyedEvent");

    test_case_sensitive();
    test_namespace_pipe();
    test_name_collisions();
    test_directory();
    test_symboliclink();
    test_query_object();
    test_type_mismatch();
    test_event();
    test_keyed_events();
}
