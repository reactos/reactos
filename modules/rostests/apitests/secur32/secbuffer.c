/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Andreas Maier <staubim@quantentunnel.de>
 */

#include "client_server.h"

/* Test-Data */
typedef struct _GLOBAL_INFO
{
    /* its used by server / client - so we have to protect it! */
    CRITICAL_SECTION cs;
    OSVERSIONINFO osVerInfo;
    WCHAR DnsHostNameW[MAX_COMPUTERNAME_LENGTH];
    WCHAR NetBIOSNameW[MAX_COMPUTERNAME_LENGTH];
    CHAR NetBIOSNameA[128];
    CHAR DomainNameA[256];
    int Auth_TargetInfoDataLen;
    BYTE Auth_TargetInfoData[500];
} GLOBAL_INFO;
GLOBAL_INFO g;

void NtlmCheckInit()
{
    int i;
    DWORD cbHNLen, cchHNLen = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD cbNBLen, cchNBLen = MAX_COMPUTERNAME_LENGTH + 1;
    LPWKSTA_INFO_100 pWksInfo = NULL;
    WCHAR DomainNameW[256];

    InitializeCriticalSection(&g.cs);

    memset(&g.osVerInfo, 0, sizeof(g.osVerInfo));
    g.osVerInfo.dwOSVersionInfoSize = sizeof(g.osVerInfo);
    if (!GetVersionEx(&g.osVerInfo))
    {
        sync_err("failed to get osversionsinfo!\n");
        memset(&g.osVerInfo, 0, sizeof(g.osVerInfo));
    }

    if (!GetComputerNameExW(ComputerNameDnsHostname, g.DnsHostNameW, &cchHNLen))
        g.DnsHostNameW[0] = 0;
    if (!GetComputerNameExW(ComputerNameNetBIOS, g.NetBIOSNameW, &cchNBLen))
        g.NetBIOSNameW[0] = 0;
    cbHNLen = sizeof(WCHAR) * cchHNLen;
    cbNBLen = sizeof(WCHAR) * cchNBLen;

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
                             DomainNameW, -1, g.DomainNameA, 256, NULL, NULL))
    {
        sync_err("could not convert domainname (W->A)!\n");
        g.DomainNameA[0] = 0;
    }

    if (!WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             g.NetBIOSNameW, -1, g.NetBIOSNameA, 128, NULL, NULL))
    {
        sync_err("could not convert NetBIOSName (W->A)!\n");
        g.NetBIOSNameA[0] = 0;
    }

    /* this is not elegant, but anyway ... */
    i = 0;
    g.Auth_TargetInfoData[i++] = MsvAvNbDomainName;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = cbNBLen;
    g.Auth_TargetInfoData[i++] = 0;
    memcpy(g.Auth_TargetInfoData + i, g.NetBIOSNameW, cbNBLen);
    i += cbNBLen;

    g.Auth_TargetInfoData[i++] = MsvAvNbComputerName;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = cbNBLen;
    g.Auth_TargetInfoData[i++] = 0;
    memcpy(g.Auth_TargetInfoData + i, g.NetBIOSNameW, cbNBLen);
    i += cbNBLen;

    g.Auth_TargetInfoData[i++] = MsvAvDnsDomainName;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = cbHNLen;
    g.Auth_TargetInfoData[i++] = 0;
    memcpy(g.Auth_TargetInfoData + i, g.DnsHostNameW, cbHNLen);
    i += cbHNLen;

    g.Auth_TargetInfoData[i++] = MsvAvDnsComputerName;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = cbHNLen;
    g.Auth_TargetInfoData[i++] = 0;
    memcpy(g.Auth_TargetInfoData + i, g.DnsHostNameW, cbHNLen);
    i += cbHNLen;

    // ???
    //07 00 08 00 ab b3 a8 2b
    //fa 49 d5 01 00 00 00 00
    g.Auth_TargetInfoData[i++] = MsvAvTimestamp;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = 8;
    g.Auth_TargetInfoData[i++] = 0;

    g.Auth_TargetInfoData[i++] = 0xab;
    g.Auth_TargetInfoData[i++] = 0xb3;
    g.Auth_TargetInfoData[i++] = 0xa8;
    g.Auth_TargetInfoData[i++] = 0x2b;

    g.Auth_TargetInfoData[i++] = 0xfa;
    g.Auth_TargetInfoData[i++] = 0x49;
    g.Auth_TargetInfoData[i++] = 0xd5;
    g.Auth_TargetInfoData[i++] = 0x01;

    /* end of list */
    g.Auth_TargetInfoData[i++] = MsvAvEOL;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = 0;
    g.Auth_TargetInfoData[i++] = 0;

    g.Auth_TargetInfoDataLen = i;
}

void NtlmCheckFini()
{
    DeleteCriticalSection(&g.cs);
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

void NtlmCheckBlobB(
    char* testName,
    char* blobName,
    byte* expected,
    int len,
    void* msg,
    PNTLM_BLOB pblob)
{
    PBYTE pData;
    BOOL isEqual = FALSE;

    pData = (PBYTE)msg + pblob->Offset;

    sync_ok(pblob->Length == len,
            "%s: blob \"%s\" len is %d, expected %d\n",
            testName, blobName,
            pblob->Length, len);
    if (pblob->Length == len)
        isEqual = memcmp(pData, expected, len);

    if (!isEqual)
    {
        sync_ok(FALSE, "%s: blob \"%s\" is\n",
                testName, blobName);
        PrintHexDumpMax(pblob->Length, pData, 256);
        sync_msg("expected\n");
        PrintHexDumpMax(len, expected, 256);
    }
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
        g.DomainNameA,
        pNego, &pNego->OemDomainName);
    NtlmCheckBlobA(testName, "OemWorkstationName",
        g.NetBIOSNameA,
        pNego, &pNego->OemWorkstationName);
    NtlmCheckWinVer(testName, &pNego->Version, &g.osVerInfo);
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
        g.NetBIOSNameW,
        pChallenge, &pChallenge->TargetName);
    sync_ok(pChallenge->NegotiateFlags == 0xe28ac235,
            "%s: pChallenge->NegotiateFlags is %x, expected %x!\n",
            testName, pChallenge->NegotiateFlags, 0xe28ac235);
    //TODO ServerChallenge[MSV1_0_CHALLENGE_LENGTH];
    //TODO Reserved[8];
    NtlmCheckBlobB(testName, "TargetInfo",
        g.Auth_TargetInfoData, g.Auth_TargetInfoDataLen,
        pChallenge, &pChallenge->TargetInfo);
    //FIXME: This is win 7!
    NtlmCheckWinVer(testName, &pChallenge->Version, &g.osVerInfo);
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
    NtlmCheckWinVer(testName, &pAuth->Version, &g.osVerInfo);
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

    EnterCriticalSection(&g.cs);

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

    LeaveCriticalSection(&g.cs);

    sync_trace("*** *** ***<<\n");
}
