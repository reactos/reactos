/*
 * Unit test suite for PSAPI
 *
 * Copyright (C) 2005 Felix Nawothnig
 * Copyright (C) 2012 Dmitry Timoshkov
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnt.h"
#include "wine/winternl.h"
#include "winnls.h"
#include "winuser.h"
#define PSAPI_VERSION 1
#include "psapi.h"
#include "wine/test.h"

static NTSTATUS (WINAPI *pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtQueryVirtualMemory)(HANDLE, LPCVOID, ULONG, PVOID, SIZE_T, SIZE_T *);
static BOOL  (WINAPI *pIsWow64Process)(HANDLE, BOOL *);
static BOOL  (WINAPI *pWow64DisableWow64FsRedirection)(void **);
static BOOL  (WINAPI *pWow64RevertWow64FsRedirection)(void *);

static BOOL wow64;

static BOOL init_func_ptrs(void)
{
    pNtQuerySystemInformation = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    pNtQueryVirtualMemory = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryVirtualMemory");
    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    pWow64DisableWow64FsRedirection = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64DisableWow64FsRedirection");
    pWow64RevertWow64FsRedirection = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64RevertWow64FsRedirection");
    return TRUE;
}

static HANDLE hpSR, hpQI, hpVR, hpQV;
static const HANDLE hBad = (HANDLE)0xdeadbeef;

static void test_EnumProcesses(void)
{
    DWORD pid, ret, cbUsed = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    ret = EnumProcesses(NULL, 0, &cbUsed);
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(cbUsed == 0, "cbUsed=%d\n", cbUsed);

    SetLastError(0xdeadbeef);
    ret = EnumProcesses(&pid, 4, &cbUsed);
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(cbUsed == 4, "cbUsed=%d\n", cbUsed);
}

static void test_EnumProcessModules(void)
{
    char buffer[200] = "C:\\windows\\system32\\notepad.exe";
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOA si = {0};
    void *cookie;
    HMODULE hMod;
    DWORD ret, cbNeeded = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    EnumProcessModules(NULL, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    EnumProcessModules(hpQI, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQI, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQV, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %d\n", GetLastError());
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));

    SetLastError(0xdeadbeef);
    ret = EnumProcessModules(hpQV, NULL, 0, &cbNeeded);
    ok(ret == 1, "failed with %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumProcessModules(hpQV, NULL, sizeof(HMODULE), &cbNeeded);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hMod = (void *)0xdeadbeef;
    ret = EnumProcessModules(hpQV, &hMod, sizeof(HMODULE), &cbNeeded);
    ok(ret == 1, "got %d, failed with %d\n", ret, GetLastError());
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));
    ok(cbNeeded % sizeof(hMod) == 0, "not a multiple of sizeof(HMODULE) cbNeeded=%d\n", cbNeeded);

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed: %u\n", GetLastError());

    ret = WaitForInputIdle(pi.hProcess, 1000);
    ok(!ret, "wait timed out\n");

    SetLastError(0xdeadbeef);
    hMod = NULL;
    ret = EnumProcessModules(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
    ok(ret == 1, "got %d, error %u\n", ret, GetLastError());
    ok(!!hMod, "expected non-NULL module\n");
    ok(cbNeeded % sizeof(hMod) == 0, "got %u\n", cbNeeded);

    TerminateProcess(pi.hProcess, 0);

    if (sizeof(void *) == 8)
    {
        MODULEINFO info;
        char name[40];

        strcpy(buffer, "C:\\windows\\syswow64\\notepad.exe");
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcess failed: %u\n", GetLastError());

        ret = WaitForInputIdle(pi.hProcess, 1000);
        ok(!ret, "wait timed out\n");

        SetLastError(0xdeadbeef);
        hMod = NULL;
        ret = EnumProcessModules(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
        ok(ret == 1, "got %d, error %u\n", ret, GetLastError());
        ok(!!hMod, "expected non-NULL module\n");
        ok(cbNeeded % sizeof(hMod) == 0, "got %u\n", cbNeeded);

        ret = GetModuleBaseNameA(pi.hProcess, hMod, name, sizeof(name));
        ok(ret, "got error %u\n", GetLastError());
        ok(!strcmp(name, "notepad.exe"), "got %s\n", name);

        ret = GetModuleFileNameExA(pi.hProcess, hMod, name, sizeof(name));
        ok(ret, "got error %u\n", GetLastError());
todo_wine
        ok(!strcmp(name, buffer), "got %s\n", name);

        ret = GetModuleInformation(pi.hProcess, hMod, &info, sizeof(info));
        ok(ret, "got error %u\n", GetLastError());
        ok(info.lpBaseOfDll == hMod, "expected %p, got %p\n", hMod, info.lpBaseOfDll);
        ok(info.SizeOfImage, "image size was 0\n");
        ok(info.EntryPoint >= info.lpBaseOfDll, "got entry point %p\n", info.EntryPoint);

        TerminateProcess(pi.hProcess, 0);
    }
    else if (wow64)
    {
        pWow64DisableWow64FsRedirection(&cookie);
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        pWow64RevertWow64FsRedirection(cookie);
        ok(ret, "CreateProcess failed: %u\n", GetLastError());

        ret = WaitForInputIdle(pi.hProcess, 1000);
        ok(!ret, "wait timed out\n");

        SetLastError(0xdeadbeef);
        ret = EnumProcessModules(pi.hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
        ok(!ret, "got %d\n", ret);
todo_wine
        ok(GetLastError() == ERROR_PARTIAL_COPY, "got error %u\n", GetLastError());

        TerminateProcess(pi.hProcess, 0);
    }
}

static void test_GetModuleInformation(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    DWORD *tmp, counter = 0;
    MODULEINFO info;
    DWORD ret;

    SetLastError(0xdeadbeef);
    GetModuleInformation(NULL, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQI, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQV, hBad, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleInformation(hpQV, hMod, &info, sizeof(info)-1);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    ret = GetModuleInformation(hpQV, hMod, &info, sizeof(info));
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(info.lpBaseOfDll == hMod, "lpBaseOfDll=%p hMod=%p\n", info.lpBaseOfDll, hMod);

    hMod = LoadLibraryA("shell32.dll");
    ok(hMod != NULL, "Failed to load shell32.dll, error: %u\n", GetLastError());

    ret = GetModuleInformation(hpQV, hMod, &info, sizeof(info));
    ok(ret == 1, "failed with %d\n", GetLastError());
    info.SizeOfImage /= sizeof(DWORD);
    for (tmp = (DWORD *)hMod; info.SizeOfImage; info.SizeOfImage--)
        counter ^= *tmp++;
    trace("xor of shell32: %08x\n", counter);

    FreeLibrary(hMod);
}

static BOOL check_with_margin(SIZE_T perf, SIZE_T sysperf, int margin)
{
    return (perf >= max(sysperf, margin) - margin && perf <= sysperf + margin);
}

static void test_GetPerformanceInfo(void)
{
    PERFORMANCE_INFORMATION info;
    NTSTATUS status;
    DWORD size;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetPerformanceInfo(&info, sizeof(info)-1);
    ok(!ret, "GetPerformanceInfo unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BAD_LENGTH, "expected error=ERROR_BAD_LENGTH but got %d\n", GetLastError());

    if (!pNtQuerySystemInformation)
        win_skip("NtQuerySystemInformation not found, skipping tests\n");
    else
    {
        char performance_buffer[sizeof(SYSTEM_PERFORMANCE_INFORMATION) + 16]; /* larger on w2k8/win7 */
        SYSTEM_PERFORMANCE_INFORMATION *sys_performance_info = (SYSTEM_PERFORMANCE_INFORMATION *)performance_buffer;
        SYSTEM_PROCESS_INFORMATION *sys_process_info = NULL, *spi;
        SYSTEM_BASIC_INFORMATION sys_basic_info;
        DWORD process_count, handle_count, thread_count;

        /* compare with values from SYSTEM_PERFORMANCE_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemPerformanceInformation, sys_performance_info, sizeof(performance_buffer), &size);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
        ok(size >= sizeof(SYSTEM_PERFORMANCE_INFORMATION), "incorrect length %d\n", size);

        SetLastError(0xdeadbeef);
        ret = GetPerformanceInfo(&info, sizeof(info));
        ok(ret, "GetPerformanceInfo failed with %d\n", GetLastError());
        ok(info.cb == sizeof(PERFORMANCE_INFORMATION), "got %d\n", info.cb);

        ok(check_with_margin(info.CommitTotal,          sys_performance_info->TotalCommittedPages,  288),
           "expected approximately %ld but got %d\n", info.CommitTotal, sys_performance_info->TotalCommittedPages);

        ok(check_with_margin(info.CommitLimit,          sys_performance_info->TotalCommitLimit,     32),
           "expected approximately %ld but got %d\n", info.CommitLimit, sys_performance_info->TotalCommitLimit);

        ok(check_with_margin(info.CommitPeak,           sys_performance_info->PeakCommitment,       32),
           "expected approximately %ld but got %d\n", info.CommitPeak, sys_performance_info->PeakCommitment);

        ok(check_with_margin(info.PhysicalAvailable,    sys_performance_info->AvailablePages,       512),
           "expected approximately %ld but got %d\n", info.PhysicalAvailable, sys_performance_info->AvailablePages);

        /* TODO: info.SystemCache not checked yet - to which field(s) does this value correspond to? */

        ok(check_with_margin(info.KernelTotal, sys_performance_info->PagedPoolUsage + sys_performance_info->NonPagedPoolUsage, 256),
           "expected approximately %ld but got %d\n", info.KernelTotal,
           sys_performance_info->PagedPoolUsage + sys_performance_info->NonPagedPoolUsage);

        ok(check_with_margin(info.KernelPaged,          sys_performance_info->PagedPoolUsage,       256),
           "expected approximately %ld but got %d\n", info.KernelPaged, sys_performance_info->PagedPoolUsage);

        ok(check_with_margin(info.KernelNonpaged,       sys_performance_info->NonPagedPoolUsage,    16),
           "expected approximately %ld but got %d\n", info.KernelNonpaged, sys_performance_info->NonPagedPoolUsage);

        /* compare with values from SYSTEM_BASIC_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemBasicInformation, &sys_basic_info, sizeof(sys_basic_info), &size);
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);
        ok(size >= sizeof(SYSTEM_BASIC_INFORMATION), "incorrect length %d\n", size);

        ok(info.PhysicalTotal == sys_basic_info.MmNumberOfPhysicalPages,
           "expected info.PhysicalTotal=%u but got %u\n",
           sys_basic_info.MmNumberOfPhysicalPages, (ULONG)info.PhysicalTotal);

        ok(info.PageSize == sys_basic_info.PageSize,
           "expected info.PageSize=%u but got %u\n",
           sys_basic_info.PageSize, (ULONG)info.PageSize);

        /* compare with values from SYSTEM_PROCESS_INFORMATION */
        size = 0;
        status = pNtQuerySystemInformation(SystemProcessInformation, NULL, 0, &size);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "expected STATUS_INFO_LENGTH_MISMATCH, got %08x\n", status);
        ok(size > 0, "incorrect length %d\n", size);
        while (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            sys_process_info = HeapAlloc(GetProcessHeap(), 0, size);
            ok(sys_process_info != NULL, "failed to allocate memory\n");
            status = pNtQuerySystemInformation(SystemProcessInformation, sys_process_info, size, &size);
            if (status == STATUS_SUCCESS) break;
            HeapFree(GetProcessHeap(), 0, sys_process_info);
        }
        ok(status == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08x\n", status);

        process_count = handle_count = thread_count = 0;
        for (spi = sys_process_info;; spi = (SYSTEM_PROCESS_INFORMATION *)(((char *)spi) + spi->NextEntryOffset))
        {
            process_count++;
            handle_count += spi->HandleCount;
            thread_count += spi->dwThreadCount;
            if (spi->NextEntryOffset == 0) break;
        }
        HeapFree(GetProcessHeap(), 0, sys_process_info);

        ok(check_with_margin(info.HandleCount,  handle_count,  256),
           "expected approximately %d but got %d\n", info.HandleCount, handle_count);

        ok(check_with_margin(info.ProcessCount, process_count, 4),
           "expected approximately %d but got %d\n", info.ProcessCount, process_count);

        ok(check_with_margin(info.ThreadCount,  thread_count,  4),
           "expected approximately %d but got %d\n", info.ThreadCount, thread_count);
    }
}


