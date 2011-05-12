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
#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(secur32);

/**
 *  Globals
 */

static const WCHAR securityProvidersKeyW[] = {
 'S','Y','S','T','E','M','\\','C','u','r','r','e','n','t','C','o','n','t','r',
 'o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','S','e','c','u','r',
 'i','t','y','P','r','o','v','i','d','e','r','s','\0'
 };
static const WCHAR securityProvidersW[] = {
 'S','e','c','u','r','i','t','y','P','r','o','v','i','d','e','r','s',0
 };

CRITICAL_SECTION cs;
SecurePackageTable *packageTable = NULL;
SecureProviderTable *providerTable = NULL;

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

SecureProvider *SECUR32_addProvider(const SecurityFunctionTableA *fnTableA,
 const SecurityFunctionTableW *fnTableW, PCWSTR moduleName)
{
    SecureProvider *ret;

    EnterCriticalSection(&cs);

    TRACE("Adding %s provider\n", debugstr_w(moduleName));

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
        ret->loaded = moduleName ? FALSE : TRUE;
    }
    else
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

    ASSERT(provider);
    ASSERT(infoA || infoW);

    EnterCriticalSection(&cs);

    TRACE("Will add %d packages\n",toAdd);

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

        TRACE("added: %s\n",debugstr_w(package->infoW.Name));
    }
    packageTable->numPackages += toAdd;

    LeaveCriticalSection(&cs);
}

BOOL LoadSSPProvider(PWSTR moduleName)
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

        TRACE("loaded %s, InitSecurityInterfaceA is %p, InitSecurityInterfaceW is %p\n",
         debugstr_w(moduleName), pInitSecurityInterfaceA,
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
                    WARN("%s has built-in providers, skip adding\n", debugstr_w(moduleName));
            }
            else if (fnTableA && fnTableA->EnumerateSecurityPackagesA)
            {
                if (fnTableA != &securityFunctionTableA)
                    ret = fnTableA->EnumerateSecurityPackagesA(&toAdd, &infoA);
                else
                    WARN("%s has built-in providers, skip adding\n", debugstr_w(moduleName));
            }
            if (ret == SEC_E_OK && toAdd > 0 && (infoW || infoA))
            {
                SecureProvider *provider = SECUR32_addProvider(NULL, NULL, moduleName);

                if (provider)
                    SECUR32_addPackages(provider, toAdd, infoA, infoW);
                if (infoW)
                    fnTableW->FreeContextBuffer(infoW);
                else
                    fnTableA->FreeContextBuffer(infoA);
            }
        }
        FreeLibrary(lib);
        return TRUE;
    }

    ERR("failed to load %s\n", debugstr_w(moduleName));
    return FALSE;
}

void SECUR32_initializeProviders(void)
{
    HKEY key;
    LSTATUS apiRet;

    TRACE("\n");
    InitializeCriticalSection(&cs);

    /* load providers from registry */
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
                    LoadSSPProvider(ptr);
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
                 (INIT_SECURITY_INTERFACE_W)GetProcAddress(ret->provider->lib,
                 SECURITY_ENTRYPOINT_ANSIW);
                INIT_SECURITY_INTERFACE_A pInitSecurityInterfaceA =
                 (INIT_SECURITY_INTERFACE_A)GetProcAddress(ret->provider->lib,
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

    if (packageTable && packageName)
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

void SECUR32_freeProviders(void)
{
    TRACE("\n");
    EnterCriticalSection(&cs);

    if (packageTable)
    {
        SecurePackage *package, *package_next;
        LIST_FOR_EACH_ENTRY_SAFE(package, package_next, &packageTable->table,
                                 SecurePackage, entry)
        {
            HeapFree(GetProcessHeap(), 0, package->infoW.Name);
            HeapFree(GetProcessHeap(), 0, package->infoW.Comment);
            HeapFree(GetProcessHeap(), 0, package);
        }

        HeapFree(GetProcessHeap(), 0, packageTable);
        packageTable = NULL;
    }

    if (providerTable)
    {
        SecureProvider *provider, *provider_next;
        LIST_FOR_EACH_ENTRY_SAFE(provider, provider_next, &providerTable->table,
                                 SecureProvider, entry)
        {
            HeapFree(GetProcessHeap(), 0, provider->moduleName);
            if (provider->lib)
                FreeLibrary(provider->lib);
            HeapFree(GetProcessHeap(), 0, provider);
        }

        HeapFree(GetProcessHeap(), 0, providerTable);
        providerTable = NULL;
    }

    LeaveCriticalSection(&cs);
    DeleteCriticalSection(&cs);
}

