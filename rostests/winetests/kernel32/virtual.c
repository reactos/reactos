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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/test.h"

#define NUM_THREADS 4
#define MAPPING_SIZE 0x100000

static HINSTANCE hkernel32;
static LPVOID (WINAPI *pVirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
static BOOL   (WINAPI *pVirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);
static UINT   (WINAPI *pGetWriteWatch)(DWORD,LPVOID,SIZE_T,LPVOID*,ULONG_PTR*,ULONG*);
static UINT   (WINAPI *pResetWriteWatch)(LPVOID,SIZE_T);
static NTSTATUS (WINAPI *pNtAreMappedFilesTheSame)(PVOID,PVOID);
static NTSTATUS (WINAPI *pNtMapViewOfSection)(HANDLE, HANDLE, PVOID *, ULONG, SIZE_T, const LARGE_INTEGER *, SIZE_T *, ULONG, ULONG, ULONG);
static DWORD (WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);

/* ############################### */

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ret = CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "error: %u\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    return pi.hProcess;
}

static void test_VirtualAllocEx(void)
{
    const unsigned int alloc_size = 1<<15;
    char *src, *dst;
    SIZE_T bytes_written = 0, bytes_read = 0, i;
    void *addr1, *addr2;
    BOOL b;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;
    HANDLE hProcess;

    /* not exported in all windows-versions  */
    if ((!pVirtualAllocEx) || (!pVirtualFreeEx)) {
        win_skip("Virtual{Alloc,Free}Ex not available\n");
        return;
    }

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hProcess, NULL, alloc_size, MEM_COMMIT,
                           PAGE_EXECUTE_READWRITE);
    if (!addr1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {   /* Win9x */
        win_skip("VirtualAllocEx not implemented\n");
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return;
    }

    src = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    dst = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    for (i = 0; i < alloc_size; i++)
        src[i] = i & 0xff;

    ok(addr1 != NULL, "VirtualAllocEx error %u\n", GetLastError());
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok(b && (bytes_written == alloc_size), "%lu bytes written\n",
       bytes_written);
    b = ReadProcessMemory(hProcess, addr1, dst, alloc_size, &bytes_read);
    ok(b && (bytes_read == alloc_size), "%lu bytes read\n", bytes_read);
    ok(!memcmp(src, dst, alloc_size), "Data from remote process differs\n");

    /* test invalid source buffers */

    b = VirtualProtect( src + 0x2000, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %u\n", GetLastError() );
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %u\n", GetLastError() );
    ok( bytes_written == 0, "%lu bytes written\n", bytes_written );
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );
    ok( bytes_read == 0, "%lu bytes written\n", bytes_read );

    b = VirtualProtect( src, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %u\n", GetLastError() );
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %u\n", GetLastError() );
    ok( bytes_written == 0, "%lu bytes written\n", bytes_written );
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );
    ok( bytes_read == 0, "%lu bytes written\n", bytes_read );

    b = pVirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE);
    ok(b != 0, "VirtualFreeEx, error %u\n", GetLastError());

    VirtualFree( src, 0, MEM_FREE );
    VirtualFree( dst, 0, MEM_FREE );

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

    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);
    memset( addr1, 0x55, 20 );
    ok( *(DWORD *)addr1 == 0x55555555, "wrong data %x\n", *(DWORD *)addr1 );

    addr2 = VirtualAlloc( addr1, 0x1000, MEM_RESET, PAGE_NOACCESS );
    ok( addr2 == addr1 || broken( !addr2 && GetLastError() == ERROR_INVALID_PARAMETER), /* win9x */
        "VirtualAlloc failed err %u\n", GetLastError() );
    ok( *(DWORD *)addr1 == 0x55555555 || *(DWORD *)addr1 == 0, "wrong data %x\n", *(DWORD *)addr1 );
    if (addr2)
    {
        ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
           "VirtualQuery failed\n");
        ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
        ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
        ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);

        addr2 = VirtualAlloc( (char *)addr1 + 0x1000, 0x1000, MEM_RESET, PAGE_NOACCESS );
        ok( (char *)addr2 == (char *)addr1 + 0x1000, "VirtualAlloc failed\n" );

        ok(VirtualQuery(addr2, &info, sizeof(info)) == sizeof(info),
           "VirtualQuery failed\n");
        ok(info.RegionSize == 0xf000, "%lx != 0xf000\n", info.RegionSize);
        ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
        ok(info.Protect == 0, "%x != 0\n", info.Protect);

        addr2 = VirtualAlloc( (char *)addr1 + 0xf000, 0x2000, MEM_RESET, PAGE_NOACCESS );
        ok( !addr2, "VirtualAlloc failed\n" );
        ok( GetLastError() == ERROR_INVALID_ADDRESS, "wrong error %u\n", GetLastError() );
    }

    /* invalid protection values */
    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_RESERVE, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_READONLY | PAGE_EXECUTE);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, PAGE_READWRITE | PAGE_EXECUTE_WRITECOPY, &old_prot),
       "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, 0, &old_prot), "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
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
    const char *name;
    HANDLE file, mapping, map2;
    void *ptr, *ptr2, *addr;
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

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ|FILE_MAP_WRITE, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );
    CloseHandle( map2 );

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    if (!ptr)
    {
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        CloseHandle( map2 );
        ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2, 0, FALSE, 0 );
        ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile succeeded\n" );
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        CloseHandle( map2 );
        ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                               FILE_MAP_READ, FALSE, 0 );
        ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
        ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile NO_ACCESS error %u\n", GetLastError() );
    }
    else win_skip( "no access checks on win9x\n" );

    UnmapViewOfFile( ptr );
    CloseHandle( map2 );
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
    name = "Local\\Foo";
    file = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, name );
    /* nt4 doesn't have Local\\ */
    if (!file && GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        name = "Foo";
        file = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, name );
    }
    ok( file != 0, "CreateFileMapping PAGE_READWRITE error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_READ, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_READ error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
    if (!ptr)
    {
        SIZE_T size;
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
        SetLastError(0xdeadbeef);
        size = VirtualQuery( ptr, &info, sizeof(info) );
        ok( size == sizeof(info),
            "VirtualQuery error %u\n", GetLastError() );
        ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
        ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
        ok( info.AllocationProtect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.AllocationProtect );
        ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
        ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
        ok( info.Protect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.Protect );
    }
    else win_skip( "no access checks on win9x\n" );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_WRITE, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_WRITE error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    if (!ptr)
    {
        SIZE_T size;
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
        SetLastError(0xdeadbeef);
        size = VirtualQuery( ptr, &info, sizeof(info) );
        ok( size == sizeof(info),
            "VirtualQuery error %u\n", GetLastError() );
        ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
        ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
        ok( info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect );
        ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
        ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
        ok( info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect );
    }
    else win_skip( "no access checks on win9x\n" );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    CloseHandle( file );

    /* read/write mapping with SEC_RESERVE */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0, MAPPING_SIZE, NULL);
    ok(mapping != INVALID_HANDLE_VALUE, "CreateFileMappingA failed with error %d\n", GetLastError());

    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %d\n", GetLastError());

    ptr2 = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    /* on NT ptr != ptr2 but on Win9x ptr == ptr2 */
    ok(ptr2 != NULL, "MapViewOfFile failed with error %d\n", GetLastError());
    trace("mapping same section resulted in views %p and %p\n", ptr, ptr2);

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == MAPPING_SIZE, "RegionSize should have been 0x%x but was 0x%lx\n", MAPPING_SIZE, info.RegionSize);
    ok(info.State == MEM_RESERVE, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
    if (info.Type == MEM_PRIVATE)  /* win9x is different for uncommitted mappings */
    {
        ok(info.AllocationProtect == PAGE_NOACCESS,
           "AllocationProtect should have been PAGE_NOACCESS but was 0x%x\n", info.AllocationProtect);
        ok(info.Protect == PAGE_NOACCESS,
           "Protect should have been PAGE_NOACCESS instead of 0x%x\n", info.Protect);
    }
    else
    {
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.Protect == 0, "Protect should have been 0 instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    if (ptr != ptr2)
    {
        ret = VirtualQuery(ptr2, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
        ok(info.BaseAddress == ptr2,
           "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
        ok(info.AllocationBase == ptr2,
           "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.RegionSize == MAPPING_SIZE,
           "RegionSize should have been 0x%x but was 0x%lx\n", MAPPING_SIZE, info.RegionSize);
        ok(info.State == MEM_RESERVE,
           "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
        ok(info.Protect == 0,
           "Protect should have been 0 instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED,
           "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READONLY);
    ok(ptr != NULL, "VirtualAlloc failed with error %d\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == 0x10000, "RegionSize should have been 0x10000 but was 0x%lx\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "State should have been MEM_COMMIT instead of 0x%x\n", info.State);
    ok(info.Protect == PAGE_READONLY, "Protect should have been PAGE_READONLY instead of 0x%x\n", info.Protect);
    if (info.Type == MEM_PRIVATE)  /* win9x is different for uncommitted mappings */
    {
        ok(info.AllocationProtect == PAGE_NOACCESS,
           "AllocationProtect should have been PAGE_NOACCESS but was 0x%x\n", info.AllocationProtect);
    }
    else
    {
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    /* shows that the VirtualAlloc above affects the mapping, not just the
     * virtual memory in this process - it also affects all other processes
     * with a view of the mapping, but that isn't tested here */
    if (ptr != ptr2)
    {
        ret = VirtualQuery(ptr2, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
        ok(info.BaseAddress == ptr2,
           "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
        ok(info.AllocationBase == ptr2,
           "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.RegionSize == 0x10000,
           "RegionSize should have been 0x10000 but was 0x%lx\n", info.RegionSize);
        ok(info.State == MEM_COMMIT,
           "State should have been MEM_COMMIT instead of 0x%x\n", info.State);
        ok(info.Protect == PAGE_READWRITE,
           "Protect should have been PAGE_READWRITE instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    addr = VirtualAlloc( ptr, MAPPING_SIZE, MEM_RESET, PAGE_READONLY );
    ok( addr == ptr || broken(!addr && GetLastError() == ERROR_INVALID_PARAMETER), /* win9x */
        "VirtualAlloc failed with error %u\n", GetLastError() );

    ret = VirtualFree( ptr, 0x10000, MEM_DECOMMIT );
    ok( !ret || broken(ret) /* win9x */, "VirtualFree succeeded\n" );
    if (!ret)
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "VirtualFree failed with %u\n", GetLastError() );

    ret = UnmapViewOfFile(ptr2);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    ret = UnmapViewOfFile(ptr);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    CloseHandle(mapping);

    addr = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_READONLY );
    ok( addr != NULL, "VirtualAlloc failed with error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile(addr), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((char *)addr + 0x3000), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((void *)0xdeadbeef), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
       "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok( VirtualFree(addr, 0, MEM_RELEASE), "VirtualFree failed\n" );

    /* close named mapping handle without unmapping */
    name = "Foo";
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok( ptr != NULL, "MapViewOfFile failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 != 0, "OpenFileMappingA failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(map2);
    ok(ret, "CloseHandle error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %d\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#lx != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#x != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#x != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    todo_wine
    ok( map2 == 0, "OpenFileMappingA succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "OpenFileMappingA set error %d\n", GetLastError() );
    if (map2) CloseHandle(map2); /* FIXME: remove once Wine is fixed */
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed\n" );
    todo_wine
    ok( GetLastError() == ERROR_SUCCESS, "CreateFileMappingA set error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %d\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#lx != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#x != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#x != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    ret = UnmapViewOfFile(ptr);
    ok( ret, "UnmapViewOfFile failed with error %d\n", GetLastError() );

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( ret, "memory is accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.Protect == PAGE_NOACCESS, "got %#x != expected PAGE_NOACCESS\n", info.Protect);
    ok(info.AllocationBase == NULL, "%p != NULL\n", info.AllocationBase);
    ok(info.AllocationProtect == 0, "%#x != 0\n", info.AllocationProtect);
    ok(info.State == MEM_FREE, "%#x != MEM_FREE\n", info.State);
    ok(info.Type == 0, "%#x != 0\n", info.Type);

    SetLastError(0xdeadbeef);
    file = CreateFileA(testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    SetFilePointer(file, 4096, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok( ptr != NULL, "MapViewOfFile failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 != 0, "OpenFileMappingA failed with error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(map2);
    ok(ret, "CloseHandle error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %d\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#lx != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#x != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#x != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    todo_wine
    ok( map2 == 0, "OpenFileMappingA succeeded\n" );
    todo_wine
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "OpenFileMappingA set error %d\n", GetLastError() );
    CloseHandle(map2);
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed\n" );
    todo_wine
    ok( GetLastError() == ERROR_SUCCESS, "CreateFileMappingA set error %d\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %d\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#lx != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#x != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#x != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#x != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#x != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    ret = UnmapViewOfFile(ptr);
    ok( ret, "UnmapViewOfFile failed with error %d\n", GetLastError() );

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( ret, "memory is accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.Protect == PAGE_NOACCESS, "got %#x != expected PAGE_NOACCESS\n", info.Protect);
    ok(info.AllocationBase == NULL, "%p != NULL\n", info.AllocationBase);
    ok(info.AllocationProtect == 0, "%#x != 0\n", info.AllocationProtect);
    ok(info.State == MEM_FREE, "%#x != MEM_FREE\n", info.State);
    ok(info.Type == 0, "%#x != 0\n", info.Type);

    CloseHandle(file);
    DeleteFileA(testfile);
}

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

    if (!pNtMapViewOfSection || !pNtUnmapViewOfSection)
    {
        win_skip( "NtMapViewOfSection not available\n" );
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

static void test_NtAreMappedFilesTheSame(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, file2, mapping, map2;
    void *ptr, *ptr2;
    NTSTATUS status;
    char path[MAX_PATH];

    if (!pNtAreMappedFilesTheSame)
    {
        win_skip( "NtAreMappedFilesTheSame not available\n" );
        return;
    }

    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );

    file2 = CreateFileA( testfile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );

    ptr2 = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );

    map2 = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );
    CloseHandle( file2 );

    status = pNtAreMappedFilesTheSame( ptr, ptr );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (char *)ptr + 30 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)0xdeadbeef );
    ok( status == STATUS_CONFLICTING_ADDRESSES || status == STATUS_INVALID_ADDRESS,
        "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)GetProcessHeap() );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( NULL, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %x\n", status );

    ptr2 = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    ok( ptr2 != NULL, "VirtualAlloc error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %x\n", status );
    VirtualFree( ptr2, 0, MEM_RELEASE );

    UnmapViewOfFile( ptr );
    CloseHandle( mapping );
    CloseHandle( file );

    status = pNtAreMappedFilesTheSame( GetModuleHandleA("ntdll.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       (char *)GetModuleHandleA("kernel32.dll") + 4096 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );

    GetSystemDirectoryA( path, MAX_PATH );
    strcat( path, "\\kernel32.dll" );
    file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    todo_wine
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );

    file2 = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );
    CloseHandle( file2 );

    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    CloseHandle( file );
    DeleteFileA( testfile );
}

static void test_CreateFileMapping(void)
{
    HANDLE handle, handle2;

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "Wine Test Mapping");
    ok( handle != NULL, "CreateFileMapping failed with error %u\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                  "Wine Test Mapping");
    ok( handle2 != NULL, "CreateFileMapping failed with error %d\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "WINE TEST MAPPING");
    ok( handle2 != NULL, "CreateFileMapping failed with error %d\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "Wine Test Mapping");
    ok( handle2 != NULL, "OpenFileMapping failed with error %d\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "WINE TEST MAPPING");
    ok( !handle2, "OpenFileMapping succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME /* win9x */,
        "wrong error %u\n", GetLastError());

    CloseHandle( handle );
}

static void test_IsBadReadPtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackvar;

    ret = IsBadReadPtr(NULL, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(NULL, 1);
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);

    ret = IsBadReadPtr(ptr, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(ptr, 1);
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);

    ret = IsBadReadPtr(&stackvar, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(&stackvar, sizeof(char));
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);
}

static void test_IsBadWritePtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackval;

    ret = IsBadWritePtr(NULL, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(NULL, 1);
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);

    ret = IsBadWritePtr(ptr, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(ptr, 1);
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);

    ret = IsBadWritePtr(&stackval, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(&stackval, sizeof(char));
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);
}

static void test_IsBadCodePtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackval;

    ret = IsBadCodePtr(NULL);
    ok(ret == TRUE, "Expected IsBadCodePtr to return TRUE, got %d\n", ret);

    ret = IsBadCodePtr(ptr);
    ok(ret == TRUE, "Expected IsBadCodePtr to return TRUE, got %d\n", ret);

    ret = IsBadCodePtr((void *)&stackval);
    ok(ret == FALSE, "Expected IsBadCodePtr to return FALSE, got %d\n", ret);
}

static void test_write_watch(void)
{
    char *base;
    DWORD ret, size, old_prot;
    MEMORY_BASIC_INFORMATION info;
    void *results[64];
    ULONG_PTR count;
    ULONG pagesize;

    if (!pGetWriteWatch || !pResetWriteWatch)
    {
        win_skip( "GetWriteWatch not supported\n" );
        return;
    }

    size = 0x10000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE );
    if (!base &&
        (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED))
    {
        win_skip( "MEM_WRITE_WATCH not supported\n" );
        return;
    }
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == PAGE_READWRITE, "wrong AllocationProtect %x\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%x\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%x\n", info.Type );

    count = 64;
    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, NULL, size, results, &count, &pagesize );
    ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        broken( GetLastError() == 0xdeadbeef ), /* win98 */
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, GetModuleHandle(0), size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    }
    else  /* win98 */
    {
        ok( count == 0, "wrong count %lu\n", count );
    }

    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    base[pagesize + 1] = 0x44;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    base[2*pagesize + 3] = 0x11;
    base[4*pagesize + 8] = 0x11;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    count = 64;
    ret = pGetWriteWatch( 0, base + 3*pagesize, 2*pagesize, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    ret = pResetWriteWatch( base, 3*pagesize );
    ok( !ret, "pResetWriteWatch failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    *(DWORD *)(base + 2*pagesize - 2) = 0xdeadbeef;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 3, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 2*pagesize, "wrong result %p\n", results[1] );
    ok( results[2] == base + 4*pagesize, "wrong result %p\n", results[2] );

    count = 1;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    /* changing protections doesn't affect watches */

    ret = VirtualProtect( base, 3*pagesize, PAGE_READONLY, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %x\n", old_prot );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == 3*pagesize, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READONLY, "wrong Protect 0x%x\n", info.Protect );

    ret = VirtualProtect( base, 3*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READONLY, "wrong old prot %x\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == size, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%x\n", info.Protect );

    /* some invalid parameter tests */

    SetLastError( 0xdeadbeef );
    count = 0;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pGetWriteWatch( 0, base, size, results, NULL, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, results, &count, NULL );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size * 2, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base + size - pagesize, pagesize + 1, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( base, 0 );
        ok( ret == ~0u, "ResetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( GetModuleHandle(0), size );
        ok( ret == ~0u, "ResetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    }
    else  /* win98 is completely different */
    {
        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ERROR_INVALID_PARAMETER, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );

        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        ret = pResetWriteWatch( base, 0 );
        ok( !ret, "ResetWriteWatch failed %u\n", ret );

        ret = pResetWriteWatch( GetModuleHandle(0), size );
        ok( !ret, "ResetWriteWatch failed %u\n", ret );
    }

    VirtualFree( base, 0, MEM_FREE );

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    VirtualFree( base, 0, MEM_FREE );

    base = VirtualAlloc( 0, size, MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( !base, "VirtualAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    /* initial protect doesn't matter */

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    base = VirtualAlloc( base, size, MEM_COMMIT, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    ret = VirtualProtect( base, 6*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_NOACCESS, "wrong old prot %x\n", old_prot );

    base[5*pagesize + 200] = 3;

    ret = VirtualProtect( base, 6*pagesize, PAGE_NOACCESS, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %x\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    ret = VirtualFree( base, size, MEM_DECOMMIT );
    ok( ret, "VirtualFree failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1 || broken(count == 0), /* win98 */
        "wrong count %lu\n", count );
    if (count) ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    VirtualFree( base, 0, MEM_FREE );
}

static void test_VirtualProtect(void)
{
    static const struct test_data
    {
        DWORD prot_set, prot_get;
    } td[] =
    {
        { 0, 0 }, /* 0x00 */
        { PAGE_NOACCESS, PAGE_NOACCESS }, /* 0x01 */
        { PAGE_READONLY, PAGE_READONLY }, /* 0x02 */
        { PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x03 */
        { PAGE_READWRITE, PAGE_READWRITE }, /* 0x04 */
        { PAGE_READWRITE | PAGE_NOACCESS, 0 }, /* 0x05 */
        { PAGE_READWRITE | PAGE_READONLY, 0 }, /* 0x06 */
        { PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x07 */
        { PAGE_WRITECOPY, 0 }, /* 0x08 */
        { PAGE_WRITECOPY | PAGE_NOACCESS, 0 }, /* 0x09 */
        { PAGE_WRITECOPY | PAGE_READONLY, 0 }, /* 0x0a */
        { PAGE_WRITECOPY | PAGE_NOACCESS | PAGE_READONLY, 0 }, /* 0x0b */
        { PAGE_WRITECOPY | PAGE_READWRITE, 0 }, /* 0x0c */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_NOACCESS, 0 }, /* 0x0d */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY, 0 }, /* 0x0e */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, 0 }, /* 0x0f */

        { PAGE_EXECUTE, PAGE_EXECUTE }, /* 0x10 */
        { PAGE_EXECUTE_READ, PAGE_EXECUTE_READ }, /* 0x20 */
        { PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0x30 */
        { PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_READWRITE }, /* 0x40 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, 0 }, /* 0x50 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, 0 }, /* 0x60 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0x70 */
        { PAGE_EXECUTE_WRITECOPY, 0 }, /* 0x80 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE, 0 }, /* 0x90 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ, 0 }, /* 0xa0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 }, /* 0xb0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE, 0 }, /* 0xc0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, 0 }, /* 0xd0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, 0 }, /* 0xe0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, 0 } /* 0xf0 */
    };
    char *base, *ptr;
    DWORD ret, old_prot, rw_prot, exec_prot, i, j;
    MEMORY_BASIC_INFORMATION info;
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    SetLastError(0xdeadbeef);
    base = VirtualAlloc(0, si.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    ok(base != NULL, "VirtualAlloc failed %d\n", GetLastError());

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(base, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %d\n", GetLastError());
        ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
        ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
        ok(info.Protect == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == PAGE_NOACCESS, "%d: %#x != PAGE_NOACCESS\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == MEM_PRIVATE, "%d: %#x != MEM_PRIVATE\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, td[i].prot_set, &old_prot);
        if (td[i].prot_get)
        {
            ok(ret, "%d: VirtualProtect error %d\n", i, GetLastError());
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == td[i].prot_get, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot_get);
            ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_NOACCESS, "%d: %#x != PAGE_NOACCESS\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_PRIVATE, "%d: %#x != MEM_PRIVATE\n", i, info.Type);
        }
        else
        {
            ok(!ret, "%d: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
        ok(ret, "%d: VirtualProtect error %d\n", i, GetLastError());
        if (td[i].prot_get)
            ok(old_prot == td[i].prot_get, "%d: got %#x != expected %#x\n", i, old_prot, td[i].prot_get);
        else
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);
    }

    exec_prot = 0;

    for (i = 0; i <= 4; i++)
    {
        rw_prot = 0;

        for (j = 0; j <= 4; j++)
        {
            DWORD prot = exec_prot | rw_prot;

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, prot);
            if ((rw_prot && exec_prot) || (!rw_prot && !exec_prot))
            {
                ok(!ptr, "VirtualAlloc(%02x) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            }
            else
            {
                if (prot & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                {
                    ok(!ptr, "VirtualAlloc(%02x) should fail\n", prot);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
                }
                else
                {
                    ok(ptr != NULL, "VirtualAlloc(%02x) error %d\n", prot, GetLastError());
                    ok(ptr == base, "expected %p, got %p\n", base, ptr);
                }
            }

            SetLastError(0xdeadbeef);
            ret = VirtualProtect(base, si.dwPageSize, prot, &old_prot);
            if ((rw_prot && exec_prot) || (!rw_prot && !exec_prot))
            {
                ok(!ret, "VirtualProtect(%02x) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
            }
            else
            {
                if (prot & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                {
                    ok(!ret, "VirtualProtect(%02x) should fail\n", prot);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
                }
                else
                    ok(ret, "VirtualProtect(%02x) error %d\n", prot, GetLastError());
            }

            rw_prot = 1 << j;
        }

        exec_prot = 1 << (i + 4);
    }

    VirtualFree(base, 0, MEM_FREE);
}

static BOOL is_mem_writable(DWORD prot)
{
    switch (prot & 0xff)
    {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return TRUE;

        default:
            return FALSE;
    }
}

static void test_VirtualAlloc_protection(void)
{
    static const struct test_data
    {
        DWORD prot;
        BOOL success;
    } td[] =
    {
        { 0, FALSE }, /* 0x00 */
        { PAGE_NOACCESS, TRUE }, /* 0x01 */
        { PAGE_READONLY, TRUE }, /* 0x02 */
        { PAGE_READONLY | PAGE_NOACCESS, FALSE }, /* 0x03 */
        { PAGE_READWRITE, TRUE }, /* 0x04 */
        { PAGE_READWRITE | PAGE_NOACCESS, FALSE }, /* 0x05 */
        { PAGE_READWRITE | PAGE_READONLY, FALSE }, /* 0x06 */
        { PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, FALSE }, /* 0x07 */
        { PAGE_WRITECOPY, FALSE }, /* 0x08 */
        { PAGE_WRITECOPY | PAGE_NOACCESS, FALSE }, /* 0x09 */
        { PAGE_WRITECOPY | PAGE_READONLY, FALSE }, /* 0x0a */
        { PAGE_WRITECOPY | PAGE_NOACCESS | PAGE_READONLY, FALSE }, /* 0x0b */
        { PAGE_WRITECOPY | PAGE_READWRITE, FALSE }, /* 0x0c */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_NOACCESS, FALSE }, /* 0x0d */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY, FALSE }, /* 0x0e */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, FALSE }, /* 0x0f */

        { PAGE_EXECUTE, TRUE }, /* 0x10 */
        { PAGE_EXECUTE_READ, TRUE }, /* 0x20 */
        { PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE }, /* 0x30 */
        { PAGE_EXECUTE_READWRITE, TRUE }, /* 0x40 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, FALSE }, /* 0x50 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, FALSE }, /* 0x60 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE }, /* 0x70 */
        { PAGE_EXECUTE_WRITECOPY, FALSE }, /* 0x80 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE, FALSE }, /* 0x90 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ, FALSE }, /* 0xa0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE }, /* 0xb0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE, FALSE }, /* 0xc0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, FALSE }, /* 0xd0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, FALSE }, /* 0xe0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE } /* 0xf0 */
    };
    char *base, *ptr;
    DWORD ret, i;
    MEMORY_BASIC_INFORMATION info;
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        base = VirtualAlloc(0, si.dwPageSize, MEM_COMMIT, td[i].prot);

        if (td[i].success)
        {
            ok(base != NULL, "%d: VirtualAlloc failed %d\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == td[i].prot, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot);
            ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == td[i].prot, "%d: %#x != %#x\n", i, info.AllocationProtect, td[i].prot);
            ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_PRIVATE, "%d: %#x != MEM_PRIVATE\n", i, info.Type);

            if (is_mem_writable(info.Protect))
            {
                base[0] = 0xfe;

                SetLastError(0xdeadbeef);
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "VirtualQuery failed %d\n", GetLastError());
                ok(info.Protect == td[i].prot, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot);
            }

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, td[i].prot);
            ok(ptr == base, "%d: VirtualAlloc failed %d\n", i, GetLastError());

            VirtualFree(base, 0, MEM_FREE);
        }
        else
        {
            ok(!base, "%d: VirtualAlloc should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        }
    }
}

static void test_CreateFileMapping_protection(void)
{
    static const struct test_data
    {
        DWORD prot;
        BOOL success;
        DWORD prot_after_write;
    } td[] =
    {
        { 0, FALSE, 0 }, /* 0x00 */
        { PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x01 */
        { PAGE_READONLY, TRUE, PAGE_READONLY }, /* 0x02 */
        { PAGE_READONLY | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x03 */
        { PAGE_READWRITE, TRUE, PAGE_READWRITE }, /* 0x04 */
        { PAGE_READWRITE | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x05 */
        { PAGE_READWRITE | PAGE_READONLY, FALSE, PAGE_NOACCESS }, /* 0x06 */
        { PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x07 */
        { PAGE_WRITECOPY, TRUE, PAGE_READWRITE }, /* 0x08 */
        { PAGE_WRITECOPY | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x09 */
        { PAGE_WRITECOPY | PAGE_READONLY, FALSE, PAGE_NOACCESS }, /* 0x0a */
        { PAGE_WRITECOPY | PAGE_NOACCESS | PAGE_READONLY, FALSE, PAGE_NOACCESS }, /* 0x0b */
        { PAGE_WRITECOPY | PAGE_READWRITE, FALSE, PAGE_NOACCESS }, /* 0x0c */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x0d */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY, FALSE, PAGE_NOACCESS }, /* 0x0e */
        { PAGE_WRITECOPY | PAGE_READWRITE | PAGE_READONLY | PAGE_NOACCESS, FALSE, PAGE_NOACCESS }, /* 0x0f */

        { PAGE_EXECUTE, FALSE, PAGE_EXECUTE }, /* 0x10 */
        { PAGE_EXECUTE_READ, TRUE, PAGE_EXECUTE_READ }, /* 0x20 */
        { PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE, PAGE_EXECUTE_READ }, /* 0x30 */
        { PAGE_EXECUTE_READWRITE, TRUE, PAGE_EXECUTE_READWRITE }, /* 0x40 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, FALSE, PAGE_NOACCESS }, /* 0x50 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, FALSE, PAGE_NOACCESS }, /* 0x60 */
        { PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE, PAGE_NOACCESS }, /* 0x70 */
        { PAGE_EXECUTE_WRITECOPY, TRUE, PAGE_EXECUTE_READWRITE }, /* 0x80 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE, FALSE, PAGE_NOACCESS }, /* 0x90 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ, FALSE, PAGE_NOACCESS }, /* 0xa0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE, PAGE_NOACCESS }, /* 0xb0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE, FALSE, PAGE_NOACCESS }, /* 0xc0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE, FALSE, PAGE_NOACCESS }, /* 0xd0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ, FALSE, PAGE_NOACCESS }, /* 0xe0 */
        { PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE, FALSE, PAGE_NOACCESS } /* 0xf0 */
    };
    char *base, *ptr;
    DWORD ret, i, alloc_prot, prot, old_prot;
    MEMORY_BASIC_INFORMATION info;
    SYSTEM_INFO si;
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    HANDLE hfile, hmap;
    BOOL page_exec_supported = TRUE;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    GetTempPath(MAX_PATH, temp_path);
    GetTempFileName(temp_path, "map", 0, file_name);

    SetLastError(0xdeadbeef);
    hfile = CreateFile(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %d\n", file_name, GetLastError());
    SetFilePointer(hfile, si.dwPageSize, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        hmap = CreateFileMapping(hfile, NULL, td[i].prot | SEC_COMMIT, 0, si.dwPageSize, NULL);

        if (td[i].success)
        {
            if (!hmap)
            {
                trace("%d: CreateFileMapping(%04x) failed: %d\n", i, td[i].prot, GetLastError());
                /* NT4 and win2k don't support EXEC on file mappings */
                if (td[i].prot == PAGE_EXECUTE_READ || td[i].prot == PAGE_EXECUTE_READWRITE)
                {
                    page_exec_supported = FALSE;
                    ok(broken(!hmap), "%d: CreateFileMapping doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
                if (td[i].prot == PAGE_EXECUTE_WRITECOPY)
                {
                    page_exec_supported = FALSE;
                    ok(broken(!hmap), "%d: CreateFileMapping doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                    continue;
                }
            }
            ok(hmap != 0, "%d: CreateFileMapping(%04x) error %d\n", i, td[i].prot, GetLastError());

            base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
            ok(base != NULL, "%d: MapViewOfFile failed %d\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == PAGE_READONLY, "%d: got %#x != expected PAGE_READONLY\n", i, info.Protect);
            ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_READONLY, "%d: %#x != PAGE_READONLY\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_MAPPED, "%d: %#x != MEM_MAPPED\n", i, info.Type);

            if (is_mem_writable(info.Protect))
            {
                base[0] = 0xfe;

                SetLastError(0xdeadbeef);
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "VirtualQuery failed %d\n", GetLastError());
                ok(info.Protect == td[i].prot, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot);
            }

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, td[i].prot);
            ok(!ptr, "%d: VirtualAlloc(%02x) should fail\n", i, td[i].prot);
            /* FIXME: remove once Wine is fixed */
            if (td[i].prot == PAGE_WRITECOPY || td[i].prot == PAGE_EXECUTE_WRITECOPY)
todo_wine
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
            else
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualProtect(base, si.dwPageSize, td[i].prot, &old_prot);
            if (td[i].prot == PAGE_READONLY || td[i].prot == PAGE_WRITECOPY)
                ok(ret, "%d: VirtualProtect(%02x) error %d\n", i, td[i].prot, GetLastError());
            else
            {
                ok(!ret, "%d: VirtualProtect(%02x) should fail\n", i, td[i].prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
            }

            UnmapViewOfFile(base);
            CloseHandle(hmap);
        }
        else
        {
            ok(!hmap, "%d: CreateFileMapping should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        }
    }

    if (page_exec_supported) alloc_prot = PAGE_EXECUTE_READWRITE;
    else alloc_prot = PAGE_READWRITE;
    SetLastError(0xdeadbeef);
    hmap = CreateFileMapping(hfile, NULL, alloc_prot, 0, si.dwPageSize, NULL);
    ok(hmap != 0, "%d: CreateFileMapping error %d\n", i, GetLastError());

    SetLastError(0xdeadbeef);
    base = MapViewOfFile(hmap, FILE_MAP_READ | FILE_MAP_WRITE | (page_exec_supported ? FILE_MAP_EXECUTE : 0), 0, 0, 0);
    ok(base != NULL, "MapViewOfFile failed %d\n", GetLastError());

    old_prot = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(ret, "VirtualProtect error %d\n", GetLastError());
    ok(old_prot == alloc_prot, "got %#x != expected %#x\n", old_prot, alloc_prot);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(base, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %d\n", GetLastError());
        ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
        ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
        ok(info.Protect == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == alloc_prot, "%d: %#x != %#x\n", i, info.AllocationProtect, alloc_prot);
        ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
        ok(info.Type == MEM_MAPPED, "%d: %#x != MEM_MAPPED\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, td[i].prot, &old_prot);
        if (td[i].success || td[i].prot == PAGE_NOACCESS || td[i].prot == PAGE_EXECUTE)
        {
            if (!ret)
            {
                /* win2k and XP don't support EXEC on file mappings */
                if (td[i].prot == PAGE_EXECUTE)
                {
                    ok(broken(!ret), "%d: VirtualProtect doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* NT4 and win2k don't support EXEC on file mappings */
                if (td[i].prot == PAGE_EXECUTE_READ || td[i].prot == PAGE_EXECUTE_READWRITE)
                {
                    ok(broken(!ret), "%d: VirtualProtect doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
                if (td[i].prot == PAGE_EXECUTE_WRITECOPY)
                {
                    ok(broken(!ret), "%d: VirtualProtect doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                    continue;
                }
            }

            ok(ret, "%d: VirtualProtect error %d\n", i, GetLastError());
            ok(old_prot == PAGE_NOACCESS, "%d: got %#x != expected PAGE_NOACCESS\n", i, old_prot);

            prot = td[i].prot;
            /* looks strange but Windows doesn't do this for PAGE_WRITECOPY */
            if (prot == PAGE_EXECUTE_WRITECOPY) prot = PAGE_EXECUTE_READWRITE;

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %d\n", GetLastError());
            ok(info.BaseAddress == base, "%d: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%d: got %#lx != expected %#x\n", i, info.RegionSize, si.dwPageSize);
            /* FIXME: remove the condition below once Wine is fixed */
            if (td[i].prot == PAGE_EXECUTE_WRITECOPY)
                todo_wine ok(info.Protect == prot, "%d: got %#x != expected %#x\n", i, info.Protect, prot);
            else
                ok(info.Protect == prot, "%d: got %#x != expected %#x\n", i, info.Protect, prot);
            ok(info.AllocationBase == base, "%d: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == alloc_prot, "%d: %#x != %#x\n", i, info.AllocationProtect, alloc_prot);
            ok(info.State == MEM_COMMIT, "%d: %#x != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_MAPPED, "%d: %#x != MEM_MAPPED\n", i, info.Type);

            if (is_mem_writable(info.Protect))
            {
                base[0] = 0xfe;

                SetLastError(0xdeadbeef);
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "VirtualQuery failed %d\n", GetLastError());
                /* FIXME: remove the condition below once Wine is fixed */
                if (td[i].prot == PAGE_WRITECOPY || td[i].prot == PAGE_EXECUTE_WRITECOPY)
                    todo_wine ok(info.Protect == td[i].prot_after_write, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot_after_write);
                else
                    ok(info.Protect == td[i].prot_after_write, "%d: got %#x != expected %#x\n", i, info.Protect, td[i].prot_after_write);
            }
        }
        else
        {
            ok(!ret, "%d: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
            continue;
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
        ok(ret, "%d: VirtualProtect error %d\n", i, GetLastError());
        /* FIXME: remove the condition below once Wine is fixed */
        if (td[i].prot == PAGE_WRITECOPY || td[i].prot == PAGE_EXECUTE_WRITECOPY)
            todo_wine ok(old_prot == td[i].prot_after_write, "%d: got %#x != expected %#x\n", i, old_prot, td[i].prot_after_write);
        else
            ok(old_prot == td[i].prot_after_write, "%d: got %#x != expected %#x\n", i, old_prot, td[i].prot_after_write);
    }

    UnmapViewOfFile(base);
    CloseHandle(hmap);

    CloseHandle(hfile);
    DeleteFile(file_name);
}

#define ACCESS_READ      0x01
#define ACCESS_WRITE     0x02
#define ACCESS_EXECUTE   0x04
#define ACCESS_WRITECOPY 0x08

static DWORD page_prot_to_access(DWORD prot)
{
    switch (prot)
    {
    case PAGE_READWRITE:
        return ACCESS_READ | ACCESS_WRITE;

    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
        return ACCESS_READ | ACCESS_EXECUTE;

    case PAGE_EXECUTE_READWRITE:
        return ACCESS_READ | ACCESS_WRITE | ACCESS_WRITECOPY | ACCESS_EXECUTE;

    case PAGE_EXECUTE_WRITECOPY:
        return ACCESS_READ | ACCESS_WRITECOPY | ACCESS_EXECUTE;

    case PAGE_READONLY:
        return ACCESS_READ;

    case PAGE_WRITECOPY:
        return ACCESS_READ;

    default:
        return 0;
    }
}

static BOOL is_compatible_protection(DWORD map_prot, DWORD view_prot, DWORD prot)
{
    DWORD map_access, view_access, prot_access;

    map_access = page_prot_to_access(map_prot);
    view_access = page_prot_to_access(view_prot);
    prot_access = page_prot_to_access(prot);

    if (view_access == prot_access) return TRUE;
    if (!view_access) return FALSE;

    if ((view_access & prot_access) != prot_access) return FALSE;
    if ((map_access & prot_access) == prot_access) return TRUE;

    return FALSE;
}

static DWORD map_prot_to_access(DWORD prot)
{
    switch (prot)
    {
    case PAGE_READWRITE:
    case PAGE_EXECUTE_READWRITE:
        return SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE | SECTION_MAP_EXECUTE_EXPLICIT | SECTION_QUERY;
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        return SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_EXECUTE_EXPLICIT | SECTION_QUERY;
    default:
        return 0;
    }
}

static BOOL is_compatible_access(DWORD map_prot, DWORD view_prot)
{
    DWORD access = map_prot_to_access(map_prot);
    if (!view_prot) view_prot = SECTION_MAP_READ;
    return (view_prot & access) == view_prot;
}

static void *map_view_of_file(HANDLE handle, DWORD access)
{
    NTSTATUS status;
    LARGE_INTEGER offset;
    SIZE_T count;
    ULONG protect;
    BOOL exec;
    void *addr;

    if (!pNtMapViewOfSection) return NULL;

    count = 0;
    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;

    exec = access & FILE_MAP_EXECUTE;
    access &= ~FILE_MAP_EXECUTE;

    if (access == FILE_MAP_COPY)
    {
        if (exec)
            protect = PAGE_EXECUTE_WRITECOPY;
        else
            protect = PAGE_WRITECOPY;
    }
    else if (access & FILE_MAP_WRITE)
    {
        if (exec)
            protect = PAGE_EXECUTE_READWRITE;
        else
            protect = PAGE_READWRITE;
    }
    else if (access & FILE_MAP_READ)
    {
        if (exec)
            protect = PAGE_EXECUTE_READ;
        else
            protect = PAGE_READONLY;
    }
    else protect = PAGE_NOACCESS;

    addr = NULL;
    status = pNtMapViewOfSection(handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                 &count, 1 /* ViewShare */, 0, protect);
    if (status)
    {
        /* for simplicity */
        SetLastError(ERROR_ACCESS_DENIED);
        addr = NULL;
    }
    return addr;
}

static void test_mapping(void)
{
    static const DWORD page_prot[] =
    {
        PAGE_NOACCESS, PAGE_READONLY, PAGE_READWRITE, PAGE_WRITECOPY,
        PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY
    };
    static const struct
    {
        DWORD access, prot;
    } view[] =
    {
        { 0, PAGE_NOACCESS }, /* 0x00 */
        { FILE_MAP_COPY, PAGE_WRITECOPY }, /* 0x01 */
        { FILE_MAP_WRITE, PAGE_READWRITE }, /* 0x02 */
        { FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_READWRITE }, /* 0x03 */
        { FILE_MAP_READ, PAGE_READONLY }, /* 0x04 */
        { FILE_MAP_READ | FILE_MAP_COPY, PAGE_READONLY }, /* 0x05 */
        { FILE_MAP_READ | FILE_MAP_WRITE, PAGE_READWRITE }, /* 0x06 */
        { FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_READWRITE }, /* 0x07 */
        { SECTION_MAP_EXECUTE, PAGE_NOACCESS }, /* 0x08 */
        { SECTION_MAP_EXECUTE | FILE_MAP_COPY, PAGE_NOACCESS }, /* 0x09 */
        { SECTION_MAP_EXECUTE | FILE_MAP_WRITE, PAGE_READWRITE }, /* 0x0a */
        { SECTION_MAP_EXECUTE | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_READWRITE }, /* 0x0b */
        { SECTION_MAP_EXECUTE | FILE_MAP_READ, PAGE_READONLY }, /* 0x0c */
        { SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_COPY, PAGE_READONLY }, /* 0x0d */
        { SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE, PAGE_READWRITE }, /* 0x0e */
        { SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_READWRITE }, /* 0x0f */
        { FILE_MAP_EXECUTE, PAGE_NOACCESS }, /* 0x20 */
        { FILE_MAP_EXECUTE | FILE_MAP_COPY, PAGE_EXECUTE_WRITECOPY }, /* 0x21 */
        { FILE_MAP_EXECUTE | FILE_MAP_WRITE, PAGE_EXECUTE_READWRITE }, /* 0x22 */
        { FILE_MAP_EXECUTE | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_EXECUTE_READWRITE }, /* 0x23 */
        { FILE_MAP_EXECUTE | FILE_MAP_READ, PAGE_EXECUTE_READ }, /* 0x24 */
        { FILE_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_COPY, PAGE_EXECUTE_READ }, /* 0x25 */
        { FILE_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE, PAGE_EXECUTE_READWRITE }, /* 0x26 */
        { FILE_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_EXECUTE_READWRITE }, /* 0x27 */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE, PAGE_NOACCESS }, /* 0x28 */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_COPY, PAGE_NOACCESS }, /* 0x29 */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_WRITE, PAGE_EXECUTE_READWRITE }, /* 0x2a */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_EXECUTE_READWRITE }, /* 0x2b */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_READ, PAGE_EXECUTE_READ }, /* 0x2c */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_COPY, PAGE_EXECUTE_READ }, /* 0x2d */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE, PAGE_EXECUTE_READWRITE }, /* 0x2e */
        { FILE_MAP_EXECUTE | SECTION_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE | FILE_MAP_COPY, PAGE_EXECUTE_READWRITE } /* 0x2f */
    };
    void *base, *nt_base, *ptr;
    DWORD i, j, k, ret, old_prot, prev_prot;
    SYSTEM_INFO si;
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    HANDLE hfile, hmap;
    MEMORY_BASIC_INFORMATION info, nt_info;

    GetSystemInfo(&si);
    trace("system page size %#x\n", si.dwPageSize);

    GetTempPath(MAX_PATH, temp_path);
    GetTempFileName(temp_path, "map", 0, file_name);

    SetLastError(0xdeadbeef);
    hfile = CreateFile(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %d\n", file_name, GetLastError());
    SetFilePointer(hfile, si.dwPageSize, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    for (i = 0; i < sizeof(page_prot)/sizeof(page_prot[0]); i++)
    {
        SetLastError(0xdeadbeef);
        hmap = CreateFileMapping(hfile, NULL, page_prot[i] | SEC_COMMIT, 0, si.dwPageSize, NULL);

        if (page_prot[i] == PAGE_NOACCESS)
        {
            HANDLE hmap2;

            ok(!hmap, "CreateFileMapping(PAGE_NOACCESS) should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

            /* A trick to create a not accessible mapping */
            SetLastError(0xdeadbeef);
            hmap = CreateFileMapping(hfile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, si.dwPageSize, NULL);
            ok(hmap != 0, "CreateFileMapping(PAGE_READWRITE) error %d\n", GetLastError());
            SetLastError(0xdeadbeef);
            ret = DuplicateHandle(GetCurrentProcess(), hmap, GetCurrentProcess(), &hmap2, 0, FALSE, 0);
            ok(ret, "DuplicateHandle error %d\n", GetLastError());
            CloseHandle(hmap);
            hmap = hmap2;
        }

        if (!hmap)
        {
            trace("%d: CreateFileMapping(%04x) failed: %d\n", i, page_prot[i], GetLastError());

            /* NT4 and win2k don't support EXEC on file mappings */
            if (page_prot[i] == PAGE_EXECUTE_READ || page_prot[i] == PAGE_EXECUTE_READWRITE)
            {
                ok(broken(!hmap), "%d: CreateFileMapping doesn't support PAGE_EXECUTE\n", i);
                continue;
            }
            /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
            if (page_prot[i] == PAGE_EXECUTE_WRITECOPY)
            {
                ok(broken(!hmap), "%d: CreateFileMapping doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                continue;
            }
        }

        ok(hmap != 0, "%d: CreateFileMapping(%04x) error %d\n", i, page_prot[i], GetLastError());

        for (j = 0; j < sizeof(view)/sizeof(view[0]); j++)
        {
            nt_base = map_view_of_file(hmap, view[j].access);
            if (nt_base)
            {
                SetLastError(0xdeadbeef);
                ret = VirtualQuery(nt_base, &nt_info, sizeof(nt_info));
                ok(ret, "%d: VirtualQuery failed %d\n", j, GetLastError());
                UnmapViewOfFile(nt_base);
            }

            SetLastError(0xdeadbeef);
            base = MapViewOfFile(hmap, view[j].access, 0, 0, 0);

            /* Vista+ supports FILE_MAP_EXECUTE properly, earlier versions don't */
            ok(!nt_base == !base ||
               broken((view[j].access & FILE_MAP_EXECUTE) && !nt_base != !base),
               "%d: (%04x/%04x) NT %p kernel %p\n", j, page_prot[i], view[j].access, nt_base, base);

            if (!is_compatible_access(page_prot[i], view[j].access))
            {
                ok(!base, "%d: MapViewOfFile(%04x/%04x) should fail\n", j, page_prot[i], view[j].access);
                ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());
                continue;
            }

            /* Vista+ properly supports FILE_MAP_EXECUTE, earlier versions don't */
            if (!base && (view[j].access & FILE_MAP_EXECUTE))
            {
                ok(broken(!base), "%d: MapViewOfFile(%04x/%04x) failed %d\n", j, page_prot[i], view[j].access, GetLastError());
                continue;
            }

            ok(base != NULL, "%d: MapViewOfFile(%04x/%04x) failed %d\n", j, page_prot[i], view[j].access, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "%d: VirtualQuery failed %d\n", j, GetLastError());
            ok(info.BaseAddress == base, "%d: (%04x) got %p, expected %p\n", j, view[j].access, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%d: (%04x) got %#lx != expected %#x\n", j, view[j].access, info.RegionSize, si.dwPageSize);
            ok(info.Protect == view[j].prot ||
               broken(view[j].prot == PAGE_EXECUTE_READ && info.Protect == PAGE_READONLY) || /* win2k */
               broken(view[j].prot == PAGE_EXECUTE_READWRITE && info.Protect == PAGE_READWRITE) || /* win2k */
               broken(view[j].prot == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
               "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, info.Protect, view[j].prot);
            ok(info.AllocationBase == base, "%d: (%04x) got %p, expected %p\n", j, view[j].access, info.AllocationBase, base);
            ok(info.AllocationProtect == info.Protect, "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, info.AllocationProtect, info.Protect);
            ok(info.State == MEM_COMMIT, "%d: (%04x) got %#x, expected MEM_COMMIT\n", j, view[j].access, info.State);
            ok(info.Type == MEM_MAPPED, "%d: (%04x) got %#x, expected MEM_MAPPED\n", j, view[j].access, info.Type);

            if (nt_base && base)
            {
                ok(nt_info.RegionSize == info.RegionSize, "%d: (%04x) got %#lx != expected %#lx\n", j, view[j].access, nt_info.RegionSize, info.RegionSize);
                ok(nt_info.Protect == info.Protect /* Vista+ */ ||
                   broken(nt_info.AllocationProtect == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
                   "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, nt_info.Protect, info.Protect);
                ok(nt_info.AllocationProtect == info.AllocationProtect /* Vista+ */ ||
                   broken(nt_info.AllocationProtect == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
                   "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, nt_info.AllocationProtect, info.AllocationProtect);
                ok(nt_info.State == info.State, "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, nt_info.State, info.State);
                ok(nt_info.Type == info.Type, "%d: (%04x) got %#x, expected %#x\n", j, view[j].access, nt_info.Type, info.Type);
            }

            prev_prot = info.Protect;

            for (k = 0; k < sizeof(page_prot)/sizeof(page_prot[0]); k++)
            {
                /*trace("map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);*/
                SetLastError(0xdeadbeef);
                old_prot = 0xdeadbeef;
                ret = VirtualProtect(base, si.dwPageSize, page_prot[k], &old_prot);
                if (is_compatible_protection(page_prot[i], view[j].prot, page_prot[k]))
                {
                    /* win2k and XP don't support EXEC on file mappings */
                    if (!ret && page_prot[k] == PAGE_EXECUTE)
                    {
                        ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE\n");
                        continue;
                    }
                    /* NT4 and win2k don't support EXEC on file mappings */
                    if (!ret && (page_prot[k] == PAGE_EXECUTE_READ || page_prot[k] == PAGE_EXECUTE_READWRITE))
                    {
                        ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE\n");
                        continue;
                    }
                    /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
                    if (!ret && page_prot[k] == PAGE_EXECUTE_WRITECOPY)
                    {
                        ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE_WRITECOPY\n");
                        continue;
                    }
                    /* win2k and XP don't support PAGE_EXECUTE_WRITECOPY views properly  */
                    if (!ret && view[j].prot == PAGE_EXECUTE_WRITECOPY)
                    {
                        ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE_WRITECOPY view properly\n");
                        continue;
                    }

                    ok(ret, "VirtualProtect error %d, map %#x, view %#x, requested prot %#x\n", GetLastError(), page_prot[i], view[j].prot, page_prot[k]);
                    ok(old_prot == prev_prot, "got %#x, expected %#x\n", old_prot, prev_prot);
                    prev_prot = page_prot[k];
                }
                else
                {
                    /* NT4 doesn't fail on incompatible map and view */
                    if (ret)
                    {
                        ok(broken(ret), "VirtualProtect should fail, map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);
                        skip("Incompatible map and view are not properly handled on this platform\n");
                        break; /* NT4 won't pass remaining tests */
                    }

                    ok(!ret, "VirtualProtect should fail, map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
                }
            }

            for (k = 0; k < sizeof(page_prot)/sizeof(page_prot[0]); k++)
            {
                /*trace("map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);*/
                SetLastError(0xdeadbeef);
                ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, page_prot[k]);
                ok(!ptr, "VirtualAlloc(%02x) should fail\n", page_prot[k]);
                /* FIXME: remove once Wine is fixed */
                if (page_prot[k] == PAGE_WRITECOPY || page_prot[k] == PAGE_EXECUTE_WRITECOPY)
todo_wine
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
                else
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
            }

            UnmapViewOfFile(base);
        }

        CloseHandle(hmap);
    }

    CloseHandle(hfile);
    DeleteFile(file_name);
}

static void test_shared_memory(int is_child)
{
    HANDLE mapping;
    LONG *p;

    SetLastError(0xdeadbef);
    mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "winetest_virtual.c");
    ok(mapping != 0, "CreateFileMapping error %d\n", GetLastError());
    if (is_child)
        ok(GetLastError() == ERROR_ALREADY_EXISTS, "expected ERROR_ALREADY_EXISTS, got %d\n", GetLastError());

    SetLastError(0xdeadbef);
    p = MapViewOfFile(mapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 4096);
    ok(p != NULL, "MapViewOfFile error %d\n", GetLastError());

    if (is_child)
    {
        ok(*p == 0x1a2b3c4d, "expected 0x1a2b3c4d in child, got %#x\n", *p);
    }
    else
    {
        char **argv;
        char cmdline[MAX_PATH];
        PROCESS_INFORMATION pi;
        STARTUPINFO si = { sizeof(si) };
        DWORD ret;

        *p = 0x1a2b3c4d;

        winetest_get_mainargs(&argv);
        sprintf(cmdline, "\"%s\" virtual sharedmem", argv[0]);
        ret = CreateProcess(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %d\n", cmdline, GetLastError());
        winetest_wait_child_process(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    UnmapViewOfFile(p);
    CloseHandle(mapping);
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
        if (!strcmp(argv[2], "sharedmem"))
        {
            test_shared_memory(1);
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
    pGetWriteWatch = (void *) GetProcAddress(hkernel32, "GetWriteWatch");
    pResetWriteWatch = (void *) GetProcAddress(hkernel32, "ResetWriteWatch");
    pNtAreMappedFilesTheSame = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"),
                                                       "NtAreMappedFilesTheSame" );
    pNtMapViewOfSection = (void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtMapViewOfSection");
    pNtUnmapViewOfSection = (void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtUnmapViewOfSection");

    test_shared_memory(0);
    test_mapping();
    test_CreateFileMapping_protection();
    test_VirtualAlloc_protection();
    test_VirtualProtect();
    test_VirtualAllocEx();
    test_VirtualAlloc();
    test_MapViewOfFile();
    test_NtMapViewOfSection();
    test_NtAreMappedFilesTheSame();
    test_CreateFileMapping();
    test_IsBadReadPtr();
    test_IsBadWritePtr();
    test_IsBadCodePtr();
    test_write_watch();
}
