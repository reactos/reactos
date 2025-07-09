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
#include "winnt.h"
#include "winternl.h"
#include "winerror.h"
#include "winuser.h"
#include "excpt.h"
#include "wine/test.h"

#define NUM_THREADS 4
#define MAPPING_SIZE 0x100000

static HINSTANCE hkernel32, hkernelbase, hntdll;
static SYSTEM_INFO si;
static BOOL is_wow64;
static UINT   (WINAPI *pGetWriteWatch)(DWORD,LPVOID,SIZE_T,LPVOID*,ULONG_PTR*,ULONG*);
static UINT   (WINAPI *pResetWriteWatch)(LPVOID,SIZE_T);
static NTSTATUS (WINAPI *pNtAreMappedFilesTheSame)(PVOID,PVOID);
static NTSTATUS (WINAPI *pNtCreateSection)(HANDLE *, ACCESS_MASK, const OBJECT_ATTRIBUTES *,
                                           const LARGE_INTEGER *, ULONG, ULONG, HANDLE );
static NTSTATUS (WINAPI *pNtMapViewOfSection)(HANDLE, HANDLE, PVOID *, ULONG_PTR, SIZE_T, const LARGE_INTEGER *, SIZE_T *, ULONG, ULONG, ULONG);
static DWORD (WINAPI *pNtUnmapViewOfSection)(HANDLE, PVOID);
static NTSTATUS (WINAPI *pNtQuerySection)(HANDLE, SECTION_INFORMATION_CLASS, void *, SIZE_T, SIZE_T *);
static PVOID  (WINAPI *pRtlAddVectoredExceptionHandler)(ULONG, PVECTORED_EXCEPTION_HANDLER);
static ULONG  (WINAPI *pRtlRemoveVectoredExceptionHandler)(PVOID);
static BOOL   (WINAPI *pGetProcessDEPPolicy)(HANDLE, LPDWORD, PBOOL);
static BOOL   (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static NTSTATUS (WINAPI *pNtProtectVirtualMemory)(HANDLE, PVOID *, SIZE_T *, ULONG, ULONG *);
static NTSTATUS (WINAPI *pNtReadVirtualMemory)(HANDLE,const void *,void *,SIZE_T, SIZE_T *);
static NTSTATUS (WINAPI *pNtWriteVirtualMemory)(HANDLE, void *, const void *, SIZE_T, SIZE_T *);
#ifndef __REACTOS__
static BOOL  (WINAPI *pPrefetchVirtualMemory)(HANDLE, ULONG_PTR, PWIN32_MEMORY_RANGE_ENTRY, ULONG);
#endif

/* ############################### */

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "error: %lu\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    return pi.hProcess;
}

