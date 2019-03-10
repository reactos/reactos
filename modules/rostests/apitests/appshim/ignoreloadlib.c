/*
* PROJECT:     appshim_apitest
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     Tests for IgnoreLoadLibrary shim
* COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
*/

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <ntndk.h>
#include "wine/test.h"

#include "appshim_apitest.h"

static DWORD g_WinVersion;
static tGETHOOKAPIS pGetHookAPIs;
static HMODULE g_hSentinelModule = (HMODULE)&pGetHookAPIs;  /* Not a valid hmodule, so a nice sentinel */
static HMODULE g_h123 = (HMODULE)123;
static HMODULE g_h111 = (HMODULE)111;
static HMODULE g_h0 = (HMODULE)0;

typedef HMODULE(WINAPI* LOADLIBRARYAPROC)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYWPROC)(LPCWSTR lpLibFileName);
typedef HMODULE(WINAPI* LOADLIBRARYEXAPROC)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI* LOADLIBRARYEXWPROC)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);


UINT
WINAPI
GetErrorMode(VOID)
{
    NTSTATUS Status;
    UINT ErrMode;

    /* Query the current setting */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDefaultHardErrorMode,
                                       &ErrMode,
                                       sizeof(ErrMode),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if we couldn't query */
        return 0;
    }

    /* Check if NOT failing critical errors was requested */
    if (ErrMode & SEM_FAILCRITICALERRORS)
    {
        /* Mask it out, since the native API works differently */
        ErrMode &= ~SEM_FAILCRITICALERRORS;
    }
    else
    {
        /* OR it if the caller didn't, due to different native semantics */
        ErrMode |= SEM_FAILCRITICALERRORS;
    }

    /* Return the mode */
    return ErrMode;
}

static HMODULE WINAPI my_LoadLibraryA(PCSTR Name)
{
    DWORD dwErrorMode = GetErrorMode();
    ok(dwErrorMode == (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX),
       "Unexpected error mode: 0x%x\n", dwErrorMode);
    return g_hSentinelModule;
}

static HMODULE WINAPI my_LoadLibraryExA(PCSTR Name, HANDLE hFile, DWORD dwFlags)
{
    DWORD dwErrorMode = GetErrorMode();
    ok(dwErrorMode == (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX),
       "Unexpected error mode: 0x%x\n", dwErrorMode);
    return g_hSentinelModule;
}

static HMODULE WINAPI my_LoadLibraryW(PCWSTR Name)
{
    DWORD dwErrorMode = GetErrorMode();
    ok(dwErrorMode == (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX),
       "Unexpected error mode: 0x%x\n", dwErrorMode);
    return g_hSentinelModule;
}

static HMODULE WINAPI my_LoadLibraryExW(PCWSTR Name, HANDLE hFile, DWORD dwFlags)
{
    DWORD dwErrorMode = GetErrorMode();
    ok(dwErrorMode == (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX),
       "Unexpected error mode: 0x%x\n", dwErrorMode);
    return g_hSentinelModule;
}


static void test_LoadLibraryA(PHOOKAPI hook)
{
    LOADLIBRARYAPROC proc;
    DWORD dwErrorMode, dwOldErrorMode;

    hook->OriginalFunction = my_LoadLibraryA;
    proc = hook->ReplacementFunction;

    dwOldErrorMode = GetErrorMode();

    /* Exact names return what is specified */
    ok_ptr(proc("test123.dll"), g_h123);
    ok_ptr(proc("test111"), g_h111);
    /* Extension is not added */
    ok_ptr(proc("test111.dll"), g_hSentinelModule);
    /* Zero can be specified */
    ok_ptr(proc("Something.mark"), g_h0);
    /* Or default returned */
    ok_ptr(proc("empty"), g_h0);

    /* Paths, do not have to be valid */
    ok_ptr(proc("\\test123.dll"), g_h123);
    ok_ptr(proc("/test123.dll"), g_h123);
    ok_ptr(proc("\\\\\\\\test123.dll"), g_h123);
    ok_ptr(proc("////test123.dll"), g_h123);
    ok_ptr(proc("mypath:something\\does\\not\\matter\\test123.dll"), g_h123);
    ok_ptr(proc("/put/whatever/you/want/here/test123.dll"), g_h123);

    /* path separator is checked, not just any point in the path */
    ok_ptr(proc("-test123.dll"), g_hSentinelModule);
    ok_ptr(proc("test123.dll-"), g_hSentinelModule);

    dwErrorMode = GetErrorMode();
    ok(dwErrorMode == dwOldErrorMode, "ErrorMode changed, was 0x%x, is 0x%x\n", dwOldErrorMode, dwErrorMode);
}

static void test_LoadLibraryW(PHOOKAPI hook)
{
    LOADLIBRARYWPROC proc;
    DWORD dwErrorMode, dwOldErrorMode;

    hook->OriginalFunction = my_LoadLibraryW;
    proc = hook->ReplacementFunction;

    dwOldErrorMode = GetErrorMode();

    /* Exact names return what is specified */
    ok_ptr(proc(L"test123.dll"), g_h123);
    ok_ptr(proc(L"test111"), g_h111);
    /* Extension is not added */
    ok_ptr(proc(L"test111.dll"), g_hSentinelModule);
    /* Zero can be specified */
    ok_ptr(proc(L"Something.mark"), g_h0);
    /* Or default returned */
    ok_ptr(proc(L"empty"), g_h0);

    /* Paths, do not have to be valid */
    ok_ptr(proc(L"\\test123.dll"), g_h123);
    ok_ptr(proc(L"/test123.dll"), g_h123);
    ok_ptr(proc(L"\\\\\\\\test123.dll"), g_h123);
    ok_ptr(proc(L"////test123.dll"), g_h123);
    ok_ptr(proc(L"mypath:something\\does\\not\\matter\\test123.dll"), g_h123);
    ok_ptr(proc(L"/put/whatever/you/want/here/test123.dll"), g_h123);

    /* path separator is checked, not just any point in the path */
    ok_ptr(proc(L"-test123.dll"), g_hSentinelModule);
    ok_ptr(proc(L"test123.dll-"), g_hSentinelModule);

    dwErrorMode = GetErrorMode();
    ok(dwErrorMode == dwOldErrorMode, "ErrorMode changed, was 0x%x, is 0x%x\n", dwOldErrorMode, dwErrorMode);
}

