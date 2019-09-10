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

/* this file contains algorithms defined in the MS-NLMP document */

#include "ntlmssp.h"
#include "protocol.h"
#include "ciphers.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* MS-NLSP 3.3.1 */
BOOL
NTOWFv1(
    IN LPCWSTR password,
    IN PUCHAR result)
{
    ULONG chLen;

    chLen = (password != NULL) ? wcslen(password) : 0;

    MD4((PUCHAR)password, chLen * sizeof(WCHAR), result);

    return TRUE;
}

/* MS-NLSP 3.3.2 */
BOOL
NTOWFv2(
    IN LPCWSTR password,
    IN LPCWSTR user,
    IN LPCWSTR domain,
    OUT UCHAR result[16])
{
    EXT_STRING UserUserDom;
    UCHAR md4pwd[16];

    /* concat upper(user) + domain */
    ExtWStrInit(&UserUserDom, (WCHAR*)user);
    ExtWStrUpper(&UserUserDom);
    ExtWStrCat(&UserUserDom, (WCHAR*)domain);

    MD4((UCHAR*)password, wcslen(password) * sizeof(WCHAR), md4pwd);
    HMAC_MD5(md4pwd, ARRAYSIZE(md4pwd),
             (UCHAR*)UserUserDom.Buffer, UserUserDom.bUsed, result);

    ExtStrFree(&UserUserDom);
    return TRUE;
}

/* MS-NLSP 3.3.1 */
VOID
LMOWFv1(
    IN PCCHAR password,
    OUT UCHAR result[16])
{
#if 0
    SystemFunction006(password, result);
#else
    UCHAR* magic = (UCHAR*)"KGS!@#$%";
    UCHAR uppercase_password[14];
    ULONG i;

    ULONG len = strlen(password);
    if (len > 14) {
        len = 14;
    }

    // Uppercase password
    for (i = 0; i < len; i++) {
        uppercase_password[i] = toupper(password[i]);
    }

    // Zero the rest
    for (; i < 14; i++) {
        uppercase_password[i] = 0;
    }

    DES(uppercase_password, magic, result);
    DES(uppercase_password + 7, magic, result + 8);
#endif
}

BOOLEAN
LMOWFv2(LPCWSTR password,
        LPCWSTR user,
        LPCWSTR domain,
        PUCHAR result)
{
    return NTOWFv2(password, user, domain, result);
}

VOID
NONCE(PUCHAR buffer,
      ULONG num)
{
    NtlmGenerateRandomBits(buffer, num);
}

/* MS-NLSP 3.4.5.1 KXKEY */
VOID
KXKEY(
    IN ULONG NegFlg,
    IN UCHAR SessionBaseKey[MSV1_0_USER_SESSION_KEY_LENGTH],
    IN PEXT_DATA LmChallengeResponse,
    IN PEXT_DATA NtChallengeResponse,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    OUT UCHAR KeyExchangeKey[NTLM_KEYEXCHANGE_KEY_LENGTH])
{
    UCHAR* LMOWF = ResponseKeyLM;
    UCHAR DESv2[8] = "\x00\xBD\xBD\xBD\xBD\xBD\xBD\xBD";

    if (LmChallengeResponse->bUsed < 8)
    {
        ERR("KXKEY: LmChallengeResponseLen < 8 Bytes!\n");
        return;
    }

    /* NTLMv2 Security - only used with NTLMv1*/
    if ((NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY) &&
        /* using NTLMv1? */
        (LmChallengeResponse->bUsed == 24) &&
        (NtChallengeResponse->bUsed == 24))
    {
        UCHAR nonce[16];
        //Define KXKEY(SessionBaseKey, LmChallengeResponse, ServerChallenge) as
        //Set KeyExchangeKey to HMAC_MD5(SessionBaseKey, ConcatenationOf(ServerChallenge,
        //LmChallengeResponse [0..7]))
        //EndDefine
        memcpy(nonce, ServerChallenge, 8);
        memcpy(nonce+8, LmChallengeResponse->Buffer, 8);
        HMAC_MD5(SessionBaseKey, MSV1_0_USER_SESSION_KEY_LENGTH,
                 nonce, 16,
                 KeyExchangeKey);
    }
    else if (NegFlg & NTLMSSP_NEGOTIATE_LM_KEY)
    {
        //Set KeyExchangeKey to
        //  ConcatenationOf(
        //    DES(LMOWF[0..6],LmChallengeResponse[0..7]),
        //    DES(ConcatenationOf(LMOWF[7], 0xBDBDBDBDBDBD),
        //        LmChallengeResponse[0..7]) )
        DES(&LMOWF[0], LmChallengeResponse->Buffer, &KeyExchangeKey[0]);
        DESv2[0] = LMOWF[7];
        DES(DESv2, LmChallengeResponse->Buffer, &KeyExchangeKey[8]);
    }
    else
    {
        if (NegFlg & NTLMSSP_REQUEST_NON_NT_SESSION_KEY)
        {
            //Set KeyExchangeKey to ConcatenationOf(LMOWF[0..7], Z(8)),
            memcpy(KeyExchangeKey, LMOWF, 8);
            memset(&KeyExchangeKey[8], 0, 8);
        }
        else
            //Set KeyExchangeKey to SessionBaseKey
            memcpy(KeyExchangeKey, SessionBaseKey, NTLM_KEYEXCHANGE_KEY_LENGTH);
    }
}

