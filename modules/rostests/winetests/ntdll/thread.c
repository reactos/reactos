/*
 * Unit test suite for ntdll thread functions
 *
 * Copyright 2021 Paul Gofman for CodeWeavers
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

static NTSTATUS (WINAPI *pNtCreateThreadEx)( HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *,
                                             HANDLE, PRTL_THREAD_START_ROUTINE, void *,
                                             ULONG, ULONG_PTR, SIZE_T, SIZE_T, PS_ATTRIBUTE_LIST * );
static int * (CDECL *p_errno)(void);

static void init_function_pointers(void)
{
    HMODULE hntdll = GetModuleHandleA( "ntdll.dll" );
#define GET_FUNC(name) p##name = (void *)GetProcAddress( hntdll, #name );
    GET_FUNC( NtCreateThreadEx );
    GET_FUNC( _errno );
#undef GET_FUNC
}

static void CALLBACK test_NtCreateThreadEx_proc(void *param)
{
}

static void test_dbg_hidden_thread_creation(void)
{
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO create_info;
    PS_ATTRIBUTE_LIST ps_attr;
    WCHAR path[MAX_PATH + 4];
    HANDLE process, thread;
    UNICODE_STRING imageW;
    BOOLEAN dbg_hidden;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( !dbg_hidden, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    status = pNtCreateThreadEx( &thread, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_NtCreateThreadEx_proc,
                                NULL, THREAD_CREATE_FLAGS_CREATE_SUSPENDED | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER,
                                0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    dbg_hidden = 0xcc;
    status = NtQueryInformationThread( thread, ThreadHideFromDebugger, &dbg_hidden, sizeof(dbg_hidden), NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    ok( dbg_hidden == 1, "Got unexpected dbg_hidden %#x.\n", dbg_hidden );

    status = NtResumeThread( thread, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );

    lstrcpyW( path, L"\\??\\" );
    GetModuleFileNameW( NULL, path + 4, MAX_PATH );

    RtlInitUnicodeString( &imageW, path );

    memset( &ps_attr, 0, sizeof(ps_attr) );
    ps_attr.Attributes[0].Attribute = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size = lstrlenW(path) * sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr = path;
    ps_attr.TotalLength = sizeof(ps_attr);

#if !defined (__REACTOS__) || (DLL_EXPORT_VERSION >= 0x601)
    status = RtlCreateProcessParametersEx( &params, &imageW, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, PROCESS_PARAMS_FLAG_NORMALIZED );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    /* NtCreateUserProcess() may return STATUS_INVALID_PARAMETER with some uninitialized data in create_info. */
    memset( &create_info, 0, sizeof(create_info) );
    create_info.Size = sizeof(create_info);

    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED
                                  | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );
    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                  &create_info, &ps_attr );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    status = NtTerminateProcess( process, 0 );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );
    CloseHandle( process );
    CloseHandle( thread );
#endif
}

struct unique_teb_thread_args
{
    TEB *teb;
    HANDLE running_event;
    HANDLE quit_event;
};

static void CALLBACK test_unique_teb_proc(void *param)
{
    struct unique_teb_thread_args *args = param;
    args->teb = NtCurrentTeb();
    SetEvent( args->running_event );
    WaitForSingleObject( args->quit_event, INFINITE );
}

static void test_unique_teb(void)
{
    HANDLE threads[2], running_events[2];
    struct unique_teb_thread_args args1, args2;
    NTSTATUS status;

    if (!pNtCreateThreadEx)
    {
        win_skip( "NtCreateThreadEx is not available.\n" );
        return;
    }

    args1.running_event = running_events[0] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args1.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args2.running_event = running_events[1] = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( args2.running_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    args1.quit_event = args2.quit_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    ok( args1.quit_event != NULL, "CreateEventW failed %lu.\n", GetLastError() );

    status = pNtCreateThreadEx( &threads[0], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args1, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    status = pNtCreateThreadEx( &threads[1], THREAD_ALL_ACCESS, NULL, GetCurrentProcess(), test_unique_teb_proc,
                                &args2, 0, 0, 0, 0, NULL );
    ok( status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status );

    WaitForMultipleObjects( 2, running_events, TRUE, INFINITE );
    SetEvent( args1.quit_event );

    WaitForMultipleObjects( 2, threads, TRUE, INFINITE );
    CloseHandle( threads[0] );
    CloseHandle( threads[1] );
    CloseHandle( args1.running_event );
    CloseHandle( args2.running_event );
    CloseHandle( args1.quit_event );

    ok( NtCurrentTeb() != args1.teb, "Multiple threads have TEB %p.\n", args1.teb );
    ok( NtCurrentTeb() != args2.teb, "Multiple threads have TEB %p.\n", args2.teb );
    ok( args1.teb != args2.teb, "Multiple threads have TEB %p.\n", args1.teb );
}

static void test_errno(void)
{
    int val;

    if (!p_errno)
    {
        win_skip( "_errno not available\n" );
        return;
    }
    ok( NtCurrentTeb()->Peb->TlsBitmap->Buffer[0] & (1 << 16), "TLS entry 16 not allocated\n" );
    *p_errno() = 0xdead;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xdead, "wrong value %x\n", val );
    *p_errno() = 0xbeef;
    val = PtrToLong( TlsGetValue( 16 ));
    ok( val == 0xbeef, "wrong value %x\n", val );
}

START_TEST(thread)
{
    init_function_pointers();

    test_dbg_hidden_thread_creation();
    test_unique_teb();
    test_errno();
}