static void test_GetProcessMemoryInfo(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    DWORD ret;

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(NULL, &pmc, sizeof(pmc));
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpSR, &pmc, sizeof(pmc));
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc)-1);
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc));
    ok(ret == 1, "failed with %d\n", GetLastError());
}

static BOOL nt_get_mapped_file_name(HANDLE process, LPVOID addr, LPWSTR name, DWORD len)
{
    MEMORY_SECTION_NAME *section_name;
    WCHAR *buf;
    SIZE_T buf_len, ret_len;
    NTSTATUS status;

    if (!pNtQueryVirtualMemory) return FALSE;

    buf_len = len * sizeof(WCHAR) + sizeof(MEMORY_SECTION_NAME);
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buf_len);

    ret_len = 0xdeadbeef;
    status = pNtQueryVirtualMemory(process, addr, MemorySectionName, buf, buf_len, &ret_len);
    ok(!status, "NtQueryVirtualMemory error %x\n", status);

    section_name = (MEMORY_SECTION_NAME *)buf;
    ok(ret_len == section_name->SectionFileName.MaximumLength + sizeof(*section_name), "got %lu, %u\n",
       ret_len, section_name->SectionFileName.MaximumLength);
    ok((char *)section_name->SectionFileName.Buffer == (char *)section_name + sizeof(*section_name), "got %p, %p\n",
       section_name, section_name->SectionFileName.Buffer);
    ok(section_name->SectionFileName.MaximumLength == section_name->SectionFileName.Length + sizeof(WCHAR), "got %u, %u\n",
       section_name->SectionFileName.MaximumLength, section_name->SectionFileName.Length);
    ok(section_name->SectionFileName.Length == lstrlenW(section_name->SectionFileName.Buffer) * sizeof(WCHAR), "got %u, %u\n",
       section_name->SectionFileName.Length, lstrlenW(section_name->SectionFileName.Buffer));

    memcpy(name, section_name->SectionFileName.Buffer, section_name->SectionFileName.MaximumLength);
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

