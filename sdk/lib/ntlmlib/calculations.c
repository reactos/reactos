/*
 * PROJECT:     ntlmlib
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     provides basic functions for ntlm implementation (msv1_0)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 *
 */

/* this file contains algorithms defined in the MS-NLMP document */

#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/* MS-NLSP 3.3.1 */
NTSTATUS
NTOWFv1(
    _In_ PUNICODE_STRING Password,
    _Out_ NTLM_NT_OWF_PASSWORD Result)
{
    #if 0
    return SystemFunction007(Password, Result);
    #else

    if (Password == NULL ||
        Password->Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    MD4((PUCHAR)Password->Buffer, Password->Length, Result);

    return STATUS_SUCCESS;
    #endif
}

/**
 * @brief creates a NTLMv2 hash. Uses the pre-computed password (NTLMv1 hash).
 */

NTSTATUS
NTOWFv2ofw(
    _In_ NTLM_NT_OWF_PASSWORD md4pwd,
    _In_ PUNICODE_STRING User,
    _In_ PUNICODE_STRING Domain,
    _Out_ NTLM_NT_OWF_PASSWORD Result)
{
    UNICODE_STRING UserUserDom;
    ULONG UserDomByteLen = 0;

    if (User)
        UserDomByteLen += User->Length;
    if (Domain)
        UserDomByteLen += Domain->Length;

    /* concat upper(user) + domain */
    NtlmUStrAlloc(&UserUserDom, UserDomByteLen + sizeof(WCHAR), 0);
    RtlAppendUnicodeStringToString(&UserUserDom, User);
    NtlmUStrUpper(&UserUserDom);
    RtlAppendUnicodeStringToString(&UserUserDom, Domain);

    HMAC_MD5(md4pwd, MSV1_0_NT_OWF_PASSWORD_LENGTH,
             (UCHAR*)UserUserDom.Buffer, UserUserDom.Length, Result);

    NtlmUStrFree(&UserUserDom);
    return TRUE;
}

/* MS-NLSP 3.3.2 */
// FIXME: Set to 0 and chec if other version is okay ...
#if 1
NTSTATUS
NTOWFv2(
    _In_ LPCWSTR Password,
    _In_ LPCWSTR User,
    _In_ LPCWSTR Domain,
    _Out_ NTLM_NT_OWF_PASSWORD Result)
{
    UNICODE_STRING UserUserDom;
    UCHAR md4pwd[16];
    ULONG UserDomChLen = 0;

    if (User)
        UserDomChLen += wcslen(User);
    if (Domain)
        UserDomChLen += wcslen(Domain);

    /* concat upper(user) + domain */
    NtlmUStrAlloc(&UserUserDom, (UserDomChLen + 1) * sizeof(WCHAR), 0);
    RtlAppendUnicodeToString(&UserUserDom, User);
    NtlmUStrUpper(&UserUserDom);
    RtlAppendUnicodeToString(&UserUserDom, Domain);

    MD4((UCHAR*)Password, wcslen(Password) * sizeof(WCHAR), md4pwd);
    HMAC_MD5(md4pwd, ARRAYSIZE(md4pwd),
             (UCHAR*)UserUserDom.Buffer, UserUserDom.Length, Result);

    NtlmUStrFree(&UserUserDom);
    return TRUE;
}
#else
//FIXME: Use PUNICODE_STRING ...
NTSTATUS
NTOWFv2(
    _In_ LPCWSTR Password,
    _In_ LPCWSTR User,
    _In_ LPCWSTR Domain,
    _Out_ NTLM_NT_OWF_PASSWORD Result)
{
    NTLM_NT_OWF_PASSWORD md4pwd;
    UNICODE_STRING xUser;
    UNICODE_STRING xDomain;

    RtlInitUnicodeString(&xUser, User);
    RtlInitUnicodeString(&xDomain, Domain);

    MD4((UCHAR*)Password, wcslen(Password) * sizeof(WCHAR), md4pwd);

    NTOWFv2ofw(md4pwd, xUser, xDomain);
    return TRUE;
}
#endif

/* MS-NLSP 3.3.1 */
NTSTATUS
LMOWFv1(
    _In_ LPCSTR Password,
    _Out_ NTLM_LM_OWF_PASSWORD Result)
{
#if 1
    int i;
    CHAR UpperPasswd[15];
    ULONG len = strlen(Password);

    /* make uppercase */
    len = len > 14 ? 14 : len;
    for (i = 0; i < len; i++)
        UpperPasswd[i] = toupper(Password[i]);
    UpperPasswd[len] = 0;

    return SystemFunction006(UpperPasswd, (LPBYTE)Result);
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

    return STATUS_SUCCESS;
#endif
}

NTSTATUS
LMOWFv2(
    _In_ LPCWSTR Password,
    _In_ LPCWSTR User,
    _In_ LPCWSTR Domain,
    _Out_ NTLM_LM_OWF_PASSWORD Result)
{
    return NTOWFv2(Password, User, Domain, Result);
}

BOOLEAN
NONCE(PUCHAR buffer,
      ULONG num)
{
    #if 1
    return SystemFunction036(buffer, num);
    #else
    NtlmGenerateRandomBits(buffer, num);
    return TRUE;
    #endif
}

/* MS-NLSP 3.4.5.1 KXKEY */
VOID
KXKEY(
    IN ULONG NegFlg,
    IN UCHAR SessionBaseKey[MSV1_0_USER_SESSION_KEY_LENGTH],
    IN PSTRING LmChallengeResponse,
    IN PSTRING NtChallengeResponse,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    OUT UCHAR KeyExchangeKey[NTLM_KEYEXCHANGE_KEY_LENGTH])
{
    UCHAR* LMOWF = ResponseKeyLM;
    UCHAR DESv2[8] = "\x00\xBD\xBD\xBD\xBD\xBD\xBD\xBD";

    if (LmChallengeResponse->Length < 8)
    {
        ERR("KXKEY: LmChallengeResponseLen < 8 Bytes!\n");
        return;
    }

    /* NTLMv2 Security - only used with NTLMv1*/
    if ((NegFlg & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY) &&
        /* using NTLMv1? */
        (LmChallengeResponse->Length == 24) &&
        (NtChallengeResponse->Length == 24))
    {
        UCHAR nonce[16];
        //Define KXKEY(SessionBaseKey, LmChallengeResponse, ServerChallenge) as
        //Set KeyExchangeKey to HMAC_MD5(SessionBaseKey, ConcatenationOf(ServerChallenge,
        //LmChallengeResponse [0..7]))
        //EndDefine
        TRACE("KXKEY: NTLMv2 security\n");
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
        TRACE("KXKEY: LM_KEY!\n");
        DES(&LMOWF[0], (UCHAR*)LmChallengeResponse->Buffer, &KeyExchangeKey[0]);
        DESv2[0] = LMOWF[7];
        DES(DESv2, (UCHAR*)LmChallengeResponse->Buffer, &KeyExchangeKey[8]);
    }
    else
    {
        if (NegFlg & NTLMSSP_REQUEST_NON_NT_SESSION_KEY)
        {
            //Set KeyExchangeKey to ConcatenationOf(LMOWF[0..7], Z(8)),
            TRACE("KXKEY: NT SESSION KEY!\n");
            memcpy(KeyExchangeKey, LMOWF, 8);
            memset(&KeyExchangeKey[8], 0, 8);
        }
        else
        {
            //Set KeyExchangeKey to SessionBaseKey
            TRACE("KXKEY: ---!\n");
            memcpy(KeyExchangeKey, SessionBaseKey, NTLM_KEYEXCHANGE_KEY_LENGTH);
        }
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

    md5_input = NtlmAllocate(16 + len, FALSE);
    if (!md5_input)
        return FALSE;

    memcpy(md5_input, RandomSessionKey, 16);
    memcpy(md5_input + 16, magic, len);
    MD5(md5_input, len + 16, Result);

    NtlmFree(md5_input, FALSE);

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

        md5_input = (UCHAR*)NtlmAllocate(16+len, FALSE);
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
        {
            TRACE("signLen to small!\n");
            return FALSE;
        }
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
        data = NtlmAllocate(dataLen, FALSE);
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

        NtlmFree(data, FALSE);

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
    _In_ ULONG NegFlg,
    _In_ prc4_key Handle,
    _In_ UCHAR* SigningKey,
    _In_ ULONG SigningKeyLength,
    _In_ PULONG pSeqNum,
    _In_ OUT UCHAR* msg,
    _In_ ULONG msgLen,
    _Out_ UCHAR* pSign,
    _Out_ PULONG pSignLen)
{
    ULONG signLen = 16;
    STRING msgOrig;
    if (*pSignLen < signLen)
        return FALSE;

    /* copy msg, needed in MAC
     * The order of calls is mandatory if NTLMSSP_NEGOTIATE_KEY_EXCH flag is set.
     * Keep in mind
     *   * MAC calls RC4(Handle,..) too.
     *   * State of Handle is modified with every call to RC4.
     *   * Result of RC4 depends on state of handle.
     * MAC needs the original (not encrypted) message. Therefor
     * we make a copy (msgOrig).
     * */
    //ExtDataInit(&msgOrig, msg, msgLen);
    NtlmAStrAlloc(&msgOrig, msgLen, msgLen);
    if (!RtlCopyMemory(msgOrig.Buffer, msg, msgLen))
    {
        ERR("failed to copy msg!\n");
        return FALSE;
    }

    printf("msg\n");
    NtlmPrintHexDump(msg, msgLen);

    if (NegFlg & NTLMSSP_NEGOTIATE_SEAL)
    {
        RC4(Handle, msg, msg, msgLen);
    }
    printf("msg (encoded)\n");
    NtlmPrintHexDump(msg, msgLen);

    MAC(NegFlg,
        Handle, SigningKey, SigningKeyLength, pSeqNum,
        (UCHAR*)msgOrig.Buffer, msgOrig.Length, pSign, signLen);

    printf("sign\n");
    NtlmPrintHexDump(pSign, signLen);

    *pSignLen = signLen;

    NtlmAStrFree(&msgOrig);

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
ComputeResponseNTLMv1(
    _In_ ULONG NegFlg,
    _In_ BOOL Anonymouse,
    _In_ UCHAR ResponseKeyLM[MSV1_0_LM_OWF_PASSWORD_LENGTH],
    _In_ UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH],
    _In_ UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    _Out_ UCHAR LmChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    _Out_ UCHAR NtChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    _Out_ PUSER_SESSION_KEY SessionBaseKey)
{
    //--Define ComputeResponse(NegFlg, ResponseKeyNT, ResponseKeyLM,
    //--CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge, Time, ServerName)
    //As
    //If (User is set to "" AND Passwd is set to "")
    if (Anonymouse)
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
    #if 0 //TODO
    CalcNtUserSessionKey();
    #else
    MD4(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH, (UCHAR*)SessionBaseKey);
    #endif
    return TRUE;
}

/* MS-NLSP 3.3.2 NTLM v2 Authentication */
//#define VALIDATE_NTLMv2
//#define VALIDATE_NTLM
BOOL
ComputeResponseNTLMv2(
    _In_ BOOL Anonymouse,
    _In_ PUNICODE_STRING UserDom,
    _In_ UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    _In_ UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH],
    _In_ PUNICODE_STRING ServerName,
    _In_ UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    _In_ ULONGLONG ChallengeTimestamp,
    _Out_ PLM2_RESPONSE pLmChallengeResponse,
    _In_ PSTRING pNtChallengeResponseData,
    _Out_ PUSER_SESSION_KEY SessionBaseKey)
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

    TRACE("UserDom %S\n", UserDom->Buffer);
    TRACE("ServerName %S\n", ServerName->Buffer);
    TRACE("ResponseKeyLM\n");
    NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_OWF_LENGTH);
    TRACE("ResponseKeyNT\n");
    NtlmPrintHexDump(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH);
    TRACE("ServerChallenge\n");
    NtlmPrintHexDump(ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ClientChallenge\n");
    NtlmPrintHexDump(ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ChallengeTimestamp\n");
    TRACE("0x%lx\n", (ULONG)ChallengeTimestamp);

    /* alloc/fill NtResponse struct */
    NtlmAStrAlloc(pNtChallengeResponseData,
                  sizeof(MSV1_0_NTLM3_RESPONSE) +
                  4 + 4 +/*?*/
                  sizeof(MSV1_0_AV_PAIR) * 3 +
                  ServerName->Length + /*Hack*/
        #ifdef VALIDATE_NTLMv2
                  20 + /* HACK */
        #endif
                  UserDom->Length,
                  0);
    pNtResponse = (PMSV1_0_NTLM3_RESPONSE)pNtChallengeResponseData->Buffer;
    pNtResponse->RespType = 1;
    pNtResponse->HiRespType = 1;
    pNtResponse->Flags = 0;
    pNtResponse->MsgWord = 0;
    pNtResponse->TimeStamp = ChallengeTimestamp;
    pNtResponse->AvPairsOff = 0;
    pNtResponse->Buffer[0] = 2; //??
    memcpy(pNtResponse->ChallengeFromClient, ClientChallenge,
           ARRAYSIZE(pNtResponse->ChallengeFromClient));
    /* Av-Pairs should begin at AvPairsOff field. So we need
       to set the used-ptr back before writing avl */
    pNtChallengeResponseData->Length = FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, Buffer)+4/*?*/;
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
    avOk = avOk &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvNbComputerName, ServerName->Buffer, ServerName->Length);
    if (UserDom->Length > 0)
        avOk = avOk &&
            NtlmAvlAdd(pNtChallengeResponseData, MsvAvDnsComputerName, UserDom->Buffer, UserDom->Length);
    avOk = avOk &&
           NtlmAvlAdd(pNtChallengeResponseData, MsvAvEOL, NULL, 0);
    #endif
    if (!avOk)
    {
       ERR("failed to write avl data\n");
       //TODO return STATUS_INTERNAL_ERROR;
       return FALSE;
    }
    pNtChallengeResponseData->Length += 4;

    #if 0
    CalcLm2UserSessionKey();
    #else
    //Define ComputeResponse(NegFlg, ResponseKeyNT, ResponseKeyLM,
    //CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge, Time, ServerName)
    //As
    //If (User is set to "" && Passwd is set to "")
    if (Anonymouse)
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
        int tempLen, ccTempLen;

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
        tempLen = pNtChallengeResponseData->Length -
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
        ccTemp = NtlmAllocate(ccTempLen, FALSE);
        memcpy(ccTemp, ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, temp, tempLen);
        HMAC_MD5(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH,
                 ccTemp, ccTempLen, NTProofStr);
        NtlmFree(ccTemp, FALSE);

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

        /* The LMv2 User Session Key */
        //Set LmChallengeResponse to ConcatenationOf(HMAC_MD5(ResponseKeyLM,
        //ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge)),
        //ClientChallenge )
        ccTempLen = 2 * MSV1_0_CHALLENGE_LENGTH;
        ccTemp = NtlmAllocate(ccTempLen, FALSE);
        memcpy(ccTemp, ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, ClientChallenge, MSV1_0_CHALLENGE_LENGTH);
        HMAC_MD5(ResponseKeyNT, MSV1_0_NT_OWF_PASSWORD_LENGTH,
                 ccTemp, ccTempLen,
                 pLmChallengeResponse->Response);
        NtlmFree(ccTemp, FALSE);
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
    #endif // #if 1 CalcLm2UserSessionKey
}

