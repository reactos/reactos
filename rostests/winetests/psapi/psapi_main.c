/*
 * Unit test suite for PSAPI
 *
 * Copyright (C) 2005 Felix Nawothnig
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

#include "windows.h"
#include "wine/test.h"
#include "psapi.h"

#define expect_eq_d(expected, actual) \
    do { \
      int value = (actual); \
      ok((expected) == value, "Expected " #actual " to be %d (" #expected ") is %d\n", \
          (expected), value); \
    } while (0)

#define PSAPI_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hpsapi, #func); \
    if(!p ## func) { \
        ok(0, "GetProcAddress(%s) failed\n", #func); \
        FreeLibrary(hpsapi); \
        return FALSE; \
    }

/* All PSAPI functions return non-zero and call SetLastError() 
 * on failure so we can use some macros for convenience */

#define w32_suc(x) \
  (SetLastError(0xdeadbeef), \
   (x) \
     ? (ok(1, "succeeded\n"), 1) \
     : GetLastError() == 0xdeadbeef \
       ? (ok(0, "failed without error code\n"), 0) \
       : (ok(0, "failed with %d\n", GetLastError()), 0))

#define w32_err(x, e) \
  (SetLastError(0xdeadbeef), \
   (x) \
     ? (ok(0, "expected error=%d but succeeded\n", e), 0) \
     : GetLastError() == e \
       ? (ok(1, "failed with %d\n", e), 1) \
       : GetLastError() == 0xdeadbeef \
         ? (ok(0, "failed without error code\n"), 0) \
         : (ok(0, "expected error=%d but failed with %d\n", \
	         e, GetLastError()), 0))

