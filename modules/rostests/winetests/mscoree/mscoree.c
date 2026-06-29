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

#include <stdio.h>

#define COBJMACROS

#include "corerror.h"
#include "mscoree.h"
#include "metahost.h"
#include "shlwapi.h"
#include "initguid.h"
#include "wine/test.h"

#if !defined(__i386__) && !defined(__x86_64__)
static int has_mono = 0;
#else
static int has_mono = 1;
#endif

DEFINE_GUID(IID__AppDomain, 0x05f696dc,0x2b29,0x3663,0xad,0x8b,0xc4,0x38,0x9c,0xf2,0xa7,0x13);

static const WCHAR v4_0[] = {'v','4','.','0','.','3','0','3','1','9',0};

static HMODULE hmscoree;

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pCorIsLatestSvc)(INT*, INT*);
static HRESULT (WINAPI *pGetCORSystemDirectory)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetRequestedRuntimeInfo)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD, LPWSTR, DWORD, DWORD*, LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR, LPCWSTR, LPVOID, HMODULE*);
static HRESULT (WINAPI *pCreateConfigStream)(LPCWSTR, IStream**);
static HRESULT (WINAPI *pCreateInterface)(REFCLSID, REFIID, VOID**);
static HRESULT (WINAPI *pCLRCreateInstance)(REFCLSID, REFIID, VOID**);

static BOOL no_legacy_runtimes;