BOOL
SIGNKEY(
    IN PUCHAR RandomSessionKey,
    IN BOOL IsClient,
    IN PUCHAR Result)
{
    PCHAR magic = IsClient
        ? "session key to client-to-server signing key magic constant"
        : "session key to server-to-client signing key magic constant";
    /* To get the correct key - magic constant and null-terminator
       will be used in MD5. */
    ULONG len = strlen(magic) + 1;
    UCHAR *md5_input = NULL;

    md5_input = NtlmAllocate(16 + len);
    if (!md5_input)
        return FALSE;

    memcpy(md5_input, RandomSessionKey, 16);
    memcpy(md5_input + 16, magic, len);
    MD5(md5_input, len + 16, Result);

    NtlmFree(md5_input);

    return TRUE;
}

/* 3.4.5.3 SEALKEY */
BOOL
SEALKEY(
    IN ULONG flags,
    IN const PUCHAR RandomSessionKey,
    IN BOOL client,
    OUT PUCHAR result)
{
    if (flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        PCHAR magic = client
            ? "session key to client-to-server sealing key magic constant"
            : "session key to server-to-client sealing key magic constant";

        ULONG len = strlen(magic) + 1;
        UCHAR* md5_input;
        ULONG key_len;

        md5_input = (UCHAR*)NtlmAllocate(16+len);
        if(!md5_input)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
        if (flags & NTLMSSP_NEGOTIATE_128) {
            TRACE("NTLM SEALKEY(): 128-bit key (Extended session security)\n");
            key_len = 16;
        } else if (flags & NTLMSSP_NEGOTIATE_56) {
            TRACE("NTLM SEALKEY(): 56-bit key (Extended session security)\n");
            key_len = 7;
        } else {
            TRACE("NTLM SEALKEY(): 40-bit key (Extended session security)\n");
            key_len = 5;
        }

        memcpy(md5_input, RandomSessionKey, key_len);
        memcpy(md5_input + key_len, magic, len);

        MD5 (md5_input, key_len + len, result);
    }
    else if (flags & NTLMSSP_NEGOTIATE_LM_KEY)    {
        if (flags & NTLMSSP_NEGOTIATE_56) {
            TRACE("NTLM SEALKEY(): 56-bit key\n");
            memcpy(result, RandomSessionKey, 7);
            result[7] = 0xA0;
        } else {
            TRACE("NTLM SEALKEY(): 40-bit key\n");
            memcpy(result, RandomSessionKey, 5);
            result[5] = 0xE5;
            result[6] = 0x38;
            result[7] = 0xB0;
        }
    }
    else
    {
        TRACE("NTLM SEALKEY(): 128-bit key\n");
        memcpy(result, RandomSessionKey, 16);
    }

    return TRUE;
}

