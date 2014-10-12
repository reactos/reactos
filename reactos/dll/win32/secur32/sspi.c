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

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(secur32);

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

static void SECUR32_initializeProviders(void);

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

static SecurityFunctionTableA securityFunctionTableA =
{
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesA,
    QueryCredentialsAttributesA,
    AcquireCredentialsHandleA,
    FreeCredentialsHandle,
    NULL, /* Reserved2 */
    InitializeSecurityContextA,
    AcceptSecurityContext,
    CompleteAuthToken,
    DeleteSecurityContext,
    ApplyControlToken,
    QueryContextAttributesA,
    ImpersonateSecurityContext,
    RevertSecurityContext,
    MakeSignature,
    VerifySignature,
    FreeContextBuffer,
    QuerySecurityPackageInfoA,
    EncryptMessage, /* Reserved3 */
    DecryptMessage, /* Reserved4 */
    ExportSecurityContext,
    ImportSecurityContextA,
    AddCredentialsA,
    NULL, /* Reserved8 */
    QuerySecurityContextToken,
    EncryptMessage,
    DecryptMessage,
    NULL
};

static SecurityFunctionTableW securityFunctionTableW =
{
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesW,
    QueryCredentialsAttributesW,
    AcquireCredentialsHandleW,
    FreeCredentialsHandle,
    NULL, /* Reserved2 */
    InitializeSecurityContextW,
    AcceptSecurityContext,
    CompleteAuthToken,
    DeleteSecurityContext,
    ApplyControlToken,
    QueryContextAttributesW,
    ImpersonateSecurityContext,
    RevertSecurityContext,
    MakeSignature,
    VerifySignature,
    FreeContextBuffer,
    QuerySecurityPackageInfoW,
    EncryptMessage, /* Reserved3 */
    DecryptMessage, /* Reserved4 */
    ExportSecurityContext,
    ImportSecurityContextW,
    AddCredentialsW,
    NULL, /* Reserved8 */
    QuerySecurityContextToken,
    EncryptMessage,
    DecryptMessage,
    NULL
};

/***********************************************************************
 *      EnumerateSecurityPackagesW (SECUR32.@)
 */
