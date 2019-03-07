/*
 * PROJECT:     appshim_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for ForceDxSetupSuccess shim
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include "wine/test.h"

#include "appshim_apitest.h"

static HMODULE g_hShimDll;
static HMODULE g_hSentinelModule = (HMODULE)&g_hShimDll;  /* Not a valid hmodule, so a nice sentinel */
static tGETHOOKAPIS pGetHookAPIs;

static INT (WINAPI *DirectXSetupGetVersion)(DWORD *lpdwVersion, DWORD *lpdwMinorVersion);


typedef HMODULE(WINAPI* LOADLIBRARYAPROC)(PCSTR);
typedef HMODULE(WINAPI* LOADLIBRARYWPROC)(PCWSTR);
typedef FARPROC(WINAPI* GETPROCADDRESSPROC)(HMODULE, PCSTR);
typedef BOOL(WINAPI* FREELIBRARYPROC)(HMODULE);


static HMODULE WINAPI my_LoadLibraryA(PCSTR Name)
{
    return g_hSentinelModule;
}

static void test_LoadLibraryA(PHOOKAPI hook)
{
    LOADLIBRARYAPROC proc;

    ok_str(hook->LibraryName, "KERNEL32.DLL");
    hook->OriginalFunction = my_LoadLibraryA;
    proc = hook->ReplacementFunction;

    /* Original function is not called */
    ok_ptr(proc("dsetup"), g_hShimDll);
    ok_ptr(proc("dsetup.dll"), g_hShimDll);
    ok_ptr(proc("DSETUP.DLL"), g_hShimDll);
    ok_ptr(proc("DSeTuP.DlL"), g_hShimDll);

    ok_ptr(proc("dsetup32"), g_hShimDll);
    ok_ptr(proc("dsetup32.dll"), g_hShimDll);
    ok_ptr(proc("DSETUP32.DLL"), g_hShimDll);
    ok_ptr(proc("DSeTuP32.DlL"), g_hShimDll);

    ok_ptr(proc("c:\\bogus/dsetup"), g_hShimDll);
    ok_ptr(proc("c:\\bogus\\dsetup.dll"), g_hShimDll);
    ok_ptr(proc("c:/bogus/DSETUP.DLL"), g_hShimDll);
    ok_ptr(proc("c:\\bogus\\DSeTuP.DlL"), g_hShimDll);

    ok_ptr(proc("////////////////////dsetup32"), g_hShimDll);
    ok_ptr(proc("\\\\\\\\\\\\\\dsetup32.dll"), g_hShimDll);
    ok_ptr(proc("xxxxxxxxxxx\\DSETUP32.DLL"), g_hShimDll);
    ok_ptr(proc("xxxxxxxxxxx//DSeTuP32.DlL"), g_hShimDll);

    /* Original function is called */
    ok_ptr(proc(NULL), g_hSentinelModule);
    ok_ptr(proc("dsetup\\something"), g_hSentinelModule);
    ok_ptr(proc("dsetup.dl"), g_hSentinelModule);
    ok_ptr(proc("c:\\DSETUP.DLL."), g_hSentinelModule);
    ok_ptr(proc("DSeTuP.D"), g_hSentinelModule);
}

static HMODULE WINAPI my_LoadLibraryW(PCWSTR Name)
{
    return g_hSentinelModule;
}

static void test_LoadLibraryW(PHOOKAPI hook)
{
    LOADLIBRARYWPROC proc;

    ok_str(hook->LibraryName, "KERNEL32.DLL");

    hook->OriginalFunction = my_LoadLibraryW;
    proc = hook->ReplacementFunction;

    /* Original function is not called */
    ok_ptr(proc(L"dsetup"), g_hShimDll);
    ok_ptr(proc(L"dsetup.dll"), g_hShimDll);
    ok_ptr(proc(L"DSETUP.DLL"), g_hShimDll);
    ok_ptr(proc(L"DSeTuP.DlL"), g_hShimDll);

    ok_ptr(proc(L"dsetup32"), g_hShimDll);
    ok_ptr(proc(L"dsetup32.dll"), g_hShimDll);
    ok_ptr(proc(L"DSETUP32.DLL"), g_hShimDll);
    ok_ptr(proc(L"DSeTuP32.DlL"), g_hShimDll);

    ok_ptr(proc(L"c:\\bogus/dsetup"), g_hShimDll);
    ok_ptr(proc(L"c:\\bogus\\dsetup.dll"), g_hShimDll);
    ok_ptr(proc(L"c:/bogus/DSETUP.DLL"), g_hShimDll);
    ok_ptr(proc(L"c:\\bogus\\DSeTuP.DlL"), g_hShimDll);

    ok_ptr(proc(L"////////////////////dsetup32"), g_hShimDll);
    ok_ptr(proc(L"\\\\\\\\\\\\\\dsetup32.dll"), g_hShimDll);
    ok_ptr(proc(L"xxxxxxxxxxx\\DSETUP32.DLL"), g_hShimDll);
    ok_ptr(proc(L"xxxxxxxxxxx//DSeTuP32.DlL"), g_hShimDll);

    /* Original function is called */
    ok_ptr(proc(NULL), g_hSentinelModule);
    ok_ptr(proc(L"dsetup\\something"), g_hSentinelModule);
    ok_ptr(proc(L"dsetup.dl"), g_hSentinelModule);
    ok_ptr(proc(L"c:\\DSETUP.DLL."), g_hSentinelModule);
    ok_ptr(proc(L"DSeTuP.D"), g_hSentinelModule);
}

