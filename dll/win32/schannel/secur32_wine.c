/* Copyright (C) 2004 Juan Lang
 *
 * This file implements loading of SSP DLLs.
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

#include "precomp.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(schannel);

typedef struct _SecurePackageTable
{
    DWORD numPackages;
    DWORD numAllocated;
    struct list table;
} SecurePackageTable;

typedef struct _SecureProviderTable
{
    DWORD numProviders;
    DWORD numAllocated;
    struct list table;
} SecureProviderTable;

/**
 *  Globals
 */

static CRITICAL_SECTION cs;
static CRITICAL_SECTION_DEBUG cs_debug =
{
    0, 0, &cs,
    { &cs_debug.ProcessLocksList, &cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs") }
};
static CRITICAL_SECTION cs = { &cs_debug, -1, 0, 0, 0, 0 };
static SecurePackageTable *packageTable = NULL;
static SecureProviderTable *providerTable = NULL;

/***********************************************************************
 *      EnumerateSecurityPackagesW (SECUR32.@)
 */
SECURITY_STATUS WINAPI schan_EnumerateSecurityPackagesW(PULONG pcPackages,
 PSecPkgInfoW *ppPackageInfo)
{
    SECURITY_STATUS ret = SEC_E_OK;

    TRACE("(%p, %p)\n", pcPackages, ppPackageInfo);

    /* windows just crashes if pcPackages or ppPackageInfo is NULL, so will I */
    *pcPackages = 0;
    EnterCriticalSection(&cs);
    if (packageTable)
    {
        SecurePackage *package;
        size_t bytesNeeded;

        bytesNeeded = packageTable->numPackages * sizeof(SecPkgInfoW);
        LIST_FOR_EACH_ENTRY(package, &packageTable->table, SecurePackage, entry)
        {
            if (package->infoW.Name)
                bytesNeeded += (lstrlenW(package->infoW.Name) + 1) * sizeof(WCHAR);
            if (package->infoW.Comment)
                bytesNeeded += (lstrlenW(package->infoW.Comment) + 1) * sizeof(WCHAR);
        }
        if (bytesNeeded)
        {
            *ppPackageInfo = HeapAlloc(GetProcessHeap(), 0, bytesNeeded);
            if (*ppPackageInfo)
            {
                ULONG i = 0;
                PWSTR nextString;

                *pcPackages = packageTable->numPackages;
                nextString = (PWSTR)((PBYTE)*ppPackageInfo +
                 packageTable->numPackages * sizeof(SecPkgInfoW));
                LIST_FOR_EACH_ENTRY(package, &packageTable->table, SecurePackage, entry)
                {
                    PSecPkgInfoW pkgInfo = *ppPackageInfo + i++;

                    *pkgInfo = package->infoW;
                    if (package->infoW.Name)
                    {
                        TRACE("Name[%d] = %S\n", i - 1, package->infoW.Name);
                        pkgInfo->Name = nextString;
                        lstrcpyW(nextString, package->infoW.Name);
                        nextString += lstrlenW(nextString) + 1;
                    }
                    else
                        pkgInfo->Name = NULL;
                    if (package->infoW.Comment)
                    {
                        TRACE("Comment[%d] = %S\n", i - 1, package->infoW.Comment);
                        pkgInfo->Comment = nextString;
                        lstrcpyW(nextString, package->infoW.Comment);
                        nextString += lstrlenW(nextString) + 1;
                    }
                    else
                        pkgInfo->Comment = NULL;
                }
            }
            else
                ret = SEC_E_INSUFFICIENT_MEMORY;
        }
    }
    LeaveCriticalSection(&cs);
    TRACE("<-- 0x%08x\n", ret);
    return ret;
}

/* Converts info (which is assumed to be an array of cPackages SecPkgInfoW
 * structures) into an array of SecPkgInfoA structures, which it returns.
 */
static PSecPkgInfoA thunk_PSecPkgInfoWToA(ULONG cPackages,
 const SecPkgInfoW *info)
{
    PSecPkgInfoA ret;

    if (info)
    {
        size_t bytesNeeded = cPackages * sizeof(SecPkgInfoA);
        ULONG i;

        for (i = 0; i < cPackages; i++)
        {
            if (info[i].Name)
                bytesNeeded += WideCharToMultiByte(CP_ACP, 0, info[i].Name,
                 -1, NULL, 0, NULL, NULL);
            if (info[i].Comment)
                bytesNeeded += WideCharToMultiByte(CP_ACP, 0, info[i].Comment,
                 -1, NULL, 0, NULL, NULL);
        }
        ret = HeapAlloc(GetProcessHeap(), 0, bytesNeeded);
        if (ret)
        {
            PSTR nextString;

            nextString = (PSTR)((PBYTE)ret + cPackages * sizeof(SecPkgInfoA));
            for (i = 0; i < cPackages; i++)
            {
                PSecPkgInfoA pkgInfo = ret + i;
                int bytes;

                memcpy(pkgInfo, &info[i], sizeof(SecPkgInfoA));
                if (info[i].Name)
                {
                    pkgInfo->Name = nextString;
                    /* just repeat back to WideCharToMultiByte how many bytes
                     * it requires, since we asked it earlier
                     */
                    bytes = WideCharToMultiByte(CP_ACP, 0, info[i].Name, -1,
                     NULL, 0, NULL, NULL);
                    WideCharToMultiByte(CP_ACP, 0, info[i].Name, -1,
                     pkgInfo->Name, bytes, NULL, NULL);
                    nextString += lstrlenA(nextString) + 1;
                }
                else
                    pkgInfo->Name = NULL;
                if (info[i].Comment)
                {
                    pkgInfo->Comment = nextString;
                    /* just repeat back to WideCharToMultiByte how many bytes
                     * it requires, since we asked it earlier
                     */
                    bytes = WideCharToMultiByte(CP_ACP, 0, info[i].Comment, -1,
                     NULL, 0, NULL, NULL);
                    WideCharToMultiByte(CP_ACP, 0, info[i].Comment, -1,
                     pkgInfo->Comment, bytes, NULL, NULL);
                    nextString += lstrlenA(nextString) + 1;
                }
                else
                    pkgInfo->Comment = NULL;
            }
        }
    }
    else
        ret = NULL;
    return ret;
}

/***********************************************************************
 *      EnumerateSecurityPackagesA (SECUR32.@)
 */
SECURITY_STATUS WINAPI schan_EnumerateSecurityPackagesA(PULONG pcPackages,
 PSecPkgInfoA *ppPackageInfo)
{
    SECURITY_STATUS ret;
    PSecPkgInfoW info;

    ret = schan_EnumerateSecurityPackagesW(pcPackages, &info);
    if (ret == SEC_E_OK && *pcPackages && info)
    {
        *ppPackageInfo = thunk_PSecPkgInfoWToA(*pcPackages, info);
        if (*pcPackages && !*ppPackageInfo)
        {
            *pcPackages = 0;
            ret = SEC_E_INSUFFICIENT_MEMORY;
        }
        schan_FreeContextBuffer(info);
    }
    return ret;
}

SECURITY_STATUS
WINAPI
schan_FreeContextBuffer (
    PVOID pvoid
    )
{
    HeapFree(GetProcessHeap(), 0, pvoid);
    return SEC_E_OK;
}



static PWSTR SECUR32_strdupW(PCWSTR str)
{
    PWSTR ret;

    if (str)
    {
        ret = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(str) + 1) * sizeof(WCHAR));
        if (ret)
            lstrcpyW(ret, str);
    }
    else
        ret = NULL;
    return ret;
}