SECURITY_STATUS WINAPI EnumerateSecurityPackagesW(PULONG pcPackages,
 PSecPkgInfoW *ppPackageInfo)
{
    SECURITY_STATUS ret = SEC_E_OK;

    TRACE("(%p, %p)\n", pcPackages, ppPackageInfo);

#ifdef __REACTOS__
    if (!packageTable)
        SECUR32_initializeProviders();
#endif

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
SECURITY_STATUS WINAPI EnumerateSecurityPackagesA(PULONG pcPackages,
 PSecPkgInfoA *ppPackageInfo)
{
    SECURITY_STATUS ret;
    PSecPkgInfoW info;

    ret = EnumerateSecurityPackagesW(pcPackages, &info);
    if (ret == SEC_E_OK && *pcPackages && info)
    {
        *ppPackageInfo = thunk_PSecPkgInfoWToA(*pcPackages, info);
        if (*pcPackages && !*ppPackageInfo)
        {
            *pcPackages = 0;
            ret = SEC_E_INSUFFICIENT_MEMORY;
        }
        FreeContextBuffer(info);
    }
    return ret;
}

SECURITY_STATUS
WINAPI
FreeContextBuffer (
	PVOID pvoid
	)
{
    HeapFree(GetProcessHeap(), 0, pvoid);
    return SEC_E_OK;
}

PSecurityFunctionTableW
WINAPI
InitSecurityInterfaceW(VOID)
{
    TRACE("InitSecurityInterfaceW() called\n");
    return &securityFunctionTableW;
}

PSecurityFunctionTableA
WINAPI
InitSecurityInterfaceA(VOID)
{
    TRACE("InitSecurityInterfaceA() called\n");
    return &securityFunctionTableA;
}

BOOLEAN
WINAPI
TranslateNameA(
	LPCSTR lpAccountName,
	EXTENDED_NAME_FORMAT AccountNameFormat,
	EXTENDED_NAME_FORMAT DesiredNameFormat,
	LPSTR lpTranslatedName,
	PULONG nSize
)
{
	UNIMPLEMENTED;
	return FALSE;
}

BOOLEAN
WINAPI
TranslateNameW(
	LPCWSTR lpAccountName,
	EXTENDED_NAME_FORMAT AccountNameFormat,
	EXTENDED_NAME_FORMAT DesiredNameFormat,
	LPWSTR lpTranslatedName,
	PULONG nSize
)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*** PRIVATE FUNCTIONS ***************************************************************************/

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

static void _makeFnTableA(PSecurityFunctionTableA fnTableA,
 const SecurityFunctionTableA *inFnTableA,
 const SecurityFunctionTableW *inFnTableW)
{
    if (fnTableA)
    {
        if (inFnTableA)
        {
            /* The size of the version 1 table is based on platform sdk's
             * sspi.h, though the sample ssp also provided with platform sdk
             * implies only functions through QuerySecurityPackageInfoA are
             * implemented (yikes)
             */
            size_t tableSize = inFnTableA->dwVersion == 1 ?
             (const BYTE *)&inFnTableA->SetContextAttributesA -
             (const BYTE *)inFnTableA : sizeof(SecurityFunctionTableA);

            memcpy(fnTableA, inFnTableA, tableSize);
            /* override this, since we can do it internally anyway */
            fnTableA->QuerySecurityPackageInfoA =
             QuerySecurityPackageInfoA;
        }
        else if (inFnTableW)
        {
            /* functions with thunks */
            if (inFnTableW->AcquireCredentialsHandleW)
                fnTableA->AcquireCredentialsHandleA =
                 thunk_AcquireCredentialsHandleA;
            if (inFnTableW->InitializeSecurityContextW)
                fnTableA->InitializeSecurityContextA =
                 thunk_InitializeSecurityContextA;
            if (inFnTableW->ImportSecurityContextW)
                fnTableA->ImportSecurityContextA =
                 thunk_ImportSecurityContextA;
            if (inFnTableW->AddCredentialsW)
                fnTableA->AddCredentialsA =
                 thunk_AddCredentialsA;
            if (inFnTableW->QueryCredentialsAttributesW)
                fnTableA->QueryCredentialsAttributesA =
                 thunk_QueryCredentialsAttributesA;
            if (inFnTableW->QueryContextAttributesW)
                fnTableA->QueryContextAttributesA =
                 thunk_QueryContextAttributesA;
            if (inFnTableW->SetContextAttributesW)
                fnTableA->SetContextAttributesA =
                 thunk_SetContextAttributesA;
            /* this can't be thunked, there's no extra param to know which
             * package to forward to */
            fnTableA->EnumerateSecurityPackagesA = NULL;
            /* functions with no thunks needed */
            fnTableA->AcceptSecurityContext = inFnTableW->AcceptSecurityContext;
            fnTableA->CompleteAuthToken = inFnTableW->CompleteAuthToken;
            fnTableA->DeleteSecurityContext = inFnTableW->DeleteSecurityContext;
            fnTableA->ImpersonateSecurityContext =
             inFnTableW->ImpersonateSecurityContext;
            fnTableA->RevertSecurityContext = inFnTableW->RevertSecurityContext;
            fnTableA->MakeSignature = inFnTableW->MakeSignature;
            fnTableA->VerifySignature = inFnTableW->VerifySignature;
            fnTableA->FreeContextBuffer = inFnTableW->FreeContextBuffer;
            fnTableA->QuerySecurityPackageInfoA =
             QuerySecurityPackageInfoA;
            fnTableA->ExportSecurityContext =
             inFnTableW->ExportSecurityContext;
            fnTableA->QuerySecurityContextToken =
             inFnTableW->QuerySecurityContextToken;
            fnTableA->EncryptMessage = inFnTableW->EncryptMessage;
            fnTableA->DecryptMessage = inFnTableW->DecryptMessage;
        }
    }
}

static void _makeFnTableW(PSecurityFunctionTableW fnTableW,
 const SecurityFunctionTableA *inFnTableA,
 const SecurityFunctionTableW *inFnTableW)
{
    if (fnTableW)
    {
        if (inFnTableW)
        {
            /* The size of the version 1 table is based on platform sdk's
             * sspi.h, though the sample ssp also provided with platform sdk
             * implies only functions through QuerySecurityPackageInfoA are
             * implemented (yikes)
             */
            size_t tableSize = inFnTableW->dwVersion == 1 ?
             (const BYTE *)&inFnTableW->SetContextAttributesW -
             (const BYTE *)inFnTableW : sizeof(SecurityFunctionTableW);

            memcpy(fnTableW, inFnTableW, tableSize);
            /* override this, since we can do it internally anyway */
            fnTableW->QuerySecurityPackageInfoW =
             QuerySecurityPackageInfoW;
        }
        else if (inFnTableA)
        {
            /* functions with thunks */
            if (inFnTableA->AcquireCredentialsHandleA)
                fnTableW->AcquireCredentialsHandleW =
                 thunk_AcquireCredentialsHandleW;
            if (inFnTableA->InitializeSecurityContextA)
                fnTableW->InitializeSecurityContextW =
                 thunk_InitializeSecurityContextW;
            if (inFnTableA->ImportSecurityContextA)
                fnTableW->ImportSecurityContextW =
                 thunk_ImportSecurityContextW;
            if (inFnTableA->AddCredentialsA)
                fnTableW->AddCredentialsW =
                 thunk_AddCredentialsW;
            if (inFnTableA->QueryCredentialsAttributesA)
                fnTableW->QueryCredentialsAttributesW =
                 thunk_QueryCredentialsAttributesW;
            if (inFnTableA->QueryContextAttributesA)
                fnTableW->QueryContextAttributesW =
                 thunk_QueryContextAttributesW;
            if (inFnTableA->SetContextAttributesA)
                fnTableW->SetContextAttributesW =
                 thunk_SetContextAttributesW;
            /* this can't be thunked, there's no extra param to know which
             * package to forward to */
            fnTableW->EnumerateSecurityPackagesW = NULL;
            /* functions with no thunks needed */
            fnTableW->AcceptSecurityContext = inFnTableA->AcceptSecurityContext;
            fnTableW->CompleteAuthToken = inFnTableA->CompleteAuthToken;
            fnTableW->DeleteSecurityContext = inFnTableA->DeleteSecurityContext;
            fnTableW->ImpersonateSecurityContext =
             inFnTableA->ImpersonateSecurityContext;
            fnTableW->RevertSecurityContext = inFnTableA->RevertSecurityContext;
            fnTableW->MakeSignature = inFnTableA->MakeSignature;
            fnTableW->VerifySignature = inFnTableA->VerifySignature;
            fnTableW->FreeContextBuffer = inFnTableA->FreeContextBuffer;
            fnTableW->QuerySecurityPackageInfoW =
             QuerySecurityPackageInfoW;
            fnTableW->ExportSecurityContext =
             inFnTableA->ExportSecurityContext;
            fnTableW->QuerySecurityContextToken =
             inFnTableA->QuerySecurityContextToken;
            fnTableW->EncryptMessage = inFnTableA->EncryptMessage;
            fnTableW->DecryptMessage = inFnTableA->DecryptMessage;
        }
    }
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

static
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

    if (fnTableA || fnTableW)
    {
        ret->moduleName = moduleName ? SECUR32_strdupW(moduleName) : NULL;
        _makeFnTableA(&ret->fnTableA, fnTableA, fnTableW);
        _makeFnTableW(&ret->fnTableW, fnTableA, fnTableW);
        ret->loaded = !moduleName;
    }
    else
    {
        ret->moduleName = SECUR32_strdupW(moduleName);
        ret->loaded = FALSE;
    }

    LeaveCriticalSection(&cs);
    return ret;
}

static
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

static void _tryLoadProvider(PWSTR moduleName)
{
    HMODULE lib = LoadLibraryW(moduleName);

    if (lib)
    {
        INIT_SECURITY_INTERFACE_W pInitSecurityInterfaceW =
         (INIT_SECURITY_INTERFACE_W)GetProcAddress(lib,
         SECURITY_ENTRYPOINT_ANSIW);
        INIT_SECURITY_INTERFACE_A pInitSecurityInterfaceA =
         (INIT_SECURITY_INTERFACE_A)GetProcAddress(lib,
         SECURITY_ENTRYPOINT_ANSIA);

        TRACE("loaded %S, InitSecurityInterfaceA is %p, InitSecurityInterfaceW is %p\n",
         moduleName, pInitSecurityInterfaceA,
         pInitSecurityInterfaceW);
        if (pInitSecurityInterfaceW || pInitSecurityInterfaceA)
        {
            PSecurityFunctionTableA fnTableA = NULL;
            PSecurityFunctionTableW fnTableW = NULL;
            ULONG toAdd = 0;
            PSecPkgInfoA infoA = NULL;
            PSecPkgInfoW infoW = NULL;
            SECURITY_STATUS ret = SEC_E_OK;

            if (pInitSecurityInterfaceA)
                fnTableA = pInitSecurityInterfaceA();
            if (pInitSecurityInterfaceW)
                fnTableW = pInitSecurityInterfaceW();
            if (fnTableW && fnTableW->EnumerateSecurityPackagesW)
            {
                if (fnTableW != &securityFunctionTableW)
                    ret = fnTableW->EnumerateSecurityPackagesW(&toAdd, &infoW);
                else
                    TRACE("%S has built-in providers, skip adding\n", moduleName);
            }
            else if (fnTableA && fnTableA->EnumerateSecurityPackagesA)
            {
                if (fnTableA != &securityFunctionTableA)
                    ret = fnTableA->EnumerateSecurityPackagesA(&toAdd, &infoA);
                else
                    TRACE("%S has built-in providers, skip adding\n", moduleName);
            }
            if (ret == SEC_E_OK && toAdd > 0 && (infoW || infoA))
            {
                SecureProvider *provider = SECUR32_addProvider(NULL, NULL,
                 moduleName);

                if (provider)
                    SECUR32_addPackages(provider, toAdd, infoA, infoW);
                if (infoW)
                    fnTableW->FreeContextBuffer(infoW);
                else
                    fnTableA->FreeContextBuffer(infoA);
            }
        }
        FreeLibrary(lib);
    }
    else
        WARN("failed to load %S\n", moduleName);
}

static const WCHAR securityProvidersKeyW[] = {
 'S','Y','S','T','E','M','\\','C','u','r','r','e','n','t','C','o','n','t','r',
 'o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','S','e','c','u','r',
 'i','t','y','P','r','o','v','i','d','e','r','s','\0'
 };
static const WCHAR securityProvidersW[] = {
 'S','e','c','u','r','i','t','y','P','r','o','v','i','d','e','r','s',0
 };

 /* FIXME: we're missing SECUR32_freeProviders, so all of this gets leaked */
static void SECUR32_initializeProviders(void)
{
    HKEY key;
    LSTATUS apiRet;

    /* Now load providers from registry */
    apiRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, securityProvidersKeyW, 0,
     KEY_READ, &key);
    if (apiRet == ERROR_SUCCESS)
    {
        WCHAR securityPkgNames[MAX_PATH]; /* arbitrary len */
        DWORD size = sizeof(securityPkgNames) / sizeof(WCHAR), type;

        apiRet = RegQueryValueExW(key, securityProvidersW, NULL, &type,
         (PBYTE)securityPkgNames, &size);
        if (apiRet == ERROR_SUCCESS && type == REG_SZ)
        {
            WCHAR *ptr;

            size = size / sizeof(WCHAR);
            for (ptr = securityPkgNames;
              ptr < securityPkgNames + size; )
            {
                WCHAR *comma;

                for (comma = ptr; *comma && *comma != ','; comma++)
                    ;
                if (*comma == ',')
                    *comma = '\0';
                for (; *ptr && isspace(*ptr) && ptr < securityPkgNames + size;
                 ptr++)
                    ;
                if (*ptr)
                    _tryLoadProvider(ptr);
                ptr += lstrlenW(ptr) + 1;
            }
        }
        RegCloseKey(key);
    }
}