static void test_GetProcAddress(PHOOKAPI hook)
{
    GETPROCADDRESSPROC proc;
    DWORD n;
    PCSTR Functions[] = {
        "DirectXSetup",
        "DirectXSetupA",
        "DirectXSetupW",
        "DirectXSetupGetVersion",
        /* And not case sensitive? */
        "directxsetup",
        "DIRECTXSETUPA",
        "DiReCtXsEtUpW",
        NULL
    };
    HMODULE mod = GetModuleHandleA("kernel32.dll");

    ok_str(hook->LibraryName, "KERNEL32.DLL");
    hook->OriginalFunction = GetProcAddress;    /* Some versions seem to call GetProcAddress regardless of what is put here.. */
    proc = hook->ReplacementFunction;

    ok_ptr(proc(mod, "CreateFileA"), GetProcAddress(mod, "CreateFileA"));
    ok_ptr(proc(g_hShimDll, "CreateFileA"), NULL);

    for (n = 0; Functions[n]; ++n)
    {
        FARPROC fn = proc(g_hShimDll, Functions[n]);
        ok(fn != NULL, "Got NULL for %s\n", Functions[n]);
        /* And yet, no export! */
        ok_ptr(GetProcAddress(g_hShimDll, Functions[n]), NULL);
    }

    /* Not every function is shimmed */
    ok_ptr(proc(g_hShimDll, "DirectXSetupShowEULA"), NULL);
    /* Retrieve a pointer for our next test */
    DirectXSetupGetVersion = (PVOID)proc(g_hShimDll, "DirectXSetupGetVersion");
}

static int g_NumShim = 0;
static int g_NumSentinel = 0;
static int g_NumCalls = 0;
BOOL WINAPI my_FreeLibrary(HMODULE module)
{
    if (module == g_hSentinelModule)
        ++g_NumSentinel;
    else if (module == g_hShimDll)
        ++g_NumShim;
    ++g_NumCalls;
    return 33;
}

static void test_FreeLibrary(PHOOKAPI hook)
{
    FREELIBRARYPROC proc;

    ok_str(hook->LibraryName, "KERNEL32.DLL");
    hook->OriginalFunction = my_FreeLibrary;
    proc = hook->ReplacementFunction;

    ok_int(proc(NULL), 33);
    ok_int(proc(NULL), 33);
    ok_int(proc(g_hSentinelModule), 33);
    ok_int(proc(g_hSentinelModule), 33);
    ok_int(proc(g_hShimDll), TRUE);
    ok_int(proc(g_hShimDll), TRUE);

    ok_int(g_NumShim, 0);
    ok_int(g_NumSentinel, 2);
    ok_int(g_NumCalls, 4);
}


START_TEST(forcedxsetup)
{
    DWORD num_shims = 0, n;
    PHOOKAPI hook;

    if (!LoadShimDLL(L"aclayers.dll", &g_hShimDll, &pGetHookAPIs))
        return;

    hook = pGetHookAPIs("", L"ForceDxSetupSuccess", &num_shims);

    ok(hook != NULL, "Expected hook to be a valid pointer\n");
    ok(num_shims == 6, "Expected num_shims to be 6, was: %u\n", num_shims);

    if (!hook || !num_shims)
        return;

    for (n = 0; n < num_shims; ++n)
    {
        if (!_stricmp(hook[n].FunctionName, "LoadLibraryA"))
            test_LoadLibraryA(hook + n);
        else if (!_stricmp(hook[n].FunctionName, "LoadLibraryW"))
            test_LoadLibraryW(hook + n);
        else if (!_stricmp(hook[n].FunctionName, "GetProcAddress"))
            test_GetProcAddress(hook + n);
        else if (!_stricmp(hook[n].FunctionName, "FreeLibrary"))
            test_FreeLibrary(hook + n);
    }

    ok(DirectXSetupGetVersion != NULL, "No DirectXSetupGetVersion\n");
    if (DirectXSetupGetVersion)
    {
        DWORD dwVersion = 0xdeadbeef;
        DWORD dwMinorVersion = 0xbeefdead;
        INT Res = DirectXSetupGetVersion(&dwVersion, &dwMinorVersion);
        ok_int(Res, 1);
        ok_hex(dwVersion, MAKELONG(7, 4));     // DirectX 7.0
        ok_hex(dwMinorVersion, MAKELONG(1792, 0));

        Res = DirectXSetupGetVersion(NULL, NULL);
        ok_int(Res, 1);
    }
}
