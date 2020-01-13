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

#ifdef __UNUSED__
PNTLMSSP_CREDENTIAL
NtlmReferenceCredential(IN ULONG_PTR Handle);

VOID
NtlmDereferenceCredential(IN ULONG_PTR Handle);
#endif

#endif
