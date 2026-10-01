/*
 * Copyright 2008 James Hawkins
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

#define COBJMACROS

#include <stdio.h>

#include <windows.h>
#include <mscoree.h>
#include <fusion.h>
#include <corerror.h>
#include <strsafe.h>

#include "wine/test.h"

/* ok-like statement which takes two unicode strings or one unicode and one ANSI string as arguments */
static CHAR string1[MAX_PATH];

#define ok_aw(aString, wString) \
    WideCharToMultiByte(CP_ACP, 0, wString, -1, string1, MAX_PATH, NULL, NULL); \
    if (lstrcmpA(string1, aString) != 0) \
        ok(0, "Expected \"%s\", got \"%s\"\n", aString, string1);

static HRESULT (WINAPI *pCreateAssemblyNameObject)(IAssemblyName **ppAssemblyNameObj,
                                                   LPCWSTR szAssemblyName, DWORD dwFlags,
                                                   LPVOID pvReserved);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR szDllName, LPCWSTR szVersion,
                                          LPVOID pvReserved, HMODULE *phModDll);

static BOOL init_functionpointers(void)
{
    HRESULT hr;
    HMODULE hfusion;
    HMODULE hmscoree;

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
        return FALSE;

    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    if (!pLoadLibraryShim)
    {
        FreeLibrary(hmscoree);
        return FALSE;
    }

    hr = pLoadLibraryShim(L"fusion.dll", NULL, NULL, &hfusion);
    if (FAILED(hr))
        return FALSE;

    pCreateAssemblyNameObject = (void *)GetProcAddress(hfusion, "CreateAssemblyNameObject");
    if (!pCreateAssemblyNameObject)
        return FALSE;

    return TRUE;
}

typedef struct _tagASMPROP_RES
{
    HRESULT hr;
    CHAR val[MAX_PATH];
    DWORD size;
} ASMPROP_RES;

static const ASMPROP_RES defaults[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES emptyname[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 2},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES winename[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES vername[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "\x01", 2},
    {S_OK, "\x02", 2},
    {S_OK, "\x03", 2},
    {S_OK, "\x04", 2},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES badvername[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "\x01", 2},
    {S_OK, "\x05", 2},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES neutralname[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 2},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES enname[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "en", 6},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_FALSE, "", MAX_PATH},
    {S_FALSE, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static const ASMPROP_RES pubkeyname[ASM_NAME_MAX_PARAMS] =
{
    {S_OK, "", 0},
    {S_OK, "\x01\x23\x45\x67\x89\x0a\xbc\xde", 8},
    {S_OK, "", 0},
    {S_OK, "wine", 10},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", MAX_PATH},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0},
    {S_OK, "", 0}
};

static inline void to_widechar(LPWSTR dest, LPCSTR src)
{
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, MAX_PATH);
}

static void test_assembly_name_props_line(IAssemblyName *name,
                                          const ASMPROP_RES *vals, int line)
{
    HRESULT hr;
    DWORD i, size;
    WCHAR expect[MAX_PATH];
    WCHAR str[MAX_PATH];

    for (i = 0; i < ASM_NAME_MAX_PARAMS; i++)
    {
        to_widechar(expect, vals[i].val);

        size = MAX_PATH;
        memset( str, 0xcc, sizeof(str) );
        hr = IAssemblyName_GetProperty(name, i, str, &size);

        ok(hr == vals[i].hr ||
           broken(i >= ASM_NAME_CONFIG_MASK && hr == E_INVALIDARG) || /* .NET 1.1 */
           broken(i >= ASM_NAME_FILE_MAJOR_VERSION && hr == E_INVALIDARG), /* .NET 1.0 */
           "%d: prop %ld: Expected %08lx, got %08lx\n", line, i, vals[i].hr, hr);
        if (hr != E_INVALIDARG)
        {
            ok(size == vals[i].size, "%d: prop %ld: Expected %ld, got %ld\n", line, i, vals[i].size, size);
            if (!size)
            {
                ok(str[0] == 0xcccc, "%d: prop %ld: str[0] = %x\n", line, i, str[0]);
            }
            else if (size != MAX_PATH)
            {
                if (i != ASM_NAME_NAME && i != ASM_NAME_CULTURE)
                    ok( !memcmp( vals[i].val, str, size ), "%d: prop %ld: wrong value\n", line, i );
                else
                    ok( !lstrcmpW( expect, str ), "%d: prop %ld: Expected %s, got %s\n",
                        line, i, wine_dbgstr_w(expect), wine_dbgstr_w(str) );
            }

            if (size != 0 && size != MAX_PATH)
            {
                size--;
                hr = IAssemblyName_GetProperty(name, i, str, &size);
                ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER,
                        "%d: prop %ld: Expected STRSAFE_E_INSUFFICIENT_BUFFER, got %08lx\n", line, i, hr);
                ok(size == vals[i].size, "%d: prop %ld: Expected %ld, got %ld\n", line, i, vals[i].size, size);
            }
        }
    }
}