static BOOL init_functionpointers(void)
{
    hmscoree = LoadLibraryA("mscoree.dll");

    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");
    pCorIsLatestSvc = (void *)GetProcAddress(hmscoree, "CorIsLatestSvc");
    pGetCORSystemDirectory = (void *)GetProcAddress(hmscoree, "GetCORSystemDirectory");
    pGetRequestedRuntimeInfo = (void *)GetProcAddress(hmscoree, "GetRequestedRuntimeInfo");
    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");
    pCreateConfigStream = (void *)GetProcAddress(hmscoree, "CreateConfigStream");
    pCreateInterface =  (void *)GetProcAddress(hmscoree, "CreateInterface");
    pCLRCreateInstance = (void *)GetProcAddress(hmscoree, "CLRCreateInstance");

    if (!pGetCORVersion || !pGetCORSystemDirectory || !pGetRequestedRuntimeInfo || !pLoadLibraryShim ||
        !pCreateInterface || !pCLRCreateInstance || !pCorIsLatestSvc
        )
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

static int check_runtime(void)
{
    ICLRMetaHost *metahost;
    ICLRRuntimeInfo *runtimeinfo;
    ICorRuntimeHost *runtimehost;
    HRESULT hr;

    if (!pCLRCreateInstance)
    {
        win_skip("Function CLRCreateInstance not found.\n");
        return 1;
    }

    hr = pCLRCreateInstance(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, (void **)&metahost);
    if (hr == E_NOTIMPL)
    {
        win_skip("CLRCreateInstance not implemented\n");
        return 1;
    }
    ok(SUCCEEDED(hr), "CLRCreateInstance failed, hr=%#.8lx\n", hr);
    if (FAILED(hr))
        return 1;

    hr = ICLRMetaHost_GetRuntime(metahost, v4_0, &IID_ICLRRuntimeInfo, (void **)&runtimeinfo);
    ok(SUCCEEDED(hr), "ICLRMetaHost::GetRuntime failed, hr=%#.8lx\n", hr);
    if (FAILED(hr))
        return 1;

    hr = ICLRRuntimeInfo_GetInterface(runtimeinfo, &CLSID_CorRuntimeHost, &IID_ICorRuntimeHost,
        (void **)&runtimehost);
    todo_wine_if(!has_mono) ok(SUCCEEDED(hr), "ICLRRuntimeInfo::GetInterface failed, hr=%#.8lx\n", hr);
    if (FAILED(hr))
        return 1;

    hr = ICorRuntimeHost_Start(runtimehost);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::Start failed, hr=%#.8lx\n", hr);
    if (FAILED(hr))
        return 1;

    ICorRuntimeHost_Release(runtimehost);

    ICLRRuntimeInfo_Release(runtimeinfo);

    ICLRMetaHost_ExitProcess(metahost, 0);

    ok(0, "ICLRMetaHost_ExitProcess is not supposed to return\n");
    return 1;
}

static BOOL runtime_is_usable(void)
{
    static const char cmdline_format[] = "\"%s\" mscoree check_runtime";
    char** argv;
    char cmdline[MAX_PATH + sizeof(cmdline_format)];
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD exitcode;

    winetest_get_mainargs(&argv);

    sprintf(cmdline, cmdline_format, argv[0]);

    si.cb = sizeof(si);

    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %lu\n", GetLastError());
    if (!ret)
        return FALSE;

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    ret = GetExitCodeProcess(pi.hProcess, &exitcode);
    ok(ret, "GetExitCodeProcess failed: %lu\n", GetLastError());
    CloseHandle(pi.hProcess);

    if (!ret || exitcode != 0)
    {
	todo_wine_if(!has_mono) win_skip(".NET 4.0 runtime is not usable\n");
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
        ok(hr == E_POINTER,"GetCORVersion returned %08lx\n", hr);
    }

    hr =  pGetCORVersion(version, 1, &size);
    if (hr == CLR_E_SHIM_RUNTIME)
    {
        no_legacy_runtimes = TRUE;
        win_skip("No legacy .NET runtimes are installed\n");
        return;
    }

    ok(hr == E_NOT_SUFFICIENT_BUFFER, "GetCORVersion returned %08lx\n", hr);

    hr =  pGetCORVersion(version, MAX_PATH, &size);
    ok(hr == S_OK,"GetCORVersion returned %08lx\n", hr);

    trace("latest installed .net runtime: %s\n", wine_dbgstr_w(version));

    hr = pGetCORSystemDirectory(path, MAX_PATH , &size);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetCORSystemDirectory returned %08lx\n", hr);
    /* size includes terminating null-character */
    todo_wine_if(!has_mono) ok(size == (lstrlenW(path) + 1),"size is %ld instead of %d\n", size, (lstrlenW(path) + 1));

    path_len = size;

    hr = pGetCORSystemDirectory(path, path_len-1 , &size);
    todo_wine_if(!has_mono) ok(hr == E_NOT_SUFFICIENT_BUFFER, "GetCORSystemDirectory returned %08lx\n", hr);

    if (0)  /* crashes on <= w2k3 */
    {
        hr = pGetCORSystemDirectory(NULL, MAX_PATH , &size);
        ok(hr == E_NOT_SUFFICIENT_BUFFER, "GetCORSystemDirectory returned %08lx\n", hr);
    }

    hr = pGetCORSystemDirectory(path, MAX_PATH , NULL);
    ok(hr == E_POINTER,"GetCORSystemDirectory returned %08lx\n", hr);

    trace("latest installed .net installed in directory: %s\n", wine_dbgstr_w(path));

    /* test GetRequestedRuntimeInfo, first get info about different versions of runtime */
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);

    if(hr == CLR_E_SHIM_RUNTIME) return; /* skipping rest of tests on win2k as .net 2.0 not installed */

    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));

    hr = pGetRequestedRuntimeInfo( NULL, v1_1, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    todo_wine_if(!has_mono) ok(hr == S_OK || hr == CLR_E_SHIM_RUNTIME /*v1_1 not installed*/, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    if(hr == S_OK)
        trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));
    /* version number NULL not allowed without RUNTIME_INFO_UPGRADE_VERSION flag */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    /* with RUNTIME_INFO_UPGRADE_VERSION flag and version number NULL, latest installed version is returned */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);

    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, 1, &path_len, version, MAX_PATH, &size);
    todo_wine_if(!has_mono) ok(hr == E_NOT_SUFFICIENT_BUFFER, "GetRequestedRuntimeInfo returned %08lx\n", hr);

    /* if one of the buffers is NULL, the other one is still happily filled */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, NULL, MAX_PATH, &path_len, version, MAX_PATH, &size);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    ok(!wcscmp(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));
    /* With NULL-pointer for bufferlength, the buffer itself still gets filled with correct string */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    ok(!wcscmp(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0cap, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    ok(!wcscmp(version, v2_0cap), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0cap));

    /* Invalid Version and RUNTIME_INFO_UPGRADE_VERSION flag*/
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    todo_wine_if(!has_mono) ok(hr == S_OK  || hr == CLR_E_SHIM_RUNTIME , "GetRequestedRuntimeInfo returned %08lx\n", hr);
    if(hr == S_OK)
    {
        /* .NET 1.1 may not be installed. */
        ok(!wcscmp(version, v1_1) || !wcscmp(version, v2_0),
           "version is %s , expected %s or %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v1_1), wine_dbgstr_w(v2_0));

    }

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v9_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08lx\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08lx\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v1_1_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    ok(!wcscmp(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08lx\n", hr);

    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0_0, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    todo_wine_if(!has_mono) ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08lx\n", hr);
    ok(!wcscmp(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));

    hr =  pCorIsLatestSvc(NULL, NULL);
    ok(hr == E_POINTER, "CorIsLatestSvc returned %08lx\n", hr);
}

static void test_loadlibraryshim(void)
{
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

    if (no_legacy_runtimes)
    {
        win_skip("No legacy .NET runtimes are installed\n");
        return;
    }

    hr = pLoadLibraryShim(fusion, v1_1, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v1_1;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine_if(!has_mono) ok(StrStrIA(dllpath, "v1.1.4322") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v2_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v2_0;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine_if(!has_mono) ok(StrStrIA(dllpath, "v2.0.50727") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v4_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        /* LoadLibraryShim with a NULL version prefers 2.0 and earlier */
        if (!latest)
            latest = v4_0;

        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        todo_wine_if(!has_mono) ok(StrStrIA(dllpath, "v4.0.30319") != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, vbogus, NULL, &hdll);
    ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    WideCharToMultiByte(CP_ACP, 0, latest, -1, latestA, MAX_PATH, NULL, NULL);

    hr = pLoadLibraryShim(fusion, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine_if(!has_mono) ok(StrStrIA(dllpath, latestA) != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusiondll, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameA(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine_if(!has_mono) ok(StrStrIA(dllpath, latestA) != 0, "incorrect fusion.dll path %s\n", dllpath);
        ok(StrStrIA(dllpath, "fusion.dll") != 0, "incorrect fusion.dll path %s\n", dllpath);

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(nosuchdll, latest, NULL, &hdll);
    ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    hr = pLoadLibraryShim(gdidll, latest, NULL, &hdll);
    ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%lx\n", hr);
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
    ok(hfile != INVALID_HANDLE_VALUE, "Could not open %s for writing: %lu\n", wine_dbgstr_w(filename), GetLastError());
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
    ok(hr == E_FAIL ||
       broken(hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND)) || /* some WinXP, Win2K3 and Win7 */
       broken(hr == S_OK && !stream), /* some Win2K3 */
       "CreateConfigStream returned %lx\n", hr);

    hr = pCreateConfigStream(path, NULL);
    ok(hr == COR_E_NULLREFERENCE, "CreateConfigStream returned %lx\n", hr);

    hr = pCreateConfigStream(NULL, NULL);
    ok(hr == COR_E_NULLREFERENCE, "CreateConfigStream returned %lx\n", hr);

    hr = pCreateConfigStream(nonexistent, &stream);
    ok(hr == COR_E_FILENOTFOUND, "CreateConfigStream returned %lx\n", hr);
    ok(stream == NULL, "Expected stream to be NULL\n");

    hr = pCreateConfigStream(path, &stream);
    ok(hr == S_OK, "CreateConfigStream failed, hr=%lx\n", hr);
    ok(stream != NULL, "Expected non-NULL stream\n");

    if (stream)
    {
        DWORD count;
        LARGE_INTEGER pos;
        ULARGE_INTEGER size;
        IStream *stream2 = NULL;
        ULONG ref;

        hr = IStream_Read(stream, buffer, strlen(xmldata), &count);
        ok(hr == S_OK, "IStream_Read failed, hr=%lx\n", hr);
        ok(count == strlen(xmldata), "wrong count: %lu\n", count);
        ok(!strcmp(buffer, xmldata), "Strings do not match\n");

        hr = IStream_Read(stream, buffer, sizeof(buffer), &count);
        ok(hr == S_OK, "IStream_Read failed, hr=%lx\n", hr);
        ok(!count, "wrong count: %lu\n", count);

        hr = IStream_Write(stream, xmldata, strlen(xmldata), &count);
        ok(hr == E_FAIL, "IStream_Write returned hr=%lx\n", hr);

        pos.QuadPart = strlen(xmldata);
        hr = IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);
        ok(hr == E_NOTIMPL, "IStream_Seek returned hr=%lx\n", hr);

        size.QuadPart = strlen(xmldata);
        hr = IStream_SetSize(stream, size);
        ok(hr == E_NOTIMPL, "IStream_SetSize returned hr=%lx\n", hr);

        hr = IStream_Clone(stream, &stream2);
        ok(hr == E_NOTIMPL, "IStream_Clone returned hr=%lx\n", hr);

        hr = IStream_Commit(stream, STGC_DEFAULT);
        ok(hr == E_NOTIMPL, "IStream_Commit returned hr=%lx\n", hr);

        hr = IStream_Revert(stream);
        ok(hr == E_NOTIMPL, "IStream_Revert returned hr=%lx\n", hr);

        ref = IStream_Release(stream);
        ok(!ref, "IStream_Release returned %lu\n", ref);
    }
    DeleteFileW(file);
}

static void test_createinstance(void)
{
    HRESULT hr;
    ICLRMetaHost *host;

    if (no_legacy_runtimes)
    {
        /* If we don't have 1.x or 2.0 runtimes, we should at least have .NET 4. */
        ok(pCreateInterface != NULL, "no legacy runtimes or .NET 4 interfaces available\n");
    }

    if(!pCreateInterface)
    {
        win_skip("Function CreateInterface not found.\n");
        return;
    }

    hr = pCreateInterface(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, (void**)&host);
    if(SUCCEEDED(hr))
    {
        ICLRMetaHost_Release(host);
    }
    else
    {
        win_skip(".NET 4 not installed.\n");
    }
}

static BOOL write_resource(const WCHAR *resource, const WCHAR *filename)
{
    HANDLE file;
    HRSRC rsrc;
    void *data;
    DWORD size;
    BOOL ret;

    rsrc = FindResourceW(GetModuleHandleW(NULL), resource, MAKEINTRESOURCEW(RT_RCDATA));
    if (!rsrc) return FALSE;

    data = LockResource(LoadResource(GetModuleHandleA(NULL), rsrc));
    if (!data) return FALSE;

    size = SizeofResource(GetModuleHandleA(NULL), rsrc);
    if (!size) return FALSE;

    file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) return FALSE;

    ret = WriteFile(file, data, size, &size, NULL);
    CloseHandle(file);
    return ret;
}

static BOOL compile_cs(const WCHAR *source, const WCHAR *target, const WCHAR *type, const WCHAR *args)
{
    static const WCHAR *csc = L"C:\\windows\\Microsoft.NET\\Framework\\v2.0.50727\\csc.exe";
    WCHAR cmdline[2 * MAX_PATH + 74];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { 0 };
    BOOL ret;

    if (!PathFileExistsW(csc))
    {
        skip("Can't find csc.exe\n");
        return FALSE;
    }

    swprintf(cmdline, ARRAY_SIZE(cmdline), L"%s /t:%s %s /out:\"%s\" \"%s\"", csc, type, args, target, source);

    si.cb = sizeof(si);
    ret = CreateProcessW(csc, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "Could not create process: %lu\n", GetLastError());

    wait_child_process(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    ret = PathFileExistsW(target);
    ok(ret, "Compilation failed\n");

    return ret;
}

static BOOL create_new_dir(WCHAR newdir[MAX_PATH], const WCHAR* prefix)
{
    WCHAR path[MAX_PATH];
    BOOL try_tmpdir = TRUE;
    static unsigned i = 0;

    GetCurrentDirectoryW(ARRAY_SIZE(path), path);

    while (1)
    {
        swprintf(newdir, MAX_PATH, L"%s\\%s%04d", path, prefix, i);
        if (CreateDirectoryW(newdir, NULL))
            return TRUE;
        switch (GetLastError())
        {
        case ERROR_ACCESS_DENIED:
            if (!try_tmpdir)
                return FALSE;
            try_tmpdir = FALSE;
            GetTempPathW(ARRAY_SIZE(path), path);
            path[wcslen(path) - 1] = 0; /* redundant trailing backslash */
            break;
        case ERROR_ALREADY_EXISTS:
            i++;
            break;
        default:
            return FALSE;
        }
    }
}

static void test_loadpaths_execute(const WCHAR *exe_name, const WCHAR *dll_name, const WCHAR *cfg_name,
                                   const WCHAR *dll_dest, BOOL expect_failure, BOOL todo)
{
    WCHAR tmpdir[MAX_PATH], tmpexe[MAX_PATH], tmpcfg[MAX_PATH], tmpdll[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { 0 };
    WCHAR *ptr, *end;
    DWORD exit_code = 0xdeadbeef, err;
    BOOL ret;

    ret = create_new_dir(tmpdir, L"loadpaths");
    ok(ret, "failed to create a new dir %lu\n", GetLastError());
    end = tmpdir + wcslen(tmpdir);

    wcscpy(tmpexe, tmpdir);
    PathAppendW(tmpexe, exe_name);
    ret = CopyFileW(exe_name, tmpexe, FALSE);
    ok(ret, "CopyFileW(%s) failed: %lu\n", debugstr_w(tmpexe), GetLastError());

    if (cfg_name)
    {
        wcscpy(tmpcfg, tmpdir);
        PathAppendW(tmpcfg, cfg_name);
        ret = CopyFileW(cfg_name, tmpcfg, FALSE);
        ok(ret, "CopyFileW(%s) failed: %lu\n", debugstr_w(tmpcfg), GetLastError());
    }

    ptr = tmpdir + wcslen(tmpdir);
    PathAppendW(tmpdir, dll_dest);
    while (*ptr && (ptr = wcschr(ptr + 1, '\\')))
    {
        *ptr = '\0';
        ret = CreateDirectoryW(tmpdir, NULL);
        ok(ret, "CreateDirectoryW(%s) failed: %lu\n", debugstr_w(tmpdir), GetLastError());
        *ptr = '\\';
    }

    wcscpy(tmpdll, tmpdir);
    if ((ptr = wcsrchr(tmpdir, '\\'))) *ptr = '\0';

    ret = CopyFileW(dll_name, tmpdll, FALSE);
    ok(ret, "CopyFileW(%s) failed: %lu\n", debugstr_w(tmpdll), GetLastError());

    si.cb = sizeof(si);
    ret = CreateProcessW(tmpexe, tmpexe, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcessW(%s) failed: %lu\n", debugstr_w(tmpexe), GetLastError());

    if (expect_failure) ret = WaitForSingleObject(pi.hProcess, 2000);
    else
    {
        ret = WaitForSingleObject(pi.hProcess, 5000);
        ok(ret == WAIT_OBJECT_0, "%s: WaitForSingleObject returned %d: %lu\n", debugstr_w(dll_dest), ret, GetLastError());
    }

    GetExitCodeProcess(pi.hProcess, &exit_code);
    if (ret == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 0xdeadbeef);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (expect_failure) todo_wine_if(todo) ok(exit_code != 0, "%s: Succeeded to execute process\n", debugstr_w(dll_dest));
    else ok(exit_code == 0, "%s: Failed to execute process\n", debugstr_w(dll_dest));

    /* sometimes the failing process never returns, in which case cleaning up won't work */
    if (ret == WAIT_TIMEOUT && expect_failure) return;

    if (cfg_name)
    {
        ret = DeleteFileW(tmpcfg);
        ok(ret, "DeleteFileW(%s) failed: %lu\n", debugstr_w(tmpcfg), GetLastError());
    }
    ret = DeleteFileW(tmpdll);
    ok(ret, "DeleteFileW(%s) failed: %lu\n", debugstr_w(tmpdll), GetLastError());
    ret = DeleteFileW(tmpexe);
    ok(ret, "DeleteFileW(%s) failed: %lu\n", debugstr_w(tmpexe), GetLastError());

    ptr = end;
    while (ptr >= end && (ptr = wcsrchr(tmpdir, '\\')))
    {
        ret = RemoveDirectoryW(tmpdir);
        err = GetLastError();
        ok(ret, "RemoveDirectoryW(%s) failed: %lu\n", debugstr_w(tmpdir), err);

        if (!ret && err == ERROR_DIR_NOT_EMPTY)
        {
            WIN32_FIND_DATAW fd;
            HANDLE hfind;

            wcscat(tmpdir, L"\\*");
            hfind = FindFirstFileW(tmpdir, &fd);
            while (hfind != INVALID_HANDLE_VALUE && (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")))
            {
                if (!FindNextFileW(hfind, &fd))
                {
                    FindClose(hfind);
                    hfind = INVALID_HANDLE_VALUE;
                }
            }
            if (hfind != INVALID_HANDLE_VALUE)
            {
                trace("file %s still present in tmpdir\n", debugstr_w(fd.cFileName));
                FindClose(hfind);
            }
        }

        *ptr = '\0';
    }
}

static void test_loadpaths(BOOL neutral)
{
    static const WCHAR *loadpaths[] = {L"", L"en", L"libloadpaths", L"en\\libloadpaths"};
    static const WCHAR *dll_source = L"loadpaths.dll.cs";
    static const WCHAR *dll_name = L"libloadpaths.dll";
    static const WCHAR *exe_source = L"loadpaths.exe.cs";
    static const WCHAR *exe_name = L"loadpaths.exe";
    static const WCHAR *cfg_name = L"loadpaths.exe.config";
    WCHAR tmp[MAX_PATH];
    BOOL ret;
    int i;

    DeleteFileW(dll_source);
    ret = write_resource(dll_source, dll_source);
    ok(ret, "Could not write resource: %lu\n", GetLastError());
    DeleteFileW(dll_name);
    ret = compile_cs(dll_source, dll_name, L"library", neutral ? L"-define:NEUTRAL" : L"");
    if (!ret) return;
    ret = DeleteFileW(dll_source);
    ok(ret, "DeleteFileW failed: %lu\n", GetLastError());

    DeleteFileW(exe_source);
    ret = write_resource(exe_source, exe_source);
    ok(ret, "Could not write resource: %lu\n", GetLastError());
    DeleteFileW(exe_name);
    ret = compile_cs(exe_source, exe_name, L"winexe", L"/reference:libloadpaths.dll");
    if (!ret) return;
    ret = DeleteFileW(exe_source);
    ok(ret, "DeleteFileW failed: %lu\n", GetLastError());

    DeleteFileW(cfg_name);
    ret = write_resource(cfg_name, cfg_name);
    ok(ret, "Could not write resource: %lu\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(loadpaths); ++i)
    {
        const WCHAR *path = loadpaths[i];
        BOOL expect_failure = neutral ? wcsstr(path, L"en") != NULL
                                      : wcsstr(path, L"en") == NULL;

        wcscpy(tmp, path);
        PathAppendW(tmp, dll_name);
        test_loadpaths_execute(exe_name, dll_name, NULL, tmp, expect_failure, !neutral && !*path);

        wcscpy(tmp, L"private");
        if (*path) PathAppendW(tmp, path);
        PathAppendW(tmp, dll_name);

        test_loadpaths_execute(exe_name, dll_name, NULL, tmp, TRUE, FALSE);
        test_loadpaths_execute(exe_name, dll_name, cfg_name, tmp, expect_failure, FALSE);

        /* exe name for dll should work too */
        if (*path)
        {
            wcscpy(tmp, path);
            PathAppendW(tmp, dll_name);
            wcscpy(tmp + wcslen(tmp) - 4, L".exe");
            test_loadpaths_execute(exe_name, dll_name, NULL, tmp, expect_failure, FALSE);
        }

        wcscpy(tmp, L"private");
        if (*path) PathAppendW(tmp, path);
        PathAppendW(tmp, dll_name);
        wcscpy(tmp + wcslen(tmp) - 4, L".exe");

        test_loadpaths_execute(exe_name, dll_name, NULL, tmp, TRUE, FALSE);
        test_loadpaths_execute(exe_name, dll_name, cfg_name, tmp, expect_failure, FALSE);
    }

    ret = DeleteFileW(cfg_name);
    ok(ret, "DeleteFileW failed: %lu\n", GetLastError());
    ret = DeleteFileW(exe_name);
    ok(ret, "DeleteFileW failed: %lu\n", GetLastError());
    ret = DeleteFileW(dll_name);
    ok(ret, "DeleteFileW failed: %lu\n", GetLastError());
}

static void test_createdomain(void)
{
    static const WCHAR test_name[] = {'t','e','s','t',0};
    static const WCHAR test2_name[] = {'t','e','s','t','2',0};
    ICLRMetaHost *metahost;
    ICLRRuntimeInfo *runtimeinfo;
    ICorRuntimeHost *runtimehost;
    IUnknown *domain, *defaultdomain_unk, *defaultdomain, *newdomain_unk, *newdomain, *domainsetup,
        *newdomain2_unk, *newdomain2;
    HRESULT hr;

    if (!pCLRCreateInstance)
    {
        win_skip("Function CLRCreateInstance not found.\n");
        return;
    }

    hr = pCLRCreateInstance(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, (void **)&metahost);
    ok(SUCCEEDED(hr), "CLRCreateInstance failed, hr=%#.8lx\n", hr);

    hr = ICLRMetaHost_GetRuntime(metahost, v4_0, &IID_ICLRRuntimeInfo, (void **)&runtimeinfo);
    ok(SUCCEEDED(hr), "ICLRMetaHost::GetRuntime failed, hr=%#.8lx\n", hr);

    hr = ICLRRuntimeInfo_GetInterface(runtimeinfo, &CLSID_CorRuntimeHost, &IID_ICorRuntimeHost,
        (void **)&runtimehost);
    ok(SUCCEEDED(hr), "ICLRRuntimeInfo::GetInterface failed, hr=%#.8lx\n", hr);

    hr = ICorRuntimeHost_Start(runtimehost);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::Start failed, hr=%#.8lx\n", hr);

    hr = ICorRuntimeHost_GetDefaultDomain(runtimehost, &domain);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::GetDefaultDomain failed, hr=%#.8lx\n", hr);

    hr = IUnknown_QueryInterface(domain, &IID_IUnknown, (void **)&defaultdomain_unk);
    ok(SUCCEEDED(hr), "COM object doesn't support IUnknown?!\n");

    hr = IUnknown_QueryInterface(domain, &IID__AppDomain, (void **)&defaultdomain);
    ok(SUCCEEDED(hr), "AppDomain object doesn't support _AppDomain interface\n");

    IUnknown_Release(domain);

    hr = ICorRuntimeHost_CreateDomain(runtimehost, test_name, NULL, &domain);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::CreateDomain failed, hr=%#.8lx\n", hr);

    hr = IUnknown_QueryInterface(domain, &IID_IUnknown, (void **)&newdomain_unk);
    ok(SUCCEEDED(hr), "COM object doesn't support IUnknown?!\n");

    hr = IUnknown_QueryInterface(domain, &IID__AppDomain, (void **)&newdomain);
    ok(SUCCEEDED(hr), "AppDomain object doesn't support _AppDomain interface\n");

    IUnknown_Release(domain);

    ok(defaultdomain_unk != newdomain_unk, "New and default domain objects are the same\n");

    hr = ICorRuntimeHost_CreateDomainSetup(runtimehost, &domainsetup);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::CreateDomainSetup failed, hr=%#.8lx\n", hr);

    hr = ICorRuntimeHost_CreateDomainEx(runtimehost, test2_name, domainsetup, NULL, &domain);
    ok(SUCCEEDED(hr), "ICorRuntimeHost::CreateDomainEx failed, hr=%#.8lx\n", hr);

    hr = IUnknown_QueryInterface(domain, &IID_IUnknown, (void **)&newdomain2_unk);
    ok(SUCCEEDED(hr), "COM object doesn't support IUnknown?!\n");

    hr = IUnknown_QueryInterface(domain, &IID__AppDomain, (void **)&newdomain2);
    ok(SUCCEEDED(hr), "AppDomain object doesn't support _AppDomain interface\n");

    IUnknown_Release(domain);

    ok(defaultdomain_unk != newdomain2_unk, "New and default domain objects are the same\n");
    ok(newdomain_unk != newdomain2_unk, "Both new domain objects are the same\n");

    IUnknown_Release(newdomain2);
    IUnknown_Release(newdomain2_unk);
    IUnknown_Release(domainsetup);
    IUnknown_Release(newdomain);
    IUnknown_Release(newdomain_unk);
    IUnknown_Release(defaultdomain);
    IUnknown_Release(defaultdomain_unk);

    ICorRuntimeHost_Release(runtimehost);

    ICLRRuntimeInfo_Release(runtimeinfo);

    ICLRMetaHost_Release(metahost);
}

START_TEST(mscoree)
{
    int argc;
    char** argv;

    if (!init_functionpointers())
        return;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3 && !strcmp(argv[2], "check_runtime"))
    {
        int result = check_runtime();
        FreeLibrary(hmscoree);
        exit(result);
    }

    test_versioninfo();
    test_loadlibraryshim();
    test_createconfigstream();
    test_createinstance();

    if (runtime_is_usable())
    {
        test_createdomain();
    }

    test_loadpaths(FALSE);
    test_loadpaths(TRUE);

    FreeLibrary(hmscoree);
}
