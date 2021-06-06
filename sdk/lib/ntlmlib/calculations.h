/*
 * PROJECT:     ntlmlib
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     provides basic functions for ntlm implementation (msv1_0) (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 *
 */

#ifndef _NTLMLIB_CALCULATI0NS_H_
#define _NTLMLIB_CALCULATI0NS_H_

/*VOID
NtpLmSessionKeys(
    _In_ PUSER_SESSION_KEY NtpUserSessionKey,
    _In_ UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    _In_ UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH],
    _Out_ PUSER_SESSION_KEY pUserSessionKey,
    _Out_ PLM_SESSION_KEY pLmSessionKey);*/

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

#endif /* _NTLMLIB_CALCULATI0NS_H_ */
