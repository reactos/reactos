/*
 *    MSDMO tests
 *
 * Copyright 2014 Nikolay Sivov for CodeWeavers
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

#include <stdio.h>
#define COBJMACROS
#include "dmo.h"
#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
static const GUID GUID_unknowndmo = {0x14d99047,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_unknowncategory = {0x14d99048,0x441f,0x4cd3,{0xbc,0xa8,0x3e,0x67,0x99,0xaf,0x34,0x75}};
static const GUID GUID_wmp1 = {0x13a7995e,0x7d8f,0x45b4,{0x9c,0x77,0x81,0x92,0x65,0x22,0x57,0x63}};

static const char *guid_to_string(const GUID *guid)
{
    static char buffer[50];
    sprintf(buffer, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return buffer;
}

static void test_DMOUnregister(void)
{
    static char buffer[200];
    HRESULT hr;

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_unknowncategory);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    /* can't register for all categories */
    hr = DMORegister(L"testdmo", &GUID_unknowndmo, &GUID_NULL, 0, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DMORegister(L"testdmo", &GUID_unknowndmo, &GUID_unknowncategory, 0, 0, NULL, 0, NULL);
    if (hr != S_OK) {
        win_skip("Failed to register DMO. Probably user doesn't have persmissions to do so.\n");
        return;
    }

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_NULL);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);

    /* clean up category since Windows doesn't */
    sprintf(buffer, "DirectShow\\MediaObjects\\Categories\\%s", guid_to_string(&GUID_unknowncategory));
    RegDeleteKeyA(HKEY_CLASSES_ROOT, buffer);
}

static void test_DMOGetName(void)
{
    WCHAR name[80];
    HRESULT hr;

    hr = DMOGetName(&GUID_unknowndmo, NULL);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);

    /* no such DMO */
    name[0] = 'a';
    hr = DMOGetName(&GUID_wmp1, name);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);
    ok(name[0] == 'a', "got %x\n", name[0]);
}

static void test_DMOEnum(void)
{
    static const DMO_PARTIAL_MEDIATYPE input_type = {{0x1111}, {0x2222}};
    static const DMO_PARTIAL_MEDIATYPE wrong_type = {{0x3333}, {0x4444}};

    IEnumDMO *enum_dmo;
    HRESULT hr;
    CLSID clsid;
    WCHAR *name;
    DWORD count;

    hr = DMOEnum(&GUID_unknowncategory, 0, 0, NULL, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "DMOEnum() failed with %#lx\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, &name, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, &name, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);
    ok(count == 0, "expected 0, got %ld\n", count);

    hr = IEnumDMO_Next(enum_dmo, 2, NULL, &name, &count);
    ok(hr == E_POINTER, "expected S_FALSE, got %#lx\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 2, &clsid, NULL, &count);
    ok(hr == S_FALSE, "expected S_FALSE, got %#lx\n", hr);
    ok(count == 0, "expected 0, got %ld\n", count);

    IEnumDMO_Release(enum_dmo);

    hr = DMORegister(L"testdmo", &GUID_unknowndmo, &GUID_unknowncategory, 0, 1, &input_type, 0, NULL);
    if (hr != S_OK)
        return;

    hr = DMOEnum(&GUID_unknowncategory, 0, 0, NULL, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&clsid, &GUID_unknowndmo), "Got clsid %s.\n", debugstr_guid(&clsid));
    ok(!wcscmp(name, L"testdmo"), "Got name %s.\n", debugstr_w(name));

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumDMO_Release(enum_dmo);

    hr = DMOEnum(&GUID_unknowncategory, 0, 1, &input_type, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(IsEqualGUID(&clsid, &GUID_unknowndmo), "Got clsid %s.\n", debugstr_guid(&clsid));
    ok(!wcscmp(name, L"testdmo"), "Got name %s.\n", debugstr_w(name));

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumDMO_Release(enum_dmo);

    hr = DMOEnum(&GUID_unknowncategory, 0, 1, &wrong_type, 0, NULL, &enum_dmo);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IEnumDMO_Next(enum_dmo, 1, &clsid, &name, NULL);
    ok(hr == S_FALSE, "Got hr %#lx.\n", hr);

    IEnumDMO_Release(enum_dmo);

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_unknowncategory);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

static void test_DMOGetTypes(void)
{
    static const DMO_PARTIAL_MEDIATYPE input_types[] =
    {
        {{0x1111}, {0x2222}},
        {{0x1111}, {0x3333}},
    };
    ULONG input_count, output_count;
    DMO_PARTIAL_MEDIATYPE types[3];
    HRESULT hr;

    hr = DMOGetTypes(&GUID_unknowndmo, 0, &input_count, types, 0, &output_count, NULL);
    ok(hr == E_FAIL, "Got hr %#lx.\n", hr);

    hr = DMORegister(L"testdmo", &GUID_unknowndmo, &GUID_unknowncategory, 0,
            ARRAY_SIZE(input_types), input_types, 0, NULL);
    if (hr != S_OK)
        return;

    hr = DMOGetTypes(&GUID_unknowndmo, 0, &input_count, types, 0, &output_count, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!input_count, "Got input count %lu.\n", input_count);
    ok(!output_count, "Got output count %lu.\n", output_count);

    memset(types, 0, sizeof(types));
    hr = DMOGetTypes(&GUID_unknowndmo, 1, &input_count, types, 0, &output_count, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(input_count == 1, "Got input count %lu.\n", input_count);
    ok(!output_count, "Got output count %lu.\n", output_count);
    todo_wine ok(!memcmp(types, input_types, sizeof(DMO_PARTIAL_MEDIATYPE)), "Types didn't match.\n");

    memset(types, 0, sizeof(types));
    hr = DMOGetTypes(&GUID_unknowndmo, 2, &input_count, types, 0, &output_count, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(input_count == 2, "Got input count %lu.\n", input_count);
    ok(!output_count, "Got output count %lu.\n", output_count);
    ok(!memcmp(types, input_types, 2 * sizeof(DMO_PARTIAL_MEDIATYPE)), "Types didn't match.\n");

    memset(types, 0, sizeof(types));
    hr = DMOGetTypes(&GUID_unknowndmo, 2, &input_count, types, 0, &output_count, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(input_count == 2, "Got input count %lu.\n", input_count);
    ok(!output_count, "Got output count %lu.\n", output_count);
    ok(!memcmp(types, input_types, 2 * sizeof(DMO_PARTIAL_MEDIATYPE)), "Types didn't match.\n");

    hr = DMOUnregister(&GUID_unknowndmo, &GUID_unknowncategory);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

START_TEST(msdmo)
{
    test_DMOUnregister();
    test_DMOGetName();
    test_DMOEnum();
    test_DMOGetTypes();
}
