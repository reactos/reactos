/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Andreas Maier <staubim@quantentunnel.de>
 */

#include "client_server.h"

/* Test-Data */
typedef struct _GLOBAL_INFO_SEC_BUFFER
{
    /* its used by server / client - so we have to protect it! */
    CRITICAL_SECTION cs;
    OSVERSIONINFO osVerInfo;
    NETSETUP_JOIN_STATUS joinState;
    WCHAR DnsHostNameW[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR NetBIOSNameW[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR NetBIOSNameA[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR DomainNameA[DNLEN];
    WCHAR DomainNameW[DNLEN];
    /* Auth-Message DomainName or ComputerName */
    WCHAR* authMsg_WorkstNameW;
} GLOBAL_INFO_SEC_BUFFER;
GLOBAL_INFO_SEC_BUFFER g_sb;

void NtlmCheckInit()
{
    DWORD cchHNLen = ARRAY_SIZE(g_sb.DnsHostNameW);
    DWORD cchNBLen = ARRAY_SIZE(g_sb.DnsHostNameW);
    LPWSTR DomainNameW;
    LPWKSTA_INFO_100 pWksInfo = NULL;

    InitializeCriticalSection(&g_sb.cs);

    memset(&g_sb.osVerInfo, 0, sizeof(g_sb.osVerInfo));
    g_sb.osVerInfo.dwOSVersionInfoSize = sizeof(g_sb.osVerInfo);
    if (!GetVersionEx(&g_sb.osVerInfo))
    {
        sync_err("failed to get osversionsinfo!\n");
        memset(&g_sb.osVerInfo, 0, sizeof(g_sb.osVerInfo));
    }

    if (NetGetJoinInformation(NULL, &DomainNameW, &g_sb.joinState) != NERR_Success)
    {
        sync_err("failed to get domain join state!\n");
        g_sb.joinState = NetSetupUnknownStatus;
    }
    //NetApiBufferFree(&DomainNameW);

    if (!GetComputerNameExW(ComputerNameDnsHostname, g_sb.DnsHostNameW, &cchHNLen))
        g_sb.DnsHostNameW[0] = 0;
    if (!GetComputerNameExW(ComputerNameNetBIOS, g_sb.NetBIOSNameW, &cchNBLen))
        g_sb.NetBIOSNameW[0] = 0;

    g_sb.DomainNameW[0] = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, (LPBYTE*)&pWksInfo))
    {
        if (FAILED(StringCchCopyW(g_sb.DomainNameW, DNLEN,
                                  pWksInfo->wki100_langroup)))
            g_sb.DomainNameW[0] = 0;
        NetApiBufferFree(pWksInfo);
    }
    if (g_sb.DomainNameW[0] == 0)
    {
        sync_err("could not get domain name!\n");
        StringCchCopyW(g_sb.DomainNameW, 10, L"WORKGROUP");
    };

    /* Convert W -> A */
    if (!WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             g_sb.DomainNameW, -1, g_sb.DomainNameA,
                             DNLEN, NULL, NULL))
    {
        sync_err("could not convert domainname (W->A)!\n");
        g_sb.DomainNameA[0] = 0;
    }

    if (!WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             g_sb.NetBIOSNameW, -1, g_sb.NetBIOSNameA, 128, NULL, NULL))
    {
        sync_err("could not convert NetBIOSName (W->A)!\n");
        g_sb.NetBIOSNameA[0] = 0;
    }

    if (g_sb.joinState == NetSetupDomainName)
        g_sb.authMsg_WorkstNameW = g_sb.DomainNameW;
    else
        g_sb.authMsg_WorkstNameW = g_sb.NetBIOSNameW;
}

void NtlmCheckFini()
{
    DeleteCriticalSection(&g_sb.cs);
}

void NtlmCheckBlobA(
    char* testName,
    char* blobName,
    char* expected,
    void* msg,
    PNTLM_BLOB pblob)
{
    PBYTE pData;
    BOOL isEqual = FALSE;
    int blobChLen = pblob->Length / sizeof(char);
    int expectedChLen = (expected == NULL) ? 0 : strlen(expected);

    pData = (PBYTE)msg + pblob->Offset;
    if (blobChLen == expectedChLen)
        isEqual = (strncmp((char*)pData, expected, blobChLen) == 0);

    sync_ok(isEqual, "%s: blob \"%s\" is %.*s, expected %s\n",
            testName, blobName, blobChLen,
            pData, expected);
}

