/*
 * PROJECT:     appshim_apitest
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test to document the hooks used by various shims in AcLayers
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <ndk/rtlfuncs.h>
#include <strsafe.h>
#include "wine/test.h"

#include "appshim_apitest.h"

static DWORD g_WinVersion;


typedef struct expect_shim_hook
{
    const char* Library;
    const char* Function;
} expect_shim_hook;

typedef struct expect_shim_data
{
    const WCHAR* ShimName;
    DWORD MinVersion;
    expect_shim_hook hooks[6];
} expect_shim_data;


static expect_shim_data data[] =
{
    {
        L"ForceDXSetupSuccess",
        0,
        {
            { "KERNEL32.DLL", "LoadLibraryA" },
            { "KERNEL32.DLL", "LoadLibraryW" },
            { "KERNEL32.DLL", "LoadLibraryExA" },
            { "KERNEL32.DLL", "LoadLibraryExW" },
            { "KERNEL32.DLL", "GetProcAddress" },
            { "KERNEL32.DLL", "FreeLibrary" },
        }
    },
    {
        L"VerifyVersionInfoLite",
        _WIN32_WINNT_VISTA,
        {
            { "KERNEL32.DLL", "VerifyVersionInfoA" },
            { "KERNEL32.DLL", "VerifyVersionInfoW" },
        }
    },
    /* Show that it is not case sensitive */
    {
        L"VeRiFyVeRsIoNInFoLiTe",
        _WIN32_WINNT_VISTA,
        {
            { "KERNEL32.DLL", "VerifyVersionInfoA" },
            { "KERNEL32.DLL", "VerifyVersionInfoW" },
        }
    },
};

static DWORD count_shims(expect_shim_data* data)
{
    DWORD num;
    for (num = 0; num < _countof(data->hooks) && data->hooks[num].Library;)
    {
        ++num;
    }
    return num;
}

static const char* safe_str(const char* ptr)
{
    static char buffer[2][30];
    static int index = 0;
    if (HIWORD(ptr))
        return ptr;

    index ^= 1;
    StringCchPrintfA(buffer[index], _countof(buffer[index]), "#%Id", (intptr_t)ptr);
    return buffer[index];
}

START_TEST(layer_hooks)
{
    RTL_OSVERSIONINFOEXW rtlinfo = {0};
    size_t n, h;

    tGETHOOKAPIS pGetHookAPIs = LoadShimDLL2(L"AcLayers.dll");
    if (!pGetHookAPIs)
        return;

    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
    RtlGetVersion((PRTL_OSVERSIONINFOW)&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;



    for (n = 0; n < _countof(data); ++n)
    {
        expect_shim_data* current = data + n;
        DWORD num_shims = 0, expected_shims = count_shims(current);

        PHOOKAPI hook = pGetHookAPIs("", current->ShimName, &num_shims);

        if (current->MinVersion > g_WinVersion && !hook)
        {
            skip("Shim %s not present\n", wine_dbgstr_w(current->ShimName));
            continue;
        }

        ok(!!hook, "Expected a valid pointer, got nothing for %s\n", wine_dbgstr_w(current->ShimName));
        ok(num_shims == expected_shims, "Expected %u shims, got %u for %s\n",
           expected_shims, num_shims, wine_dbgstr_w(current->ShimName));
        for (h = 0; h < min(num_shims, expected_shims); ++h)
        {
            expect_shim_hook* expect_hk = current->hooks + h;
            PHOOKAPI got_hk = hook+h;
            int lib = lstrcmpA(expect_hk->Library, got_hk->LibraryName);
            int fn = lstrcmpA(safe_str(expect_hk->Function), safe_str(got_hk->FunctionName));
            ok(lib == 0, "Expected LibraryName to be %s, was: %s for %s\n",
               expect_hk->Library, got_hk->LibraryName, wine_dbgstr_w(current->ShimName));
            ok(fn == 0, "Expected FunctionName to be %s, was: %s for %s\n",
               safe_str(expect_hk->Function), safe_str(got_hk->FunctionName), wine_dbgstr_w(current->ShimName));
        }
        if (num_shims > expected_shims)
        {
            for (h = expected_shims; h < num_shims; ++h)
            {
                PHOOKAPI got_hk = hook+h;

                ok(0, "Extra shim: %s!%s for %s\n",
                   got_hk->LibraryName, safe_str(got_hk->FunctionName), wine_dbgstr_w(current->ShimName));
            }
        }
        else
        {
            for (h = num_shims; h < expected_shims; ++h)
            {
                expect_shim_hook* expect_hk = current->hooks + h;

                ok(0, "Missing shim: %s!%s for %s\n",
                   expect_hk->Library, safe_str(expect_hk->Function), wine_dbgstr_w(current->ShimName));
            }
        }
    }
}
