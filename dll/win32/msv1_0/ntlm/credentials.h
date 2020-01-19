/*
 * PROJECT:     ReactOS ntlm implementation (msv1_0)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ntlm credentials (header)
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */
#ifndef _CREDENTIALS_H_
#define _CREDENTIALS_H_

NTSTATUS
NtlmCredentialInitialize(VOID);

VOID
NtlmCredentialTerminate(VOID);

SECURITY_STATUS
SEC_ENTRY
NtlmAcquireCredentialsHandle(
    IN OPTIONAL SEC_WCHAR *pszPrincipal,
    IN OPTIONAL SEC_WCHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pLogonID,
    IN PVOID pAuthData,
    IN SEC_GET_KEY_FN pGetKeyFn,
    IN PVOID pGetKeyArgument,
    OUT PLSA_SEC_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry);

PNTLMSSP_CREDENTIAL
NtlmReferenceCredential(IN ULONG_PTR Handle);

VOID
NtlmDereferenceCredential(IN ULONG_PTR Handle);

#endif