void NtlmCheckBlobW(
    char* testName,
    char* blobName,
    WCHAR* expected,
    void* msg,
    PNTLM_BLOB pblob)
{
    PBYTE pData;
    BOOL isEqual = FALSE;
    int blobChLen = pblob->Length / sizeof(WCHAR);
    int expectedChLen = (expected == NULL) ? 0 : wcslen(expected);

    pData = (PBYTE)msg + pblob->Offset;
    if (blobChLen == expectedChLen)
        isEqual = (wcsncmp((WCHAR*)pData, expected, blobChLen) == 0);

    sync_ok(isEqual, "%s: blob \"%s\" is %.*S, expected %S\n",
            testName, blobName, blobChLen,
            pData, expected);
}

void NtlmCheckTargetInfoAvl(
    char* testName,
    char* blobName,
    void* msg,
    PNTLM_BLOB pblob,
    PAUTH_TEST_DATA_SVR ptest)
{
    PBYTE pData;
    PMSV1_0_AV_PAIR pAvp;
    PBYTE pEnd;
    int entryCount = 0;
    WCHAR* avpCmpStr;
    WCHAR* avpCmpName;

    BOOL hasMsvAvNbDomainName = FALSE;
    BOOL hasMsvAvNbComputerName = FALSE;
    BOOL hasMsvAvDnsDomainName = FALSE;
    BOOL hasMsvAvDnsComputerName = FALSE;
    BOOL hasMsvAvTimestamp = FALSE;
    BOOL hasMsvAvEOL = FALSE;

    pData = (PBYTE)msg + pblob->Offset;
    pEnd  = pData + pblob->Length;

    while (pData < pEnd)
    {
        pAvp = (PMSV1_0_AV_PAIR)pData;
        pData += sizeof(MSV1_0_AV_PAIR);

        if (hasMsvAvEOL)
            sync_err("avl %s: data after eol!\n", blobName);

        avpCmpStr = NULL;
        avpCmpName = NULL;

        switch (pAvp->AvId)
        {
            case MsvAvEOL:
            {
                hasMsvAvEOL = TRUE;
                break;
            }
            case MsvAvNbComputerName:
            {
                avpCmpName = L"MsvAvNbComputerName";
                avpCmpStr = g_sb.NetBIOSNameW;
                sync_ok(!hasMsvAvNbComputerName, "avl %s: duplicate!\n", blobName);
                hasMsvAvNbComputerName = TRUE;
                break;
            }
            case MsvAvNbDomainName:
            {
                avpCmpName = L"MsvAvNbDomainName";
                avpCmpStr = g_sb.NetBIOSNameW;
                sync_ok(!hasMsvAvNbDomainName, "avl %s: duplicate!\n", blobName);
                hasMsvAvNbDomainName = TRUE;
                break;
            }
            case MsvAvDnsComputerName:
            {
                avpCmpName = L"MsvAvDnsComputerName";
                avpCmpStr = g_sb.DnsHostNameW;
                sync_ok(!hasMsvAvDnsComputerName, "avl %s: duplicate!\n", blobName);
                hasMsvAvDnsComputerName = TRUE;
                break;
            }
            case MsvAvDnsDomainName:
            {
                avpCmpName = L"MsvAvDnsDomainName";
                avpCmpStr = g_sb.DnsHostNameW;
                sync_ok(!hasMsvAvDnsDomainName, "avl %s: duplicate!\n", blobName);
                hasMsvAvDnsDomainName = TRUE;
                break;
            }
            case MsvAvTimestamp:
            {
                sync_ok(!hasMsvAvTimestamp, "avl %s: duplicate!\n", blobName);
                hasMsvAvTimestamp = TRUE;
                break;
            }
            default:
            {
                sync_err("avl %s: unexpected entry. (AvId %d)\n", blobName, pAvp->AvId);
                break;
            }
        }

        if (avpCmpStr != NULL)
        {
            int blobChLen = pAvp->AvLen / sizeof(WCHAR);
            sync_ok(wcsncmp(avpCmpStr, (WCHAR*)pData, blobChLen) == 0,
                    "avl %s: %S: expected %S, got %.*S.\n",
                    blobName, avpCmpName, avpCmpStr,
                    blobChLen, (WCHAR*)pData);
        }
        pData += pAvp->AvLen;
        entryCount++;
    }

    sync_ok(hasMsvAvNbComputerName, "avl %s: missing entry MsvAvNbComputerName!\n", blobName);
    sync_ok(hasMsvAvNbDomainName, "avl %s: missing entry MsvAvNbDomainName!\n", blobName);
    sync_ok(hasMsvAvDnsComputerName, "avl %s: missing entry MsvAvDnsComputerName!\n", blobName);
    sync_ok(hasMsvAvDnsDomainName, "avl %s: missing entry MsvAvDnsDomainName!\n", blobName);
    if (ptest->ChaMsg_hasAvTimestamp)
        sync_ok(hasMsvAvTimestamp,"avl %s: missing entry MsvAvTimestamp!\n", blobName);
    else
        sync_ok(!hasMsvAvTimestamp,"avl %s: entry MsvAvTimestamp not needed!\n", blobName);
    sync_ok(hasMsvAvEOL, "avl %s: missing entry MsvAvEOL!\n", blobName);

    PrintHexDumpMax(pblob->Length, (PBYTE)msg + pblob->Offset, 1024);
}

