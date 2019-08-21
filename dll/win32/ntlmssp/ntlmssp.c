/*
 * Copyright 2011 Samuel Serapión
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
 *
 */
#include "ntlmssp.h"
#include "protocol.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* globals */
NTLM_MODE NtlmMode = NtlmUserMode; /* FIXME: No LSA mode support */

NTLMSSP_GLOBALS ntlmGlobals;
NTLMSSP_GLOBALS_CLI ntlmGlobalsCli;
NTLMSSP_GLOBALS_SVR ntlmGlobalsSvr;

PNTLMSSP_GLOBALS
lockGlobals(VOID)
{
    EnterCriticalSection(&ntlmGlobals.cs);
    return &ntlmGlobals;
}

PNTLMSSP_GLOBALS
getGlobals(VOID)
{
    return &ntlmGlobals;
}

VOID
unlockGlobals(
    IN OUT PNTLMSSP_GLOBALS* g)
{
    LeaveCriticalSection(&(*g)->cs);
    *g = NULL;
}

PNTLMSSP_GLOBALS_CLI
lockGlobalsCli(VOID)
{
    EnterCriticalSection(&ntlmGlobalsCli.cs);
    return &ntlmGlobalsCli;
}

VOID
unlockGlobalsCli(
    IN OUT PNTLMSSP_GLOBALS_CLI* g)
{
    LeaveCriticalSection(&(*g)->cs);
    *g = NULL;
}

PNTLMSSP_GLOBALS_CLI
getGlobalsCli(VOID)
{
    return &ntlmGlobalsCli;
}

PNTLMSSP_GLOBALS_SVR
lockGlobalsSvr(VOID)
{
    EnterCriticalSection(&ntlmGlobalsSvr.cs);
    return &ntlmGlobalsSvr;
}

VOID
unlockGlobalsSvr(
    IN OUT PNTLMSSP_GLOBALS_SVR *g)
{
    LeaveCriticalSection(&(*g)->cs);
    *g = NULL;
}

PNTLMSSP_GLOBALS_SVR
getGlobalsSvr(VOID)
{
    return &ntlmGlobalsSvr;
}

/* private functions */

