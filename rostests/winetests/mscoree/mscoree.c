/*
 * Copyright 2010 Louis Lenders
 * Copyright 2011 André Hentschel
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

#include "corerror.h"
#include "mscoree.h"
#include "shlwapi.h"
#include "wine/test.h"

static HMODULE hmscoree;

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetCORSystemDirectory)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetRequestedRuntimeInfo)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD, LPWSTR, DWORD, DWORD*, LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR, LPCWSTR, LPVOID, HMODULE*);
static HRESULT (WINAPI *pCreateConfigStream)(LPCWSTR, IStream**);

static BOOL init_functionpointers(void)
{
    hmscoree = LoadLibraryA("mscoree.dll");

    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");
    pGetCORSystemDirectory = (void *)GetProcAddress(hmscoree, "GetCORSystemDirectory");
    pGetRequestedRuntimeInfo = (void *)GetProcAddress(hmscoree, "GetRequestedRuntimeInfo");
    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    pCreateConfigStream = (void *)GetProcAddress(hmscoree, "CreateConfigStream");

    if (!pGetCORVersion || !pGetCORSystemDirectory || !pGetRequestedRuntimeInfo || !pLoadLibraryShim)
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

static void test_versioninfo(void)
{
    const WCHAR v9_0[] = {'v','9','.','0','.','3','0','3','1','9',0};
    const WCHAR v2_0cap[] = {'V','2','.','0','.','5','0','7','2','7',0};
    const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};
    const WCHAR v2_0_0[] = {'v','2','.','0','.','0',0};
    const WCHAR v1_1[] = {'v','1','.','1','.','4','3','2','2',0};
    const WCHAR v1_1_0[] = {'v','1','.','1','.','0',0};

    WCHAR version[MAX_PATH];
    WCHAR path[MAX_PATH];
    DWORD size, path_len;
    HRESULT hr;

    if (0)  /* crashes on <= w2k3 */
    {
        hr = pGetCORVersion(NULL, MAX_PATH, &size);
        ok(hr == E_POINTER,"GetCORVersion returned %08x\n", hr);
    }

    hr =  pGetCORVersion(version, 1, &size);
    if (hr == CLR_E_SHIM_RUNTIME)
    {
        /* FIXME: Get Mono packaged properly so we can fail here. */
        todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),"GetCORVersion returned %08x\n", hr);
        skip("No .NET runtimes are installed\n");
        return;
    }

    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),"GetCORVersion returned %08x\n", hr);

    hr =  pGetCORVersion(version, MAX_PATH, &size);
    ok(hr == S_OK,"GetCORVersion returned %08x\n", hr);

    trace("latest installed .net runtime: %s\n", wine_dbgstr_w(version));

    hr = pGetCORSystemDirectory(path, MAX_PATH , &size);
    ok(hr == S_OK, "GetCORSystemDirectory returned %08x\n", hr);
    /* size includes terminating null-character */
    ok(size == (lstrlenW(path) + 1),"size is %d instead of %d\n", size, (lstrlenW(path) + 1));

    path_len = size;

    hr = pGetCORSystemDirectory(path, path_len-1 , &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetCORSystemDirectory returned %08x\n", hr);

    if (0)  /* crashes on <= w2k3 */
    {
        hr = pGetCORSystemDirectory(NULL, MAX_PATH , &size);
        ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetCORSystemDirectory returned %08x\n", hr);
    }

    hr = pGetCORSystemDirectory(path, MAX_PATH , NULL);
    ok(hr == E_POINTER,"GetCORSystemDirectory returned %08x\n", hr);

    trace("latest installed .net installed in directory: %s\n", wine_dbgstr_w(path));

    /* test GetRequestedRuntimeInfo, first get info about different versions of runtime */
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);

    if(hr == CLR_E_SHIM_RUNTIME) return; /* skipping rest of tests on win2k as .net 2.0 not installed */

    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));

    hr = pGetRequestedRuntimeInfo( NULL, v1_1, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK || hr == CLR_E_SHIM_RUNTIME /*v1_1 not installed*/, "GetRequestedRuntimeInfo returned %08x\n", hr);
    if(hr == S_OK)
        trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));
    /* version number NULL not allowed without RUNTIME_INFO_UPGRADE_VERSION flag */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08x\n", hr);
    /* with RUNTIME_INFO_UPGRADE_VERSION flag and version number NULL, latest installed version is returned */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);

    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, 1, &path_len, version, MAX_PATH, &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetRequestedRuntimeInfo returned %08x\n", hr);

    /* if one of the buffers is NULL, the other one is still happily filled */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, NULL, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!winetest_strcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));
    /* With NULL-pointer for bufferlength, the buffer itself still gets filled with correct string */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!winetest_strcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0cap, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!winetest_strcmpW(version, v2_0cap), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0cap));

    /* Invalid Version and RUNTIME_INFO_UPGRADE_VERSION flag*/
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK  || hr == CLR_E_SHIM_RUNTIME , "GetRequestedRuntimeInfo returned %08x\n", hr);
    if(hr == S_OK)
    {
        /* .NET 1.1 may not be installed. */
        ok(!winetest_strcmpW(version, v1_1) || !winetest_strcmpW(version, v2_0),
           "version is %s , expected %s or %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v1_1), wine_dbgstr_w(v2_0));

    }

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v9_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08x\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08x\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!winetest_strcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08x\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!winetest_strcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));
}

