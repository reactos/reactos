/*
 * PROJECT:     ntlmlib
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     internal definitions for ntlmlib
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 *
 */

#ifndef _NTLMLIB_INTERNAL_H_
#define _NTLMLIB_INTERNAL_H_

#include <ntsecapi.h>

/* advapi */
NTSTATUS WINAPI SystemFunction007(PUNICODE_STRING string, LPBYTE hash);
NTSTATUS WINAPI SystemFunction006(LPCSTR password, LPBYTE hash);
NTSTATUS WINAPI SystemFunction009(BYTE* challenge, BYTE* hash, LPBYTE response);

#define MSV1_0_CHALLENGE_LENGTH 8
#define NTLM_KEYEXCHANGE_KEY_LENGTH 16

/*typedef UCHAR NTLM_LM_OWF_PASSWORD[MSV1_0_LM_OWF_PASSWORD_LENGTH];
typedef NTLM_LM_OWF_PASSWORD *PNTLM_LM_OWF_PASSWORD;
typedef UCHAR NTLM_NT_OWF_PASSWORD[MSV1_0_NT_OWF_PASSWORD_LENGTH];
typedef NTLM_NT_OWF_PASSWORD *PNTLM_NT_OWF_PASSWORD;*/

typedef UCHAR USER_SESSION_KEY[MSV1_0_USER_SESSION_KEY_LENGTH];
//typedef USER_SESSION_KEY *PUSER_SESSION_KEY;
typedef UCHAR LANMAN_SESSION_KEY[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
typedef LANMAN_SESSION_KEY *PLANMAN_SESSION_KEY;

#define NtlmPrintHexDump(b, l) PrintHexDump(l, b)

/* flags */
#define NTLMSSP_NEGOTIATE_UNICODE                     0x00000001
#define NTLMSSP_NEGOTIATE_OEM                         0x00000002
#define NTLMSSP_REQUEST_TARGET                        0x00000004
#define NTLMSSP_RESERVED_9                            0x00000008
#define NTLMSSP_NEGOTIATE_SIGN                        0x00000010
#define NTLMSSP_NEGOTIATE_SEAL                        0x00000020
#define NTLMSSP_NEGOTIATE_DATAGRAM                    0x00000040
#define NTLMSSP_NEGOTIATE_LM_KEY                      0x00000080
#define NTLMSSP_NEGOTIATE_NETWARE                     0x00000100 //forget about it
#define NTLMSSP_NEGOTIATE_NTLM                        0x00000200
#define NTLMSSP_NEGOTIATE_NT_ONLY                     0x00000400
#define NTLMSSP_NEGOTIATE_NULL_SESSION                0x00000800
#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED         0x00001000
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED    0x00002000
#define NTLMSSP_NEGOTIATE_LOCAL_CALL                  0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN                 0x00008000
#define NTLMSSP_TARGET_TYPE_DOMAIN                    0x00010000
#define NTLMSSP_TARGET_TYPE_SERVER                    0x00020000
#define NTLMSSP_TARGET_TYPE_SHARE                     0x00040000
#define NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY    0x00080000
#define NTLMSSP_REQUEST_INIT_RESP                     0x00100000
#define NTLMSSP_REQUEST_ACCEPT_RESP                   0x00200000 //get session key and luid
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY            0x00400000
#define NTLMSSP_NEGOTIATE_TARGET_INFO                 0x00800000
#define NTLMSSP_RESERVED_4                            0x01000000
#define NTLMSSP_NEGOTIATE_VERSION                     0x02000000
#define NTLMSSP_RESERVED_3                            0x04000000
#define NTLMSSP_RESERVED_2                            0x08000000
#define NTLMSSP_RESERVED_1                            0x10000000
#define NTLMSSP_NEGOTIATE_128                         0x20000000
#define NTLMSSP_NEGOTIATE_KEY_EXCH                    0x40000000
#define NTLMSSP_NEGOTIATE_56                          0x80000000

#endif /*  _NTLMLIB_INTERNAL_H_ */