NTSTATUS
NtlmInitializeGlobals(VOID)
{
    NTSTATUS status = STATUS_SUCCESS;
    LPWKSTA_INFO_100 pBuf = NULL;
    WCHAR compName[CNLEN + 1], domName[DNLEN+1], dnsName[256];
    ULONG compNamelen = sizeof(compName), dnsNamelen = sizeof(dnsName);
    ULONG AvPairsLen;
    /* shortcuts */
    PNTLMSSP_GLOBALS g = &ntlmGlobals;
    PNTLMSSP_GLOBALS_SVR gsvr = &ntlmGlobalsSvr;
    PNTLMSSP_GLOBALS_CLI gcli = &ntlmGlobalsCli;

    InitializeCriticalSection(&g->cs);
    InitializeCriticalSection(&gcli->cs);
    InitializeCriticalSection(&gsvr->cs);

    /* maybe read from registry ... */
    /*  NTLMV1 works */
    /*  NTLMV2 not fully working (AUTH_MESSAGE receives INVALID_PARAMETER :-( ) */
    gcli->CfgFlags = NTLMSSP_CLICFGFLAG_NTLMV1_ENABLED;
    gcli->LMCompatibilityLevel = 2;// totally unimplemented

    if(!GetComputerNameW(compName, &compNamelen))
    {
        compName[0] = L'\0';
        ERR("could not get computer name!\n");
    }
    TRACE("%s\n",debugstr_w(compName));

    if (!GetComputerNameExW(ComputerNameDnsHostname, dnsName, &dnsNamelen))
    {
        dnsName[0] = L'\0';
        ERR("could not get dns name!\n");
    }
    TRACE("%s\n",debugstr_w(dnsName));

    if (NERR_Success == NetWkstaGetInfo(0, 100, (LPBYTE*)&pBuf))
    {
        wcscpy(domName, pBuf->wki100_langroup);
        NetApiBufferFree(pBuf);
    }
    else
    {
        wcscpy(domName, L"WORKGROUP");
        ERR("could not get domain name!\n");
    }

    ERR("%s\n", debugstr_w(domName));

    ExtWStrInit(&gsvr->NbMachineName, compName);
    ExtWStrInit(&gsvr->DnsMachineName, dnsName);
    ExtWStrInit(&gsvr->NbDomainName, domName);

    ExtWStrToAStr(&g->NbMachineNameOEM,
                  &gsvr->NbMachineName, TRUE, TRUE);

    ExtWStrToAStr(&g->NbDomainNameOEM,
                  &gsvr->NbDomainName, TRUE, TRUE);

    ExtWStrToAStr(&gsvr->DnsMachineNameOEM,
                  &gsvr->DnsMachineName, TRUE, TRUE);

    status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY | TOKEN_DUPLICATE,
                                &g->NtlmSystemSecurityToken);

    if(!NT_SUCCESS(status))
    {
        ERR("could not get process token!!\n");
    }
    /* init global target AV pairs */
    AvPairsLen = gsvr->NbDomainName.bUsed + //fix me: domain controller name
                 gsvr->NbMachineName.bUsed + //computer name
                 gsvr->DnsMachineName.bUsed + //dns computer name
                 gsvr->DnsMachineName.bUsed + //fix me: dns domain name
                 sizeof(MSV1_0_AV_PAIR)*4;

    if (!NtlmAvlInit(&gsvr->NtlmAvTargetInfoPart, AvPairsLen))
    {
        ERR("failed to allocate NtlmAvTargetInfo\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill NtlmAvTargetInfoPart. It contains not all data we need.
     * Timestamp and EOL is appended when challange message is
     * generated. */
    if (gsvr->NbMachineName.bUsed > 0)
        NtlmAvlAdd(&gsvr->NtlmAvTargetInfoPart, MsvAvNbComputerName,
                   gsvr->NbMachineName.Buffer, gsvr->NbMachineName.bUsed);
    if (gsvr->NbDomainName.bUsed > 0)
        NtlmAvlAdd(&gsvr->NtlmAvTargetInfoPart, MsvAvNbDomainName,
                   gsvr->NbDomainName.Buffer, gsvr->NbDomainName.bUsed);
    if (gsvr->DnsMachineName.bUsed > 0)
        NtlmAvlAdd(&gsvr->NtlmAvTargetInfoPart, MsvAvDnsComputerName,
                   gsvr->DnsMachineName.Buffer, gsvr->DnsMachineName.bUsed);
    /* FIXME: This is not correct! - (same value as above??) */
    if (gsvr->DnsMachineName.bUsed > 0)
        NtlmAvlAdd(&gsvr->NtlmAvTargetInfoPart, MsvAvDnsDomainName,
                   gsvr->DnsMachineName.Buffer, gsvr->DnsMachineName.bUsed);
    //TODO: MsvAvDnsTreeName

    ERR("NtlmAvTargetInfoPart len 0x%x\n", gsvr->NtlmAvTargetInfoPart.bUsed);
    NtlmPrintAvPairs(&gsvr->NtlmAvTargetInfoPart);
    return status;
}

VOID
NtlmTerminateGlobals(VOID)
{
    /* shortcuts */
    PNTLMSSP_GLOBALS g = &ntlmGlobals;
    PNTLMSSP_GLOBALS_SVR gsvr = &ntlmGlobalsSvr;
    PNTLMSSP_GLOBALS_CLI gcli = &ntlmGlobalsCli;

    ExtStrFree(&gsvr->NbMachineName);
    ExtStrFree(&gsvr->NbDomainName);
    ExtStrFree(&gsvr->DnsMachineName);
    ExtStrFree(&g->NbMachineNameOEM);
    ExtStrFree(&g->NbDomainNameOEM);
    ExtStrFree(&gsvr->DnsMachineNameOEM);
    NtlmAvlFree(&gsvr->NtlmAvTargetInfoPart);
    NtClose(g->NtlmSystemSecurityToken);

    DeleteCriticalSection(&g->cs);
    DeleteCriticalSection(&gcli->cs);
    DeleteCriticalSection(&gsvr->cs);
}

/* public functions */

static SecurityFunctionTableA ntlmTableA = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesA,
    QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    InitializeSecurityContextA,    /* InitializeSecurityContextA */
    AcceptSecurityContext,         /* AcceptSecurityContext */
    CompleteAuthToken,             /* CompleteAuthToken */
    DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    QueryContextAttributesA,       /* QueryContextAttributesA */
    ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    RevertSecurityContext,         /* RevertSecurityContext */
    MakeSignature,                 /* MakeSignature */
    VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoA */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextA */
    NULL,   /* AddCredentialsA */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    EncryptMessage,                /* EncryptMessage */
    DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesA */
};

static SecurityFunctionTableW ntlmTableW = {
    SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
    EnumerateSecurityPackagesW,   /* EnumerateSecurityPackagesW */
    QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    InitializeSecurityContextW,    /* InitializeSecurityContextW */
    AcceptSecurityContext,         /* AcceptSecurityContext */
    CompleteAuthToken,             /* CompleteAuthToken */
    DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    QueryContextAttributesW,       /* QueryContextAttributesW */
    ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    RevertSecurityContext,         /* RevertSecurityContext */
    MakeSignature,                 /* MakeSignature */
    VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoW */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextW */
    NULL,   /* AddCredentialsW */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    EncryptMessage,                /* EncryptMessage */
    DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesW */
};

SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesA(OUT unsigned long* pcPackages,
                           OUT PSecPkgInfoA * ppPackageInfo)
{
    SECURITY_STATUS ret;

    ret = QuerySecurityPackageInfoA(NULL, ppPackageInfo);

    *pcPackages = 1;
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(OUT unsigned long* pcPackages,
                           OUT PSecPkgInfoW * ppPackageInfo)
{
    SECURITY_STATUS ret;

    ret = QuerySecurityPackageInfoW(NULL, ppPackageInfo);

    *pcPackages = 1;
    return ret;
}


PSecurityFunctionTableA
SEC_ENTRY
InitSecurityInterfaceA(void)
{
    return &ntlmTableA;
}

PSecurityFunctionTableW
SEC_ENTRY
InitSecurityInterfaceW(void)
{
    return &ntlmTableW;
}

SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoA(SEC_CHAR *pszPackageName,
                          PSecPkgInfoA *ppPackageInfo)
{
    SECURITY_STATUS ret;
    size_t bytesNeeded = sizeof(SecPkgInfoA);
    int nameLen = 0, commentLen = 0;

    TRACE("%s %p\n", pszPackageName, ppPackageInfo);

    /* get memory needed */
    nameLen = strlen(NTLM_NAME_A) + 1;
    bytesNeeded += nameLen * sizeof(CHAR);
    commentLen = strlen(NTLM_COMMENT_A) + 1;
    bytesNeeded += commentLen * sizeof(CHAR);

    /* allocate it */
    *ppPackageInfo = HeapAlloc(GetProcessHeap(), 0, bytesNeeded);

    if (*ppPackageInfo)
    {
        PSTR nextString = (PSTR)((PBYTE)*ppPackageInfo +
            sizeof(SecPkgInfoA));

        /* copy easy stuff */
        (*ppPackageInfo)->fCapabilities = NTLM_CAPS;
        (*ppPackageInfo)->wVersion = 1;
        (*ppPackageInfo)->wRPCID = RPC_C_AUTHN_WINNT;
        (*ppPackageInfo)->cbMaxToken = NTLM_MAX_BUF;

        /* copy strings */
        (*ppPackageInfo)->Name = nextString;
        strncpy(nextString, NTLM_NAME_A, nameLen);
        nextString += nameLen;

        (*ppPackageInfo)->Comment = nextString;
        strncpy(nextString, NTLM_COMMENT_A, commentLen);
        nextString += commentLen;
        
        ret = SEC_E_OK;
    }
    else
        ret = SEC_E_INSUFFICIENT_MEMORY;
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoW(SEC_WCHAR *pszPackageName,
                          PSecPkgInfoW *ppPackageInfo)
{
    SECURITY_STATUS ret;
    size_t bytesNeeded = sizeof(SecPkgInfoW);
    int nameLen = 0, commentLen = 0;

    TRACE("%s %p\n", debugstr_w(pszPackageName), ppPackageInfo);

    /* get memory needed */
    nameLen = lstrlenW(NTLM_NAME_W) + 1;
    bytesNeeded += nameLen * sizeof(WCHAR);
    commentLen = lstrlenW(NTLM_COMMENT_W) + 1;
    bytesNeeded += commentLen * sizeof(WCHAR);

    /* allocate it */
    *ppPackageInfo = HeapAlloc(GetProcessHeap(), 0, bytesNeeded);

    if (*ppPackageInfo)
    {
        PWSTR nextString = (PWSTR)((PBYTE)*ppPackageInfo +
            sizeof(SecPkgInfoW));

        /* copy easy stuff */
        (*ppPackageInfo)->fCapabilities = NTLM_CAPS;
        (*ppPackageInfo)->wVersion = 1;
        (*ppPackageInfo)->wRPCID = RPC_C_AUTHN_WINNT;
        (*ppPackageInfo)->cbMaxToken = NTLM_MAX_BUF;

        /* copy strings */
        (*ppPackageInfo)->Name = nextString;
        lstrcpynW(nextString, NTLM_NAME_W, nameLen);
        nextString += nameLen;

        (*ppPackageInfo)->Comment = nextString;
        lstrcpynW(nextString, NTLM_COMMENT_W, commentLen);
        nextString += commentLen;
        
        ret = SEC_E_OK;
    }
    else
        ret = SEC_E_INSUFFICIENT_MEMORY;
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
CompleteAuthToken(PCtxtHandle phContext,
                  PSecBufferDesc pToken)
{
    TRACE("%p %p\n", phContext, pToken);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;
    return SEC_E_OK;
}