#if 0
BOOL
CliComputeKeys(
    IN ULONG ChallengeMsg_NegFlg,
    IN PUSER_SESSION_KEY pSessionBaseKey,
    IN PEXT_DATA pLmChallengeResponseData,
    IN PEXT_DATA pNtChallengeResponseData,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    OUT PEXT_DATA pEncryptedRandomSessionKey,
    OUT PNTLMSSP_CONTEXT_MSG ctxmsg)
{
    UCHAR ExportedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
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
        TRACE("(RC4K) KeyExchangeKey\n");
        NtlmPrintHexDump(KeyExchangeKey, ARRAYSIZE(KeyExchangeKey));
        TRACE("(RC4K) ExportedSessionKey\n");
        NtlmPrintHexDump(ExportedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH);
        RC4K(KeyExchangeKey, ARRAYSIZE(KeyExchangeKey),
             ExportedSessionKey, MSV1_0_USER_SESSION_KEY_LENGTH,
             pEncryptedRandomSessionKey->Buffer);
        TRACE("RC4K Output - EncryptedRandomSessionKey\n");
        NtlmPrintHexDump(pEncryptedRandomSessionKey->Buffer, pEncryptedRandomSessionKey->bUsed);
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

    PrintSignSealKeyInfo(ctxmsg);

    return TRUE;
}

