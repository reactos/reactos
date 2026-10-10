/*
 * ATL test program
 *
 * Copyright 2010 Damjan Jovanovic
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

#define COBJMACROS

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <wtypes.h>
#include <olectl.h>
#include <ocidl.h>
#include <initguid.h>
#include <atliface.h>

static const char textA[] =
"HKCU \n"
"{ \n"
"    ForceRemove eebf73c4-50fd-478f-bbcf-db212221227a \n"
"    { \n"
"        val 'str1' = s 'string' \n"
"        val 'str2' = s 'str\\\"ing' \n"
"        val 'str3' = s 'str''ing' \n"
"        val 'dword_quoted_dec' = d '1' \n"
"        val 'dword_unquoted_dec' = d 1 \n"
"        val 'dword_quoted_hex' = d '0xA' \n"
"        val 'dword_unquoted_hex' = d 0xA \n"
"        val 'dword_negative' = d -2147483648 \n"
"        val 'dword_ulong' = d 2147483649 \n"
"        val 'dword_max' = d 4294967295 \n"
"        val 'dword_overrange' = d 4294967296 \n"
"        val 'binary_quoted' = b 'deadbeef' \n"
"        val 'binary_unquoted' = b dead0123 \n"
"    } \n"
"}";

static void test_registrar(void)
{
    IRegistrar *registrar = NULL;
    HRESULT hr;
    INT count;
    int i;
    WCHAR *textW = NULL;
    struct dword_test
    {
        const char *name;
        BOOL preserved;
        LONGLONG value;
    } dword_tests[] =
    {
        { "dword_unquoted_dec", TRUE,  1 },
        { "dword_quoted_dec",   TRUE,  1 },
        { "dword_quoted_hex",   FALSE, 0xA },
        { "dword_unquoted_hex", FALSE, 0xA },
        { "dword_negative",     FALSE, -2147483648 },
        { "dword_ulong",        TRUE,  2147483649 },
        { "dword_max",          TRUE,  4294967295 },
        { "dword_overrange",    FALSE, 4294967296 },
    };

    if (!GetProcAddress(GetModuleHandleA("atl.dll"), "AtlAxAttachControl"))
    {
        win_skip("Old versions of atl.dll don't support binary values\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_Registrar, NULL, CLSCTX_INPROC_SERVER, &IID_IRegistrar, (void**)&registrar);
    if (FAILED(hr))
    {
        win_skip("creating IRegistrar failed, hr = 0x%08lX\n", hr);
        return;
    }

    count = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
    textW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
    if (textW)
    {
        DWORD dword;
        DWORD size;
        LONG lret;
        HKEY key;
        BYTE bytes[4];
        char buffer[16];

        MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, count);
        hr = IRegistrar_StringRegister(registrar, textW);
        ok(hr == S_OK, "StringRegister failed: %08lx\n", hr);
        if (FAILED(hr))
        {
            IRegistrar_Release(registrar);
            return;
        }

        lret = RegOpenKeyA(HKEY_CURRENT_USER, "eebf73c4-50fd-478f-bbcf-db212221227a", &key);
        ok(lret == ERROR_SUCCESS, "error %ld opening registry key\n", lret);

        size = sizeof(buffer);
        lret = RegQueryValueExA(key, "str1", NULL, NULL, (BYTE*)buffer, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %ld\n", lret);
        ok(!strcmp( buffer, "string"), "wrong data %s\n", debugstr_a(buffer));

        size = sizeof(buffer);
        lret = RegQueryValueExA(key, "str2", NULL, NULL, (BYTE*)buffer, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %ld\n", lret);
        ok(!strcmp( buffer, "str\\\"ing"), "wrong data %s\n", debugstr_a(buffer));

        size = sizeof(buffer);
        lret = RegQueryValueExA(key, "str3", NULL, NULL, (BYTE*)buffer, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %ld\n", lret);
        ok(!strcmp( buffer, "str'ing"), "wrong data %s\n", debugstr_a(buffer));

        for (i = 0; i < ARRAYSIZE(dword_tests); i++)
        {
            size = sizeof(dword);
            lret = RegQueryValueExA(key, dword_tests[i].name, NULL, NULL, (BYTE*)&dword, &size);
            ok(lret == ERROR_SUCCESS, "Test %d: RegQueryValueExA failed %ld.\n", i, lret);
            if (dword_tests[i].preserved)
                ok(dword == dword_tests[i].value, "Test %d: got unexpected value %lu.\n", i, dword);
            else
                ok(dword != dword_tests[i].value, "Test %d: is not supposed to be preserved.\n", i);
        }

        size = 4;
        lret = RegQueryValueExA(key, "binary_quoted", NULL, NULL, bytes, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueA, failed, error %ld\n", lret);
        ok(bytes[0] == 0xde && bytes[1] == 0xad && bytes[2] == 0xbe && bytes[3] == 0xef,
            "binary quoted value was not preserved (it's 0x%02X%02X%02X%02X)\n",
            0xff & bytes[0], 0xff & bytes[1], 0xff & bytes[2], 0xff & bytes[3]);

        size = 4;
        lret = RegQueryValueExA(key, "binary_unquoted", NULL, NULL, bytes, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueA, failed, error %ld\n", lret);
        ok(bytes[0] == 0xde && bytes[1] == 0xad && bytes[2] == 0x01 && bytes[3] == 0x23,
            "binary unquoted value was not preserved (it's 0x%02X%02X%02X%02X)\n",
            0xff & bytes[0], 0xff & bytes[1], 0xff & bytes[2], 0xff & bytes[3]);

        hr = IRegistrar_StringUnregister(registrar, textW);
        ok(SUCCEEDED(hr), "IRegistrar_StringUnregister failed, hr = 0x%08lX\n", hr);
        RegCloseKey(key);

        HeapFree(GetProcessHeap(), 0, textW);
    }
    else
        skip("allocating memory failed\n");

    IRegistrar_Release(registrar);
}

static void test_aggregation(void)
{
    IUnknown *unk = (IUnknown*)0xdeadbeef;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_Registrar, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == CLASS_E_NOAGGREGATION || broken(hres == E_INVALIDARG),
            "CoCreateInstance failed: %08lx, expected CLASS_E_NOAGGREGATION\n", hres);
    ok(!unk || unk == (IUnknown*)0xdeadbeef, "unk = %p\n", unk);
}

START_TEST(registrar)
{
    CoInitialize(NULL);

    test_registrar();
    test_aggregation();

    CoUninitialize();
}
