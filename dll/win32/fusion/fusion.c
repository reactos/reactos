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

#include "fusionpriv.h"

/******************************************************************
 *  InitializeFusion   (FUSION.@)
 */
HRESULT WINAPI InitializeFusion(void)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************
 *  ClearDownloadCache   (FUSION.@)
 */
HRESULT WINAPI ClearDownloadCache(void)
{
    FIXME("stub!\n");
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
 *  CreateApplicationContext   (FUSION.@)
 */
HRESULT WINAPI CreateApplicationContext(IAssemblyName *name, void *ctx)
{
    FIXME("%p, %p\n", name, ctx);
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
        hr = E_FAIL;
    else
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
    static const WCHAR assembly[] = {'\\','a','s','s','e','m','b','l','y',0};
    static const WCHAR gac[] = {'\\','G','A','C',0};
    static const WCHAR nativeimg[] = {'N','a','t','i','v','e','I','m','a','g','e','s','_',0};
    static const WCHAR dotnet[] = {'\\','M','i','c','r','o','s','o','f','t','.','N','E','T',0};
#ifdef _WIN64
    static const WCHAR zapfmt[] = {'%','s','\\','%','s','\\','%','s','%','s','_','6','4',0};
#else
    static const WCHAR zapfmt[] = {'%','s','\\','%','s','\\','%','s','%','s','_','3','2',0};
#endif
    WCHAR path[MAX_PATH], windir[MAX_PATH], version[MAX_PATH];
    DWORD len;
    HRESULT hr = S_OK;

    TRACE("(%08x, %p, %p)\n", dwCacheFlags, pwzCachePath, pcchPath);

    if (!pcchPath)
        return E_INVALIDARG;

    len = GetWindowsDirectoryW(windir, MAX_PATH);
    strcpyW(path, windir);

    switch (dwCacheFlags)
    {
        case ASM_CACHE_ZAP:
        {
            hr = get_corversion(version, MAX_PATH);
            if (FAILED(hr))
                return hr;

            len = sprintfW(path, zapfmt, windir, assembly + 1, nativeimg, version);
            break;
        }
        case ASM_CACHE_GAC:
        {
            strcpyW(path + len, assembly);
            len += sizeof(assembly)/sizeof(WCHAR) - 1;
            strcpyW(path + len, gac);
            len += sizeof(gac)/sizeof(WCHAR) - 1;
            break;
        }
        case ASM_CACHE_DOWNLOAD:
        {
            FIXME("Download cache not implemented\n");
            return E_FAIL;
        }
        case ASM_CACHE_ROOT:
            strcpyW(path + len, assembly);
            len += sizeof(assembly)/sizeof(WCHAR) - 1;
            break;
        case ASM_CACHE_ROOT_EX:
            strcpyW(path + len, dotnet);
            len += sizeof(dotnet)/sizeof(WCHAR) - 1;
            strcpyW(path + len, assembly);
            len += sizeof(assembly)/sizeof(WCHAR) - 1;
            break;
        default:
            return E_INVALIDARG;
    }

    len++;
    if (*pcchPath <= len || !pwzCachePath)
        hr = E_NOT_SUFFICIENT_BUFFER;
    else if (pwzCachePath)
        strcpyW(pwzCachePath, path);

    *pcchPath = len;
    return hr;
}