BOOL CliComputeResponseKeys(
    IN BOOL UseNTLMv2,
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN PEXT_STRING_W userdom,
    OUT UCHAR ResponseKeyLM[MSV1_0_NTLM3_OWF_LENGTH],
    OUT UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH])
{
    NTSTATUS Status;

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
        Status = NTOWFv1((PUNICODE_STRING)passwd, ResponseKeyNT);
        if (!NT_SUCCESS(Status))
        {
            ERR("NTOWFv1 failed 0x%lx\n", Status);
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
        Status = LMOWFv1((char*)passwdOEM.Buffer, ResponseKeyLM);
        if (!NT_SUCCESS(Status))
        {
            ERR("LMOWFv1 failed 0x%lx\n", Status);
            return FALSE;
        }
        ExtStrFree(&passwdOEM);
        #ifdef VALIDATE_NTLMv1
        TRACE("**** VALIDATE **** ResponseKeyLM\n");
        NtlmPrintHexDump(ResponseKeyLM, MSV1_0_NTLM3_RESPONSE_LENGTH);
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
    }
    return TRUE;
}
#endif

/* used by server and client */
BOOL
ComputeResponse(
    _In_ ULONG Context_NegFlg,
    _In_ BOOL UseNTLMv2,
    _In_ BOOL Anonymouse,
    _In_ PUNICODE_STRING UserDom,
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _In_ NTLM_NT_OWF_PASSWORD NtOwfPwd,
    _In_ PUNICODE_STRING ServerName,
    _In_ UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ ULONGLONG ChallengeTimestamp,
    _Inout_ PSTRING LmChallengeResponseData,
    _Inout_ PSTRING NtChallengeResponseData,
    _Out_ PUSER_SESSION_KEY pSessionBaseKey)
{
    TRACE("%S %p %p\n",
        ServerName->Buffer, ChallengeToClient,
        UserDom->Buffer);
    TRACE("UseNTLMv2 %i\n", UseNTLMv2);

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
        /* prepare ComputeResponse */
        NtlmAStrAlloc(NtChallengeResponseData, MSV1_0_RESPONSE_LENGTH, MSV1_0_RESPONSE_LENGTH);
        NtlmAStrAlloc(LmChallengeResponseData, MSV1_0_RESPONSE_LENGTH, MSV1_0_RESPONSE_LENGTH);

        if (!ComputeResponseNTLMv1(Context_NegFlg,
                                   Anonymouse,
                                   (PUCHAR)LmOwfPwd,
                                   (PUCHAR)NtOwfPwd,
                                   ChallengeToClient,
                                   ChallengeFromClient,
                                   (PUCHAR)LmChallengeResponseData->Buffer,
                                   (PUCHAR)NtChallengeResponseData->Buffer,
                                   pSessionBaseKey))
        {
            NtlmAStrFree(NtChallengeResponseData);
            NtlmAStrFree(LmChallengeResponseData);
            ERR("ComputeResponseNTLMv1 failed!\n");
            return FALSE;
        }
        #ifdef VALIDATE_NTLMv1
        DebugBreak();
        #endif
    }
    else
    {
        /* prepare CompureResponse */
        NtlmAStrAlloc(LmChallengeResponseData, sizeof(LM2_RESPONSE), sizeof(LM2_RESPONSE));

        if (!ComputeResponseNTLMv2(Anonymouse,
                                   UserDom,
                                   LmOwfPwd,
                                   NtOwfPwd,
                                   ServerName,
                                   ChallengeToClient,
                                   ChallengeFromClient,
                                   ChallengeTimestamp,
                                   (PLM2_RESPONSE)LmChallengeResponseData->Buffer,
                                   NtChallengeResponseData,
                                   pSessionBaseKey))
        {
            NtlmAStrFree(LmChallengeResponseData);
            ERR("ComputeResponseNVLMv2 failed!\n");
            return FALSE;
        }
        /* uses same key ... */
        //memcpy(pLmSessionKey, pUserSessionKey, sizeof(*pLmSessionKey));
    }

    TRACE("=== CliComputeKeys === \n\n");
    TRACE("SessionBaseKey\n");
    NtlmPrintHexDump((PBYTE)pSessionBaseKey, sizeof(USER_SESSION_KEY));
    TRACE("pLmChallengeResponseData\n");
    NtlmPrintHexDump((PUCHAR)LmChallengeResponseData->Buffer, LmChallengeResponseData->Length);
    TRACE("ClientChallenge\n");
    NtlmPrintHexDump(ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ServerChallenge\n");
    NtlmPrintHexDump(ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    TRACE("ResponseKeyLM\n");
    NtlmPrintHexDump((PUCHAR)LmOwfPwd, MSV1_0_NTLM3_OWF_LENGTH);

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
    _Out_ prc4_key pHandle,
    _In_ UCHAR* Key,
    _In_ ULONG KeyLen)
{
    rc4_init(pHandle, Key, KeyLen);
}

VOID
RC4(_In_ prc4_key pHandle,
    _In_ UCHAR* pDataI,
    _Out_ UCHAR* pDataO,
    _In_ ULONG len)
{
    /* use pData for in/out should be okay - i think! */
    rc4_crypt(pHandle, pDataI, pDataO, len);
}

//#define VALIDATE_CYPHER

/* Keys from
 * http://davenport.sourceforge.net/ntlm.html#theLanManagerSessionKey
 */

/* The LM User Session Key */
NTSTATUS
CalcLmUserSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _Out_ PSTRING Key)//->MoveToLib
{
    TRACE("The LM User Session Key\n");

    if (!NtlmAStrAlloc(Key, 0x08, 0x08))
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Key->Buffer, LmOwfPwd, 0x08);
    Key->Length = 8;//MSV1_0...;

    NtlmPrintHexDump((PBYTE)Key->Buffer, Key->Length);

    return STATUS_SUCCESS;
}

