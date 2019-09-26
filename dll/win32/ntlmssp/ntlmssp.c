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
    /* FIXME value is stored in registry ... so get it from there! */
    g->LMCompatibilityLevel = 2;// partly unimplemented
    /* LMCompatibilityLevel - matrix
     * cli/DC  lvl   LM-     NTLMv1-   NTLMv2   v2-Session-
     *               auth.   auth.     auth.     Security
     * cli      0    use     use       -         never
     * DC       0    accept  accept    accept    accept
     * cli      1    use     use                 use if svr supports it
     * DC       1    accept  accept    -         accept
     * cli      2    -       use       -         use if svr supports it
     * DC       2    accept  accept    accept    accept
     * cli      3    -       -         use       use if svr supports it
     * DC       3    accept  accept    accept    accept
     * cli      4    -       -         use       use if svr supports it
     * DC       4    refuse  accept    accept    accept
     * cli      5    -       -         use       use if svr supports it
     * DC       5    refuse  refuse    accept    accept
     *
     * W2k-default = 2 */

    /* FIXME implement the following options ...
       Send LM & NTLM responses - never NTLMv2 */
    //#define NTLMSSP_LMCOMPLVL_LM_NTLM 0;
    /* Send LM & NTLM - use NTLMv2 session security if negotiated */
    //#define NTLMSSP_LMCOMPLVL_LM_NTLM_NTLMv2 1
    /* Send NTLM responses only */
    //#define NTLMSSP_LMCOMPLVL_NTLM 2 // w2k default
    /* Send NTLMv2 responses only */
    //#define NTLMSSP_LMCOMPLVL_NTLMv2 3
    /* Send NTLMv2 responses only. Refuse LM */
    //#define NTLMSSP_LMCOMPLVL_NTLMv2_NoLM 4
    /* Send NTLMv2 responses only. Refuse LM & NTLM */
    //#define NTLMSSP_LMCOMPLVL_NTLMv2_NoLM_NTLM 5;

    switch (g->LMCompatibilityLevel)
    {
        case 0 :
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_LM |
                               CLI_LMFLAG_USE_AUTH_NTLMv1;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
        case 1 :
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_LM |
                               CLI_LMFLAG_USE_AUTH_NTLMv1 |
                               CLI_LMFLAG_USE_SSEC_NTLMv2;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
        case 2:
        default:
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_NTLMv1 |
                               CLI_LMFLAG_USE_SSEC_NTLMv2;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
        case 3 :
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_NTLMv2 |
                               CLI_LMFLAG_USE_SSEC_NTLMv2;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
        case 4 :
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_NTLMv2 |
                               CLI_LMFLAG_USE_SSEC_NTLMv2;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
        case 5 :
        {
            gcli->CliLMLevel = CLI_LMFLAG_USE_AUTH_NTLMv2 |
                               CLI_LMFLAG_USE_SSEC_NTLMv2;
            gsvr->SvrLMLevel = SVR_LMFLAG_ACCPT_AUTH_LM |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv1 |
                               SVR_LMFLAG_ACCPT_AUTH_NTLMv2;
            break;
        }
    }
    /* server supported features */
    gsvr->CfgFlg = NTLMSSP_NEGOTIATE_UNICODE |
                   NTLMSSP_NEGOTIATE_OEM |
                   NTLMSSP_NEGOTIATE_LM_KEY |
                   NTLMSSP_NEGOTIATE_NTLM |
                   NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY |
                   NTLMSSP_REQUEST_TARGET |
                   NTLMSSP_TARGET_TYPE_SERVER |
                   NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                   NTLMSSP_NEGOTIATE_SEAL |
                   NTLMSSP_NEGOTIATE_SIGN |
                   NTLMSSP_NEGOTIATE_TARGET_INFO |
                   NTLMSSP_NEGOTIATE_VERSION |
                   NTLMSSP_NEGOTIATE_KEY_EXCH |
                   NTLMSSP_NEGOTIATE_56 |
                   NTLMSSP_NEGOTIATE_128;
    gcli->ClientConfigFlags =
                   NTLMSSP_NEGOTIATE_UNICODE |
                   NTLMSSP_NEGOTIATE_OEM |
                   NTLMSSP_NEGOTIATE_LM_KEY |
                   NTLMSSP_NEGOTIATE_NTLM |
                   NTLMSSP_REQUEST_TARGET |
                   NTLMSSP_TARGET_TYPE_SERVER |
                   NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                   NTLMSSP_NEGOTIATE_SEAL |
                   NTLMSSP_NEGOTIATE_SIGN |
                   NTLMSSP_NEGOTIATE_TARGET_INFO |
                   NTLMSSP_NEGOTIATE_VERSION |
                   NTLMSSP_NEGOTIATE_KEY_EXCH |
                   NTLMSSP_NEGOTIATE_56 |
                   NTLMSSP_NEGOTIATE_128;
    if (gcli->CliLMLevel & CLI_LMFLAG_USE_SSEC_NTLMv2)
        gcli->ClientConfigFlags |= NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;

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
