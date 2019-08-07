/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#include "client_server.h"

struct CapName
{
    DWORD bits;
    const char *name;
    const char *comment;
} capNames[] = {
    { SECPKG_FLAG_INTEGRITY,            "SECPKG_FLAG_INTEGRITY",            "Supports integrity on messages"          },
    { SECPKG_FLAG_PRIVACY,              "SECPKG_FLAG_PRIVACY",              "Supports privacy (confidentiality)"      },
    { SECPKG_FLAG_TOKEN_ONLY,           "SECPKG_FLAG_TOKEN_ONLY",           "Only security token needed"              },
    { SECPKG_FLAG_DATAGRAM,             "SECPKG_FLAG_DATAGRAM",             "Datagram RPC support"                    },
    { SECPKG_FLAG_CONNECTION,           "SECPKG_FLAG_CONNECTION",           "Connection oriented RPC support"         },
    { SECPKG_FLAG_MULTI_REQUIRED,       "SECPKG_FLAG_MULTI_REQUIRED",       "Full 3-leg required for re-auth."        },
    { SECPKG_FLAG_CLIENT_ONLY,          "SECPKG_FLAG_CLIENT_ONLY",          "Server side functionality not available" },
    { SECPKG_FLAG_EXTENDED_ERROR,       "SECPKG_FLAG_EXTENDED_ERROR",       "Supports extended error msgs"            },
    { SECPKG_FLAG_IMPERSONATION,        "SECPKG_FLAG_IMPERSONATION",        "Supports impersonation"                  },
    { SECPKG_FLAG_ACCEPT_WIN32_NAME,    "SECPKG_FLAG_ACCEPT_WIN32_NAME",    "Accepts Win32 names"                     },
    { SECPKG_FLAG_STREAM,               "SECPKG_FLAG_STREAM",               "Supports stream semantics"               },
    { SECPKG_FLAG_NEGOTIABLE,           "SECPKG_FLAG_NEGOTIABLE",           "Can be used by the negotiate package"    },
    { SECPKG_FLAG_GSS_COMPATIBLE,       "SECPKG_FLAG_GSS_COMPATIBLE",       "GSS Compatibility Available"             },
    { SECPKG_FLAG_LOGON,                "SECPKG_FLAG_LOGON",                "Supports common LsaLogonUser"            },
    { SECPKG_FLAG_ASCII_BUFFERS,        "SECPKG_FLAG_ASCII_BUFFERS",        "Token Buffers are in ASCII"              },
    { 0xffffffffL, "(fence)", "(fence)" }
};

void initSecLib(
    HINSTANCE* phSec,
    PSecurityFunctionTable* pSecFuncTable)
{
    SECURITY_STATUS rc;
    DWORD numPacks = 0, i, j;
    SecPkgInfoW *pPacks = NULL;

    PSecurityFunctionTable (*pInitSecurityInterfaceW)(void);

    assert(phSec);
    assert(pSecFuncTable);

    *phSec = LoadLibraryW(L"security.dll");
    pInitSecurityInterfaceW = (PSecurityFunctionTable(*)(void))GetProcAddress(*phSec, "InitSecurityInterfaceW");
    if (pInitSecurityInterfaceW == NULL)
    {
        sync_err("security.dll loading failed...");
        exit(1);
    }

    *pSecFuncTable = pInitSecurityInterfaceW();
    if (*pSecFuncTable == NULL)
    {
        sync_err("no function table?!?");
        exit(1);
    }

    rc = ((*pSecFuncTable)->EnumerateSecurityPackages)(&numPacks, &pPacks);
    if (rc != 0)
    {
        sync_err("ESP() returned %ld\n", rc);
        exit(1);
    }

    for (i = 0; i < numPacks; ++ i)
    {
        sync_trace("\nPackage: %S\n", pPacks[i].Name);
        sync_trace("  Version:      %hu\n", pPacks[i].wVersion);
        sync_trace("  RPCID:        %hu%s\n", pPacks[i].wRPCID,
              pPacks[i].wRPCID == SECPKG_ID_NONE? " [none]": "");
        sync_trace("  Comment:      \"%S\"\n", pPacks[i].Comment);
        sync_trace("  Capabilities: %08lxh\n", pPacks[i].fCapabilities);
        for (j = 0; capNames[j].bits != 0xffffffffL; ++j)
        {
            if ((capNames[j].bits & pPacks[i].fCapabilities) == capNames[j].bits)
                sync_msg("    %s (%s)\n", capNames[j].name, capNames[j].comment);
        }
    }

    if (pPacks != NULL)
        ((*pSecFuncTable)->FreeContextBuffer)(pPacks);
}