PWSTR SECUR32_AllocWideFromMultiByte(PCSTR str)
{
    PWSTR ret;

    if (str)
    {
        int charsNeeded = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);

        if (charsNeeded)
        {
            ret = HeapAlloc(GetProcessHeap(), 0, charsNeeded * sizeof(WCHAR));
            if (ret)
                MultiByteToWideChar(CP_ACP, 0, str, -1, ret, charsNeeded);
        }
        else
            ret = NULL;
    }
    else
        ret = NULL;
    return ret;
}

PSTR SECUR32_AllocMultiByteFromWide(PCWSTR str)
{
    PSTR ret;

    if (str)
    {
        int charsNeeded = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0,
         NULL, NULL);

        if (charsNeeded)
        {
            ret = HeapAlloc(GetProcessHeap(), 0, charsNeeded);
            if (ret)
                WideCharToMultiByte(CP_ACP, 0, str, -1, ret, charsNeeded,
                 NULL, NULL);
        }
        else
            ret = NULL;
    }
    else
        ret = NULL;
    return ret;
}

static void _copyPackageInfo(PSecPkgInfoW info, const SecPkgInfoA *inInfoA,
 const SecPkgInfoW *inInfoW)
{
    if (info && (inInfoA || inInfoW))
    {
        /* odd, I know, but up until Name and Comment the structures are
         * identical
         */
        memcpy(info, inInfoW ? inInfoW : (const SecPkgInfoW *)inInfoA, sizeof(*info));
        if (inInfoW)
        {
            info->Name = SECUR32_strdupW(inInfoW->Name);
            info->Comment = SECUR32_strdupW(inInfoW->Comment);
        }
        else
        {
            info->Name = SECUR32_AllocWideFromMultiByte(inInfoA->Name);
            info->Comment = SECUR32_AllocWideFromMultiByte(inInfoA->Comment);
        }
    }
}

