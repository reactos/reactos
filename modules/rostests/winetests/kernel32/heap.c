/*
 * Unit test suite for heap functions
 *
 * Copyright 2002 Geoffrey Hausheer
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2006 Detlef Riekenberg
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
#include <stdlib.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/test.h"

/* some undocumented flags (names are made up) */
#define HEAP_ADD_USER_INFO    0x00000100
#define HEAP_PRIVATE          0x00001000
#define HEAP_PAGE_ALLOCS      0x01000000
#define HEAP_VALIDATE         0x10000000
#define HEAP_VALIDATE_ALL     0x20000000
#define HEAP_VALIDATE_PARAMS  0x40000000
#ifdef __REACTOS__
#define HEAP_SHARED           0x04000000
#endif

#define BLOCK_ALIGN         (2 * sizeof(void *) - 1)
#define ALIGN_BLOCK_SIZE(x) (((x) + BLOCK_ALIGN) & ~BLOCK_ALIGN)

/* use function pointers to avoid warnings for invalid parameter tests */
static LPVOID (WINAPI *pHeapAlloc)(HANDLE,DWORD,SIZE_T);
static LPVOID (WINAPI *pHeapReAlloc)(HANDLE,DWORD,LPVOID,SIZE_T);
static BOOL (WINAPI *pHeapFree)(HANDLE,DWORD,LPVOID);
static BOOL (WINAPI *pGetPhysicallyInstalledSystemMemory)( ULONGLONG * );
static BOOLEAN (WINAPI *pRtlGetUserInfoHeap)(HANDLE,ULONG,void*,void**,ULONG*);
static BOOLEAN (WINAPI *pRtlSetUserValueHeap)(HANDLE,ULONG,void*,void*);
static BOOLEAN (WINAPI *pRtlSetUserFlagsHeap)(HANDLE,ULONG,void*,ULONG,ULONG);
static HGLOBAL (WINAPI *pGlobalAlloc)(UINT,SIZE_T);
static HGLOBAL (WINAPI *pGlobalFree)(HGLOBAL);
static HGLOBAL (WINAPI *pLocalAlloc)(UINT,SIZE_T);
static HGLOBAL (WINAPI *pLocalFree)(HLOCAL);
static BOOL (WINAPI *pHeapQueryInformation)(HANDLE,HEAP_INFORMATION_CLASS,void*,SIZE_T,SIZE_T*);
static BOOL (WINAPI *pHeapSetInformation)(HANDLE,HEAP_INFORMATION_CLASS,void*,SIZE_T);
static UINT (WINAPI *pGlobalFlags)(HGLOBAL);
static ULONG (WINAPI *pRtlGetNtGlobalFlags)(void);

static void load_functions(void)
{
    HMODULE kernel32 = GetModuleHandleW( L"kernel32.dll" );
    HMODULE ntdll = GetModuleHandleW( L"ntdll.dll" );

#define LOAD_FUNC(m, f) p ## f = (void *)GetProcAddress( m, #f );
    LOAD_FUNC( kernel32, HeapAlloc );
    LOAD_FUNC( kernel32, HeapReAlloc );
    LOAD_FUNC( kernel32, HeapFree );
    LOAD_FUNC( kernel32, HeapQueryInformation );
    LOAD_FUNC( kernel32, HeapSetInformation );
    LOAD_FUNC( kernel32, GetPhysicallyInstalledSystemMemory );
    LOAD_FUNC( kernel32, GlobalAlloc );
    LOAD_FUNC( kernel32, GlobalFlags );
    LOAD_FUNC( kernel32, GlobalFree );
    LOAD_FUNC( kernel32, LocalAlloc );
    LOAD_FUNC( kernel32, LocalFree );
    LOAD_FUNC( ntdll, RtlGetNtGlobalFlags );
    LOAD_FUNC( ntdll, RtlGetUserInfoHeap );
    LOAD_FUNC( ntdll, RtlSetUserValueHeap );
    LOAD_FUNC( ntdll, RtlSetUserFlagsHeap );
#undef LOAD_FUNC
}

struct heap
{
    UINT_PTR unknown1[2];
    UINT     ffeeffee;
    UINT     auto_flags;
    UINT_PTR unknown2[7];
    UINT     unknown3[2];
    UINT_PTR unknown4[3];
    UINT     flags;
    UINT     force_flags;
};


/* undocumented RtlWalkHeap structure */

struct rtl_heap_entry
{
    LPVOID lpData;
    SIZE_T cbData; /* differs from PROCESS_HEAP_ENTRY */
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags; /* value differs from PROCESS_HEAP_ENTRY */
    union {
        struct {
            HANDLE hMem;
            DWORD dwReserved[3];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    };
};

/* rtl_heap_entry flags, names made up */

#define RTL_HEAP_ENTRY_BUSY         0x0001
#define RTL_HEAP_ENTRY_REGION       0x0002
#define RTL_HEAP_ENTRY_BLOCK        0x0010
#define RTL_HEAP_ENTRY_UNCOMMITTED  0x1000
#define RTL_HEAP_ENTRY_COMMITTED    0x4000
#define RTL_HEAP_ENTRY_LFH          0x8000


struct heap_thread_params
{
    HANDLE ready_event;
    HANDLE start_event;
    BOOL done;

    HANDLE heap;
    DWORD flags;
    BOOL lock;
};

DWORD WINAPI heap_thread_proc( void *arg )
{
    struct heap_thread_params *params = arg;
    void *ptr;
    DWORD res;
    BOOL ret;

    SetEvent( params->ready_event );

    while (!(res = WaitForSingleObject( params->start_event, INFINITE )) && !params->done)
    {
        if (params->lock)
        {
            ret = HeapLock( params->heap );
            ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
        }

        ptr = HeapAlloc( params->heap, params->flags, 0 );
        ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
        ret = HeapFree( params->heap, params->flags, ptr );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

        if (params->lock)
        {
            ret = HeapUnlock( params->heap );
            ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );
        }

        SetEvent( params->ready_event );
    }
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    return 0;
}