static void test_VirtualAllocEx(void)
{
    const unsigned int alloc_size = 1<<15;
    char *src, *dst;
    SIZE_T bytes_written = 0, bytes_read = 0, i;
    void *addr1, *addr2;
    BOOL b, ret;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;
    HANDLE hProcess;
    NTSTATUS status;

    /* Same process */
    addr1 = VirtualAllocEx(GetCurrentProcess(), NULL, alloc_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(!!addr1, "Failed to allocated, error %lu.\n", GetLastError());
    ret = VirtualFreeEx(NULL, addr1, 0, MEM_RELEASE);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Unexpected value %d, error %lu.\n", ret, GetLastError());
    addr2 = VirtualAllocEx(NULL, NULL, alloc_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(!addr2 && GetLastError() == ERROR_INVALID_HANDLE, "Unexpected value %p, error %lu.\n", addr2, GetLastError());
    ret = VirtualFreeEx(GetCurrentProcess(), addr1, 0, MEM_RELEASE);
    ok(ret, "Unexpected value %d, error %lu.\n", ret, GetLastError());

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    SetLastError(0xdeadbeef);
    addr1 = VirtualAllocEx(hProcess, NULL, alloc_size, MEM_COMMIT,
                           PAGE_EXECUTE_READWRITE);
    ok(addr1 != NULL, "VirtualAllocEx error %lu\n", GetLastError());

    src = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    dst = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    for (i = 0; i < alloc_size; i++)
        src[i] = i & 0xff;

    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok(b && (bytes_written == alloc_size), "%Iu bytes written\n",
       bytes_written);
    b = ReadProcessMemory(hProcess, addr1, dst, alloc_size, &bytes_read);
    ok(b && (bytes_read == alloc_size), "%Iu bytes read\n", bytes_read);
    ok(!memcmp(src, dst, alloc_size), "Data from remote process differs\n");
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( bytes_written == alloc_size, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    memset( dst, 0, alloc_size );
    status = pNtReadVirtualMemory( hProcess, addr1, dst, alloc_size, &bytes_read );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( bytes_read == alloc_size, "%Iu bytes read\n", bytes_read );
    ok(!memcmp(src, dst, alloc_size), "Data from remote process differs\n");

    /* test 0 length */
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, 0, &bytes_written);
    ok((b && !bytes_written) || broken(!b && GetLastError() == ERROR_INVALID_PARAMETER), "write failed: %lu\n", GetLastError());
    bytes_read = 0xdeadbeef;
    b = ReadProcessMemory(hProcess, addr1, src, 0, &bytes_read);
    ok(b && !bytes_read, "read failed: %lu\n", GetLastError());
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, 0, &bytes_written );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    status = pNtReadVirtualMemory( hProcess, addr1, src, 0, &bytes_read );
    ok( status == STATUS_SUCCESS, "wrong status %lx\n", status );
    ok( !bytes_read, "%Iu bytes read\n", bytes_read );

    /* test invalid source buffers */

    b = VirtualProtect( src + 0x2000, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %lu\n", GetLastError() );
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* win10 v1607+ */
        "wrong error %lu\n", GetLastError() );
    if (GetLastError() == ERROR_NOACCESS)
        ok( bytes_read == 0, "%Iu bytes read\n", bytes_read );
    else
        ok( bytes_read == 0x2000, "%Iu bytes read\n", bytes_read );
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    ok( status == STATUS_PARTIAL_COPY, "wrong status %lx\n", status );
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    status = pNtReadVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_read );
    ok( status == STATUS_PARTIAL_COPY || status == STATUS_ACCESS_VIOLATION, "wrong status %lx\n", status );
    ok( bytes_read == (status == STATUS_PARTIAL_COPY ? 0x2000 : 0), "%Iu bytes read\n", bytes_read );

    b = VirtualProtect( src, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %lu\n", GetLastError() );
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* win10 v1607+ */
        "wrong error %lu\n", GetLastError() );
    ok( bytes_read == 0, "%Iu bytes read\n", bytes_read );
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    ok( status == STATUS_PARTIAL_COPY, "wrong status %lx\n", status );
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );
    bytes_read = 0xdeadbeef;
    status = pNtReadVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_read );
    ok( status == STATUS_PARTIAL_COPY || status == STATUS_ACCESS_VIOLATION, "wrong status %lx\n", status );
    ok( bytes_read == 0, "%Iu bytes read\n", bytes_read );
    b = VirtualProtect( src, alloc_size, PAGE_READWRITE, &old_prot );
    ok( b, "VirtualProtect failed error %lu\n", GetLastError() );

    /* test readonly buffers */

    b = VirtualProtectEx( hProcess, addr1, alloc_size, PAGE_READONLY, &old_prot );
    ok( b, "VirtualProtectEx, error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    if (!b) ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );
    ok( bytes_written == 0xdeadbeef, "%Iu bytes written\n", bytes_written );
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    todo_wine
    ok( status == STATUS_PARTIAL_COPY || broken(status == STATUS_ACCESS_VIOLATION),
        "wrong status %lx\n", status );
    todo_wine
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );

    b = VirtualProtectEx( hProcess, addr1, alloc_size, PAGE_EXECUTE_READ, &old_prot );
    ok( b, "VirtualProtectEx, error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( b, "WriteProcessMemory failed\n" );
    ok( bytes_written == alloc_size, "%Iu bytes written\n", bytes_written );
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    todo_wine
    ok( status == STATUS_PARTIAL_COPY || broken(status == STATUS_ACCESS_VIOLATION),
        "wrong status %lx\n", status );
    todo_wine
    ok( bytes_written == 0, "%Iu bytes written\n", bytes_written );

    b = VirtualProtectEx( hProcess, addr1, 0x2000, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( b, "VirtualProtectEx, error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    todo_wine
    ok( !b || broken(b), /* <= win10 1507 */ "WriteProcessMemory succeeded\n" );
    bytes_written = 0xdeadbeef;
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    todo_wine
    ok( status == STATUS_PARTIAL_COPY || broken(status == STATUS_SUCCESS), /* <= win10 1507 */
        "wrong status %lx\n", status );
    ok( bytes_written == (status ? 0x2000 : alloc_size), "%Iu bytes written\n", bytes_written );

    b = VirtualProtectEx( hProcess, (char *)addr1 + 0x2000, alloc_size - 0x2000, PAGE_READONLY, &old_prot );
    ok( b, "VirtualProtectEx, error %lu\n", GetLastError() );
    bytes_written = 0xdeadbeef;
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    todo_wine
    ok( !b || broken(b), /* <= win10 1507 */ "WriteProcessMemory succeeded\n" );
    status = pNtWriteVirtualMemory( hProcess, addr1, src, alloc_size, &bytes_written );
    todo_wine
    ok( status == STATUS_PARTIAL_COPY || broken(status == STATUS_SUCCESS), /* <= win10 1507 */
        "wrong status %lx\n", status );
    ok( bytes_written == (status ? 0x2000 : alloc_size), "%Iu bytes written\n", bytes_written );

    VirtualFree( src, 0, MEM_RELEASE );
    VirtualFree( dst, 0, MEM_RELEASE );

    /*
     * The following tests parallel those in test_VirtualAlloc()
     */

    SetLastError(0xdeadbeef);
    addr1 = VirtualAllocEx(hProcess, 0, 0, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 == NULL, "VirtualAllocEx should fail on zero-sized allocation\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "got %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = VirtualAllocEx(hProcess, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAllocEx failed\n");

    /* test a not committed memory */
    memset(&info, 'q', sizeof(info));
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info), "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%Ix != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
    ok(info.Protect == 0, "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS,
        "got %lu, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = VirtualAllocEx(hProcess, addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAllocEx failed\n");

    /* test a committed memory */
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%Ix != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS,
       "got %lu, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_NOACCESS, "wrong old protection: got %04lx instead of PAGE_NOACCESS\n", old_prot);

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_READONLY, "wrong old protection: got %04lx instead of PAGE_READONLY\n", old_prot);

    ok(!VirtualFreeEx(hProcess, addr1, 0x10000, 0),
       "VirtualFreeEx should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFreeEx(hProcess, addr1, 0x10000, MEM_DECOMMIT), "VirtualFreeEx failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!VirtualFreeEx(hProcess, addr1, 1, MEM_RELEASE),
       "VirtualFreeEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE), "VirtualFreeEx failed\n");

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
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = VirtualAlloc(0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAlloc failed\n");

    /* test a not committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%Ix != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
    ok(info.Protect == 0, "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtect should fail on a not committed memory\n");
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %ld, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAlloc failed\n");

    /* test a committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%Ix != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtect should fail on a not committed memory\n");
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %ld, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_NOACCESS,
        "wrong old protection: got %04lx instead of PAGE_NOACCESS\n", old_prot);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_READONLY,
        "wrong old protection: got %04lx instead of PAGE_READONLY\n", old_prot);

    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.RegionSize == 0x1000, "%Ix != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.Protect);
    memset( addr1, 0x55, 20 );
    ok( *(DWORD *)addr1 == 0x55555555, "wrong data %lx\n", *(DWORD *)addr1 );

    addr2 = VirtualAlloc( addr1, 0x1000, MEM_RESET, PAGE_NOACCESS );
    ok( addr2 == addr1, "VirtualAlloc failed err %lu\n", GetLastError() );
    ok( *(DWORD *)addr1 == 0x55555555 || *(DWORD *)addr1 == 0, "wrong data %lx\n", *(DWORD *)addr1 );
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
       "VirtualQuery failed\n");
    ok(info.RegionSize == 0x1000, "%Ix != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.Protect);

    addr2 = VirtualAlloc( (char *)addr1 + 0x1000, 0x1000, MEM_RESET, PAGE_NOACCESS );
    ok( (char *)addr2 == (char *)addr1 + 0x1000, "VirtualAlloc failed\n" );

    ok(VirtualQuery(addr2, &info, sizeof(info)) == sizeof(info),
       "VirtualQuery failed\n");
    ok(info.RegionSize == 0xf000, "%Ix != 0xf000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
    ok(info.Protect == 0, "%lx != 0\n", info.Protect);

    addr2 = VirtualAlloc( (char *)addr1 + 0xf000, 0x2000, MEM_RESET, PAGE_NOACCESS );
    ok( !addr2, "VirtualAlloc failed\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS, "wrong error %lu\n", GetLastError() );

    /* invalid protection values */
    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_RESERVE, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_READONLY | PAGE_EXECUTE);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, PAGE_READWRITE | PAGE_EXECUTE_WRITECOPY, &old_prot),
       "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, 0, &old_prot), "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualFree(addr1, 0x10000, 0), "VirtualFree should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualFree(addr1, 0, MEM_FREE), "VirtualFree should fail with type MEM_FREE\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0x10000, MEM_DECOMMIT), "VirtualFree failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!VirtualFree(addr1, 1, MEM_RELEASE), "VirtualFree should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");

    /* PAGE_NOCACHE cannot be set per page in recent Windows */
    addr1 = VirtualAlloc( NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE | PAGE_NOCACHE );
    ok( addr1 != NULL, "VirtualAlloc failed\n");
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info), "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == (PAGE_READWRITE | PAGE_NOCACHE),
       "wrong protect %lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x2000, "wrong size %Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong state %lx\n", info.State);
    ok(info.Protect == (PAGE_READWRITE | PAGE_NOCACHE), "wrong protect %lx\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "wrong type %lx\n", info.Type);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtect failed\n");
    ok( old_prot == (PAGE_READWRITE | PAGE_NOCACHE), "wrong protect %lx\n", old_prot );
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info), "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == (PAGE_READWRITE | PAGE_NOCACHE),
       "wrong protect %lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x2000 || broken(info.RegionSize == 0x1000),
       "wrong size %Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong state %lx\n", info.State);
    ok(info.Protect == (PAGE_READWRITE | PAGE_NOCACHE) || broken(info.Protect == PAGE_READWRITE),
       "wrong protect %lx\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "wrong type %lx\n", info.Type);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtect failed\n");
    ok( old_prot == (PAGE_READWRITE | PAGE_NOCACHE) || broken(old_prot == PAGE_READWRITE),
        "wrong protect %lx\n", old_prot );
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info), "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == (PAGE_READWRITE | PAGE_NOCACHE),
       "wrong protect %lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "wrong size %Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong state %lx\n", info.State);
    ok(info.Protect == (PAGE_READONLY | PAGE_NOCACHE) || broken(info.Protect == PAGE_READONLY),
       "wrong protect %lx\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "wrong type %lx\n", info.Type);

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");

    addr1 = VirtualAlloc( NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE );
    ok( addr1 != NULL, "VirtualAlloc failed\n");
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info), "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_READWRITE,
       "wrong protect %lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x2000, "wrong size %Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong state %lx\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "wrong protect %lx\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "wrong type %lx\n", info.Type);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY | PAGE_NOCACHE, &old_prot), "VirtualProtect failed\n");
    ok( old_prot == PAGE_READWRITE, "wrong protect %lx\n", old_prot );
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info), "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_READWRITE, "wrong protect %lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "wrong size %Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong state %lx\n", info.State);
    ok(info.Protect == PAGE_READONLY || broken(info.Protect == (PAGE_READONLY | PAGE_NOCACHE)),
       "wrong protect %lx\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "wrong type %lx\n", info.Type);

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");

    /* memory returned by VirtualAlloc should be aligned to 64k */
    addr1 = VirtualAlloc(0, 0x2000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(addr1 != NULL, "VirtualAlloc failed\n");
    ok(!((ULONG_PTR)addr1 & 0xffff), "returned memory %p is not aligned to 64k\n", addr1);
    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
    addr2 = VirtualAlloc(addr1, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(addr2 == addr1, "VirtualAlloc returned %p, expected %p\n", addr2, addr1);

    /* AT_ROUND_TO_PAGE flag is not supported for VirtualAlloc */
    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(addr1, 0x1000, MEM_RESERVE | MEM_COMMIT | AT_ROUND_TO_PAGE, PAGE_EXECUTE_READWRITE);
    ok(!addr2, "VirtualAlloc unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %ld, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
}

static void test_MapViewOfFile(void)
{
    static const char testfile[] = "testfile.xxx";
    const char *name;
    HANDLE file, mapping, map2;
    void *ptr, *ptr2, *addr;
    SECTION_BASIC_INFORMATION section_info;
    SECTION_IMAGE_INFORMATION image_info;
    MEMORY_BASIC_INFORMATION info;
    BOOL ret;
    SIZE_T size;
    NTSTATUS status;
    SIZE_T info_size;
    LARGE_INTEGER map_size;

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer( file, 12288, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ|FILE_MAP_WRITE, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %lu\n", GetLastError());
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );
    CloseHandle( map2 );

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    CloseHandle( map2 );
    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2, 0, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    CloseHandle( map2 );
    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %lu\n", GetLastError());
    ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile NO_ACCESS error %lu\n", GetLastError() );

    UnmapViewOfFile( ptr );
    CloseHandle( map2 );
    CloseHandle( mapping );

    /* read-only mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    CloseHandle( mapping );

    /* copy-on-write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %lu\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    CloseHandle( mapping );

    /* no access mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_NOACCESS, 0, 4096, NULL );
    ok( !mapping, "CreateFileMappingA succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %ld\n", GetLastError() );
    CloseHandle( file );

    /* now try read-only file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_WRITECOPY error %lu\n", GetLastError() );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_READONLY error %lu\n", GetLastError() );
    CloseHandle( mapping );
    CloseHandle( file );

    /* now try no access file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_WRITECOPY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READONLY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );

    CloseHandle( file );
    DeleteFileA( testfile );

    SetLastError(0xdeadbeef);
    name = "Local\\Foo";
    file = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4090, name );
    /* nt4 doesn't have Local\\ */
    if (!file && GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        name = "Foo";
        file = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4090, name );
    }
    ok( file != 0, "CreateFileMapping PAGE_READWRITE error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMappingA( FILE_MAP_READ, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_READ error %lu\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    SetLastError(0xdeadbeef);
    size = VirtualQuery( ptr, &info, sizeof(info) );
    ok( size == sizeof(info),
        "VirtualQuery error %lu\n", GetLastError() );
    ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
    ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
    ok( info.AllocationProtect == PAGE_READONLY, "%lx != PAGE_READONLY\n", info.AllocationProtect );
    ok( info.RegionSize == 4096, "%Ix != 4096\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State );
    ok( info.Protect == PAGE_READONLY, "%lx != PAGE_READONLY\n", info.Protect );
    UnmapViewOfFile( ptr );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( status == STATUS_ACCESS_DENIED, "NtQuerySection failed err %lx\n", status );
    CloseHandle( mapping );
    mapping = OpenFileMappingA( FILE_MAP_READ | SECTION_QUERY, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_READ error %lu\n", GetLastError() );
    info_size = (SIZE_T)0xdeadbeef << 16;
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info_size == sizeof(section_info), "NtQuerySection wrong size %Iu\n", info_size );
    ok( section_info.Attributes == SEC_COMMIT, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == info.RegionSize, "NtQuerySection wrong size %lx%08lx / %08Ix\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart, info.RegionSize );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMappingA( FILE_MAP_WRITE, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_WRITE error %lu\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( !ptr, "MapViewOfFile succeeded\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %lu\n", GetLastError() );
    SetLastError(0xdeadbeef);
    size = VirtualQuery( ptr, &info, sizeof(info) );
    ok( size == sizeof(info),
        "VirtualQuery error %lu\n", GetLastError() );
    ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
    ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
    ok( info.AllocationProtect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.AllocationProtect );
    ok( info.RegionSize == 4096, "%Ix != 4096\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "%lx != MEM_COMMIT\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "%lx != PAGE_READWRITE\n", info.Protect );
    UnmapViewOfFile( ptr );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( status == STATUS_ACCESS_DENIED, "NtQuerySection failed err %lx\n", status );
    CloseHandle( mapping );

    mapping = OpenFileMappingA( FILE_MAP_WRITE | SECTION_QUERY, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_WRITE error %lu\n", GetLastError() );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info_size == sizeof(section_info), "NtQuerySection wrong size %Iu\n", info_size );
    ok( section_info.Attributes == SEC_COMMIT, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == info.RegionSize, "NtQuerySection wrong size %lx%08lx / %08Ix\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart, info.RegionSize );
    CloseHandle( mapping );

    CloseHandle( file );

    /* read/write mapping with SEC_RESERVE */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0, MAPPING_SIZE, NULL);
    ok(mapping != 0, "CreateFileMappingA failed with error %ld\n", GetLastError());
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( section_info.Attributes == SEC_RESERVE, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == MAPPING_SIZE, "NtQuerySection wrong size %lx%08lx / %08x\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart, MAPPING_SIZE );

    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %ld\n", GetLastError());

    ptr2 = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok( ptr2 != NULL, "MapViewOfFile failed with error %ld\n", GetLastError());
    ok( ptr != ptr2, "MapViewOfFile returned same pointer\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == MAPPING_SIZE, "RegionSize should have been 0x%x but was 0x%Ix\n", MAPPING_SIZE, info.RegionSize);
    ok(info.State == MEM_RESERVE, "State should have been MEM_RESERVE instead of 0x%lx\n", info.State);
    ok(info.AllocationProtect == PAGE_READWRITE,
       "AllocationProtect should have been PAGE_READWRITE but was 0x%lx\n", info.AllocationProtect);
    ok(info.Protect == 0, "Protect should have been 0 instead of 0x%lx\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%lx\n", info.Type);

    ret = VirtualQuery(ptr2, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr2,
       "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
    ok(info.AllocationBase == ptr2,
       "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
    ok(info.AllocationProtect == PAGE_READWRITE,
       "AllocationProtect should have been PAGE_READWRITE but was 0x%lx\n", info.AllocationProtect);
    ok(info.RegionSize == MAPPING_SIZE,
       "RegionSize should have been 0x%x but was 0x%Ix\n", MAPPING_SIZE, info.RegionSize);
    ok(info.State == MEM_RESERVE, "State should have been MEM_RESERVE instead of 0x%lx\n", info.State);
    ok(info.Protect == 0, "Protect should have been 0 instead of 0x%lx\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%lx\n", info.Type);

    ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READONLY);
    ok(ptr != NULL, "VirtualAlloc failed with error %ld\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == 0x10000, "RegionSize should have been 0x10000 but was 0x%Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "State should have been MEM_COMMIT instead of 0x%lx\n", info.State);
    ok(info.Protect == PAGE_READONLY, "Protect should have been PAGE_READONLY instead of 0x%lx\n", info.Protect);
    ok(info.AllocationProtect == PAGE_READWRITE,
       "AllocationProtect should have been PAGE_READWRITE but was 0x%lx\n", info.AllocationProtect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%lx\n", info.Type);

    /* shows that the VirtualAlloc above affects the mapping, not just the
     * virtual memory in this process - it also affects all other processes
     * with a view of the mapping, but that isn't tested here */
    ret = VirtualQuery(ptr2, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr2,
       "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
    ok(info.AllocationBase == ptr2,
       "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
    ok(info.AllocationProtect == PAGE_READWRITE,
       "AllocationProtect should have been PAGE_READWRITE but was 0x%lx\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000,
       "RegionSize should have been 0x10000 but was 0x%Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT,
       "State should have been MEM_COMMIT instead of 0x%lx\n", info.State);
    ok(info.Protect == PAGE_READWRITE,
       "Protect should have been PAGE_READWRITE instead of 0x%lx\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%lx\n", info.Type);

    addr = VirtualAlloc( ptr, MAPPING_SIZE, MEM_RESET, PAGE_READONLY );
    ok( addr == ptr, "VirtualAlloc failed with error %lu\n", GetLastError() );

    ret = VirtualFree( ptr, 0x10000, MEM_DECOMMIT );
    ok( !ret, "VirtualFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "VirtualFree failed with %lu\n", GetLastError() );

    ret = UnmapViewOfFile(ptr2);
    ok(ret, "UnmapViewOfFile failed with error %ld\n", GetLastError());
    ret = UnmapViewOfFile(ptr);
    ok(ret, "UnmapViewOfFile failed with error %ld\n", GetLastError());
    CloseHandle(mapping);

    /* same thing with SEC_COMMIT */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, MAPPING_SIZE, NULL);
    ok(mapping != 0, "CreateFileMappingA failed with error %ld\n", GetLastError());
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( section_info.Attributes == SEC_COMMIT, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == MAPPING_SIZE, "NtQuerySection wrong size %lx%08lx / %08x\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart, MAPPING_SIZE );

    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %ld\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "wrong BaseAddress %p/%p\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "wrong AllocationBase %p/%p\n", ptr, info.AllocationBase);
    ok(info.RegionSize == MAPPING_SIZE, "wrong RegionSize 0x%Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State);
    ok(info.AllocationProtect == PAGE_READWRITE, "wrong AllocationProtect 0x%lx\n", info.AllocationProtect);
    ok(info.Protect == PAGE_READWRITE, "wrong Protect 0x%lx\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "wrong Type 0x%lx\n", info.Type);

    ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READONLY);
    ok(ptr != NULL, "VirtualAlloc failed with error %ld\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "wrong BaseAddress %p/%p\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "wrong AllocationBase %p/%p\n", ptr, info.AllocationBase);
    ok(info.RegionSize == 0x10000, "wrong RegionSize 0x%Ix\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State);
    ok(info.AllocationProtect == PAGE_READWRITE, "wrong AllocationProtect 0x%lx\n", info.AllocationProtect);
    ok(info.Protect == PAGE_READONLY, "wrong Protect 0x%lx\n", info.Protect);
    ok(info.Type == MEM_MAPPED, "wrong Type 0x%lx\n", info.Type);

    addr = VirtualAlloc( ptr, MAPPING_SIZE, MEM_RESET, PAGE_READONLY );
    ok( addr == ptr, "VirtualAlloc failed with error %lu\n", GetLastError() );

    ret = VirtualFree( ptr, 0x10000, MEM_DECOMMIT );
    ok( !ret, "VirtualFree succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "VirtualFree failed with %lu\n", GetLastError() );

    ret = UnmapViewOfFile(ptr);
    ok(ret, "UnmapViewOfFile failed with error %ld\n", GetLastError());
    CloseHandle(mapping);

    /* same thing with SEC_NOCACHE (only supported on recent Windows versions) */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT | SEC_NOCACHE,
                                 0, MAPPING_SIZE, NULL);
    ok(mapping != 0, "CreateFileMappingA failed with error %ld\n", GetLastError());
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( section_info.Attributes == (SEC_COMMIT | SEC_NOCACHE) ||
        broken(section_info.Attributes == SEC_COMMIT),
        "NtQuerySection wrong attr %08lx\n", section_info.Attributes );
    if (section_info.Attributes & SEC_NOCACHE)
    {
        ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
        ok(ptr != NULL, "MapViewOfFile failed with error %ld\n", GetLastError());

        ret = VirtualQuery(ptr, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
        ok(info.BaseAddress == ptr, "wrong BaseAddress %p/%p\n", ptr, info.BaseAddress);
        ok(info.AllocationBase == ptr, "wrong AllocationBase %p/%p\n", ptr, info.AllocationBase);
        ok(info.RegionSize == MAPPING_SIZE, "wrong RegionSize 0x%Ix\n", info.RegionSize);
        ok(info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State);
        ok(info.AllocationProtect == (PAGE_READWRITE | PAGE_NOCACHE),
           "wrong AllocationProtect 0x%lx\n", info.AllocationProtect);
        ok(info.Protect == (PAGE_READWRITE | PAGE_NOCACHE), "wrong Protect 0x%lx\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "wrong Type 0x%lx\n", info.Type);

        ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READONLY);
        ok(ptr != NULL, "VirtualAlloc failed with error %ld\n", GetLastError());

        ret = VirtualQuery(ptr, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %ld\n", GetLastError());
        ok(info.BaseAddress == ptr, "wrong BaseAddress %p/%p\n", ptr, info.BaseAddress);
        ok(info.AllocationBase == ptr, "wrong AllocationBase %p/%p\n", ptr, info.AllocationBase);
        ok(info.RegionSize == 0x10000, "wrong RegionSize 0x%Ix\n", info.RegionSize);
        ok(info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State);
        ok(info.AllocationProtect == (PAGE_READWRITE | PAGE_NOCACHE),
           "wrong AllocationProtect 0x%lx\n", info.AllocationProtect);
        ok(info.Protect == (PAGE_READONLY | PAGE_NOCACHE), "wrong Protect 0x%lx\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "wrong Type 0x%lx\n", info.Type);

        ret = UnmapViewOfFile(ptr);
        ok(ret, "UnmapViewOfFile failed with error %ld\n", GetLastError());
    }
    CloseHandle(mapping);

    addr = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_READONLY );
    ok( addr != NULL, "VirtualAlloc failed with error %lu\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile(addr), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %lu, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((char *)addr + 0x3000), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %lu, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((void *)0xdeadbeef), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
       "got %lu, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok( VirtualFree(addr, 0, MEM_RELEASE), "VirtualFree failed\n" );

    /* close named mapping handle without unmapping */
    name = "Foo";
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok( ptr != NULL, "MapViewOfFile failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 != 0, "OpenFileMappingA failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(map2);
    ok(ret, "CloseHandle error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %ld\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#Ix != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#lx != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#lx != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 == 0, "OpenFileMappingA succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "OpenFileMappingA set error %ld\n", GetLastError() );
    if (map2) CloseHandle(map2); /* FIXME: remove once Wine is fixed */
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed\n" );
    ok( GetLastError() == ERROR_SUCCESS, "CreateFileMappingA set error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %ld\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#Ix != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#lx != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#lx != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    ret = UnmapViewOfFile(ptr);
    ok( ret, "UnmapViewOfFile failed with error %ld\n", GetLastError() );

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( ret, "memory is accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.Protect == PAGE_NOACCESS, "got %#lx != expected PAGE_NOACCESS\n", info.Protect);
    ok(info.AllocationBase == NULL, "%p != NULL\n", info.AllocationBase);
    ok(info.AllocationProtect == 0, "%#lx != 0\n", info.AllocationProtect);
    ok(info.State == MEM_FREE, "%#lx != MEM_FREE\n", info.State);
    ok(info.Type == 0, "%#lx != 0\n", info.Type);

    SetLastError(0xdeadbeef);
    file = CreateFileA(testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer(file, 4096, NULL, FILE_BEGIN);
    SetEndOfFile(file);

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok( ptr != NULL, "MapViewOfFile failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 != 0, "OpenFileMappingA failed with error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(map2);
    ok(ret, "CloseHandle error %ld\n", GetLastError());
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info_size == sizeof(section_info), "NtQuerySection wrong size %Iu\n", info_size );
    ok( section_info.Attributes == SEC_FILE, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == MAPPING_SIZE, "NtQuerySection wrong size %lx%08lx\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %ld\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#Ix != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#lx != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#lx != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    map2 = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
    ok( map2 == 0, "OpenFileMappingA succeeded\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "OpenFileMappingA set error %ld\n", GetLastError() );
    CloseHandle(map2);
    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, MAPPING_SIZE, name);
    ok( mapping != 0, "CreateFileMappingA failed\n" );
    ok( GetLastError() == ERROR_SUCCESS, "CreateFileMappingA set error %ld\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ret = CloseHandle(mapping);
    ok(ret, "CloseHandle error %ld\n", GetLastError());

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( !ret, "memory is not accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.RegionSize == MAPPING_SIZE, "got %#Ix != expected %#x\n", info.RegionSize, MAPPING_SIZE);
    ok(info.Protect == PAGE_READWRITE, "got %#lx != expected PAGE_READWRITE\n", info.Protect);
    ok(info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr);
    ok(info.AllocationProtect == PAGE_READWRITE, "%#lx != PAGE_READWRITE\n", info.AllocationProtect);
    ok(info.State == MEM_COMMIT, "%#lx != MEM_COMMIT\n", info.State);
    ok(info.Type == MEM_MAPPED, "%#lx != MEM_MAPPED\n", info.Type);

    SetLastError(0xdeadbeef);
    ret = UnmapViewOfFile(ptr);
    ok( ret, "UnmapViewOfFile failed with error %ld\n", GetLastError() );

    ret = IsBadReadPtr(ptr, MAPPING_SIZE);
    ok( ret, "memory is accessible\n" );

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery error %ld\n", GetLastError());
    ok(info.BaseAddress == ptr, "got %p != expected %p\n", info.BaseAddress, ptr);
    ok(info.Protect == PAGE_NOACCESS, "got %#lx != expected PAGE_NOACCESS\n", info.Protect);
    ok(info.AllocationBase == NULL, "%p != NULL\n", info.AllocationBase);
    ok(info.AllocationProtect == 0, "%#lx != 0\n", info.AllocationProtect);
    ok(info.State == MEM_FREE, "%#lx != MEM_FREE\n", info.State);
    ok(info.Type == 0, "%#lx != 0\n", info.Type);

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 12288, NULL );
    ok( mapping != NULL, "CreateFileMappingA failed with error %lu\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 12288 );
    ok( ptr != NULL, "MapViewOfFile failed with error %lu\n", GetLastError() );

    ret = UnmapViewOfFile( (char *)ptr + 100 );
    ok( ret, "UnmapViewOfFile failed with error %lu\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 12288 );
    ok( ptr != NULL, "MapViewOfFile failed with error %lu\n", GetLastError() );

    ret = UnmapViewOfFile( (char *)ptr + 4096 );
    ok( ret, "UnmapViewOfFile failed with error %lu\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 12288 );
    ok( ptr != NULL, "MapViewOfFile failed with error %lu\n", GetLastError() );

    ret = UnmapViewOfFile( (char *)ptr + 4096 + 100 );
    ok( ret, "UnmapViewOfFile failed with error %lu\n", GetLastError() );

    CloseHandle(mapping);

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 36, NULL );
    ok( mapping != NULL, "CreateFileMappingA failed with error %lu\n", GetLastError() );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info_size == sizeof(section_info), "NtQuerySection wrong size %Iu\n", info_size );
    ok( section_info.Attributes == SEC_FILE, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == 36, "NtQuerySection wrong size %lx%08lx\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart );
    CloseHandle(mapping);

    SetFilePointer(file, 0x3456, NULL, FILE_BEGIN);
    SetEndOfFile(file);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 0, NULL );
    ok( mapping != NULL, "CreateFileMappingA failed with error %lu\n", GetLastError() );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info,
                              sizeof(section_info), &info_size );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( info_size == sizeof(section_info), "NtQuerySection wrong size %Iu\n", info_size );
    ok( section_info.Attributes == SEC_FILE, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == 0x3456, "NtQuerySection wrong size %lx%08lx\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart );
    CloseHandle(mapping);

    map_size.QuadPart = 0x3457;
    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               &map_size, PAGE_READONLY, SEC_COMMIT, file );
    ok( status == STATUS_SECTION_TOO_BIG, "NtCreateSection failed %lx\n", status );
    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               &map_size, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_INVALID_IMAGE_NOT_MZ, "NtCreateSection failed %lx\n", status );
    if (!status) CloseHandle( mapping );
    map_size.QuadPart = 0x3452;
    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               &map_size, PAGE_READONLY, SEC_COMMIT, file );
    ok( !status, "NtCreateSection failed %lx\n", status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info, sizeof(section_info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( section_info.Attributes == SEC_FILE, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == 0x3452, "NtQuerySection wrong size %lx%08lx\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart );
    size = map_size.QuadPart;
    status = pNtMapViewOfSection( mapping, GetCurrentProcess(), &ptr, 0, 0, NULL,
                                  &size, ViewShare, 0, PAGE_READONLY );
    ok( !status, "NtMapViewOfSection failed err %lx\n", status );
    pNtUnmapViewOfSection( GetCurrentProcess(), ptr );
    size = map_size.QuadPart + 1;
    status = pNtMapViewOfSection( mapping, GetCurrentProcess(), &ptr, 0, 0, NULL,
                                  &size, ViewShare, 0, PAGE_READONLY );
    ok( status == STATUS_INVALID_VIEW_SIZE, "NtMapViewOfSection failed err %lx\n", status );
    CloseHandle(mapping);

    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               &map_size, PAGE_READONLY, SEC_COMMIT, 0 );
    ok( !status, "NtCreateSection failed %lx\n", status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info, sizeof(section_info), NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    ok( section_info.Attributes == SEC_COMMIT, "NtQuerySection wrong attr %08lx\n",
        section_info.Attributes );
    ok( section_info.BaseAddress == NULL, "NtQuerySection wrong base %p\n", section_info.BaseAddress );
    ok( section_info.Size.QuadPart == 0x4000, "NtQuerySection wrong size %lx%08lx\n",
        section_info.Size.u.HighPart, section_info.Size.u.LowPart );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info, sizeof(section_info)-1, NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySection failed err %lx\n", status );
    status = pNtQuerySection( mapping, SectionBasicInformation, &section_info, sizeof(section_info)+1, NULL );
    ok( !status, "NtQuerySection failed err %lx\n", status );
    status = pNtQuerySection( mapping, SectionImageInformation, &image_info, sizeof(image_info)-1, NULL );
    ok( status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySection failed err %lx\n", status );
    status = pNtQuerySection( mapping, SectionImageInformation, &image_info, sizeof(image_info), NULL );
    ok( status == STATUS_SECTION_NOT_IMAGE, "NtQuerySection failed err %lx\n", status );
    status = pNtQuerySection( mapping, SectionImageInformation, &image_info, sizeof(image_info)+1, NULL );
    ok( status == STATUS_SECTION_NOT_IMAGE, "NtQuerySection failed err %lx\n", status );
    if (sizeof(SIZE_T) > sizeof(int))
    {
        status = pNtQuerySection( mapping, SectionImageInformation, &image_info,
                                  sizeof(image_info) + ((SIZE_T)0x10000000 << 8), NULL );
        todo_wine
        ok( status == STATUS_ACCESS_VIOLATION, "NtQuerySection wrong err %lx\n", status );
    }
    CloseHandle(mapping);

    SetFilePointer(file, 0, NULL, FILE_BEGIN);
    SetEndOfFile(file);
    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               NULL, PAGE_READONLY, SEC_COMMIT, file );
    ok( status == STATUS_MAPPED_FILE_SIZE_ZERO, "NtCreateSection failed %lx\n", status );
    status = pNtCreateSection( &mapping, SECTION_QUERY | SECTION_MAP_READ, NULL,
                               NULL, PAGE_READONLY, SEC_IMAGE, file );
    ok( status == STATUS_INVALID_FILE_FOR_SECTION, "NtCreateSection failed %lx\n", status );

    CloseHandle(file);
    DeleteFileA(testfile);
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
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );

    file2 = CreateFileA( testfile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );

    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %lu\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );
    UnmapViewOfFile( ptr2 );

    ptr2 = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );

    map2 = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %lu\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );
    CloseHandle( file2 );

    status = pNtAreMappedFilesTheSame( ptr, ptr );
    ok( status == STATUS_SUCCESS || broken(status == STATUS_NOT_SAME_DEVICE),
        "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (char *)ptr + 30 );
    ok( status == STATUS_SUCCESS || broken(status == STATUS_NOT_SAME_DEVICE),
        "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)0xdeadbeef );
    ok( status == STATUS_CONFLICTING_ADDRESSES || status == STATUS_INVALID_ADDRESS,
        "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( ptr, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)GetProcessHeap() );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %lx\n", status );

    status = pNtAreMappedFilesTheSame( NULL, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %lx\n", status );

    ptr2 = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    ok( ptr2 != NULL, "VirtualAlloc error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %lx\n", status );
    VirtualFree( ptr2, 0, MEM_RELEASE );

    UnmapViewOfFile( ptr );
    CloseHandle( mapping );
    CloseHandle( file );

    status = pNtAreMappedFilesTheSame( GetModuleHandleA("ntdll.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       (char *)GetModuleHandleA("kernel32.dll") + 4096 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );

    GetSystemDirectoryA( path, MAX_PATH );
    strcat( path, "\\kernel32.dll" );
    file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %lx\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"), ptr );
    todo_wine
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping error %lu\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"), ptr );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );

    file2 = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( map2 != 0, "CreateFileMapping error %lu\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %lu\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );
    status = pNtAreMappedFilesTheSame( ptr2, ptr );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %lx\n", status );
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
    HANDLE handle, handle2, file[3];
    char path[MAX_PATH], filename[MAX_PATH];
    unsigned int i;
    NTSTATUS status;

    static const struct { DWORD file, flags, error, attrs; } sec_flag_tests[] =
    {
        /* anonymous mapping */
        { 0, SEC_RESERVE, 0 }, /* 0 */
        { 0, SEC_RESERVE | SEC_NOCACHE, 0 },
        { 0, SEC_RESERVE | SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER },
        { 0, SEC_RESERVE | SEC_WRITECOMBINE, 0 },
        { 0, SEC_COMMIT, 0 },
        { 0, SEC_COMMIT | SEC_NOCACHE, 0 }, /* 5 */
        { 0, SEC_COMMIT | SEC_WRITECOMBINE, 0 },
        { 0, SEC_RESERVE | SEC_COMMIT, ERROR_INVALID_PARAMETER },
        { 0, SEC_IMAGE, ERROR_BAD_EXE_FORMAT },
        { 0, SEC_IMAGE | SEC_RESERVE, ERROR_INVALID_PARAMETER },
        { 0, SEC_IMAGE | SEC_COMMIT, ERROR_INVALID_PARAMETER }, /* 10 */
        { 0, SEC_NOCACHE, ERROR_INVALID_PARAMETER },
        { 0, SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER },
        { 0, SEC_WRITECOMBINE, ERROR_INVALID_PARAMETER },
        { 0, SEC_FILE, ERROR_INVALID_PARAMETER },
        { 0, SEC_FILE | SEC_RESERVE, ERROR_INVALID_PARAMETER }, /* 15 */
        { 0, SEC_FILE | SEC_COMMIT, ERROR_INVALID_PARAMETER },
        { 0, 0, 0, SEC_COMMIT },
        /* normal file */
        { 1, SEC_RESERVE, 0, SEC_FILE },
        { 1, SEC_RESERVE | SEC_NOCACHE, 0, SEC_FILE | SEC_NOCACHE },
        { 1, SEC_RESERVE | SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER }, /* 20 */
        { 1, SEC_RESERVE | SEC_WRITECOMBINE, 0, SEC_FILE | SEC_WRITECOMBINE },
        { 1, SEC_COMMIT, 0, SEC_FILE },
        { 1, SEC_COMMIT | SEC_NOCACHE, 0, SEC_FILE | SEC_NOCACHE },
        { 1, SEC_COMMIT | SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER },
        { 1, SEC_COMMIT | SEC_WRITECOMBINE, 0, SEC_FILE | SEC_WRITECOMBINE }, /* 25 */
        { 1, SEC_RESERVE | SEC_COMMIT, ERROR_INVALID_PARAMETER },
        { 1, SEC_IMAGE, ERROR_BAD_EXE_FORMAT },
        { 1, SEC_IMAGE | SEC_RESERVE, ERROR_INVALID_PARAMETER },
        { 1, SEC_IMAGE | SEC_COMMIT, ERROR_INVALID_PARAMETER },
        { 1, SEC_NOCACHE, ERROR_INVALID_PARAMETER }, /* 30 */
        { 1, SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER },
        { 1, SEC_WRITECOMBINE, ERROR_INVALID_PARAMETER },
        { 1, SEC_FILE, ERROR_INVALID_PARAMETER },
        { 1, SEC_FILE | SEC_RESERVE, ERROR_INVALID_PARAMETER },
        { 1, SEC_FILE | SEC_COMMIT, ERROR_INVALID_PARAMETER }, /* 35 */
        { 1, 0, 0, SEC_FILE },
        /* PE image file */
        { 2, SEC_IMAGE, 0, SEC_FILE | SEC_IMAGE },
        { 2, SEC_IMAGE | SEC_FILE, ERROR_INVALID_PARAMETER },
        { 2, SEC_IMAGE | SEC_NOCACHE, 0, SEC_FILE | SEC_IMAGE },
        { 2, SEC_IMAGE | SEC_LARGE_PAGES, ERROR_INVALID_PARAMETER }, /* 40 */
        { 2, SEC_IMAGE | SEC_WRITECOMBINE, ERROR_INVALID_PARAMETER },
        { 2, SEC_IMAGE | SEC_RESERVE, ERROR_INVALID_PARAMETER },
        { 2, SEC_IMAGE | SEC_COMMIT, ERROR_INVALID_PARAMETER },
    };

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "Wine Test Mapping");
    ok( handle != NULL, "CreateFileMapping failed with error %lu\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                  "Wine Test Mapping");
    ok( handle2 != NULL, "CreateFileMapping failed with error %ld\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %lu\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "WINE TEST MAPPING");
    ok( handle2 != NULL, "CreateFileMapping failed with error %ld\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %lu\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "Wine Test Mapping");
    ok( handle2 != NULL, "OpenFileMapping failed with error %ld\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "WINE TEST MAPPING");
    ok( !handle2, "OpenFileMapping succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());

    CloseHandle( handle );

    /* test SEC_* flags */

    file[0] = INVALID_HANDLE_VALUE;
    GetTempPathA( MAX_PATH, path );
    GetTempFileNameA( path, "map", 0, filename );

    file[1] = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file[1] != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer( file[1], 0x2000, NULL, FILE_BEGIN );
    SetEndOfFile( file[1] );

    GetSystemDirectoryA( path, MAX_PATH );
    strcat( path, "\\kernel32.dll" );
    file[2] = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file[2] != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE(sec_flag_tests); i++)
    {
        DWORD flags = sec_flag_tests[i].flags;
        DWORD perm = sec_flag_tests[i].file == 2 ? PAGE_READONLY : PAGE_READWRITE;
        SetLastError( 0xdeadbeef );
        handle = CreateFileMappingA( file[sec_flag_tests[i].file], NULL,
                                     flags | perm, 0, 0x1000, "Wine Test Mapping" );
        if (sec_flag_tests[i].error)
        {
            ok( !handle, "%u: CreateFileMapping succeeded\n", i );
            ok( GetLastError() == sec_flag_tests[i].error, "%u: wrong error %lu\n", i, GetLastError());
        }
        else
        {
            /* SEC_WRITECOMBINE and SEC_IMAGE_NO_EXECUTE not supported on older Windows */
            BOOL new_flags = ((flags & SEC_WRITECOMBINE) ||
                              ((flags & SEC_IMAGE_NO_EXECUTE) == SEC_IMAGE_NO_EXECUTE));
            ok( handle != NULL || broken(new_flags),
                "%u: CreateFileMapping failed with error %lu\n", i, GetLastError());
            ok( GetLastError() == 0 || broken(new_flags && GetLastError() == ERROR_INVALID_PARAMETER),
                "%u: wrong error %lu\n", i, GetLastError());
        }

        if (handle)
        {
            SECTION_BASIC_INFORMATION info;
            DWORD expect = sec_flag_tests[i].attrs ? sec_flag_tests[i].attrs : sec_flag_tests[i].flags;

            status = pNtQuerySection( handle, SectionBasicInformation, &info, sizeof(info), NULL );
            ok( !status, "%u: NtQuerySection failed err %lx\n", i, status );
            /* SEC_NOCACHE not supported on older Windows */
            ok( info.Attributes == expect || broken( info.Attributes == (expect & ~SEC_NOCACHE) ),
                "%u: NtQuerySection wrong attr %08lx\n", i, info.Attributes );
            CloseHandle( handle );
        }
    }
    CloseHandle( file[1] );
    CloseHandle( file[2] );
    DeleteFileA( filename );
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

    ret = IsBadReadPtr((char *)NtCurrentTeb()->DeallocationStack + 4096, sizeof(DWORD));
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);

    ret = IsBadReadPtr((char *)NtCurrentTeb()->DeallocationStack + 4096, sizeof(DWORD));
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);
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

    ret = IsBadWritePtr((char *)NtCurrentTeb()->DeallocationStack + 4096, sizeof(DWORD));
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);

    ret = IsBadWritePtr((char *)NtCurrentTeb()->DeallocationStack + 4096, sizeof(DWORD));
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);
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

struct read_pipe_args
{
    HANDLE pipe;
    int index;
    void *base;
    DWORD size;
};

static const char testdata[] = "Hello World";

static DWORD CALLBACK read_pipe( void *arg )
{
    struct read_pipe_args *args = arg;
    DWORD num_bytes;
    BOOL success = ConnectNamedPipe( args->pipe, NULL );
    ok( success || GetLastError() == ERROR_PIPE_CONNECTED,
        "%u: ConnectNamedPipe failed %lu\n", args->index, GetLastError() );

    success = ReadFile( args->pipe, args->base, args->size, &num_bytes, NULL );
    ok( success, "%u: ReadFile failed %lu\n", args->index, GetLastError() );
    ok( num_bytes == sizeof(testdata), "%u: wrong number of bytes read %lu\n", args->index, num_bytes );
    ok( !memcmp( args->base, testdata, sizeof(testdata)),
        "%u: didn't receive expected data\n", args->index );
    return 0;
}

static void test_write_watch(void)
{
    static const char pipename[] = "\\\\.\\pipe\\test_write_watch_pipe";
    DWORD ret, size, old_prot, num_bytes;
    MEMORY_BASIC_INFORMATION info;
    HANDLE readpipe, writepipe, file;
    OVERLAPPED overlapped, *overlapped2;
    void *results[64];
    ULONG_PTR count;
    ULONG i, pagesize;
    BOOL success;
    char path[MAX_PATH], filename[MAX_PATH], *base;

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
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == PAGE_READWRITE, "wrong AllocationProtect %lx\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%lx\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%lx\n", info.Type );

    count = 64;
    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, NULL, size, results, &count, &pagesize );
    ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        broken( GetLastError() == 0xdeadbeef ), /* win98 */
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, GetModuleHandleW(NULL), size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }
    else  /* win98 */
    {
        ok( count == 0, "wrong count %Iu\n", count );
    }

    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    base[pagesize + 1] = 0x44;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    base[2*pagesize + 3] = 0x11;
    base[4*pagesize + 8] = 0x11;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 2, "wrong count %Iu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    count = 64;
    ret = pGetWriteWatch( 0, base + 3*pagesize, 2*pagesize, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    ret = pResetWriteWatch( base, 3*pagesize );
    ok( !ret, "pResetWriteWatch failed %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    *(DWORD *)(base + 2*pagesize - 2) = 0xdeadbeef;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 3, "wrong count %Iu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 2*pagesize, "wrong result %p\n", results[1] );
    ok( results[2] == base + 4*pagesize, "wrong result %p\n", results[2] );

    count = 1;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 2, "wrong count %Iu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    /* changing protections doesn't affect watches */

    ret = VirtualProtect( base, 3*pagesize, PAGE_READONLY, &old_prot );
    ok( ret, "VirtualProtect failed error %lu\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == 3*pagesize, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    ok( info.Protect == PAGE_READONLY, "wrong Protect 0x%lx\n", info.Protect );

    ret = VirtualProtect( base, 3*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %lu\n", GetLastError() );
    ok( old_prot == PAGE_READONLY, "wrong old prot %lx\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 2, "wrong count %Iu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == size, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%lx\n", info.Protect );

    /* ReadFile should trigger write watches */

    for (i = 0; i < 2; i++)
    {
        memset( &overlapped, 0, sizeof(overlapped) );
        overlapped.hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );

        readpipe = CreateNamedPipeA( pipename, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_INBOUND,
                                     (i ? PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE) | PIPE_WAIT, 1, 1024, 1024,
                                     NMPWAIT_USE_DEFAULT_WAIT, NULL );
        ok( readpipe != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed %lu\n", GetLastError() );

        success = ConnectNamedPipe( readpipe, &overlapped );
        ok( !success, "%lu: ConnectNamedPipe unexpectedly succeeded\n", i );
        ok( GetLastError() == ERROR_IO_PENDING, "%lu: expected ERROR_IO_PENDING, got %lu\n",
            i, GetLastError() );

        writepipe = CreateFileA( pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( writepipe != INVALID_HANDLE_VALUE, "%lu: CreateFileA failed %lu\n", i, GetLastError() );

        ret = WaitForSingleObject( overlapped.hEvent, 1000 );
        ok( ret == WAIT_OBJECT_0, "%lu: expected WAIT_OBJECT_0, got %lu\n", i, ret );

        memset( base, 0, size );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 16, "%lu: wrong count %Iu\n", i, count );

        success = ReadFile( readpipe, base, size, NULL, &overlapped );
        ok( !success, "%lu: ReadFile unexpectedly succeeded\n", i );
        ok( GetLastError() == ERROR_IO_PENDING, "%lu: expected ERROR_IO_PENDING, got %lu\n",
            i, GetLastError() );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 16, "%lu: wrong count %Iu\n", i, count );

        num_bytes = 0;
        success = WriteFile( writepipe, testdata, sizeof(testdata), &num_bytes, NULL );
        ok( success, "%lu: WriteFile failed %lu\n", i, GetLastError() );
        ok( num_bytes == sizeof(testdata), "%lu: wrong number of bytes written %lu\n", i, num_bytes );

        num_bytes = 0;
        success = GetOverlappedResult( readpipe, &overlapped, &num_bytes, TRUE );
        ok( success, "%lu: GetOverlappedResult failed %lu\n", i, GetLastError() );
        ok( num_bytes == sizeof(testdata), "%lu: wrong number of bytes read %lu\n", i, num_bytes );
        ok( !memcmp( base, testdata, sizeof(testdata)), "%lu: didn't receive expected data\n", i );

        count = 64;
        memset( results, 0, sizeof(results) );
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 1, "%lu: wrong count %Iu\n", i, count );
        ok( results[0] == base, "%lu: wrong result %p\n", i, results[0] );

        CloseHandle( readpipe );
        CloseHandle( writepipe );
        CloseHandle( overlapped.hEvent );
    }

    for (i = 0; i < 2; i++)
    {
        struct read_pipe_args args;
        HANDLE thread;

        readpipe = CreateNamedPipeA( pipename, PIPE_ACCESS_INBOUND,
                                     (i ? PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE) | PIPE_WAIT, 1, 1024, 1024,
                                     NMPWAIT_USE_DEFAULT_WAIT, NULL );
        ok( readpipe != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed %lu\n", GetLastError() );

        memset( base, 0, size );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 16, "%lu: wrong count %Iu\n", i, count );

        args.pipe = readpipe;
        args.index = i;
        args.base = base;
        args.size = size;
        thread = CreateThread( NULL, 0, read_pipe, &args, 0, NULL );

        writepipe = CreateFileA( pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
        ok( writepipe != INVALID_HANDLE_VALUE, "%lu: CreateFileA failed %lu\n", i, GetLastError() );
        Sleep( 200 );

        count = 64;
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 16, "%lu: wrong count %Iu\n", i, count );

        num_bytes = 0;
        success = WriteFile( writepipe, testdata, sizeof(testdata), &num_bytes, NULL );
        ok( success, "%lu: WriteFile failed %lu\n", i, GetLastError() );
        ok( num_bytes == sizeof(testdata), "%lu: wrong number of bytes written %lu\n", i, num_bytes );
        WaitForSingleObject( thread, 10000 );

        count = 64;
        memset( results, 0, sizeof(results) );
        ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
        ok( !ret, "%lu: GetWriteWatch failed %lu\n", i, GetLastError() );
        ok( count == 1, "%lu: wrong count %Iu\n", i, count );
        ok( results[0] == base, "%lu: wrong result %p\n", i, results[0] );

        CloseHandle( readpipe );
        CloseHandle( writepipe );
        CloseHandle( thread );
    }

    GetTempPathA( MAX_PATH, path );
    GetTempFileNameA( path, "map", 0, filename );
    file = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    SetFilePointer( file, 2 * pagesize + 3, NULL, FILE_BEGIN );
    SetEndOfFile( file );
    SetFilePointer( file, 0, NULL, FILE_BEGIN );

    success = ReadFile( file, base, size, &num_bytes, NULL );
    ok( success, "ReadFile failed %lu\n", GetLastError() );
    ok( num_bytes == 2 * pagesize + 3, "wrong bytes %lu\n", num_bytes );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

    success = ReadFile( file, base, size, &num_bytes, NULL );
    ok( success, "ReadFile failed %lu\n", GetLastError() );
    ok( num_bytes == 0, "wrong bytes %lu\n", num_bytes );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

    CloseHandle( file );
    DeleteFileA( filename );

    success = ReadFile( (HANDLE)0xdead, base, size, &num_bytes, NULL );
    ok( !success, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    /* OVERLAPPED structure write watch */
    memset( &overlapped, 0, sizeof(overlapped) );
    overlapped.hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );

    readpipe = CreateNamedPipeA( pipename, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_INBOUND,
                                 PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 1024, 1024,
                                 NMPWAIT_USE_DEFAULT_WAIT, NULL );
    ok( readpipe != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed %lu\n", GetLastError() );

    success = ConnectNamedPipe( readpipe, &overlapped );
    ok( !success, "ConnectNamedPipe unexpectedly succeeded\n" );
    ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %lu\n", GetLastError() );

    writepipe = CreateFileA( pipename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    ok( writepipe != INVALID_HANDLE_VALUE, "CreateFileA failed %lu\n", GetLastError() );

    ret = WaitForSingleObject( overlapped.hEvent, 1000 );
    ok( ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %lu\n", ret );

    memset( base, 0, size );
    overlapped2 = (OVERLAPPED*)(base + size - sizeof(*overlapped2));
    overlapped2->hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 16, "wrong count %Iu\n", count );

    success = ReadFile( readpipe, base, sizeof(testdata), NULL, overlapped2 );
    ok( !success, "ReadFile unexpectedly succeeded\n" );
    ok( GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %lu\n", GetLastError() );
    overlapped2->Internal = 0xdeadbeef;

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 2, "wrong count %Iu\n", count );

    num_bytes = 0;
    success = WriteFile( writepipe, testdata, sizeof(testdata), &num_bytes, NULL );
    ok( success, "WriteFile failed %lu\n", GetLastError() );
    ok( num_bytes == sizeof(testdata), "wrong number of bytes written %lu\n", num_bytes );

    num_bytes = 0;
    success = GetOverlappedResult( readpipe, overlapped2, &num_bytes, TRUE );
    ok( success, "GetOverlappedResult failed %lu\n", GetLastError() );
    ok( num_bytes == sizeof(testdata), "wrong number of bytes read %lu\n", num_bytes );
    ok( !memcmp( base, testdata, sizeof(testdata)), "didn't receive expected data\n" );

    count = 64;
    memset( results, 0, sizeof(results) );
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 2, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    CloseHandle( readpipe );
    CloseHandle( writepipe );
    CloseHandle( overlapped.hEvent );
    CloseHandle( overlapped2->hEvent );

    /* some invalid parameter tests */

    SetLastError( 0xdeadbeef );
    count = 0;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pGetWriteWatch( 0, base, size, results, NULL, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, results, &count, NULL );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size * 2, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base + size - pagesize, pagesize + 1, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( base, 0 );
        ok( ret == ~0u, "ResetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( GetModuleHandleW(NULL), size );
        ok( ret == ~0u, "ResetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }
    else  /* win98 is completely different */
    {
        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ERROR_INVALID_PARAMETER, "GetWriteWatch succeeded %lu\n", ret );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %lu\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %lu\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %lu\n", ret );

        ret = pResetWriteWatch( base, 0 );
        ok( !ret, "ResetWriteWatch failed %lu\n", ret );

        ret = pResetWriteWatch( GetModuleHandleW(NULL), size );
        ok( !ret, "ResetWriteWatch failed %lu\n", ret );
    }

    VirtualFree( base, 0, MEM_RELEASE );

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    VirtualFree( base, 0, MEM_RELEASE );

    base = VirtualAlloc( 0, size, MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( !base, "VirtualAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    /* initial protect doesn't matter */

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    base = VirtualAlloc( base, size, MEM_COMMIT, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    ret = VirtualProtect( base, 6*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %lu\n", GetLastError() );
    ok( old_prot == PAGE_NOACCESS, "wrong old prot %lx\n", old_prot );

    base[5*pagesize + 200] = 3;

    ret = VirtualProtect( base, 6*pagesize, PAGE_NOACCESS, &old_prot );
    ok( ret, "VirtualProtect failed error %lu\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    ret = VirtualFree( base, size, MEM_DECOMMIT );
    ok( ret, "VirtualFree failed %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1 || broken(count == 0), /* win98 */
        "wrong count %Iu\n", count );
    if (count) ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    VirtualFree( base, 0, MEM_RELEASE );
}

#if defined(__i386__) || defined(__x86_64__)

static DWORD WINAPI stack_commit_func( void *arg )
{
    volatile char *p = (char *)&p;

    /* trigger all guard pages, to ensure that the pages are committed */
    while (p >= (char *)NtCurrentTeb()->DeallocationStack + 4 * 0x1000)
    {
        p[0] |= 0;
        p -= 0x1000;
    }

    ok( arg == (void *)0xdeadbeef, "expected 0xdeadbeef, got %p\n", arg );
    return 42;
}

static void test_stack_commit(void)
{
#ifdef __i386__
    static const char code_call_on_stack[] = {
        0x55,                   /* pushl %ebp */
        0x56,                   /* pushl %esi */
        0x89, 0xe6,             /* movl %esp,%esi */
        0x8b, 0x4c, 0x24, 0x0c, /* movl 12(%esp),%ecx - func */
        0x8b, 0x54, 0x24, 0x10, /* movl 16(%esp),%edx - arg */
        0x8b, 0x44, 0x24, 0x14, /* movl 20(%esp),%eax - stack */
        0x83, 0xe0, 0xf0,       /* andl $~15,%eax */
        0x83, 0xe8, 0x0c,       /* subl $12,%eax */
        0x89, 0xc4,             /* movl %eax,%esp */
        0x52,                   /* pushl %edx */
        0x31, 0xed,             /* xorl %ebp,%ebp */
        0xff, 0xd1,             /* call *%ecx */
        0x89, 0xf4,             /* movl %esi,%esp */
        0x5e,                   /* popl %esi */
        0x5d,                   /* popl %ebp */
        0xc2, 0x0c, 0x00 };     /* ret $12 */
#else
    static const char code_call_on_stack[] = {
        0x55,                   /* pushq %rbp */
        0x48, 0x89, 0xe5,       /* movq %rsp,%rbp */
                                /* %rcx - func, %rdx - arg, %r8 - stack */
        0x48, 0x87, 0xca,       /* xchgq %rcx,%rdx */
        0x49, 0x83, 0xe0, 0xf0, /* andq $~15,%r8 */
        0x49, 0x83, 0xe8, 0x20, /* subq $0x20,%r8 */
        0x4c, 0x89, 0xc4,       /* movq %r8,%rsp */
        0xff, 0xd2,             /* callq *%rdx */
        0x48, 0x89, 0xec,       /* movq %rbp,%rsp */
        0x5d,                   /* popq %rbp */
        0xc3 };                 /* ret */
#endif
    DWORD (WINAPI *call_on_stack)( DWORD (WINAPI *func)(void *), void *arg, void *stack );
    void *old_stack, *old_stack_base, *old_stack_limit;
    void *new_stack, *new_stack_base;
    DWORD result;

    call_on_stack = VirtualAlloc( 0, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    ok( call_on_stack != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    memcpy( call_on_stack, code_call_on_stack, sizeof(code_call_on_stack) );

    /* allocate a new stack, only the first guard page is committed */
    new_stack = VirtualAlloc( 0, 0x400000, MEM_RESERVE, PAGE_READWRITE );
    ok( new_stack != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    new_stack_base = (char *)new_stack + 0x400000;
    VirtualAlloc( (char *)new_stack_base - 0x1000, 0x1000, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD );

    old_stack       = NtCurrentTeb()->DeallocationStack;
    old_stack_base  = NtCurrentTeb()->Tib.StackBase;
    old_stack_limit = NtCurrentTeb()->Tib.StackLimit;

    NtCurrentTeb()->DeallocationStack  = new_stack;
    NtCurrentTeb()->Tib.StackBase      = new_stack_base;
    NtCurrentTeb()->Tib.StackLimit     = new_stack_base;

    result = call_on_stack( stack_commit_func, (void *)0xdeadbeef, new_stack_base );

    NtCurrentTeb()->DeallocationStack  = old_stack;
    NtCurrentTeb()->Tib.StackBase      = old_stack_base;
    NtCurrentTeb()->Tib.StackLimit     = old_stack_limit;

    ok( result == 42, "expected 42, got %lu\n", result );

    VirtualFree( new_stack, 0, MEM_RELEASE );
    VirtualFree( call_on_stack, 0, MEM_RELEASE );
}

#endif  /* defined(__i386__) || defined(__x86_64__) */
#ifdef __i386__
#ifndef __REACTOS__

static LONG num_guard_page_calls;

static DWORD guard_page_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                 CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace( "exception: %08lx flags:%lx addr:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->NumberParameters == 2, "NumberParameters is %ld instead of 2\n", rec->NumberParameters );
    ok( rec->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION, "ExceptionCode is %08lx instead of %08lx\n",
        rec->ExceptionCode, STATUS_GUARD_PAGE_VIOLATION );

    InterlockedIncrement( &num_guard_page_calls );
    *(int *)rec->ExceptionInformation[1] += 0x100;

    return ExceptionContinueExecution;
}

static void test_guard_page(void)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    MEMORY_BASIC_INFORMATION info;
    DWORD ret, size, old_prot;
    LONG *value, old_value;
    void *results[64];
    ULONG_PTR count;
    ULONG pagesize;
    BOOL success;
    char *base;

    size = 0x1000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    value = (LONG *)base;

    /* verify info structure */
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok( ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == (PAGE_READWRITE | PAGE_GUARD), "wrong AllocationProtect %lx\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    ok( info.Protect == (PAGE_READWRITE | PAGE_GUARD), "wrong Protect 0x%lx\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%lx\n", info.Type );

    /* put some initial value into the memory */
    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );
    ok( old_prot == (PAGE_READWRITE | PAGE_GUARD), "wrong old prot %lx\n", old_prot );

    *value       = 1;
    *(value + 1) = 2;

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

    /* test behaviour of VirtualLock - first attempt should fail */
    SetLastError( 0xdeadbeef );
    success = VirtualLock( base, size );
    ok( !success, "VirtualLock unexpectedly succeeded\n" );
    todo_wine
    ok( GetLastError() == STATUS_GUARD_PAGE_VIOLATION, "wrong error %lu\n", GetLastError() );

    success = VirtualLock( base, size );
    todo_wine
    ok( success, "VirtualLock failed %lu\n", GetLastError() );
    if (success)
    {
        ok( *value == 1, "memory block contains wrong value, expected 1, got 0x%lx\n", *value );
        success = VirtualUnlock( base, size );
        ok( success, "VirtualUnlock failed %lu\n", GetLastError() );
    }

    /* check info structure again, PAGE_GUARD should be removed now */
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok( ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == (PAGE_READWRITE | PAGE_GUARD), "wrong AllocationProtect %lx\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    todo_wine
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%lx\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%lx\n", info.Type );

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );
    todo_wine
    ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

    /* test directly accessing the memory - we need to setup an exception handler first */
    frame.Handler = guard_page_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    InterlockedExchange( &old_value, *value ); /* exception handler increments value by 0x100 */
    *value = 2;
    ok( old_value == 0x101, "memory block contains wrong value, expected 0x101, got 0x%lx\n", old_value );
    ok( num_guard_page_calls == 1, "expected one callback of guard page handler, got %ld calls\n", num_guard_page_calls );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    /* check info structure again, PAGE_GUARD should be removed now */
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok( ret, "VirtualQuery failed %lu\n", GetLastError());
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%lx\n", info.Protect );

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

    /* test accessing second integer in memory */
    frame.Handler = guard_page_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    old_value = *(value + 1);
    ok( old_value == 0x102, "memory block contains wrong value, expected 0x102, got 0x%lx\n", old_value );
    ok( *value == 2, "memory block contains wrong value, expected 2, got 0x%lx\n", *value );
    ok( num_guard_page_calls == 1, "expected one callback of guard page handler, got %ld calls\n", num_guard_page_calls );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    success = VirtualLock( base, size );
    ok( success, "VirtualLock failed %lu\n", GetLastError() );
    if (success)
    {
        ok( *value == 2, "memory block contains wrong value, expected 2, got 0x%lx\n", *value );
        success = VirtualUnlock( base, size );
        ok( success, "VirtualUnlock failed %lu\n", GetLastError() );
    }

    VirtualFree( base, 0, MEM_RELEASE );

    /* combined guard page / write watch tests */
    if (!pGetWriteWatch || !pResetWriteWatch)
    {
        win_skip( "GetWriteWatch not supported, skipping combined guard page / write watch tests\n" );
        return;
    }

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE | PAGE_GUARD  );
    if (!base && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED))
    {
        win_skip( "MEM_WRITE_WATCH not supported\n" );
        return;
    }
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );
    value = (LONG *)base;

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok( ret, "VirtualQuery failed %lu\n", GetLastError() );
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == (PAGE_READWRITE | PAGE_GUARD), "wrong AllocationProtect %lx\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%Ix\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%lx\n", info.State );
    ok( info.Protect == (PAGE_READWRITE | PAGE_GUARD), "wrong Protect 0x%lx\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%lx\n", info.Type );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    /* writing to a page should trigger should trigger guard page, even if write watch is set */
    frame.Handler = guard_page_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    *value       = 1;
    *(value + 1) = 2;
    ok( num_guard_page_calls == 1, "expected one callback of guard page handler, got %ld calls\n", num_guard_page_calls );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    /* write watch is triggered from inside of the guard page handler */
    frame.Handler = guard_page_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    old_value = *(value + 1); /* doesn't trigger write watch */
    ok( old_value == 0x102, "memory block contains wrong value, expected 0x102, got 0x%lx\n", old_value );
    ok( *value == 1, "memory block contains wrong value, expected 1, got 0x%lx\n", *value );
    ok( num_guard_page_calls == 1, "expected one callback of guard page handler, got %ld calls\n", num_guard_page_calls );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    /* test behaviour of VirtualLock - first attempt should fail without triggering write watches */
    SetLastError( 0xdeadbeef );
    success = VirtualLock( base, size );
    ok( !success, "VirtualLock unexpectedly succeeded\n" );
    todo_wine
    ok( GetLastError() == STATUS_GUARD_PAGE_VIOLATION, "wrong error %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    success = VirtualLock( base, size );
    todo_wine
    ok( success, "VirtualLock failed %lu\n", GetLastError() );
    if (success)
    {
        ok( *value == 1, "memory block contains wrong value, expected 1, got 0x%lx\n", *value );
        success = VirtualUnlock( base, size );
        ok( success, "VirtualUnlock failed %lu\n", GetLastError() );
    }

    count = 64;
    results[0] = (void *)0xdeadbeef;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    todo_wine
    ok( count == 1 || broken(count == 0) /* Windows 8 */, "wrong count %Iu\n", count );
    todo_wine
    ok( results[0] == base || broken(results[0] == (void *)0xdeadbeef) /* Windows 8 */, "wrong result %p\n", results[0] );

    VirtualFree( base, 0, MEM_RELEASE );
}

static LONG num_execute_fault_calls;

static DWORD execute_fault_seh_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ULONG flags = MEM_EXECUTE_OPTION_ENABLE;
    DWORD err;

    trace( "exception: %08lx flags:%lx addr:%p info[0]:%Id info[1]:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           rec->ExceptionInformation[0], (void *)rec->ExceptionInformation[1] );

    ok( rec->NumberParameters == 2, "NumberParameters is %ld instead of 2\n", rec->NumberParameters );
    ok( rec->ExceptionCode == STATUS_ACCESS_VIOLATION || rec->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION,
        "ExceptionCode is %08lx instead of STATUS_ACCESS_VIOLATION or STATUS_GUARD_PAGE_VIOLATION\n", rec->ExceptionCode );

    NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &flags, sizeof(flags), NULL );

    if (rec->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {

        err = IsProcessorFeaturePresent( PF_NX_ENABLED ) ? EXCEPTION_EXECUTE_FAULT : EXCEPTION_READ_FAULT;
        ok( rec->ExceptionInformation[0] == err, "ExceptionInformation[0] is %ld instead of %ld\n",
            (DWORD)rec->ExceptionInformation[0], err );

        InterlockedIncrement( &num_guard_page_calls );
    }
    else if (rec->ExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        DWORD old_prot;
        BOOL success;

        err = (flags & MEM_EXECUTE_OPTION_DISABLE) ? EXCEPTION_EXECUTE_FAULT : EXCEPTION_READ_FAULT;
        ok( rec->ExceptionInformation[0] == err, "ExceptionInformation[0] is %ld instead of %ld\n",
            (DWORD)rec->ExceptionInformation[0], err );

        success = VirtualProtect( (void *)rec->ExceptionInformation[1], 16, PAGE_EXECUTE_READWRITE, &old_prot );
        ok( success, "VirtualProtect failed %lu\n", GetLastError() );
        ok( old_prot == PAGE_READWRITE, "wrong old prot %lx\n", old_prot );

        InterlockedIncrement( &num_execute_fault_calls );
    }

    return ExceptionContinueExecution;
}

static LONG CALLBACK execute_fault_vec_handler( EXCEPTION_POINTERS *ExceptionInfo )
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    DWORD old_prot;
    BOOL success;

    trace( "exception: %08lx flags:%lx addr:%p info[0]:%Id info[1]:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           rec->ExceptionInformation[0], (void *)rec->ExceptionInformation[1] );

    ok( rec->NumberParameters == 2, "NumberParameters is %ld instead of 2\n", rec->NumberParameters );
    ok( rec->ExceptionCode == STATUS_ACCESS_VIOLATION,
        "ExceptionCode is %08lx instead of STATUS_ACCESS_VIOLATION\n", rec->ExceptionCode );

    if (rec->ExceptionCode == STATUS_ACCESS_VIOLATION)
        InterlockedIncrement( &num_execute_fault_calls );

    if (rec->ExceptionInformation[0] == EXCEPTION_READ_FAULT)
        return EXCEPTION_CONTINUE_SEARCH;

    success = VirtualProtect( (void *)rec->ExceptionInformation[1], 16, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );
    ok( old_prot == PAGE_NOACCESS, "wrong old prot %lx\n", old_prot );

    return EXCEPTION_CONTINUE_EXECUTION;
}

static inline DWORD send_message_excpt( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DWORD ret;

    frame.Handler = execute_fault_seh_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    InterlockedExchange( &num_execute_fault_calls, 0 );
    ret = SendMessageA( hWnd, uMsg, wParam, lParam );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    return ret;
}

static inline DWORD call_proc_excpt( DWORD (CALLBACK *code)(void *), void *arg )
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DWORD ret;

    frame.Handler = execute_fault_seh_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = &frame;

    InterlockedExchange( &num_guard_page_calls, 0 );
    InterlockedExchange( &num_execute_fault_calls, 0 );
    ret = code( arg );

    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;

    return ret;
}

static LRESULT CALLBACK jmp_test_func( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (uMsg == WM_USER)
        return 42;

    return DefWindowProcA( hWnd, uMsg, wParam, lParam );
}

static LRESULT CALLBACK atl_test_func( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    DWORD arg = (DWORD)hWnd;
    if (uMsg == WM_USER)
        ok( arg == 0x11223344, "arg is 0x%08lx instead of 0x11223344\n", arg );
    else
        ok( arg != 0x11223344, "arg is unexpectedly 0x11223344\n" );
    return 43;
}

static DWORD CALLBACK atl5_test_func( void )
{
    return 44;
}

static void test_atl_thunk_emulation( ULONG dep_flags )
{
    static const char code_jmp[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    static const char code_atl1[] = {0xC7, 0x44, 0x24, 0x04, 0x44, 0x33, 0x22, 0x11, 0xE9, 0x00, 0x00, 0x00, 0x00};
    static const char code_atl2[] = {0xB9, 0x44, 0x33, 0x22, 0x11, 0xE9, 0x00, 0x00, 0x00, 0x00};
    static const char code_atl3[] = {0xBA, 0x44, 0x33, 0x22, 0x11, 0xB9, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE1};
    static const char code_atl4[] = {0xB9, 0x44, 0x33, 0x22, 0x11, 0xB8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0};
    static const char code_atl5[] = {0x59, 0x58, 0x51, 0xFF, 0x60, 0x04};
    static const char cls_name[] = "atl_thunk_class";
    DWORD ret, size, old_prot;
    ULONG old_flags = MEM_EXECUTE_OPTION_ENABLE;
    BOOL success, restore_flags = FALSE;
    void *results[64];
    ULONG_PTR count;
    ULONG pagesize;
    WNDCLASSEXA wc;
    char *base;
    HWND hWnd;

    trace( "Running DEP tests with ProcessExecuteFlags = %ld\n", dep_flags );

    NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &old_flags, sizeof(old_flags), NULL );
    if (old_flags != dep_flags)
    {
        ret = NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &dep_flags, sizeof(dep_flags) );
        if (ret == STATUS_INVALID_INFO_CLASS /* Windows 2000 */ ||
            ret == STATUS_ACCESS_DENIED)
        {
            win_skip( "Skipping DEP tests with ProcessExecuteFlags = %ld\n", dep_flags );
            return;
        }
        ok( !ret, "NtSetInformationProcess failed with status %08lx\n", ret );
        restore_flags = TRUE;
    }

    size = 0x1000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );

    /* Check result of GetProcessDEPPolicy */
    if (!pGetProcessDEPPolicy)
        win_skip( "GetProcessDEPPolicy not supported\n" );
    else
    {
        BOOL (WINAPI *get_dep_policy)(HANDLE, LPDWORD, PBOOL) = (void *)base;
        BOOL policy_permanent = 0xdeadbeef;
        DWORD policy_flags = 0xdeadbeef;

        /* GetProcessDEPPolicy crashes on Windows when a NULL pointer is passed.
         * Moreover this function has a bug on Windows 8, which has the effect that
         * policy_permanent is set to the content of the CL register instead of 0,
         * when the policy is not permanent. To detect that we use an assembler
         * wrapper to call the function. */

        memcpy( base, code_atl2, sizeof(code_atl2) );
        *(DWORD *)(base + 6) = (DWORD_PTR)pGetProcessDEPPolicy - (DWORD_PTR)(base + 10);

        success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
        ok( success, "VirtualProtect failed %lu\n", GetLastError() );

        success = get_dep_policy( GetCurrentProcess(), &policy_flags, &policy_permanent );
        ok( success, "GetProcessDEPPolicy failed %lu\n", GetLastError() );

        ret = 0;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
            ret |= PROCESS_DEP_ENABLE;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            ret |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;

        ok( policy_flags == ret, "expected policy flags %ld, got %ld\n", ret, policy_flags );
        ok( !policy_permanent || broken(policy_permanent == 0x44),
            "expected policy permanent FALSE, got %d\n", policy_permanent );
    }

    memcpy( base, code_jmp, sizeof(code_jmp) );
    *(DWORD *)(base + 1) = (DWORD_PTR)jmp_test_func - (DWORD_PTR)(base + 5);

    /* On Windows, the ATL Thunk emulation is only enabled while running WndProc functions,
     * whereas in Wine such a limitation doesn't exist yet. We want to test in a scenario
     * where it is active, so that application which depend on that still work properly.
     * We have no exception handler enabled yet, so give proper EXECUTE permissions to
     * prevent crashes while creating the window. */

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    memset( &wc, 0, sizeof(wc) );
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandleA( 0 );
    wc.hCursor       = LoadCursorA( NULL, (LPCSTR)IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszClassName = cls_name;
    wc.lpfnWndProc   = (WNDPROC)base;
    success = RegisterClassExA(&wc) != 0;
    ok( success, "RegisterClassExA failed %lu\n", GetLastError() );

    hWnd = CreateWindowExA(0, cls_name, "Test", WS_TILEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok( hWnd != 0, "CreateWindowExA failed %lu\n", GetLastError() );

    ret = SendMessageA(hWnd, WM_USER, 0, 0);
    ok( ret == 42, "SendMessage returned unexpected result %ld\n", ret );

    /* At first try with an instruction which is not recognized as proper ATL thunk
     * by the Windows ATL Thunk Emulator. Removing execute permissions will lead to
     * STATUS_ACCESS_VIOLATION exceptions when DEP is enabled. */

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && !IsProcessorFeaturePresent( PF_NX_ENABLED ))
    {
        trace( "DEP hardware support is not available\n" );
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
        dep_flags = MEM_EXECUTE_OPTION_ENABLE;
    }
    else if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
    {
        trace( "DEP hardware support is available\n" );
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    }
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Now a bit more complicated, the page containing the code is protected with
     * PAGE_GUARD memory protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 1, "expected one STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Now test with a proper ATL thunk instruction. */

    memcpy( base, code_atl1, sizeof(code_atl1) );
    *(DWORD *)(base + 9) = (DWORD_PTR)atl_test_func - (DWORD_PTR)(base + 13);

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = SendMessageA(hWnd, WM_USER, 0, 0);
    ok( ret == 43, "SendMessage returned unexpected result %ld\n", ret );

    /* Try executing with PAGE_READWRITE protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Now a bit more complicated, the page containing the code is protected with
     * PAGE_GUARD memory protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    /* the same, but with PAGE_GUARD set */
    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 1, "expected one STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* The following test shows that on Windows, even a vectored exception handler
     * cannot intercept internal exceptions thrown by the ATL thunk emulation layer. */

    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && !(dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
    {
        if (pRtlAddVectoredExceptionHandler && pRtlRemoveVectoredExceptionHandler)
        {
            PVOID vectored_handler;

            success = VirtualProtect( base, size, PAGE_NOACCESS, &old_prot );
            ok( success, "VirtualProtect failed %lu\n", GetLastError() );

            vectored_handler = pRtlAddVectoredExceptionHandler( TRUE, &execute_fault_vec_handler );
            ok( vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n" );

            ret = send_message_excpt( hWnd, WM_USER, 0, 0 );

            pRtlRemoveVectoredExceptionHandler( vectored_handler );

            ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
            ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
        }
        else
            win_skip( "RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n" );
    }

    /* Test alternative ATL thunk instructions. */

    memcpy( base, code_atl2, sizeof(code_atl2) );
    *(DWORD *)(base + 6) = (DWORD_PTR)atl_test_func - (DWORD_PTR)(base + 10);

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER + 1, 0, 0 );
    /* FIXME: we don't check the content of the register ECX yet */
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    memcpy( base, code_atl3, sizeof(code_atl3) );
    *(DWORD *)(base + 6) = (DWORD_PTR)atl_test_func;

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER + 1, 0, 0 );
    /* FIXME: we don't check the content of the registers ECX/EDX yet */
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    memcpy( base, code_atl4, sizeof(code_atl4) );
    *(DWORD *)(base + 6) = (DWORD_PTR)atl_test_func;

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER + 1, 0, 0 );
    /* FIXME: We don't check the content of the registers EAX/ECX yet */
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
        ok( num_execute_fault_calls == 0 || broken(num_execute_fault_calls == 1) /* Windows XP */,
            "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    memcpy( base, code_atl5, sizeof(code_atl5) );

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    results[1] = atl5_test_func;
    ret = call_proc_excpt( (void *)base, results );
    /* FIXME: We don't check the content of the registers EAX/ECX yet */
    ok( ret == 44, "call returned wrong result, expected 44, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
        ok( num_execute_fault_calls == 0 || broken(num_execute_fault_calls == 1) /* Windows XP */,
            "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Restore the JMP instruction, set to executable, and then destroy the Window */

    memcpy( base, code_jmp, sizeof(code_jmp) );
    *(DWORD *)(base + 1) = (DWORD_PTR)jmp_test_func - (DWORD_PTR)(base + 5);

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    DestroyWindow( hWnd );

    success = UnregisterClassA( cls_name, GetModuleHandleA(0) );
    ok( success, "UnregisterClass failed %lu\n", GetLastError() );

    VirtualFree( base, 0, MEM_RELEASE );

    /* Repeat the tests from above with MEM_WRITE_WATCH protected memory. */

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE  );
    if (!base && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED))
    {
        win_skip( "MEM_WRITE_WATCH not supported\n" );
        goto out;
    }
    ok( base != NULL, "VirtualAlloc failed %lu\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    memcpy( base, code_jmp, sizeof(code_jmp) );
    *(DWORD *)(base + 1) = (DWORD_PTR)jmp_test_func - (DWORD_PTR)(base + 5);

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    /* Create a new window class and associated Window (see above) */

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    memset( &wc, 0, sizeof(wc) );
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandleA( 0 );
    wc.hCursor       = LoadCursorA( NULL, (LPCSTR)IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszClassName = cls_name;
    wc.lpfnWndProc   = (WNDPROC)base;
    success = RegisterClassExA(&wc) != 0;
    ok( success, "RegisterClassExA failed %lu\n", GetLastError() );

    hWnd = CreateWindowExA(0, cls_name, "Test", WS_TILEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok( hWnd != 0, "CreateWindowExA failed %lu\n", GetLastError() );

    ret = SendMessageA(hWnd, WM_USER, 0, 0);
    ok( ret == 42, "SendMessage returned unexpected result %ld\n", ret );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    /* At first try with an instruction which is not recognized as proper ATL thunk
     * by the Windows ATL Thunk Emulator. Removing execute permissions will lead to
     * STATUS_ACCESS_VIOLATION exceptions when DEP is enabled. */

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Now a bit more complicated, the page containing the code is protected with
     * PAGE_GUARD memory protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 1, "expected one STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 42, "call returned wrong result, expected 42, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0 || broken(count == 1) /* Windows 8 */, "wrong count %Iu\n", count );

    /* Now test with a proper ATL thunk instruction. */

    memcpy( base, code_atl1, sizeof(code_atl1) );
    *(DWORD *)(base + 9) = (DWORD_PTR)atl_test_func - (DWORD_PTR)(base + 13);

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = SendMessageA(hWnd, WM_USER, 0, 0);
    ok( ret == 43, "SendMessage returned unexpected result %ld\n", ret );

    /* Try executing with PAGE_READWRITE protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0, "wrong count %Iu\n", count );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    /* Now a bit more complicated, the page containing the code is protected with
     * PAGE_GUARD memory protection. */

    success = VirtualProtect( base, size, PAGE_READWRITE | PAGE_GUARD, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    /* the same, but with PAGE_GUARD set */
    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 1, "expected one STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    if ((dep_flags & MEM_EXECUTE_OPTION_DISABLE) && (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION))
        ok( num_execute_fault_calls == 1, "expected one STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );
    else
        ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    ret = send_message_excpt( hWnd, WM_USER, 0, 0 );
    ok( ret == 43, "call returned wrong result, expected 43, got %ld\n", ret );
    ok( num_guard_page_calls == 0, "expected no STATUS_GUARD_PAGE_VIOLATION exception, got %ld exceptions\n", num_guard_page_calls );
    ok( num_execute_fault_calls == 0, "expected no STATUS_ACCESS_VIOLATION exception, got %ld exceptions\n", num_execute_fault_calls );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 0 || broken(count == 1) /* Windows 8 */, "wrong count %Iu\n", count );

    /* Restore the JMP instruction, set to executable, and then destroy the Window */

    memcpy( base, code_jmp, sizeof(code_jmp) );
    *(DWORD *)(base + 1) = (DWORD_PTR)jmp_test_func - (DWORD_PTR)(base + 5);

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %lu\n", GetLastError() );
    ok( count == 1, "wrong count %Iu\n", count );
    ok( results[0] == base, "wrong result %p\n", results[0] );

    success = VirtualProtect( base, size, PAGE_EXECUTE_READWRITE, &old_prot );
    ok( success, "VirtualProtect failed %lu\n", GetLastError() );

    DestroyWindow( hWnd );

    success = UnregisterClassA( cls_name, GetModuleHandleA(0) );
    ok( success, "UnregisterClass failed %lu\n", GetLastError() );

    VirtualFree( base, 0, MEM_RELEASE );

out:
    if (restore_flags)
    {
        ret = NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &old_flags, sizeof(old_flags) );
        ok( !ret, "NtSetInformationProcess failed with status %08lx\n", ret );
    }
}
#endif
#endif  /* __i386__ */

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
    void *addr;
    SIZE_T size;
    NTSTATUS status;

    SetLastError(0xdeadbeef);
    base = VirtualAlloc(0, si.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    ok(base != NULL, "VirtualAlloc failed %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = VirtualProtect(base, si.dwPageSize, PAGE_READONLY, NULL);
    ok(!ret, "VirtualProtect should fail\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected ERROR_NOACCESS, got %ld\n", GetLastError());
    old_prot = 0xdeadbeef;
    ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(ret, "VirtualProtect failed %ld\n", GetLastError());
    ok(old_prot == PAGE_NOACCESS, "got %#lx != expected PAGE_NOACCESS\n", old_prot);

    addr = base;
    size = si.dwPageSize;
    status = pNtProtectVirtualMemory(GetCurrentProcess(), &addr, &size, PAGE_READONLY, NULL);
    ok(status == STATUS_ACCESS_VIOLATION, "NtProtectVirtualMemory should fail, got %08lx\n", status);
    addr = base;
    size = si.dwPageSize;
    old_prot = 0xdeadbeef;
    status = pNtProtectVirtualMemory(GetCurrentProcess(), &addr, &size, PAGE_NOACCESS, &old_prot);
    ok(status == STATUS_SUCCESS, "NtProtectVirtualMemory should succeed, got %08lx\n", status);
    ok(old_prot == PAGE_NOACCESS, "got %#lx != expected PAGE_NOACCESS\n", old_prot);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        SetLastError(0xdeadbeef);
        ret = VirtualQuery(base, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %ld\n", GetLastError());
        ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
        ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
        ok(info.Protect == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == PAGE_NOACCESS, "%ld: %#lx != PAGE_NOACCESS\n", i, info.AllocationProtect);
        ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
        ok(info.Type == MEM_PRIVATE, "%ld: %#lx != MEM_PRIVATE\n", i, info.Type);

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, td[i].prot_set, &old_prot);
        if (td[i].prot_get)
        {
            ok(ret, "%ld: VirtualProtect error %ld\n", i, GetLastError());
            ok(old_prot == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %ld\n", GetLastError());
            ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == td[i].prot_get, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot_get);
            ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_NOACCESS, "%ld: %#lx != PAGE_NOACCESS\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_PRIVATE, "%ld: %#lx != MEM_PRIVATE\n", i, info.Type);
        }
        else
        {
            ok(!ret, "%ld: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
        ok(ret, "%ld: VirtualProtect error %ld\n", i, GetLastError());
        if (td[i].prot_get)
            ok(old_prot == td[i].prot_get, "%ld: got %#lx != expected %#lx\n", i, old_prot, td[i].prot_get);
        else
            ok(old_prot == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, old_prot);
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
                ok(!ptr, "VirtualAlloc(%02lx) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
            }
            else
            {
                if (prot & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                {
                    ok(!ptr, "VirtualAlloc(%02lx) should fail\n", prot);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
                }
                else
                {
                    ok(ptr != NULL, "VirtualAlloc(%02lx) error %ld\n", prot, GetLastError());
                    ok(ptr == base, "expected %p, got %p\n", base, ptr);
                }
            }

            SetLastError(0xdeadbeef);
            ret = VirtualProtect(base, si.dwPageSize, prot, &old_prot);
            if ((rw_prot && exec_prot) || (!rw_prot && !exec_prot))
            {
                ok(!ret, "VirtualProtect(%02lx) should fail\n", prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
            }
            else
            {
                if (prot & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
                {
                    ok(!ret, "VirtualProtect(%02lx) should fail\n", prot);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
                }
                else
                    ok(ret, "VirtualProtect(%02lx) error %ld\n", prot, GetLastError());
            }

            rw_prot = 1 << j;
        }

        exec_prot = 1 << (i + 4);
    }

    VirtualFree(base, 0, MEM_RELEASE);
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

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        SetLastError(0xdeadbeef);
        base = VirtualAlloc(0, si.dwPageSize, MEM_COMMIT, td[i].prot);

        if (td[i].success)
        {
            ok(base != NULL, "%ld: VirtualAlloc failed %ld\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %ld\n", GetLastError());
            ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == td[i].prot, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot);
            ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == td[i].prot, "%ld: %#lx != %#lx\n", i, info.AllocationProtect, td[i].prot);
            ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_PRIVATE, "%ld: %#lx != MEM_PRIVATE\n", i, info.Type);

            if (is_mem_writable(info.Protect))
            {
                base[0] = 0xfe;

                SetLastError(0xdeadbeef);
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "VirtualQuery failed %ld\n", GetLastError());
                ok(info.Protect == td[i].prot, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot);
            }

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, td[i].prot);
            ok(ptr == base, "%ld: VirtualAlloc failed %ld\n", i, GetLastError());

            VirtualFree(base, 0, MEM_RELEASE);
        }
        else
        {
            ok(!base, "%ld: VirtualAlloc should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
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
    DWORD ret, i, alloc_prot, old_prot;
    MEMORY_BASIC_INFORMATION info;
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    HANDLE hfile, hmap;
    BOOL page_exec_supported = TRUE;

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "map", 0, file_name);

    SetLastError(0xdeadbeef);
    hfile = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %ld\n", file_name, GetLastError());
    SetFilePointer(hfile, si.dwPageSize, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        SetLastError(0xdeadbeef);
        hmap = CreateFileMappingW(hfile, NULL, td[i].prot | SEC_COMMIT, 0, si.dwPageSize, NULL);

        if (td[i].success)
        {
            if (!hmap)
            {
                trace("%ld: CreateFileMapping(%04lx) failed: %ld\n", i, td[i].prot, GetLastError());
                /* NT4 and win2k don't support EXEC on file mappings */
                if (td[i].prot == PAGE_EXECUTE_READ || td[i].prot == PAGE_EXECUTE_READWRITE)
                {
                    page_exec_supported = FALSE;
                    ok(broken(!hmap), "%ld: CreateFileMapping doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
                if (td[i].prot == PAGE_EXECUTE_WRITECOPY)
                {
                    page_exec_supported = FALSE;
                    ok(broken(!hmap), "%ld: CreateFileMapping doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                    continue;
                }
            }
            ok(hmap != 0, "%ld: CreateFileMapping(%04lx) error %ld\n", i, td[i].prot, GetLastError());

            base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
            ok(base != NULL, "%ld: MapViewOfFile failed %ld\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %ld\n", GetLastError());
            ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == PAGE_READONLY, "%ld: got %#lx != expected PAGE_READONLY\n", i, info.Protect);
            ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == PAGE_READONLY, "%ld: %#lx != PAGE_READONLY\n", i, info.AllocationProtect);
            ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_MAPPED, "%ld: %#lx != MEM_MAPPED\n", i, info.Type);

            SetLastError(0xdeadbeef);
            ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, td[i].prot);
            ok(!ptr, "%ld: VirtualAlloc(%02lx) should fail\n", i, td[i].prot);
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%ld: expected ERROR_ACCESS_DENIED, got %ld\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualProtect(base, si.dwPageSize, td[i].prot, &old_prot);
            if (td[i].prot == PAGE_READONLY || td[i].prot == PAGE_WRITECOPY)
                ok(ret, "%ld: VirtualProtect(%02lx) error %ld\n", i, td[i].prot, GetLastError());
            else
            {
                ok(!ret, "%ld: VirtualProtect(%02lx) should fail\n", i, td[i].prot);
                ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
            }

            UnmapViewOfFile(base);
            CloseHandle(hmap);
        }
        else
        {
            ok(!hmap, "%ld: CreateFileMapping should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
        }
    }

    if (page_exec_supported) alloc_prot = PAGE_EXECUTE_READWRITE;
    else alloc_prot = PAGE_READWRITE;
    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingW(hfile, NULL, alloc_prot, 0, si.dwPageSize, NULL);
    ok(hmap != 0, "%ld: CreateFileMapping error %ld\n", i, GetLastError());

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        SetLastError(0xdeadbeef);
        base = MapViewOfFile(hmap, FILE_MAP_READ | FILE_MAP_WRITE | (page_exec_supported ? FILE_MAP_EXECUTE : 0), 0, 0, 0);
        ok(base != NULL, "MapViewOfFile failed %ld\n", GetLastError());

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
        ok(ret, "VirtualProtect error %ld\n", GetLastError());
        ok(old_prot == alloc_prot, "got %#lx != expected %#lx\n", old_prot, alloc_prot);

        SetLastError(0xdeadbeef);
        ret = VirtualQuery(base, &info, sizeof(info));
        ok(ret, "VirtualQuery failed %ld\n", GetLastError());
        ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
        ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
        ok(info.Protect == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, info.Protect);
        ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
        ok(info.AllocationProtect == alloc_prot, "%ld: %#lx != %#lx\n", i, info.AllocationProtect, alloc_prot);
        ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
        ok(info.Type == MEM_MAPPED, "%ld: %#lx != MEM_MAPPED\n", i, info.Type);

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
                    ok(broken(!ret), "%ld: VirtualProtect doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* NT4 and win2k don't support EXEC on file mappings */
                if (td[i].prot == PAGE_EXECUTE_READ || td[i].prot == PAGE_EXECUTE_READWRITE)
                {
                    ok(broken(!ret), "%ld: VirtualProtect doesn't support PAGE_EXECUTE\n", i);
                    continue;
                }
                /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
                if (td[i].prot == PAGE_EXECUTE_WRITECOPY)
                {
                    ok(broken(!ret), "%ld: VirtualProtect doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                    continue;
                }
            }

            ok(ret, "%ld: VirtualProtect error %ld\n", i, GetLastError());
            ok(old_prot == PAGE_NOACCESS, "%ld: got %#lx != expected PAGE_NOACCESS\n", i, old_prot);

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "VirtualQuery failed %ld\n", GetLastError());
            ok(info.BaseAddress == base, "%ld: got %p != expected %p\n", i, info.BaseAddress, base);
            ok(info.RegionSize == si.dwPageSize, "%ld: got %#Ix != expected %#lx\n", i, info.RegionSize, si.dwPageSize);
            ok(info.Protect == td[i].prot, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot);
            ok(info.AllocationBase == base, "%ld: %p != %p\n", i, info.AllocationBase, base);
            ok(info.AllocationProtect == alloc_prot, "%ld: %#lx != %#lx\n", i, info.AllocationProtect, alloc_prot);
            ok(info.State == MEM_COMMIT, "%ld: %#lx != MEM_COMMIT\n", i, info.State);
            ok(info.Type == MEM_MAPPED, "%ld: %#lx != MEM_MAPPED\n", i, info.Type);

            if (is_mem_writable(info.Protect))
            {
                base[0] = 0xfe;

                SetLastError(0xdeadbeef);
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "VirtualQuery failed %ld\n", GetLastError());
                /* FIXME: remove the condition below once Wine is fixed */
                todo_wine_if (td[i].prot == PAGE_WRITECOPY || td[i].prot == PAGE_EXECUTE_WRITECOPY)
                    ok(info.Protect == td[i].prot_after_write, "%ld: got %#lx != expected %#lx\n", i, info.Protect, td[i].prot_after_write);
            }
        }
        else
        {
            ok(!ret, "%ld: VirtualProtect should fail\n", i);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%ld: expected ERROR_INVALID_PARAMETER, got %ld\n", i, GetLastError());
            continue;
        }

        old_prot = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = VirtualProtect(base, si.dwPageSize, PAGE_NOACCESS, &old_prot);
        ok(ret, "%ld: VirtualProtect error %ld\n", i, GetLastError());
        /* FIXME: remove the condition below once Wine is fixed */
        todo_wine_if (td[i].prot == PAGE_WRITECOPY || td[i].prot == PAGE_EXECUTE_WRITECOPY)
            ok(old_prot == td[i].prot_after_write, "%ld: got %#lx != expected %#lx\n", i, old_prot, td[i].prot_after_write);

        UnmapViewOfFile(base);
    }

    CloseHandle(hmap);

    CloseHandle(hfile);
    DeleteFileA(file_name);
}

#define ACCESS_READ      0x01
#define ACCESS_WRITE     0x02
#define ACCESS_EXECUTE   0x04

static DWORD page_prot_to_access(DWORD prot)
{
    switch (prot)
    {
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
        return ACCESS_READ;
    case PAGE_READWRITE:
        return ACCESS_READ | ACCESS_WRITE;
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        return ACCESS_READ | ACCESS_EXECUTE;
    case PAGE_EXECUTE_READWRITE:
        return ACCESS_READ | ACCESS_WRITE | ACCESS_EXECUTE;
    default:
        return 0;
    }
}

static BOOL is_compatible_protection(DWORD view_prot, DWORD prot)
{
    DWORD view_access, prot_access;

    view_access = page_prot_to_access(view_prot);
    prot_access = page_prot_to_access(prot);

    return ((view_access & prot_access) == prot_access);
}

static DWORD map_prot_to_access(DWORD prot)
{
    switch (prot)
    {
    case PAGE_READWRITE:
        return SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_QUERY;
    case PAGE_EXECUTE_READWRITE:
        return SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE | SECTION_QUERY;
    case PAGE_READONLY:
    case PAGE_WRITECOPY:
        return SECTION_MAP_READ | SECTION_QUERY;
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_WRITECOPY:
        return SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_QUERY;
    default:
        return 0;
    }
}

static DWORD map_prot_no_write(DWORD prot)
{
    switch (prot)
    {
    case PAGE_READWRITE: return PAGE_WRITECOPY;
    case PAGE_EXECUTE_READWRITE: return PAGE_EXECUTE_WRITECOPY;
    default: return prot;
    }
}

static DWORD map_prot_written(DWORD prot)
{
    switch (prot)
    {
    case PAGE_WRITECOPY: return PAGE_READWRITE;
    case PAGE_EXECUTE_WRITECOPY: return PAGE_EXECUTE_READWRITE;
    default: return prot;
    }
}

static DWORD file_access_to_prot( DWORD access )
{
    BOOL exec = access & FILE_MAP_EXECUTE;
    access &= ~FILE_MAP_EXECUTE;

    if (access == FILE_MAP_COPY) return exec ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
    if (access & FILE_MAP_WRITE) return exec ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    if (access & FILE_MAP_READ)  return exec ? PAGE_EXECUTE_READ : PAGE_READONLY;
    return PAGE_NOACCESS;
}

static BOOL is_compatible_access(DWORD map_prot, DWORD view_prot)
{
    DWORD access = map_prot_to_access(map_prot);
    DWORD view_access = map_prot_to_access( file_access_to_prot( view_prot ));
    if (!view_access) view_access = SECTION_MAP_READ;
    return (view_access & access) == view_access;
}

static void *map_view_of_file(HANDLE handle, DWORD access)
{
    NTSTATUS status;
    LARGE_INTEGER offset;
    SIZE_T count;
    ULONG protect;
    void *addr;

    if (!pNtMapViewOfSection) return NULL;

    count = 0;
    offset.u.LowPart  = 0;
    offset.u.HighPart = 0;

    protect = file_access_to_prot( access );
    addr = NULL;
    status = pNtMapViewOfSection(handle, GetCurrentProcess(), &addr, 0, 0, &offset,
                                 &count, 1 /* ViewShare */, 0, protect);
    if ((int)status < 0) addr = NULL;
    return addr;
}

static void test_mapping( HANDLE hfile, DWORD sec_flags, BOOL readonly )
{
    static const DWORD page_prot[] =
    {
        PAGE_NOACCESS, PAGE_READONLY, PAGE_READWRITE, PAGE_WRITECOPY,
        PAGE_EXECUTE, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_EXECUTE_WRITECOPY
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
    DWORD i, j, k, ret, old_prot, prev_prot, alloc_prot;
    HANDLE hmap;
    MEMORY_BASIC_INFORMATION info, nt_info;
    BOOL anon_mapping = (hfile == INVALID_HANDLE_VALUE);

    trace( "testing %s mapping flags %08lx %s\n", anon_mapping ? "anonymous" : "file",
            sec_flags, readonly ? "readonly file" : "" );
    for (i = 0; i < ARRAY_SIZE(page_prot); i++)
    {
        SetLastError(0xdeadbeef);
        hmap = CreateFileMappingW(hfile, NULL, page_prot[i] | sec_flags, 0, 2*si.dwPageSize, NULL);

        if (readonly && (page_prot[i] == PAGE_READWRITE || page_prot[i] == PAGE_EXECUTE_READ
                    || page_prot[i] == PAGE_EXECUTE_READWRITE || page_prot[i] == PAGE_EXECUTE_WRITECOPY))
        {
            todo_wine_if(page_prot[i] == PAGE_EXECUTE_READ || page_prot[i] == PAGE_EXECUTE_WRITECOPY)
            {
                ok(!hmap, "%ld: CreateFileMapping(%04lx) should fail\n", i, page_prot[i]);
                ok(GetLastError() == ERROR_ACCESS_DENIED || broken(GetLastError() == ERROR_INVALID_PARAMETER),
                        "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
            }
            if (hmap) CloseHandle(hmap);
            continue;
        }

        if (page_prot[i] == PAGE_NOACCESS)
        {
            HANDLE hmap2;

            ok(!hmap, "CreateFileMapping(PAGE_NOACCESS) should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

            /* A trick to create a not accessible mapping */
            SetLastError(0xdeadbeef);
            if (sec_flags & SEC_IMAGE)
                hmap = CreateFileMappingW(hfile, NULL, PAGE_WRITECOPY | sec_flags, 0, si.dwPageSize, NULL);
            else
                hmap = CreateFileMappingW(hfile, NULL, PAGE_READONLY | sec_flags, 0, si.dwPageSize, NULL);
            ok(hmap != 0, "CreateFileMapping(PAGE_READWRITE) error %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            ret = DuplicateHandle(GetCurrentProcess(), hmap, GetCurrentProcess(), &hmap2, 0, FALSE, 0);
            ok(ret, "DuplicateHandle error %ld\n", GetLastError());
            CloseHandle(hmap);
            hmap = hmap2;
        }
        if (page_prot[i] == PAGE_EXECUTE)
        {
            ok(!hmap, "CreateFileMapping(PAGE_EXECUTE) should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
            continue;
        }

        if (!hmap)
        {
            trace("%ld: CreateFileMapping(%04lx) failed: %ld\n", i, page_prot[i], GetLastError());

            if ((sec_flags & SEC_IMAGE) &&
                (page_prot[i] == PAGE_READWRITE || page_prot[i] == PAGE_EXECUTE_READWRITE))
                continue;  /* SEC_IMAGE doesn't support write access */

            /* NT4 and win2k don't support EXEC on file mappings */
            if (page_prot[i] == PAGE_EXECUTE_READ || page_prot[i] == PAGE_EXECUTE_READWRITE)
            {
                ok(broken(!hmap), "%ld: CreateFileMapping doesn't support PAGE_EXECUTE\n", i);
                continue;
            }
            /* Vista+ supports PAGE_EXECUTE_WRITECOPY, earlier versions don't */
            if (page_prot[i] == PAGE_EXECUTE_WRITECOPY)
            {
                ok(broken(!hmap), "%ld: CreateFileMapping doesn't support PAGE_EXECUTE_WRITECOPY\n", i);
                continue;
            }
        }

        ok(hmap != 0, "%ld: CreateFileMapping(%04lx) error %ld\n", i, page_prot[i], GetLastError());

        for (j = 0; j < ARRAY_SIZE(view); j++)
        {
            nt_base = map_view_of_file(hmap, view[j].access);
            if (nt_base)
            {
                SetLastError(0xdeadbeef);
                ret = VirtualQuery(nt_base, &nt_info, sizeof(nt_info));
                ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                UnmapViewOfFile(nt_base);
            }

            SetLastError(0xdeadbeef);
            base = MapViewOfFile(hmap, view[j].access, 0, 0, 0);

            /* Vista+ supports FILE_MAP_EXECUTE properly, earlier versions don't */
            ok(!nt_base == !base ||
               broken((view[j].access & FILE_MAP_EXECUTE) && !nt_base != !base),
               "%ld: (%04lx/%04lx) NT %p kernel %p\n", j, page_prot[i], view[j].access, nt_base, base);

            if (!is_compatible_access(page_prot[i], view[j].access))
            {
                /* FILE_MAP_EXECUTE | FILE_MAP_COPY broken on XP */
                if (base != NULL && view[j].access == (FILE_MAP_EXECUTE | FILE_MAP_COPY))
                {
                    ok( broken(base != NULL), "%ld: MapViewOfFile(%04lx/%04lx) should fail\n",
                        j, page_prot[i], view[j].access);
                    UnmapViewOfFile( base );
                }
                else
                {
                    ok(!base, "%ld: MapViewOfFile(%04lx/%04lx) should fail\n",
                       j, page_prot[i], view[j].access);
                    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());
                }
                continue;
            }

            /* Vista+ properly supports FILE_MAP_EXECUTE, earlier versions don't */
            if (!base && (view[j].access & FILE_MAP_EXECUTE))
            {
                ok(broken(!base), "%ld: MapViewOfFile(%04lx/%04lx) failed %ld\n", j, page_prot[i], view[j].access, GetLastError());
                continue;
            }

            ok(base != NULL, "%ld: MapViewOfFile(%04lx/%04lx) failed %ld\n", j, page_prot[i], view[j].access, GetLastError());

            SetLastError(0xdeadbeef);
            ret = VirtualQuery(base, &info, sizeof(info));
            ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
            ok(info.BaseAddress == base, "%ld: (%04lx) got %p, expected %p\n", j, view[j].access, info.BaseAddress, base);
            ok(info.RegionSize == 2*si.dwPageSize || (info.RegionSize == si.dwPageSize && (sec_flags & SEC_IMAGE)),
               "%ld: (%04lx) got %#Ix != expected %#lx\n", j, view[j].access, info.RegionSize, 2*si.dwPageSize);
            if (sec_flags & SEC_IMAGE)
                ok(info.Protect == PAGE_READONLY,
                    "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, info.Protect, view[j].prot);
            else
                ok(info.Protect == view[j].prot ||
                   broken(view[j].prot == PAGE_EXECUTE_READ && info.Protect == PAGE_READONLY) || /* win2k */
                   broken(view[j].prot == PAGE_EXECUTE_READWRITE && info.Protect == PAGE_READWRITE) || /* win2k */
                   broken(view[j].prot == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
                   "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, info.Protect, view[j].prot);
            ok(info.AllocationBase == base, "%ld: (%04lx) got %p, expected %p\n", j, view[j].access, info.AllocationBase, base);
            if (sec_flags & SEC_IMAGE)
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%ld: (%04lx) got %#lx, expected %#lx\n",
                   j, view[j].access, info.AllocationProtect, info.Protect);
            else
                ok(info.AllocationProtect == info.Protect, "%ld: (%04lx) got %#lx, expected %#lx\n",
                   j, view[j].access, info.AllocationProtect, info.Protect);
            ok(info.State == MEM_COMMIT, "%ld: (%04lx) got %#lx, expected MEM_COMMIT\n", j, view[j].access, info.State);
            ok(info.Type == (sec_flags & SEC_IMAGE) ? SEC_IMAGE : MEM_MAPPED,
               "%ld: (%04lx) got %#lx, expected MEM_MAPPED\n", j, view[j].access, info.Type);

            if (nt_base && base)
            {
                ok(nt_info.RegionSize == info.RegionSize, "%ld: (%04lx) got %#Ix != expected %#Ix\n", j, view[j].access, nt_info.RegionSize, info.RegionSize);
                ok(nt_info.Protect == info.Protect /* Vista+ */ ||
                   broken(nt_info.AllocationProtect == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
                   "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, nt_info.Protect, info.Protect);
                ok(nt_info.AllocationProtect == info.AllocationProtect /* Vista+ */ ||
                   broken(nt_info.AllocationProtect == PAGE_EXECUTE_WRITECOPY && info.Protect == PAGE_NOACCESS), /* XP */
                   "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, nt_info.AllocationProtect, info.AllocationProtect);
                ok(nt_info.State == info.State, "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, nt_info.State, info.State);
                ok(nt_info.Type == info.Type, "%ld: (%04lx) got %#lx, expected %#lx\n", j, view[j].access, nt_info.Type, info.Type);
            }

            prev_prot = info.Protect;
            alloc_prot = info.AllocationProtect;

            for (k = 0; k < ARRAY_SIZE(page_prot); k++)
            {
                /*trace("map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);*/
                DWORD actual_prot = (sec_flags & SEC_IMAGE) ? map_prot_no_write(page_prot[k]) : page_prot[k];
                SetLastError(0xdeadbeef);
                old_prot = 0xdeadbeef;
                ret = VirtualProtect(base, si.dwPageSize, page_prot[k], &old_prot);
                if (is_compatible_protection(alloc_prot, actual_prot))
                {
                    /* win2k and XP don't support EXEC on file mappings */
                    if (!ret && (page_prot[k] == PAGE_EXECUTE || page_prot[k] == PAGE_EXECUTE_WRITECOPY || view[j].prot == PAGE_EXECUTE_WRITECOPY))
                    {
                        ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE\n");
                        continue;
                    }

                    todo_wine_if(readonly && page_prot[k] == PAGE_WRITECOPY && view[j].prot != PAGE_WRITECOPY)
                    ok(ret, "VirtualProtect error %ld, map %#lx, view %#lx, requested prot %#lx\n", GetLastError(), page_prot[i], view[j].prot, page_prot[k]);
                    todo_wine_if(readonly && page_prot[k] == PAGE_WRITECOPY && view[j].prot != PAGE_WRITECOPY)
                    ok(old_prot == prev_prot, "got %#lx, expected %#lx\n", old_prot, prev_prot);
                    prev_prot = actual_prot;

                    ret = VirtualQuery(base, &info, sizeof(info));
                    ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                    todo_wine_if(readonly && page_prot[k] == PAGE_WRITECOPY && view[j].prot != PAGE_WRITECOPY)
                    ok(info.Protect == actual_prot,
                       "VirtualProtect wrong prot, map %#lx, view %#lx, requested prot %#lx got %#lx\n",
                       page_prot[i], view[j].prot, page_prot[k], info.Protect );
                }
                else
                {
                    ok(!ret, "VirtualProtect should fail, map %#lx, view %#lx, requested prot %#lx\n", page_prot[i], view[j].prot, page_prot[k]);
                    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
                }
            }

            for (k = 0; k < ARRAY_SIZE(page_prot); k++)
            {
                /*trace("map %#x, view %#x, requested prot %#x\n", page_prot[i], view[j].prot, page_prot[k]);*/
                SetLastError(0xdeadbeef);
                ptr = VirtualAlloc(base, si.dwPageSize, MEM_COMMIT, page_prot[k]);
                if (anon_mapping)
                {
                    if (is_compatible_protection(view[j].prot, page_prot[k]))
                    {
                        ok(ptr != NULL, "VirtualAlloc error %lu, map %#lx, view %#lx, requested prot %#lx\n",
                           GetLastError(), page_prot[i], view[j].prot, page_prot[k]);
                    }
                    else
                    {
                        /* versions <= Vista accept all protections without checking */
                        ok(!ptr || broken(ptr != NULL),
                           "VirtualAlloc should fail, map %#lx, view %#lx, requested prot %#lx\n",
                           page_prot[i], view[j].prot, page_prot[k]);
                        if (!ptr) ok( GetLastError() == ERROR_INVALID_PARAMETER,
                                      "wrong error %lu\n", GetLastError());
                    }
                    if (ptr)
                    {
                        ret = VirtualQuery(base, &info, sizeof(info));
                        ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                        ok(info.Protect == page_prot[k] ||
                           /* if the mapping doesn't have write access,
                            *  broken versions silently switch to WRITECOPY */
                           broken( info.Protect == map_prot_no_write(page_prot[k]) ),
                           "VirtualAlloc wrong prot, map %#lx, view %#lx, requested prot %#lx got %#lx\n",
                           page_prot[i], view[j].prot, page_prot[k], info.Protect );
                    }
                }
                else
                {
                    ok(!ptr, "VirtualAlloc(%02lx) should fail\n", page_prot[k]);
                    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %ld\n", GetLastError());
                }
            }

            if (!anon_mapping && is_compatible_protection(alloc_prot, PAGE_WRITECOPY))
            {
                ret = VirtualProtect(base, sec_flags & SEC_IMAGE ? si.dwPageSize : 2*si.dwPageSize, PAGE_WRITECOPY, &old_prot);
                todo_wine_if(readonly && view[j].prot != PAGE_WRITECOPY)
                ok(ret, "VirtualProtect error %ld, map %#lx, view %#lx\n", GetLastError(), page_prot[i], view[j].prot);
                if (ret) *(DWORD*)base = 0xdeadbeef;
                ret = VirtualQuery(base, &info, sizeof(info));
                ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                todo_wine
                ok(info.Protect == PAGE_READWRITE, "VirtualProtect wrong prot, map %#lx, view %#lx got %#lx\n",
                   page_prot[i], view[j].prot, info.Protect );
                todo_wine_if (!(sec_flags & SEC_IMAGE))
                ok(info.RegionSize == si.dwPageSize, "wrong region size %#Ix after write, map %#lx, view %#lx got %#lx\n",
                   info.RegionSize, page_prot[i], view[j].prot, info.Protect );

                prev_prot = info.Protect;
                alloc_prot = info.AllocationProtect;

                if (!(sec_flags & SEC_IMAGE))
                {
                    ret = VirtualQuery((char*)base + si.dwPageSize, &info, sizeof(info));
                    ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                    todo_wine_if(readonly && view[j].prot != PAGE_WRITECOPY)
                    ok(info.Protect == PAGE_WRITECOPY, "wrong prot, map %#lx, view %#lx got %#lx\n",
                       page_prot[i], view[j].prot, info.Protect);
                }

                for (k = 0; k < ARRAY_SIZE(page_prot); k++)
                {
                    DWORD actual_prot = (sec_flags & SEC_IMAGE) ? map_prot_no_write(page_prot[k]) : page_prot[k];
                    SetLastError(0xdeadbeef);
                    old_prot = 0xdeadbeef;
                    ret = VirtualProtect(base, si.dwPageSize, page_prot[k], &old_prot);
                    if (is_compatible_protection(alloc_prot, actual_prot))
                    {
                        /* win2k and XP don't support EXEC on file mappings */
                        if (!ret && (page_prot[k] == PAGE_EXECUTE || page_prot[k] == PAGE_EXECUTE_WRITECOPY || view[j].prot == PAGE_EXECUTE_WRITECOPY))
                        {
                            ok(broken(!ret), "VirtualProtect doesn't support PAGE_EXECUTE\n");
                            continue;
                        }

                        todo_wine_if(readonly && page_prot[k] == PAGE_WRITECOPY && view[j].prot != PAGE_WRITECOPY)
                        ok(ret, "VirtualProtect error %ld, map %#lx, view %#lx, requested prot %#lx\n", GetLastError(), page_prot[i], view[j].prot, page_prot[k]);
                        todo_wine_if(readonly && page_prot[k] == PAGE_WRITECOPY && view[j].prot != PAGE_WRITECOPY)
                        ok(old_prot == prev_prot, "got %#lx, expected %#lx\n", old_prot, prev_prot);

                        ret = VirtualQuery(base, &info, sizeof(info));
                        ok(ret, "%ld: VirtualQuery failed %ld\n", j, GetLastError());
                        todo_wine_if( map_prot_written( page_prot[k] ) != actual_prot )
                        ok(info.Protect == map_prot_written( page_prot[k] ),
                           "VirtualProtect wrong prot, map %#lx, view %#lx, requested prot %#lx got %#lx\n",
                           page_prot[i], view[j].prot, page_prot[k], info.Protect );
                        prev_prot = info.Protect;
                    }
                    else
                    {
                        ok(!ret, "VirtualProtect should fail, map %#lx, view %#lx, requested prot %#lx\n", page_prot[i], view[j].prot, page_prot[k]);
                        ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
                    }
                }
            }
            UnmapViewOfFile(base);
        }

        CloseHandle(hmap);
    }
}

static void test_mappings(void)
{
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    DWORD data, num_bytes;
    HANDLE hfile;

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "map", 0, file_name);

    hfile = CreateFileA(file_name, GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %ld\n", file_name, GetLastError());
    SetFilePointer(hfile, 2*si.dwPageSize, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    test_mapping( hfile, SEC_COMMIT, FALSE );

    /* test that file was not modified */
    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);
    ok(ReadFile(hfile, &data, sizeof(data), &num_bytes, NULL), "ReadFile failed\n");
    ok(num_bytes == sizeof(data), "num_bytes = %ld\n", num_bytes);
    todo_wine
    ok(!data, "data = %lx\n", data);

    CloseHandle( hfile );

    hfile = CreateFileA(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %ld\n", file_name, GetLastError());

    test_mapping( hfile, SEC_COMMIT, TRUE );

    CloseHandle( hfile );
    DeleteFileA( file_name );

    /* SEC_IMAGE mapping */
    GetSystemDirectoryA( file_name, MAX_PATH );
    strcat( file_name, "\\kernel32.dll" );

    hfile = CreateFileA( file_name, GENERIC_READ|GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile(%s) error %ld\n", file_name, GetLastError());

    test_mapping( hfile, SEC_IMAGE, FALSE );

    CloseHandle( hfile );

    /* now anonymous mappings */
    test_mapping( INVALID_HANDLE_VALUE, SEC_COMMIT, FALSE );
}

static void test_shared_memory(BOOL is_child)
{
    HANDLE mapping;
    LONG *p;

    SetLastError(0xdeadbef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "winetest_virtual.c");
    ok(mapping != 0, "CreateFileMapping error %ld\n", GetLastError());
    if (is_child)
        ok(GetLastError() == ERROR_ALREADY_EXISTS, "expected ERROR_ALREADY_EXISTS, got %ld\n", GetLastError());

    SetLastError(0xdeadbef);
    p = MapViewOfFile(mapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 4096);
    ok(p != NULL, "MapViewOfFile error %ld\n", GetLastError());

    if (is_child)
    {
        ok(*p == 0x1a2b3c4d, "expected 0x1a2b3c4d in child, got %#lx\n", *p);
    }
    else
    {
        char **argv;
        char cmdline[MAX_PATH];
        PROCESS_INFORMATION pi;
        STARTUPINFOA si = { sizeof(si) };
        DWORD ret;

        *p = 0x1a2b3c4d;

        winetest_get_mainargs(&argv);
        sprintf(cmdline, "\"%s\" virtual sharedmem", argv[0]);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
        wait_child_process(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    UnmapViewOfFile(p);
    CloseHandle(mapping);
}

static void test_shared_memory_ro(BOOL is_child, DWORD child_access)
{
    HANDLE mapping;
    LONG *p;

    SetLastError(0xdeadbef);
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "winetest_virtual.c_ro");
    ok(mapping != 0, "CreateFileMapping error %ld\n", GetLastError());
    if (is_child)
        ok(GetLastError() == ERROR_ALREADY_EXISTS, "expected ERROR_ALREADY_EXISTS, got %ld\n", GetLastError());

    SetLastError(0xdeadbef);
    p = MapViewOfFile(mapping, is_child ? child_access : FILE_MAP_READ, 0, 0, 4096);
    ok(p != NULL, "MapViewOfFile error %ld\n", GetLastError());

    if (is_child)
    {
        *p = 0xdeadbeef;
    }
    else
    {
        char **argv;
        char cmdline[MAX_PATH];
        PROCESS_INFORMATION pi;
        STARTUPINFOA si = { sizeof(si) };
        DWORD ret;

        winetest_get_mainargs(&argv);
        sprintf(cmdline, "\"%s\" virtual sharedmemro %lx", argv[0], child_access);
        ret = CreateProcessA(argv[0], cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess(%s) error %ld\n", cmdline, GetLastError());
        wait_child_process(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        if(child_access & FILE_MAP_WRITE)
            ok(*p == 0xdeadbeef, "*p = %lx, expected 0xdeadbeef\n", *p);
        else
            ok(!*p, "*p = %lx, expected 0\n", *p);
    }

    UnmapViewOfFile(p);
    CloseHandle(mapping);
}

#ifndef __REACTOS__
static void test_PrefetchVirtualMemory(void)
{
    WIN32_MEMORY_RANGE_ENTRY entries[2];
    char stackmem[] = "Test stack mem";
    static char testmem[] = "Test memory range data";
    unsigned int page_size = si.dwPageSize;
    BOOL ret;

    if (!pPrefetchVirtualMemory)
    {
        skip("no PrefetchVirtualMemory in kernelbase\n");
        return;
    }

    ok( !pPrefetchVirtualMemory( GetCurrentProcess(), 0, NULL, 0 ),
        "PrefetchVirtualMemory unexpected success on 0 entries\n" );

    entries[0].VirtualAddress = ULongToPtr(PtrToUlong(testmem) & -(ULONG_PTR)page_size);
    entries[0].NumberOfBytes = page_size;
    ret = pPrefetchVirtualMemory( GetCurrentProcess(), 1, entries, 0 );
    ok( ret || broken( is_wow64 && GetLastError() == ERROR_INVALID_PARAMETER ) /* win10 1507 */,
        "PrefetchVirtualMemory unexpected status on 1 page-aligned entry: %ld\n", GetLastError() );

    entries[0].VirtualAddress = testmem;
    entries[0].NumberOfBytes = sizeof(testmem);
    ret = pPrefetchVirtualMemory( GetCurrentProcess(), 1, entries, 0 );
    ok( ret || broken( is_wow64 && GetLastError() == ERROR_INVALID_PARAMETER ) /* win10 1507 */,
        "PrefetchVirtualMemory unexpected status on 1 entry: %ld\n", GetLastError() );

    entries[0].VirtualAddress = NULL;
    entries[0].NumberOfBytes = page_size;
    ret = pPrefetchVirtualMemory( GetCurrentProcess(), 1, entries, 0 );
    ok( ret ||broken( is_wow64 && GetLastError() == ERROR_INVALID_PARAMETER ) /* win10 1507 */,
        "PrefetchVirtualMemory unexpected status on 1 unmapped entry: %ld\n", GetLastError() );

    entries[0].VirtualAddress = ULongToPtr(PtrToUlong(testmem) & -(ULONG_PTR)page_size);
    entries[0].NumberOfBytes = page_size;
    entries[1].VirtualAddress = ULongToPtr(PtrToUlong(stackmem) & -(ULONG_PTR)page_size);
    entries[1].NumberOfBytes = page_size;
    ret = pPrefetchVirtualMemory( GetCurrentProcess(), 2, entries, 0 );
    ok( ret ||broken( is_wow64 && GetLastError() == ERROR_INVALID_PARAMETER ) /* win10 1507 */,
        "PrefetchVirtualMemory unexpected status on 2 page-aligned entries: %ld\n", GetLastError() );
}
#endif

static void test_ReadProcessMemory(void)
{
    BYTE *buf;
    DWORD old_prot;
    SIZE_T copied;
    HANDLE hproc;
    void *ptr;
    BOOL ret;

    buf = malloc(2 * si.dwPageSize);
    ok(buf != NULL, "OOM\n");
    ret = DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
            &hproc, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "DuplicateHandle failed %lu\n", GetLastError());
    ptr = VirtualAlloc(NULL, 2 * si.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    ok(ptr != NULL, "Virtual failed %lu\n", GetLastError());
    ret = VirtualProtect(((BYTE *)ptr) + si.dwPageSize, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(ret, "VirtualProtect failed %lu\n", GetLastError());

    copied = 1;
    ret = ReadProcessMemory(GetCurrentProcess(), ptr, buf, 2 * si.dwPageSize, &copied);
    ok(!ret, "ReadProcessMemory succeeded\n");
    ok(!copied, "copied = %Id\n", copied);
    ok(GetLastError() == ERROR_PARTIAL_COPY, "GetLastError() = %lu\n", GetLastError());

    ret = ReadProcessMemory(GetCurrentProcess(), ptr, buf, si.dwPageSize, &copied);
    ok(ret, "ReadProcessMemory failed %lu\n", GetLastError());
    ok(copied == si.dwPageSize, "copied = %Id\n", copied);

    ret = ReadProcessMemory(hproc, ptr, buf, 2 * si.dwPageSize, &copied);
    todo_wine ok(!ret, "ReadProcessMemory succeeded\n");

    ret = ReadProcessMemory(hproc, ptr, buf, si.dwPageSize, &copied);
    ok(ret, "ReadProcessMemory failed %lu\n", GetLastError());
    ok(copied == si.dwPageSize, "copied = %Id\n", copied);

    ret = VirtualFree(ptr, 0, MEM_RELEASE);
    ok(ret, "VirtualFree failed %lu\n", GetLastError());
    CloseHandle(hproc);
    free(buf);
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
            test_shared_memory(TRUE);
            return;
        }
        if (!strcmp(argv[2], "sharedmemro"))
        {
            test_shared_memory_ro(TRUE, strtol(argv[3], NULL, 16));
            return;
        }
        while (1)
        {
            void *mem;
            BOOL ret;
            mem = VirtualAlloc(NULL, 1<<20, MEM_COMMIT|MEM_RESERVE,
                               PAGE_EXECUTE_READWRITE);
            ok(mem != NULL, "VirtualAlloc failed %lu\n", GetLastError());
            if (mem == NULL) break;
            ret = VirtualFree(mem, 0, MEM_RELEASE);
            ok(ret, "VirtualFree failed %lu\n", GetLastError());
            if (!ret) break;
        }
        return;
    }

    hkernel32 = GetModuleHandleA("kernel32.dll");
    hkernelbase = GetModuleHandleA("kernelbase.dll");
    hntdll    = GetModuleHandleA("ntdll.dll");

    pGetWriteWatch = (void *) GetProcAddress(hkernel32, "GetWriteWatch");
    pResetWriteWatch = (void *) GetProcAddress(hkernel32, "ResetWriteWatch");
    pGetProcessDEPPolicy = (void *)GetProcAddress( hkernel32, "GetProcessDEPPolicy" );
    pIsWow64Process = (void *)GetProcAddress( hkernel32, "IsWow64Process" );
    pNtAreMappedFilesTheSame = (void *)GetProcAddress( hntdll, "NtAreMappedFilesTheSame" );
    pNtCreateSection = (void *)GetProcAddress( hntdll, "NtCreateSection" );
    pNtMapViewOfSection = (void *)GetProcAddress( hntdll, "NtMapViewOfSection" );
    pNtUnmapViewOfSection = (void *)GetProcAddress( hntdll, "NtUnmapViewOfSection" );
    pNtQuerySection = (void *)GetProcAddress( hntdll, "NtQuerySection" );
    pRtlAddVectoredExceptionHandler = (void *)GetProcAddress( hntdll, "RtlAddVectoredExceptionHandler" );
    pRtlRemoveVectoredExceptionHandler = (void *)GetProcAddress( hntdll, "RtlRemoveVectoredExceptionHandler" );
    pNtProtectVirtualMemory = (void *)GetProcAddress( hntdll, "NtProtectVirtualMemory" );
    pNtReadVirtualMemory = (void *)GetProcAddress( hntdll, "NtReadVirtualMemory" );
    pNtWriteVirtualMemory = (void *)GetProcAddress( hntdll, "NtWriteVirtualMemory" );
#ifndef __REACTOS__
    pPrefetchVirtualMemory = (void *)GetProcAddress( hkernelbase, "PrefetchVirtualMemory" );
#endif

    GetSystemInfo(&si);
    trace("system page size %#lx\n", si.dwPageSize);

    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    test_shared_memory(FALSE);
    test_shared_memory_ro(FALSE, FILE_MAP_READ|FILE_MAP_WRITE);
    test_shared_memory_ro(FALSE, FILE_MAP_COPY);
    test_shared_memory_ro(FALSE, FILE_MAP_COPY|FILE_MAP_WRITE);
    test_mappings();
    test_CreateFileMapping_protection();
    test_VirtualAlloc_protection();
    test_VirtualProtect();
    test_VirtualAllocEx();
    test_VirtualAlloc();
    test_MapViewOfFile();
    test_NtAreMappedFilesTheSame();
    test_CreateFileMapping();
    test_IsBadReadPtr();
    test_IsBadWritePtr();
    test_IsBadCodePtr();
    test_write_watch();
#ifndef __REACTOS__
    test_PrefetchVirtualMemory();
#endif
    test_ReadProcessMemory();
#if defined(__i386__) || defined(__x86_64__)
    test_stack_commit();
#endif
#ifdef __i386__
#ifndef __REACTOS__
    test_guard_page();
    /* The following tests should be executed as a last step, and in exactly this
     * order, since ATL thunk emulation cannot be enabled anymore on Windows. */
    test_atl_thunk_emulation( MEM_EXECUTE_OPTION_ENABLE );
    test_atl_thunk_emulation( MEM_EXECUTE_OPTION_DISABLE );
    test_atl_thunk_emulation( MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION );
#endif
#endif
}
