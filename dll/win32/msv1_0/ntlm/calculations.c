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

#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

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