static BOOL  (WINAPI *pEmptyWorkingSet)(HANDLE);
static BOOL  (WINAPI *pEnumProcesses)(DWORD*, DWORD, DWORD*);
static BOOL  (WINAPI *pEnumProcessModules)(HANDLE, HMODULE*, DWORD, LPDWORD);
static DWORD (WINAPI *pGetModuleBaseNameA)(HANDLE, HMODULE, LPSTR, DWORD);
static DWORD (WINAPI *pGetModuleFileNameExA)(HANDLE, HMODULE, LPSTR, DWORD);
static BOOL  (WINAPI *pGetModuleInformation)(HANDLE, HMODULE, LPMODULEINFO, DWORD);
static DWORD (WINAPI *pGetMappedFileNameA)(HANDLE, LPVOID, LPSTR, DWORD);
static DWORD (WINAPI *pGetProcessImageFileNameA)(HANDLE, LPSTR, DWORD);
static DWORD (WINAPI *pGetProcessImageFileNameW)(HANDLE, LPWSTR, DWORD);
static BOOL  (WINAPI *pGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
static BOOL  (WINAPI *pGetWsChanges)(HANDLE, PPSAPI_WS_WATCH_INFORMATION, DWORD);
static BOOL  (WINAPI *pInitializeProcessForWsWatch)(HANDLE);
static BOOL  (WINAPI *pQueryWorkingSet)(HANDLE, PVOID, DWORD);
      
static BOOL InitFunctionPtrs(HMODULE hpsapi)
{
    PSAPI_GET_PROC(EmptyWorkingSet);
    PSAPI_GET_PROC(EnumProcessModules);
    PSAPI_GET_PROC(EnumProcesses);
    PSAPI_GET_PROC(GetModuleBaseNameA);
    PSAPI_GET_PROC(GetModuleFileNameExA);
    PSAPI_GET_PROC(GetModuleInformation);
    PSAPI_GET_PROC(GetMappedFileNameA);
    PSAPI_GET_PROC(GetProcessMemoryInfo);
    PSAPI_GET_PROC(GetWsChanges);
    PSAPI_GET_PROC(InitializeProcessForWsWatch);
    PSAPI_GET_PROC(QueryWorkingSet);
    /* GetProcessImageFileName is not exported on NT4 */
    pGetProcessImageFileNameA =
      (void *)GetProcAddress(hpsapi, "GetProcessImageFileNameA");
    pGetProcessImageFileNameW =
      (void *)GetProcAddress(hpsapi, "GetProcessImageFileNameW");
    return TRUE;
}

static HANDLE hpSR, hpQI, hpVR, hpQV, hpAA;
static const HANDLE hBad = (HANDLE)0xdeadbeef;

static void test_EnumProcesses(void)
{
    DWORD pid, cbUsed = 0xdeadbeef;

    if(w32_suc(pEnumProcesses(NULL, 0, &cbUsed)))
        ok(cbUsed == 0, "cbUsed=%d\n", cbUsed);
    if(w32_suc(pEnumProcesses(&pid, 4, &cbUsed)))
        ok(cbUsed == 4, "cbUsed=%d\n", cbUsed);
}

static void test_EnumProcessModules(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    DWORD cbNeeded = 0xdeadbeef;

    w32_err(pEnumProcessModules(NULL, NULL, 0, &cbNeeded), ERROR_INVALID_HANDLE);
    w32_err(pEnumProcessModules(hpQI, NULL, 0, &cbNeeded), ERROR_ACCESS_DENIED);
    w32_suc(pEnumProcessModules(hpQV, NULL, 0, &cbNeeded));
    if(!w32_suc(pEnumProcessModules(hpQV, &hMod, sizeof(HMODULE), &cbNeeded)))
        return;
    ok(cbNeeded / sizeof(HMODULE) >= 3 && cbNeeded / sizeof(HMODULE) <= 5 * sizeof(HMODULE),
       "cbNeeded=%d\n", cbNeeded);
    ok(hMod == GetModuleHandle(NULL),
       "hMod=%p GetModuleHandle(NULL)=%p\n", hMod, GetModuleHandle(NULL));
}

static void test_GetModuleInformation(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    MODULEINFO info;
    
    w32_err(pGetModuleInformation(NULL, hMod, &info, sizeof(info)), ERROR_INVALID_HANDLE);
    w32_err(pGetModuleInformation(hpQI, hMod, &info, sizeof(info)), ERROR_ACCESS_DENIED);
    w32_err(pGetModuleInformation(hpQV, hBad, &info, sizeof(info)), ERROR_INVALID_HANDLE);
    w32_err(pGetModuleInformation(hpQV, hMod, &info, sizeof(info)-1), ERROR_INSUFFICIENT_BUFFER);
    if(w32_suc(pGetModuleInformation(hpQV, hMod, &info, sizeof(info))))
        ok(info.lpBaseOfDll == hMod, "lpBaseOfDll=%p hMod=%p\n", info.lpBaseOfDll, hMod);
}

static void test_GetProcessMemoryInfo(void)
{
    PROCESS_MEMORY_COUNTERS pmc;

    w32_err(pGetProcessMemoryInfo(NULL, &pmc, sizeof(pmc)), ERROR_INVALID_HANDLE);
    todo_wine w32_err(pGetProcessMemoryInfo(hpSR, &pmc, sizeof(pmc)), ERROR_ACCESS_DENIED);
    w32_err(pGetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc)-1), ERROR_INSUFFICIENT_BUFFER);
    w32_suc(pGetProcessMemoryInfo(hpQI, &pmc, sizeof(pmc)));
}

static void test_GetMappedFileName(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    char szMapPath[MAX_PATH], szModPath[MAX_PATH], *szMapBaseName;
    DWORD ret;
    
    w32_err(pGetMappedFileNameA(NULL, hMod, szMapPath, sizeof(szMapPath)), ERROR_INVALID_HANDLE);
    w32_err(pGetMappedFileNameA(hpSR, hMod, szMapPath, sizeof(szMapPath)), ERROR_ACCESS_DENIED);
    if(!w32_suc(ret = pGetMappedFileNameA(hpQI, hMod, szMapPath, sizeof(szMapPath))))
        return;
    ok(ret == strlen(szMapPath), "szMapPath=\"%s\" ret=%d\n", szMapPath, ret);
    ok(szMapPath[0] == '\\', "szMapPath=\"%s\"\n", szMapPath);
    szMapBaseName = strrchr(szMapPath, '\\'); /* That's close enough for us */
    if(!szMapBaseName || !*szMapBaseName)
    {
        ok(0, "szMapPath=\"%s\"\n", szMapPath);
        return;
    }
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strcmp(strrchr(szModPath, '\\'), szMapBaseName),
       "szModPath=\"%s\" szMapBaseName=\"%s\"\n", szModPath, szMapBaseName);
}