void NtlmCheckWinVer(
    char* testName,
    PNTLM_WINDOWS_VERSION pVer,
    POSVERSIONINFO pVerInfo)
{
    int expMajor = pVerInfo->dwMajorVersion;
    int expMinor = pVerInfo->dwMinorVersion;
    int expBuild = pVerInfo->dwBuildNumber;
    /* FIXME: How to get this information? */
    int expRevision = 15; // w2k3 + w7
    sync_ok(pVer->ProductMajor == expMajor,
            "%s: WinVer.ProductMajor is %d, expected %d.\n",
            testName, pVer->ProductMajor, expMajor);
    sync_ok(pVer->ProductMinor == expMinor,
            "%s: WinVer.ProductMinor is %d, expected %d.\n",
            testName, pVer->ProductMinor, expMinor);
    sync_ok(pVer->ProductBuild == expBuild,
            "%s: WinVer.ProductBuild is %d, expected %d.\n",
            testName, pVer->ProductBuild, expBuild);
    sync_ok(pVer->NtlmRevisionCurrent == expRevision,
            "%s: WinVer.NtlmRevisionCurrent is %d, expected %d.\n",
            testName, pVer->NtlmRevisionCurrent, expRevision);
}

void NtlmCheckSecBuffer_CliAuthInit(
    IN char* testName,
    IN PBYTE buffer,
    IN PAUTH_TEST_DATA_CLI ptest)
{
    PNTLM_MESSAGE_HEAD pHdr;
    PNEGOTIATE_MESSAGE pNego;

    pHdr = (PNTLM_MESSAGE_HEAD)buffer;
    sync_ok(pHdr->MsgType == NtlmNegotiate,
            "Wrong MsgType type %x\n", pHdr->MsgType);
    if (pHdr->MsgType != NtlmNegotiate)
        return;

    pNego = (PNEGOTIATE_MESSAGE)pHdr;
    sync_ok(strncmp(pNego->Signature, NTLMSSP_SIGNATURE, 8) == 0,
        "invalid signature (%.8s)", pNego->Signature);
    sync_ok(pNego->NegotiateFlags == ptest->NegMsg_NegotiateFlags,
            "%s: pNego->NegotiateFlags is %x, expected %x!\n",
            testName, pNego->NegotiateFlags, ptest->NegMsg_NegotiateFlags);
    NtlmCheckBlobA(testName, "OemDomainName",
        "",//g_sb.DomainNameA,
        pNego, &pNego->OemDomainName);
    NtlmCheckBlobA(testName, "OemWorkstationName",
        "",//g_sb.NetBIOSNameA,
        pNego, &pNego->OemWorkstationName);
    NtlmCheckWinVer(testName, &pNego->Version, &g_sb.osVerInfo);
}