SecureProvider *SECUR32_addProvider(const SecurityFunctionTableA *fnTableA,
 const SecurityFunctionTableW *fnTableW, PCWSTR moduleName)
{
    SecureProvider *ret;

    EnterCriticalSection(&cs);

    if (!providerTable)
    {
        providerTable = HeapAlloc(GetProcessHeap(), 0, sizeof(SecureProviderTable));
        if (!providerTable)
        {
            LeaveCriticalSection(&cs);
            return NULL;
        }

        list_init(&providerTable->table);
    }

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(SecureProvider));
    if (!ret)
    {
        LeaveCriticalSection(&cs);
        return NULL;
    }

    list_add_tail(&providerTable->table, &ret->entry);
    ret->lib = NULL;

#ifndef __REACTOS__
    if (fnTableA || fnTableW)
    {
        ret->moduleName = moduleName ? SECUR32_strdupW(moduleName) : NULL;
        _makeFnTableA(&ret->fnTableA, fnTableA, fnTableW);
        _makeFnTableW(&ret->fnTableW, fnTableA, fnTableW);
        ret->loaded = !moduleName;
    }
    else
#endif
    {
        ret->moduleName = SECUR32_strdupW(moduleName);
        ret->loaded = FALSE;
    }

    LeaveCriticalSection(&cs);
    return ret;
}

void SECUR32_addPackages(SecureProvider *provider, ULONG toAdd,
 const SecPkgInfoA *infoA, const SecPkgInfoW *infoW)
{
    ULONG i;

    assert(provider);
    assert(infoA || infoW);

    EnterCriticalSection(&cs);

    if (!packageTable)
    {
        packageTable = HeapAlloc(GetProcessHeap(), 0, sizeof(SecurePackageTable));
        if (!packageTable)
        {
            LeaveCriticalSection(&cs);
            return;
        }

        packageTable->numPackages = 0;
        list_init(&packageTable->table);
    }

    for (i = 0; i < toAdd; i++)
    {
        SecurePackage *package = HeapAlloc(GetProcessHeap(), 0, sizeof(SecurePackage));
        if (!package)
            continue;

        list_add_tail(&packageTable->table, &package->entry);

        package->provider = provider;
        _copyPackageInfo(&package->infoW,
            infoA ? &infoA[i] : NULL,
            infoW ? &infoW[i] : NULL);
    }
    packageTable->numPackages += toAdd;

    LeaveCriticalSection(&cs);
}