static void test_GetProcessImageFileName(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    char szImgPath[MAX_PATH], szMapPath[MAX_PATH];
    WCHAR szImgPathW[MAX_PATH];
    DWORD ret;

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

    todo_wine w32_err(pGetProcessImageFileNameA(NULL, szImgPath, sizeof(szImgPath)), ERROR_INVALID_HANDLE);
    todo_wine w32_err(pGetProcessImageFileNameA(hpSR, szImgPath, sizeof(szImgPath)), ERROR_ACCESS_DENIED);
    todo_wine w32_err(pGetProcessImageFileNameA(hpQI, szImgPath, 0), ERROR_INSUFFICIENT_BUFFER);
    todo_wine
    if(w32_suc(ret = pGetProcessImageFileNameA(hpQI, szImgPath, sizeof(szImgPath))) &&
       w32_suc(pGetMappedFileNameA(hpQV, hMod, szMapPath, sizeof(szMapPath)))) {
        /* Windows returns 2*strlen-1 */
        ok(ret >= strlen(szImgPath), "szImgPath=\"%s\" ret=%d\n", szImgPath, ret);
        ok(!strcmp(szImgPath, szMapPath),
           "szImgPath=\"%s\" szMapPath=\"%s\"\n", szImgPath, szMapPath);
    }

    w32_err(pGetProcessImageFileNameW(NULL, szImgPathW, sizeof(szImgPathW)), ERROR_INVALID_HANDLE);
    /* no information about correct buffer size returned: */
    w32_err(pGetProcessImageFileNameW(hpQI, szImgPathW, 0), ERROR_INSUFFICIENT_BUFFER);
    w32_err(pGetProcessImageFileNameW(hpQI, NULL, 0), ERROR_INSUFFICIENT_BUFFER);

    /* correct call */
    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = pGetProcessImageFileNameW(hpQI, szImgPathW, sizeof(szImgPathW)/sizeof(WCHAR));
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    expect_eq_d(lstrlenW(szImgPathW), ret);

    /* boundary values of 'size' */
    w32_err(pGetProcessImageFileNameW(hpQI, szImgPathW, ret), ERROR_INSUFFICIENT_BUFFER);

    memset(szImgPathW, 0xff, sizeof(szImgPathW));
    ret = pGetProcessImageFileNameW(hpQI, szImgPathW, ret + 1);
    ok(ret > 0, "GetProcessImageFileNameW should have succeeded.\n");
    ok(szImgPathW[0] == '\\', "GetProcessImageFileNameW should have returned an NT path.\n");
    expect_eq_d(lstrlenW(szImgPathW), ret);
}

static void test_GetModuleFileNameEx(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    char szModExPath[MAX_PATH+1], szModPath[MAX_PATH+1];
    DWORD ret;
    
    w32_err(pGetModuleFileNameExA(NULL, hMod, szModExPath, sizeof(szModExPath)), ERROR_INVALID_HANDLE);
    w32_err(pGetModuleFileNameExA(hpQI, hMod, szModExPath, sizeof(szModExPath)), ERROR_ACCESS_DENIED);
    w32_err(pGetModuleFileNameExA(hpQV, hBad, szModExPath, sizeof(szModExPath)), ERROR_INVALID_HANDLE);
    if(!w32_suc(ret = pGetModuleFileNameExA(hpQV, NULL, szModExPath, sizeof(szModExPath))))
        return;
    ok(ret == strlen(szModExPath), "szModExPath=\"%s\" ret=%d\n", szModExPath, ret);
    GetModuleFileNameA(NULL, szModPath, sizeof(szModPath));
    ok(!strncmp(szModExPath, szModPath, MAX_PATH), 
       "szModExPath=\"%s\" szModPath=\"%s\"\n", szModExPath, szModPath);
}