/* ChallengeFromClient also known as nonce */
VOID
CalcLm2UserSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD Lm2OwfPwd,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer)//->MoveToLib
{
    STRING Key;
    STRING Buffer;

    if (TRUE)  // no null session
    {
        /* The LMv2 User Session Key */
        TRACE("The LMv2 User Session Key\n");

        //Set LmChallengeResponse to ConcatenationOf(HMAC_MD5(ResponseKeyLM,
        //ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge)),
        //ClientChallenge )
        NtlmAStrAlloc(&Buffer, 2 * MSV1_0_CHALLENGE_LENGTH, 0);
        NtlmAStrAlloc(&Key, 0x10, 0x10);

        RtlAppendStringToString(&Buffer, ChallengeFromServer);
        RtlAppendStringToString(&Buffer, ChallengeFromClient);
        HMAC_MD5(Lm2OwfPwd, MSV1_0_LM_OWF_PASSWORD_LENGTH,
                 (PUCHAR)Buffer.Buffer, Buffer.Length,
                 (PUCHAR)Key.Buffer);

        NtlmPrintHexDump((PBYTE)Key.Buffer, Key.Length);


        NtlmAStrFree(&Buffer);
        NtlmAStrFree(&Key);
    }
}

/* The LM User Session Key */
NTSTATUS
CalcLanmanSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _In_ PSTRING LmResponse)//->MoveToLib
{
    STRING Key;
    STRING Buffer;

    TRACE("The Lan Manager Session Key\n");

    if (!NtlmAStrAlloc(&Key, 0x10, 0x10) ||
        !NtlmAStrAlloc(&Buffer, 0x10, 0xe))
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Buffer.Buffer, LmOwfPwd, 8);
    RtlCopyMemory(Buffer.Buffer + 8, "\xbd\xbd\xbd\xbd\xbd\xbd", 6);
    //PrintHexDumpMax(Buffer.Length, (PBYTE)Buffer.Buffer, 256);

    DES((PUCHAR)Buffer.Buffer, (PUCHAR)LmResponse->Buffer, (PUCHAR)Key.Buffer);
    DES((PUCHAR)Buffer.Buffer+7, (PUCHAR)LmResponse->Buffer, (PUCHAR)Key.Buffer+8);
    NtlmPrintHexDump((PBYTE)Key.Buffer, Key.Length);

    NtlmAStrFree(&Buffer);
    NtlmAStrFree(&Key);
    return STATUS_SUCCESS;
}

