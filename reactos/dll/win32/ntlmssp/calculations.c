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

VOID
NTOWFv1(LPCWSTR password,
        PUCHAR result)
{
    ULONG i, len = wcslen(password);
    WCHAR pass[14];
    memcpy(pass, password, len * sizeof(WCHAR));
    for(i = len; i<14; i++)
    {
        pass[i] = L'0';
    }
    MD4((PUCHAR)pass, 14, result);
}

BOOLEAN
NTOWFv2(LPCWSTR password,
        LPCWSTR user,
        LPCWSTR domain,
        PUCHAR result)
{
    UCHAR response_key_nt_v1 [16];
    ULONG len_user = user ? wcslen(user) : 0;
    ULONG len_domain = domain ? wcslen(domain) : 0;
    ULONG len_user_u = len_user * sizeof(WCHAR);
    ULONG len_domain_u = len_domain * sizeof(WCHAR);
    WCHAR user_upper[len_user + 1];
    WCHAR buff[len_user + len_domain];
    ULONG i;

    /* Uppercase user */
    for (i = 0; i < len_user; i++) {
        user_upper[i] = toupper(user[i]);
    }
    user_upper[len_user] = 0;
    len_user_u = swprintf(buff, user_upper, len_user_u) * sizeof(WCHAR);
    len_domain_u = swprintf(buff+len_user_u, domain ? domain : L"", len_domain_u) * sizeof(WCHAR);

    NTOWFv1(password, response_key_nt_v1);
    HMAC_MD5(response_key_nt_v1, 16, (PUCHAR)buff, len_user_u + len_domain_u, result);

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
    UCHAR md5_input[16 + len];

    memcpy(md5_input, RandomSessionKey, 16);
    memcpy(md5_input + 16, magic, len);
    MD5(md5_input, len + 16, Result);

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
        UCHAR tmp[4 + buf_len];

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

        res_ptr = (ULONG *)tmp;
        res_ptr[0] = sequence;
        memcpy(tmp+4, buf, buf_len);

        HMAC_MD5(sign_key, sign_key_len, tmp, 4 + buf_len, hmac);

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

VOID
NtlmLmResponse(IN PUNICODE_STRING pUserName,
               IN PUNICODE_STRING pPassword,
               IN PUNICODE_STRING pDomainName,
               IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
               IN PLM2_RESPONSE pLm2Response,
               OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH])
{
    HMAC_MD5_CTX ctx;
    UCHAR NtlmOwf[MSV1_0_NTLM3_OWF_LENGTH];

    NTOWFv2(pPassword->Buffer,
            pUserName->Buffer,
            pDomainName->Buffer,
            NtlmOwf);

    HMACMD5Init(&ctx, NtlmOwf, MSV1_0_NTLM3_OWF_LENGTH);
    HMACMD5Update(&ctx, ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Update(&ctx, (PUCHAR)pLm2Response->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    HMACMD5Final(&ctx, Response);

    return;
}

VOID
NtlmNtResponse(IN PUNICODE_STRING pUserName,
               IN PUNICODE_STRING pPassword,
               IN PUNICODE_STRING pDomainName,
               IN ULONG ServerNameLength,
               IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
               IN PMSV1_0_NTLM3_RESPONSE pNtResponse,
               OUT UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH],
               OUT PUSER_SESSION_KEY pUserSessionKey,
               OUT PLM_SESSION_KEY pLmSessionKey)
{
    HMAC_MD5_CTX ctx;
    UCHAR NtlmOwf[MSV1_0_NTLM3_OWF_LENGTH];

    NTOWFv2(pPassword->Buffer,
            pUserName->Buffer,
            pDomainName->Buffer,
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
NtlmChallengeResponse(IN PUNICODE_STRING pUserName,
                      IN PUNICODE_STRING pPassword,
                      IN PUNICODE_STRING pDomainName,
                      IN PUNICODE_STRING pServerName,
                      IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
                      OUT PMSV1_0_NTLM3_RESPONSE pNtResponse,
                      OUT PLM2_RESPONSE pLm2Response,
                      OUT PUSER_SESSION_KEY pUserSessionKey,
                      OUT PLM_SESSION_KEY pLmSessionKey)
{
    pNtResponse->RespType = 1;
    pNtResponse->HiRespType = 1;
    pNtResponse->Flags = 0;
    pNtResponse->MsgWord = 0;

    TRACE("%wZ %wZ %wZ %wZ %p %p %p %p %p\n",
        pUserName, pPassword, pDomainName, pServerName, ChallengeToClient,
        pNtResponse, pLm2Response, pUserSessionKey, pLmSessionKey);

    NtQuerySystemTime((PLARGE_INTEGER)&pNtResponse->TimeStamp);
    NtlmGenerateRandomBits(pNtResponse->ChallengeFromClient, MSV1_0_CHALLENGE_LENGTH);
    memcpy(pNtResponse->Buffer, pServerName->Buffer, pServerName->Length);

    NtlmNtResponse(pUserName,
                   pPassword,
                   pDomainName,
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

    NtlmLmResponse(pUserName,
                   pPassword,
                   pDomainName,
                   ChallengeToClient,
                   pLm2Response,
                   pLm2Response->Response);
}

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
