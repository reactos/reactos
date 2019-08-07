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
    WCHAR DnsHostNameW[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR NetBIOSNameW[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR NetBIOSNameA[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR DomainNameA[256];
} GLOBAL_INFO_SEC_BUFFER;
GLOBAL_INFO_SEC_BUFFER g_sb;

void NtlmCheckInit()
{
    DWORD cchHNLen = ARRAY_SIZE(g_sb.DnsHostNameW);
    DWORD cchNBLen = ARRAY_SIZE(g_sb.DnsHostNameW);
    LPWKSTA_INFO_100 pWksInfo = NULL;
    WCHAR DomainNameW[256];

    InitializeCriticalSection(&g_sb.cs);

    memset(&g_sb.osVerInfo, 0, sizeof(g_sb.osVerInfo));
    g_sb.osVerInfo.dwOSVersionInfoSize = sizeof(g_sb.osVerInfo);
    if (!GetVersionEx(&g_sb.osVerInfo))
    {
        sync_err("failed to get osversionsinfo!\n");
        memset(&g_sb.osVerInfo, 0, sizeof(g_sb.osVerInfo));
    }

    if (!GetComputerNameExW(ComputerNameDnsHostname, g_sb.DnsHostNameW, &cchHNLen))
        g_sb.DnsHostNameW[0] = 0;
    if (!GetComputerNameExW(ComputerNameNetBIOS, g_sb.NetBIOSNameW, &cchNBLen))
        g_sb.NetBIOSNameW[0] = 0;

    DomainNameW[0] = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, (LPBYTE*)&pWksInfo))
    {
        if (FAILED(StringCchCopyW(DomainNameW, ARRAY_SIZE(DomainNameW),
                                  pWksInfo->wki100_langroup)))
            DomainNameW[0] = 0;
        NetApiBufferFree(pWksInfo);
    }
    if (DomainNameW[0] == 0)
    {
        sync_err("could not get domain name!\n");
        StringCchCopyW(DomainNameW, 10, L"WORKGROUP");
    };

    /* Convert W -> A */
    if (!WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             DomainNameW, -1, g_sb.DomainNameA,
                             ARRAY_SIZE(g_sb.DomainNameA), NULL, NULL))
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
    pData = (PBYTE)msg + pblob->Offset;
    if (blobChLen == strlen(expected))
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

    pData = (PBYTE)msg + pblob->Offset;
    if (blobChLen == wcslen(expected))
        isEqual = (wcsncmp((WCHAR*)pData, expected, blobChLen) == 0);

    sync_ok(isEqual, "%s: blob \"%s\" is %.*S, expected %S\n",
            testName, blobName, blobChLen,
            pData, expected);
}

void NtlmCheckTargetInfoAvl(
    char* testName,
    char* blobName,
    void* msg,
    PNTLM_BLOB pblob)
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
            sync_ok(wcsncmp(avpCmpStr, (WCHAR*)pData, blobChLen) == 01,
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
    sync_ok(hasMsvAvTimestamp, "avl %s: missing entry MsvAvTimestamp!\n", blobName);
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

void NtlmCheckSecBuffer_CliAuthInit(char* testName, PBYTE buffer)
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
    sync_ok(pNego->NegotiateFlags == 0xe208b2b7,
            "%s: pChallenge->NegotiateFlags is %x, expected %x!\n",
            testName, pNego->NegotiateFlags, 0xe208b2b7);
    NtlmCheckBlobA(testName, "OemDomainName",
        g_sb.DomainNameA,
        pNego, &pNego->OemDomainName);
    NtlmCheckBlobA(testName, "OemWorkstationName",
        g_sb.NetBIOSNameA,
        pNego, &pNego->OemWorkstationName);
    NtlmCheckWinVer(testName, &pNego->Version, &g_sb.osVerInfo);
}

void NtlmCheckSecBuffer_SvrAuth(char* testName, PBYTE buffer)
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
    sync_ok(pChallenge->NegotiateFlags == 0xe28ac235,
            "%s: pChallenge->NegotiateFlags is %x, expected %x!\n",
            testName, pChallenge->NegotiateFlags, 0xe28ac235);
    //TODO ServerChallenge[MSV1_0_CHALLENGE_LENGTH];
    //TODO Reserved[8];
    NtlmCheckTargetInfoAvl(testName, "TargetInfo",
        pChallenge, &pChallenge->TargetInfo);
    //FIXME: This is win 7!
    NtlmCheckWinVer(testName, &pChallenge->Version, &g_sb.osVerInfo);
}

void NtlmCheckSecBuffer_CliAuth(char* testName, PBYTE buffer)
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

    NtlmCheckBlobW(testName, "LmChallengeResponse",
        L"",
        pAuth, &pAuth->LmChallengeResponse);
    NtlmCheckBlobW(testName, "NtChallengeResponse",
        L"",
        pAuth, &pAuth->NtChallengeResponse);
    NtlmCheckBlobW(testName, "DomainName",
        L"",
        pAuth, &pAuth->DomainName);
    NtlmCheckBlobW(testName, "UserName",
        L"",
        pAuth, &pAuth->UserName);
    NtlmCheckBlobW(testName, "WorkstationName",
        L"",
        pAuth, &pAuth->WorkstationName);
    NtlmCheckBlobW(testName, "EncryptedRandomSessionKey",
        L"",
        pAuth, &pAuth->EncryptedRandomSessionKey);
    sync_ok(pAuth->NegotiateFlags == 0xe288c235,
            "%s: pAuth->NegotiateFlags is %x, expected %x!\n",
            testName, pAuth->NegotiateFlags, 0xe288c235);
    NtlmCheckWinVer(testName, &pAuth->Version, &g_sb.osVerInfo);
    // MIC[16]; //doc says its ommited in nt,2k,xp,2k3
}

void NtlmCheckSecBuffer(
    IN int TESTSEC_idx,
    IN PBYTE buffer)
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
            NtlmCheckSecBuffer_CliAuthInit("cli-init", buffer);
            break;
        }
        case TESTSEC_SVR_AUTH:
        {
            NtlmCheckSecBuffer_SvrAuth("svr-auth", buffer);
            break;
        }
        case TESTSEC_CLI_AUTH_FINI:
        {
            NtlmCheckSecBuffer_CliAuth("cli-auth", buffer);
            break;
        }
    }

    LeaveCriticalSection(&g_sb.cs);

    sync_trace("*** *** ***<<\n");
}