SecurePackage *SECUR32_findPackageW(PCWSTR packageName)
{
    SecurePackage *ret = NULL;
    BOOL matched = FALSE;

#ifdef __REACTOS__
    if (!packageTable)
        SECUR32_initializeProviders();
#endif

    if (packageTable && packageName)
    {
        LIST_FOR_EACH_ENTRY(ret, &packageTable->table, SecurePackage, entry)
        {
            matched = !lstrcmpiW(ret->infoW.Name, packageName);
            if (matched)
                break;
        }

        if (!matched)
            return NULL;

        if (ret->provider && !ret->provider->loaded)
        {
            ret->provider->lib = LoadLibraryW(ret->provider->moduleName);
            if (ret->provider->lib)
            {
                INIT_SECURITY_INTERFACE_W pInitSecurityInterfaceW =
                    (INIT_SECURITY_INTERFACE_W) GetProcAddress(ret->provider->lib,
                    SECURITY_ENTRYPOINT_ANSIW);
                INIT_SECURITY_INTERFACE_A pInitSecurityInterfaceA =
                    (INIT_SECURITY_INTERFACE_A) GetProcAddress(ret->provider->lib,
                    SECURITY_ENTRYPOINT_ANSIA);
                PSecurityFunctionTableA fnTableA = NULL;
                PSecurityFunctionTableW fnTableW = NULL;

                if (pInitSecurityInterfaceA)
                    fnTableA = pInitSecurityInterfaceA();
                if (pInitSecurityInterfaceW)
                    fnTableW = pInitSecurityInterfaceW();
                /* don't update built-in SecurityFunctionTable */
                if (fnTableA != &securityFunctionTableA)
                    _makeFnTableA(&ret->provider->fnTableA, fnTableA, fnTableW);
                if (fnTableW != &securityFunctionTableW)
                    _makeFnTableW(&ret->provider->fnTableW, fnTableA, fnTableW);
                ret->provider->loaded = TRUE;
            }
            else
                ret = NULL;
        }
    }
    return ret;
}

SecurePackage *SECUR32_findPackageA(PCSTR packageName)
{
    SecurePackage *ret;

    if (packageName)
    {
        UNICODE_STRING package;

        RtlCreateUnicodeStringFromAsciiz(&package, packageName);
        ret = SECUR32_findPackageW(package.Buffer);
        RtlFreeUnicodeString(&package);
    }
    else
        ret = NULL;
    return ret;
}
