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
#include "psapi.h"
#include "wine/test.h"

#define PSAPI_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hpsapi, #func); \
    if(!p ## func) { \
        ok(0, "GetProcAddress(%s) failed\n", #func); \
        FreeLibrary(hpsapi); \
        return FALSE; \
    }

static BOOL  (WINAPI *pEmptyWorkingSet)(HANDLE);
static BOOL  (WINAPI *pEnumProcesses)(DWORD*, DWORD, DWORD*);
static BOOL  (WINAPI *pEnumProcessModules)(HANDLE, HMODULE*, DWORD, LPDWORD);
static DWORD (WINAPI *pGetModuleBaseNameA)(HANDLE, HMODULE, LPSTR, DWORD);
static DWORD (WINAPI *pGetModuleFileNameExA)(HANDLE, HMODULE, LPSTR, DWORD);
static DWORD (WINAPI *pGetModuleFileNameExW)(HANDLE, HMODULE, LPWSTR, DWORD);
static BOOL  (WINAPI *pGetModuleInformation)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
static DWORD (WINAPI *pGetMappedFileNameA)(HANDLE, LPVOID, LPSTR, DWORD);
static DWORD (WINAPI *pGetMappedFileNameW)(HANDLE, LPVOID, LPWSTR, DWORD);
static DWORD (WINAPI *pGetProcessImageFileNameA)(HANDLE, LPSTR, DWORD);
static DWORD (WINAPI *pGetProcessImageFileNameW)(HANDLE, LPWSTR, DWORD);
static BOOL  (WINAPI *pGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
static BOOL  (WINAPI *pGetWsChanges)(HANDLE, PPSAPI_WS_WATCH_INFORMATION, DWORD);
static BOOL  (WINAPI *pInitializeProcessForWsWatch)(HANDLE);
static BOOL  (WINAPI *pQueryWorkingSet)(HANDLE, PVOID, DWORD);
static NTSTATUS (WINAPI *pNtQueryVirtualMemory)(HANDLE, LPCVOID, ULONG, PVOID, SIZE_T, SIZE_T *);
      
static BOOL InitFunctionPtrs(HMODULE hpsapi)
{
    PSAPI_GET_PROC(EmptyWorkingSet);
    PSAPI_GET_PROC(EnumProcessModules);
    PSAPI_GET_PROC(EnumProcesses);
    PSAPI_GET_PROC(GetModuleBaseNameA);
    PSAPI_GET_PROC(GetModuleFileNameExA);
    PSAPI_GET_PROC(GetModuleFileNameExW);
    PSAPI_GET_PROC(GetModuleInformation);
    PSAPI_GET_PROC(GetMappedFileNameA);
    PSAPI_GET_PROC(GetMappedFileNameW);
    PSAPI_GET_PROC(GetProcessMemoryInfo);
    PSAPI_GET_PROC(GetWsChanges);
    PSAPI_GET_PROC(InitializeProcessForWsWatch);
    PSAPI_GET_PROC(QueryWorkingSet);
    /* GetProcessImageFileName is not exported on NT4 */
    pGetProcessImageFileNameA =
      (void *)GetProcAddress(hpsapi, "GetProcessImageFileNameA");
    pGetProcessImageFileNameW =
      (void *)GetProcAddress(hpsapi, "GetProcessImageFileNameW");
    pNtQueryVirtualMemory = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryVirtualMemory");
    return TRUE;
}

static HANDLE hpSR, hpQI, hpVR, hpQV, hpAA;
static const HANDLE hBad = (HANDLE)0xdeadbeef;

static void test_EnumProcesses(void)
{
    DWORD pid, ret, cbUsed = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    ret = pEnumProcesses(NULL, 0, &cbUsed);
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(cbUsed == 0, "cbUsed=%d\n", cbUsed);

    SetLastError(0xdeadbeef);
    ret = pEnumProcesses(&pid, 4, &cbUsed);
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(cbUsed == 4, "cbUsed=%d\n", cbUsed);
}

static void test_EnumProcessModules(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    DWORD ret, cbNeeded = 0xdeadbeef;

    SetLastError(0xdeadbeef);
    pEnumProcessModules(NULL, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pEnumProcessModules(hpQI, NULL, 0, &cbNeeded);
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumProcessModules(hpQI, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumProcessModules(hpQV, &hMod, sizeof(HMODULE), NULL);
    ok(!ret, "succeeded\n");
    ok(GetLastError() == ERROR_NOACCESS, "expected error=ERROR_NOACCESS but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumProcessModules(hpQV, NULL, 0, &cbNeeded);
    ok(ret == 1, "failed with %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumProcessModules(hpQV, &hMod, sizeof(HMODULE), &cbNeeded);
    if(ret != 1)
        return;
    ok(hMod == GetModuleHandleA(NULL),
       "hMod=%p GetModuleHandleA(NULL)=%p\n", hMod, GetModuleHandleA(NULL));
    ok(cbNeeded % sizeof(hMod) == 0, "not a multiple of sizeof(HMODULE) cbNeeded=%d\n", cbNeeded);
    /* Windows sometimes has a bunch of extra dlls, presumably brought in by
     * aclayers.dll.
     */
    if (cbNeeded < 4 * sizeof(HMODULE) || cbNeeded > 30 * sizeof(HMODULE))
    {
        HMODULE hmods[100];
        int i;
        ok(0, "cbNeeded=%d\n", cbNeeded);

        pEnumProcessModules(hpQV, hmods, sizeof(hmods), &cbNeeded);
        for (i = 0 ; i < cbNeeded/sizeof(*hmods); i++)
        {
            char path[1024];
            GetModuleFileNameA(hmods[i], path, sizeof(path));
            trace("i=%d hmod=%p path=[%s]\n", i, hmods[i], path);
        }
    }
}

static void test_GetModuleInformation(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    MODULEINFO info;
    DWORD ret;

    SetLastError(0xdeadbeef);
    pGetModuleInformation(NULL, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetModuleInformation(hpQI, hMod, &info, sizeof(info));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetModuleInformation(hpQV, hBad, &info, sizeof(info));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetModuleInformation(hpQV, hMod, &info, sizeof(info)-1);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetModuleInformation(hpQV, hMod, &info, sizeof(info));
    ok(ret == 1, "failed with %d\n", GetLastError());
    ok(info.lpBaseOfDll == hMod, "lpBaseOfDll=%p hMod=%p\n", info.lpBaseOfDll, hMod);
}

static void test_GetProcessMemoryInfo(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    DWORD ret;

    SetLastError(0xdeadbeef);
    ret = pGetProcessMemoryInfo(NULL, &pmc, sizeof(pmc));
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetProcessMemoryInfo(hpSR, &pmc, sizeof(pmc));
todo_wine
    ok(!ret, "GetProcessMemoryInfo should fail\n");
todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc)-1);
    ok(!ret, "GetProcessMemoryInfo should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc));
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
todo_wine
    ok(!status, "NtQueryVirtualMemory error %x\n", status);
    /* FIXME: remove once Wine is fixed */
    if (status)
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

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

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(NULL, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(hpSR, hMod, szMapPath, sizeof(szMapPath));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = pGetMappedFileNameA(hpQI, hMod, szMapPath, sizeof(szMapPath));
todo_wine
    ok( ret || broken(GetLastError() == ERROR_UNEXP_NET_ERR), /* win2k */
        "GetMappedFileNameA failed with error %u\n", GetLastError() );
    if (ret)
    {
        ok(ret == strlen(szMapPath), "szMapPath=\"%s\" ret=%d\n", szMapPath, ret);
        todo_wine
        ok(szMapPath[0] == '\\', "szMapPath=\"%s\"\n", szMapPath);
        szMapBaseName = strrchr(szMapPath, '\\'); /* That's close enough for us */
        todo_wine
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
    ret = pGetMappedFileNameA(GetCurrentProcess(), base, map_name, 0);
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "wrong error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), base, 0, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), base, map_name, 1);
todo_wine
    ok(ret == 1, "GetMappedFileName error %d\n", GetLastError());
    ok(!map_name[0] || broken(map_name[0] == device_name[0]) /* before win2k */, "expected 0, got %c\n", map_name[0]);

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
todo_wine {
    ok(ret, "GetMappedFileName error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
}

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameW(GetCurrentProcess(), base, map_nameW, sizeof(map_nameW)/sizeof(map_nameW[0]));
todo_wine {
    ok(ret, "GetMappedFileNameW error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
}
    if (nt_get_mapped_file_name(GetCurrentProcess(), base, nt_map_name, sizeof(nt_map_name)/sizeof(nt_map_name[0])))
    {
        ok(memcmp(map_nameW, nt_map_name, lstrlenW(map_nameW)) == 0, "map name does not start with a device name: %s\n", map_name);
        WideCharToMultiByte(CP_ACP, 0, map_nameW, -1, map_name, MAX_PATH, NULL, NULL);
        ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
    }

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), base + 0x2000, map_name, sizeof(map_name));
todo_wine {
    ok(ret, "GetMappedFileName error %d\n", GetLastError());
    ok(ret > strlen(device_name), "map_name should be longer than device_name\n");
    ok(memcmp(map_name, device_name, strlen(device_name)) == 0, "map name does not start with a device name: %s\n", map_name);
}

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), base + 0x4000, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(GetCurrentProcess(), NULL, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_UNEXP_NET_ERR, "expected ERROR_UNEXP_NET_ERR, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetMappedFileNameA(0, base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
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
    ret = pGetMappedFileNameA(GetCurrentProcess(), base, map_name, sizeof(map_name));
    ok(!ret, "GetMappedFileName should fail\n");
todo_wine
    ok(GetLastError() == ERROR_FILE_INVALID, "expected ERROR_FILE_INVALID, got %d\n", GetLastError());

    UnmapViewOfFile(base);
    CloseHandle(hmap);
}

static void test_GetProcessImageFileName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szImgPath[MAX_PATH], szMapPath[MAX_PATH];
    WCHAR szImgPathW[MAX_PATH];
    DWORD ret, ret1;

    if(pGetProcessImageFileNameA == NULL)
        return;

    /* This function is available on WinXP+ only */
    SetLastError(0xdeadbeef);
    if(!pGetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath)))
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
    pGetProcessImageFileNameA(NULL, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameA(hpSR, szImgPath, sizeof(szImgPath));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameA(hpQI, szImgPath, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    ret = pGetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath));
    ret1 = pGetMappedFileNameA(hpQV, hMod, szMapPath, sizeof(szMapPath));
    if(ret && ret1)
    {
        /* Windows returns 2*strlen-1 */
        todo_wine ok(ret >= strlen(szImgPath), "szImgPath=\"%s\" ret=%d\n", szImgPath, ret);
        todo_wine ok(!strcmp(szImgPath, szMapPath), "szImgPath=\"%s\" szMapPath=\"%s\"\n", szImgPath, szMapPath);
    }

    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameW(NULL, szImgPathW, sizeof(szImgPathW));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    /* no information about correct buffer size returned: */
    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameW(hpQI, szImgPathW, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameW(hpQI, NULL, 0);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    /* correct call */
    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = pGetProcessImageFileNameW(hpQI, szImgPathW, sizeof(szImgPathW)/sizeof(WCHAR));
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    ok(lstrlenW(szImgPathW) == ret, "Expected length to be %d, got %d\n", ret, lstrlenW(szImgPathW));

    /* boundary values of 'size' */
    SetLastError(0xdeadbeef);
    pGetProcessImageFileNameW(hpQI, szImgPathW, ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected error=ERROR_INSUFFICIENT_BUFFER but got %d\n", GetLastError());

    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = pGetProcessImageFileNameW(hpQI, szImgPathW, ret + 1);
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
    ret = pGetModuleFileNameExA(NULL, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetModuleFileNameExA(hpQI, hMod, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetModuleFileNameExA(hpQV, hBad, szModExPath, sizeof(szModExPath));
    ok( !ret, "GetModuleFileNameExA succeeded\n" );
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    ret = pGetModuleFileNameExA(hpQV, NULL, szModExPath, sizeof(szModExPath));
    if(!ret)
            return;
    ok(ret == strlen(szModExPath), "szModExPath=\"%s\" ret=%d\n", szModExPath, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strncmp(szModExPath, szModPath, MAX_PATH), 
       "szModExPath=\"%s\" szModPath=\"%s\"\n", szModExPath, szModPath);

    SetLastError(0xdeadbeef);
    memset( szModExPath, 0xcc, sizeof(szModExPath) );
    ret = pGetModuleFileNameExA(hpQV, NULL, szModExPath, 4 );
    ok( ret == 4, "wrong length %u\n", ret );
    ok( broken(szModExPath[3]) /*w2kpro*/ || strlen(szModExPath) == 3,
        "szModExPath=\"%s\" ret=%d\n", szModExPath, ret );
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetModuleFileNameExA(hpQV, NULL, szModExPath, 0 );
    ok( ret == 0, "wrong length %u\n", ret );
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    memset( buffer, 0xcc, sizeof(buffer) );
    ret = pGetModuleFileNameExW(hpQV, NULL, buffer, 4 );
    ok( ret == 4, "wrong length %u\n", ret );
    ok( broken(buffer[3]) /*w2kpro*/ || lstrlenW(buffer) == 3,
        "buffer=%s ret=%d\n", wine_dbgstr_w(buffer), ret );
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    buffer[0] = 0xcc;
    ret = pGetModuleFileNameExW(hpQV, NULL, buffer, 0 );
    ok( ret == 0, "wrong length %u\n", ret );
    ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());
    ok( buffer[0] == 0xcc, "buffer modified %s\n", wine_dbgstr_w(buffer) );
}

static void test_GetModuleBaseName(void)
{
    HMODULE hMod = GetModuleHandleA(NULL);
    char szModPath[MAX_PATH], szModBaseName[MAX_PATH];
    DWORD ret;

    SetLastError(0xdeadbeef);
    pGetModuleBaseNameA(NULL, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetModuleBaseNameA(hpQI, hMod, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pGetModuleBaseNameA(hpQV, hBad, szModBaseName, sizeof(szModBaseName));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    ret = pGetModuleBaseNameA(hpQV, NULL, szModBaseName, sizeof(szModBaseName));
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
    char *addr;
    unsigned int i;
    BOOL ret;

    SetLastError(0xdeadbeef);
    pEmptyWorkingSet(NULL);
    todo_wine ok(GetLastError() == ERROR_INVALID_HANDLE, "expected error=ERROR_INVALID_HANDLE but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    pEmptyWorkingSet(hpSR);
    todo_wine ok(GetLastError() == ERROR_ACCESS_DENIED, "expected error=ERROR_ACCESS_DENIED but got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEmptyWorkingSet(hpAA);
    ok(ret == 1, "failed with %d\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = pInitializeProcessForWsWatch( NULL );
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
    ret = pInitializeProcessForWsWatch(hpAA);
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
    ret = pQueryWorkingSet(hpQI, pages, 4096 * sizeof(ULONG_PTR));
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
    ret = pGetWsChanges(hpQI, wswi, sizeof(wswi));
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
    HMODULE hpsapi = LoadLibraryA("psapi.dll");

    if(!hpsapi)
    {
        win_skip("Could not load psapi.dll\n");
        return;
    }

    if(InitFunctionPtrs(hpsapi))
    {
        DWORD pid = GetCurrentProcessId();

    hpSR = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, pid);
    hpQI = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    hpVR = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    hpQV = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    hpAA = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if(hpSR && hpQI && hpVR && hpQV && hpAA)
        {
	    test_EnumProcesses();
	    test_EnumProcessModules();
	    test_GetModuleInformation();
	    test_GetProcessMemoryInfo();
            test_GetMappedFileName();
            test_GetProcessImageFileName();
            test_GetModuleFileNameEx();
            test_GetModuleBaseName();
	    test_ws_functions();
	}
	CloseHandle(hpSR);
	CloseHandle(hpQI);
	CloseHandle(hpVR);
	CloseHandle(hpQV);
	CloseHandle(hpAA);
    }
    
    FreeLibrary(hpsapi);
}