static void test_HeapCreate(void)
{
    static const BYTE buffer[512] = {0};
    SIZE_T alloc_size = 0x8000 * sizeof(void *), size, i;
    struct rtl_heap_entry rtl_entry, rtl_entries[256];
    struct heap_thread_params thread_params = {0};
    PROCESS_HEAP_ENTRY entry, entries[256];
    HANDLE heap, heap1, heaps[8], thread;
    BYTE *ptr, *ptr1, *ptrs[128];
    DWORD heap_count, count;
    ULONG compat_info;
    UINT_PTR align;
    DWORD res;
    BOOL ret;

    thread_params.ready_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!thread_params.ready_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread_params.start_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!thread_params.start_event, "CreateEventW failed, error %lu\n", GetLastError() );
    thread = CreateThread( NULL, 0, heap_thread_proc, &thread_params, 0, NULL );
    ok( !!thread, "CreateThread failed, error %lu\n", GetLastError() );
    res = WaitForSingleObject( thread_params.ready_event, INFINITE );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    heap_count = GetProcessHeaps( 0, NULL );
    ok( heap_count <= 6, "GetProcessHeaps returned %lu\n", heap_count );

    /* check heap alignment */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );
    count = GetProcessHeaps( 0, NULL );
    ok( count == heap_count + 1, "GetProcessHeaps returned %lu\n", count );
    heap1 = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap1 & 0xffff), "wrong heap alignment\n" );
    count = GetProcessHeaps( 0, NULL );
    ok( count == heap_count + 2, "GetProcessHeaps returned %lu\n", count );
    count = GetProcessHeaps( ARRAY_SIZE(heaps), heaps );
    ok( count == heap_count + 2, "GetProcessHeaps returned %lu\n", count );
    ok( heaps[0] == GetProcessHeap(), "got wrong heap\n" );
    ok( heaps[heap_count + 0] == heap, "got wrong heap\n" );
    todo_wine
    ok( heaps[heap_count + 1] == heap1, "got wrong heap\n" );
    ret = HeapDestroy( heap1 );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );
    count = GetProcessHeaps( 0, NULL );
    ok( count == heap_count, "GetProcessHeaps returned %lu\n", count );

    /* growable heap */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    /* test some border cases */

    ret = HeapFree( NULL, 0, NULL );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, NULL );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    if (0) /* crashes */
    {
        SetLastError( 0xdeadbeef );
        ret = HeapFree( heap, 0, (void *)0xdeadbe00 );
        ok( !ret, "HeapFree succeeded\n" );
        ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ptr = (BYTE *)((UINT_PTR)buffer & ~63) + 64;
        ret = HeapFree( heap, 0, ptr );
        ok( !ret, "HeapFree succeeded\n" );
        ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    ptr = HeapReAlloc( heap, 0, NULL, 1 );
    ok( !ptr, "HeapReAlloc succeeded\n" );
    todo_wine
    ok( GetLastError() == NO_ERROR, "got error %lu\n", GetLastError() );
    if (0) /* crashes */
    {
        SetLastError( 0xdeadbeef );
        ptr1 = HeapReAlloc( heap, 0, (void *)0xdeadbe00, 1 );
        ok( !ptr1, "HeapReAlloc succeeded\n" );
        ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
        ret = HeapValidate( heap, 0, (void *)0xdeadbe00 );
        ok( !ret, "HeapValidate succeeded\n" );
        ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ptr = (BYTE *)((UINT_PTR)buffer & ~63) + 64;
        ptr1 = HeapReAlloc( heap, 0, ptr, 1 );
        ok( !ptr1, "HeapReAlloc succeeded\n" );
        ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    ret = HeapValidate( heap, 0, NULL );
    ok( ret, "HeapValidate failed, error %lu\n", GetLastError() );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    ptr = (BYTE *)((UINT_PTR)buffer & ~63) + 64;
    ret = HeapValidate( heap, 0, ptr );
    ok( !ret, "HeapValidate succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, 0, 0 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 0, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    ptr1 = pHeapReAlloc( heap, 0, ptr, ~(SIZE_T)0 - 7 );
    ok( !ptr1, "HeapReAlloc succeeded\n" );
    ptr1 = pHeapReAlloc( heap, 0, ptr, ~(SIZE_T)0 );
    ok( !ptr1, "HeapReAlloc succeeded\n" );
    ret = HeapValidate( heap, 0, ptr );
    ok( ret, "HeapValidate failed, error %lu\n", GetLastError() );
    ret = pHeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = HeapValidate( heap, 0, ptr );
    ok( !ret, "HeapValidate succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );

    ptr = pHeapAlloc( heap, 0, ~(SIZE_T)0 );
    ok( !ptr, "HeapAlloc succeeded\n" );

    ptr = HeapAlloc( heap, 0, 1 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    ptr1 = HeapReAlloc( heap, 0, ptr, 0 );
    ok( !!ptr1, "HeapReAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr1 );
    ok( size == 0, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, 0, 5 * alloc_size + 1 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, 0, alloc_size );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    ptr1 = HeapAlloc( heap, 0, 4 * alloc_size );
    ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    /* test pointer alignment */

    align = 0;
    for (i = 0; i < ARRAY_SIZE(ptrs); ++i)
    {
        ptrs[i] = HeapAlloc( heap, 0, alloc_size );
        ok( !!ptrs[i], "HeapAlloc failed, error %lu\n", GetLastError() );
        align |= (UINT_PTR)ptrs[i];
    }
    ok( !(align & (2 * sizeof(void *) - 1)), "got wrong alignment\n" );
    ok( align & (2 * sizeof(void *)), "got wrong alignment\n" );
    for (i = 0; i < ARRAY_SIZE(ptrs); ++i)
    {
        ret = HeapFree( heap, 0, ptrs[i] );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    }

    align = 0;
    for (i = 0; i < ARRAY_SIZE(ptrs); ++i)
    {
        ptrs[i] = HeapAlloc( heap, 0, 4 * alloc_size );
        ok( !!ptrs[i], "HeapAlloc failed, error %lu\n", GetLastError() );
        align |= (UINT_PTR)ptrs[i];
    }
    ok( !(align & (8 * sizeof(void *) - 1)), "got wrong alignment\n" );
    ok( align & (8 * sizeof(void *)), "got wrong alignment\n" );
    for (i = 0; i < ARRAY_SIZE(ptrs); ++i)
    {
        ret = HeapFree( heap, 0, ptrs[i] );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    }

    /* test HEAP_ZERO_MEMORY */

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, 1 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 1, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, (1 << 20) );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == (1 << 20), "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, alloc_size );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    ptr = HeapReAlloc( heap, HEAP_ZERO_MEMORY, ptr, 3 * alloc_size );
    ok( !!ptr, "HeapReAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 3 * alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    /* shrinking a small-ish block in place and growing back is okay */
    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, alloc_size * 3 / 2 );
    ok( ptr1 == ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY failed, error %lu\n", GetLastError() );
    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, 2 * alloc_size );
    ok( ptr1 == ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY failed, error %lu\n", GetLastError() );

    ptr = HeapReAlloc( heap, HEAP_ZERO_MEMORY, ptr, 1 );
    ok( !!ptr, "HeapReAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 1, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    ptr = HeapReAlloc( heap, HEAP_ZERO_MEMORY, ptr, (1 << 20) );
    ok( !!ptr, "HeapReAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == (1 << 20), "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    /* shrinking a very large block decommits pages and fail to grow in place */
    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, alloc_size * 3 / 2 );
    ok( ptr1 == ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, 2 * alloc_size );
    todo_wine
    ok( ptr1 != ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );

    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* fixed size heaps */

    heap = HeapCreate( 0, alloc_size, alloc_size );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    /* threshold between failure and success varies, and w7pro64 has a much larger overhead. */

    ptr = HeapAlloc( heap, 0, alloc_size - (0x400 + 0x100 * sizeof(void *)) );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == alloc_size - (0x400 + 0x100 * sizeof(void *)),
        "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ptr1 = HeapAlloc( heap, 0, alloc_size - (0x200 + 0x80 * sizeof(void *)) );
    ok( !ptr1, "HeapAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    heap = HeapCreate( 0, 8 * alloc_size, 8 * alloc_size );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ptr = HeapAlloc( heap, 0, 0 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 0, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    /* cannot allocate large blocks from fixed size heap */

    SetLastError( 0xdeadbeef );
    ptr1 = HeapAlloc( heap, 0, 4 * alloc_size );
    ok( !ptr1, "HeapAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, 0, alloc_size );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr1 = HeapAlloc( heap, 0, 4 * alloc_size );
    ok( !ptr1, "HeapAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, alloc_size );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    ptr = HeapReAlloc( heap, HEAP_ZERO_MEMORY, ptr, 2 * alloc_size );
    ok( !!ptr, "HeapReAlloc failed, error %lu\n", GetLastError() );
    size = HeapSize( heap, 0, ptr );
    ok( size == 2 * alloc_size, "HeapSize returned %#Ix, error %lu\n", size, GetLastError() );
    while (size) if (ptr[--size]) break;
    ok( !size && !ptr[0], "memory wasn't zeroed\n" );

    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, alloc_size * 3 / 2 );
    ok( ptr1 == ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY failed, error %lu\n", GetLastError() );
    ptr1 = HeapReAlloc( heap, HEAP_REALLOC_IN_PLACE_ONLY, ptr, 2 * alloc_size );
    ok( ptr1 == ptr, "HeapReAlloc HEAP_REALLOC_IN_PLACE_ONLY failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    count = 0;
    memset( &rtl_entries, 0, sizeof(rtl_entries) );
    memset( &rtl_entry, 0xcd, sizeof(rtl_entry) );
    rtl_entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while (!RtlWalkHeap( heap, &rtl_entry )) rtl_entries[count++] = rtl_entry;
    ok( count == 3, "got count %lu\n", count );

    count = 0;
    memset( &entries, 0, sizeof(entries) );
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 3, "got count %lu\n", count );

    for (i = 0; i < count; ++i)
    {
        winetest_push_context( "%Iu", i );
        ok( rtl_entries[i].lpData == entries[i].lpData, "got lpData %p\n", rtl_entries[i].lpData );
        ok( rtl_entries[i].cbData == entries[i].cbData, "got cbData %#Ix\n", rtl_entries[i].cbData );
        ok( rtl_entries[i].cbOverhead == entries[i].cbOverhead, "got cbOverhead %#x\n", rtl_entries[i].cbOverhead );
        ok( rtl_entries[i].iRegionIndex == entries[i].iRegionIndex, "got iRegionIndex %#x\n", rtl_entries[i].iRegionIndex );
        if (!entries[i].wFlags)
            ok( rtl_entries[i].wFlags == 0, "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_ENTRY_BUSY)
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_COMMITTED|RTL_HEAP_ENTRY_BLOCK|RTL_HEAP_ENTRY_BUSY) || broken(rtl_entries[i].wFlags == 0x411) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_UNCOMMITTED || broken(rtl_entries[i].wFlags == 0x100) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_REGION)
        {
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_REGION, "got wFlags %#x\n", rtl_entries[i].wFlags );
            ok( rtl_entries[i].Region.dwCommittedSize == entries[i].Region.dwCommittedSize,
                "got Region.dwCommittedSize %#lx\n", rtl_entries[i].Region.dwCommittedSize );
            ok( rtl_entries[i].Region.dwUnCommittedSize == entries[i].Region.dwUnCommittedSize,
                "got Region.dwUnCommittedSize %#lx\n", rtl_entries[i].Region.dwUnCommittedSize );
            ok( rtl_entries[i].Region.lpFirstBlock == entries[i].Region.lpFirstBlock,
                "got Region.lpFirstBlock %p\n", rtl_entries[i].Region.lpFirstBlock );
            ok( rtl_entries[i].Region.lpLastBlock == entries[i].Region.lpLastBlock,
                "got Region.lpLastBlock %p\n", rtl_entries[i].Region.lpLastBlock );
        }
        winetest_pop_context();
    }

    ok( entries[0].wFlags == PROCESS_HEAP_REGION, "got wFlags %#x\n", entries[0].wFlags );
    ok( entries[0].lpData == heap, "got lpData %p\n", entries[0].lpData );
    ok( entries[0].cbData <= 0x1000 /* sizeof(*heap) */, "got cbData %#lx\n", entries[0].cbData );
    ok( entries[0].cbOverhead == 0, "got cbOverhead %#x\n", entries[0].cbOverhead );
    ok( entries[0].iRegionIndex == 0, "got iRegionIndex %d\n", entries[0].iRegionIndex );
    todo_wine
    ok( entries[0].Region.dwCommittedSize == 0x400 * sizeof(void *),
        "got Region.dwCommittedSize %#lx\n", entries[0].Region.dwCommittedSize );
    ok( entries[0].Region.dwUnCommittedSize == 0x10000 - entries[0].Region.dwCommittedSize ||
        entries[0].Region.dwUnCommittedSize == 0x10000 * sizeof(void *) - entries[0].Region.dwCommittedSize /* win7 */,
        "got Region.dwUnCommittedSize %#lx\n", entries[0].Region.dwUnCommittedSize );
    todo_wine
    ok( (BYTE *)entries[0].Region.lpFirstBlock == (BYTE *)entries[0].lpData + entries[0].cbData + 2 * sizeof(void *) ||
        (BYTE *)entries[0].Region.lpFirstBlock == (BYTE *)entries[0].lpData + entries[0].cbData + 4 * sizeof(void *),
        "got Region.lpFirstBlock %p\n", entries[0].Region.lpFirstBlock );
    ok( entries[0].Region.lpLastBlock == (BYTE *)entries[2].lpData + entries[2].cbData,
        "got Region.lpLastBlock %p\n", entries[0].Region.lpLastBlock );

    ok( entries[1].wFlags == 0, "got wFlags %#x\n", entries[1].wFlags );
    ok( entries[1].lpData != NULL, "got lpData %p\n", entries[1].lpData );
    ok( entries[1].cbData != 0, "got cbData %#lx\n", entries[1].cbData );
    ok( entries[1].cbOverhead == 4 * sizeof(void *), "got cbOverhead %#x\n", entries[1].cbOverhead );
    ok( entries[1].iRegionIndex == 0, "got iRegionIndex %d\n", entries[1].iRegionIndex );

    ok( entries[2].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[2].wFlags );
    ok( entries[2].lpData == (BYTE *)entries[0].lpData + entries[0].Region.dwCommittedSize,
        "got lpData %p\n", entries[2].lpData );
    ok( entries[2].lpData == (BYTE *)entries[1].lpData + entries[1].cbData + 2 * entries[1].cbOverhead,
        "got lpData %p\n", entries[2].lpData );
    ok( entries[2].cbData == entries[0].Region.dwUnCommittedSize - 0x1000 ||
        entries[2].cbData == entries[0].Region.dwUnCommittedSize /* win7 */,
        "got cbData %#lx\n", entries[2].cbData );
    ok( entries[2].cbOverhead == 0, "got cbOverhead %#x\n", entries[2].cbOverhead );
    ok( entries[2].iRegionIndex == 0, "got iRegionIndex %d\n", entries[2].iRegionIndex );

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, 5 * alloc_size );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );

    count = 0;
    memset( &rtl_entries, 0, sizeof(rtl_entries) );
    memset( &rtl_entry, 0xcd, sizeof(rtl_entry) );
    rtl_entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while (!RtlWalkHeap( heap, &rtl_entry )) rtl_entries[count++] = rtl_entry;
    ok( count == 4, "got count %lu\n", count );

    memmove( entries + 16, entries, 3 * sizeof(entry) );
    count = 0;
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 4, "got count %lu\n", count );
    ok( !memcmp( entries + 16, entries, 3 * sizeof(entry) ), "entries differ\n" );

    for (i = 0; i < count; ++i)
    {
        winetest_push_context( "%Iu", i );
        ok( rtl_entries[i].lpData == entries[i].lpData, "got lpData %p\n", rtl_entries[i].lpData );
        ok( rtl_entries[i].cbData == entries[i].cbData, "got cbData %#Ix\n", rtl_entries[i].cbData );
        ok( rtl_entries[i].cbOverhead == entries[i].cbOverhead, "got cbOverhead %#x\n", rtl_entries[i].cbOverhead );
        ok( rtl_entries[i].iRegionIndex == entries[i].iRegionIndex, "got iRegionIndex %#x\n", rtl_entries[i].iRegionIndex );
        if (!entries[i].wFlags)
            ok( rtl_entries[i].wFlags == 0, "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_ENTRY_BUSY)
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_COMMITTED|RTL_HEAP_ENTRY_BLOCK|RTL_HEAP_ENTRY_BUSY) || broken(rtl_entries[i].wFlags == 0x411) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_UNCOMMITTED || broken(rtl_entries[i].wFlags == 0x100) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_REGION)
        {
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_REGION, "got wFlags %#x\n", rtl_entries[i].wFlags );
            ok( rtl_entries[i].Region.dwCommittedSize == entries[i].Region.dwCommittedSize,
                "got Region.dwCommittedSize %#lx\n", rtl_entries[i].Region.dwCommittedSize );
            ok( rtl_entries[i].Region.dwUnCommittedSize == entries[i].Region.dwUnCommittedSize,
                "got Region.dwUnCommittedSize %#lx\n", rtl_entries[i].Region.dwUnCommittedSize );
            ok( rtl_entries[i].Region.lpFirstBlock == entries[i].Region.lpFirstBlock,
                "got Region.lpFirstBlock %p\n", rtl_entries[i].Region.lpFirstBlock );
            ok( rtl_entries[i].Region.lpLastBlock == entries[i].Region.lpLastBlock,
                "got Region.lpLastBlock %p\n", rtl_entries[i].Region.lpLastBlock );
        }
        winetest_pop_context();
    }

    ok( entries[3].wFlags == PROCESS_HEAP_ENTRY_BUSY ||
        broken(entries[3].wFlags == (PROCESS_HEAP_ENTRY_BUSY | PROCESS_HEAP_ENTRY_DDESHARE)) /* win7 */,
        "got wFlags %#x\n", entries[3].wFlags );
    ok( entries[3].lpData == ptr, "got lpData %p\n", entries[3].lpData );
    ok( entries[3].cbData == 5 * alloc_size, "got cbData %#lx\n", entries[3].cbData );
    ok( entries[3].cbOverhead == 0 || entries[3].cbOverhead == 8 * sizeof(void *) /* win7 */,
        "got cbOverhead %#x\n", entries[3].cbOverhead );
    ok( entries[3].iRegionIndex == 64, "got iRegionIndex %d\n", entries[3].iRegionIndex );

    ptr1 = HeapAlloc( heap, HEAP_ZERO_MEMORY, 5 * alloc_size );
    ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );

    count = 0;
    memset( &rtl_entries, 0, sizeof(rtl_entries) );
    memset( &rtl_entry, 0xcd, sizeof(rtl_entry) );
    rtl_entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while (!RtlWalkHeap( heap, &rtl_entry )) rtl_entries[count++] = rtl_entry;
    ok( count == 5, "got count %lu\n", count );

    memmove( entries + 16, entries, 4 * sizeof(entry) );
    count = 0;
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 5, "got count %lu\n", count );
    ok( !memcmp( entries + 16, entries, 4 * sizeof(entry) ), "entries differ\n" );

    for (i = 0; i < count; ++i)
    {
        winetest_push_context( "%Iu", i );
        ok( rtl_entries[i].lpData == entries[i].lpData, "got lpData %p\n", rtl_entries[i].lpData );
        ok( rtl_entries[i].cbData == entries[i].cbData, "got cbData %#Ix\n", rtl_entries[i].cbData );
        ok( rtl_entries[i].cbOverhead == entries[i].cbOverhead, "got cbOverhead %#x\n", rtl_entries[i].cbOverhead );
        ok( rtl_entries[i].iRegionIndex == entries[i].iRegionIndex, "got iRegionIndex %#x\n", rtl_entries[i].iRegionIndex );
        if (!entries[i].wFlags)
            ok( rtl_entries[i].wFlags == 0, "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_ENTRY_BUSY)
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_COMMITTED|RTL_HEAP_ENTRY_BLOCK|RTL_HEAP_ENTRY_BUSY) || broken(rtl_entries[i].wFlags == 0x411) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_UNCOMMITTED || broken(rtl_entries[i].wFlags == 0x100) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_REGION)
        {
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_REGION, "got wFlags %#x\n", rtl_entries[i].wFlags );
            ok( rtl_entries[i].Region.dwCommittedSize == entries[i].Region.dwCommittedSize,
                "got Region.dwCommittedSize %#lx\n", rtl_entries[i].Region.dwCommittedSize );
            ok( rtl_entries[i].Region.dwUnCommittedSize == entries[i].Region.dwUnCommittedSize,
                "got Region.dwUnCommittedSize %#lx\n", rtl_entries[i].Region.dwUnCommittedSize );
            ok( rtl_entries[i].Region.lpFirstBlock == entries[i].Region.lpFirstBlock,
                "got Region.lpFirstBlock %p\n", rtl_entries[i].Region.lpFirstBlock );
            ok( rtl_entries[i].Region.lpLastBlock == entries[i].Region.lpLastBlock,
                "got Region.lpLastBlock %p\n", rtl_entries[i].Region.lpLastBlock );
        }
        winetest_pop_context();
    }

    ok( entries[4].wFlags == PROCESS_HEAP_ENTRY_BUSY ||
        broken(entries[4].wFlags == (PROCESS_HEAP_ENTRY_BUSY | PROCESS_HEAP_ENTRY_DDESHARE)) /* win7 */,
        "got wFlags %#x\n", entries[4].wFlags );
    ok( entries[4].lpData == ptr1, "got lpData %p\n", entries[4].lpData );
    ok( entries[4].cbData == 5 * alloc_size, "got cbData %#lx\n", entries[4].cbData );
    ok( entries[4].cbOverhead == 0 || entries[4].cbOverhead == 8 * sizeof(void *) /* win7 */,
        "got cbOverhead %#x\n", entries[4].cbOverhead );
    ok( entries[4].iRegionIndex == 64, "got iRegionIndex %d\n", entries[4].iRegionIndex );

    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    memmove( entries + 16, entries, 3 * sizeof(entry) );
    count = 0;
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 3, "got count %lu\n", count );
    ok( !memcmp( entries + 16, entries, 3 * sizeof(entry) ), "entries differ\n" );

    ptr = HeapAlloc( heap, HEAP_ZERO_MEMORY, 123 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );

    memmove( entries + 16, entries, 3 * sizeof(entry) );
    count = 0;
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 4, "got count %lu\n", count );
    ok( !memcmp( entries + 16, entries, 1 * sizeof(entry) ), "entries differ\n" );
    ok( memcmp( entries + 17, entries + 2, 2 * sizeof(entry) ), "entries differ\n" );

    ok( entries[1].wFlags == PROCESS_HEAP_ENTRY_BUSY, "got wFlags %#x\n", entries[1].wFlags );
    ok( entries[1].lpData == ptr, "got lpData %p\n", entries[1].lpData );
    ok( entries[1].cbData == 123, "got cbData %#lx\n", entries[1].cbData );
    ok( entries[1].cbOverhead != 0, "got cbOverhead %#x\n", entries[1].cbOverhead );
    ok( entries[1].iRegionIndex == 0, "got iRegionIndex %d\n", entries[1].iRegionIndex );

    ok( entries[2].wFlags == 0, "got wFlags %#x\n", entries[2].wFlags );
    ok( entries[2].lpData == (BYTE *)entries[1].lpData + entries[1].cbData + entries[1].cbOverhead + 2 * sizeof(void *),
        "got lpData %p\n", entries[2].lpData );
    ok( entries[2].cbData != 0, "got cbData %#lx\n", entries[2].cbData );
    ok( entries[2].cbOverhead == 4 * sizeof(void *), "got cbOverhead %#x\n", entries[2].cbOverhead );
    ok( entries[2].iRegionIndex == 0, "got iRegionIndex %d\n", entries[2].iRegionIndex );

    ok( entries[3].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[3].wFlags );
    ok( entries[3].lpData == (BYTE *)entries[0].lpData + entries[0].Region.dwCommittedSize,
        "got lpData %p\n", entries[3].lpData );
    ok( entries[3].lpData == (BYTE *)entries[2].lpData + entries[2].cbData + 2 * entries[2].cbOverhead,
        "got lpData %p\n", entries[3].lpData );
    ok( entries[3].cbData == entries[0].Region.dwUnCommittedSize - 0x1000 ||
        entries[3].cbData == entries[0].Region.dwUnCommittedSize /* win7 */,
        "got cbData %#lx\n", entries[3].cbData );
    ok( entries[3].cbOverhead == 0, "got cbOverhead %#x\n", entries[3].cbOverhead );
    ok( entries[3].iRegionIndex == 0, "got iRegionIndex %d\n", entries[3].iRegionIndex );

    ptr1 = HeapAlloc( heap, HEAP_ZERO_MEMORY, 456 );
    ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );

    memmove( entries + 16, entries, 4 * sizeof(entry) );
    count = 0;
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 5, "got count %lu\n", count );
    ok( !memcmp( entries + 16, entries, 2 * sizeof(entry) ), "entries differ\n" );
    ok( memcmp( entries + 18, entries + 3, 2 * sizeof(entry) ), "entries differ\n" );

    ok( entries[2].wFlags == PROCESS_HEAP_ENTRY_BUSY, "got wFlags %#x\n", entries[2].wFlags );
    ok( entries[2].lpData == ptr1, "got lpData %p\n", entries[2].lpData );
    ok( entries[2].cbData == 456, "got cbData %#lx\n", entries[2].cbData );
    ok( entries[2].cbOverhead != 0, "got cbOverhead %#x\n", entries[2].cbOverhead );
    ok( entries[2].iRegionIndex == 0, "got iRegionIndex %d\n", entries[2].iRegionIndex );

    ok( entries[3].wFlags == 0, "got wFlags %#x\n", entries[3].wFlags );
    ok( entries[3].lpData == (BYTE *)entries[2].lpData + entries[2].cbData + entries[2].cbOverhead + 2 * sizeof(void *),
        "got lpData %p\n", entries[3].lpData );
    ok( entries[3].cbData != 0, "got cbData %#lx\n", entries[3].cbData );
    ok( entries[3].cbOverhead == 4 * sizeof(void *), "got cbOverhead %#x\n", entries[3].cbOverhead );
    ok( entries[3].iRegionIndex == 0, "got iRegionIndex %d\n", entries[3].iRegionIndex );

    ok( entries[4].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[4].wFlags );
    ok( entries[4].lpData == (BYTE *)entries[0].lpData + entries[0].Region.dwCommittedSize,
        "got lpData %p\n", entries[4].lpData );
    ok( entries[4].lpData == (BYTE *)entries[3].lpData + entries[3].cbData + 2 * entries[3].cbOverhead,
        "got lpData %p\n", entries[4].lpData );
    ok( entries[4].cbData == entries[0].Region.dwUnCommittedSize - 0x1000 ||
        entries[4].cbData == entries[0].Region.dwUnCommittedSize /* win7 */,
        "got cbData %#lx\n", entries[4].cbData );
    ok( entries[4].cbOverhead == 0, "got cbOverhead %#x\n", entries[4].cbOverhead );
    ok( entries[4].iRegionIndex == 0, "got iRegionIndex %d\n", entries[4].iRegionIndex );

    ret = HeapFree( heap, 0, ptr1 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
    ret = HeapFree( heap, 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    size = 0;
    SetLastError( 0xdeadbeef );
    ret = pHeapQueryInformation( 0, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( !ret, "HeapQueryInformation succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    ok( size == 0, "got size %Iu\n", size );

    size = 0;
    SetLastError( 0xdeadbeef );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, NULL, 0, &size );
    ok( !ret, "HeapQueryInformation succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError() );
    ok( size == sizeof(ULONG), "got size %Iu\n", size );

    SetLastError( 0xdeadbeef );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, NULL, 0, NULL );
    ok( !ret, "HeapQueryInformation succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    compat_info = 0xdeadbeef;
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) + 1, NULL );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got compat_info %lu\n", compat_info );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* check setting LFH compat info */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got HeapCompatibilityInformation %lu\n", compat_info );

    compat_info = 2;
    ret = pHeapSetInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) );
    ok( ret, "HeapSetInformation failed, error %lu\n", GetLastError() );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 2, "got HeapCompatibilityInformation %lu\n", compat_info );

    /* cannot be undone */

    compat_info = 0;
    SetLastError( 0xdeadbeef );
    ret = pHeapSetInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) );
    ok( !ret, "HeapSetInformation succeeded\n" );
    ok( GetLastError() == ERROR_GEN_FAILURE, "got error %lu\n", GetLastError() );
    compat_info = 1;
    SetLastError( 0xdeadbeef );
    ret = pHeapSetInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) );
    ok( !ret, "HeapSetInformation succeeded\n" );
    ok( GetLastError() == ERROR_GEN_FAILURE, "got error %lu\n", GetLastError() );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 2, "got HeapCompatibilityInformation %lu\n", compat_info );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* cannot set LFH with HEAP_NO_SERIALIZE */

    heap = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got HeapCompatibilityInformation %lu\n", compat_info );

    compat_info = 2;
    SetLastError( 0xdeadbeef );
    ret = pHeapSetInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) );
    ok( !ret, "HeapSetInformation succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got HeapCompatibilityInformation %lu\n", compat_info );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* some allocation pattern automatically enables LFH */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got HeapCompatibilityInformation %lu\n", compat_info );

    for (i = 0; i < 0x12; i++) ptrs[i] = pHeapAlloc( heap, 0, 0 );
    for (i = 0; i < 0x12; i++) HeapFree( heap, 0, ptrs[i] );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 2, "got HeapCompatibilityInformation %lu\n", compat_info );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* LFH actually doesn't enable immediately, the pattern is required */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    compat_info = 2;
    ret = pHeapSetInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info) );
    ok( ret, "HeapSetInformation failed, error %lu\n", GetLastError() );
    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 2, "got HeapCompatibilityInformation %lu\n", compat_info );

    for (i = 0; i < 0x11; i++) ptrs[i] = pHeapAlloc( heap, 0, 24 + 2 * sizeof(void *) );
    for (i = 0; i < 0x11; i++) HeapFree( heap, 0, ptrs[i] );

    count = 0;
    memset( &entries, 0xcd, sizeof(entries) );
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ok( count == 3, "got count %lu\n", count );

    ok( entries[0].wFlags == PROCESS_HEAP_REGION, "got wFlags %#x\n", entries[0].wFlags );
    ok( entries[0].lpData == heap, "got lpData %p\n", entries[0].lpData );
    ok( entries[0].cbData <= 0x1000 /* sizeof(*heap) */, "got cbData %#lx\n", entries[0].cbData );
    ok( entries[0].cbOverhead == 0, "got cbOverhead %#x\n", entries[0].cbOverhead );
    ok( entries[0].iRegionIndex == 0, "got iRegionIndex %d\n", entries[0].iRegionIndex );
    ok( entries[1].wFlags == 0, "got wFlags %#x\n", entries[1].wFlags );
    ok( entries[2].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[2].wFlags );

    for (i = 0; i < 0x12; i++) ptrs[i] = pHeapAlloc( heap, 0, 24 + 2 * sizeof(void *) );
    for (i = 0; i < 0x12; i++) HeapFree( heap, 0, ptrs[i] );

    count = 0;
    memset( &entries, 0xcd, sizeof(entries) );
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    todo_wine
    ok( count > 24, "got count %lu\n", count );
    if (count < 2) count = 2;

    ok( entries[0].wFlags == PROCESS_HEAP_REGION, "got wFlags %#x\n", entries[0].wFlags );
    ok( entries[0].lpData == heap, "got lpData %p\n", entries[0].lpData );
    ok( entries[0].cbData <= 0x1000 /* sizeof(*heap) */, "got cbData %#lx\n", entries[0].cbData );
    ok( entries[0].cbOverhead == 0, "got cbOverhead %#x\n", entries[0].cbOverhead );
    ok( entries[0].iRegionIndex == 0, "got iRegionIndex %d\n", entries[0].iRegionIndex );
    todo_wine /* Wine currently reports the LFH group as a single block here */
    ok( entries[1].wFlags == 0, "got wFlags %#x\n", entries[1].wFlags );

    for (i = 0; i < 0x12; i++)
    {
        todo_wine
        ok( entries[4 + i].wFlags == 0, "got wFlags %#x\n", entries[4 + i].wFlags );
        todo_wine
        ok( entries[4 + i].cbData == 0x20, "got cbData %#lx\n", entries[4 + i].cbData );
        todo_wine
        ok( entries[4 + i].cbOverhead == 2 * sizeof(void *), "got cbOverhead %#x\n", entries[4 + i].cbOverhead );
    }

    if (entries[count - 1].wFlags == PROCESS_HEAP_REGION) /* > win7 */
        ok( entries[count - 2].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[count - 2].wFlags );
    else
        ok( entries[count - 1].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[count - 2].wFlags );

    count = 0;
    memset( &rtl_entries, 0, sizeof(rtl_entries) );
    memset( &rtl_entry, 0xcd, sizeof(rtl_entry) );
    rtl_entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while (!RtlWalkHeap( heap, &rtl_entry )) rtl_entries[count++] = rtl_entry;
    todo_wine
    ok( count > 24, "got count %lu\n", count );
    if (count < 2) count = 2;

    for (i = 3; i < count; ++i)
    {
        winetest_push_context( "%Iu", i );
        ok( rtl_entries[i].lpData == entries[i].lpData, "got lpData %p\n", rtl_entries[i].lpData );
        ok( rtl_entries[i].cbData == entries[i].cbData, "got cbData %#Ix\n", rtl_entries[i].cbData );
        ok( rtl_entries[i].cbOverhead == entries[i].cbOverhead, "got cbOverhead %#x\n", rtl_entries[i].cbOverhead );
        ok( rtl_entries[i].iRegionIndex == entries[i].iRegionIndex, "got iRegionIndex %#x\n", rtl_entries[i].iRegionIndex );
        if (!entries[i].wFlags)
            ok( rtl_entries[i].wFlags == 0 || rtl_entries[i].wFlags == RTL_HEAP_ENTRY_LFH, "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_ENTRY_BUSY)
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_LFH|RTL_HEAP_ENTRY_BUSY) || broken(rtl_entries[i].wFlags == 1) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_UNCOMMITTED || broken(rtl_entries[i].wFlags == 0x100) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_REGION)
        {
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_LFH|RTL_HEAP_ENTRY_REGION), "got wFlags %#x\n", rtl_entries[i].wFlags );
            ok( rtl_entries[i].Region.dwCommittedSize == entries[i].Region.dwCommittedSize,
                "got Region.dwCommittedSize %#lx\n", rtl_entries[i].Region.dwCommittedSize );
            ok( rtl_entries[i].Region.dwUnCommittedSize == entries[i].Region.dwUnCommittedSize,
                "got Region.dwUnCommittedSize %#lx\n", rtl_entries[i].Region.dwUnCommittedSize );
            ok( rtl_entries[i].Region.lpFirstBlock == entries[i].Region.lpFirstBlock,
                "got Region.lpFirstBlock %p\n", rtl_entries[i].Region.lpFirstBlock );
            ok( rtl_entries[i].Region.lpLastBlock == entries[i].Region.lpLastBlock,
                "got Region.lpLastBlock %p\n", rtl_entries[i].Region.lpLastBlock );
        }
        winetest_pop_context();
    }

    for (i = 0; i < 0x12; i++) ptrs[i] = pHeapAlloc( heap, 0, 24 + 2 * sizeof(void *) );

    count = 0;
    memset( &entries, 0xcd, sizeof(entries) );
    memset( &entry, 0xcd, sizeof(entry) );
    entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while ((ret = HeapWalk( heap, &entry ))) entries[count++] = entry;
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    todo_wine
    ok( count > 24, "got count %lu\n", count );
    if (count < 2) count = 2;

    ok( entries[0].wFlags == PROCESS_HEAP_REGION, "got wFlags %#x\n", entries[0].wFlags );
    ok( entries[0].lpData == heap, "got lpData %p\n", entries[0].lpData );
    ok( entries[0].cbData <= 0x1000 /* sizeof(*heap) */, "got cbData %#lx\n", entries[0].cbData );
    ok( entries[0].cbOverhead == 0, "got cbOverhead %#x\n", entries[0].cbOverhead );
    ok( entries[0].iRegionIndex == 0, "got iRegionIndex %d\n", entries[0].iRegionIndex );
    ok( entries[1].wFlags == 0 || entries[1].wFlags == PROCESS_HEAP_ENTRY_BUSY /* win7 */, "got wFlags %#x\n", entries[1].wFlags );

    for (i = 1; i < count - 2; i++)
    {
        if (entries[i].wFlags != PROCESS_HEAP_ENTRY_BUSY) continue;
        todo_wine /* Wine currently reports the LFH group as a single block */
        ok( entries[i].cbData == 0x18 + 2 * sizeof(void *), "got cbData %#lx\n", entries[i].cbData );
        ok( entries[i].cbOverhead == 0x8, "got cbOverhead %#x\n", entries[i].cbOverhead );
    }

    if (entries[count - 1].wFlags == PROCESS_HEAP_REGION) /* > win7 */
        ok( entries[count - 2].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[count - 2].wFlags );
    else
        ok( entries[count - 1].wFlags == PROCESS_HEAP_UNCOMMITTED_RANGE, "got wFlags %#x\n", entries[count - 2].wFlags );

    count = 0;
    memset( &rtl_entries, 0, sizeof(rtl_entries) );
    memset( &rtl_entry, 0xcd, sizeof(rtl_entry) );
    rtl_entry.lpData = NULL;
    SetLastError( 0xdeadbeef );
    while (!RtlWalkHeap( heap, &rtl_entry )) rtl_entries[count++] = rtl_entry;
    todo_wine
    ok( count > 24, "got count %lu\n", count );
    if (count < 2) count = 2;

    for (i = 3; i < count; ++i)
    {
        winetest_push_context( "%Iu", i );
        ok( rtl_entries[i].lpData == entries[i].lpData, "got lpData %p\n", rtl_entries[i].lpData );
        ok( rtl_entries[i].cbData == entries[i].cbData, "got cbData %#Ix\n", rtl_entries[i].cbData );
        ok( rtl_entries[i].cbOverhead == entries[i].cbOverhead, "got cbOverhead %#x\n", rtl_entries[i].cbOverhead );
        ok( rtl_entries[i].iRegionIndex == entries[i].iRegionIndex, "got iRegionIndex %#x\n", rtl_entries[i].iRegionIndex );
        if (!entries[i].wFlags)
            ok( rtl_entries[i].wFlags == 0 || rtl_entries[i].wFlags == RTL_HEAP_ENTRY_LFH, "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_ENTRY_BUSY)
        {
            todo_wine
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_LFH|RTL_HEAP_ENTRY_BUSY) || broken(rtl_entries[i].wFlags == 1) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        }
        else if (entries[i].wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            ok( rtl_entries[i].wFlags == RTL_HEAP_ENTRY_UNCOMMITTED || broken(rtl_entries[i].wFlags == 0x100) /* win7 */,
                "got wFlags %#x\n", rtl_entries[i].wFlags );
        else if (entries[i].wFlags & PROCESS_HEAP_REGION)
        {
            ok( rtl_entries[i].wFlags == (RTL_HEAP_ENTRY_LFH|RTL_HEAP_ENTRY_REGION), "got wFlags %#x\n", rtl_entries[i].wFlags );
            ok( rtl_entries[i].Region.dwCommittedSize == entries[i].Region.dwCommittedSize,
                "got Region.dwCommittedSize %#lx\n", rtl_entries[i].Region.dwCommittedSize );
            ok( rtl_entries[i].Region.dwUnCommittedSize == entries[i].Region.dwUnCommittedSize,
                "got Region.dwUnCommittedSize %#lx\n", rtl_entries[i].Region.dwUnCommittedSize );
            ok( rtl_entries[i].Region.lpFirstBlock == entries[i].Region.lpFirstBlock,
                "got Region.lpFirstBlock %p\n", rtl_entries[i].Region.lpFirstBlock );
            ok( rtl_entries[i].Region.lpLastBlock == entries[i].Region.lpLastBlock,
                "got Region.lpLastBlock %p\n", rtl_entries[i].Region.lpLastBlock );
        }
        winetest_pop_context();
    }

    for (i = 0; i < 0x12; i++) HeapFree( heap, 0, ptrs[i] );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* check HEAP_NO_SERIALIZE HeapCreate flag effect */

    heap = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = TRUE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = FALSE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* check HEAP_NO_SERIALIZE HeapAlloc / HeapFree flag effect */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = TRUE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = FALSE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = FALSE;
    thread_params.flags = HEAP_NO_SERIALIZE;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    /* check LFH heap locking */

    heap = HeapCreate( 0, 0, 0 );
    ok( !!heap, "HeapCreate failed, error %lu\n", GetLastError() );
    ok( !((ULONG_PTR)heap & 0xffff), "wrong heap alignment\n" );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 0, "got HeapCompatibilityInformation %lu\n", compat_info );

    for (i = 0; i < 0x12; i++) ptrs[i] = pHeapAlloc( heap, 0, 0 );
    for (i = 0; i < 0x12; i++) HeapFree( heap, 0, ptrs[i] );

    ret = pHeapQueryInformation( heap, HeapCompatibilityInformation, &compat_info, sizeof(compat_info), &size );
    ok( ret, "HeapQueryInformation failed, error %lu\n", GetLastError() );
    ok( compat_info == 2, "got HeapCompatibilityInformation %lu\n", compat_info );

    /* locking is serialized */

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = TRUE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );

    /* but allocation is not */

    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    thread_params.heap = heap;
    thread_params.lock = FALSE;
    thread_params.flags = 0;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread_params.ready_event, 100 );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );
    if (res) res = WaitForSingleObject( thread_params.ready_event, 100 );

    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );


    thread_params.done = TRUE;
    SetEvent( thread_params.start_event );
    res = WaitForSingleObject( thread, INFINITE );
    ok( !res, "WaitForSingleObject returned %#lx, error %lu\n", res, GetLastError() );
    CloseHandle( thread_params.start_event );
    CloseHandle( thread_params.ready_event );
    CloseHandle( thread );
}


