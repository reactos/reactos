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
"        val 'string' = s 'string' \n"
"        val 'dword_quoted_dec' = d '1' \n"
"        val 'dword_unquoted_dec' = d 1 \n"
"        val 'dword_quoted_hex' = d '0xA' \n"
"        val 'dword_unquoted_hex' = d 0xA \n"
"        val 'binary_quoted' = b 'deadbeef' \n"
"        val 'binary_unquoted' = b deadbeef \n"
"    } \n"
"}";

static void test_registrar(void)
{
    IRegistrar *registrar = NULL;
    HRESULT hr;
    INT count;
    WCHAR *textW = NULL;

    if (!GetProcAddress(GetModuleHandleA("atl.dll"), "AtlAxAttachControl"))
    {
        win_skip("Old versions of atl.dll don't support binary values\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_Registrar, NULL, CLSCTX_INPROC_SERVER, &IID_IRegistrar, (void**)&registrar);
    if (FAILED(hr))
    {
        win_skip("creating IRegistrar failed, hr = 0x%08X\n", hr);
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

        MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, count);
        hr = IRegistrar_StringRegister(registrar, textW);
        ok(hr == S_OK, "StringRegister failed: %08x\n", hr);
        if (FAILED(hr))
        {
            IRegistrar_Release(registrar);
            return;
        }

        lret = RegOpenKeyA(HKEY_CURRENT_USER, "eebf73c4-50fd-478f-bbcf-db212221227a", &key);
        ok(lret == ERROR_SUCCESS, "error %d opening registry key\n", lret);

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_unquoted_hex", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword != 0xA, "unquoted hex is not supposed to be preserved\n");

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_quoted_hex", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword != 0xA, "quoted hex is not supposed to be preserved\n");

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_unquoted_dec", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword == 1, "unquoted dec is not supposed to be %d\n", dword);

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_quoted_dec", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword == 1, "quoted dec is not supposed to be %d\n", dword);

        size = 4;
        lret = RegQueryValueExA(key, "binary_quoted", NULL, NULL, bytes, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueA, failed, error %d\n", lret);
        ok(bytes[0] == 0xde && bytes[1] == 0xad && bytes[2] == 0xbe && bytes[3] == 0xef,
            "binary quoted value was not preserved (it's 0x%02X%02X%02X%02X)\n",
            0xff & bytes[0], 0xff & bytes[1], 0xff & bytes[2], 0xff & bytes[3]);

        size = 4;
        lret = RegQueryValueExA(key, "binary_unquoted", NULL, NULL, bytes, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueA, failed, error %d\n", lret);
        ok(bytes[0] == 0xde && bytes[1] == 0xad && bytes[2] == 0xbe && bytes[3] == 0xef,
            "binary unquoted value was not preserved (it's 0x%02X%02X%02X%02X)\n",
            0xff & bytes[0], 0xff & bytes[1], 0xff & bytes[2], 0xff & bytes[3]);

        hr = IRegistrar_StringUnregister(registrar, textW);
        ok(SUCCEEDED(hr), "IRegistrar_StringUnregister failed, hr = 0x%08X\n", hr);
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
            "CoCreateInstance failed: %08x, expected CLASS_E_NOAGGREGATION\n", hres);
    ok(!unk || unk == (IUnknown*)0xdeadbeef, "unk = %p\n", unk);
}

START_TEST(registrar)
{
    CoInitialize(NULL);

    test_registrar();
    test_aggregation();

    CoUninitialize();
}
