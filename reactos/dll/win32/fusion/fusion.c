/*
 * Implementation of the Fusion API
 *
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "fusion.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

/******************************************************************
 *  ClearDownloadCache   (FUSION.@)
 */
HRESULT WINAPI ClearDownloadCache(void)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

/******************************************************************
 *  CompareAssemblyIdentity   (FUSION.@)
 */
HRESULT WINAPI CompareAssemblyIdentity(LPCWSTR pwzAssemblyIdentity1, BOOL fUnified1,
                                       LPCWSTR pwzAssemblyIdentity2, BOOL fUnified2,
                                       BOOL *pfEquivalent, AssemblyComparisonResult *pResult)
{
    FIXME("(%s, %d, %s, %d, %p, %p) stub!\n", debugstr_w(pwzAssemblyIdentity1),
          fUnified1, debugstr_w(pwzAssemblyIdentity2), fUnified2, pfEquivalent, pResult);

    return E_NOTIMPL;
}

/******************************************************************
 *  CreateInstallReferenceEnum   (FUSION.@)
 */
HRESULT WINAPI CreateInstallReferenceEnum(IInstallReferenceEnum **ppRefEnum,
                                          IAssemblyName *pName, DWORD dwFlags,
                                          LPVOID pvReserved)
{
    FIXME("(%p, %p, %08x, %p) stub!\n", ppRefEnum, pName, dwFlags, pvReserved);
    return E_NOTIMPL;
}

/******************************************************************
 *  GetAssemblyIdentityFromFile   (FUSION.@)
 */
HRESULT WINAPI GetAssemblyIdentityFromFile(LPCWSTR pwzFilePath, REFIID riid,
                                           IUnknown **ppIdentity)
{
    FIXME("(%s, %s, %p) stub!\n", debugstr_w(pwzFilePath), debugstr_guid(riid),
          ppIdentity);

    return E_NOTIMPL;
}

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer,
                                        DWORD *dwLength);

static HRESULT get_corversion(LPWSTR version, DWORD size)
{
    HMODULE hmscoree;
    HRESULT hr;
    DWORD len;

    hmscoree = LoadLibraryA("mscoree.dll");
    if (!hmscoree)
        return E_FAIL;

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");
    if (!pGetCORVersion)
        return E_FAIL;

    hr = pGetCORVersion(version, size, &len);

    FreeLibrary(hmscoree);
    return hr;
}

/******************************************************************
 *  GetCachePath   (FUSION.@)
 */
HRESULT WINAPI GetCachePath(ASM_CACHE_FLAGS dwCacheFlags, LPWSTR pwzCachePath,
                            PDWORD pcchPath)
{
    WCHAR path[MAX_PATH];
    WCHAR windir[MAX_PATH];
    WCHAR version[MAX_PATH];
    DWORD len;
    HRESULT hr = S_OK;

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR assembly[] = {'a','s','s','e','m','b','l','y',0};
    static const WCHAR gac[] = {'G','A','C',0};
    static const WCHAR nativeimg[] = {
        'N','a','t','i','v','e','I','m','a','g','e','s','_',0};
#ifdef _WIN64
    static const WCHAR zapfmt[] = {'%','s','\\','%','s','\\','%','s','%','s','_','6','4',0};
#else
    static const WCHAR zapfmt[] = {'%','s','\\','%','s','\\','%','s','%','s','_','3','2',0};
#endif

    TRACE("(%08x, %p, %p)\n", dwCacheFlags, pwzCachePath, pcchPath);

    if (!pcchPath)
        return E_INVALIDARG;

    GetWindowsDirectoryW(windir, MAX_PATH);
    lstrcpyW(path, windir);
    lstrcatW(path, backslash);
    lstrcatW(path, assembly);

    switch (dwCacheFlags)
    {
        case ASM_CACHE_ZAP:
        {
            hr = get_corversion(version, MAX_PATH);
            if (FAILED(hr))
                return hr;

            sprintfW(path, zapfmt, windir, assembly, nativeimg, version);
            break;
        }

        case ASM_CACHE_GAC:
        {
            lstrcatW(path, backslash);
            lstrcatW(path, gac);
            break;
        }

        case ASM_CACHE_DOWNLOAD:
        {
            FIXME("Download cache not implemented\n");
            return E_FAIL;
        }

        case ASM_CACHE_ROOT:
            break; /* already set */

        default:
            return E_INVALIDARG;
    }

    len = lstrlenW(path) + 1;
    if (*pcchPath <= len || !pwzCachePath)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    else if (pwzCachePath)
        lstrcpyW(pwzCachePath, path);

    *pcchPath = len;

    return hr;
}
