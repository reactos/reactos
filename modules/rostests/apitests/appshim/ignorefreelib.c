/*
 * PROJECT:     appshim_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for IgnoreFreeLibrary shim
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include "wine/test.h"

#include "appshim_apitest.h"

static tGETHOOKAPIS pGetHookAPIs;

typedef BOOL(WINAPI* FREELIBRARYPROC)(HMODULE);


static void test_IgnoreFreeLibrary(VOID)
{
    DWORD num_shims = 0;
    PHOOKAPI hook;
    HMODULE avifil32, esent, msi;
    FREELIBRARYPROC freeProc;

    // Show that:
    // * partial names (both start + end) do not work.
    // * wildcards do not work
    // * comparison is case insensitive
    // * multiple dlls can be specified
    hook = pGetHookAPIs("AvIFiL32.Dll;esent;esent*;ent.dll;msi.dll", L"IgnoreFreeLibrary", &num_shims);

    ok(hook != NULL, "Expected hook to be a valid pointer\n");
    ok(num_shims == 1, "Expected num_shims to be 1, was: %u\n", num_shims);

    if (!hook || !num_shims)
        return;

    ok_str(hook->LibraryName, "KERNEL32.DLL");
    ok_str(hook->FunctionName, "FreeLibrary");

    avifil32 = LoadLibraryA("avifil32.dll");
    ok(avifil32 != NULL, "Unable to load avifil32\n");

    esent = LoadLibraryA("esent.dll");
    ok(esent != NULL, "Unable to load esent\n");

    msi = LoadLibraryA("msi.dll");
    ok(msi != NULL, "Unable to load msi\n");

    hook->OriginalFunction = FreeLibrary;
    freeProc = hook->ReplacementFunction;

    ok(freeProc != NULL, "\n");

    if (!freeProc)
        return;

    // Not unloading
    ok(freeProc(avifil32), "Unable to unload avifil32\n");
    avifil32 = GetModuleHandleA("avifil32.dll");
    ok(avifil32 != NULL, "avifil32 should not unload\n");

    // Unloading
    ok(freeProc(esent), "Unable to unload esent\n");
    esent = GetModuleHandleA("esent.dll");
    ok(esent == NULL, "esent should be unloaded\n");

    // Not unloading
    ok(freeProc(msi), "Unable to unload msi\n");
    msi = GetModuleHandleA("msi.dll");
    ok(msi != NULL, "msi should not unload\n");
}


START_TEST(ignorefreelib)
{
    HMODULE avifil32, esent, msi;
    LONG initial;

    pGetHookAPIs = LoadShimDLL2(L"acgenral.dll");

    if (!pGetHookAPIs)
        return;

    /* Ensure that we can freely load / unload avifil32, esent and msi */

    initial = winetest_get_failures();

    avifil32 = GetModuleHandleA("avifil32.dll");
    ok_ptr(avifil32, NULL);
    esent = GetModuleHandleA("esent.dll");
    ok_ptr(esent, NULL);
    msi = GetModuleHandleA("msi.dll");
    ok_ptr(msi, NULL);

    avifil32 = LoadLibraryA("avifil32.dll");
    ok(avifil32 != NULL, "Unable to load avifil32\n");
    esent = GetModuleHandleA("esent.dll");
    ok_ptr(esent, NULL);
    msi = GetModuleHandleA("msi.dll");
    ok_ptr(msi, NULL);

    ok(FreeLibrary(avifil32), "Unable to unload avifil32\n");
    avifil32 = GetModuleHandleA("avifil32.dll");
    ok_ptr(avifil32, NULL);

    esent = LoadLibraryA("esent.dll");
    ok(esent != NULL, "Unable to load esent\n");
    avifil32 = GetModuleHandleA("avifil32.dll");
    ok_ptr(avifil32, NULL);
    msi = GetModuleHandleA("msi.dll");
    ok_ptr(msi, NULL);

    ok(FreeLibrary(esent), "Unable to unload esent\n");
    esent = GetModuleHandleA("esent.dll");
    ok_ptr(esent, NULL);

    msi = LoadLibraryA("msi.dll");
    ok(msi != NULL, "Unable to load msi\n");
    avifil32 = GetModuleHandleA("avifil32.dll");
    ok_ptr(avifil32, NULL);
    esent = GetModuleHandleA("esent.dll");
    ok_ptr(esent, NULL);

    ok(FreeLibrary(msi), "Unable to unload msi\n");
    msi = GetModuleHandleA("msi.dll");
    ok_ptr(msi, NULL);

    if (initial != winetest_get_failures())
    {
        // We cannot continue, one or more of our target dll's does not behave as expected
        return;
    }

    test_IgnoreFreeLibrary();
}