#define test_assembly_name_props(name, vals) \
    test_assembly_name_props_line(name, vals, __LINE__);

static void test_CreateAssemblyNameObject(void)
{
    IAssemblyName *name;
    WCHAR str[MAX_PATH];
    WCHAR namestr[MAX_PATH];
    DWORD size, hi, lo;
    PEKIND arch;
    HRESULT hr;

    /* NULL ppAssemblyNameObj */
    to_widechar(namestr, "wine.dll");
    hr = pCreateAssemblyNameObject(NULL, namestr, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);

    /* NULL szAssemblyName, CANOF_PARSE_DISPLAY_NAME */
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, NULL, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* empty szAssemblyName, CANOF_PARSE_DISPLAY_NAME */
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, L"", CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* check the contents of the AssemblyName for default values */

    /* NULL szAssemblyName */
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, NULL, CANOF_SET_DEFAULT_VALUES, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == E_INVALIDARG), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);

    str[0] = 'a';
    size = MAX_PATH;
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(str[0] == 'a', "got %c\n", str[0]);
    ok(!size, "got %lu\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    if (hr == S_OK)
        win_skip(".NET 1.x doesn't handle ASM_NAME_PROCESSOR_ID_ARRAY"
                 " and ASM_NAME_OSINFO_ARRAY correctly\n");
    else
        test_assembly_name_props(name, defaults);

    IAssemblyName_Release(name);

    /* empty szAssemblyName */
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, L"", CANOF_SET_DEFAULT_VALUES, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);

    size = 0;
    hr = IAssemblyName_GetName(name, &size, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(size == 1, "got %lu\n", size);

    if (0) /* crash */
    {
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, NULL, str);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(!str[0], "got %c\n", str[0]);
    }

    size = 0;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(!str[0], "got %c\n", str[0]);
    ok(size == 1, "got %lu\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(!str[0], "Expected empty name\n");
    ok(size == 1, "Expected 1, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    if (hr == S_OK)
        win_skip(".NET 1.x doesn't handle ASM_NAME_PROCESSOR_ID_ARRAY"
                 " and ASM_NAME_OSINFO_ARRAY correctly\n");
    else
        test_assembly_name_props(name, emptyname);

    IAssemblyName_Release(name);

    /* 'wine' */
    to_widechar(namestr, "wine");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_SET_DEFAULT_VALUES, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = 0;
    hr = IAssemblyName_GetDisplayName(name, NULL, &size, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(size == 5, "got %lu\n", size);

    size = 3;
    hr = IAssemblyName_GetDisplayName(name, NULL, &size, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER || broken(hr == E_INVALIDARG), "got %08lx\n", hr);
    ok(size == 5 || broken(size == 3), "got %lu\n", size);

    size = 3;
    str[0] = 'a';
    hr = IAssemblyName_GetDisplayName(name, str, &size, 0);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(str[0] == 'a', "got %c\n", str[0]);
    ok(size == 5, "got %lu\n", size);

    size = 0;
    str[0] = 'a';
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(str[0] == 'a', "got %c\n", str[0]);
    ok(size == 5, "Wrong size %lu\n", size);

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    size = 0;
    str[0] = 0;
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "got %08lx\n", hr);
    ok(!str[0], "got %c\n", str[0]);
    ok(size == 5, "got %lu\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    if (hr == S_OK)
        win_skip(".NET 1.x doesn't handle ASM_NAME_PROCESSOR_ID_ARRAY"
                 " and ASM_NAME_OSINFO_ARRAY correctly\n");
    else
        test_assembly_name_props(name, winename);

    IAssemblyName_Release(name);

    /* check the contents of the AssemblyName with parsing */

    /* 'wine' */
    to_widechar(namestr, "wine");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, winename);

    IAssemblyName_Release(name);

    /* 'wine, Version=1.2.3.4' */
    to_widechar(namestr, "wine, Version=1.2.3.4");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Version=1.2.3.4", str);
    ok(size == 22, "Expected 22, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(hi == 0x00010002, "Expected 0x00010002, got %08lx\n", hi);
    ok(lo == 0x00030004, "Expected 0x00030004, got %08lx\n", lo);

    test_assembly_name_props(name, vername);

    IAssemblyName_Release(name);

    /* Version isn't of the form 1.x.x.x */
    to_widechar(namestr, "wine, Version=1.5");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Version=1.5", str);
    ok(size == 18, "Expected 18, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0 ||
       broken(hi == 0x10005), /* .NET 1.x */
       "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, badvername);

    IAssemblyName_Release(name);

    /* 'wine, Culture=neutral' */
    to_widechar(namestr, "wine, Culture=neutral");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Culture=neutral", str);
    ok(size == 22, "Expected 22, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, neutralname);

    IAssemblyName_Release(name);

    /* 'wine, Culture=en' */
    to_widechar(namestr, "wine, Culture=en");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Culture=en", str);
    ok(size == 17, "Expected 17, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, enname);

    IAssemblyName_Release(name);

    /* 'wine, PublicKeyToken=01234567890abcde' */
    to_widechar(namestr, "wine, PublicKeyToken=01234567890abcde");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, PublicKeyToken=01234567890abcde", str);
    ok(size == 38, "Expected 38, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, pubkeyname);

    IAssemblyName_Release(name);

    /* Processor architecture tests */
    to_widechar(namestr, "wine, processorArchitecture=x86");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_PROCESSORARCHITECTURE);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

    WideCharToMultiByte(CP_ACP, 0, str, -1, string1, MAX_PATH, NULL, NULL);

    if (lstrcmpA(string1, "wine") == 0)
        win_skip("processorArchitecture not supported on .NET 1.x\n");
    else
    {
        ok_aw("wine, processorArchitecture=x86", str);
        ok(size == 32, "Expected 32, got %ld\n", size);

        size = sizeof(arch);
        hr = IAssemblyName_GetProperty(name, ASM_NAME_ARCHITECTURE, &arch, &size);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(arch == peI386, "Expected peI386, got %d\n", arch);
        ok(size == sizeof(arch), "Wrong size %ld\n", size);

        IAssemblyName_Release(name);

        /* amd64 */
        to_widechar(namestr, "wine, processorArchitecture=AMD64");
        name = NULL;
        hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

        size = MAX_PATH;
        hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_PROCESSORARCHITECTURE);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok_aw("wine, processorArchitecture=AMD64", str);
        ok(size == 34, "Expected 34, got %ld\n", size);

        size = sizeof(arch);
        hr = IAssemblyName_GetProperty(name, ASM_NAME_ARCHITECTURE, &arch, &size);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(arch == peAMD64, "Expected peAMD64, got %d\n", arch);
        ok(size == sizeof(arch), "Wrong size %ld\n", size);

        IAssemblyName_Release(name);

        /* ia64 */
        to_widechar(namestr, "wine, processorArchitecture=IA64");
        name = NULL;
        hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

        size = MAX_PATH;
        hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_PROCESSORARCHITECTURE);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok_aw("wine, processorArchitecture=IA64", str);
        ok(size == 33, "Expected 33, got %ld\n", size);

        size = sizeof(arch);
        hr = IAssemblyName_GetProperty(name, ASM_NAME_ARCHITECTURE, &arch, &size);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(arch == peIA64, "Expected peIA64, got %d\n", arch);
        ok(size == sizeof(arch), "Wrong size %ld\n", size);

        IAssemblyName_Release(name);

        /* msil */
        to_widechar(namestr, "wine, processorArchitecture=MSIL");
        name = NULL;
        hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);

        size = MAX_PATH;
        hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_PROCESSORARCHITECTURE);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok_aw("wine, processorArchitecture=MSIL", str);
        ok(size == 33, "Expected 33, got %ld\n", size);

        size = sizeof(arch);
        hr = IAssemblyName_GetProperty(name, ASM_NAME_ARCHITECTURE, &arch, &size);
        ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
        ok(arch == peMSIL, "Expected peMSIL, got %d\n", arch);
        ok(size == sizeof(arch), "Wrong size %ld\n", size);

        IAssemblyName_Release(name);
    }

    /* Pulling out various different values */
    to_widechar(namestr, "wine, Version=1.2.3.4, Culture=en, PublicKeyToken=1234567890abcdef");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Version=1.2.3.4, Culture=en", str);
    ok(size == 34, "Expected 34, got %ld\n", size);

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Culture=en, PublicKeyToken=1234567890abcdef", str);
    ok(size == 50, "Expected 50, got %ld\n", size);

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine, Version=1.2.3.4, Culture=en, PublicKeyToken=1234567890abcdef", str);
    ok(size == 67, "Expected 67, got %ld\n", size);

    IAssemblyName_Release(name);

    /* invalid property */
    to_widechar(namestr, "wine, BadProp=42");
    name = NULL;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetDisplayName(name, str, &size, ASM_DISPLAYF_FULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    size = MAX_PATH;
    str[0] = '\0';
    hr = IAssemblyName_GetName(name, &size, str);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok_aw("wine", str);
    ok(size == 5, "Expected 5, got %ld\n", size);

    hi = 0xbeefcace;
    lo = 0xcafebabe;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == FUSION_E_INVALID_NAME ||
       broken(hr == S_OK), /* .NET 1.x */
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(hi == 0, "Expected 0, got %08lx\n", hi);
    ok(lo == 0, "Expected 0, got %08lx\n", lo);

    test_assembly_name_props(name, winename);

    IAssemblyName_Release(name);

    /* PublicKeyToken is not 16 chars long */
    to_widechar(namestr, "wine, PublicKeyToken=567890abcdef");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    if (hr == S_OK && name != (IAssemblyName *)0xdeadbeef)
    {
        win_skip(".NET 1.x doesn't check PublicKeyToken correctly\n");
        IAssemblyName_Release(name);
        return;
    }
    ok(hr == FUSION_E_INVALID_NAME,
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* PublicKeyToken contains invalid chars */
    to_widechar(namestr, "wine, PublicKeyToken=1234567890ghijkl");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == FUSION_E_INVALID_NAME,
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* no comma separator */
    to_widechar(namestr, "wine PublicKeyToken=1234567890abcdef");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == FUSION_E_INVALID_NAME,
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);
    if(SUCCEEDED(hr)) IAssemblyName_Release(name);

    /* no '=' */
    to_widechar(namestr, "wine, PublicKeyToken");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == FUSION_E_INVALID_NAME,
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* no value */
    to_widechar(namestr, "wine, PublicKeyToken=");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == FUSION_E_INVALID_NAME,
       "Expected FUSION_E_INVALID_NAME, got %08lx\n", hr);
    ok(name == (IAssemblyName *)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", name);

    /* no spaces */
    to_widechar(namestr, "wine,version=1.0.0.0");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");
    hi = lo = 0xdeadbeef;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(hi == 65536, "Expected 536, got %lu\n", hi);
    ok(lo == 0, "Expected 0, got %lu\n", lo);
    IAssemblyName_Release(name);

    /* quoted values */
    to_widechar(namestr, "wine, version=\"1.0.0.0\",culture=\"en\"");
    name = (IAssemblyName *)0xdeadbeef;
    hr = pCreateAssemblyNameObject(&name, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(name != NULL, "Expected non-NULL name\n");
    hi = lo = 0xdeadbeef;
    hr = IAssemblyName_GetVersion(name, &hi, &lo);
    ok(hr == S_OK, "Expected S_OK, got %08lx\n", hr);
    ok(hi == 65536, "Expected 65536, got %lu\n", hi);
    ok(lo == 0, "Expected 0, got %lu\n", lo);
    IAssemblyName_Release(name);
}

static void test_IAssemblyName_IsEqual(void)
{
    HRESULT hr;
    IAssemblyName *name1, *name2;

    hr = pCreateAssemblyNameObject( &name1, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = pCreateAssemblyNameObject( &name2, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    if (0) { /* crashes on some old version */
    hr = IAssemblyName_IsEqual( name1, NULL, 0 );
    ok( hr == S_FALSE, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, NULL, ASM_CMPF_IL_ALL );
    ok( hr == S_FALSE, "got %08lx\n", hr );
    }

    hr = IAssemblyName_IsEqual( name1, name1, ASM_CMPF_IL_ALL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, name2, ASM_CMPF_IL_ALL );
    ok( hr == S_OK, "got %08lx\n", hr );

    IAssemblyName_Release( name2 );
    hr = pCreateAssemblyNameObject( &name2, L"wine,version=1.0.0.0", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, name2, ASM_CMPF_IL_ALL );
    ok( hr == S_OK, "got %08lx\n", hr );

    IAssemblyName_Release( name2 );
    hr = pCreateAssemblyNameObject( &name2, L"wine,version=1.0.0.0,culture=neutral",
                                    CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, name2, ASM_CMPF_IL_ALL );
    ok( hr == S_OK, "got %08lx\n", hr );

    IAssemblyName_Release( name1 );
    hr = pCreateAssemblyNameObject( &name1, L"wine,version=1.0.0.0,culture=en",
                                    CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, name2, ASM_CMPF_IL_ALL );
    ok( hr == S_FALSE, "got %08lx\n", hr );

    IAssemblyName_Release( name1 );
    hr = pCreateAssemblyNameObject( &name1, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    IAssemblyName_Release( name2 );
    hr = pCreateAssemblyNameObject( &name2, L"wine,version=1.0.0.0,publicKeyToken=1234567890abcdef",
                                    CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    hr = IAssemblyName_IsEqual( name1, name2, ASM_CMPF_IL_ALL );
    ok( hr == S_OK, "got %08lx\n", hr );

    IAssemblyName_Release( name1 );
    IAssemblyName_Release( name2 );
}

START_TEST(asmname)
{
    if (!init_functionpointers())
    {
        win_skip("fusion.dll not available\n");
        return;
    }

    test_CreateAssemblyNameObject();
    test_IAssemblyName_IsEqual();
}
