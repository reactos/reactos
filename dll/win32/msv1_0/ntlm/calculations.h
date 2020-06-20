#ifndef _MSV1_0_CALCULATI0NS_H_
#define _MSV1_0_CALCULATI0NS_H_

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
//#define LMOWFv2ofw NTOWFv2ofw

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

/*VOID
NtpLmSessionKeys(
    _In_ PUSER_SESSION_KEY NtpUserSessionKey,
    _In_ UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    _Out_ PUSER_SESSION_KEY pUserSessionKey,
    _Out_ PLM_SESSION_KEY pLmSessionKey);*/

/* all these should go to an extra lib */
NTSTATUS
CalcLmUserSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _Out_ PSTRING Key);

VOID
CalcLm2UserSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD Lm2OwfPwd,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer);

NTSTATUS
CalcLanmanSessionKey(
    _In_ NTLM_LM_OWF_PASSWORD LmOwfPwd,
    _In_ PSTRING LmResponse);

NTSTATUS
CalcNtUserSessionKey(
    _In_ NTLM_NT_OWF_PASSWORD NtOwfPwd,
    _Out_ PSTRING Key);

VOID
CalcNtLm2UserSessionKey(
    _In_ NTLM_NT_OWF_PASSWORD Nt2OwfPwd,
    _In_ PSTRING TargetInfo,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer,
    _In_ PSTRING NTLMv2Response,
    _Out_ PSTRING SessionBaseKey);

#endif /* _MSV1_0_CALCULATI0NS_H_ */