static void test_GetMappedFileName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szMapPath[MAX_PATH], szModPath[MAX_PATH], *szMapBaseName;
    DWORD ret;
    char *base;
    char temp_path[MAX_PATH], file_name[MAX_PATH], map_name[MAX_PATH], device_name[MAX_PATH], drive[3];
    WCHAR map_nameW[MAX_PATH], nt_map_name[MAX_PATH];
    HANDLE hfile, hmap;
    HANDLE current_process;

    DuplicateHandle( GetCurrentProcess(), GetCurrentProcess(),
                     GetCurrentProcess(), &current_process, 0, 0, DUPLICATE_SAME_ACCESS );

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(NULL, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(hpSR, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = GetMappedFileNameA(hpQI, hMod, szMapPath, sizeof(szMapPath));
    ok( ret || broken(GetLastError() == ERROR_UNEXP_NET_ERR), /* win2k */
        "GetMappedFileNameA failed with error %u\n", GetLastError() );
    if (ret)
    {
        ok(ret == strlen(szMapPath), "szMapPath=\"%s\" ret=%d\n", szMapPath, ret);
        ok(szMapPath[0] == '\\', "szMapPath=\"%s\"\n", szMapPath);
        szMapBaseName = strrchr(szMapPath, '\\'); /* That's close enough for us */
        ok(szMapBaseName && *szMapBaseName, "szMapPath=\"%s\"\n", szMapPath);
        if (szMapBaseName)
        {
            GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
            ok(!strcmp(strrchr(szModPath, '\\'), szMapBaseName),
               "szModPath=\"%s\" szMapBaseName=\"%s\"\n", szModPath, szMapBaseName);
        }
    }

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "map", 0, file_name);

    drive[0] = file_name[0];
    drive[1] = ':';
    drive[2] = 0;
    SetLastError(0xdeadbeef);
    ret = QueryDosDeviceA(drive, device_name, sizeof(device_name));
    ok(ret, "QueryDosDeviceA error %d\n", GetLastError());
    trace("%s -> %s\n", drive, device_name);

    SetLastError(0xdeadbeef);
    hfile = CreateFileA(file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFileA(%s) error %d\n", file_name, GetLastError());
    SetFilePointer(hfile, 0x4000, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingA(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmap != 0, "CreateFileMappingA error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
    ok(base != NULL, "MapViewOfFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, 0);
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "wrong error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, 0, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, 1);
    ok(ret == 1, "GetMappedFileName error %d\n", GetLastError());
    ok(!map_name[0] || broken(map_name[0] == device_name[0]) /* before win2k */, "expected 0, got %c\n", map_name[0]);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
    ok(ret, "GetMappedFileName error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameW(GetCurrentProcess(), base, map_nameW, ARRAY_SIZE(map_nameW));
todo_wine {
    ok(ret, "GetMappedFileNameW error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
}
    if (nt_get_mapped_file_name(GetCurrentProcess(), base, nt_map_name, ARRAY_SIZE(nt_map_name)))
    {
        ok(memcmp(map_nameW, nt_map_name, lstrlenW(map_nameW)) == 0, "map name does not start with a device name: %s\n", map_name);
        WideCharToMultiByte(CP_ACP, 0, map_nameW, -1, map_name, MAX_PATH, NULL, NULL);
        ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
    }

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameW(current_process, base, map_nameW, sizeof(map_nameW)/sizeof(map_nameW[0]));
    ok(ret, "GetMappedFileNameW error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");

    if (nt_get_mapped_file_name(current_process, base, nt_map_name, sizeof(nt_map_name)/sizeof(nt_map_name[0])))
    {
        ok(memcmp(map_nameW, nt_map_name, lstrlenW(map_nameW)) == 0, "map name does not start with a device name: %s\n", map_name);
        WideCharToMultiByte(CP_ACP, 0, map_nameW, -1, map_name, MAX_PATH, NULL, NULL);
        ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
    }

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base + 0x2000, map_name, sizeof(map_name));
    ok(ret, "GetMappedFileName error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base + 0x4000, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), NULL, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(0, base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    UnmapViewOfFile(base);
    CloseHandle(hmap);
    CloseHandle(hfile);
    DeleteFileA(file_name);

    SetLastError(0xdeadbeef);
    hmap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY | SEC_COMMIT, 0, 4096, NULL);
    ok(hmap != 0, "CreateFileMappingA error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    base = MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
    ok(base != NULL, "MapViewOfFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
    ok(GetLastError() == ERROR_FILE_INVALID, "expected ERROR_FILE_INVALID, got %d\n", GetLastError());

    CloseHandle(current_process);
    UnmapViewOfFile(base);
    CloseHandle(hmap);
}

static void test_GetProcessImageFileName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szImgPath[MAX_PATH], szMapPath[MAX_PATH];
    WCHAR szImgPathW[MAX_PATH];
    DWORD ret, ret1;

    /* This function is available on WinXP+ only */
    SetLastError(0xdeadbeef);
    if(!GetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath)))
    {
        if(GetLastError() == ERROR_INVALID_FUNCTION) {
	    win_skip("GetProcessImageFileName not implemented\n");
            return;
        }

        if(GetLastError() == 0xdeadbeef)
	    todo_wine ok(0, "failed without error code\n");
	else
	    todo_wine ok(0, "failed with %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(NULL, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(hpSR, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameA(hpQI, szImgPath, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    ret = GetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath));
    ret1 = GetMappedFileNameA(hpQV, hMod, szMapPath, sizeof(szMapPath));
    if(ret && ret1)
    {
        /* Windows returns 2*strlen-1 */
        ok(ret >= strlen(szImgPath), "szImgPath=\"%s\" ret=%d\n", szImgPath, ret);
        ok(!strcmp(szImgPath, szMapPath), "szImgPath=\"%s\" szMapPath=\"%s\"\n", szImgPath, szMapPath);
    }

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(NULL, szImgPathW, ARRAY_SIZE(szImgPathW));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    /* no information about correct buffer size returned: */
    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, szImgPathW, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, NULL, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    /* correct call */
    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = GetProcessImageFileNameW(hpQI, szImgPathW, ARRAY_SIZE(szImgPathW));
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    ok(lstrlenW(szImgPathW) == ret, "Expected length to be %d, got %d\n", ret, lstrlenW(szImgPathW));

    /* boundary values of 'size' */
    SetLastError(0xdeadbeef);
    GetProcessImageFileNameW(hpQI, szImgPathW, ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = GetProcessImageFileNameW(hpQI, szImgPathW, ret + 1);
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    ok(lstrlenW(szImgPathW) == ret, "Expected length to be %d, got %d\n", ret, lstrlenW(szImgPathW));
}

static void test_GetModuleFileNameEx(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szModExPath[MAX_PATH+1], szModPath[MAX_PATH+1];
    WCHAR buffer[MAX_PATH];
    DWORD ret;

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(NULL, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(hpQI, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetModuleFileNameExA(hpQV, hBad, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, sizeof(szModExPath));
    if(!ret)
            return;
    ok(ret == strlen(szModExPath), "szModExPath=\"%s\" ret=%d\n", szModExPath, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strncmp(szModExPath, szModPath, MAX_PATH), 
       "szModExPath=\"%s\" szModPath=\"%s\"\n", szModExPath, szModPath);

    SetLastError(0xdeadbeef);
    memset( szModExPath, 0xcc, sizeof(szModExPath) );
    ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, 4 );
    ok( ret == 4 || ret == strlen(szModExPath), "wrong length %u\n", ret );
    ok( broken(szModExPath[3]) /*w2kpro*/ || strlen(szModExPath) == 3,
        "szModExPath=\"%s\" ret=%d\n", szModExPath, ret );
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

    if (0) /* crashes on Windows 10 */
    {
        SetLastError(0xdeadbeef);
        ret = GetModuleFileNameExA(hpQV, NULL, szModExPath, 0 );
        ok( ret == 0, "wrong length %u\n", ret );
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetModuleFileNameExW(hpQV, NULL, buffer, 4 );
    ok( ret == 4 || ret == lstrlenW(buffer), "wrong length %u\n", ret );
    ok( broken(buffer[3]) /*w2kpro*/ || lstrlenW(buffer) == 3,
        "buffer=%s ret=%d\n", wine_dbgstr_w(buffer), ret );
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

    if (0) /* crashes on Windows 10 */
    {
        SetLastError(0xdeadbeef);
        buffer[0] = 0xcc;
        ret = GetModuleFileNameExW(hpQV, NULL, buffer, 0 );
        ok( ret == 0, "wrong length %u\n", ret );
        ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());
        ok( buffer[0] == 0xcc, "buffer modified %s\n", wine_dbgstr_w(buffer) );
    }
}

static void test_GetModuleBaseName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szModPath[MAX_PATH], szModBaseName[MAX_PATH];
    DWORD ret;

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(NULL, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(hpQI, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    GetModuleBaseNameA(hpQV, hBad, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    ret = GetModuleBaseNameA(hpQV, NULL, szModBaseName, sizeof(szModBaseName));
    if(!ret)
        return;
    ok(ret == strlen(szModBaseName), "szModBaseName=\"%s\" ret=%d\n", szModBaseName, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strcmp(strrchr(szModPath, '\\') + 1, szModBaseName),
       "szModPath=\"%s\" szModBaseName=\"%s\"\n", szModPath, szModBaseName);
}

static void test_ws_functions(void)
{
    PSAPI_WS_WATCH_INFORMATION wswi[4096];
    ULONG_PTR pages[4096];
    HANDLE ws_handle;
    char *addr;
    unsigned int i;
    BOOL ret;

    ws_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA |
        PROCESS_SET_INFORMATION, FALSE, GetCurrentProcessId());
    ok(!!ws_handle, "got error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    EmptyWorkingSet(NULL);
    todo_wine ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    EmptyWorkingSet(hpSR);
    todo_wine ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EmptyWorkingSet(ws_handle);
    ok(ret == 1, "failed with %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = InitializeProcessForWsWatch( NULL );
    todo_wine ok( !ret, "InitializeProcessForWsWatch succeeded\n" );
    if (!ret)
    {
        if (GetLastError() == ERROR_INVALID_FUNCTION)  /* not supported on xp in wow64 mode */
        {
            trace( "InitializeProcessForWsWatch not supported\n" );
            return;
        }
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );
    }
    SetLastError(0xdeadbeef);
    ret = InitializeProcessForWsWatch(ws_handle);
    ok(ret == 1, "failed with %d\n", GetLastError());
    
    addr = VirtualAlloc(NULL, 1, MEM_COMMIT, PAGE_READWRITE);
    if(!addr)
        return;

    *addr = 0; /* make sure it's paged in (needed on wow64) */
    if(!VirtualLock(addr, 1))
    {
        trace("locking failed (error=%d) - skipping test\n", GetLastError());
        goto free_page;
    }

    SetLastError(0xdeadbeef);
    ret = QueryWorkingSet(hpQI, pages, 4096 * sizeof(ULONG_PTR));
    todo_wine ok(ret == 1, "failed with %d\n", GetLastError());
    if(ret == 1)
    {
       for(i = 0; i < pages[0]; i++)
           if((pages[i+1] & ~0xfffL) == (ULONG_PTR)addr)
	   {
	       todo_wine ok(ret == 1, "QueryWorkingSet found our page\n");
	       goto test_gwsc;
	   }
       
       todo_wine ok(0, "QueryWorkingSet didn't find our page\n");
    }

test_gwsc:
    SetLastError(0xdeadbeef);
    ret = GetWsChanges(hpQI, wswi, sizeof(wswi));
    todo_wine ok(ret == 1, "failed with %d\n", GetLastError());
    if(ret == 1)
    {
        for(i = 0; wswi[i].FaultingVa; i++)
	    if(((ULONG_PTR)wswi[i].FaultingVa & ~0xfffL) == (ULONG_PTR)addr)
	    {
	        todo_wine ok(ret == 1, "GetWsChanges found our page\n");
		goto free_page;
	    }

	todo_wine ok(0, "GetWsChanges didn't find our page\n");
    }
    
free_page:
    VirtualFree(addr, 0, MEM_RELEASE);
}

START_TEST(psapi_main)
{
    DWORD pid = GetCurrentProcessId();

    init_func_ptrs();

    if (pIsWow64Process)
        IsWow64Process(GetCurrentProcess(), &wow64);

    hpSR = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, pid);
    ok(!!hpSR, "got error %u\n", GetLastError());
    hpQI = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    ok(!!hpQI, "got error %u\n", GetLastError());
    hpVR = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    ok(!!hpVR, "got error %u\n", GetLastError());
    hpQV = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    ok(!!hpQV, "got error %u\n", GetLastError());

    test_EnumProcesses();
    test_EnumProcessModules();
    test_GetModuleInformation();
    test_GetPerformanceInfo();
    test_GetProcessMemoryInfo();
    test_GetMappedFileName();
    test_GetProcessImageFileName();
    test_GetModuleFileNameEx();
    test_GetModuleBaseName();
    test_ws_functions();

    CloseHandle(hpSR);
    CloseHandle(hpQI);
    CloseHandle(hpVR);
    CloseHandle(hpQV);
}