void NtlmCheckSecBuffer_SvrAuth(
    IN char* testName,
    IN PBYTE buffer,
    IN PAUTH_TEST_DATA_SVR ptest)
{
    PNTLM_MESSAGE_HEAD pHdr;
    PCHALLENGE_MESSAGE pChallenge;

    pHdr = (PNTLM_MESSAGE_HEAD)buffer;
    sync_ok(pHdr->MsgType == NtlmChallenge,
            "Wrong MsgType type %x\n", pHdr->MsgType);
    if (pHdr->MsgType != NtlmChallenge)
        return;

    pChallenge = (PCHALLENGE_MESSAGE)pHdr;
    sync_ok(strncmp(pChallenge->Signature, NTLMSSP_SIGNATURE, 8) == 0,
        "invalid signature (%.8s)", pChallenge->Signature);
    NtlmCheckBlobW(testName, "TargetName",
        g_sb.NetBIOSNameW,
        pChallenge, &pChallenge->TargetName);
    sync_ok(pChallenge->NegotiateFlags == ptest->ChaMsg_NegotiateFlags,
            "%s: pChallenge->NegotiateFlags is %x, expected %x!\n",
            testName, pChallenge->NegotiateFlags,
            ptest->ChaMsg_NegotiateFlags);
    //TODO ServerChallenge[MSV1_0_CHALLENGE_LENGTH];
    //TODO Reserved[8];
    NtlmCheckTargetInfoAvl(testName, "TargetInfo",
        pChallenge, &pChallenge->TargetInfo, ptest);
    //FIXME: This is win 7!
    NtlmCheckWinVer(testName, &pChallenge->Version, &g_sb.osVerInfo);
}

void NtlmCheckSecBuffer_CliAuth(
    IN char* testName,
    IN PBYTE buffer,
    IN PAUTH_TEST_DATA_CLI ptest)
{
    PNTLM_MESSAGE_HEAD pHdr;
    PAUTHENTICATE_MESSAGE pAuth;

    pHdr = (PNTLM_MESSAGE_HEAD)buffer;
    sync_ok(pHdr->MsgType == NtlmAuthenticate,
            "Wrong MsgType type %x\n", pHdr->MsgType);
    if (pHdr->MsgType != NtlmAuthenticate)
        return;

    pAuth = (PAUTHENTICATE_MESSAGE)pHdr;
    sync_ok(strncmp(pAuth->Signature, NTLMSSP_SIGNATURE, 8) == 0,
        "invalid signature (%.8s)", pAuth->Signature);

    //NtlmCheckBlobW(testName, "LmChallengeResponse",
    //    L"",
    //    pAuth, &pAuth->LmChallengeResponse);
    //NtlmCheckBlobW(testName, "NtChallengeResponse",
    //    L"",
    //    pAuth, &pAuth->NtChallengeResponse);
    NtlmCheckBlobW(testName, "DomainName",
        ptest->userdom,
        pAuth, &pAuth->DomainName);
    NtlmCheckBlobW(testName, "UserName",
        ptest->user,
        pAuth, &pAuth->UserName);
    NtlmCheckBlobW(testName, "WorkstationName",
        g_sb.authMsg_WorkstNameW,
        pAuth, &pAuth->WorkstationName);
    //NtlmCheckBlobW(testName, "EncryptedRandomSessionKey",
    //    L"",
    //    pAuth, &pAuth->EncryptedRandomSessionKey);
    sync_ok(pAuth->NegotiateFlags == ptest->AuthMsg_NegotiateFlags,
            "%s: pAuth->NegotiateFlags is %x, expected %x!\n",
            testName, pAuth->NegotiateFlags, ptest->AuthMsg_NegotiateFlags);
    NtlmCheckWinVer(testName, &pAuth->Version, &g_sb.osVerInfo);
    // MIC[16]; //doc says its ommited in nt,2k,xp,2k3
}

void NtlmCheckSecBuffer(
    IN int TESTSEC_idx,
    IN PBYTE buffer,
    IN PCLI_PARAMS pcp,
    IN PSVR_PARAMS psp)
{
    sync_trace("*** *** ***>>\n");

    sync_ok(buffer != NULL, "buf is null!\n");
    if (!buffer)
        return;

    EnterCriticalSection(&g_sb.cs);

    switch (TESTSEC_idx)
    {
        case TESTSEC_CLI_AUTH_INIT:
        {
            NtlmCheckSecBuffer_CliAuthInit("cli-nego", buffer, pcp->ptest);
            break;
        }
        case TESTSEC_SVR_AUTH:
        {
            NtlmCheckSecBuffer_SvrAuth("svr-chal", buffer, psp->ptest);
            break;
        }
        case TESTSEC_CLI_AUTH_FINI:
        {
            NtlmCheckSecBuffer_CliAuth("cli-auth", buffer, pcp->ptest);
            break;
        }
    }

    LeaveCriticalSection(&g_sb.cs);

    sync_trace("*** *** ***<<\n");
}