/* wserr() displays winsock errors and aborts. No grace there. */
void wserr(int rc, LPCWSTR const funcname)
{
    DWORD errnum = WSAGetLastError();
    LPWSTR errbuffer;

    if (rc == 0)
        return;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   errnum,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&errbuffer,
                   0,
                   NULL);

    sync_err("\nWinsock error %d [gle %ld] returned by %s().\n", rc, errnum, (char*)funcname);
    sync_err("\nError string: %s.\n", (char*)errbuffer);

    LocalFree(errbuffer);
    WSACleanup();
    // exit(rc);
}

void PrintHexDumpMax(
    IN DWORD length,
    IN PBYTE buffer,
    IN int printmax)
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[512];
    int cbLine;

    if (length > printmax)
        length = printmax;

    for (index = 0; length;
         length -= count, buffer += count, index += count)
    {
        count = (length > 32) ? 32:length;

        snprintf(rgbLine, 512, "%4.4x  ",index);
        cbLine = 6;

        for (i = 0; i < count; i++)
        {
            rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[cbLine++] = ':';
            }
            else
            {
                rgbLine[cbLine++] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
        }

        rgbLine[cbLine++] = ' ';

        for (i = 0; i < count; i++)
        {
            if (buffer[i] < 32 || buffer[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = buffer[i];
            }
        }

        rgbLine[cbLine++] = 0;
        sync_trace("%s\n", rgbLine);
    }
}

void PrintHexDump(
    IN DWORD length,
    IN PBYTE buffer)
{
    PrintHexDumpMax(length, buffer, 32);
}

//TODO void PrintNegtiaonMessage(PCHALLENGE_MESSAGE pmsg)
void PrintNtlmBlob(const char* name, void* pmsg, ULONG msgsize, PNTLM_BLOB pblob)
{
    PBYTE pData;

    sync_trace("%s (PNTLM_BLOB %p)\n", name, pblob);
    if (pblob == NULL)
        return;

    sync_trace("->Length    %d\n", pblob->Length);
    sync_trace("->MaxLength %d\n", pblob->MaxLength);
    sync_trace("->Offset    %d\n", pblob->Offset);

    if ((pblob->Offset + pblob->Length) > msgsize)
    {
        sync_err("blob points beyond buffer bounds.\n");
        return;
    }

    pData = ((PBYTE)pmsg + pblob->Offset);
    PrintHexDumpMax(pblob->Length, pData, 265);
}

