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

/* see "NT LAN Manager (NTLM) Authentication Protocol Specification"
 * [MS-NLMP] — v20110504 for more details */

/* signature */
#define NTLMSSP_SIGNATURE "NTLMSSP\0"

/* message types */
#define NtlmNegotiate 0x00000001
#define NtlmChallenge 0x00000002
#define NtlmAuthenticate 0x00000003

/* flags */
#define NTLMSSP_NEGOTIATE_UNICODE                     0x00000001
#define NTLMSSP_NEGOTIATE_OEM                         0x00000002
#define NTLMSSP_REQUEST_TARGET                        0x00000004
#define NTLMSSP_RESERVED_9                            0x00000008
#define NTLMSSP_NEGOTIATE_SIGN                        0x00000010
#define NTLMSSP_NEGOTIATE_SEAL                        0x00000020
#define NTLMSSP_NEGOTIATE_DATAGRAM                    0x00000040
#define NTLMSSP_NEGOTIATE_LM_KEY                      0x00000080
#define NTLMSSP_RESERVED_8                            0x00000100
#define NTLMSSP_NEGOTIATE_NTLM                        0x00000200
#define NTLMSSP_NEGOTIATE_NT_ONLY                     0x00000400
#define NTLMSSP_RESERVED_7                            0x00000800
#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED         0x00001000
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED    0x00002000
#define NTLMSSP_RESERVED_6                            0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN                 0x00008000
#define NTLMSSP_TARGET_TYPE_DOMAIN                    0x00010000
#define NTLMSSP_TARGET_TYPE_SERVER                    0x00020000
#define NTLMSSP_TARGET_TYPE_SHARE                     0x00040000
#define NTLMSSP_NEGOTIATE_NTLM2                       0x00080000
#define NTLMSSP_NEGOTIATE_IDENTIFY                    0x00100000
#define NTLMSSP_RESERVED_5                            0x00200000
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

#define NTLMSSP_REVISION_W2K3 0x0F

//only filled if NTLMSSP_NEGOTIATE_VERSION is present
//ignored on retail builds
typedef struct _NTLM_WINDOWS_VERSION
{
    BYTE ProductMajor;
    BYTE ProductMinor;
    USHORT ProductBuild;
    BYTE Reserved[3];
    BYTE NtlmRevisionCurrent;
}NTLM_WINDOWS_VERSION, *PNTLM_WINDOWS_VERSION;

/*
 * Offset contains the offset from the beginning of the message to the
 * actual value in the payload area. In the event of no data being sent
 * Length and MaxLength should generaly be set to zero and ignored.
 */
//NTLM_UNICODE_STRING_OVER_THE_WIRE
typedef struct _NTLM_BLOB
{
    USHORT Length;
    USHORT MaxLength;
    ULONG Offset;
}NTLM_BLOB, *PNTLM_BLOB;

typedef struct _NEGOTIATE_MESSAGE
{
    CHAR Signature[8];
    ULONG MsgType;
    ULONG NegotiateFlags;
    NTLM_BLOB OemDomainName;
    NTLM_BLOB OemWorkstationName;
    NTLM_WINDOWS_VERSION Version;
    /* payload (DomainName, WorkstationName)*/
}NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;

typedef struct _CHALLENGE_MESSAGE
{
    CHAR Signature[8];
    ULONG MsgType;
    NTLM_BLOB TargetName;
    ULONG NegotiateFlags;
    UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH];
    UCHAR Reserved[8];
    NTLM_BLOB TargetInfo; //only if NTLMSSP_REQUEST_TARGET, contains AV_PAIRs
    NTLM_WINDOWS_VERSION Version;
    /* payload (TargetName, TargetInfo)*/
}CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;


typedef struct _AUTHENTICATE_MESSAGE
{
    CHAR Signature[8];
    ULONG MsgType;
    NTLM_BLOB LmChallengeResponse; // An LM_RESPONSE or LMv2_RESPONSE
    NTLM_BLOB NtChallengeResponse; // An NTLM_RESPONSE or NTLMv2_RESPONSE
    NTLM_BLOB DomainName;
    NTLM_BLOB UserName;
    NTLM_BLOB WorkstationName;
    NTLM_BLOB EncryptedRandomSessionKey; //only if NTLMSSP_NEGOTIATE_KEY_EXCHANGE 
    ULONG NegotiateFlags;
    NTLM_WINDOWS_VERSION Version;
    BYTE MIC[16]; //doc says its ommited in nt,2k,xp,2k3
    /* payload */
}AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

SECURITY_STATUS
NtlmGenerateNegotiateMessage(
    IN ULONG_PTR hContext,
    IN ULONG ContextReq,
    IN ULONG NegotiateFlags,
    IN PSecBuffer InputToken,
    OUT PSecBuffer *OutputToken);

SECURITY_STATUS
NtlmHandleNegotiateMessage(
    IN ULONG_PTR hCredential,
    IN OUT PULONG_PTR phContext,
    IN ULONG fContextReq,
    IN PSecBuffer InputToken,
    OUT PSecBuffer *OutputToken,
    OUT PULONG fContextAttributes,
    OUT PTimeStamp ptsExpiry);

SECURITY_STATUS
NtlmHandleAuthenticateMessage(
    IN ULONG_PTR hCredential,
    IN OUT PULONG_PTR phContext,
    IN ULONG fContextReq,
    IN PSecBuffer *pInputTokens,
    OUT PSecBuffer OutputToken,
    OUT PULONG fContextAttributes,
    OUT PTimeStamp ptsExpiry,
    OUT PUCHAR pSessionKey,
    OUT PULONG pfNegotiateFlags,
    OUT PHANDLE TokenHandle,
    OUT PNTSTATUS pSubStatus,
    OUT PTimeStamp ptsPasswordExpiry,
    OUT PULONG pfUserFlags);

