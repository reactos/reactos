/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ntlmcomn.h

Abstract:

    Header file describing the interface to code common to the
    NT Lanman Security Support Provider (NtLmSsp) Service and the DLL.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:
    ChandanS  03-Aug-1996 Stolen from net\svcdlls\ntlmssp\ntlmcomn.h

--*/

#ifndef _NTLMCOMN_INCLUDED_
#define _NTLMCOMN_INCLUDED_

////////////////////////////////////////////////////////////////////////////
//
// Common include files needed by ALL NtLmSsp files
//
////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <ntmsv1_0.h>   // MSV 1.0 Authentication Package

#include <security.h>   // General definition of a Security Support Provider
#include <spseal.h>     // Prototypes for Seal & Unseal

#include <ntlmssp.h>    // External definition of the NtLmSsp service
#include <lmcons.h>
#include <debug.h>      // NtLmSsp debugging


////////////////////////////////////////////////////////////////////////
//
// Global Definitions
//
////////////////////////////////////////////////////////////////////////

#define NTLMSSP_KEY_SALT    0xbd


//
// Procedure forwards from utility.cxx
//

NTSTATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    );

BOOLEAN
SspTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    );

NTSTATUS
SspDuplicateToken(
    IN HANDLE OriginalToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicatedToken
    );

LPWSTR
SspAllocWStrFromWStr(
    IN LPWSTR Unicode
    );

VOID
SspHidePassword(
    IN OUT PUNICODE_STRING Password
    );

VOID
SspRevealPassword(
    IN OUT PUNICODE_STRING HiddenPassword
    );

BOOLEAN SspGetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * Token,
    IN BOOLEAN ReadonlyOK
    );

//
// Procedure forwards from credhand.cxx
//

NTSTATUS
SsprAcquireCredentialHandle(
    IN PHANDLE TokenHandle,
    IN PLUID LogonId,
    IN ULONG ClientProcessId,
    IN ULONG CredentialUseFlags,
    OUT PULONG_PTR CredentialHandle,
    OUT PTimeStamp Lifetime,
    IN OPTIONAL PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING UserName,
    IN OPTIONAL PUNICODE_STRING Password
    );

//
// Procedure forwards from context.cxx
//

NTSTATUS
SsprHandleFirstCall(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    );


NTSTATUS
SsprHandleNegotiateMessage(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime
    );

NTSTATUS
SsprHandleChallengeMessage(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN HANDLE ClientTokenHandle, OPTIONAL
    IN PLUID LogonId, OPTIONAL
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    IN OUT PULONG SecondOutputTokenSize,
    OUT PVOID *SecondOutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags
    );

NTSTATUS
SsprHandleAuthenticateMessage(
    IN ULONG_PTR CredentialHandle,
    IN OUT PULONG_PTR ContextHandle,
    IN ULONG ContextReqFlags,
    IN ULONG InputTokenSize,
    IN PVOID InputToken,
    IN ULONG SecondInputTokenSize,
    IN PVOID SecondInputToken,
    IN OUT PULONG OutputTokenSize,
    OUT PVOID *OutputToken,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PUCHAR SessionKey,
    OUT PULONG NegotiateFlags,
    OUT PHANDLE TokenHandle,
    OUT PNTSTATUS SubStatus,
    OUT PTimeStamp PasswordExpiry,
    OUT PULONG UserFlags
    );

NTSTATUS
SsprDeleteSecurityContext (
    ULONG_PTR ContextHandle
    );

NTSTATUS
SspGetToken (
    OUT PHANDLE ReturnedTokenHandle
    );
//
// Procedure forwards from encrypt.cxx
//

BOOLEAN
IsEncryptionPermitted(VOID);

//
// Procedure forwards from userapi.cxx
//

NTSTATUS
SspMapContext(
    IN PULONG_PTR phContext,
    IN PUCHAR pSessionKey,
    IN ULONG NegotiateFlags,
    IN HANDLE TokenHandle,
    IN PTimeStamp PasswordExpiry OPTIONAL,
    IN ULONG UserFlags,
    OUT PSecBuffer ContextData
    );

#endif // ifndef _NTLMCOMN_INCLUDED_