static void test_LoadLibraryExA(PHOOKAPI hook)
{
    LOADLIBRARYEXAPROC proc;
    DWORD dwErrorMode, dwOldErrorMode;

    hook->OriginalFunction = my_LoadLibraryExA;
    proc = hook->ReplacementFunction;

    dwOldErrorMode = GetErrorMode();

    /* Exact names return what is specified */
    ok_ptr(proc("test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("test111", INVALID_HANDLE_VALUE, 0), g_h111);
    /* Extension is not added */
    ok_ptr(proc("test111.dll", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);
    /* Zero can be specified */
    ok_ptr(proc("Something.mark", INVALID_HANDLE_VALUE, 0), g_h0);
    /* Or default returned */
    ok_ptr(proc("empty", INVALID_HANDLE_VALUE, 0), g_h0);

    /* Paths, do not have to be valid */
    ok_ptr(proc("\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("/test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("\\\\\\\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("////test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("mypath:something\\does\\not\\matter\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc("/put/whatever/you/want/here/test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);

    /* path separator is checked, not just any point in the path */
    ok_ptr(proc("-test123.dll", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);
    ok_ptr(proc("test123.dll-", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);

    dwErrorMode = GetErrorMode();
    ok(dwErrorMode == dwOldErrorMode, "ErrorMode changed, was 0x%x, is 0x%x\n", dwOldErrorMode, dwErrorMode);
}

static void test_LoadLibraryExW(PHOOKAPI hook)
{
    LOADLIBRARYEXWPROC proc;
    DWORD dwErrorMode, dwOldErrorMode;

    hook->OriginalFunction = my_LoadLibraryExW;
    proc = hook->ReplacementFunction;

    dwOldErrorMode = GetErrorMode();

    /* Exact names return what is specified */
    ok_ptr(proc(L"test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"test111", INVALID_HANDLE_VALUE, 0), g_h111);
    /* Extension is not added */
    ok_ptr(proc(L"test111.dll", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);
    /* Zero can be specified */
    ok_ptr(proc(L"Something.mark", INVALID_HANDLE_VALUE, 0), g_h0);
    /* Or default returned */
    ok_ptr(proc(L"empty", INVALID_HANDLE_VALUE, 0), g_h0);

    /* Paths, do not have to be valid */
    ok_ptr(proc(L"\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"/test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"\\\\\\\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"////test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"mypath:something\\does\\not\\matter\\test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);
    ok_ptr(proc(L"/put/whatever/you/want/here/test123.dll", INVALID_HANDLE_VALUE, 0), g_h123);

    /* path separator is checked, not just any point in the path */
    ok_ptr(proc(L"-test123.dll", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);
    ok_ptr(proc(L"test123.dll-", INVALID_HANDLE_VALUE, 0), g_hSentinelModule);

    dwErrorMode = GetErrorMode();
    ok(dwErrorMode == dwOldErrorMode, "ErrorMode changed, was 0x%x, is 0x%x\n", dwOldErrorMode, dwErrorMode);
}

/* versionlie.c */
DWORD get_host_winver(void);

START_TEST(ignoreloadlib)
{
    DWORD num_shims = 0, n, dwErrorMode;
    PHOOKAPI hook;

    g_WinVersion = get_host_winver();

    if (g_WinVersion < _WIN32_WINNT_WIN8)
        pGetHookAPIs = LoadShimDLL2(L"aclayers.dll");
    else
        pGetHookAPIs = LoadShimDLL2(L"acgenral.dll");

    if (!pGetHookAPIs)
    {
        skip("GetHookAPIs not found\n");
        return;
    }

    hook = pGetHookAPIs("test123.dll:123;test111:111;Something.mark:0;empty", L"IgnoreLoadLibrary", &num_shims);

    ok(hook != NULL, "Expected hook to be a valid pointer\n");
    ok(num_shims == 4, "Expected num_shims to be 0, was: %u\n", num_shims);

    if (!hook || num_shims != 4)
        return;

    dwErrorMode = GetErrorMode();
    trace("Error mode: 0x%x\n", dwErrorMode);

    for (n = 0; n < num_shims; ++n)
    {
        ok_str(hook[n].LibraryName, "KERNEL32.DLL");
        if (!_stricmp(hook[n].FunctionName, "LoadLibraryA"))
        {
            ok_int(n, 0);
            test_LoadLibraryA(hook + n);
        }
        else if (!_stricmp(hook[n].FunctionName, "LoadLibraryExA"))
        {
            ok_int(n, 1);
            test_LoadLibraryExA(hook + n);
        }
        else if (!_stricmp(hook[n].FunctionName, "LoadLibraryW"))
        {
            ok_int(n, 2);
            test_LoadLibraryW(hook + n);
        }
        else if (!_stricmp(hook[n].FunctionName, "LoadLibraryExW"))
        {
            ok_int(n, 3);
            test_LoadLibraryExW(hook + n);
        }
        else
        {
            ok(0, "Unknown function %s\n", hook[n].FunctionName);
        }
    }
}
