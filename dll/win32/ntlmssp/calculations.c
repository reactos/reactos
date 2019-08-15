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

BOOLEAN
NTOWFv1(LPCWSTR password,
        PUCHAR result)
{
    ULONG i;
    size_t len;
    WCHAR pass[14];

    if ((password == NULL) ||
        (FAILED(RtlStringCchLengthW(password, MAX_PASSWD_LEN, &len))))
    {
        *result = 0;
        return FALSE;
    }

    memcpy(pass, password, len * sizeof(WCHAR));
    for(i = len; i<14; i++)
    {
        pass[i] = L'0';
    }
    MD4((PUCHAR)pass, 14, result);

    return TRUE;
}

// MS-NLSP 3.3.2
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

VOID
LMOWFv1(const PCCHAR password, PUCHAR result)
{
#if 0
    SystemFunction006(password, result);
#else
    /* "KGS!@#$%" */
    UCHAR magic[] = { 0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25 };
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

VOID
KXKEY(ULONG flags,
      const PUCHAR session_base_key,
      const PUCHAR lm_challenge_resonse,
      const PUCHAR server_challenge,
      PUCHAR key_exchange_key)
{
    /* fix me */
    memcpy(key_exchange_key, session_base_key, 16);
}

BOOLEAN
SIGNKEY(const PUCHAR RandomSessionKey, BOOLEAN IsClient, PUCHAR Result)
{
    PCHAR magic = IsClient
        ? "session key to client-to-server signing key magic constant"
        : "session key to server-to-client signing key magic constant";
    ULONG len = strlen(magic);
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

BOOLEAN
SEALKEY(ULONG flags, const PUCHAR RandomSessionKey, BOOLEAN client, PUCHAR result)
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

BOOLEAN
MAC(ULONG flags,
    PCCHAR buf,
    ULONG buf_len,
    PUCHAR sign_key,
    ULONG sign_key_len,
    PUCHAR seal_key,
    ULONG seal_key_len,
    ULONG random_pad,
    ULONG sequence,
    PUCHAR result)
{
    ULONG *res_ptr;

    if (flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        UCHAR seal_key_ [16];
        UCHAR hmac[16];
        UCHAR *tmp;

        /* SealingKey' = MD5(ConcatenationOf(SealingKey, SequenceNumber))
        RC4Init(Handle, SealingKey')

        */
        if (flags & NTLMSSP_NEGOTIATE_DATAGRAM)
        {
            UCHAR tmp2 [16+4];
            memcpy(tmp2, seal_key, seal_key_len);
            *((ULONG *)(tmp2+16)) = sequence;
            MD5 (tmp2, 16+4, seal_key_);
        } else {
            memcpy(seal_key_, seal_key, seal_key_len);
        }

        TRACE("NTLM MAC(): Extented Session Security\n");

        res_ptr = (ULONG *)result;
        res_ptr[0] = 0x00000001L;
        res_ptr[3] = sequence;

        tmp = NtlmAllocate(4 + buf_len);
        res_ptr = (ULONG *)tmp;
        res_ptr[0] = sequence;
        memcpy(tmp+4, buf, buf_len);

        HMAC_MD5(sign_key, sign_key_len, tmp, 4 + buf_len, hmac);

        NtlmFree(tmp);

        if (flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            TRACE("NTLM MAC(): Key Exchange\n");
            RC4K(seal_key_, seal_key_len, hmac, 8, result+4);
        } else {
            TRACE("NTLM MAC(): *NO* Key Exchange\n");
            memcpy(result+4, hmac, 8);
        }
    } else {
        /* The content of the first 4 bytes is irrelevant */
        ULONG crc = CRC32(buf, strlen(buf));
        ULONG plaintext [] = { 0, crc, sequence };

        TRACE("NTLM MAC(): *NO* Extented Session Security\n");

        RC4K(seal_key, seal_key_len, (const PUCHAR )plaintext, 12, result+4);

        res_ptr = (ULONG *)result;
        // Highest four bytes are the Version
        res_ptr[0] = 0x00000001; // 4 bytes

        // Replace the first four bytes of the ciphertext with the random_pad
        res_ptr[1] = random_pad; // 4 bytes
    }

    return TRUE;
}

#if 0 /* old calcs */
VOID
NtlmLmResponse(IN PEXT_STRING pUserNameW,
               IN PEXT_STRING pPasswordW,
               IN PEXT_STRING pDomainNameW,
               IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
               IN PLM2_RESPONSE pLm2Response,
               OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH])
{
    HMAC_MD5_CTX ctx;
    UCHAR NtlmOwf[MSV1_0_NTLM3_OWF_LENGTH];

    NTOWFv2((WCHAR*)pPasswordW->Buffer,
            (WCHAR*)pUserNameW->Buffer,
            (WCHAR*)pDomainNameW->Buffer,
            NtlmOwf);

    HMACMD5Init(&ctx, NtlmOwf, MSV1_0_NTLM3_OWF_LENGTH);
    HMACMD5Update(&ctx, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Update(&ctx, (PUCHAR)pLm2Response->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Final(&ctx, Response);

    return;
}

VOID
NtlmNtResponse(IN PEXT_STRING pUserNameW,
               IN PEXT_STRING pPasswordW,
               IN PEXT_STRING pDomainNameW,
               IN ULONG ServerNameLength,
               IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
               IN PMSV1_0_NTLM3_RESPONSE pNtResponse,
               OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH],
               OUT PUSER_SESSION_KEY pUserSessionKey,
               OUT PLM_SESSION_KEY pLmSessionKey)
{
    HMAC_MD5_CTX ctx;
    UCHAR NtlmOwf[MSV1_0_NTLM3_OWF_LENGTH];

    NTOWFv2((WCHAR*)pPasswordW->Buffer,
            (WCHAR*)pUserNameW->Buffer,
            (WCHAR*)pDomainNameW->Buffer,
            NtlmOwf);

    HMACMD5Init(&ctx, NtlmOwf, MSV1_0_NTLM3_OWF_LENGTH);
    HMACMD5Update(&ctx, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Update(&ctx, &pNtResponse->RespType,
                  MSV1_0_NTLM3_INPUT_LENGTH + ServerNameLength);
    HMACMD5Final(&ctx, Response);

    /* session keys */
    HMAC_MD5(NtlmOwf, MSV1_0_NTLM3_OWF_LENGTH,
             Response, MSV1_0_NTLM3_RESPONSE_LENGTH, (PUCHAR)pUserSessionKey);

    //*pLmSessionKey = pUserSessionKey;
    memcpy(pLmSessionKey, pUserSessionKey, MSV1_0_LANMAN_SESSION_KEY_LENGTH);
    return;
}

VOID
NtlmChallengeResponse(
    IN PEXT_STRING pUserNameW,
    IN PEXT_STRING pPasswordW,
    IN PEXT_STRING pDomainNameW,
    IN PUNICODE_STRING pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TimeStamp,
    OUT PNTLM_DATABUF pNtResponseData,
    OUT PLM2_RESPONSE pLm2Response,
    OUT PUSER_SESSION_KEY pUserSessionKey,
    OUT PLM_SESSION_KEY pLmSessionKey)
{
    PMSV1_0_NTLM3_RESPONSE pNtResponse;
    BOOL avOk;

    /* alloc memory */
    NtlmDataBufAlloc(pNtResponseData,
                     sizeof(MSV1_0_NTLM3_RESPONSE) +
                     sizeof(MSV1_0_AV_PAIR) * 3 +
                     pServerName->Length +
                     pDomainNameW->bUsed,
                     TRUE);
    pNtResponse = (PMSV1_0_NTLM3_RESPONSE)pNtResponseData->pData;

    pNtResponse->RespType = 1;
    pNtResponse->HiRespType = 1;
    pNtResponse->Flags = 0;
    pNtResponse->MsgWord = 0;
    pNtResponse->TimeStamp = TimeStamp;

    /* Av-Pairs should begin at AvPairsOff field. So we need
       to set the used-ptr back before writing avl */
    pNtResponseData->bUsed = FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, Buffer);

    avOk = NtlmAvlAdd(pNtResponseData, MsvAvNbComputerName, pServerName->Buffer, pServerName->Length);
    if (pDomainNameW->bUsed > 0)
        avOk = avOk &&
               NtlmAvlAdd(pNtResponseData, MsvAvNbDomainName, (WCHAR*)pDomainNameW->Buffer, pDomainNameW->bUsed);
    avOk = avOk &&
           NtlmAvlAdd(pNtResponseData, MsvAvEOL, NULL, 0);
    if (!avOk)
       ERR("failed to write avl data\n");

    TRACE("%wZ %wZ %wZ %wZ %p %p %p %p %p\n",
        pUserNameW, pPasswordW, pDomainNameW, pServerName, ChallengeToClient,
        pNtResponse, pLm2Response, pUserSessionKey, pLmSessionKey);

    /* 3.1.5.1.2 nonce */
    NtlmGenerateRandomBits(pNtResponse->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

    NtlmNtResponse(pUserNameW,
                   pPasswordW,
                   pDomainNameW,
                   pServerName->Length,
                   ChallengeToClient,
                   pNtResponse,
                   pNtResponse->Response,
                   pUserSessionKey,
                   pLmSessionKey);

    /* Use same challenge to compute the LM3 response */
    memcpy(pLm2Response->ChallengeFromClient,
           pNtResponse->ChallengeFromClient,
           MSV1_0_CHALLENGE_LENGTH);

    NtlmLmResponse(pUserNameW,
                   pPasswordW,
                   pDomainNameW,
                   ChallengeToClient,
                   pLm2Response,
                   pLm2Response->Response);
}
#else
/* MS-NLSP 3.3.2 NTLM v2 Authentication */
//#define VALIDATE_NTLMv2
BOOL
ComputeResponseNVLMv2(
    IN PEXT_STRING user,
    IN PEXT_STRING passwd,
    IN PEXT_STRING userdom,
    IN PUNICODE_STRING ServerName,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    // fixme: NÖTIG ... evtl hier füllen oder länge ist nötig ...
    IN PMSV1_0_NTLM3_RESPONSE pNtResponse,
    IN ULONG pNtResponseLen,
    OUT UCHAR NtChallengeResponse[24],
    OUT PLM2_RESPONSE pLmChallengeResponse,
    OUT PUSER_SESSION_KEY SessionBaseKey)
{
    UCHAR NTProofStr[16];
    UCHAR ResponseKeyLM[MSV1_0_NTLM3_RESPONSE_LENGTH];
    UCHAR ResponseKeyNT[MSV1_0_NTLM3_RESPONSE_LENGTH];

    //Define NTOWFv2(Passwd, User, UserDom) as HMAC_MD5(
    //MD4(UNICODE(Passwd)), UNICODE(ConcatenationOf( Uppercase(User),
    //UserDom ) ) )
    //EndDefine

    //Define LMOWFv2(Passwd, User, UserDom) as NTOWFv2(Passwd, User,
    //UserDom)
    //EndDefine

    #ifdef VALIDATE_NTLMv2
    {
        ExtWStrSet(user, L"User");
        ExtWStrSet(passwd, L"Password");
        ExtWStrSet(userdom, L"Domain");
        memcpy(ClientChallenge, "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 8);
        memcpy(ServerChallenge, "\x01\x23\x45\x67\x89\xab\xcd\xef", 8);
    }
    #endif

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
        OEM_STRING ServerNameOEM;
        int tempLen, ccTempLen;

        RtlUnicodeStringToOemString(&ServerNameOEM, ServerName, TRUE);
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
        tempLen = pNtResponseLen -
                  FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, RespType);
        #ifdef VALIDATE_NTLMv2
        NtlmPrintHexDump((PBYTE)pNtResponse, pNtResponseLen);
        TRACE("%p %i %p %i\n", pNtResponse, temp, pNtResponseLen, tempLen);
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
        HMAC_MD5(ResponseKeyNT, MSV1_0_NTLM3_RESPONSE_LENGTH,
                 ccTemp, ccTempLen, NTProofStr);
        NtlmFree(ccTemp);

        //Set NtChallengeResponse to ConcatenationOf(NTProofStr, temp)
        memcpy(&NtChallengeResponse[0], NTProofStr, ARRAYSIZE(NTProofStr));//16
        memcpy(&NtChallengeResponse[16], temp, 8);
        #ifdef VALIDATE_NTLMv2
        //TRACE("**** VALIDATE **** NTProofStr\n");
        //NtlmPrintHexDump(NTProofStr, ARRAYSIZE(NTProofStr));
        TRACE("**** VALIDATE **** NtChallengeResponse\n");
        NtlmPrintHexDump(NtChallengeResponse, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

        //Set LmChallengeResponse to ConcatenationOf(HMAC_MD5(ResponseKeyLM,
        //ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge)),
        //ClientChallenge )
        ccTempLen = 2 * MSV1_0_CHALLENGE_LENGTH;
        ccTemp = NtlmAllocate(ccTempLen);
        memcpy(ccTemp, ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
        HMAC_MD5(ResponseKeyNT, MSV1_0_NTLM3_RESPONSE_LENGTH,
                 ccTemp, ccTempLen,
                 pLmChallengeResponse->Response);
        NtlmFree(ccTemp);
        memcpy(pLmChallengeResponse->ChallengeFromClient, ClientChallenge, 8);
        #ifdef VALIDATE_NTLMv2
        TRACE("**** VALIDATE **** LmChallengeResponse\n");
        NtlmPrintHexDump(LmChallengeResponse, 24);
        #endif
    }
    //EndIf
    //Set SessionBaseKey to HMAC_MD5(ResponseKeyNT, NTProofStr)
    //EndDefine
    HMAC_MD5(ResponseKeyNT, MSV1_0_NTLM3_RESPONSE_LENGTH,
             NTProofStr, ARRAYSIZE(NTProofStr),
             (UCHAR*)SessionBaseKey);
    #ifdef VALIDATE_NTLMv2
    TRACE("**** VALIDATE **** SessionBaseKey\n");
    NtlmPrintHexDump((PBYTE)SessionBaseKey, 16);
    #endif
    return TRUE;
}

BOOL
NtlmChallengeResponse(
    IN PEXT_STRING pUserNameW,
    IN PEXT_STRING pPasswordW,
    IN PEXT_STRING pDomainNameW,
    IN PUNICODE_STRING pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TimeStamp,
    OUT PNTLM_DATABUF pNtResponseData,
    OUT PLM2_RESPONSE pLm2ChallengeResponse,
    OUT PUSER_SESSION_KEY pUserSessionKey,
    OUT PLM_SESSION_KEY pLmSessionKey)
{
    PMSV1_0_NTLM3_RESPONSE pNtResponse;
    BOOL avOk;

    /* alloc memory */
    NtlmDataBufAlloc(pNtResponseData,
                     sizeof(MSV1_0_NTLM3_RESPONSE) +
                     sizeof(MSV1_0_AV_PAIR) * 3 +
                     pServerName->Length +
                     pDomainNameW->bUsed,
        #ifdef VALIDATE_NTLMv2
                     + 20 /* HACK */
        #endif
                     TRUE);
    pNtResponse = (PMSV1_0_NTLM3_RESPONSE)pNtResponseData->pData;

    TRACE("%wZ %wZ %wZ %wZ %p %p %p %p %p\n",
        pUserNameW, pPasswordW, pDomainNameW, pServerName, ChallengeToClient,
        pNtResponse, pLm2ChallengeResponse, pUserSessionKey, pLmSessionKey);

    pNtResponse->RespType = 1;
    pNtResponse->HiRespType = 1;
    pNtResponse->Flags = 0;
    pNtResponse->MsgWord = 0;
    pNtResponse->TimeStamp = TimeStamp;
    pNtResponse->AvPairsOff = 0;

    /* Av-Pairs should begin at AvPairsOff field. So we need
       to set the used-ptr back before writing avl */
    pNtResponseData->bUsed = FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, Buffer);

    /* 3.1.5.1.2 nonce */
    NtlmGenerateRandomBits(pNtResponse->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);

    // TEST_CALCULATION
    #ifdef VALIDATE_NTLMv2
    pNtResponse->TimeStamp = 0;
    memset(pNtResponse->ChallengeFromClient, 0xaa, 8);
    /* AV-Pais (Domain / Server) */
    avOk = TRUE;
    if (pDomainNameW->bUsed > 0)
        avOk = avOk &&
               NtlmAvlAdd(pNtResponseData, MsvAvNbDomainName, (WCHAR*)L"Domain", 12);
    avOk = avOk &&
           NtlmAvlAdd(pNtResponseData, MsvAvNbComputerName, (WCHAR*)L"Server", 12) &&
           NtlmAvlAdd(pNtResponseData, MsvAvEOL, NULL, 0);
    #else
    avOk = TRUE;
    if (pDomainNameW->bUsed > 0)
        avOk = avOk &&
               NtlmAvlAdd(pNtResponseData, MsvAvNbDomainName, (WCHAR*)pDomainNameW->Buffer, pDomainNameW->bUsed);
    avOk = avOk &&
           NtlmAvlAdd(pNtResponseData, MsvAvNbComputerName, pServerName->Buffer, pServerName->Length) &&
           NtlmAvlAdd(pNtResponseData, MsvAvEOL, NULL, 0);
    #endif
    if (!avOk)
       ERR("failed to write avl data\n");

    /*NtlmNtResponse(pUserNameW,
                   pPasswordW,
                   pDomainNameW,
                   pServerName->Length,
                   ChallengeToClient,
                   pNtResponse,
                   pNtResponse->Response,
                   pUserSessionKey,
                   pLmSessionKey);

    / * Use same challenge to compute the LM3 response * /
    memcpy(pLm2Response->ChallengeFromClient,
           pNtResponse->ChallengeFromClient,
           MSV1_0_CHALLENGE_LENGTH);

    NtlmLmResponse(pUserNameW,
                   pPasswordW,
                   pDomainNameW,
                   ChallengeToClient,
                   pLm2Response,
                   pLm2Response->Response);*/
    if (!ComputeResponseNVLMv2(pUserNameW,
                               pPasswordW,
                               pDomainNameW,
                               pServerName,
                               ChallengeToClient,
                               pNtResponse->ChallengeFromClient,
                               pNtResponse,
                               pNtResponseData->bUsed,
                               pNtResponse->Response,
                               pLm2ChallengeResponse,
                               pUserSessionKey))
    {
        ERR("ComputeResponseNVLMv2 failed!\n");
        return FALSE;
    }
    /* uses same key ... */
    memcpy(pLmSessionKey, pUserSessionKey, sizeof(*pLmSessionKey));
    return TRUE;}
#endif

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
