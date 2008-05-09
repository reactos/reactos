/*
 * Unit test suite for Virtual* family of APIs.
 *
 * Copyright 2004 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/test.h"

#define NUM_THREADS 4
#define MAPPING_SIZE 0x100000

static HINSTANCE hkernel32;
static LPVOID (WINAPI *pVirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
static BOOL   (WINAPI *pVirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);

/* ############################### */

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ok(CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL,
                     &si, &pi) != 0, "error: %u\n", GetLastError());
    ok(CloseHandle(pi.hThread) != 0, "error %u\n", GetLastError());
    return pi.hProcess;
}

static void test_VirtualAllocEx(void)
{
    const unsigned int alloc_size = 1<<15;
    char *src, *dst;
    unsigned long bytes_written = 0, bytes_read = 0, i;
    void *addr1, *addr2;
    BOOL b;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;
    HANDLE hProcess;

    /* not exported in all windows-versions  */
    if ((!pVirtualAllocEx) || (!pVirtualFreeEx)) {
        skip("VirtualAllocEx not found\n");
        return;
    }

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hProcess, NULL, alloc_size, MEM_COMMIT,
                           PAGE_EXECUTE_READWRITE);
    if (!addr1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {   /* Win9x */
        skip("VirtualAllocEx not implemented\n");
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return;
    }

    src = HeapAlloc( GetProcessHeap(), 0, alloc_size );
    dst = HeapAlloc( GetProcessHeap(), 0, alloc_size );
    for (i = 0; i < alloc_size; i++)
        src[i] = i & 0xff;

    ok(addr1 != NULL, "VirtualAllocEx error %u\n", GetLastError());
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok(b && (bytes_written == alloc_size), "%lu bytes written\n",
       bytes_written);
    b = ReadProcessMemory(hProcess, addr1, dst, alloc_size, &bytes_read);
    ok(b && (bytes_read == alloc_size), "%lu bytes read\n", bytes_read);
    ok(!memcmp(src, dst, alloc_size), "Data from remote process differs\n");
    b = pVirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE);
    ok(b != 0, "VirtualFreeEx, error %u\n", GetLastError());

    HeapFree( GetProcessHeap(), 0, src );
    HeapFree( GetProcessHeap(), 0, dst );

    /*
     * The following tests parallel those in test_VirtualAlloc()
     */

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hProcess, 0, 0, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 == NULL, "VirtualAllocEx should fail on zero-sized allocation\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER /* NT */ ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Win9x */
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = pVirtualAllocEx(hProcess, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAllocEx failed\n");

    /* test a not committed memory */
    memset(&info, 'q', sizeof(info));
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info), "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = pVirtualAllocEx(hProcess, addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAllocEx failed\n");

    /* test a committed memory */
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_NOACCESS, "wrong old protection: got %04x instead of PAGE_NOACCESS\n", old_prot);

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_READONLY, "wrong old protection: got %04x instead of PAGE_READONLY\n", old_prot);

    ok(!pVirtualFreeEx(hProcess, addr1, 0x10000, 0),
       "VirtualFreeEx should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(pVirtualFreeEx(hProcess, addr1, 0x10000, MEM_DECOMMIT), "VirtualFreeEx failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!pVirtualFreeEx(hProcess, addr1, 1, MEM_RELEASE),
       "VirtualFreeEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(pVirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE), "VirtualFreeEx failed\n");

    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

static void test_VirtualAlloc(void)
{
    void *addr1, *addr2;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;

    SetLastError(0xdeadbeef);
    addr1 = VirtualAlloc(0, 0, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 == NULL, "VirtualAlloc should fail on zero-sized allocation\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER /* NT */ ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Win9x */
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = VirtualAlloc(0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAlloc failed\n");

    /* test a not committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %d, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAlloc failed\n");

    /* test a committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %d, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_NOACCESS,
        "wrong old protection: got %04x instead of PAGE_NOACCESS\n", old_prot);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_READONLY,
        "wrong old protection: got %04x instead of PAGE_READONLY\n", old_prot);

    ok(!VirtualFree(addr1, 0x10000, 0), "VirtualFree should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0x10000, MEM_DECOMMIT), "VirtualFree failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!VirtualFree(addr1, 1, MEM_RELEASE), "VirtualFree should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
}

static void test_MapViewOfFile(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, mapping;
    void *ptr, *ptr2;
    MEMORY_BASIC_INFORMATION info;
    BOOL ret;

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAPE_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    /* read-only mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    CloseHandle( mapping );

    /* copy-on-write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    CloseHandle( mapping );

    /* no access mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_NOACCESS, 0, 4096, NULL );
    /* fails on NT but succeeds on win9x */
    if (!mapping) ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );
    else
    {
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
        UnmapViewOfFile( ptr );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_COPY succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
        UnmapViewOfFile( ptr );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

        CloseHandle( mapping );
    }

    CloseHandle( file );

    /* now try read-only file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_WRITECOPY error %u\n", GetLastError() );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_READONLY error %u\n", GetLastError() );
    CloseHandle( mapping );
    CloseHandle( file );

    /* now try no access file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_WRITECOPY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READONLY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    CloseHandle( file );
    DeleteFileA( testfile );

    SetLastError(0xdeadbeef);
    file = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "Global\\Foo");
    ok( file != 0, "CreateFileMapping PAGE_READWRITE error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_READ, FALSE, "Global\\Foo" );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_READ error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
todo_wine ok( !ptr, "MapViewOfFile FILE_MAP_WRITE should fail\n" );
todo_wine ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ok( VirtualQuery( ptr, &info, sizeof(info) ) == sizeof(info),
        "VirtualQuery error %u\n", GetLastError() );
    ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
    ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
todo_wine ok( info.AllocationProtect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.AllocationProtect );
    ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
todo_wine ok( info.Protect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.Protect );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_WRITE, FALSE, "Global\\Foo" );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_WRITE error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
todo_wine ok( !ptr, "MapViewOfFile FILE_MAP_READ should fail\n" );
todo_wine ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ok( VirtualQuery( ptr, &info, sizeof(info) ) == sizeof(info),
        "VirtualQuery error %u\n", GetLastError() );
    ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
    ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
    ok( info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect );
    ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    CloseHandle( file );

    /* read/write mapping with SEC_RESERVE */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0, MAPPING_SIZE, NULL);
    ok(mapping != INVALID_HANDLE_VALUE, "CreateFileMappingA failed with error %d\n", GetLastError());

    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %d\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.AllocationProtect == PAGE_READWRITE, "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
    ok(info.RegionSize == MAPPING_SIZE, "RegionSize should have been 0x%x but was 0x%x\n", MAPPING_SIZE, (unsigned int)info.RegionSize);
todo_wine
    ok(info.State == MEM_RESERVE, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
todo_wine
    ok(info.Protect == 0, "Protect should have been 0 instead of 0x%x\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);

    ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READWRITE);
    ok(ptr != NULL, "VirtualAlloc failed with error %d\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.AllocationProtect == PAGE_READWRITE, "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
todo_wine
    ok(info.RegionSize == 0x10000, "RegionSize should have been 0x10000 but was 0x%x\n", (unsigned int)info.RegionSize);
    ok(info.State == MEM_COMMIT, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "Protect should have been 0 instead of 0x%x\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);

    ptr2 = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    /* on NT ptr != ptr2 but on Win9x ptr == ptr2 */
    ok(ptr2 != NULL, "MapViewOfFile failed with error %d\n", GetLastError());
    trace("mapping same section resulted in views %p and %p\n", ptr, ptr2);

    /* shows that the VirtualAlloc above affects the mapping, not just the
     * virtual memory in this process - it also affects all other processes
     * with a view of the mapping, but that isn't tested here */
    ret = VirtualQuery(ptr2, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr2, "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
    ok(info.AllocationBase == ptr2, "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
    ok(info.AllocationProtect == PAGE_READWRITE, "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
todo_wine
    ok(info.RegionSize == 0x10000, "RegionSize should have been 0x10000 but was 0x%x\n", (unsigned int)info.RegionSize);
    ok(info.State == MEM_COMMIT, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "Protect should have been 0 instead of 0x%x\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);

    ret = UnmapViewOfFile(ptr2);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    ret = UnmapViewOfFile(ptr);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    CloseHandle(mapping);
}

static DWORD (WINAPI *pNtMapViewOfSection)( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                            ULONG zero_bits, SIZE_T commit_size,
                                            const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                            ULONG inherit, ULONG alloc_type, ULONG protect );
static DWORD (WINAPI *pNtUnmapViewOfSection)( HANDLE process, PVOID addr );

static void test_NtMapViewOfSection(void)
{
    HANDLE hProcess;

    static const char testfile[] = "testfile.xxx";
    static const char data[] = "test data for NtMapViewOfSection";
    char buffer[sizeof(data)];
    HANDLE file, mapping;
    void *ptr;
    BOOL ret;
    DWORD status, written;
    SIZE_T size, result;
    LARGE_INTEGER offset;

    pNtMapViewOfSection = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"), "NtMapViewOfSection" );
    pNtUnmapViewOfSection = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"), "NtUnmapViewOfSection" );
    if (!pNtMapViewOfSection || !pNtUnmapViewOfSection)
    {
        skip( "NtMapViewOfSection not found\n" );
        return;
    }

    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create test file\n" );
    WriteFile( file, data, sizeof(data), &written, NULL );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    ptr = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = pNtMapViewOfSection( mapping, hProcess, &ptr, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE );
    ok( !status, "NtMapViewOfSection failed status %x\n", status );

    ret = ReadProcessMemory( hProcess, ptr, buffer, sizeof(buffer), &result );
    ok( ret, "ReadProcessMemory failed\n" );
    ok( result == sizeof(buffer), "ReadProcessMemory didn't read all data (%lx)\n", result );
    ok( !memcmp( buffer, data, sizeof(buffer) ), "Wrong data read\n" );

    status = pNtUnmapViewOfSection( hProcess, ptr );
    ok( !status, "NtUnmapViewOfSection failed status %x\n", status );

    CloseHandle( mapping );
    CloseHandle( file );
    DeleteFileA( testfile );

    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

static void test_BadPtr(void)
{
    void *ptr = (void*)1;
    /* We assume address 1 is not mapped. */
    ok(IsBadReadPtr(ptr,1),"IsBadReadPtr(1) failed.\n");
    ok(IsBadWritePtr(ptr,1),"IsBadWritePtr(1) failed.\n");
    ok(IsBadCodePtr(ptr),"IsBadCodePtr(1) failed.\n");
}

START_TEST(virtual)
{
    int argc;
    char **argv;
    argc = winetest_get_mainargs( &argv );

    if (argc >= 3)
    {
        if (!strcmp(argv[2], "sleep"))
        {
            Sleep(5000); /* spawned process runs for at most 5 seconds */
            return;
        }
        while (1)
        {
            void *mem;
            BOOL ret;
            mem = VirtualAlloc(NULL, 1<<20, MEM_COMMIT|MEM_RESERVE,
                               PAGE_EXECUTE_READWRITE);
            ok(mem != NULL, "VirtualAlloc failed %u\n", GetLastError());
            if (mem == NULL) break;
            ret = VirtualFree(mem, 0, MEM_RELEASE);
            ok(ret, "VirtualFree failed %u\n", GetLastError());
            if (!ret) break;
        }
        return;
    }

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pVirtualAllocEx = (void *) GetProcAddress(hkernel32, "VirtualAllocEx");
    pVirtualFreeEx = (void *) GetProcAddress(hkernel32, "VirtualFreeEx");

    test_VirtualAllocEx();
    test_VirtualAlloc();
    test_MapViewOfFile();
    test_NtMapViewOfSection();
    test_BadPtr();
}
