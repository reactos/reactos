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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#define SECURITY_WIN32
#define _NO_KSECDD_IMPORT_
#include "rpc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#define NTLM_NAME_A "NTLM\0"
#define NTLM_NAME_W L"NTLM\0"

#define NTLM_COMMENT_A "NTLM Security Package\0"
#define NTLM_COMMENT_W L"NTLM Security Package\0"

/* According to Windows, NTLM has the following capabilities.  */
#define NTLM_CAPS ( \
        SECPKG_FLAG_INTEGRITY | \
        SECPKG_FLAG_PRIVACY | \
        SECPKG_FLAG_TOKEN_ONLY | \
        SECPKG_FLAG_CONNECTION | \
        SECPKG_FLAG_MULTI_REQUIRED | \
        SECPKG_FLAG_IMPERSONATION | \
        SECPKG_FLAG_ACCEPT_WIN32_NAME | \
        SECPKG_FLAG_READONLY_WITH_CHECKSUM)

#define NTLM_MAX_BUF 1904 /* wtf? */

/* NTLMSSP flags indicating the negotiated features */
#define NTLMSSP_NEGOTIATE_UNICODE                   0x00000001
#define NTLMSSP_NEGOTIATE_OEM                       0x00000002
#define NTLMSSP_REQUEST_TARGET                      0x00000004
#define NTLMSSP_NEGOTIATE_SIGN                      0x00000010
#define NTLMSSP_NEGOTIATE_SEAL                      0x00000020
#define NTLMSSP_NEGOTIATE_DATAGRAM_STYLE            0x00000040
#define NTLMSSP_NEGOTIATE_LM_SESSION_KEY            0x00000080
#define NTLMSSP_NEGOTIATE_NTLM                      0x00000200
#define NTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED           0x00001000
#define NTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED      0x00002000
#define NTLMSSP_NEGOTIATE_LOCAL_CALL                0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN               0x00008000
#define NTLMSSP_NEGOTIATE_TARGET_TYPE_DOMAIN        0x00010000
#define NTLMSSP_NEGOTIATE_TARGET_TYPE_SERVER        0x00020000
#define NTLMSSP_NEGOTIATE_NTLM2                     0x00080000
#define NTLMSSP_NEGOTIATE_TARGET_INFO               0x00800000
#define NTLMSSP_NEGOTIATE_128                       0x20000000
#define NTLMSSP_NEGOTIATE_KEY_EXCHANGE              0x40000000
#define NTLMSSP_NEGOTIATE_56                        0x80000000

typedef struct tag_arc4_info {
    unsigned char x, y;
    unsigned char state[256];
} arc4_info;

typedef enum _helper_mode /* remove? */
{
    NTLM_SERVER,
    NTLM_CLIENT,
    NUM_HELPER_MODES
} HelperMode;

typedef struct _NtlmCredentials /* remove? */
{
    HelperMode mode;
    char *username_arg;
    char *domain_arg;
    char *password;
    int pwlen;
} NtlmCredentials, *PNtlmCredentials;

typedef struct _NegoHelper { /* remove? */
    HelperMode mode;
    int pipe_in;
    int pipe_out;
    int major;
    int minor;
    int micro;
    char *com_buf;
    int com_buf_size;
    int com_buf_offset;
    BYTE *session_key;
    ULONG neg_flags;
    struct {
        struct {
            ULONG seq_num;
            arc4_info *a4i;
        } ntlm;
        struct {
            BYTE *send_sign_key;
            BYTE *send_seal_key;
            BYTE *recv_sign_key;
            BYTE *recv_seal_key;
            ULONG send_seq_no;
            ULONG recv_seq_no;
            arc4_info *send_a4i;
            arc4_info *recv_a4i;
        } ntlm2;
    } crypt;
} NegoHelper, *PNegoHelper;

typedef enum _sign_direction { /* remove? */
    NTLM_SEND,
    NTLM_RECV
} SignDirection;

/* functions */ 

SECURITY_STATUS
SEC_ENTRY
ntlm_QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer);

SECURITY_STATUS
SEC_ENTRY
ntlm_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
ntlm_FreeCredentialsHandle(
        PCredHandle phCredential);

SECURITY_STATUS
SEC_ENTRY
ntlm_InitializeSecurityContextA(
        PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
        ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
        PSecBufferDesc pInput,ULONG Reserved2, PCtxtHandle phNewContext, 
        PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
ntlm_AcceptSecurityContext(
        PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
        ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, 
        PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
ntlm_CompleteAuthToken(PCtxtHandle phContext,
        PSecBufferDesc pToken);

SECURITY_STATUS
SEC_ENTRY
ntlm_DeleteSecurityContext(
        PCtxtHandle phContext);

SECURITY_STATUS
SEC_ENTRY
ntlm_QueryContextAttributesA(
        PCtxtHandle phContext,
        ULONG ulAttribute, void *pBuffer);

SECURITY_STATUS
SEC_ENTRY
ntlm_ImpersonateSecurityContext(
        PCtxtHandle phContext);

SECURITY_STATUS
SEC_ENTRY
ntlm_RevertSecurityContext(
        PCtxtHandle phContext);

SECURITY_STATUS
SEC_ENTRY
ntlm_MakeSignature(
        PCtxtHandle phContext, ULONG fQOP,
        PSecBufferDesc pMessage, ULONG MessageSeqNo);

SECURITY_STATUS
SEC_ENTRY
ntlm_VerifySignature(
        PCtxtHandle phContext,
        PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP);

SECURITY_STATUS
SEC_ENTRY
ntlm_EncryptMessage(
        PCtxtHandle phContext,
        ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo);

SECURITY_STATUS
SEC_ENTRY
ntlm_DecryptMessage(
        PCtxtHandle phContext,
        PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP);

SECURITY_STATUS
SEC_ENTRY
ntlm_QueryCredentialsAttributesW(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer);

SECURITY_STATUS
SEC_ENTRY
ntlm_QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer);

SECURITY_STATUS
SEC_ENTRY
ntlm_AcquireCredentialsHandleW(
        SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
        PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
        PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
ntlm_InitializeSecurityContextW(
        PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName, 
        ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
        PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, 
        PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry);

SECURITY_STATUS
SEC_ENTRY
ntlm_QueryContextAttributesW(
        PCtxtHandle phContext,
        ULONG ulAttribute, void *pBuffer);


#endif
