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
CRITICAL_SECTION GlobalCritSect; /* use to read/write global state */

NTLM_MODE NtlmMode = NtlmUserMode; /* FIXME: No LSA mode support */
UNICODE_STRING NtlmComputerNameString;
UNICODE_STRING NtlmDomainNameString;
UNICODE_STRING NtlmDnsNameString;
UNICODE_STRING NtlmAvTargetInfo;
OEM_STRING NtlmOemComputerNameString;
OEM_STRING NtlmOemDomainNameString;
OEM_STRING NtlmOemDnsNameString;
HANDLE NtlmSystemSecurityToken;

/* private functions */

NTSTATUS
NtlmInitializeGlobals(VOID)
{
    NTSTATUS status = STATUS_SUCCESS;
    LPWKSTA_USER_INFO_1 pBuf = NULL;
    WCHAR compName[CNLEN + 1], domName[DNLEN+1], dnsName[256];
    ULONG compNamelen = sizeof(compName), dnsNamelen = sizeof(dnsName);
    PMSV1_0_AV_PAIR pAvPairs;
    ULONG AvPairsLen;
    InitializeCriticalSection(&GlobalCritSect);

    if(!GetComputerNameW(compName, &compNamelen))
    {
        compName[0] = L'\0';
        ERR("could not get computer name!\n");
    }
    TRACE("%s\n",debugstr_w(compName));

    if (!GetComputerNameExW(ComputerNameDnsFullyQualified, dnsName, &dnsNamelen))
    {
        dnsName[0] = L'\0';
        ERR("could not get dns name!\n");
    }
    TRACE("%s\n",debugstr_w(dnsName));

	/* FIXME: this still does not match what msv1_0 returns */
    if (!(NERR_Success == NetWkstaUserGetInfo(0, 1, (LPBYTE*)&pBuf)))
    {
        wcscpy(domName, L"WORKGROUP");
        ERR("could not get domain name!\n");
    }
    else
    {
        wcscpy(domName, pBuf->wkui1_logon_domain);
    }

    if (pBuf != NULL)
        NetApiBufferFree(pBuf);
        ERR("%s\n",debugstr_w(domName));

    RtlCreateUnicodeString(&NtlmComputerNameString, compName);
    RtlCreateUnicodeString(&NtlmDnsNameString, dnsName);
    RtlCreateUnicodeString(&NtlmDomainNameString, domName);

    RtlUnicodeStringToOemString(&NtlmOemComputerNameString,
                                &NtlmComputerNameString,
                                TRUE);

    RtlUnicodeStringToOemString(&NtlmOemDomainNameString,
                                &NtlmDomainNameString,
                                TRUE);

    RtlUnicodeStringToOemString(&NtlmOemDnsNameString,
                                &NtlmDnsNameString,
                                TRUE);

    status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY | TOKEN_DUPLICATE,
                                &NtlmSystemSecurityToken);

    if(!NT_SUCCESS(status))
    {
        ERR("could not get process token!!\n");
    }
    //FIX ME: This is broken
    /* init global target AV pairs */
    RtlInitUnicodeString(&NtlmAvTargetInfo, NULL);
    AvPairsLen = NtlmDomainNameString.Length + //fix me: domain controller name
           NtlmComputerNameString.Length + //computer name
           NtlmDnsNameString.Length + //dns computer name
           NtlmDnsNameString.Length + //fix me: dns domain name
           sizeof(MSV1_0_AV_PAIR)*4;

    NtlmAvTargetInfo.Buffer = (PWSTR)NtlmAllocate(AvPairsLen);

    if(!NtlmAvTargetInfo.Buffer)
    {
        ERR("failed to allocate NtlmAvTargetInfo\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pAvPairs = NtlmAvlInit(NtlmAvTargetInfo.Buffer);
    NtlmAvlAdd(pAvPairs, MsvAvNbDomainName, &NtlmDomainNameString, AvPairsLen);
    NtlmAvlAdd(pAvPairs, MsvAvNbComputerName, &NtlmComputerNameString, AvPairsLen);
    NtlmAvlAdd(pAvPairs, MsvAvDnsDomainName, &NtlmDnsNameString, AvPairsLen);
    NtlmAvlAdd(pAvPairs, MsvAvDnsComputerName, &NtlmDnsNameString, AvPairsLen);
    NtlmAvTargetInfo.Length = (USHORT)NtlmAvlLen(pAvPairs, AvPairsLen);

    ERR("NtlmAvTargetInfo %S\n", NtlmAvTargetInfo.Buffer);
    NtlmPrintAvPairs(pAvPairs);
    return status;
}

VOID
NtlmTerminateGlobals(VOID)
{
    NtlmFree(NtlmComputerNameString.Buffer);
    NtlmFree(NtlmDomainNameString.Buffer);
    NtlmFree(NtlmDnsNameString.Buffer);
    NtlmFree(NtlmOemComputerNameString.Buffer);
    NtlmFree(NtlmOemDomainNameString.Buffer);
    NtlmFree(NtlmOemDnsNameString.Buffer);
    NtlmFree(NtlmAvTargetInfo.Buffer);
    NtClose(NtlmSystemSecurityToken);
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
