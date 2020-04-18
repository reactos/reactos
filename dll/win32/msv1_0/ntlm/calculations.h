#ifndef _MSV1_0_CALCULATI0NS_H_
#define _MSV1_0_CALCULATI0NS_H_

NTSTATUS
NTOWFv1(
    _In_ PUNICODE_STRING Password,
    _Out_ PVOID Result);

NTSTATUS
LMOWFv1(
    _In_ LPCSTR Password,
    _Out_ BYTE Result[16]);

BOOL
NTOWFv2(
    IN LPCWSTR password,
    IN LPCWSTR user,
    IN LPCWSTR domain,
    OUT UCHAR result[16]);

BOOL
LMOWFv2(LPCWSTR password,
        LPCWSTR user,
        LPCWSTR domain,
        PUCHAR result);

/* used by server and client */
BOOL
ComputeResponse(
    _In_ ULONG Context_NegFlg,
    _In_ BOOL UseNTLMv2,
    _In_ BOOL Anonymouse,
    _In_ PEXT_STRING_W userdom,
    _In_ PNTLM_LM_OWF_PASSWORD LmOwfPwd,
    _In_ PNTLM_NT_OWF_PASSWORD NtOwfPwd,
    _In_ PEXT_STRING_W pServerName,
    _In_ UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ ULONGLONG ChallengeTimestamp,
    _Inout_ PEXT_DATA pLmChallengeResponseData,
    _Inout_ PEXT_DATA pNtChallengeResponseData,
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
    _In_ PNTLM_LM_OWF_PASSWORD LmOwfPwd,
    _Out_ PSTRING Key);

VOID
CalcLm2UserSessionKey(
    _In_ PSTRING Lm2ResponseKey,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING ChallengeFromServer);

NTSTATUS
CalcLanmanSessionKey(
    _In_ PNTLM_LM_OWF_PASSWORD LmOwfPwd,
    _In_ PSTRING LmResponse);

NTSTATUS
CalcNtUserSessionKey(
    _In_ PNTLM_NT_OWF_PASSWORD NtOwfPwd,
    _Out_ PSTRING Key);

VOID
CalcNtLm2UserSessionKey(
    _In_ PSTRING Nt2ResponseKey,
    _In_ PSTRING ChallengeFromClient,
    _In_ PSTRING NTLMv2Response);

#endif /* _MSV1_0_CALCULATI0NS_H_ */
