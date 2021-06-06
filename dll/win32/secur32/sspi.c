#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(secur32);

extern CRITICAL_SECTION cs;
extern SecurePackageTable *packageTable;

SECURITY_STATUS WINAPI ApplyControlTokenW(PCtxtHandle Handle, PSecBufferDesc Buffer);
SECURITY_STATUS WINAPI ApplyControlTokenA(PCtxtHandle Handle, PSecBufferDesc Buffer);

SecurityFunctionTableA securityFunctionTableA =
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
    ApplyControlTokenA,
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

SecurityFunctionTableW securityFunctionTableW =
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
    ApplyControlTokenW,
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
 *		EnumerateSecurityPackagesW (SECUR32.@)
 */
SECURITY_STATUS WINAPI EnumerateSecurityPackagesW(PULONG pcPackages,
 PSecPkgInfoW *ppPackageInfo)
{
    SECURITY_STATUS ret = SEC_E_OK;

    TRACE("(%p, %p)\n", pcPackages, ppPackageInfo);

    *ppPackageInfo = NULL;
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
                        TRACE("Name[%d] = %s\n", i - 1, debugstr_w(package->infoW.Name));
                        pkgInfo->Name = nextString;
                        lstrcpyW(nextString, package->infoW.Name);
                        nextString += lstrlenW(nextString) + 1;
                    }
                    else
                        pkgInfo->Name = NULL;
                    if (package->infoW.Comment)
                    {
                        TRACE("Comment[%d] = %s\n", i - 1, debugstr_w(package->infoW.Comment));
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
PSecPkgInfoA thunk_PSecPkgInfoWToA(ULONG cPackages,
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
 *		EnumerateSecurityPackagesA (SECUR32.@)
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
ApplyControlTokenW(PCtxtHandle Handle,
                  PSecBufferDesc Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

SECURITY_STATUS
WINAPI
ApplyControlTokenA(PCtxtHandle Handle,
                  PSecBufferDesc Buffer)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    return &securityFunctionTableW;
}


PSecurityFunctionTableA
WINAPI
InitSecurityInterfaceA(VOID)
{
    return &securityFunctionTableA;
}