static void test_GetModuleBaseName(void)
{
    HMODULE hMod = GetModuleHandle(NULL);
    char szModPath[MAX_PATH], szModBaseName[MAX_PATH];
    DWORD ret;

    w32_err(pGetModuleBaseNameA(NULL, hMod, szModBaseName, sizeof(szModBaseName)), ERROR_INVALID_HANDLE);
    w32_err(pGetModuleBaseNameA(hpQI, hMod, szModBaseName, sizeof(szModBaseName)), ERROR_ACCESS_DENIED);
    w32_err(pGetModuleBaseNameA(hpQV, hBad, szModBaseName, sizeof(szModBaseName)), ERROR_INVALID_HANDLE);
    if(!w32_suc(ret = pGetModuleBaseNameA(hpQV, NULL, szModBaseName, sizeof(szModBaseName))))
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
    
    todo_wine w32_err(pEmptyWorkingSet(NULL), ERROR_INVALID_HANDLE);
    todo_wine w32_err(pEmptyWorkingSet(hpSR), ERROR_ACCESS_DENIED);
    w32_suc(pEmptyWorkingSet(hpAA));
    
    todo_wine w32_err(pInitializeProcessForWsWatch(NULL), ERROR_INVALID_HANDLE);
    w32_suc(pInitializeProcessForWsWatch(hpAA));
    
    if(!w32_suc(addr = VirtualAlloc(NULL, 1, MEM_COMMIT, PAGE_READWRITE)))
        return;

    if(!VirtualLock(addr, 1))
    {
        trace("locking failed (error=%d) - skipping test\n", GetLastError());
        goto free_page;
    }
	
    todo_wine if(w32_suc(pQueryWorkingSet(hpQI, pages, 4096 * sizeof(ULONG_PTR))))
    {
       for(i = 0; i < pages[0]; i++)
           if((pages[i+1] & ~0xfffL) == (ULONG_PTR)addr)
	   {
	       ok(1, "QueryWorkingSet found our page\n");
	       goto test_gwsc;
	   }
       
       ok(0, "QueryWorkingSet didn't find our page\n");
    }

test_gwsc:
    todo_wine if(w32_suc(pGetWsChanges(hpQI, wswi, sizeof(wswi))))
    {
        for(i = 0; wswi[i].FaultingVa; i++)
	    if(((ULONG_PTR)wswi[i].FaultingVa & ~0xfffL) == (ULONG_PTR)addr)
	    {
	        ok(1, "GetWsChanges found our page\n");
		goto free_page;
	    }

	ok(0, "GetWsChanges didn't find our page\n");
    }
    
free_page:
    VirtualFree(addr, 0, MEM_RELEASE);
}

START_TEST(psapi_main)
{
    HMODULE hpsapi = LoadLibraryA("psapi.dll");
    
    if(!hpsapi)
    {
        trace("Could not load psapi.dll\n");
        return;
    }

    if(InitFunctionPtrs(hpsapi))
    {
        DWORD pid = GetCurrentProcessId();

        w32_suc(hpSR = OpenProcess(STANDARD_RIGHTS_REQUIRED, FALSE, pid));
        w32_suc(hpQI = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid));
        w32_suc(hpVR = OpenProcess(PROCESS_VM_READ, FALSE, pid));
        w32_suc(hpQV = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid));
	w32_suc(hpAA = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
        if(hpSR && hpQI && hpVR && hpQV && hpAA)
        {
	    test_EnumProcesses();
	    test_EnumProcessModules();
	    test_GetModuleInformation();
	    test_GetProcessMemoryInfo();
	    todo_wine test_GetMappedFileName();
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
