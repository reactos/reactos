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

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

//#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <objbase.h>
#include <fusion.h>

#include <wine/test.h>

static HMODULE hmscoree;

static HRESULT (WINAPI *pGetCachePath)(ASM_CACHE_FLAGS dwCacheFlags,
                                       LPWSTR pwzCachePath, PDWORD pcchPath);
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR szDllName, LPCWSTR szVersion,
                                          LPVOID pvReserved, HMODULE *phModDll);
static HRESULT (WINAPI *pGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer,
                                        DWORD *dwLength);

static BOOL init_functionpointers(void)
{
    HRESULT hr;
    HMODULE hfusion;

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

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");

    hr = pLoadLibraryShim(szFusion, NULL, NULL, &hfusion);
    if (FAILED(hr))
    {
        win_skip("fusion.dll not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    pGetCachePath = (void *)GetProcAddress(hfusion, "GetCachePath");
    return TRUE;
}

static void test_GetCachePath(void)
{
    CHAR windirA[MAX_PATH];
    WCHAR windir[MAX_PATH];
    WCHAR cachepath[MAX_PATH];
    WCHAR version[MAX_PATH];
    WCHAR path[MAX_PATH];
    DWORD size;
    HRESULT hr;

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR nochange[] = {'n','o','c','h','a','n','g','e',0};
    static const WCHAR assembly[] = {'a','s','s','e','m','b','l','y',0};
    static const WCHAR gac[] = {'G','A','C',0};

    if (!pGetCachePath)
    {
        win_skip("GetCachePath not implemented\n");
        return;
    }

    GetWindowsDirectoryA(windirA, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, windirA, -1, windir, MAX_PATH);
    lstrcpyW(cachepath, windir);
    lstrcatW(cachepath, backslash);
    lstrcatW(cachepath, assembly);
    lstrcatW(cachepath, backslash);
    lstrcatW(cachepath, gac);

    /* NULL pwzCachePath, pcchPath is 0 */
    size = 0;
    hr = pGetCachePath(ASM_CACHE_GAC, NULL, &size);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08x\n", hr);
    ok(size == lstrlenW(cachepath) + 1,
       "Expected %d, got %d\n", lstrlenW(cachepath) + 1, size);

    /* NULL pwszCachePath, pcchPath is MAX_PATH */
    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC, NULL, &size);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08x\n", hr);
    ok(size == lstrlenW(cachepath) + 1,
       "Expected %d, got %d\n", lstrlenW(cachepath) + 1, size);

    /* both pwszCachePath and pcchPath NULL */
    hr = pGetCachePath(ASM_CACHE_GAC, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    /* NULL pcchPath */
    lstrcpyW(path, nochange);
    hr = pGetCachePath(ASM_CACHE_GAC, path, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok( !lstrcmpW( nochange, path ), "Expected %s,  got %s\n", wine_dbgstr_w(nochange), wine_dbgstr_w(path));

    /* get the cache path */
    lstrcpyW(path, nochange);
    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC, path, &size);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok( !lstrcmpW( cachepath, path ), "Expected %s,  got %s\n", wine_dbgstr_w(cachepath), wine_dbgstr_w(path));

    /* pcchPath has no room for NULL terminator */
    lstrcpyW(path, nochange);
    size = lstrlenW(cachepath);
    hr = pGetCachePath(ASM_CACHE_GAC, path, &size);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Expected E_NOT_SUFFICIENT_BUFFER, got %08x\n", hr);
    ok( !lstrcmpW( nochange, path ), "Expected %s,  got %s\n", wine_dbgstr_w(nochange), wine_dbgstr_w(path));

    lstrcpyW(cachepath, windir);
    lstrcatW(cachepath, backslash);
    lstrcatW(cachepath, assembly);

    /* ASM_CACHE_ROOT */
    lstrcpyW(path, nochange);
    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_ROOT, path, &size);
    ok(hr == S_OK ||
       broken(hr == E_INVALIDARG), /* .NET 1.1 */
       "Expected S_OK, got %08x\n", hr);
    if (hr == S_OK)
        ok( !lstrcmpW( cachepath, path ), "Expected %s,  got %s\n", wine_dbgstr_w(cachepath), wine_dbgstr_w(path));

    if (pGetCORVersion)
    {
        CHAR versionA[MAX_PATH];
        CHAR cachepathA[MAX_PATH];
        CHAR nativeimgA[MAX_PATH];
        CHAR zapfmtA[MAX_PATH];

        if (hr == S_OK)
        {
            lstrcpyA(nativeimgA, "NativeImages_");
#ifdef _WIN64
            lstrcpyA(zapfmtA, "%s\\%s\\%s%s_64");
#else
            lstrcpyA(zapfmtA, "%s\\%s\\%s%s_32");
#endif
        }
        else
        {
            lstrcpyA(nativeimgA, "NativeImages1_");
            lstrcpyA(zapfmtA, "%s\\%s\\%s%s");
        }

        pGetCORVersion(version, MAX_PATH, &size);
        WideCharToMultiByte(CP_ACP, 0, version, -1, versionA, MAX_PATH, 0, 0);

        wsprintfA(cachepathA, zapfmtA, windirA, "assembly", nativeimgA, versionA);
        MultiByteToWideChar(CP_ACP, 0, cachepathA, -1, cachepath, MAX_PATH);

        /* ASM_CACHE_ZAP */
        lstrcpyW(path, nochange);
        size = MAX_PATH;
        hr = pGetCachePath(ASM_CACHE_ZAP, path, &size);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
        ok( !lstrcmpW( cachepath, path ), "Expected %s,  got %s\n", wine_dbgstr_w(cachepath), wine_dbgstr_w(path));
    }

    /* two flags at once */
    lstrcpyW(path, nochange);
    size = MAX_PATH;
    hr = pGetCachePath(ASM_CACHE_GAC | ASM_CACHE_ROOT, path, &size);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok( !lstrcmpW( nochange, path ), "Expected %s,  got %s\n", wine_dbgstr_w(nochange), wine_dbgstr_w(path));
}

START_TEST(fusion)
{
    if (!init_functionpointers())
        return;

    test_GetCachePath();

    FreeLibrary(hmscoree);
}