struct mem_entry
{
    UINT_PTR flags;
    void *ptr;
};

static struct mem_entry *mem_entry_from_HANDLE( HLOCAL handle )
{
    return CONTAINING_RECORD( handle, struct mem_entry, ptr );
}

static BOOL is_mem_entry( HLOCAL handle )
{
    return ((UINT_PTR)handle & ((sizeof(void *) << 1) - 1)) == sizeof(void *);
}

static void test_GlobalAlloc(void)
{
    static const UINT flags_tests[] =
    {
        GMEM_FIXED | GMEM_NOTIFY,
        GMEM_FIXED | GMEM_DISCARDABLE,
        GMEM_MOVEABLE | GMEM_NOTIFY,
        GMEM_MOVEABLE | GMEM_DDESHARE,
        GMEM_MOVEABLE | GMEM_NOT_BANKED,
        GMEM_MOVEABLE | GMEM_NODISCARD,
        GMEM_MOVEABLE | GMEM_DISCARDABLE,
        GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_DISCARDABLE | GMEM_LOWER | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_NOT_BANKED | GMEM_NOTIFY,
    };
    static const UINT realloc_flags_tests[] =
    {
        GMEM_FIXED,
        GMEM_FIXED | GMEM_MODIFY,
        GMEM_MOVEABLE,
        GMEM_MOVEABLE | GMEM_MODIFY,
        GMEM_MOVEABLE | GMEM_DISCARDABLE,
        GMEM_MOVEABLE | GMEM_MODIFY | GMEM_DISCARDABLE,
        GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_DISCARDABLE | GMEM_LOWER | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_NOT_BANKED | GMEM_NOTIFY,
        GMEM_MOVEABLE | GMEM_MODIFY | GMEM_DDESHARE | GMEM_DISCARDABLE | GMEM_LOWER | GMEM_NOCOMPACT | GMEM_NODISCARD | GMEM_NOT_BANKED | GMEM_NOTIFY,
    };
    static const char zero_buffer[100000] = {0};
    static const SIZE_T buffer_size = ARRAY_SIZE(zero_buffer);
    const HGLOBAL invalid_mem = LongToHandle( 0xdeadbee0 + sizeof(void *) );
    SIZE_T size, alloc_size, small_size = 12, nolfh_size = 0x20000;
    void *const invalid_ptr = LongToHandle( 0xdeadbee0 );
    HANDLE heap = GetProcessHeap();
    PROCESS_HEAP_ENTRY walk_entry;
    struct mem_entry *entry;
    HGLOBAL globals[0x10000];
    HGLOBAL mem, tmp_mem;
    BYTE *ptr, *tmp_ptr;
    ULONG tmp_flags;
    UINT i, flags;
    BOOL ret;

    mem = GlobalFree( 0 );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );
    mem = GlobalReAlloc( 0, 10, GMEM_MOVEABLE );
    ok( !mem, "GlobalReAlloc succeeded\n" );

    for (i = 0; i < ARRAY_SIZE(globals); ++i)
    {
        mem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 0 );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        globals[i] = mem;
    }

    memset( &walk_entry, 0xcd, sizeof(walk_entry) );
    walk_entry.lpData = NULL;
    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    while ((ret = HeapWalk( heap, &walk_entry )))
    {
        ok( !(walk_entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE), "got PROCESS_HEAP_ENTRY_MOVEABLE\n" );
        ok( !(walk_entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE), "got PROCESS_HEAP_ENTRY_DDESHARE\n" );
    }
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    mem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 0 );
    ok( !mem, "GlobalAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    mem = LocalAlloc( LMEM_MOVEABLE | LMEM_DISCARDABLE, 0 );
    ok( !mem, "LocalAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );

    mem = GlobalAlloc( GMEM_DISCARDABLE, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE(globals); ++i)
    {
        mem = GlobalFree( globals[i] );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );
    }

    /* make sure LFH is enabled for some small block size */
    for (i = 0; i < 0x12; i++) globals[i] = pGlobalAlloc( GMEM_FIXED, small_size );
    for (i = 0; i < 0x12; i++) pGlobalFree( globals[i] );

    mem = GlobalAlloc( GMEM_MOVEABLE, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    mem = GlobalReAlloc( mem, 10, GMEM_MOVEABLE );
    ok( !!mem, "GlobalReAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size >= 10 && size <= 16, "GlobalSize returned %Iu\n", size );
    mem = GlobalReAlloc( mem, 0, GMEM_MOVEABLE );
    ok( !!mem, "GlobalReAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size == 0, "GlobalSize returned %Iu\n", size );
    mem = GlobalReAlloc( mem, 10, GMEM_MOVEABLE );
    ok( !!mem, "GlobalReAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size >= 10 && size <= 16, "GlobalSize returned %Iu\n", size );
    tmp_mem = pGlobalFree( mem );
    ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size == 0, "GlobalSize returned %Iu\n", size );

    mem = pGlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    entry = mem_entry_from_HANDLE( mem );
    size = GlobalSize( mem );
    ok( size == 0, "GlobalSize returned %Iu\n", size );
    ret = HeapValidate( GetProcessHeap(), 0, entry );
    ok( !ret, "HeapValidate succeeded\n" );
    ok( entry->flags == 0xf, "got unexpected flags %#Ix\n", entry->flags );
    ok( !entry->ptr, "got unexpected ptr %p\n", entry->ptr );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = pGlobalAlloc( GMEM_MOVEABLE, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    entry = mem_entry_from_HANDLE( mem );
    size = GlobalSize( mem );
    ok( size == 0, "GlobalSize returned %Iu\n", size );
    ret = HeapValidate( GetProcessHeap(), 0, entry );
    ok( !ret, "HeapValidate succeeded\n" );
    ok( entry->flags == 0xb, "got unexpected flags %#Ix\n", entry->flags );
    ok( !entry->ptr, "got unexpected ptr %p\n", entry->ptr );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    for (alloc_size = 1; alloc_size < 0x10000000; alloc_size <<= 5)
    {
        winetest_push_context( "size %#Ix", alloc_size );

        mem = GlobalAlloc( GMEM_FIXED, alloc_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !((UINT_PTR)mem & sizeof(void *)), "got unexpected ptr align\n" );
        ok( !((UINT_PTR)mem & (sizeof(void *) - 1)), "got unexpected ptr align\n" );
        ret = HeapValidate( GetProcessHeap(), 0, mem );
        ok( ret, "HeapValidate failed, error %lu\n", GetLastError() );
        tmp_mem = GlobalFree( mem );
        ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );

        mem = pGlobalAlloc( GMEM_MOVEABLE, alloc_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( ((UINT_PTR)mem & sizeof(void *)), "got unexpected entry align\n" );
        ok( !((UINT_PTR)mem & (sizeof(void *) - 1)), "got unexpected entry align\n" );

        entry = mem_entry_from_HANDLE( mem );
        ret = HeapValidate( GetProcessHeap(), 0, entry );
        ok( !ret, "HeapValidate succeeded\n" );
        ret = HeapValidate( GetProcessHeap(), 0, entry->ptr );
        ok( ret, "HeapValidate failed, error %lu\n", GetLastError() );
        size = HeapSize( GetProcessHeap(), 0, entry->ptr );
        ok( size == alloc_size, "HeapSize returned %Iu\n", size );

        tmp_mem = invalid_mem;
        tmp_flags = 0xdeadbeef;
        ret = pRtlGetUserInfoHeap( GetProcessHeap(), 0, entry->ptr, (void **)&tmp_mem, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( tmp_mem == mem, "got user ptr %p\n", tmp_mem );
        ok( tmp_flags == 0x200, "got user flags %#lx\n", tmp_flags );

        ret = pRtlSetUserValueHeap( GetProcessHeap(), 0, entry->ptr, invalid_mem );
        ok( ret, "RtlSetUserValueHeap failed, error %lu\n", GetLastError() );
        tmp_mem = GlobalHandle( entry->ptr );
        ok( tmp_mem == invalid_mem, "GlobalHandle returned unexpected handle\n" );
        ret = pRtlSetUserValueHeap( GetProcessHeap(), 0, entry->ptr, mem );
        ok( ret, "RtlSetUserValueHeap failed, error %lu\n", GetLastError() );

        ptr = GlobalLock( mem );
        ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        ok( ptr != mem, "got unexpected ptr %p\n", ptr );
        ok( ptr == entry->ptr, "got unexpected ptr %p\n", ptr );
        ok( !((UINT_PTR)ptr & sizeof(void *)), "got unexpected ptr align\n" );
        ok( !((UINT_PTR)ptr & (sizeof(void *) - 1)), "got unexpected ptr align\n" );
        for (i = 1; i < 0xff; ++i)
        {
            ok( entry->flags == ((i<<16)|3), "got unexpected flags %#Ix\n", entry->flags );
            ptr = GlobalLock( mem );
            ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        }
        ptr = GlobalLock( mem );
        ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        ok( entry->flags == 0xff0003, "got unexpected flags %#Ix\n", entry->flags );
        for (i = 1; i < 0xff; ++i)
        {
            ret = GlobalUnlock( mem );
            ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
        }
        ret = GlobalUnlock( mem );
        ok( !ret, "GlobalUnlock succeeded, error %lu\n", GetLastError() );
        ok( entry->flags == 0x3, "got unexpected flags %#Ix\n", entry->flags );

        tmp_mem = pGlobalFree( mem );
        ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );
        ok( !!entry->flags, "got unexpected flags %#Ix\n", entry->flags );
        ok( !((UINT_PTR)entry->flags & sizeof(void *)), "got unexpected ptr align\n" );
        ok( !((UINT_PTR)entry->flags & (sizeof(void *) - 1)), "got unexpected ptr align\n" );
        ok( !entry->ptr, "got unexpected ptr %p\n", entry->ptr );

        mem = pGlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 0 );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        entry = mem_entry_from_HANDLE( mem );
        ok( entry->flags == 0xf, "got unexpected flags %#Ix\n", entry->flags );
        ok( !entry->ptr, "got unexpected ptr %p\n", entry->ptr );
        flags = GlobalFlags( mem );
        ok( flags == (GMEM_DISCARDED | GMEM_DISCARDABLE), "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

        mem = pGlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 1 );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        entry = mem_entry_from_HANDLE( mem );
        ok( entry->flags == 0x7, "got unexpected flags %#Ix\n", entry->flags );
        ok( !!entry->ptr, "got unexpected ptr %p\n", entry->ptr );
        flags = GlobalFlags( mem );
        ok( flags == GMEM_DISCARDABLE, "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

        mem = pGlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE | GMEM_DDESHARE, 1 );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        entry = mem_entry_from_HANDLE( mem );
        ok( entry->flags == 0x8007, "got unexpected flags %#Ix\n", entry->flags );
        ok( !!entry->ptr, "got unexpected ptr %p\n", entry->ptr );
        flags = GlobalFlags( mem );
        ok( flags == (GMEM_DISCARDABLE | GMEM_DDESHARE), "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

        winetest_pop_context();
    }

    mem = GlobalAlloc( GMEM_MOVEABLE, 256 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    ptr = GlobalLock( mem );
    ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( ptr != mem, "got unexpected ptr %p\n", ptr );
    tmp_mem = GlobalHandle( ptr );
    ok( tmp_mem == mem, "GlobalHandle returned unexpected handle\n" );
    flags = GlobalFlags( mem );
    ok( flags == 1, "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
    tmp_ptr = GlobalLock( mem );
    ok( !!tmp_ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( tmp_ptr == ptr, "got ptr %p, expected %p\n", tmp_ptr, ptr );
    flags = GlobalFlags( mem );
    ok( flags == 2, "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
    ret = GlobalUnlock( mem );
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    flags = GlobalFlags( mem );
    ok( flags == 1, "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = GlobalUnlock( mem );
    ok( !ret, "GlobalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError() );
    flags = GlobalFlags( mem );
    ok( !flags, "GlobalFlags returned %#x, error %lu\n", flags, GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = GlobalUnlock( mem );
    ok( !ret, "GlobalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );
    tmp_mem = GlobalFree( mem );
    ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = GlobalAlloc( GMEM_DDESHARE, 100 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = pGlobalFree( mem );
    ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );
    if (sizeof(void *) != 8) /* crashes on 64-bit */
    {
        SetLastError( 0xdeadbeef );
        tmp_mem = pGlobalFree( mem );
        ok( tmp_mem == mem, "GlobalFree succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        size = GlobalSize( (HGLOBAL)0xc042 );
        ok( size == 0, "GlobalSize succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    }

    /* freed handles are caught */
    mem = GlobalAlloc( GMEM_MOVEABLE, 256 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = pGlobalFree( mem );
    ok( !tmp_mem, "GlobalFree failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = pGlobalFree( mem );
    ok( tmp_mem == mem, "GlobalFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = GlobalFlags( mem );
    ok( flags == GMEM_INVALID_HANDLE, "GlobalFlags succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = GlobalSize( mem );
    ok( size == 0, "GlobalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = GlobalLock( mem );
    ok( !ptr, "GlobalLock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = GlobalUnlock( mem );
    todo_wine
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalReAlloc( mem, 0, GMEM_MOVEABLE );
    ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    if (sizeof(void *) != 8) /* crashes on 64-bit */
    {
        SetLastError( 0xdeadbeef );
        tmp_mem = GlobalHandle( mem );
        ok( !tmp_mem, "GlobalHandle succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    }

    /* invalid handles are caught */
    SetLastError( 0xdeadbeef );
    tmp_mem = pGlobalFree( invalid_mem );
    ok( tmp_mem == invalid_mem, "GlobalFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = GlobalFlags( invalid_mem );
    ok( flags == GMEM_INVALID_HANDLE, "GlobalFlags succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = GlobalSize( invalid_mem );
    ok( size == 0, "GlobalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = GlobalLock( invalid_mem );
    ok( !ptr, "GlobalLock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = GlobalUnlock( invalid_mem );
    todo_wine
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalReAlloc( invalid_mem, 0, GMEM_MOVEABLE );
    ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    if (sizeof(void *) != 8) /* crashes on 64-bit */
    {
        SetLastError( 0xdeadbeef );
        tmp_mem = GlobalHandle( invalid_mem );
        ok( !tmp_mem, "GlobalHandle succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = pRtlGetUserInfoHeap( GetProcessHeap(), 0, invalid_mem, (void **)&tmp_ptr, &tmp_flags );
        ok( !ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    }

    /* invalid pointers are caught */
    SetLastError( 0xdeadbeef );
    tmp_mem = pGlobalFree( invalid_ptr );
    ok( tmp_mem == invalid_ptr, "GlobalFree succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = GlobalFlags( invalid_ptr );
    todo_wine
    ok( flags == GMEM_INVALID_HANDLE, "GlobalFlags succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = GlobalSize( invalid_ptr );
    ok( size == 0, "GlobalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = GlobalLock( invalid_ptr );
    ok( !ptr, "GlobalLock succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = GlobalUnlock( invalid_ptr );
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalReAlloc( invalid_ptr, 0, GMEM_MOVEABLE );
    ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalHandle( invalid_ptr );
    ok( !tmp_mem, "GlobalHandle succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    if (0) /* crashes */
    {
        SetLastError( 0xdeadbeef );
        ret = pRtlGetUserInfoHeap( GetProcessHeap(), 0, invalid_ptr, (void **)&tmp_ptr, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    }

    /* GMEM_FIXED block doesn't allow resize, though it succeeds with GMEM_MODIFY */
    mem = GlobalAlloc( GMEM_FIXED, small_size );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = GlobalReAlloc( mem, small_size - 1, GMEM_MODIFY );
    ok( !!tmp_mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem == mem, "got ptr %p, expected %p\n", tmp_mem, mem );
    size = GlobalSize( mem );
    ok( size == small_size, "GlobalSize returned %Iu\n", size );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalReAlloc( mem, small_size, 0 );
    ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    if (tmp_mem) mem = tmp_mem;
    tmp_mem = GlobalReAlloc( mem, 1024 * 1024, GMEM_MODIFY );
    ok( !!tmp_mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem == mem, "got ptr %p, expected %p\n", tmp_mem, mem );
    size = GlobalSize( mem );
    ok( size == small_size, "GlobalSize returned %Iu\n", size );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    /* GMEM_FIXED block can be relocated with GMEM_MOVEABLE */
    mem = GlobalAlloc( GMEM_FIXED, small_size );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = GlobalReAlloc( mem, small_size + 1, GMEM_MOVEABLE );
    ok( !!tmp_mem, "GlobalReAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem != mem, "GlobalReAlloc didn't relocate memory\n" );
    ptr = GlobalLock( tmp_mem );
    ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( ptr == tmp_mem, "got ptr %p, expected %p\n", ptr, tmp_mem );
    mem = GlobalFree( tmp_mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    /* test GlobalReAlloc flags / GlobalLock combinations */

    for (i = 0; i < ARRAY_SIZE(realloc_flags_tests); i++)
    {
        struct mem_entry expect_entry, entry;
        BOOL expect_convert;

        flags = realloc_flags_tests[i];
        expect_convert = (flags & (GMEM_MOVEABLE | GMEM_MODIFY)) == (GMEM_MOVEABLE | GMEM_MODIFY);

        winetest_push_context( "flags %#x", flags );

        mem = pGlobalAlloc( GMEM_FIXED, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, 512, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags == GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
            else if (flags != GMEM_MOVEABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else ok( tmp_mem != mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            todo_wine ok( entry.ptr != mem, "got ptr %p was %p\n", entry.ptr, mem );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags == GMEM_MOVEABLE) ok( size == 512, "GlobalSize returned %Iu\n", size );
        else ok( size == small_size, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_FIXED, nolfh_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, nolfh_size + 512, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            todo_wine ok( entry.ptr != mem, "got ptr %p was %p\n", entry.ptr, mem );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags & (GMEM_MODIFY | GMEM_DISCARDABLE)) ok( size == nolfh_size, "GlobalSize returned %Iu\n", size );
        else ok( size == nolfh_size + 512, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_FIXED, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, 10, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags == GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
            else if (flags != GMEM_MOVEABLE) todo_wine_if(!flags) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else todo_wine ok( tmp_mem != mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            ok( entry.ptr == ptr, "got ptr %p was %p\n", entry.ptr, ptr );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags == GMEM_MOVEABLE) ok( size == 10, "GlobalSize returned %Iu\n", size );
        else todo_wine_if(!flags) ok( size == small_size, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_FIXED, nolfh_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, 10, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            ok( entry.ptr != ptr, "got ptr %p was %p\n", entry.ptr, ptr );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags & (GMEM_MODIFY | GMEM_DISCARDABLE)) ok( size == nolfh_size, "GlobalSize returned %Iu\n", size );
        else ok( size == 10, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_FIXED, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, 0, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags == GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
            else if (flags != GMEM_MOVEABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else ok( tmp_mem != mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            ok( entry.ptr == ptr, "got ptr %p was %p\n", entry.ptr, ptr );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags == GMEM_MOVEABLE) ok( size == 0 || broken( size == 1 ) /* w7 */, "GlobalSize returned %Iu\n", size );
        else ok( size == small_size, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_FIXED, nolfh_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = GlobalReAlloc( mem, 0, flags );
        if (!expect_convert)
        {
            ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
            else ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
        }
        else
        {
            ok( is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
            entry = *mem_entry_from_HANDLE( tmp_mem );
            ok( entry.ptr != ptr, "got ptr %p was %p\n", entry.ptr, ptr );
            if (flags & GMEM_DISCARDABLE) ok( (entry.flags & 0x7fff) == 0x7, "got flags %#Ix\n", entry.flags );
            else ok( (entry.flags & 0x7fff) == 0x3, "got flags %#Ix\n", entry.flags );
        }
        if (tmp_mem) mem = tmp_mem;

        size = GlobalSize( mem );
        if (flags & (GMEM_MODIFY | GMEM_DISCARDABLE)) ok( size == nolfh_size, "GlobalSize returned %Iu\n", size );
        else ok( size == 0 || broken( size == 1 ) /* w7 */, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = GlobalLock( mem );
        ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 512, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p\n", tmp_mem );
        else if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else if (flags & GMEM_MOVEABLE) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        if (flags == GMEM_MOVEABLE) ok( entry.ptr != expect_entry.ptr, "got unexpected ptr %p\n", entry.ptr );
        else ok( entry.ptr == expect_entry.ptr, "got ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        if (flags == GMEM_MOVEABLE) ok( size == 512, "GlobalSize returned %Iu\n", size );
        else ok( size == small_size, "GlobalSize returned %Iu\n", size );

        ret = GlobalUnlock( mem );
        ok( !ret, "GlobalUnlock succeeded\n" );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = GlobalLock( mem );
        ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 10, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        if (flags & (GMEM_DISCARDABLE | GMEM_MODIFY)) ok( size == small_size, "GlobalSize returned %Iu\n", size );
        else ok( size == 10, "GlobalSize returned %Iu\n", size );

        ret = GlobalUnlock( mem );
        ok( !ret, "GlobalUnlock succeeded\n" );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = GlobalLock( mem );
        ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 0, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        ok( size == small_size, "GlobalSize returned %Iu\n", size );

        ret = GlobalUnlock( mem );
        ok( !ret, "GlobalUnlock succeeded\n" );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 512, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        if (flags & (GMEM_DISCARDABLE | GMEM_MODIFY)) ok( entry.ptr == expect_entry.ptr, "got ptr %p\n", entry.ptr );
        else ok( entry.ptr != expect_entry.ptr, "got unexpected ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        if (flags & (GMEM_DISCARDABLE | GMEM_MODIFY)) ok( size == small_size, "GlobalSize returned %Iu\n", size );
        else ok( size == 512, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 10, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        if (flags & (GMEM_DISCARDABLE | GMEM_MODIFY)) ok( size == small_size, "GlobalSize returned %Iu\n", size );
        else ok( size == 10, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );


        mem = pGlobalAlloc( GMEM_MOVEABLE, small_size );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = GlobalReAlloc( mem, 0, flags );
        if (flags & GMEM_MODIFY) ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags == GMEM_FIXED) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else if (flags & GMEM_DISCARDABLE) ok( !tmp_mem, "GlobalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "GlobalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if (flags == GMEM_MOVEABLE)
        {
            expect_entry.flags |= 8;
            expect_entry.ptr = NULL;
        }
        else if ((flags & GMEM_DISCARDABLE) && (flags & GMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = GlobalSize( mem );
        if (flags == GMEM_MOVEABLE) ok( size == 0, "GlobalSize returned %Iu\n", size );
        else ok( size == small_size, "GlobalSize returned %Iu\n", size );

        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

        winetest_pop_context();
    }

    mem = GlobalAlloc( GMEM_DDESHARE, 100 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    ret = GlobalUnlock( mem );
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    ret = GlobalUnlock( mem );
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );

    memset( &walk_entry, 0xcd, sizeof(walk_entry) );
    walk_entry.lpData = NULL;
    ret = HeapLock( heap );
    ok( ret, "HeapLock failed, error %lu\n", GetLastError() );
    while ((ret = HeapWalk( heap, &walk_entry )))
    {
        ok( !(walk_entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE), "got PROCESS_HEAP_ENTRY_MOVEABLE\n" );
        ok( !(walk_entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE), "got PROCESS_HEAP_ENTRY_DDESHARE\n" );
    }
    ok( GetLastError() == ERROR_NO_MORE_ITEMS, "got error %lu\n", GetLastError() );
    ret = HeapUnlock( heap );
    ok( ret, "HeapUnlock failed, error %lu\n", GetLastError() );

    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = GlobalAlloc( GMEM_FIXED, 100 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    ret = GlobalUnlock( mem );
    ok( ret, "GlobalUnlock failed, error %lu\n", GetLastError() );
    tmp_mem = GlobalHandle( mem );
    ok( tmp_mem == mem, "GlobalHandle returned unexpected handle\n" );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = GlobalAlloc( GMEM_FIXED, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size == 1, "GlobalSize returned %Iu\n", size );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    /* trying to lock empty memory should give an error */
    mem = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 0 );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = GlobalLock( mem );
    ok( !ptr, "GlobalLock succeeded\n" );
    ok( GetLastError() == ERROR_DISCARDED, "got error %lu\n", GetLastError() );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = GlobalAlloc( 0, buffer_size );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size >= buffer_size, "GlobalSize returned %Iu, error %lu\n", size, GetLastError() );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    mem = GlobalAlloc( GMEM_ZEROINIT, buffer_size );
    ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
    size = GlobalSize( mem );
    ok( size >= buffer_size, "GlobalSize returned %Iu, error %lu\n", size, GetLastError() );
    ptr = GlobalLock( mem );
    ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( ptr == mem, "got ptr %p, expected %p\n", ptr, mem );
    ok( !memcmp( ptr, zero_buffer, buffer_size ), "GlobalAlloc didn't clear memory\n" );

    /* Check that we can change GMEM_FIXED to GMEM_MOVEABLE */
    mem = GlobalReAlloc( mem, 0, GMEM_MODIFY | GMEM_MOVEABLE );
    ok( !!mem, "GlobalReAlloc failed, error %lu\n", GetLastError() );
    ok( mem != ptr, "GlobalReAlloc returned unexpected handle\n" );
    size = GlobalSize( mem );
    ok( size == buffer_size, "GlobalSize returned %Iu, error %lu\n", size, GetLastError() );

    ptr = GlobalLock( mem );
    ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( ptr != mem, "got unexpected ptr %p\n", ptr );
    ret = GlobalUnlock( mem );
    ok( !ret, "GlobalUnlock succeeded, error %lu\n", GetLastError() );
    ok( GetLastError() == NO_ERROR, "got error %lu\n", GetLastError() );

    tmp_mem = GlobalReAlloc( mem, 2 * buffer_size, GMEM_MOVEABLE | GMEM_ZEROINIT );
    ok( !!tmp_mem, "GlobalReAlloc failed\n" );
    ok( tmp_mem == mem || broken( tmp_mem != mem ) /* happens sometimes?? */,
        "GlobalReAlloc returned unexpected handle\n" );
    mem = tmp_mem;

    size = GlobalSize( mem );
    ok( size >= 2 * buffer_size, "GlobalSize returned %Iu, error %lu\n", size, GetLastError() );
    ptr = GlobalLock( mem );
    ok( !!ptr, "GlobalLock failed, error %lu\n", GetLastError() );
    ok( ptr != mem, "got unexpected ptr %p\n", ptr );
    ok( !memcmp( ptr, zero_buffer, buffer_size ), "GlobalReAlloc didn't clear memory\n" );
    ok( !memcmp( ptr + buffer_size, zero_buffer, buffer_size ),
        "GlobalReAlloc didn't clear memory\n" );

    tmp_mem = GlobalHandle( ptr );
    ok( tmp_mem == mem, "GlobalHandle returned unexpected handle\n" );
    /* Check that we can't discard locked memory */
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalDiscard( mem );
    ok( !tmp_mem, "GlobalDiscard succeeded\n" );
    ret = GlobalUnlock( mem );
    ok( !ret, "GlobalUnlock succeeded, error %lu\n", GetLastError() );
    ok( GetLastError() == NO_ERROR, "got error %lu\n", GetLastError() );

    tmp_mem = GlobalDiscard( mem );
    ok( tmp_mem == mem, "GlobalDiscard failed, error %lu\n", GetLastError() );
    mem = GlobalFree( mem );
    ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE(flags_tests); i++)
    {
        mem = GlobalAlloc( flags_tests[i], 4 );
        ok( !!mem, "GlobalAlloc failed, error %lu\n", GetLastError() );
        flags = GlobalFlags( mem );
        ok( !(flags & ~(GMEM_DDESHARE | GMEM_DISCARDABLE)), "got flags %#x, error %lu\n", flags, GetLastError() );
        mem = GlobalFree( mem );
        ok( !mem, "GlobalFree failed, error %lu\n", GetLastError() );
    }

    ptr = HeapAlloc( GetProcessHeap(), 0, 16 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = GlobalHandle( ptr );
    ok( !!tmp_mem, "GlobalHandle failed, error %lu\n", GetLastError() );
    ok( tmp_mem == ptr, "GlobalHandle returned unexpected handle\n" );
    tmp_ptr = (void *)0xdeadbeef;
    tmp_flags = 0xdeadbeef;
    ret = pRtlGetUserInfoHeap( GetProcessHeap(), 0, ptr, (void **)&tmp_ptr, &tmp_flags );
    ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
    ok( tmp_ptr == (void *)0xdeadbeef, "got user value %p\n", tmp_ptr );
    ok( tmp_flags == 0, "got user flags %#lx\n", tmp_flags );
    ret = HeapFree( GetProcessHeap(), 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
}

static void test_LocalAlloc(void)
{
    static const UINT flags_tests[] =
    {
        LMEM_FIXED,
        LMEM_FIXED | LMEM_DISCARDABLE,
        LMEM_MOVEABLE,
        LMEM_MOVEABLE | LMEM_NODISCARD,
        LMEM_MOVEABLE | LMEM_DISCARDABLE,
        LMEM_MOVEABLE | LMEM_DISCARDABLE | LMEM_NOCOMPACT | LMEM_NODISCARD,
    };
    static const UINT realloc_flags_tests[] =
    {
        LMEM_FIXED,
        LMEM_FIXED | LMEM_MODIFY,
        LMEM_MOVEABLE,
        LMEM_MOVEABLE | LMEM_MODIFY,
        LMEM_MOVEABLE | LMEM_DISCARDABLE,
        LMEM_MOVEABLE | LMEM_MODIFY | LMEM_DISCARDABLE,
        LMEM_MOVEABLE | LMEM_DISCARDABLE | LMEM_NOCOMPACT | LMEM_NODISCARD,
        LMEM_MOVEABLE | LMEM_MODIFY | LMEM_DISCARDABLE | LMEM_NOCOMPACT | LMEM_NODISCARD,
    };
    static const char zero_buffer[100000] = {0};
    static const SIZE_T buffer_size = ARRAY_SIZE(zero_buffer);
    const HLOCAL invalid_mem = LongToHandle( 0xdeadbee0 + sizeof(void *) );
    void *const invalid_ptr = LongToHandle( 0xdeadbee0 );
    SIZE_T size, small_size = 12, nolfh_size = 0x20000;
    HLOCAL locals[0x10000];
    HLOCAL mem, tmp_mem;
    BYTE *ptr, *tmp_ptr;
    ULONG tmp_flags;
    UINT i, flags;
    BOOL ret;

    mem = LocalFree( 0 );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );
    mem = LocalReAlloc( 0, 10, LMEM_MOVEABLE );
    ok( !mem, "LocalReAlloc succeeded\n" );

    for (i = 0; i < ARRAY_SIZE(locals); ++i)
    {
        mem = LocalAlloc( LMEM_MOVEABLE | LMEM_DISCARDABLE, 0 );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        locals[i] = mem;
    }

    SetLastError( 0xdeadbeef );
    mem = LocalAlloc( LMEM_MOVEABLE | LMEM_DISCARDABLE, 0 );
    ok( !mem, "LocalAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    mem = GlobalAlloc( GMEM_MOVEABLE | GMEM_DISCARDABLE, 0 );
    ok( !mem, "GlobalAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );

    mem = LocalAlloc( LMEM_DISCARDABLE, 0 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE(locals); ++i)
    {
        mem = LocalFree( locals[i] );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );
    }

    /* make sure LFH is enabled for some small block size */
    for (i = 0; i < 0x12; i++) locals[i] = pLocalAlloc( LMEM_FIXED, small_size );
    for (i = 0; i < 0x12; i++) LocalFree( locals[i] );

    mem = LocalAlloc( LMEM_MOVEABLE, 0 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    mem = LocalReAlloc( mem, 10, LMEM_MOVEABLE );
    ok( !!mem, "LocalReAlloc failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size >= 10 && size <= 16, "LocalSize returned %Iu\n", size );
    mem = LocalReAlloc( mem, 0, LMEM_MOVEABLE );
    ok( !!mem, "LocalReAlloc failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size == 0, "LocalSize returned %Iu\n", size );
    mem = LocalReAlloc( mem, 10, LMEM_MOVEABLE );
    ok( !!mem, "LocalReAlloc failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size >= 10 && size <= 16, "LocalSize returned %Iu\n", size );
    tmp_mem = pLocalFree( mem );
    ok( !tmp_mem, "LocalFree failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size == 0, "LocalSize returned %Iu\n", size );

    mem = LocalAlloc( LMEM_MOVEABLE, 256 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    ptr = LocalLock( mem );
    ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( ptr != mem, "got unexpected ptr %p\n", ptr );
    tmp_mem = LocalHandle( ptr );
    ok( tmp_mem == mem, "LocalHandle returned unexpected handle\n" );
    flags = LocalFlags( mem );
    ok( flags == 1, "LocalFlags returned %#x, error %lu\n", flags, GetLastError() );
    tmp_ptr = LocalLock( mem );
    ok( !!tmp_ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( tmp_ptr == ptr, "got ptr %p, expected %p\n", tmp_ptr, ptr );
    flags = LocalFlags( mem );
    ok( flags == 2, "LocalFlags returned %#x, error %lu\n", flags, GetLastError() );
    ret = LocalUnlock( mem );
    ok( ret, "LocalUnlock failed, error %lu\n", GetLastError() );
    flags = LocalFlags( mem );
    ok( flags == 1, "LocalFlags returned %#x, error %lu\n", flags, GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError() );
    flags = LocalFlags( mem );
    ok( !flags, "LocalFlags returned %#x, error %lu\n", flags, GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );
    tmp_mem = LocalFree( mem );
    ok( !tmp_mem, "LocalFree failed, error %lu\n", GetLastError() );

    /* freed handles are caught */
    mem = LocalAlloc( LMEM_MOVEABLE, 256 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = pLocalFree( mem );
    ok( !tmp_mem, "LocalFree failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = pLocalFree( mem );
    ok( tmp_mem == mem, "LocalFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = LocalFlags( mem );
    ok( flags == LMEM_INVALID_HANDLE, "LocalFlags succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = LocalSize( mem );
    ok( size == 0, "LocalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = LocalLock( mem );
    ok( !ptr, "LocalLock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalReAlloc( mem, 0, LMEM_MOVEABLE );
    ok( !tmp_mem, "LocalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    if (sizeof(void *) != 8) /* crashes on 64-bit */
    {
        SetLastError( 0xdeadbeef );
        tmp_mem = LocalHandle( mem );
        ok( !tmp_mem, "LocalHandle succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    }

    /* invalid handles are caught */
    SetLastError( 0xdeadbeef );
    tmp_mem = pLocalFree( invalid_mem );
    ok( tmp_mem == invalid_mem, "LocalFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = LocalFlags( invalid_mem );
    ok( flags == LMEM_INVALID_HANDLE, "LocalFlags succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = LocalSize( invalid_mem );
    ok( size == 0, "LocalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = LocalLock( invalid_mem );
    ok( !ptr, "LocalLock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( invalid_mem );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalReAlloc( invalid_mem, 0, LMEM_MOVEABLE );
    ok( !tmp_mem, "LocalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    if (sizeof(void *) != 8) /* crashes on 64-bit */
    {
        SetLastError( 0xdeadbeef );
        tmp_mem = LocalHandle( invalid_mem );
        ok( !tmp_mem, "LocalHandle succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    }

    /* invalid pointers are caught */
    SetLastError( 0xdeadbeef );
    tmp_mem = pLocalFree( invalid_ptr );
    ok( tmp_mem == invalid_ptr, "LocalFree succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    flags = LocalFlags( invalid_ptr );
    todo_wine
    ok( flags == LMEM_INVALID_HANDLE, "LocalFlags succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = LocalSize( invalid_ptr );
    ok( size == 0, "LocalSize succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = LocalLock( invalid_ptr );
    ok( !ptr, "LocalLock succeeded\n" );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( invalid_ptr );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalReAlloc( invalid_ptr, 0, LMEM_MOVEABLE );
    ok( !tmp_mem, "LocalReAlloc succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalHandle( invalid_ptr );
    ok( !tmp_mem, "LocalHandle succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_NOACCESS, "got error %lu\n", GetLastError() );

    /* LMEM_FIXED block doesn't allow resize, though it succeeds with LMEM_MODIFY */
    mem = LocalAlloc( LMEM_FIXED, small_size );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = LocalReAlloc( mem, small_size - 1, LMEM_MODIFY );
    ok( !!tmp_mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem == mem, "got ptr %p, expected %p\n", tmp_mem, mem );
    size = LocalSize( mem );
    ok( size == small_size, "LocalSize returned %Iu\n", size );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalReAlloc( mem, small_size, 0 );
    ok( !tmp_mem, "LocalReAlloc succeeded\n" );
    ok( GetLastError() == ERROR_NOT_ENOUGH_MEMORY, "got error %lu\n", GetLastError() );
    if (tmp_mem) mem = tmp_mem;
    tmp_mem = LocalReAlloc( mem, 1024 * 1024, LMEM_MODIFY );
    ok( !!tmp_mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem == mem, "got ptr %p, expected %p\n", tmp_mem, mem );
    size = LocalSize( mem );
    ok( size == small_size, "LocalSize returned %Iu\n", size );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    /* LMEM_FIXED block can be relocated with LMEM_MOVEABLE */
    mem = LocalAlloc( LMEM_FIXED, small_size );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    tmp_mem = LocalReAlloc( mem, small_size + 1, LMEM_MOVEABLE );
    ok( !!tmp_mem, "LocalReAlloc failed, error %lu\n", GetLastError() );
    ok( tmp_mem != mem, "LocalReAlloc didn't relocate memory\n" );
    ptr = LocalLock( tmp_mem );
    ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( ptr == tmp_mem, "got ptr %p, expected %p\n", ptr, tmp_mem );
    tmp_mem = LocalFree( tmp_mem );
    ok( !tmp_mem, "LocalFree failed, error %lu\n", GetLastError() );

    /* test LocalReAlloc flags / LocalLock / size combinations */

    for (i = 0; i < ARRAY_SIZE(realloc_flags_tests); i++)
    {
        struct mem_entry expect_entry, entry;

        flags = realloc_flags_tests[i];

        winetest_push_context( "flags %#x", flags );

        mem = pLocalAlloc( LMEM_FIXED, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = LocalReAlloc( mem, 512, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags != LMEM_MOVEABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem != mem, "LocalReAlloc returned %p\n", tmp_mem );
        if (tmp_mem) mem = tmp_mem;

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 512, "LocalSize returned %Iu\n", size );
        else ok( size == small_size, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_FIXED, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        tmp_mem = LocalReAlloc( mem, nolfh_size + 512, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == nolfh_size + 512, "LocalSize returned %Iu\n", size );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_FIXED, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = LocalReAlloc( mem, 10, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags != LMEM_MOVEABLE) todo_wine_if(!flags) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else todo_wine ok( tmp_mem != mem, "LocalReAlloc returned %p\n", tmp_mem );
        if (tmp_mem) mem = tmp_mem;

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 10, "LocalSize returned %Iu\n", size );
        else todo_wine_if(!flags) ok( size == small_size, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_FIXED, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        tmp_mem = LocalReAlloc( mem, 10, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == 10, "LocalSize returned %Iu\n", size );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_FIXED, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );

        tmp_mem = LocalReAlloc( mem, 0, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags != LMEM_MOVEABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem != mem, "LocalReAlloc returned %p\n", tmp_mem );
        if (tmp_mem) mem = tmp_mem;

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 0 || broken( size == 1 ) /* w7 */, "LocalSize returned %Iu\n", size );
        else ok( size == small_size, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_FIXED, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( !is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        tmp_mem = LocalReAlloc( mem, 0, flags );
        ok( !is_mem_entry( tmp_mem ), "unexpected moveable %p\n", tmp_mem );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == 0 || broken( size == 1 ) /* w7 */, "LocalSize returned %Iu\n", size );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 512, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else if (flags & LMEM_MOVEABLE) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        if (flags == LMEM_MOVEABLE) ok( entry.ptr != expect_entry.ptr, "got unexpected ptr %p\n", entry.ptr );
        else ok( entry.ptr == expect_entry.ptr, "got ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 512, "LocalSize returned %Iu\n", size );
        else ok( size == small_size, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, nolfh_size + 512, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == nolfh_size + 512, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 10, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == small_size, "LocalSize returned %Iu\n", size );
        else ok( size == 10, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 10, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p\n", tmp_mem );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == 10, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 0, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        ok( size == small_size, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        ptr = LocalLock( mem );
        ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 0, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        ok( size == nolfh_size, "LocalSize returned %Iu\n", size );

        ret = LocalUnlock( mem );
        ok( !ret, "LocalUnlock succeeded\n" );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 512, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( entry.ptr == expect_entry.ptr, "got ptr %p\n", entry.ptr );
        else ok( entry.ptr != expect_entry.ptr, "got unexpected ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == small_size, "LocalSize returned %Iu\n", size );
        else ok( size == 512, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, nolfh_size + 512, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got unexpected ptr %p\n", entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == nolfh_size + 512, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 10, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == small_size, "LocalSize returned %Iu\n", size );
        else ok( size == 10, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 10, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags & (LMEM_DISCARDABLE | LMEM_MODIFY)) ok( size == nolfh_size, "LocalSize returned %Iu\n", size );
        else ok( size == 10, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, small_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 0, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags == LMEM_FIXED) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if (flags == LMEM_MOVEABLE)
        {
            expect_entry.flags |= 8;
            expect_entry.ptr = NULL;
        }
        else if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 0, "LocalSize returned %Iu\n", size );
        else ok( size == small_size, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );


        mem = pLocalAlloc( LMEM_MOVEABLE, nolfh_size );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        ok( is_mem_entry( mem ), "unexpected moveable %p\n", mem );
        expect_entry = *mem_entry_from_HANDLE( mem );

        tmp_mem = LocalReAlloc( mem, 0, flags );
        if (flags & LMEM_MODIFY) ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        else if (flags == LMEM_FIXED) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else if (flags & LMEM_DISCARDABLE) ok( !tmp_mem, "LocalReAlloc succeeded\n" );
        else ok( tmp_mem == mem, "LocalReAlloc returned %p, error %lu\n", tmp_mem, GetLastError() );
        entry = *mem_entry_from_HANDLE( mem );
        if (flags == LMEM_MOVEABLE)
        {
            expect_entry.flags |= 8;
            expect_entry.ptr = NULL;
        }
        else if ((flags & LMEM_DISCARDABLE) && (flags & LMEM_MODIFY)) expect_entry.flags |= 4;
        ok( entry.ptr == expect_entry.ptr, "got ptr %p was %p\n", entry.ptr, expect_entry.ptr );
        ok( entry.flags == expect_entry.flags, "got flags %#Ix was %#Ix\n", entry.flags, expect_entry.flags );

        size = LocalSize( mem );
        if (flags == LMEM_MOVEABLE) ok( size == 0, "LocalSize returned %Iu\n", size );
        else ok( size == nolfh_size, "LocalSize returned %Iu\n", size );

        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

        winetest_pop_context();
    }

    mem = LocalAlloc( LMEM_FIXED, 100 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded\n" );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );
    tmp_mem = LocalHandle( mem );
    ok( tmp_mem == mem, "LocalHandle returned unexpected handle\n" );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    mem = LocalAlloc( LMEM_FIXED, 0 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    size = LocalSize( mem );
    ok( size == 0, "LocalSize returned %Iu\n", size );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    /* trying to lock empty memory should give an error */
    mem = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT, 0 );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ptr = LocalLock( mem );
    ok( !ptr, "LocalLock succeeded\n" );
    ok( GetLastError() == ERROR_DISCARDED, "got error %lu\n", GetLastError() );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    mem = LocalAlloc( 0, buffer_size );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size >= buffer_size, "LocalSize returned %Iu, error %lu\n", size, GetLastError() );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    mem = LocalAlloc( LMEM_ZEROINIT, buffer_size );
    ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
    size = LocalSize( mem );
    ok( size >= buffer_size, "LocalSize returned %Iu, error %lu\n", size, GetLastError() );
    ptr = LocalLock( mem );
    ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( ptr == mem, "got ptr %p, expected %p\n", ptr, mem );
    ok( !memcmp( ptr, zero_buffer, buffer_size ), "LocalAlloc didn't clear memory\n" );

    /* Check that we cannot change LMEM_FIXED to LMEM_MOVEABLE */
    mem = LocalReAlloc( mem, 0, LMEM_MODIFY | LMEM_MOVEABLE );
    ok( !!mem, "LocalReAlloc failed, error %lu\n", GetLastError() );
    ok( mem == ptr, "LocalReAlloc returned unexpected handle\n" );
    size = LocalSize( mem );
    ok( size == buffer_size, "LocalSize returned %Iu, error %lu\n", size, GetLastError() );

    ptr = LocalLock( mem );
    ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( ptr == mem, "got unexpected ptr %p\n", ptr );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded, error %lu\n", GetLastError() );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );

    tmp_mem = LocalReAlloc( mem, 2 * buffer_size, LMEM_MOVEABLE | LMEM_ZEROINIT );
    ok( !!tmp_mem, "LocalReAlloc failed\n" );
    ok( tmp_mem == mem || broken( tmp_mem != mem ) /* happens sometimes?? */,
        "LocalReAlloc returned unexpected handle\n" );
    mem = tmp_mem;

    size = LocalSize( mem );
    ok( size >= 2 * buffer_size, "LocalSize returned %Iu, error %lu\n", size, GetLastError() );
    ptr = LocalLock( mem );
    ok( !!ptr, "LocalLock failed, error %lu\n", GetLastError() );
    ok( ptr == mem, "got unexpected ptr %p\n", ptr );
    ok( !memcmp( ptr, zero_buffer, buffer_size ), "LocalReAlloc didn't clear memory\n" );
    ok( !memcmp( ptr + buffer_size, zero_buffer, buffer_size ),
        "LocalReAlloc didn't clear memory\n" );

    tmp_mem = LocalHandle( ptr );
    ok( tmp_mem == mem, "LocalHandle returned unexpected handle\n" );
    tmp_mem = LocalDiscard( mem );
    ok( !!tmp_mem, "LocalDiscard failed, error %lu\n", GetLastError() );
    ok( tmp_mem == mem, "LocalDiscard returned unexpected handle\n" );
    ret = LocalUnlock( mem );
    ok( !ret, "LocalUnlock succeeded, error %lu\n", GetLastError() );
    ok( GetLastError() == ERROR_NOT_LOCKED, "got error %lu\n", GetLastError() );

    tmp_mem = LocalDiscard( mem );
    ok( tmp_mem == mem, "LocalDiscard failed, error %lu\n", GetLastError() );
    mem = LocalFree( mem );
    ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE(flags_tests); i++)
    {
        mem = LocalAlloc( flags_tests[i], 4 );
        ok( !!mem, "LocalAlloc failed, error %lu\n", GetLastError() );
        flags = LocalFlags( mem );
        ok( !(flags & ~LMEM_DISCARDABLE), "got flags %#x, error %lu\n", flags, GetLastError() );
        mem = LocalFree( mem );
        ok( !mem, "LocalFree failed, error %lu\n", GetLastError() );
    }

    ptr = HeapAlloc( GetProcessHeap(), 0, 16 );
    ok( !!ptr, "HeapAlloc failed, error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    tmp_mem = LocalHandle( ptr );
    ok( !!tmp_mem, "LocalHandle failed, error %lu\n", GetLastError() );
    ok( tmp_mem == ptr, "LocalHandle returned unexpected handle\n" );
    tmp_ptr = (void *)0xdeadbeef;
    tmp_flags = 0xdeadbeef;
    ret = pRtlGetUserInfoHeap( GetProcessHeap(), 0, ptr, (void **)&tmp_ptr, &tmp_flags );
    ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
    ok( tmp_ptr == (void *)0xdeadbeef, "got user value %p\n", tmp_ptr );
    ok( tmp_flags == 0, "got user flags %#lx\n", tmp_flags );
    ret = HeapFree( GetProcessHeap(), 0, ptr );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
}

static void test_block_layout( HANDLE heap, DWORD global_flags, DWORD heap_flags, DWORD alloc_flags )
{
    DWORD padd_flags = HEAP_VALIDATE | HEAP_VALIDATE_ALL | HEAP_VALIDATE_PARAMS;
    SIZE_T expect_size, diff, alloc_size, extra_size, tail_size = 0;
    unsigned char *ptr0, *ptr1, *ptr2, tail;
    char tail_buf[64], padd_buf[64];
    void *tmp_ptr, **user_ptr;
    ULONG tmp_flags;
    UINT_PTR align;
    BOOL ret;

    if (global_flags & (FLG_HEAP_DISABLE_COALESCING|FLG_HEAP_PAGE_ALLOCS|FLG_POOL_ENABLE_TAGGING|
                        FLG_HEAP_ENABLE_TAGGING|FLG_HEAP_ENABLE_TAG_BY_DLL))
    {
        skip( "skipping block tests\n" );
        return;
    }

    if (!global_flags && !alloc_flags) extra_size = 8;
    else extra_size = 2 * sizeof(void *);
    if (heap_flags & HEAP_TAIL_CHECKING_ENABLED) extra_size += 2 * sizeof(void *);
    if (heap_flags & padd_flags) extra_size += 2 * sizeof(void *);

    if ((heap_flags & HEAP_TAIL_CHECKING_ENABLED)) tail_size = 2 * sizeof(void *);
    memset( tail_buf, 0xab, sizeof(tail_buf) );
    memset( padd_buf, 0, sizeof(padd_buf) );

    for (alloc_size = 0x20000 * sizeof(void *) - 0x3000; alloc_size > 0; alloc_size >>= 1)
    {
        winetest_push_context( "size %#Ix", alloc_size );

        ptr0 = pHeapAlloc( heap, alloc_flags|HEAP_ZERO_MEMORY, alloc_size );
        ok( !!ptr0, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr1 = pHeapAlloc( heap, alloc_flags|HEAP_ZERO_MEMORY, alloc_size );
        ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr2 = pHeapAlloc( heap, alloc_flags|HEAP_ZERO_MEMORY, alloc_size );
        ok( !!ptr2, "HeapAlloc failed, error %lu\n", GetLastError() );

        align = (UINT_PTR)ptr0 | (UINT_PTR)ptr1 | (UINT_PTR)ptr2;
        ok( !(align & (2 * sizeof(void *) - 1)), "wrong align\n" );

        expect_size = max( alloc_size, 2 * sizeof(void *) );
        expect_size = ALIGN_BLOCK_SIZE( expect_size + extra_size );
        diff = min( llabs( ptr2 - ptr1 ), llabs( ptr1 - ptr0 ) );
        todo_wine_if( (!global_flags && alloc_size < 2 * sizeof(void *)) ||
                      ((heap_flags & HEAP_FREE_CHECKING_ENABLED) && diff >= 0x100000) )
        ok( diff == expect_size, "got diff %#Ix exp %#Ix\n", diff, expect_size );
        ok( !memcmp( ptr0 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );
        ok( !memcmp( ptr1 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );
        ok( !memcmp( ptr2 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );

        ret = HeapFree( heap, 0, ptr2 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr1 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr0 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

        winetest_pop_context();

        if (diff != expect_size)
        {
            todo_wine
            win_skip("skipping sizes\n");
            break;
        }
    }


    /* between the two thresholds, tail may still be set but block position is inconsistent */

    alloc_size = 0x20000 * sizeof(void *) - 0x2000;
    winetest_push_context( "size %#Ix", alloc_size );

    ptr0 = pHeapAlloc( heap, alloc_flags|HEAP_ZERO_MEMORY, alloc_size );
    ok( !!ptr0, "HeapAlloc failed, error %lu\n", GetLastError() );
    ok( !((UINT_PTR)ptr0 & (2 * sizeof(void *) - 1)), "got unexpected ptr align\n" );

    ok( !memcmp( ptr0 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );

    ret = HeapFree( heap, 0, ptr0 );
    ok( ret, "HeapFree failed, error %lu\n", GetLastError() );

    winetest_pop_context();


    for (alloc_size = 0x20000 * sizeof(void *) - 0x1000; alloc_size < 0x800000; alloc_size <<= 1)
    {
        winetest_push_context( "size %#Ix", alloc_size );

        ptr0 = pHeapAlloc( heap, alloc_flags|HEAP_ZERO_MEMORY, alloc_size );
        ok( !!ptr0, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr1 = pHeapAlloc( heap, alloc_flags, alloc_size );
        ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr2 = pHeapAlloc( heap, alloc_flags, alloc_size );
        ok( !!ptr2, "HeapAlloc failed, error %lu\n", GetLastError() );

        align = (UINT_PTR)ptr0 | (UINT_PTR)ptr1 | (UINT_PTR)ptr2;
        ok( !(align & (8 * sizeof(void *) - 1)), "wrong align\n" );

        expect_size = max( alloc_size, 2 * sizeof(void *) );
        expect_size = ALIGN_BLOCK_SIZE( expect_size + extra_size );
        diff = min( llabs( ptr2 - ptr1 ), llabs( ptr1 - ptr0 ) );
        todo_wine_if( alloc_size == 0x7efe9 )
        ok( diff > expect_size, "got diff %#Ix\n", diff );

        tail = ptr0[alloc_size] | ptr1[alloc_size] | ptr2[alloc_size];
        ok( !tail, "got tail\n" );

        ret = HeapFree( heap, 0, ptr2 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr1 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr0 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        winetest_pop_context();

        if (diff == expect_size || (align & (8 * sizeof(void *) - 1)) || tail)
        {
            todo_wine
            win_skip("skipping sizes\n");
            break;
        }
    }

    /* Undocumented HEAP_ADD_USER_INFO flag can be used to force an additional padding
     * on small block sizes. Small block use it to store user info, larger blocks
     * store them in their block header instead.
     *
     * RtlGetUserInfoHeap also requires the flag to work consistently, and otherwise
     * causes crashes when heap flags are used, or on 32-bit.
     */
    if (!(heap_flags & padd_flags))
    {
        alloc_size = 0x1000;
        winetest_push_context( "size %#Ix", alloc_size );
        ptr0 = pHeapAlloc( heap, 0xc00|HEAP_ADD_USER_INFO, alloc_size );
        ok( !!ptr0, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr1 = HeapAlloc( heap, 0x200|HEAP_ADD_USER_INFO, alloc_size );
        ok( !!ptr1, "HeapAlloc failed, error %lu\n", GetLastError() );
        ptr2 = HeapAlloc( heap, HEAP_ADD_USER_INFO, alloc_size );
        ok( !!ptr2, "HeapAlloc failed, error %lu\n", GetLastError() );

        expect_size = max( alloc_size, 2 * sizeof(void *) );
        expect_size = ALIGN_BLOCK_SIZE( expect_size + extra_size + 2 * sizeof(void *) );
        diff = min( llabs( ptr2 - ptr1 ), llabs( ptr1 - ptr0 ) );
        ok( diff == expect_size, "got diff %#Ix\n", diff );

        ok( !memcmp( ptr0 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );
        ok( !memcmp( ptr1 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );
        ok( !memcmp( ptr2 + alloc_size, tail_buf, tail_size ), "missing block tail\n" );

        ok( !memcmp( ptr0 + alloc_size + tail_size, padd_buf, 2 * sizeof(void *) ), "unexpected padding\n" );

        tmp_ptr = (void *)0xdeadbeef;
        tmp_flags = 0xdeadbeef;
        ret = pRtlGetUserInfoHeap( heap, 0, ptr0, (void **)&tmp_ptr, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( tmp_ptr == NULL, "got ptr %p\n", tmp_ptr );
        ok( tmp_flags == 0xc00, "got flags %#lx\n", tmp_flags );

        tmp_ptr = (void *)0xdeadbeef;
        tmp_flags = 0xdeadbeef;
        ret = pRtlGetUserInfoHeap( heap, 0, ptr1, (void **)&tmp_ptr, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( tmp_ptr == NULL, "got ptr %p\n", tmp_ptr );
        ok( tmp_flags == 0x200, "got flags %#lx\n", tmp_flags );

        ret = pRtlSetUserValueHeap( heap, 0, ptr0, (void *)0xdeadbeef );
        ok( ret, "RtlSetUserValueHeap failed, error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = pRtlSetUserFlagsHeap( heap, 0, ptr0, 0, 0x1000 );
        ok( !ret, "RtlSetUserFlagsHeap succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
        SetLastError( 0xdeadbeef );
        ret = pRtlSetUserFlagsHeap( heap, 0, ptr0, 0x100, 0 );
        ok( !ret, "RtlSetUserFlagsHeap succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
        ret = pRtlSetUserFlagsHeap( heap, 0, ptr0, 0x400, 0x200 );
        ok( ret, "RtlSetUserFlagsHeap failed, error %lu\n", GetLastError() );

        tmp_ptr = NULL;
        tmp_flags = 0;
        ret = pRtlGetUserInfoHeap( heap, 0, ptr0, (void **)&tmp_ptr, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( tmp_ptr == (void *)0xdeadbeef, "got ptr %p\n", tmp_ptr );
        ok( tmp_flags == 0xa00 || broken(tmp_flags == 0xc00) /* w1064v1507 */,
            "got flags %#lx\n", tmp_flags );

        user_ptr = (void **)(ptr0 + alloc_size + tail_size);
        ok( user_ptr[1] == (void *)0xdeadbeef, "unexpected user value\n" );
        user_ptr[0] = (void *)0xdeadbeef;
        user_ptr[1] = (void *)0xdeadbee0;

        tmp_ptr = NULL;
        tmp_flags = 0;
        ret = pRtlGetUserInfoHeap( heap, 0, ptr0, (void **)&tmp_ptr, &tmp_flags );
        ok( ret, "RtlGetUserInfoHeap failed, error %lu\n", GetLastError() );
        ok( tmp_ptr == (void *)0xdeadbee0, "got ptr %p\n", tmp_ptr );
        ok( tmp_flags == 0xa00 || broken(tmp_flags == 0xc00) /* w1064v1507 */,
            "got flags %#lx\n", tmp_flags );

        ret = HeapFree( heap, 0, ptr2 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr1 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        ret = HeapFree( heap, 0, ptr0 );
        ok( ret, "HeapFree failed, error %lu\n", GetLastError() );
        winetest_pop_context();
    }
}

static void test_heap_checks( DWORD flags )
{
    BYTE old, *p, *p2;
    BOOL ret;
    SIZE_T i, size, large_size = 3000 * 1024 + 37;

    if (flags & HEAP_PAGE_ALLOCS) return;  /* no tests for that case yet */

    p = pHeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 17 );
    ok( p != NULL, "HeapAlloc failed\n" );

    ret = HeapValidate( GetProcessHeap(), 0, p );
    ok( ret, "HeapValidate failed\n" );

    size = HeapSize( GetProcessHeap(), 0, p );
    ok( size == 17, "Wrong size %Iu\n", size );

    ok( p[14] == 0, "wrong data %x\n", p[14] );
    ok( p[15] == 0, "wrong data %x\n", p[15] );
    ok( p[16] == 0, "wrong data %x\n", p[16] );

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        ok( p[17] == 0xab, "wrong padding %x\n", p[17] );
        ok( p[18] == 0xab, "wrong padding %x\n", p[18] );
        ok( p[19] == 0xab, "wrong padding %x\n", p[19] );
    }

    p2 = HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, p, 14 );
    if (p2 == p)
    {
        if (flags & HEAP_TAIL_CHECKING_ENABLED)
        {
            ok( p[14] == 0xab, "wrong padding %x\n", p[14] );
            ok( p[15] == 0xab, "wrong padding %x\n", p[15] );
            ok( p[16] == 0xab, "wrong padding %x\n", p[16] );
        }
        else
        {
            ok( p[14] == 0, "wrong padding %x\n", p[14] );
            ok( p[15] == 0, "wrong padding %x\n", p[15] );
        }
    }
    else skip( "realloc in place failed\n");

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    p = pHeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 17 );
    ok( p != NULL, "HeapAlloc failed\n" );
    old = p[17];
    p[17] = 0xcc;

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        ret = HeapValidate( GetProcessHeap(), 0, p );
        ok( !ret, "HeapValidate succeeded\n" );

        /* other calls only check when HEAP_VALIDATE is set */
        if (flags & HEAP_VALIDATE)
        {
            size = HeapSize( GetProcessHeap(), 0, p );
            ok( size == ~(SIZE_T)0 || broken(size == ~0u), "Wrong size %Iu\n", size );

            p2 = HeapReAlloc( GetProcessHeap(), 0, p, 14 );
            ok( p2 == NULL, "HeapReAlloc succeeded\n" );

            ret = HeapFree( GetProcessHeap(), 0, p );
            ok( !ret || broken(sizeof(void*) == 8), /* not caught on xp64 */
                "HeapFree succeeded\n" );
        }

        p[17] = old;
        size = HeapSize( GetProcessHeap(), 0, p );
        ok( size == 17, "Wrong size %Iu\n", size );

        p2 = HeapReAlloc( GetProcessHeap(), 0, p, 14 );
        ok( p2 != NULL, "HeapReAlloc failed\n" );
        p = p2;
    }

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        UINT *p32, tmp = 0;

        size = 4 + 3;
        p = pHeapAlloc( GetProcessHeap(), 0, size );
        ok( !!p, "HeapAlloc failed\n" );
        p32 = (UINT *)p;

        ok( p32[0] == 0xbaadf00d, "got %#x\n", p32[0] );
        memcpy( &tmp, p + size - 3, 3 );
        ok( tmp != 0xadf00d, "got %#x\n", tmp );
        memset( p, 0xcc, size );

        size += 2 * 4;
        p = pHeapReAlloc( GetProcessHeap(), 0, p, size );
        ok( !!p, "HeapReAlloc failed\n" );
        p32 = (UINT *)p;

        ok( p32[0] == 0xcccccccc, "got %#x\n", p32[0] );
        ok( p32[1] << 8 == 0xcccccc00, "got %#x\n", p32[1] );
        ok( p32[2] == 0xbaadf00d, "got %#x\n", p32[2] );
        memcpy( &tmp, p + size - 3, 3 );
        ok( tmp != 0xadf00d, "got %#x\n", tmp );

        ret = pHeapFree( GetProcessHeap(), 0, p );
        ok( ret, "failed.\n" );
    }

    p = HeapAlloc( GetProcessHeap(), 0, 37 );
    ok( p != NULL, "HeapAlloc failed\n" );
    memset( p, 0xcc, 37 );

    ret = pHeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        ok( p[16] == 0xee, "wrong data %x\n", p[16] );
        ok( p[17] == 0xfe, "wrong data %x\n", p[17] );
        ok( p[18] == 0xee, "wrong data %x\n", p[18] );
        ok( p[19] == 0xfe, "wrong data %x\n", p[19] );

        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( ret, "HeapValidate failed\n" );

        old = p[16];
        p[16] = 0xcc;
        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( !ret, "HeapValidate succeeded\n" );

        p[16] = old;
        ret = HeapValidate( GetProcessHeap(), 0, NULL );
        ok( ret, "HeapValidate failed\n" );
    }

    /* now test large blocks */

    p = pHeapAlloc( GetProcessHeap(), 0, large_size );
    ok( p != NULL, "HeapAlloc failed\n" );

    ret = HeapValidate( GetProcessHeap(), 0, p );
    ok( ret, "HeapValidate failed\n" );

    size = HeapSize( GetProcessHeap(), 0, p );
    ok( size == large_size, "Wrong size %Iu\n", size );

    ok( p[large_size - 2] == 0, "wrong data %x\n", p[large_size - 2] );
    ok( p[large_size - 1] == 0, "wrong data %x\n", p[large_size - 1] );

    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        /* Windows doesn't do tail checking on large blocks */
        ok( p[large_size] == 0, "wrong data %x\n", p[large_size] );
        ok( p[large_size + 1] == 0, "wrong data %x\n", p[large_size + 1] );
        ok( p[large_size + 2] == 0, "wrong data %x\n", p[large_size + 2] );
    }

    ret = HeapFree( GetProcessHeap(), 0, p );
    ok( ret, "HeapFree failed\n" );

    /* test block sizes when tail checking */
    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        for (size = 0; size < 64; size++)
        {
            p = HeapAlloc( GetProcessHeap(), 0, size );
            for (i = 0; i < 32; i++) if (p[size + i] != 0xab) break;
            ok( i >= 8, "only %Iu tail bytes for size %Iu\n", i, size );
            HeapFree( GetProcessHeap(), 0, p );
        }
    }
}

static void test_debug_heap( const char *argv0, DWORD flags )
{
    char keyname[MAX_PATH];
    char buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    BOOL ret;
    DWORD err;
    HKEY hkey;
    const char *basename;

    if ((basename = strrchr( argv0, '\\' ))) basename++;
    else basename = argv0;

    sprintf( keyname, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
             basename );
    if (!strcmp( keyname + strlen(keyname) - 3, ".so" )) keyname[strlen(keyname) - 3] = 0;

    err = RegCreateKeyA( HKEY_LOCAL_MACHINE, keyname, &hkey );
    if (err == ERROR_ACCESS_DENIED)
    {
        skip("Not authorized to change the image file execution options\n");
        return;
    }
    ok( !err, "failed to create '%s' error %lu\n", keyname, err );
    if (err) return;

    if (flags == 0xdeadbeef)  /* magic value for unsetting it */
        RegDeleteValueA( hkey, "GlobalFlag" );
    else
        RegSetValueExA( hkey, "GlobalFlag", 0, REG_DWORD, (BYTE *)&flags, sizeof(flags) );

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);

    sprintf( buffer, "%s heap.c 0x%lx", argv0, flags );
    ret = CreateProcessA( NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok( ret, "failed to create child process error %lu\n", GetLastError() );
    if (ret)
    {
        wait_child_process( info.hProcess );
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    RegDeleteValueA( hkey, "GlobalFlag" );
    RegCloseKey( hkey );
    RegDeleteKeyA( HKEY_LOCAL_MACHINE, keyname );
}

static DWORD heap_flags_from_global_flag( DWORD flag )
{
    DWORD ret = 0;

    if (flag & FLG_HEAP_ENABLE_TAIL_CHECK)
        ret |= HEAP_TAIL_CHECKING_ENABLED;
    if (flag & FLG_HEAP_ENABLE_FREE_CHECK)
        ret |= HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_PARAMETERS)
        ret |= HEAP_VALIDATE_PARAMS | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_ALL)
        ret |= HEAP_VALIDATE_ALL | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_DISABLE_COALESCING)
        ret |= HEAP_DISABLE_COALESCE_ON_FREE;
    if (flag & FLG_HEAP_PAGE_ALLOCS)
        ret |= HEAP_PAGE_ALLOCS;
    return ret;
}

static void test_heap_layout( HANDLE handle, DWORD global_flag, DWORD heap_flags )
{
    DWORD force_flags = heap_flags & ~(HEAP_SHARED|HEAP_DISABLE_COALESCE_ON_FREE);
    struct heap *heap = handle;

    if (global_flag & FLG_HEAP_ENABLE_TAGGING) heap_flags |= HEAP_SHARED;
    if (!(global_flag & FLG_HEAP_PAGE_ALLOCS)) force_flags &= ~(HEAP_GROWABLE|HEAP_PRIVATE);

    ok( heap->force_flags == force_flags, "got force_flags %#x\n", heap->force_flags );
    ok( heap->flags == heap_flags, "got flags %#x\n", heap->flags );

    if (heap->flags & HEAP_PAGE_ALLOCS)
    {
        struct heap expect_heap;
        memset( &expect_heap, 0xee, sizeof(expect_heap) );
        expect_heap.force_flags = heap->force_flags;
        expect_heap.flags = heap->flags;
        todo_wine
        ok( !memcmp( heap, &expect_heap, sizeof(expect_heap) ), "got unexpected data\n" );
    }
    else
    {
        ok( heap->ffeeffee == 0xffeeffee, "got ffeeffee %#x\n", heap->ffeeffee );
        ok( heap->auto_flags == (heap_flags & HEAP_GROWABLE) || !heap->auto_flags,
            "got auto_flags %#x\n", heap->auto_flags );
    }
}

static void test_child_heap( const char *arg )
{
    char buffer[32];
    DWORD global_flags = strtoul( arg, 0, 16 ), type, size = sizeof(buffer);
    DWORD heap_flags;
    HANDLE heap;
    HKEY hkey;
    BOOL ret;

    if (global_flags == 0xdeadbeef)  /* global_flags value comes from Session Manager global flags */
    {
        ret = RegOpenKeyA( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager", &hkey );
        if (!ret)
        {
            skip( "Session Manager flags not set\n" );
            return;
        }

        ret = RegQueryValueExA( hkey, "GlobalFlag", 0, &type, (BYTE *)buffer, &size );
        ok( ret, "RegQueryValueExA failed, error %lu\n", GetLastError() );

        if (type == REG_DWORD) global_flags = *(DWORD *)buffer;
        else if (type == REG_SZ) global_flags = strtoul( buffer, 0, 16 );

        ret = RegCloseKey( hkey );
        ok( ret, "RegCloseKey failed, error %lu\n", GetLastError() );
    }
    if (global_flags && !pRtlGetNtGlobalFlags())  /* not working on NT4 */
    {
        win_skip( "global flags not set\n" );
        return;
    }

    heap_flags = heap_flags_from_global_flag( global_flags );
    trace( "testing global flags %#lx, heap flags %08lx\n", global_flags, heap_flags );

    ok( pRtlGetNtGlobalFlags() == global_flags, "got global flags %#lx\n", pRtlGetNtGlobalFlags() );

    test_heap_layout( GetProcessHeap(), global_flags, heap_flags|HEAP_GROWABLE );

    heap = HeapCreate( 0, 0, 0 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_heap_layout( heap, global_flags, heap_flags|HEAP_GROWABLE|HEAP_PRIVATE );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    heap = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_heap_layout( heap, global_flags, heap_flags|HEAP_NO_SERIALIZE|HEAP_GROWABLE|HEAP_PRIVATE );
    test_block_layout( heap, global_flags, heap_flags|HEAP_NO_SERIALIZE|HEAP_GROWABLE|HEAP_PRIVATE, 0 );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    heap = HeapCreate( HEAP_TAIL_CHECKING_ENABLED|HEAP_FREE_CHECKING_ENABLED|HEAP_NO_SERIALIZE, 0, 0 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_heap_layout( heap, global_flags, heap_flags|HEAP_NO_SERIALIZE|HEAP_GROWABLE|HEAP_PRIVATE );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    heap = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_block_layout( heap, global_flags, heap_flags|HEAP_NO_SERIALIZE|HEAP_GROWABLE|HEAP_PRIVATE,
                       HEAP_TAIL_CHECKING_ENABLED|HEAP_FREE_CHECKING_ENABLED );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    heap = HeapCreate( 0, 0x1000, 0x10000 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_heap_layout( heap, global_flags, heap_flags|HEAP_PRIVATE );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    heap = HeapCreate( HEAP_SHARED, 0, 0 );
    ok( heap != GetProcessHeap(), "got unexpected heap\n" );
    test_heap_layout( heap, global_flags, heap_flags|HEAP_GROWABLE|HEAP_PRIVATE );
    ret = HeapDestroy( heap );
    ok( ret, "HeapDestroy failed, error %lu\n", GetLastError() );

    test_heap_checks( heap_flags );
}

static void test_GetPhysicallyInstalledSystemMemory(void)
{
    MEMORYSTATUSEX memstatus;
    ULONGLONG total_memory;
    BOOL ret;

    if (!pGetPhysicallyInstalledSystemMemory)
    {
        win_skip("GetPhysicallyInstalledSystemMemory is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetPhysicallyInstalledSystemMemory(NULL);
    ok(!ret, "GetPhysicallyInstalledSystemMemory should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    total_memory = 0;
    ret = pGetPhysicallyInstalledSystemMemory(&total_memory);
    ok(ret, "GetPhysicallyInstalledSystemMemory unexpectedly failed (%lu)\n", GetLastError());
    ok(total_memory != 0, "expected total_memory != 0\n");

    memstatus.dwLength = sizeof(memstatus);
    ret = GlobalMemoryStatusEx(&memstatus);
    ok(ret, "GlobalMemoryStatusEx unexpectedly failed\n");
    ok(total_memory >= memstatus.ullTotalPhys / 1024,
       "expected total_memory >= memstatus.ullTotalPhys / 1024\n");
}

static void test_GlobalMemoryStatus(void)
{
    char buffer[sizeof(SYSTEM_PERFORMANCE_INFORMATION) + 16]; /* some Win 7 versions need a larger info */
    SYSTEM_PERFORMANCE_INFORMATION *perf_info = (void *)buffer;
    SYSTEM_BASIC_INFORMATION basic_info;
    MEMORYSTATUSEX memex = {0}, expect;
    MEMORYSTATUS mem = {0};
    VM_COUNTERS_EX vmc;
    NTSTATUS status;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = GlobalMemoryStatusEx( &memex );
    ok( !ret, "GlobalMemoryStatusEx succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    GlobalMemoryStatus( &mem );
    ok( GetLastError() == 0xdeadbeef, "got error %lu\n", GetLastError() );

    status = NtQuerySystemInformation( SystemBasicInformation, &basic_info, sizeof(basic_info), NULL );
    ok( !status, "NtQuerySystemInformation returned %#lx\n", status );
    status = NtQuerySystemInformation( SystemPerformanceInformation, perf_info, sizeof(buffer), NULL );
    ok( !status, "NtQuerySystemInformation returned %#lx\n", status );
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessVmCounters, &vmc, sizeof(vmc), NULL );
    ok( !status, "NtQueryInformationProcess returned %#lx\n", status );
    mem.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &mem );
    memex.dwLength = sizeof(MEMORYSTATUSEX);
    ret = GlobalMemoryStatusEx( &memex );
    ok( ret, "GlobalMemoryStatusEx succeeded\n" );

    ok( basic_info.PageSize, "got 0 PageSize\n" );
    ok( basic_info.MmNumberOfPhysicalPages, "got 0 MmNumberOfPhysicalPages\n" );
    ok( !!basic_info.HighestUserAddress, "got 0 HighestUserAddress\n" );
    ok( !!basic_info.LowestUserAddress, "got 0 LowestUserAddress\n" );
    ok( perf_info->TotalCommittedPages, "got 0 TotalCommittedPages\n" );
    ok( perf_info->TotalCommitLimit, "got 0 TotalCommitLimit\n" );
    ok( perf_info->AvailablePages, "got 0 AvailablePages\n" );

    expect.dwMemoryLoad = (memex.ullTotalPhys - memex.ullAvailPhys) / (memex.ullTotalPhys / 100);
    expect.ullTotalPhys = (ULONGLONG)basic_info.MmNumberOfPhysicalPages * basic_info.PageSize;
    expect.ullAvailPhys = (ULONGLONG)perf_info->AvailablePages * basic_info.PageSize;
    expect.ullTotalPageFile = (ULONGLONG)perf_info->TotalCommitLimit * basic_info.PageSize;
    expect.ullAvailPageFile = (ULONGLONG)(perf_info->TotalCommitLimit - perf_info->TotalCommittedPages) * basic_info.PageSize;
    expect.ullTotalVirtual = (ULONG_PTR)basic_info.HighestUserAddress - (ULONG_PTR)basic_info.LowestUserAddress + 1;
    expect.ullAvailVirtual = expect.ullTotalVirtual - (ULONGLONG)vmc.WorkingSetSize /* approximate */;
    expect.ullAvailExtendedVirtual = 0;

/* allow some variability, info sources are not always in sync */
#define IS_WITHIN_RANGE(a, b) (((a) - (b) + (256 * basic_info.PageSize)) <= (512 * basic_info.PageSize))

    ok( memex.dwMemoryLoad == expect.dwMemoryLoad, "got dwMemoryLoad %lu\n", memex.dwMemoryLoad );
    ok( memex.ullTotalPhys == expect.ullTotalPhys, "got ullTotalPhys %#I64x\n", memex.ullTotalPhys );
    ok( IS_WITHIN_RANGE( memex.ullAvailPhys, expect.ullAvailPhys ), "got ullAvailPhys %#I64x\n", memex.ullAvailPhys );
    ok( memex.ullTotalPageFile == expect.ullTotalPageFile, "got ullTotalPageFile %#I64x\n", memex.ullTotalPageFile );
    ok( IS_WITHIN_RANGE( memex.ullAvailPageFile, expect.ullAvailPageFile ), "got ullAvailPageFile %#I64x\n", memex.ullAvailPageFile );
    ok( memex.ullTotalVirtual == expect.ullTotalVirtual, "got ullTotalVirtual %#I64x\n", memex.ullTotalVirtual );
    ok( memex.ullAvailVirtual <= expect.ullAvailVirtual, "got ullAvailVirtual %#I64x\n", memex.ullAvailVirtual );
    ok( memex.ullAvailExtendedVirtual == 0, "got ullAvailExtendedVirtual %#I64x\n", memex.ullAvailExtendedVirtual );

    ok( mem.dwMemoryLoad == memex.dwMemoryLoad, "got dwMemoryLoad %lu\n", mem.dwMemoryLoad );
    ok( mem.dwTotalPhys == min( ~(SIZE_T)0 >> 1, memex.ullTotalPhys ) ||
        broken( mem.dwTotalPhys == ~(SIZE_T)0 ) /* Win <= 8.1 with RAM size > 4GB */,
        "got dwTotalPhys %#Ix\n", mem.dwTotalPhys );
    ok( IS_WITHIN_RANGE( mem.dwAvailPhys, min( ~(SIZE_T)0 >> 1, memex.ullAvailPhys ) ) ||
        broken( mem.dwAvailPhys == ~(SIZE_T)0 ) /* Win <= 8.1 with RAM size > 4GB */,
        "got dwAvailPhys %#Ix\n", mem.dwAvailPhys );
#ifndef _WIN64
    todo_wine_if(memex.ullTotalPageFile > 0xfff7ffff)
#endif
    ok( mem.dwTotalPageFile == min( ~(SIZE_T)0, memex.ullTotalPageFile ), "got dwTotalPageFile %#Ix\n", mem.dwTotalPageFile );
    ok( IS_WITHIN_RANGE( mem.dwAvailPageFile, min( ~(SIZE_T)0, memex.ullAvailPageFile ) ), "got dwAvailPageFile %#Ix\n", mem.dwAvailPageFile );
    ok( mem.dwTotalVirtual == memex.ullTotalVirtual, "got dwTotalVirtual %#Ix\n", mem.dwTotalVirtual );
    ok( mem.dwAvailVirtual == memex.ullAvailVirtual, "got dwAvailVirtual %#Ix\n", mem.dwAvailVirtual );

#undef IS_WITHIN_RANGE
}

static void get_valloc_info( void *mem, char **base, SIZE_T *alloc_size )
{
    MEMORY_BASIC_INFORMATION info, info2;
    SIZE_T size;
    char *p;

    size = VirtualQuery( mem, &info, sizeof(info) );
    ok( size == sizeof(info), "got %Iu.\n", size );

    info2 = info;
    p = info.AllocationBase;
    while (1)
    {
        size = VirtualQuery( p, &info2, sizeof(info2) );
        ok( size == sizeof(info), "got %Iu.\n", size );
        if (info2.AllocationBase != info.AllocationBase)
            break;
        ok( info2.State == MEM_RESERVE || info2.State == MEM_COMMIT, "got %#lx.\n", info2.State );
        p += info2.RegionSize;
    }

    *base = info.AllocationBase;
    *alloc_size = p - *base;
}

static void test_heap_size( SIZE_T initial_size )
{
    static const SIZE_T default_heap_size = 0x10000, init_grow_size = 0x100000, max_grow_size = 0xfd0000;

    BOOL initial_subheap = TRUE, max_size_reached = FALSE;
    SIZE_T alloc_size, current_subheap_size;
    char *base, *current_base;
    unsigned int i;
    HANDLE heap;
    void *p;

    winetest_push_context( "init size %#Ix", initial_size );
    heap = HeapCreate( HEAP_NO_SERIALIZE, initial_size, 0 );
    get_valloc_info( heap, &current_base, &alloc_size );

    ok( alloc_size == initial_size + default_heap_size || broken( (initial_size && alloc_size == initial_size)
        || (!initial_size && (alloc_size == default_heap_size * sizeof(void*))) ) /* Win7 */,
        "got %#Ix.\n", alloc_size );

    current_subheap_size = alloc_size;
    for (i = 0; i < 100; ++i)
    {
        winetest_push_context( "i %u, current_subheap_size %#Ix", i, current_subheap_size );
        p = HeapAlloc( heap, 0, 0x60000 );
        get_valloc_info( p, &base, &alloc_size );
        if (base != current_base)
        {
            current_base = base;
            if (initial_subheap)
            {
                current_subheap_size = init_grow_size;
                initial_subheap = FALSE;
            }
            else
            {
                current_subheap_size = min( current_subheap_size * 2, max_grow_size );
                if (current_subheap_size == max_grow_size)
                    max_size_reached = TRUE;
            }
        }
        ok( alloc_size == current_subheap_size, "got %#Ix.\n", alloc_size );
        winetest_pop_context();
    }
    ok( max_size_reached, "Did not reach maximum subheap size.\n" );

    HeapDestroy( heap );
    winetest_pop_context();
}

static void test_heap_sizes(void)
{
    unsigned int i;
    SIZE_T size, round_size = 0x400 * sizeof(void*);
    char *base;

    test_heap_size( 0 );
    test_heap_size( 0x80000 );
    test_heap_size( 0x150000 );

    for (i = 1; i < 0x100; i++)
    {
        HANDLE heap = HeapCreate( 0, i * 0x100, i * 0x100 );
        ok( heap != NULL, "%x: creation failed\n", i * 0x100 );
        get_valloc_info( heap, &base, &size );
        ok( size == ((i * 0x100 + round_size - 1) & ~(round_size - 1)),
            "%x: wrong size %Ix\n", i * 0x100, size );
        HeapDestroy( heap );
    }
}

START_TEST(heap)
{
    int argc;
    char **argv;

    load_functions();

    argc = winetest_get_mainargs( &argv );
    if (argc >= 3)
    {
        test_child_heap( argv[2] );
        return;
    }

    test_HeapCreate();
    test_GlobalAlloc();
    test_LocalAlloc();

    test_GetPhysicallyInstalledSystemMemory();
    test_GlobalMemoryStatus();

    if (pRtlGetNtGlobalFlags)
    {
        test_debug_heap( argv[0], 0 );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAIL_CHECK );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_FREE_CHECK );
        test_debug_heap( argv[0], FLG_HEAP_VALIDATE_PARAMETERS );
        test_debug_heap( argv[0], FLG_HEAP_VALIDATE_ALL );
        test_debug_heap( argv[0], FLG_POOL_ENABLE_TAGGING );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAGGING );
        test_debug_heap( argv[0], FLG_HEAP_ENABLE_TAG_BY_DLL );
        test_debug_heap( argv[0], FLG_HEAP_DISABLE_COALESCING );
        test_debug_heap( argv[0], FLG_HEAP_PAGE_ALLOCS );
        test_debug_heap( argv[0], 0xdeadbeef );
    }
    else win_skip( "RtlGetNtGlobalFlags not found, skipping heap debug tests\n" );
    test_heap_sizes();
}