BOOL
MAC(
    IN ULONG NegFlg,
    IN prc4_key Handle,
    IN UCHAR* SigningKey,
    IN ULONG SigningKeyLength,
    IN PULONG pSeqNum,
    IN UCHAR* msg,
    IN ULONG msgLen,
    OUT PBYTE pSign,
    IN ULONG signLen)
{
    if (NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        PNTLMSSP_MESSAGE_SIGNATURE pmsig;
        PBYTE data;
        ULONG dataLen;
        UCHAR dataMD5[16];

        if (signLen < sizeof(NTLMSSP_MESSAGE_SIGNATURE))
            return 0;
        pmsig = (PNTLMSSP_MESSAGE_SIGNATURE)pSign;

        TRACE("NTLM MAC(): Extented Session Security\n");

        //Define MAC(Handle, SigningKey, SeqNum, Message) as
        //Set NTLMSSP_MESSAGE_SIGNATURE.Version to 0x00000001
        //Set NTLMSSP_MESSAGE_SIGNATURE.Checksum to RC4(Handle,
        //HMAC_MD5(SigningKey, ConcatenationOf(SeqNum, Message))[0..7])
        //Set NTLMSSP_MESSAGE_SIGNATURE.SeqNum to SeqNum
        //Set SeqNum to SeqNum + 1
        //EndDefine
        // -----

        //Set NTLMSSP_MESSAGE_SIGNATURE.Checksum to RC4(Handle,
        //HMAC_MD5(SigningKey, ConcatenationOf(SeqNum, Message))[0..7])
        dataLen = sizeof(ULONG) + msgLen;
        data = NtlmAllocate(dataLen);
        memcpy(data + sizeof(ULONG), msg, msgLen);
        *(PULONG)(data) = *pSeqNum;
        HMAC_MD5(SigningKey, SigningKeyLength, data, dataLen, dataMD5);

        printf("md5\n");
        NtlmPrintHexDump(dataMD5, 16);

       if (NegFlg & NTLMSSP_NEGOTIATE_KEY_EXCH)
       {
            TRACE("NTLM MAC(): Key Exchange\n");
            RC4(Handle, dataMD5, (UCHAR*)(&pmsig->u1.extsec.CheckSum), sizeof(pmsig->u1.extsec.CheckSum));
        }
        else
        {
            TRACE("NTLM MAC(): *NO* Key Exchange\n");
            pmsig->u1.extsec.CheckSum = *(PULONGLONG)dataMD5;
        }
        printf("checksum\n");
        NtlmPrintHexDump((PBYTE)(&pmsig->u1.extsec.CheckSum), 8);

        pmsig->Version = 1;
        pmsig->SeqNum = *pSeqNum;

        NtlmFree(data);

        (*pSeqNum)++;
    }
    else
    {
        UINT32* pData = (UINT32*)pSign;
        UINT32* pDataRC4 = pData + 1;

        if (signLen < 16)
            return 0;

        TRACE("NTLM MAC(): *NO* Extented Session Security\n");

        //Define MAC(Handle, SigningKey, SeqNum, Message) as
        //Set NTLMSSP_MESSAGE_SIGNATURE.Version to 0x00000001
        //Set NTLMSSP_MESSAGE_SIGNATURE.Checksum to CRC32(Message)
        //Set NTLMSSP_MESSAGE_SIGNATURE.RandomPad RC4(Handle, RandomPad)
        //Set NTLMSSP_MESSAGE_SIGNATURE.Checksum to RC4(Handle,
        //NTLMSSP_MESSAGE_SIGNATURE.Checksum)
        //Set NTLMSSP_MESSAGE_SIGNATURE.SeqNum to RC4(Handle, 0x00000000)
        pData[0] = 1; /* version */
        pDataRC4[0] = 0;
        pDataRC4[1] = CRC32((char*)msg, msgLen);
        pDataRC4[2] = *pSeqNum;

        printf("MAC\n");
        NtlmPrintHexDump((UCHAR*)pDataRC4, 12);
        RC4(Handle, (UCHAR*)pDataRC4, (UCHAR*)pDataRC4, 12);
        printf("MAC ENC\n");
        NtlmPrintHexDump((UCHAR*)pDataRC4, 12);
        /* override first 4 bytes (any value) */
        pDataRC4[0] = 0x00090178;// 78010900;
        printf("MAC FULL\n");
        NtlmPrintHexDump((UCHAR*)pData, 16);

        /* connection oriented */
        if (!(NegFlg & NTLMSSP_NEGOTIATE_DATAGRAM))
        {
            //Set NTLMSSP_MESSAGE_SIGNATURE.SeqNum to
            //NTLMSSP_MESSAGE_SIGNATURE.SeqNum XOR SeqNum
            //Set SeqNum to SeqNum + 1
            //pMacData->SeqNum = /*?pMacData->SeqNum ^*/ *pSeqNum;
            (*pSeqNum)++;
        }
        else
        {
            //TODO Connection-less (DATAGRAM)
            //Set NTLMSSP_MESSAGE_SIGNATURE.SeqNum to                NTLMSSP_MESSAGE_SIGNATURE.SeqNum XOR
            //(application supplied SeqNum)
        }
        //Set NTLMSSP_MESSAGE_SIGNATURE.RandomPad to 0
        //EndDefine
        (*pSeqNum)++;
    }
    return TRUE;
}

//
//Define SEAL(Handle, SigningKey, SeqNum, Message) as
//Set Sealed message to RC4(Handle, Message)
//Set Signature to MAC(Handle, SigningKey, SeqNum, Message)
//EndDefin
BOOL
SEAL(
    IN ULONG NegFlg,
    IN prc4_key Handle,
    IN UCHAR* SigningKey,
    IN ULONG SigningKeyLength,
    IN PULONG pSeqNum,
    IN OUT UCHAR* msg,
    IN ULONG msgLen,
    OUT UCHAR* pSign,
    OUT PULONG pSignLen)
{
    ULONG signLen = 16;
    if (*pSignLen < signLen)
        return FALSE;

    printf("msg\n");
    NtlmPrintHexDump(msg, msgLen);

    MAC(NegFlg,
        Handle, SigningKey, SigningKeyLength, pSeqNum, msg,
        msgLen, pSign, signLen);

    printf("sign\n");
    NtlmPrintHexDump(pSign, signLen);

    if (NegFlg & NTLMSSP_NEGOTIATE_SEAL)
    {
        RC4(Handle, msg, msg, msgLen);
    }
    printf("result\n");
    NtlmPrintHexDump(msg, msgLen);

    *pSignLen = signLen;

    return TRUE;
}

BOOL
UNSEAL(
    IN ULONG NegFlg,
    IN prc4_key Handle,
    IN UCHAR* SigningKey,
    IN ULONG SigningKeyLength,
    IN PULONG pSeqNum,
    IN OUT UCHAR* msg,
    IN ULONG msgLen,
    OUT UCHAR* pSign,
    OUT PULONG pSignLen)
{
    ULONG signLen = 16;
    if (*pSignLen < signLen)
        return FALSE;

    printf("msg (encoded)\n");
    NtlmPrintHexDump(msg, msgLen);
    if (NegFlg & NTLMSSP_NEGOTIATE_SEAL)
    {
        RC4(Handle, msg, msg, msgLen);
    }

    printf("msg (decodec)\n");
    NtlmPrintHexDump(msg, msgLen);

    MAC(NegFlg,
        Handle, SigningKey, SigningKeyLength, pSeqNum, msg,
        msgLen, pSign, signLen);

    printf("sign\n");
    NtlmPrintHexDump(pSign, signLen);

    *pSignLen = signLen;

    return TRUE;
}

