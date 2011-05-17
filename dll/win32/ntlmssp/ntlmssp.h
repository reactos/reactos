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
#ifndef _NTLMSSP_H
#define _NTLMSSP_H

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <wincred.h>

#include <ndk/ntndk.h>
#define SECURITY_WIN32
#define _NO_KSECDD_IMPORT_
#include <rpc.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <lmcons.h>
#include "wine/unicode.h"

/* globals */
extern SECPKG_FUNCTION_TABLE NtLmPkgFuncTable; //functions we provide to LSA in SpLsaModeInitialize
extern PSECPKG_DLL_FUNCTIONS NtlmPkgDllFuncTable; //fuctions provided by LSA in SpInstanceInit
extern SECPKG_USER_FUNCTION_TABLE NtlmUmodeFuncTable; //fuctions we provide via SpUserModeInitialize
extern PLSA_SECPKG_FUNCTION_TABLE NtlmLsaFuncTable; // functions provided by LSA in SpInitialize

extern UNICODE_STRING NtlmComputerNameString;
extern UNICODE_STRING NtlmDomainNameString;
extern OEM_STRING NtlmOemComputerNameString;
extern OEM_STRING NtlmOemDomainNameString;
extern HANDLE NtlmSystemSecurityToken;

typedef enum _NTLM_MODE {
    NtlmLsaMode = 1,
    NtlmUserMode
} NTLM_MODE, *PNTLM_MODE;

extern NTLM_MODE NtlmMode;

#define inLsaMode (NtlmMode == NtlmLsaMode)
#define inUserMode (NtlmMode == NtlmUserMode)

#define NTLM_NAME_A "NTLM\0"
#define NTLM_NAME_W L"NTLM\0"

#define NTLM_COMMENT_A "NTLM Security Package\0"
#define NTLM_COMMENT_W L"NTLM Security Package\0"

/* NTLM has the following capabilities. */
#define NTLM_CAPS ( \
        SECPKG_FLAG_ACCEPT_WIN32_NAME | \
        SECPKG_FLAG_CONNECTION | \
        SECPKG_FLAG_IMPERSONATION | \
        SECPKG_FLAG_INTEGRITY | \
        SECPKG_FLAG_LOGON | \
        SECPKG_FLAG_MULTI_REQUIRED | \
        SECPKG_FLAG_NEGOTIABLE | \
        SECPKG_FLAG_PRIVACY | \
        SECPKG_FLAG_TOKEN_ONLY)

#define NTLM_DEFAULT_TIMEOUT (5*60*1000) //context valid for 5 mins
#define NTLM_MAX_BUF 1904
#define NTLM_CRED_NULLSESSION SECPKG_CRED_RESERVED

typedef struct _NTLMSSP_CREDENTIAL
{
    LIST_ENTRY Entry;
    ULONG RefCount;
    ULONG UseFlags;
    UNICODE_STRING DomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    ULONG ProcId;
    HANDLE SecToken;
    LUID LogonId;
} NTLMSSP_CREDENTIAL, *PNTLMSSP_CREDENTIAL;

typedef enum {
    Idle,
    NegotiateSent,
    ChallengeSent,
    AuthenticateSent,
    Authenticated,
    PassedToService
} NTLMSSP_CONTEXT_STATE, *PNTLMSSP_CONTEXT_STATE;

typedef struct _NTLMSSP_CONTEXT
{
    LIST_ENTRY Entry;
    LARGE_INTEGER StartTime;//context creation time
    BOOL isServer;
    BOOL isLocal;
    ULONG Timeout;//how long context is valid pre-authentication
    ULONG RefCount;
    ULONG NegotiateFlags;
    ULONG ContextFlags;
    NTLMSSP_CONTEXT_STATE State;
    PNTLMSSP_CREDENTIAL Credential; //creator
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH]; //ChallengeSent
    UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH]; //LSA
    HANDLE ClientToken;
    ULONG ProcId;
} NTLMSSP_CONTEXT, *PNTLMSSP_CONTEXT;

/* private functions */

/* ntlmssp.c */
NTSTATUS
NtlmInitializeGlobals(VOID);

VOID
NtlmTerminateGlobals(VOID);

/* credentials.c */
NTSTATUS
NtlmCredentialInitialize(VOID);

VOID
NtlmCredentialTerminate(VOID);

PNTLMSSP_CREDENTIAL
NtlmReferenceCredential(IN ULONG_PTR Handle);

VOID
NtlmDereferenceCredential(IN ULONG_PTR Handle);

/* context.c */

NTSTATUS
NtlmContextInitialize(VOID);

VOID
NtlmContextTerminate(VOID);

PNTLMSSP_CONTEXT
NtlmAllocateContext(VOID);

/* crypt.c */
BOOL
NtlmInitializeRNG(VOID);

VOID
NtlmTerminateRNG(VOID);

NTSTATUS
NtlmGenerateRandomBits(
    VOID *Bits,
    ULONG Size);

BOOL
NtlmInitializeProtectedMemory(VOID);

VOID
NtlmTerminateProtectedMemory(VOID);

BOOL
NtlmProtectMemory(
    VOID *Data,
    ULONG Size);

BOOL
NtlmUnProtectMemory(
    VOID *Data,
    ULONG Size);

/* util.c */

PVOID
NtlmAllocate(IN ULONG Size);

VOID
NtlmFree(IN PVOID Buffer);

BOOLEAN
NtlmHasIntervalElapsed(
    IN LARGE_INTEGER Start,
    IN LONG Timeout);

BOOLEAN
NtlmGetSecBuffer(
    IN OPTIONAL PSecBufferDesc pInputDesc,
    IN ULONG BufferIndex,
    OUT PSecBuffer *pOutBuffer,
    IN BOOLEAN Output);

/* debug.c */

void
NtlmPrintNegotiateFlags(ULONG Flags);

#endif
