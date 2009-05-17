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
#include <shlwapi.h>
#include <mscoree.h>
#include <fusion.h>
#include <corerror.h>

#include "wine/test.h"
#include "wine/list.h"

static HRESULT (WINAPI *pCreateAssemblyEnum)(IAssemblyEnum **pEnum,
                                             IUnknown *pUnkReserved,
                                             IAssemblyName *pName,
                                             DWORD dwFlags, LPVOID pvReserved);
static HRESULT (WINAPI *pCreateAssemblyNameObject)(LPASSEMBLYNAME *ppAssemblyNameObj,
                                                   LPCWSTR szAssemblyName, DWORD dwFlags,
                                                   LPVOID pvReserved);
static HRESULT (WINAPI *pGetCachePath)(ASM_CACHE_FLAGS dwCacheFlags,
                                       LPWSTR pwzCachePath, PDWORD pcchPath);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR szDllName, LPCWSTR szVersion,
                                          LPVOID pvReserved, HMODULE *phModDll);

static BOOL init_functionpointers(void)
{
    HRESULT hr;
    HMODULE hfusion;
    HMODULE hmscoree;

    static const WCHAR szFusion[] = {'f','u','s','i','o','n','.','d','l','l',0};

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    if (!pLoadLibraryShim)
    {
        win_skip("LoadLibraryShim not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    hr = pLoadLibraryShim(szFusion, NULL, NULL, &hfusion);
    if (FAILED(hr))
    {
        win_skip("fusion.dll not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    pCreateAssemblyEnum = (void *)GetProcAddress(hfusion, "CreateAssemblyEnum");
    pCreateAssemblyNameObject = (void *)GetProcAddress(hfusion, "CreateAssemblyNameObject");
    pGetCachePath = (void *)GetProcAddress(hfusion, "GetCachePath");

    if (!pCreateAssemblyEnum ||
        !pCreateAssemblyNameObject || !pGetCachePath)
    {
        win_skip("fusion.dll not implemented\n");
        return FALSE;
    }

    FreeLibrary(hmscoree);
    return TRUE;
}

static inline void to_widechar(LPWSTR dest, LPCSTR src)
{
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, MAX_PATH);
}

static inline void to_multibyte(LPSTR dest, LPWSTR src)
{
    WideCharToMultiByte(CP_ACP, 0, src, -1, dest, MAX_PATH, NULL, NULL);
}

static BOOL create_full_path(LPCSTR path)
{
    LPSTR new_path;
    BOOL ret = TRUE;
    int len;

    new_path = HeapAlloc(GetProcessHeap(), 0, lstrlenA(path) + 1);
    if (!new_path)
        return FALSE;

    lstrcpyA(new_path, path);

    while ((len = lstrlenA(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectoryA(new_path, NULL))
    {
        LPSTR slash;
        DWORD last_error = GetLastError();

        if(last_error == ERROR_ALREADY_EXISTS)
            break;

        if(last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if(!(slash = strrchr(new_path, '\\')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if(!create_full_path(new_path))
        {
            ret = FALSE;
            break;
        }

        new_path[len] = '\\';
    }

    HeapFree(GetProcessHeap(), 0, new_path);
    return ret;
}

static void create_file_data(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, data, strlen(data), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define create_file(name, size) create_file_data(name, name, size)

static void test_CreateAssemblyEnum(void)
{
    HRESULT hr;
    WCHAR namestr[MAX_PATH];
    IAssemblyEnum *asmenum;
    IAssemblyName *asmname;

    to_widechar(namestr, "wine");
    asmname = NULL;
    hr = pCreateAssemblyNameObject(&asmname, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmname != NULL, "Expected non-NULL asmname\n");

    /* pEnum is NULL */
    if (0)
    {
        /* Crashes on .NET 1.x */
        hr = pCreateAssemblyEnum(NULL, NULL, asmname, ASM_CACHE_GAC, NULL);
        ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    }

    /* pName is NULL */
    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, NULL, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    IAssemblyEnum_Release(asmenum);

    /* dwFlags is ASM_CACHE_ROOT */
    asmenum = (IAssemblyEnum *)0xdeadbeef;
    hr = pCreateAssemblyEnum(&asmenum, NULL, NULL, ASM_CACHE_ROOT, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok(asmenum == (IAssemblyEnum *)0xdeadbeef,
       "Expected asmenum to be unchanged, got %p\n", asmenum);

    /* invalid dwFlags */
    asmenum = (IAssemblyEnum *)0xdeadbeef;
    hr = pCreateAssemblyEnum(&asmenum, NULL, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok(asmenum == (IAssemblyEnum *)0xdeadbeef,
       "Expected asmenum to be unchanged, got %p\n", asmenum);
}

typedef struct _tagASMNAME
{
    struct list entry;
    LPSTR data;
} ASMNAME;

static BOOL enum_gac_assemblies(struct list *assemblies, int depth, LPSTR path)
{
    WIN32_FIND_DATAA ffd;
    CHAR buf[MAX_PATH];
    CHAR disp[MAX_PATH];
    ASMNAME *name;
    HANDLE hfind;
    LPSTR ptr;

    static CHAR parent[MAX_PATH];

    sprintf(buf, "%s\\*", path);
    hfind = FindFirstFileA(buf, &ffd);
    if (hfind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if (!lstrcmpA(ffd.cFileName, ".") || !lstrcmpA(ffd.cFileName, ".."))
            continue;

        if (depth == 0)
        {
            lstrcpyA(parent, ffd.cFileName);
        }
        else if (depth == 1)
        {
            char culture[MAX_PATH];
            char dll[MAX_PATH], exe[MAX_PATH];

            /* Directories with no dll or exe will not be enumerated */
            sprintf(dll, "%s\\%s\\%s.dll", path, ffd.cFileName, parent);
            sprintf(exe, "%s\\%s\\%s.exe", path, ffd.cFileName, parent);
            if (GetFileAttributesA(dll) == INVALID_FILE_ATTRIBUTES &&
                GetFileAttributesA(exe) == INVALID_FILE_ATTRIBUTES)
                continue;

            ptr = strstr(ffd.cFileName, "_");
            *ptr = '\0';
            ptr++;

            if (*ptr != '_')
            {
                lstrcpyA(culture, ptr);
                *strstr(culture, "_") = '\0';
            }
            else
                lstrcpyA(culture, "neutral");

            ptr = strchr(ptr, '_');
            ptr++;
            sprintf(buf, ", Version=%s, Culture=%s, PublicKeyToken=%s",
                    ffd.cFileName, culture, ptr);
            lstrcpyA(disp, parent);
            lstrcatA(disp, buf);

            name = HeapAlloc(GetProcessHeap(), 0, sizeof(ASMNAME));
            name->data = HeapAlloc(GetProcessHeap(), 0, lstrlenA(disp) + 1);
            lstrcpyA(name->data, disp);
            list_add_tail(assemblies, &name->entry);

            continue;
        }

        sprintf(buf, "%s\\%s", path, ffd.cFileName);
        enum_gac_assemblies(assemblies, depth + 1, buf);
    } while (FindNextFileA(hfind, &ffd) != 0);

    FindClose(hfind);
    return TRUE;
}

static void test_enumerate(void)
{
    struct list assemblies = LIST_INIT(assemblies);
    struct list *item, *cursor;
    IAssemblyEnum *asmenum;
    IAssemblyName *next;
    WCHAR buf[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR disp[MAX_PATH];
    HRESULT hr;
    BOOL found;
    DWORD size;

    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC, buf, &size);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    to_multibyte(path, buf);
    lstrcatA(path, "_32");
    enum_gac_assemblies(&assemblies, 0, path);

    to_multibyte(path, buf);
    lstrcatA(path, "_64");
    enum_gac_assemblies(&assemblies, 0, path);

    to_multibyte(path, buf);
    lstrcatA(path, "_MSIL");
    enum_gac_assemblies(&assemblies, 0, path);

    to_multibyte(path, buf);
    enum_gac_assemblies(&assemblies, 0, path);

    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, NULL, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    while (IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0) == S_OK)
    {
        size = MAX_PATH;
        IAssemblyName_GetDisplayName(next, buf, &size, 0);
        to_multibyte(disp, buf);

        found = FALSE;
        LIST_FOR_EACH_SAFE(item, cursor, &assemblies)
        {
            ASMNAME *asmname = LIST_ENTRY(item, ASMNAME, entry);

            if (!lstrcmpA(asmname->data, disp))
            {
                found = TRUE;

                list_remove(&asmname->entry);
                HeapFree(GetProcessHeap(), 0, asmname->data);
                HeapFree(GetProcessHeap(), 0, asmname);
                break;
            }
        }

        ok(found, "Extra assembly enumerated: %s\n", disp);
        IAssemblyName_Release(next);
    }

    /* enumeration is exhausted */
    next = (IAssemblyName *)0xdeadbeef;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(next == (IAssemblyName *)0xdeadbeef,
       "Expected next to be unchanged, got %p\n", next);

    LIST_FOR_EACH_SAFE(item, cursor, &assemblies)
    {
        ASMNAME *asmname = LIST_ENTRY(item, ASMNAME, entry);

        ok(FALSE, "Assembly not enumerated: %s\n", asmname->data);

        list_remove(&asmname->entry);
        HeapFree(GetProcessHeap(), 0, asmname->data);
        HeapFree(GetProcessHeap(), 0, asmname);
    }

    IAssemblyEnum_Release(asmenum);
}

static void test_enumerate_name(void)
{
    IAssemblyEnum *asmenum;
    IAssemblyName *asmname, *next;
    WCHAR buf[MAX_PATH];
    CHAR gac[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR disp[MAX_PATH];
    WCHAR namestr[MAX_PATH];
    CHAR exp[6][MAX_PATH];
    HRESULT hr;
    DWORD size;

    lstrcpyA(exp[0], "wine, Version=1.0.0.0, Culture=neutral, PublicKeyToken=16a3fcd171e93a8d");
    lstrcpyA(exp[1], "wine, Version=1.0.1.2, Culture=neutral, PublicKeyToken=123456789abcdef0");
    lstrcpyA(exp[2], "wine, Version=1.0.1.2, Culture=neutral, PublicKeyToken=16a3fcd171e93a8d");
    lstrcpyA(exp[3], "Wine, Version=1.0.0.0, Culture=neutral, PublicKeyToken=16a3fcd171e93a8d");
    lstrcpyA(exp[4], "Wine, Version=1.0.1.2, Culture=neutral, PublicKeyToken=123456789abcdef0");
    lstrcpyA(exp[5], "Wine, Version=1.0.1.2, Culture=neutral, PublicKeyToken=16a3fcd171e93a8d");

    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC, buf, &size);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    to_multibyte(gac, buf);
    create_full_path(gac);

    sprintf(path, "%s\\Wine", gac);
    CreateDirectoryA(path, NULL);

    sprintf(path, "%s\\Wine\\1.0.0.0__16a3fcd171e93a8d", gac);
    CreateDirectoryA(path, NULL);

    lstrcatA(path, "\\Wine.dll");
    create_file(path, 100);

    sprintf(path, "%s\\Wine\\1.0.1.2__16a3fcd171e93a8d", gac);
    CreateDirectoryA(path, NULL);

    lstrcatA(path, "\\Wine.dll");
    create_file(path, 100);

    sprintf(path, "%s\\Wine\\1.0.1.2__123456789abcdef0", gac);
    CreateDirectoryA(path, NULL);

    lstrcatA(path, "\\Wine.dll");
    create_file(path, 100);

    /* test case sensitivity */
    to_widechar(namestr, "wine");
    asmname = NULL;
    hr = pCreateAssemblyNameObject(&asmname, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmname != NULL, "Expected non-NULL asmname\n");

    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, asmname, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[0]),
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[0], exp[1], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[1]) ||
       !lstrcmpA(disp, exp[2]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[1], exp[2], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[2]) ||
       !lstrcmpA(disp, exp[1]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[2], exp[1], disp);

    IAssemblyName_Release(next);

    next = (IAssemblyName *)0xdeadbeef;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(next == (IAssemblyName *)0xdeadbeef,
       "Expected next to be unchanged, got %p\n", next);

    IAssemblyEnum_Release(asmenum);
    IAssemblyName_Release(asmname);

    /* only Version */
    to_widechar(namestr, "Wine, Version=1.0.1.2");
    asmname = NULL;
    hr = pCreateAssemblyNameObject(&asmname, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmname != NULL, "Expected non-NULL asmname\n");

    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, asmname, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[4]) ||
       !lstrcmpA(disp, exp[5]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[4], exp[5], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[5]) ||
       !lstrcmpA(disp, exp[4]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[5], exp[4], disp);

    IAssemblyName_Release(next);

    next = (IAssemblyName *)0xdeadbeef;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(next == (IAssemblyName *)0xdeadbeef,
       "Expected next to be unchanged, got %p\n", next);

    IAssemblyEnum_Release(asmenum);
    IAssemblyName_Release(asmname);

    /* only PublicKeyToken */
    to_widechar(namestr, "Wine, PublicKeyToken=16a3fcd171e93a8d");
    asmname = NULL;
    hr = pCreateAssemblyNameObject(&asmname, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmname != NULL, "Expected non-NULL asmname\n");

    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, asmname, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[3]), "Expected \"%s\", got \"%s\"\n", exp[3], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[5]), "Expected \"%s\", got \"%s\"\n", exp[5], disp);

    IAssemblyName_Release(next);

    next = (IAssemblyName *)0xdeadbeef;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(next == (IAssemblyName *)0xdeadbeef,
       "Expected next to be unchanged, got %p\n", next);

    IAssemblyEnum_Release(asmenum);
    IAssemblyName_Release(asmname);

    /* only Culture */
    to_widechar(namestr, "wine, Culture=neutral");
    asmname = NULL;
    hr = pCreateAssemblyNameObject(&asmname, namestr, CANOF_PARSE_DISPLAY_NAME, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmname != NULL, "Expected non-NULL asmname\n");

    asmenum = NULL;
    hr = pCreateAssemblyEnum(&asmenum, NULL, asmname, ASM_CACHE_GAC, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(asmenum != NULL, "Expected non-NULL asmenum\n");

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[0]), "Expected \"%s\", got \"%s\"\n", exp[0], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[1]) ||
       !lstrcmpA(disp, exp[2]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[1], exp[2], disp);

    IAssemblyName_Release(next);

    next = NULL;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(next != NULL, "Expected non-NULL next\n");

    size = MAX_PATH;
    hr = IAssemblyName_GetDisplayName(next, buf, &size, 0);
    to_multibyte(disp, buf);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpA(disp, exp[2]) ||
       !lstrcmpA(disp, exp[1]), /* Win98 */
       "Expected \"%s\" or \"%s\", got \"%s\"\n", exp[2], exp[1], disp);

    IAssemblyName_Release(next);

    next = (IAssemblyName *)0xdeadbeef;
    hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(next == (IAssemblyName *)0xdeadbeef,
       "Expected next to be unchanged, got %p\n", next);

    IAssemblyEnum_Release(asmenum);
    IAssemblyName_Release(asmname);

    sprintf(path, "%s\\Wine\\1.0.0.0__16a3fcd171e93a8d\\Wine.dll", gac);
    DeleteFileA(path);
    sprintf(path, "%s\\Wine\\1.0.1.2__16a3fcd171e93a8d\\Wine.dll", gac);
    DeleteFileA(path);
    sprintf(path, "%s\\Wine\\1.0.1.2__123456789abcdef0\\Wine.dll", gac);
    DeleteFileA(path);
    sprintf(path, "%s\\Wine\\1.0.0.0__16a3fcd171e93a8d", gac);
    RemoveDirectoryA(path);
    sprintf(path, "%s\\Wine\\1.0.1.2__16a3fcd171e93a8d", gac);
    RemoveDirectoryA(path);
    sprintf(path, "%s\\Wine\\1.0.1.2__123456789abcdef0", gac);
    RemoveDirectoryA(path);
    sprintf(path, "%s\\Wine", gac);
    RemoveDirectoryA(path);
}

START_TEST(asmenum)
{
    if (!init_functionpointers())
        return;

    test_CreateAssemblyEnum();
    test_enumerate();
    test_enumerate_name();
}