/* The NTLM User Session Key */
NTSTATUS
CalcNtUserSessionKey(
    _In_ NTLM_NT_OWF_PASSWORD NtOwfPwd,
    _Out_ PSTRING Key)//->MoveToLib
{
    TRACE("The NTLM User Session Key (maybe wrong)\n");

    if (!NtlmAStrAlloc(Key, 0x10, 0x10))
        return STATUS_NO_MEMORY;

    MD4((PUCHAR)NtOwfPwd, sizeof(NTLM_NT_OWF_PASSWORD), (PUCHAR)Key->Buffer);

    NtlmPrintHexDump((PBYTE)Key->Buffer, Key->Length);
    return STATUS_SUCCESS;
}

VOID
CalcNtLm2UserSessionKey(
    _In_ NTLM_NT_OWF_PASSWORD Nt2OwfPwd,
    _In_ PSTRING TargetInfo,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer,
    _In_ PSTRING NTLMv2Response,
    _Out_ PSTRING SessionBaseKey)//->MoveToLib
{
    STRING NtProofStr;
    BOOL Anonymouse = FALSE;
    STRING NtChallengeResponseData;

    //TODO Check size of SessionBaseKey -> Should 0x10

    // The NTLMv2 User Session Key
    TRACE("The NTLMv2 User Session Key\n");

    NtlmAStrAlloc(&NtProofStr, 0x10, 0x10);

    //Define ComputeResponse(NegFlg, ResponseKeyNT, ResponseKeyLM,
    //CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge, Time, ServerName)
    //As
    //If (User is set to "" && Passwd is set to "")
    if (Anonymouse)
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
        int tempLen, ccTempLen;

        NtChallengeResponseData = *NTLMv2Response;
        NtChallengeResponseData.Buffer += 0x10;
        NtChallengeResponseData.Length -= 0x10;
        NtChallengeResponseData.MaximumLength -= 0x10;

        //Set temp to ConcatenationOf(Responserversion, HiResponserversion,
        //Z(6), Time, ClientChallenge, Z(4), ServerName, Z(4))
        //temp = ((PBYTE)NtResponse) +
        //       FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, RespType);
        temp = (PBYTE)NtChallengeResponseData.Buffer;
        /* Spec (4.2.4.1.3 temp) shows 4 bytes more
         * (all 0) may be a bug in spec-doc. It works
         * without these extra bytes. */
        tempLen = NtChallengeResponseData.Length;// -
                  //FIELD_OFFSET(MSV1_0_NTLM3_RESPONSE, RespType) +
                  //FIXME check length (4 Byte all 0)
                  //sizeof(ULONG);//??;
        #ifdef VALIDATE_NTLMv2
        PrintHexDumpMax(NtChallengeResponseData.Length, (PBYTE)NtChallengeResponseData.Buffer, 0x100);
        TRACE("%p %p %p %i\n", NtResponse->Buffer, temp, NtChallengeResponseData.Buffer, tempLen);
        TRACE("**** VALIDATE **** temp\n");
        PrintHexDumpMax(tempLen, (PBYTE)temp, tempLen);
        #endif

        //Set NTProofStr to
        //  HMAC_MD5(ResponseKeyNT,
        //           ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge,temp))
        ccTempLen = MSV1_0_CHALLENGE_LENGTH + tempLen;
        ccTemp = (PBYTE)NtlmAllocate(ccTempLen, FALSE);
        memcpy(ccTemp, ChallengeFromServer->Buffer, MSV1_0_CHALLENGE_LENGTH);
        memcpy(ccTemp + MSV1_0_CHALLENGE_LENGTH, temp, tempLen);
        HMAC_MD5(Nt2OwfPwd, MSV1_0_NT_OWF_PASSWORD_LENGTH,
                 ccTemp, ccTempLen, (PUCHAR)NtProofStr.Buffer);
        NtlmFree(ccTemp, FALSE);

        //Set NtChallengeResponse to ConcatenationOf(NTProofStr, temp)
        //memcpy(&NtChallengeResponse[0], NTProofStr, ARRAYSIZE(NTProofStr));//16
        //memcpy(&NtResponse->Response[0], NtProofStr.Buffer, NtProofStr.Length);//16
        /* MS-NLMP says concat temp ... however
         * Response is oly 16 Byte ... temp points after it
         * and this is the same address (temp == &pNtResponse->Response[16])
         * which is already filled - so copy makes no sense */
        #ifdef VALIDATE_NTLMv2
        TRACE("**** VALIDATE **** NTProofStr\n");
        NtlmPrintHexDump((PBYTE)NtProofStr.Buffer, NtProofStr.Length);
        //TRACE("**** VALIDATE **** NtChallengeResponse\n");
        //NtlmPrintHexDump(NtResponse->Buffer, MSV1_0_NTLM3_RESPONSE_LENGTH);
        #endif

    }

    //EndIf
    //Set SessionBaseKey to HMAC_MD5(ResponseKeyNT, NTProofStr)
    //EndDefine
    HMAC_MD5(Nt2OwfPwd, MSV1_0_NT_OWF_PASSWORD_LENGTH,
             (PUCHAR)NtProofStr.Buffer, NtProofStr.Length,
             (PUCHAR)SessionBaseKey->Buffer);

    NtlmPrintHexDump((PBYTE)SessionBaseKey->Buffer, SessionBaseKey->Length);

    NtlmAStrFree(&NtProofStr);
}