/* MS-NLSP 3.3.1 NTLM v1 Authentication */
//#define VALIDATE_NTLMv1
//#define VALIDATE_NTLM
BOOL
CliComputeResponseNTLMv1(
    IN ULONG NegFlg,
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    IN UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH],
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR NtChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    OUT UCHAR LmChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    OUT PUSER_SESSION_KEY SessionBaseKey)
{
    //--Define ComputeResponse(NegFlg, ResponseKeyNT, ResponseKeyLM,
    //--CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge, Time, ServerName)
    //As
    //If (User is set to "" AND Passwd is set to "")
    if ((user->bUsed == 0) &&
        (passwd->bUsed == 0))
    {
        //-- Special case for anonymous authentication
        //Set NtChallengeResponseLen to 0
        //Set NtChallengeResponseMaxLen to 0
        //Set NtChallengeResponseBufferOffset to 0
        //Set LmChallengeResponse to Z(1)
        //ElseIf
        FIXME("anonymouse not implemented\n");
        return FALSE;
    }
    else
    {
    //If (NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY flag is set in NegFlg)
        if (NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
        {
            UCHAR tmpCat[MSV1_0_CHALLENGE_LENGTH * 2];
            UCHAR tmpMD5[16];
            //Set NtChallengeResponse to
            //  DESL(ResponseKeyNT,
            //    MD5(
            //      ConcatenationOf(
            //        CHALLENGE_MESSAGE.ServerChallenge,
            //        ClientChallenge
            //      )
            //    )[0..7]
            //  )
            memcpy(&tmpCat[0], ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
            memcpy(&tmpCat[MSV1_0_CHALLENGE_LENGTH], ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
            MD5(tmpCat, MSV1_0_CHALLENGE_LENGTH * 2, tmpMD5);
            /* uses only first 8 bytes from tmpMD5! */
            DESL(ResponseKeyNT, tmpMD5, NtChallengeResponse);
            //Set LmChallengeResponse to ConcatenationOf{ClientChallenge,
            //Z(16)}
            memcpy(LmChallengeResponse, ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
            memset(LmChallengeResponse + MSV1_0_CHALLENGE_LENGTH, 0,
                   MSV1_0_NTLM3_RESPONSE_LENGTH - MSV1_0_CHALLENGE_LENGTH);
        }
        //Else
        else
        {
            //Set NtChallengeResponse to DESL(ResponseKeyNT,
            //CHALLENGE_MESSAGE.ServerChallenge)
            DESL(ResponseKeyNT, ServerChallenge, NtChallengeResponse);
            FIXME("TODO: Implement client option NoLMResponseNTLMv1 - assume TRUE!\n");
            //If (NoLMResponseNTLMv1 is TRUE)
            if (TRUE)
            {
                // set LmChallengeResponse to NtChallengeResponse
                memcpy(LmChallengeResponse, NtChallengeResponse, MSV1_0_RESPONSE_LENGTH);
            }
            else
            {
                //Set LmChallengeResponse to DESL(ResponseKeyLM,
                //CHALLENGE_MESSAGE.ServerChallenge)
                DESL(ResponseKeyLM, ServerChallenge, LmChallengeResponse);
            }
        }
    //EndI
    }
    // Set SessionBaseKey to MD4(NTOWF)
    MD4(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH, (UCHAR*)SessionBaseKey);
    return TRUE;
}

/* MS-NLSP 3.3.2 NTLM v2 Authentication */
//#define VALIDATE_NTLMv2
//#define VALIDATE_NTLM
BOOL
CliComputeResponseNTLMv2(
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN PEXT_STRING_W userdom,
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    IN UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH],
    IN PEXT_STRING_W ServerName,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TimeStamp,
    IN OUT PEXT_DATA pNtChallengeResponseData,
    OUT PLM2_RESPONSE pLmChallengeResponse,
    OUT PUSER_SESSION_KEY SessionBaseKey)
{
    UCHAR NTProofStr[16];
    BOOL avOk;
    PMSV1_0_NTLM3_RESPONSE pNtResponse;
    UCHAR ClientChallenge0[MSV1_0_CHALLENGE_LENGTH];

    /* prevent access to NULL pointer */
    if (ClientChallenge == NULL)
    {
        ClientChallenge = ClientChallenge0;
        memset(ClientChallenge0, 0, MSV1_0_CHALLENGE_LENGTH);
    }


    TRACE("XXX\n");
    TRACE("user %S\n", user->Buffer);
    TRACE("pass %S\n", passwd->Buffer);
    TRACE("userdom %S\n", userdom->Buffer);
    TRACE("ServerName %S\n", ServerName->Buffer);
    TRACE("ResponseKeyLM\n");
    NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_OWF_LENGTH);
    TRACE("ResponseKeyNT\n");
    NtlmPrintHexDump(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH);
    TRACE("ServerChallenge\n");
    NtlmPrintHexDump(ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ClientChallenge\n");
    NtlmPrintHexDump(ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
    TRACE("TimeStamp\n");
    TRACE("0x%x\n", TimeStamp);

    /* alloc/fill NtResponse struct */
    ExtDataSetLength(pNtChallengeResponseData,
                     sizeof(MSV1_0_NTLM3_RESPONSE) +
                     sizeof(MSV1_0_AV_PAIR) * 3 +
                     ServerName->bUsed +
        #ifdef VALIDATE_NTLMv2
                     20 + /* HACK */
        #endif
                     userdom->bUsed,
                     TRUE);
    pNtResponse = (PMSV1_0_NTLM3_RESPONSE)pNtChallengeResponseData->Buffer;
    pNtResponse->RespType = 1;
    pNtResponse->HiRespType = 1;
    pNtResponse->Flags = 0;
    pNtResponse->MsgWord = 0;
    pNtResponse->TimeStamp = TimeStamp;
    pNtResponse->AvPairsOff = 0;
    memcpy(pNtResponse->ChallengeFromClient, ClientChallenge,
           ARRAYSIZE(pNtResponse->ChallengeFromClient));
    /* Av-Pairs should begin at AvPairsOff field. So we need
       to set the used-ptr back before writing avl */
    pNtChallengeResponseData->bUsed = FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, Buffer);
    // TEST_CALCULATION
    #ifdef VALIDATE_NTLMv2
    pNtResponse->TimeStamp = 0;
    memset(pNtResponse->ChallengeFromClient, 0xaa, 8);
    /* AV-Pais (Domain / Server) */
    avOk = TRUE;
    if (userdom->bUsed > 0)
        avOk = avOk &&
               NtlmAvlAdd(pNtChallengeResponseData, MsvAvNbDomainName, (WCHAR*)L"Domain", 12);
    avOk = avOk &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvNbComputerName, (WCHAR*)L"Server", 12) &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvEOL, NULL, 0);
    #else
    avOk = TRUE;
    if (userdom->bUsed > 0)
        avOk = avOk &&
               NtlmAvlAdd(pNtChallengeResponseData, MsvAvNbDomainName, (WCHAR*)userdom->Buffer, userdom->bUsed);
    avOk = avOk &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvNbComputerName, ServerName->Buffer, ServerName->bUsed) &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvEOL, NULL, 0);
    #endif
    if (!avOk)
       ERR("failed to write avl data\n");

    //Define ComputeResponse(NegFlg, ResponseKeyNT, ResponseKeyLM,
    //CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge, Time, ServerName)
    //As
    //If (User is set to "" && Passwd is set to "")
    if ((user->bUsed == 0) &&
        (passwd->bUsed == 0))
    {
        //-- Special case for anonymous authentication
        //Set NtChallengeResponseLen to 0
        //Set NtChallengeResponseMaxLen to 0
        //Set NtChallengeResponseBufferOffset to 0
        //Set LmChallengeResponse to Z(1)
        //Else
        ERR("FIXME no user/pass case!\n");
    }
    else
    {
        //UNICODE_STRING val;
        PBYTE ccTemp;
        PBYTE temp;
        EXT_STRING_A ServerNameOEM;
        int tempLen, ccTempLen;

        ExtWStrToAStr(&ServerNameOEM, ServerName, TRUE, TRUE);
        //FILETIME ft;
        //SYSTEMTIME st;
        //ft.dwLowDateTime = (pNtResponse->TimeStamp && 0xFFFFFFFF);
        //ft.dwHighDateTime = ((pNtResponse->TimeStamp << 32) && 0xFFFFFFFF);
        //if (!FileTimeToSystemTime(&ft, &st))
        //    RtlZeroMemory(&st, sizeof(st));

        //Set temp to ConcatenationOf(Responserversion, HiResponserversion,
        //Z(6), Time, ClientChallenge, Z(4), ServerName, Z(4))
        temp = ((PBYTE)pNtResponse) +
               FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, RespType);
        /* Spec (4.2.4.1.3 temp) shows 4 bytes more
         * (all 0) may be a bug in spec-doc. It works
         * without these extra bytes. */
        tempLen = pNtChallengeResponseData->bUsed -
                  FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, RespType);
        #ifdef VALIDATE_NTLMv2
        NtlmPrintHexDump((PBYTE)pNtChallengeResponseData->pData, pNtChallengeResponseData->bUsed);
        TRACE("%p %i %p %i\n", pNtResponse, temp, pNtChallengeResponseData->pData, tempLen);
        TRACE("**** VALIDATE **** temp\n");
        NtlmPrintHexDump((PBYTE)temp, tempLen);
        #endif

        //Set NTProofStr to
        //  HMAC_MD5(ResponseKeyNT,
        //           ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge,temp))
        ccTempLen = MSV1_0_CHALLENGE_LENGTH + tempLen;
        ccTemp = NtlmAllocate(ccTempLen);
        memcpy(ccTemp, ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, temp, tempLen);
        HMAC_MD5(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH,
                 ccTemp, ccTempLen, NTProofStr);
        NtlmFree(ccTemp);

        //Set NtChallengeResponse to ConcatenationOf(NTProofStr, temp)
        //memcpy(&NtChallengeResponse[0], NTProofStr, ARRAYSIZE(NTProofStr));//16
        memcpy(&pNtResponse->Response[0], NTProofStr, ARRAYSIZE(NTProofStr));//16
        /* MS-NLMP says concat temp ... however
         * Response is oly 16 Byte ... temp points after it
         * and this is the same address (temp == &pNtResponse->Response[16])
         * which is already filled - so copy makes no sense */
        #ifdef VALIDATE_NTLMv2
        //TRACE("**** VALIDATE **** NTProofStr\n");
        //NtlmPrintHexDump(NTProofStr, ARRAYSIZE(NTProofStr));
        TRACE("**** VALIDATE **** NtChallengeResponse\n");
        NtlmPrintHexDump(pNtResponse->Response, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        //Set LmChallengeResponse to ConcatenationOf(HMAC_MD5(ResponseKeyLM,
        //ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge)),
        //ClientChallenge )
        ccTempLen = 2 * MSV1_0_CHALLENGE_LENGTH;
        ccTemp = NtlmAllocate(ccTempLen);
        memcpy(ccTemp, ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
        HMAC_MD5(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH,
                 ccTemp, ccTempLen,
                 pLmChallengeResponse->Response);
        NtlmFree(ccTemp);
        memcpy(pLmChallengeResponse->ChallengeFromClient, ClientChallenge, 8);
        #ifdef VALIDATE_NTLMv2
        TRACE("**** VALIDATE **** LmChallengeResponse\n");
        NtlmPrintHexDump((PBYTE)pLmChallengeResponse, 24);
        #endif
    }
    //EndIf
    //Set SessionBaseKey to HMAC_MD5(ResponseKeyNT, NTProofStr)
    //EndDefine
    HMAC_MD5(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH,
             NTProofStr, ARRAYSIZE(NTProofStr),
             (UCHAR*)SessionBaseKey);
    #ifdef VALIDATE_NTLMv2
    TRACE("**** VALIDATE **** SessionBaseKey\n");
    NtlmPrintHexDump((PBYTE)SessionBaseKey, 16);
    #endif
    return TRUE;
}

