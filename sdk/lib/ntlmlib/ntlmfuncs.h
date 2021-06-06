/*
 * PROJECT:     ntlmlib
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     public functions for ntlmlib
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 *
 */

#ifndef _NTLMFUNCS_H_
#define _NTLMFUNCS_H_

/* calculations.h */

NTSTATUS
NTOWFv1(
    _In_ PUNICODE_STRING Password,
    _Out_ NTLM_NT_OWF_PASSWORD Result);

NTSTATUS
LMOWFv1(
    _In_ LPCSTR Password,
    _Out_ BYTE Result[16]);

NTSTATUS
NTOWFv2ofw(
    _In_ NTLM_NT_OWF_PASSWORD md4pwd,
    _In_ PUNICODE_STRING User,
    _In_ PUNICODE_STRING Domain,
    _Out_ NTLM_NT_OWF_PASSWORD Result);

NTSTATUS
NTOWFv2(
    _In_ LPCWSTR Password,
    _In_ LPCWSTR User,
    _In_ LPCWSTR Domain,
    _Out_ NTLM_NT_OWF_PASSWORD Result);

NTSTATUS
LMOWFv2(
    _In_ LPCWSTR Password,
    _In_ LPCWSTR User,
    _In_ LPCWSTR Domain,
    _Out_ NTLM_LM_OWF_PASSWORD Result);

NTSTATUS
CalcLmUserSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _Out_ PSTRING Key);

VOID
RC4Init(
    _Out_ prc4_key pHandle,
    _In_ UCHAR* Key,
    _In_ ULONG KeyLen);
VOID
RC4(_In_ prc4_key pHandle,
    _In_ UCHAR* pDataI,
    _Out_ UCHAR* pDataO,
    _In_ ULONG len);

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
    _Out_ PULONG pSignLen);

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
    _Out_ PUSER_SESSION_KEY pSessionBaseKey);

/*BOOL
ComputeResponseNTLMv1(
    _In_ ULONG NegFlg,
    _In_ BOOL Anonymouse,
    _In_ UCHAR ResponseKeyLM[MSV1_0_LM_OWF_PASSWORD_LENGTH],
    _In_ UCHAR ResponseKeyNT[MSV1_0_NT_OWF_PASSWORD_LENGTH],
    _In_ UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    _Out_ UCHAR LmChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    _Out_ UCHAR NtChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    _Out_ PUSER_SESSION_KEY SessionBaseKey);

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
    _Out_ PUSER_SESSION_KEY SessionBaseKey);
*/
/* ciphers */
void
RC4K (const unsigned char * k, unsigned long key_len, const unsigned char * d, int len, unsigned char * result);

void rc4_init(rc4_key *const state, const unsigned char *key, int keylen);
void rc4_crypt(rc4_key *const state, const unsigned char *inbuf, unsigned char *outbuf, int buflen);

/* util */
PVOID
NtlmAllocate(
    _In_ SIZE_T Size,
    _In_ BOOL UsePrivateLsaHeap);

VOID
NtlmFree(
    _In_ PVOID Buffer,
    _In_ BOOL FromPrivateLsaHeap);

BOOL
NtlmAStrAlloc(
    _Out_ PSTRING Dst,
    _In_ USHORT SizeInBytes,
    _In_ USHORT InitLength);

BOOL
NtlmUStrAlloc(
    _Out_ PUNICODE_STRING Dst,
    _In_ USHORT SizeInBytes,
    _In_ USHORT InitLength);

VOID
NtlmAStrFree(
    _In_ PSTRING String);

VOID
NtlmUStrFree(
    _In_ PUNICODE_STRING String);

VOID
NtlmUStrUpper(
    _Inout_ PUNICODE_STRING Str);

#endif /* _NTLMFUNCS_H_ */