static void test_loadlibraryshim(void)
{
    const WCHAR v4_0[] = {'v','4','.','0','.','3','0','3','1','9',0};
    const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};
    const WCHAR v1_1[] = {'v','1','.','1','.','4','3','2','2',0};
    const WCHAR vbogus[] = {'v','b','o','g','u','s',0};
    const WCHAR fusion[] = {'f','u','s','i','o','n',0};
    const WCHAR fusiondll[] = {'f','u','s','i','o','n','.','d','l','l',0};
    const WCHAR nosuchdll[] = {'j','n','v','n','l','.','d','l','l',0};
    const WCHAR gdidll[] = {'g','d','i','3','2','.','d','l','l',0};
    HRESULT hr;
    const WCHAR *latest = NULL;
    CHAR latestA[MAX_PATH];
    HMODULE hdll;
    CHAR dllpath[MAX_PATH];

    hr = pLoadLibraryShim(fusion, v1_1, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v1_1;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine ok(StrStrIA(dllpath, "v1.1.4322") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v2_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v2_0;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine ok(StrStrIA(dllpath, "v2.0.50727") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v4_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        /* LoadLibraryShim with a NULL version prefers 2.0 and earlier */
        if (!latest)
            latest = v4_0;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine ok(StrStrIA(dllpath, "v4.0.30319") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, vbogus, NULL, &hdll);
    todo_wine ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    WideCharToMultiByte(CP_ACP, 0, latest, -1, latestA, MAX_PATH, NULL, NULL);

    hr = pLoadLibraryShim(fusion, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine ok(StrStrIA(dllpath, latestA) != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusiondll, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine ok(StrStrIA(dllpath, latestA) != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(nosuchdll, latest, NULL, &hdll);
    ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    hr = pLoadLibraryShim(gdidll, latest, NULL, &hdll);
    todo_wine ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);
}

static const char xmldata[] =
    "<?xml version=\"1.0\" ?>\n"
    "<!DOCTYPE Config>\n"
    "<Configuration>\n"
    "  <Name>Test</Name>\n"
    "  <Value>1234</Value>\n"
    "</Configuration>";

static void create_xml_file(LPCWSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hfile = CreateFileW(filename, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "File creation failed\n");
    WriteFile(hfile, xmldata, sizeof(xmldata) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hfile);
}

static void test_createconfigstream(void)
{
    IStream *stream = NULL;
    WCHAR file[] = {'c', 'o', 'n', 'f', '.', 'x', 'm', 'l', 0};
    WCHAR nonexistent[] = {'n', 'o', 'n', 'e', 'x', 'i', 's', 't', '.', 'x', 'm', 'l', 0};
    WCHAR path[MAX_PATH];
    HRESULT hr;
    char buffer[256] = {0};

    if (!pCreateConfigStream)
    {
        win_skip("CreateConfigStream not available\n");
        return;
    }

    create_xml_file(file);
    GetFullPathNameW(file, MAX_PATH, path, NULL);

    hr = pCreateConfigStream(NULL, &stream);
    todo_wine ok(hr == E_FAIL ||
                 broken(hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND)) || /* some WinXP, Win2K3 and Win7 */
                 broken(hr == S_OK && !stream), /* some Win2K3 */
                 "CreateConfigStream returned %x\n", hr);

    hr = pCreateConfigStream(path, NULL);
    todo_wine ok(hr == COR_E_NULLREFERENCE, "CreateConfigStream returned %x\n", hr);

    hr = pCreateConfigStream(NULL, NULL);
    todo_wine ok(hr == COR_E_NULLREFERENCE, "CreateConfigStream returned %x\n", hr);

    hr = pCreateConfigStream(nonexistent, &stream);
    todo_wine ok(hr == COR_E_FILENOTFOUND, "CreateConfigStream returned %x\n", hr);
    ok(stream == NULL, "Expected stream to be NULL\n");

    hr = pCreateConfigStream(path, &stream);
    todo_wine ok(hr == S_OK, "CreateConfigStream failed, hr=%x\n", hr);
    todo_wine ok(stream != NULL, "Expected non-NULL stream\n");

    if (stream)
    {
        DWORD count;
        LARGE_INTEGER pos;
        ULARGE_INTEGER size;
        IStream *stream2 = NULL;

        hr = IStream_Read(stream, buffer, strlen(xmldata), &count);
        ok(hr == S_OK, "IStream_Read failed, hr=%x\n", hr);
        ok(count == strlen(xmldata), "wrong count: %u\n", count);
        ok(!strcmp(buffer, xmldata), "Strings do not match\n");

        hr = IStream_Write(stream, xmldata, strlen(xmldata), &count);
        ok(hr == E_FAIL, "IStream_Write returned hr=%x\n", hr);

        pos.QuadPart = strlen(xmldata);
        hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == E_NOTIMPL, "IStream_Seek returned hr=%x\n", hr);

        size.QuadPart = strlen(xmldata);
        hr = IStream_SetSize(stream, size);
        ok(hr == E_NOTIMPL, "IStream_SetSize returned hr=%x\n", hr);

        hr = IStream_Clone(stream, &stream2);
        ok(hr == E_NOTIMPL, "IStream_Clone returned hr=%x\n", hr);

        hr = IStream_Commit(stream, STGC_DEFAULT);
        ok(hr == E_NOTIMPL, "IStream_Commit returned hr=%x\n", hr);

        hr = IStream_Revert(stream);
        ok(hr == E_NOTIMPL, "IStream_Revert returned hr=%x\n", hr);

        hr = IStream_Release(stream);
        ok(hr == S_OK, "IStream_Release returned hr=%x\n", hr);
    }
    DeleteFileW(file);
}

START_TEST(mscoree)
{
    if (!init_functionpointers())
        return;

    test_versioninfo();
    test_loadlibraryshim();
    test_createconfigstream();

    FreeLibrary(hmscoree);
}
