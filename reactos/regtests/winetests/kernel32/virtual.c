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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/test.h"

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
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = VirtualAlloc(0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAlloc failed\n");

    /* test a not committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %ld, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAlloc failed\n");

    /* test a committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %ld, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_NOACCESS,
        "wrong old protection: got %04lx instead of PAGE_NOACCESS\n", old_prot);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_READONLY,
        "wrong old protection: got %04lx instead of PAGE_READONLY\n", old_prot);

    ok(!VirtualFree(addr1, 0x10000, 0), "VirtualFree should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0x10000, MEM_DECOMMIT), "VirtualFree failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!VirtualFree(addr1, 1, MEM_RELEASE), "VirtualFree should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
}

static void test_MapViewOfFile(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, mapping;
    void *ptr;

    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create test file\n" );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAPE_READ failed\n" );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %lx\n", GetLastError() );

    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 failed\n" );
    UnmapViewOfFile( ptr );

    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE failed\n" );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    /* read-only mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ failed\n" );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %lx\n", GetLastError() );

    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 failed\n" );
    UnmapViewOfFile( ptr );

    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );
    CloseHandle( mapping );

    /* copy-on-write mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ failed\n" );
    UnmapViewOfFile( ptr );

    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY failed\n" );
    UnmapViewOfFile( ptr );

    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 failed\n" );
    UnmapViewOfFile( ptr );

    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );
    CloseHandle( mapping );

    /* no access mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_NOACCESS, 0, 4096, NULL );
    /* fails on NT but succeeds on win9x */
    if (!mapping) ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %lx\n", GetLastError() );
    else
    {
        ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ failed\n" );
        UnmapViewOfFile( ptr );

        ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_COPY succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %lx\n", GetLastError() );

        ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile 0 failed\n" );
        UnmapViewOfFile( ptr );

        ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %lx\n", GetLastError() );

        CloseHandle( mapping );
    }

    CloseHandle( file );

    /* now try read-only file */

    file = CreateFileA( testfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create test file\n" );

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_WRITECOPY failed\n" );
    CloseHandle( mapping );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_READONLY failed\n" );
    CloseHandle( mapping );
    CloseHandle( file );

    /* now try no access file */

    file = CreateFileA( testfile, 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create test file\n" );

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_WRITECOPY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READONLY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %lx\n", GetLastError() );

    CloseHandle( file );

    CloseHandle( file );
    DeleteFileA( testfile );
}

START_TEST(virtual)
{
    test_VirtualAlloc();
    test_MapViewOfFile();
}