void PrintNtlmAvl(const char* name, void* pmsg, ULONG msgsize, PNTLM_BLOB pblob)
{
    PBYTE pData;
    PMSV1_0_AV_PAIR pAvp;
    PBYTE pEnd;
    WCHAR* avpStrName;
    int avlNr;

    sync_trace("%s (AV list %p)\n", name, pblob);
    if (pblob == NULL)
        return;

    sync_trace("->Length    %d\n", pblob->Length);
    sync_trace("->MaxLength %d\n", pblob->MaxLength);
    sync_trace("->Offset    %d\n", pblob->Offset);

    if ((pblob->Offset + pblob->Length) > msgsize)
    {
        sync_err("blob points beyond buffer bounds.\n");
        return;
    }

    pData = (PBYTE)pmsg + pblob->Offset;
    pEnd  = pData + pblob->Length;

    avlNr = 0;
    while (pData < pEnd)
    {
        pAvp = (PMSV1_0_AV_PAIR)pData;
        pData += sizeof(MSV1_0_AV_PAIR);

        if (pData + pAvp->AvLen > pEnd)
        {
            sync_err("invlaid PMSV1_0_AV_PAIR ... len excedes message size!");
            break;
        }

        avpStrName = NULL;
        switch (pAvp->AvId)
        {
            case MsvAvEOL:
            {
                sync_trace("%.2i MsvAvEOL\n", avlNr);
                break;
            }
            case MsvAvNbComputerName:
            {
                avpStrName = L"MsvAvNbComputerName";
                break;
            }
            case MsvAvNbDomainName:
            {
                avpStrName = L"MsvAvNbDomainName";
                break;
            }
            case MsvAvDnsComputerName:
            {
                avpStrName = L"MsvAvDnsComputerName";
                break;
            }
            case MsvAvDnsDomainName:
            {
                avpStrName = L"MsvAvDnsDomainName";
                break;
            }
            case MsvAvDnsTreeName:
            {
                avpStrName = L"MsvAvDnsTreeName";
                break;
            }
            case MsvAvFlags :;
            {
                /* is it a ULONG? */
                assert(pAvp->AvLen == 4);
                sync_trace("%.2i MsvAvFlags 0x%x\n", avlNr, (ULONG)pData);
                break;
            }
            case MsvAvTimestamp :
            {
                PFILETIME pFt = (PFILETIME)pData;
                SYSTEMTIME st;
                if (!FileTimeToSystemTime(pFt, &st))
                    RtlZeroMemory(&st, sizeof(st));
                sync_trace("%.2i MsvAvTimestamp %.2d.%.2d.%.4d %.2d:%.2d:%.2d,%.4d\n",
                           avlNr, st.wDay, st.wMonth, st.wYear,
                           st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                break;
            }
            /*TODO
            case MsvAvSingleHost :;
            case MsvAvTargetName:;
            case MsvChannelBindings :;*/
            default:
            {
                sync_err("AvId %d not implemented!\n", pAvp->AvId);
                break;
            }
        }
        /* is ist a string? */
        if (avpStrName != NULL)
        {
            sync_trace("%.2i %S %.*S\n", avlNr, avpStrName,
                        pAvp->AvLen / sizeof(WCHAR), (WCHAR*)pData);
        }

        pData += pAvp->AvLen;
        avlNr++;
    }
}

void PrintNtlmWindowsVersion(const char* name, PNTLM_WINDOWS_VERSION pver)
{
    sync_trace("%s (PNTLM_WINDOWS_VERSION %p)\n", name, pver);
    if (pver == NULL)
        return;

    sync_trace("->Major.Minor.Build %d.%d.%d\n",
              pver->ProductMajor, pver->ProductMinor,
              pver->ProductBuild);
    PrintHexDump(3, (PBYTE)&pver->Reserved);
    sync_trace("->NtlmRevisionCurrent %d\n", pver->NtlmRevisionCurrent);
}

void PrintNegotiateMessage(PNEGOTIATE_MESSAGE pmsg, ULONG msgsize)
{
    sync_trace("NEGOTIATE_MESSAGE 0x%p\n", pmsg);
    if (pmsg == NULL)
        return;
    sync_trace("Signature   %.*s\n", 8, pmsg->Signature);
    sync_trace("MsgType     0x%x\n", 8, pmsg->MsgType);
    sync_trace("NegotiateFlags 0x%x\n", 8, pmsg->NegotiateFlags);
    PrintNtlmBlob("OemDomainName", pmsg, msgsize, &pmsg->OemDomainName);
    PrintNtlmBlob("OemWorkstationName", pmsg, msgsize, &pmsg->OemWorkstationName);
    PrintNtlmWindowsVersion("Version", &pmsg->Version);
}

void PrintChallengeMessage(PCHALLENGE_MESSAGE pmsg, ULONG msgsize)
{
    sync_trace("CHALLENGE_MESSAGE 0x%p\n", pmsg);
    if (pmsg == NULL)
        return;
    sync_trace("Signature   %.*s\n", 8, pmsg->Signature);
    sync_trace("MsgType     0x%x\n", 8, pmsg->MsgType);
    PrintNtlmBlob("TargetName", pmsg, msgsize, &pmsg->TargetName);
    sync_trace("NegotiateFlags 0x%x\n", 8, pmsg->NegotiateFlags);
    //sys_trace("ServerChallenge %.*s\n", 8, pmsg->ServerChallenge);
    PrintHexDump(MSV1_0_CHALLENGE_LENGTH, (PBYTE)&pmsg->ServerChallenge);
    PrintHexDump(8, (PBYTE)&pmsg->Reserved);
    PrintNtlmAvl("TargetInfo", pmsg, msgsize, &pmsg->TargetInfo);
    PrintNtlmWindowsVersion("Version", &pmsg->Version);
}

void PrintAuthenticateMessage(PAUTHENTICATE_MESSAGE pmsg, ULONG msgsize)
{
    sync_trace("AUTHENTICATE_MESSAGE 0x%p\n", pmsg);
    if (pmsg == NULL)
        return;
    sync_trace("Signature   %.*s\n", 8, pmsg->Signature);
    sync_trace("MsgType     0x%x\n", 8, pmsg->MsgType);
    PrintNtlmBlob("LmChallengeResponse", pmsg, msgsize, &pmsg->LmChallengeResponse);
    PrintNtlmBlob("NtChallengeResponse", pmsg, msgsize, &pmsg->NtChallengeResponse);
    PrintNtlmBlob("DomainName", pmsg, msgsize, &pmsg->DomainName);
    PrintNtlmBlob("UserName", pmsg, msgsize, &pmsg->UserName);
    PrintNtlmBlob("WorkstationName", pmsg, msgsize, &pmsg->WorkstationName);
    PrintNtlmBlob("EncryptedRandomSessionKey", pmsg, msgsize, &pmsg->EncryptedRandomSessionKey);
    sync_trace("NegotiateFlags 0x%x\n", 8, pmsg->NegotiateFlags);
    PrintNtlmWindowsVersion("Version", &pmsg->Version);
    PrintHexDump(sizeof(pmsg->MIC), (PBYTE)&pmsg->MIC);
}

void PrintSecBuffer(PSecBuffer buf)
{
    PNTLM_MESSAGE_HEAD pHead;

    if (buf->cbBuffer == 0)
        return;

    //sync_trace("buf->BufferType 0x%x\n", buf->BufferType);
    if (buf->BufferType == SECBUFFER_TOKEN)
    {
        pHead = (PNTLM_MESSAGE_HEAD)buf->pvBuffer;
        if (memcmp(pHead->Signature, NTLMSSP_SIGNATURE, 8) != 0)
        {
            sync_err("**** wrong signature DUMPING ...\n");
            PrintHexDump(buf->cbBuffer, buf->pvBuffer);
            return;
        }
        if (pHead->MsgType == NtlmNegotiate)
        {
            PrintNegotiateMessage((PNEGOTIATE_MESSAGE)buf->pvBuffer, buf->cbBuffer);
        }
        else if (pHead->MsgType == NtlmChallenge)
        {
            PrintChallengeMessage((PCHALLENGE_MESSAGE)buf->pvBuffer, buf->cbBuffer);
        }
        else if (pHead->MsgType == NtlmAuthenticate)
        {
            PrintAuthenticateMessage((PAUTHENTICATE_MESSAGE)buf->pvBuffer, buf->cbBuffer);
        }
        else
        {
            sync_trace("unknown message type %x\n", pHead->MsgType);
            PrintHexDump(buf->cbBuffer, buf->pvBuffer);
        }
    }
    else
    {
        sync_trace("unknown buffer type\n");
        PrintHexDump(buf->cbBuffer, buf->pvBuffer);
    }
}

void PrintISCReqAttr(
    IN ULONG ReqAttr)
{
    sync_trace("ISCReqAttr \"0x%08lx\"{\n", ReqAttr);
    if (ReqAttr & ISC_REQ_DELEGATE)
       sync_trace("\tISC_REQ_DELEGATE\n");
    if (ReqAttr & ISC_REQ_MUTUAL_AUTH)
       sync_trace("\tISC_REQ_MUTUAL_AUTH\n");
    if (ReqAttr & ISC_REQ_REPLAY_DETECT)
       sync_trace("\tISC_REQ_REPLAY_DETECT\n");
    if (ReqAttr & ISC_REQ_SEQUENCE_DETECT)
       sync_trace("\tISC_REQ_SEQUENCE_DETECT\n");
    if (ReqAttr & ISC_REQ_CONFIDENTIALITY)
       sync_trace("\tISC_REQ_CONFIDENTIALITY\n");
    if (ReqAttr & ISC_REQ_USE_SESSION_KEY)
       sync_trace("\tISC_REQ_USE_SESSION_KEY\n");
    if (ReqAttr & ISC_REQ_PROMPT_FOR_CREDS)
       sync_trace("\tISC_REQ_PROMPT_FOR_CREDS\n");
    if (ReqAttr & ISC_REQ_USE_SUPPLIED_CREDS)
       sync_trace("\tISC_REQ_USE_SUPPLIED_CREDS\n");
    if (ReqAttr & ISC_REQ_ALLOCATE_MEMORY)
       sync_trace("\tISC_REQ_ALLOCATE_MEMORY\n");
    if (ReqAttr & ISC_REQ_USE_DCE_STYLE)
       sync_trace("\tISC_REQ_USE_DCE_STYLE\n");
    if (ReqAttr & ISC_REQ_DATAGRAM)
       sync_trace("\tISC_REQ_DATAGRAM\n");
    if (ReqAttr & ISC_REQ_CONNECTION)
       sync_trace("\tISC_REQ_CONNECTION\n");
    if (ReqAttr & ISC_REQ_CALL_LEVEL)
       sync_trace("\tISC_REQ_CALL_LEVEL\n");
    if (ReqAttr & ISC_REQ_FRAGMENT_SUPPLIED)
       sync_trace("\tISC_REQ_FRAGMENT_SUPPLIED\n");
    if (ReqAttr & ISC_REQ_EXTENDED_ERROR)
       sync_trace("\tISC_REQ_EXTENDED_ERROR\n");
    if (ReqAttr & ISC_REQ_STREAM)
       sync_trace("\tISC_REQ_STREAM\n");
    if (ReqAttr & ISC_REQ_INTEGRITY)
       sync_trace("\tISC_REQ_INTEGRITY\n");
    if (ReqAttr & ISC_REQ_IDENTIFY)
       sync_trace("\tISC_REQ_IDENTIFY\n");
    if (ReqAttr & ISC_REQ_NULL_SESSION)
       sync_trace("\tISC_REQ_NULL_SESSION\n");
    if (ReqAttr & ISC_REQ_MANUAL_CRED_VALIDATION)
       sync_trace("\tISC_REQ_MANUAL_CRED_VALIDATION\n");
    if (ReqAttr & ISC_REQ_RESERVED1)
       sync_trace("\tISC_REQ_RESERVED1\n");
    if (ReqAttr & ISC_REQ_FRAGMENT_TO_FIT)
       sync_trace("\tISC_REQ_FRAGMENT_TO_FIT\n");
    sync_trace("}\n");
}

void PrintISCRetAttr(
    IN ULONG RetAttr)
{
    sync_trace("ISCRetAttr \"0x%08lx\"{\n", RetAttr);
    if (RetAttr & ISC_RET_DELEGATE)
       sync_trace("\tISC_RET_DELEGATE\n");
    if (RetAttr & ISC_RET_MUTUAL_AUTH)
       sync_trace("\tISC_RET_MUTUAL_AUTH\n");
    if (RetAttr & ISC_RET_REPLAY_DETECT)
       sync_trace("\tISC_RET_REPLAY_DETECT\n");
    if (RetAttr & ISC_RET_SEQUENCE_DETECT)
       sync_trace("\tISC_RET_SEQUENCE_DETECT\n");
    if (RetAttr & ISC_RET_CONFIDENTIALITY)
       sync_trace("\tISC_RET_CONFIDENTIALITY\n");
    if (RetAttr & ISC_RET_USE_SESSION_KEY)
       sync_trace("\tISC_RET_USE_SESSION_KEY\n");
    if (RetAttr & ISC_RET_USED_COLLECTED_CREDS)
       sync_trace("\tISC_RET_USED_COLLECTED_CREDS\n");
    if (RetAttr & ISC_RET_USED_SUPPLIED_CREDS)
       sync_trace("\tISC_RET_USED_SUPPLIED_CREDS\n");
    if (RetAttr & ISC_RET_ALLOCATED_MEMORY)
       sync_trace("\tISC_RET_ALLOCATED_MEMORY\n");
    if (RetAttr & ISC_RET_USED_DCE_STYLE)
       sync_trace("\tISC_RET_USED_DCE_STYLE\n");
    if (RetAttr & ISC_RET_DATAGRAM)
       sync_trace("\tISC_RET_DATAGRAM\n");
    if (RetAttr & ISC_RET_CONNECTION)
       sync_trace("\tISC_RET_CONNECTION\n");
    if (RetAttr & ISC_RET_INTERMEDIATE_RETURN)
       sync_trace("\tISC_RET_INTERMEDIATE_RETURN\n");
    if (RetAttr & ISC_RET_CALL_LEVEL)
       sync_trace("\tISC_RET_CALL_LEVEL\n");
    if (RetAttr & ISC_RET_EXTENDED_ERROR)
       sync_trace("\tISC_RET_EXTENDED_ERROR\n");
    if (RetAttr & ISC_RET_STREAM)
       sync_trace("\tISC_RET_STREAM\n");
    if (RetAttr & ISC_RET_INTEGRITY)
       sync_trace("\tISC_RET_INTEGRITY\n");
    if (RetAttr & ISC_RET_IDENTIFY)
       sync_trace("\tISC_RET_IDENTIFY\n");
    if (RetAttr & ISC_RET_NULL_SESSION)
       sync_trace("\tISC_RET_NULL_SESSION\n");
    if (RetAttr & ISC_RET_MANUAL_CRED_VALIDATION)
       sync_trace("\tISC_RET_MANUAL_CRED_VALIDATION\n");
    if (RetAttr & ISC_RET_RESERVED1)
       sync_trace("\tISC_RET_RESERVED1\n");
    if (RetAttr & ISC_RET_FRAGMENT_ONLY)
       sync_trace("\tISC_RET_FRAGMENT_ONLY\n");
    sync_trace("}\n");
}

void PrintASCRetAttr(IN ULONG RetAttr)
{
    sync_trace("ASCRetAttr \"0x%08lx\"{\n", RetAttr);
    if (RetAttr & ASC_RET_DELEGATE)
       sync_trace("\tASC_RET_DELEGATE\n");
    if (RetAttr & ASC_RET_MUTUAL_AUTH)
       sync_trace("\tASC_RET_MUTUAL_AUTH\n");
    if (RetAttr & ASC_RET_REPLAY_DETECT)
       sync_trace("\tASC_RET_REPLAY_DETECT\n");
    if (RetAttr & ASC_RET_SEQUENCE_DETECT)
       sync_trace("\tASC_RET_SEQUENCE_DETECT\n");
    if (RetAttr & ASC_RET_CONFIDENTIALITY)
       sync_trace("\tASC_RET_CONFIDENTIALITY\n");
    if (RetAttr & ASC_RET_USE_SESSION_KEY)
       sync_trace("\tASC_RET_USE_SESSION_KEY\n");
    if (RetAttr & ASC_RET_ALLOCATED_MEMORY)
       sync_trace("\tASC_RET_ALLOCATED_MEMORY\n");
    if (RetAttr & ASC_RET_USED_DCE_STYLE)
       sync_trace("\tASC_RET_USED_DCE_STYLE\n");
    if (RetAttr & ASC_RET_DATAGRAM)
       sync_trace("\tASC_RET_DATAGRAM\n");
    if (RetAttr & ASC_RET_CONNECTION)
       sync_trace("\tASC_RET_CONNECTION\n");
    if (RetAttr & ASC_RET_CALL_LEVEL)
       sync_trace("\tASC_RET_CALL_LEVEL\n");
    if (RetAttr & ASC_RET_THIRD_LEG_FAILED)
       sync_trace("\tASC_RET_THIRD_LEG_FAILED\n");
    if (RetAttr & ASC_RET_EXTENDED_ERROR)
       sync_trace("\tASC_RET_EXTENDED_ERROR\n");
    if (RetAttr & ASC_RET_STREAM)
       sync_trace("\tASC_RET_STREAM\n");
    if (RetAttr & ASC_RET_INTEGRITY)
       sync_trace("\tASC_RET_INTEGRITY\n");
    if (RetAttr & ASC_RET_LICENSING)
       sync_trace("\tASC_RET_LICENSING\n");
    if (RetAttr & ASC_RET_IDENTIFY)
       sync_trace("\tASC_RET_IDENTIFY\n");
    if (RetAttr & ASC_RET_NULL_SESSION)
       sync_trace("\tASC_RET_NULL_SESSION\n");
    if (RetAttr & ASC_RET_ALLOW_NON_USER_LOGONS)
       sync_trace("\tASC_RET_ALLOW_NON_USER_LOGONS\n");
    if (RetAttr & ASC_RET_ALLOW_CONTEXT_REPLAY)
       sync_trace("\tASC_RET_ALLOW_CONTEXT_REPLAY\n");
    if (RetAttr & ASC_RET_FRAGMENT_ONLY)
       sync_trace("\tASC_RET_FRAGMENT_ONLY\n");
    if (RetAttr & ASC_RET_NO_TOKEN)
       sync_trace("\tASC_RET_NO_TOKEN\n");
    sync_trace("}\n");
}

void PrintASCReqAttr(IN ULONG ReqAttr)
{
    sync_trace("ASCReqAttr \"0x%08lx\"{\n", ReqAttr);
    if (ReqAttr & ASC_REQ_DELEGATE)
       sync_trace("\tASC_REQ_DELEGATE\n");
    if (ReqAttr & ASC_REQ_MUTUAL_AUTH)
       sync_trace("\tASC_REQ_MUTUAL_AUTH\n");
    if (ReqAttr & ASC_REQ_REPLAY_DETECT)
       sync_trace("\tASC_REQ_REPLAY_DETECT\n");
    if (ReqAttr & ASC_REQ_SEQUENCE_DETECT)
       sync_trace("\tASC_REQ_SEQUENCE_DETECT\n");
    if (ReqAttr & ASC_REQ_CONFIDENTIALITY)
       sync_trace("\tASC_REQ_CONFIDENTIALITY\n");
    if (ReqAttr & ASC_REQ_USE_SESSION_KEY)
       sync_trace("\tASC_REQ_USE_SESSION_KEY\n");
    if (ReqAttr & ASC_REQ_ALLOCATE_MEMORY)
       sync_trace("\tASC_REQ_ALLOCATE_MEMORY\n");
    if (ReqAttr & ASC_REQ_USE_DCE_STYLE)
       sync_trace("\tASC_REQ_USE_DCE_STYLE\n");
    if (ReqAttr & ASC_REQ_DATAGRAM)
       sync_trace("\tASC_REQ_DATAGRAM\n");
    if (ReqAttr & ASC_REQ_CONNECTION)
       sync_trace("\tASC_REQ_CONNECTION\n");
    if (ReqAttr & ASC_REQ_CALL_LEVEL)
       sync_trace("\tASC_REQ_CALL_LEVEL\n");
    if (ReqAttr & ASC_REQ_FRAGMENT_SUPPLIED)
       sync_trace("\tASC_REQ_FRAGMENT_SUPPLIED\n");
    if (ReqAttr & ASC_REQ_EXTENDED_ERROR)
       sync_trace("\tASC_REQ_EXTENDED_ERROR\n");
    if (ReqAttr & ASC_REQ_STREAM)
       sync_trace("\tASC_REQ_STREAM\n");
    if (ReqAttr & ASC_REQ_INTEGRITY)
       sync_trace("\tASC_REQ_INTEGRITY\n");
    if (ReqAttr & ASC_REQ_LICENSING)
       sync_trace("\tASC_REQ_LICENSING\n");
    if (ReqAttr & ASC_REQ_IDENTIFY)
       sync_trace("\tASC_REQ_IDENTIFY\n");
    if (ReqAttr & ASC_REQ_ALLOW_NULL_SESSION)
       sync_trace("\tASC_REQ_ALLOW_NULL_SESSION\n");
    if (ReqAttr & ASC_REQ_ALLOW_NON_USER_LOGONS)
       sync_trace("\tASC_REQ_ALLOW_NON_USER_LOGONS\n");
    if (ReqAttr & ASC_REQ_ALLOW_CONTEXT_REPLAY)
       sync_trace("\tASC_REQ_ALLOW_CONTEXT_REPLAY\n");
    if (ReqAttr & ASC_REQ_FRAGMENT_TO_FIT)
       sync_trace("\tASC_REQ_FRAGMENT_TO_FIT\n");
    if (ReqAttr & ASC_REQ_FRAGMENT_NO_TOKEN)
       sync_trace("\tASC_REQ_FRAGMENT_NO_TOKEN\n");
    sync_trace("}\n");
}


BOOL SendMsg(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf)
{
    if (cbBuf == 0)
        return TRUE;

    /* Send the size of the message */
    if (!SendBytes(s,
                   (PBYTE)&cbBuf,
                   sizeof(cbBuf)))
    {
        return FALSE;
    }
    /* Send the body of the message */
    if (!SendBytes(s,
                   pBuf,
                   cbBuf))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL SendBytes(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf)
{
    PBYTE pTemp = pBuf;
    int cbSent, cbRemaining = cbBuf;

    sync_trace("sending %lu bytes from buffer %p to socket %x\n", cbBuf, pBuf, s);

    if (!pBuf)
    {
        sync_err("received null buffer!\n");
        return FALSE;
    }

    if (cbBuf == 0)
        return TRUE;

    while (cbRemaining)
    {
        cbSent = send(s,
                      (const char *)pTemp,
                      cbRemaining,
                      0);

        if (SOCKET_ERROR == cbSent)
        {
            sync_err("send failed: %lu\n", GetLastError());
            printerr(GetLastError());
            return FALSE;
        }

        pTemp += cbSent;
        cbRemaining -= cbSent;
    }

    return TRUE;
}

BOOL ReceiveMsg(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf,
    DWORD *pcbRead)
{
    DWORD cbRead;
    DWORD cbData;

    /* Retrieve the number of bytes in the message */
    if (!ReceiveBytes(s,
                      (PBYTE)&cbData,
                      sizeof(cbData),
                      &cbRead))
    {
        return FALSE;
    }

    sync_trace("message size = %lu\n", cbData);

    if (sizeof(cbData) != cbRead)
        return FALSE;

    /* Read the full message */
    if (!ReceiveBytes(s,
                      pBuf,
                      cbData,
                      &cbRead))
    {
        return FALSE;
    }

    if (cbRead != cbData)
    {
        sync_err("cbRead != cbData, was %lu %lu\n",cbRead, cbData);
        return FALSE;
    }

    *pcbRead = cbRead;
    return TRUE;
}

BOOL ReceiveBytes(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf,
    DWORD *pcbRead)
{
    PBYTE pTemp = pBuf;
    int cbRead, cbRemaining = cbBuf;

    sync_trace("reading %lu bytes from socket %x to buffer %p\n", cbBuf, s, pBuf);

    if (!pBuf)
    {
        sync_err("received null buffer!\n");
        return FALSE;
    }

    while (cbRemaining)
    {
        cbRead = recv(s, (char*)pTemp, cbRemaining, 0);
        if (cbRead < 0 || cbRead == SOCKET_ERROR)
        {
            sync_err("recv failed: %d\n", WSAGetLastError());
            printerr(WSAGetLastError());
            sync_err("abort");
            return FALSE;
        }
        sync_trace("received %d bytes, %d remain in buffer\n", cbRead, cbRemaining);

        if (cbRead == 0)
            break;

        cbRemaining -= cbRead;
        pTemp += cbRead;
    }
    *pcbRead = cbBuf - cbRemaining;

    return TRUE;
}

DWORD inet_addr_w(const WCHAR *pszAddr)
{
    char szBuffer[256];

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        pszAddr, -1, szBuffer, 256, NULL, NULL);

    return inet_addr(szBuffer);
}