BOOL
CliComputeKeys(
    IN ULONG ChallengeMsg_NegFlg,
    IN PUSER_SESSION_KEY pSessionBaseKey,
    IN PEXT_DATA pLmChallengeResponseData,
    IN PEXT_DATA pNtChallengeResponseData,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    OUT UCHAR ExportedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH],
    OUT PEXT_DATA pEncryptedRandomSessionKey,
    OUT PNTLMSSP_CONTEXT_MSG ctxmsg)
{
    UCHAR KeyExchangeKey[16];
    TRACE("=== ComputeKeys ===\n");
    TRACE("ChallengeMsg_NegFlg\n");
    NtlmPrintNegotiateFlags(ChallengeMsg_NegFlg);
    TRACE("pSessionBaseKey\n");
    NtlmPrintHexDump((PBYTE)pSessionBaseKey, sizeof(USER_SESSION_KEY));
    TRACE("LmChallengeResponse\n");
    NtlmPrintHexDump(pLmChallengeResponseData->Buffer, pLmChallengeResponseData->bUsed);
    TRACE("ServerChallenge\n");
    NtlmPrintHexDump(ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ResponseKeyLM\n");
    NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_OWF_LENGTH);
    //Set KeyExchangeKey to KXKEY(SessionBaseKey, LmChallengeResponse,
    //CHALLENGE_MESSAGE.ServerChallenge)
    KXKEY(ChallengeMsg_NegFlg, (PUCHAR)pSessionBaseKey,
          pLmChallengeResponseData, pNtChallengeResponseData,
          ServerChallenge, ResponseKeyLM, KeyExchangeKey);
    TRACE("KeyExchangeKey\n");
    NtlmPrintHexDump(KeyExchangeKey, 16);

    if (ChallengeMsg_NegFlg & NTLMSSP_NEGOTIATE_KEY_EXCH)
    {
        //Set ExportedSessionKey to NONCE(16)
        NONCE(ExportedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        //Set AUTHENTICATE_MESSAGE.EncryptedRandomSessionKey to
        //RC4K(KeyExchangeKey, ExportedSessionKey)
        ExtDataSetLength(pEncryptedRandomSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH, TRUE);
        RC4K(KeyExchangeKey, ARRAYSIZE(KeyExchangeKey),
             ExportedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH,
             pEncryptedRandomSessionKey->Buffer);
    }
    else
    {
        // Set ExportedSessionKey to KeyExchangeKey
        memcpy(ExportedSessionKey, KeyExchangeKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        // Set AUTHENTICATE_MESSAGE.EncryptedRandomSessionKey to NIL
    }
    //ClientSigningKey SIGNKEY(NegFlg,ExportedSessionKey,C
    SIGNKEY(ExportedSessionKey, TRUE, ctxmsg->ClientSigningKey);
    //ServerSigningKey SIGNKEY(NegFlg,ExportedSessionKey,S
    SIGNKEY(ExportedSessionKey, FALSE, ctxmsg->ServerSigningKey);
    //ClientSealingKey SEALKEY(NegFlg,ExportedSessionKey,C
    SEALKEY(ChallengeMsg_NegFlg, ExportedSessionKey, TRUE, ctxmsg->ClientSealingKey);
    //ServerSealingKey SEALKEY(NegFlg,ExportedSessionKey,S
    SEALKEY(ChallengeMsg_NegFlg, ExportedSessionKey, FALSE, ctxmsg->ServerSealingKey);

    //RC4Init(ClientHandle, ctxmsg->ClientSealingKey)
    RC4Init(&ctxmsg->ClientHandle, ctxmsg->ClientSealingKey, ARRAYSIZE(ctxmsg->ClientSealingKey));
    //RC4Init(ServerHandle, ctxmsg->ServerSealingKey)
    RC4Init(&ctxmsg->ServerHandle, ctxmsg->ServerSealingKey, ARRAYSIZE(ctxmsg->ServerSealingKey));
    //TODO  Set MIC to HMAC_MD5(ExportedSessionKey, ConcatenationOf(
    //TODO  NEGOTIATE_MESSAGE, CHALLENGE_MESSAGE, AUTHENTICATE_MESSAGE))
    //TODO  Set AUTHENTICATE_MESSAGE.MIC to MIC
    //... HMAC_MD5(ExportdSessionKey, sizeof(ExportedSessionKey), );
    return TRUE;
}

BOOL
CliComputeResponse(
    /* really 2 x Negflg needed ? */
    IN ULONG NegFlg,
    IN ULONG Challenge_NegFlg,
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN PEXT_STRING_W userdom,
    IN PEXT_STRING_W pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TimeStamp,
    IN OUT PNTLMSSP_CONTEXT_MSG ctxmsg,
    IN OUT PEXT_DATA pNtChallengeResponseData,
    /* NTLMv1 UCHAR[16]
     * NTLMv2 PLM2_RESPONSE */
    IN OUT PEXT_DATA pLmChallengeResponseData,
    IN OUT PEXT_DATA EncryptedRandomSessionKey)
{
    BOOL UseNTLMv2 = (getGlobalsCli()->CliLMLevel & CLI_LMFLAG_USE_AUTH_NTLMv2);
    UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH];
    UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH];
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
    USER_SESSION_KEY SessionBaseKey;
    UCHAR ExportedSessionKey[16];

    TRACE("%wZ %wZ %wZ %wZ %p %p %p %p\n",
        user, passwd, userdom, pServerName, ChallengeToClient,
        pNtChallengeResponseData->Buffer,
        pLmChallengeResponseData->Buffer);

    /* 3.1.5.1.2 nonce */
    NtlmGenerateRandomBits(ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

    /* MS-NLSP 3.3.2 NTLM v2 Authentication */
    //Define NTOWFv2(Passwd, User, UserDom) as HMAC_MD5(
    //MD4(UNICODE(Passwd)), UNICODE(ConcatenationOf( Uppercase(User),
    //UserDom ) ) )
    //EndDefine

    //Define LMOWFv2(Passwd, User, UserDom) as NTOWFv2(Passwd, User,
    //UserDom)
    //EndDefine

    #ifdef VALIDATE_NTLM
    {
        ExtWStrSet(user, L"User");
        ExtWStrSet(passwd, L"Password");
        ExtWStrSet(userdom, L"Domain");
        memcpy(ChallengeFromClient, "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 8);
        memcpy(ChallengeToClient, "\x01\x23\x45\x67\x89\xab\xcd\xef", 8);
    }
    #endif
    if (!UseNTLMv2)
    {
        EXT_STRING_A passwdOEM;
        //Z(M)- Defined in section 6.
        //Define NTOWFv1(Passwd, User, UserDom) as MD4(UNICODE(Passwd))
        //EndDefine

        //Define LMOWFv1(Passwd, User, UserDom) as
        //ConcatenationOf( DES( UpperCase( Passwd)[0..6],"KGS!@#$%"),
        //DES( UpperCase( Passwd)[7..13],"KGS!@#$%"))
        //EndDefine

        //Set ResponseKeyNT to NTOWFv1(Passwd, User, UserDom)
        if (!NTOWFv1((WCHAR*)passwd->Buffer, ResponseKeyNT))
        {
            ERR("NTOWFv1 failed\n");
            return FALSE;
        }
        #ifdef VALIDATE_NTLMv1
        TRACE("**** VALIDATE **** ResponseKeyNT\n");
        NtlmPrintHexDump(ResponseKeyNT, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        //Set ResponseKeyLM to LMOWFv1( Passwd, User, UserDom )
        if (!ExtWStrToAStr(&passwdOEM, passwd, TRUE, TRUE))
        {
            ERR("ExtWStrToAStr failed\n");
            return FALSE;
        }
        LMOWFv1((char*)passwdOEM.Buffer, ResponseKeyLM);
        ExtStrFree(&passwdOEM);
        #ifdef VALIDATE_NTLMv1
        TRACE("**** VALIDATE **** ResponseKeyLM\n");
        NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        /* prepare ComputeResponse */
        ExtDataSetLength(pNtChallengeResponseData, MSV1_0_RESPONSE_LENGTH, TRUE);
        ExtDataSetLength(pLmChallengeResponseData, MSV1_0_RESPONSE_LENGTH, TRUE);

        if (!CliComputeResponseNTLMv1(NegFlg,
                                      user,
                                      passwd,
                                      ResponseKeyLM,
                                      ResponseKeyNT,
                                      ChallengeToClient,
                                      ChallengeFromClient,
                                      (PUCHAR)pNtChallengeResponseData->Buffer,
                                      (PUCHAR)pLmChallengeResponseData->Buffer,
                                      &SessionBaseKey))
        {
            ExtStrFree(pNtChallengeResponseData);
            ExtStrFree(pLmChallengeResponseData);
            ERR("CliComputeResponseNTLMv1 failed!\n");
            return FALSE;
        }
        #ifdef VALIDATE_NTLMv1
        DebugBreak();
        #endif
    }
    else
    {
        //Set ResponseKeyNT to NTOWFv2(Passwd, User, UserDom)
        if (!NTOWFv2((WCHAR*)passwd->Buffer, (WCHAR*)user->Buffer,
                     (WCHAR*)userdom->Buffer, ResponseKeyNT))
        {
            ERR("NTOWFv2 failed\n");
            return FALSE;
        }
        #ifdef VALIDATE_NTLMv2
        //TRACE("**** VALIDATE **** ResponseKeyNT\n");
        //NtlmPrintHexDump(ResponseKeyNT, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        //Set ResponseKeyLM to LMOWFv2(Passwd, User, UserDom)
        if (!LMOWFv2((WCHAR*)passwd->Buffer, (WCHAR*)user->Buffer,
                     (WCHAR*)userdom->Buffer, ResponseKeyLM))
        {
            ERR("LMOWFv2 failed\n");
            return FALSE;
        }
        #ifdef VALIDATE_NTLMv2
        TRACE("**** VALIDATE **** ResponseKeyLM\n");
        NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        /* prepare CompureResponse */
        ExtDataSetLength(pLmChallengeResponseData, sizeof(LM2_RESPONSE), TRUE);

        if (!CliComputeResponseNTLMv2(user,
                                      passwd,
                                      userdom,
                                      ResponseKeyLM,
                                      ResponseKeyNT,
                                      pServerName,
                                      ChallengeToClient,
                                      ChallengeFromClient,
                                      TimeStamp,
                                      pNtChallengeResponseData,
                                      (PLM2_RESPONSE)pLmChallengeResponseData->Buffer,
                                      &SessionBaseKey))
        {
            ExtStrFree(pLmChallengeResponseData);
            ERR("ComputeResponseNVLMv2 failed!\n");
            return FALSE;
        }
        /* uses same key ... */
        //memcpy(pLmSessionKey, pUserSessionKey, sizeof(*pLmSessionKey));
    }

    TRACE("=== CliComputeKeys === \n\n");
    TRACE("Challenge_NegFlg\n");
    TRACE("0x%x\n", Challenge_NegFlg);
    TRACE("SessionBaseKey\n");
    NtlmPrintHexDump((PBYTE)&SessionBaseKey, sizeof(SessionBaseKey));
    TRACE("pLmChallengeResponseData\n");
    NtlmPrintHexDump(pLmChallengeResponseData->Buffer, pLmChallengeResponseData->bUsed);
    TRACE("ServerChallenge\n");
    NtlmPrintHexDump(ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ResponseKeyLM\n");
    NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_OWF_LENGTH);
    TRACE("ExportedSessionKey\n");
    NtlmPrintHexDump(ExportedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);

    if (!CliComputeKeys(Challenge_NegFlg,
                        &SessionBaseKey,
                        pLmChallengeResponseData,
                        pNtChallengeResponseData,
                        ChallengeToClient,/* = ServerChallenge*/
                        ResponseKeyLM,
                        ExportedSessionKey,
                        EncryptedRandomSessionKey,
                        ctxmsg))
    {
        ERR("CliComputeKeys error\n");
        return FALSE;
    }

    return TRUE;
}

/*
VOID
NtpLmSessionKeys(IN PUSER_SESSION_KEY NtpUserSessionKey,
                IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
                IN UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
                OUT PUSER_SESSION_KEY pUserSessionKey,
                OUT PLM_SESSION_KEY pLmSessionKey)
{
    HMAC_MD5_CTX ctx;

    HMACMD5Init(&ctx, (PUCHAR)NtpUserSessionKey, sizeof(*NtpUserSessionKey));
    HMACMD5Update(&ctx, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Update(&ctx, ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Final(&ctx, (PUCHAR)pUserSessionKey);
    memcpy(pLmSessionKey, pUserSessionKey, sizeof(*pLmSessionKey));
}
*/

VOID
RC4Init(
    OUT prc4_key pHandle,
    IN UCHAR* Key,
    IN ULONG KeyLen)
{
    rc4_init(pHandle, Key, KeyLen);
}

VOID
RC4(IN prc4_key pHandle,
    IN UCHAR* pDataI,
    OUT UCHAR* pDataO,
    IN ULONG len)
{
    /* use pData for in/out should be okay - i think! */
    rc4_crypt(pHandle, pDataI, pDataO, len);
}