VOID
CalcNtLm2SessionResponseKey(
    _In_ NTLM_NT_OWF_PASSWORD NtOwfPwd,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer,
    _Out_ PSTRING SessionResponseKey)
{

    STRING Buffer;
    STRING UserSessionKey;

    /* The NTLM2 Session Response User Session Key */
    TRACE("The NTLM2 Session Response User Session Key\n");

    //Set LmChallengeResponse to ConcatenationOf(HMAC_MD5(ResponseKeyLM,
    //ConcatenationOf(CHALLENGE_MESSAGE.ServerChallenge, ClientChallenge)),
    //ClientChallenge )
    RtlInitString(&UserSessionKey, NULL);
    NtlmAStrAlloc(&Buffer, 0x10, 0);
    if (SessionResponseKey->Length != 0x10)
    {
        ERR("ERR\n");
        return;
    }

    CalcNtUserSessionKey(NtOwfPwd, &UserSessionKey);

    RtlAppendStringToString(&Buffer, ChallengeFromServer);
    RtlAppendStringToString(&Buffer, ChallengeFromClient);
    HMAC_MD5((PUCHAR)UserSessionKey.Buffer, UserSessionKey.Length,
             (PUCHAR)Buffer.Buffer, Buffer.Length,
             (PUCHAR)SessionResponseKey->Buffer);

    NtlmPrintHexDump((PBYTE)SessionResponseKey->Buffer, SessionResponseKey->Length);

    NtlmAStrFree(&UserSessionKey);
    NtlmAStrFree(&Buffer);
}
