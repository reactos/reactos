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

#define NTLMSSP_REVISION_W2K3 0x0F

/* basic types */
typedef char const* PCCHAR;

typedef struct _CYPHER_BLOCK
{
    CHAR data[8];
}CYPHER_BLOCK, *PCYPHER_BLOCK;

typedef struct _USER_SESSION_KEY
{
    CYPHER_BLOCK data[2];
}USER_SESSION_KEY, *PUSER_SESSION_KEY;

typedef struct _NT_OWF_PASSWORD
{
    CYPHER_BLOCK data[2];
}NT_OWF_PASSWORD, *PNT_OWF_PASSWORD;

typedef struct _LM_OWF_PASSWORD
{
    CYPHER_BLOCK data[2];
}LM_OWF_PASSWORD, *PLM_OWF_PASSWORD;

/* where to put? correct ?*/
typedef struct _LM_SESSION_KEY
{
    UCHAR data[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} LM_SESSION_KEY, *PLM_SESSION_KEY;

typedef struct _LM2_RESPONSE
{
    UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
    UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
} LM2_RESPONSE, *PLM2_RESPONSE;

/* message types */

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

typedef struct _MESSAGE_SIGNATURE
{
    ULONG Version;
    ULONG RandomPad;
    ULONG Checksum;
    ULONG Nonce;
}MESSAGE_SIGNATURE, *PMESSAGE_SIGNATURE;

C_ASSERT(sizeof(MESSAGE_SIGNATURE) == 16);
/* basic functions */

VOID
NTOWFv1(
    LPCWSTR password,
    PUCHAR result);

BOOLEAN
NTOWFv2(
    LPCWSTR password,
    LPCWSTR user,
    LPCWSTR domain,
    PUCHAR result);

VOID
LMOWFv1(
    const PCCHAR password,
    PUCHAR result);

BOOLEAN
LMOWFv2(
    LPCWSTR password,
    LPCWSTR user,
    LPCWSTR domain,
    PUCHAR result);

VOID
NONCE(
    PUCHAR buffer,
    ULONG num);

VOID
KXKEY(
    ULONG flags,
    const PUCHAR session_base_key,
    const PUCHAR lm_challenge_resonse,
    const PUCHAR server_challenge,
    PUCHAR key_exchange_key);

BOOLEAN
SIGNKEY(
    const PUCHAR RandomSessionKey,
    BOOLEAN IsClient,
    PUCHAR Result);

BOOLEAN
SEALKEY(
    ULONG flags,
    const PUCHAR  RandomSessionKey,
    BOOLEAN client,
    PUCHAR result);

BOOLEAN
MAC(ULONG flags,
    PCCHAR buf,
    ULONG buf_len,
    PUCHAR sign_key,
    ULONG sign_key_len,
    PUCHAR seal_key,
    ULONG seal_key_len,
    ULONG random_pad,
    ULONG sequence,
    PUCHAR result);

VOID
NtlmChallengeResponse(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pPassword,
    IN PUNICODE_STRING pDomainName,
    IN PUNICODE_STRING pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    OUT PMSV1_0_NTLM3_RESPONSE pNtResponse,
    OUT PLM2_RESPONSE pLm2Response,
    OUT PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_SESSION_KEY LmSessionKey);

/* avl functions */

PMSV1_0_AV_PAIR
NtlmAvlInit(
    IN void * pAvList);

PMSV1_0_AV_PAIR
NtlmAvlGet(
    IN PMSV1_0_AV_PAIR pAvList,
    IN MSV1_0_AVID AvId,
    IN LONG cAvList);

ULONG
NtlmAvlLen(
    IN PMSV1_0_AV_PAIR pAvList,
    IN LONG cAvList);

PMSV1_0_AV_PAIR
NtlmAvlAdd(
    IN PMSV1_0_AV_PAIR pAvList,
    IN MSV1_0_AVID AvId,
    IN PUNICODE_STRING pString,
    IN LONG cAvList);

ULONG
NtlmAvlSize(
    IN ULONG Pairs,
    IN ULONG PairsLen);

/* message functions */
SECURITY_STATUS
NtlmGenerateNegotiateMessage(
    IN ULONG_PTR hContext,
    IN ULONG ContextReq,
    OUT PSecBuffer OutputToken);

SECURITY_STATUS
NtlmHandleNegotiateMessage(
    IN ULONG_PTR hCredential,
    IN OUT ULONG_PTR hContext,
    IN ULONG fContextReq,
    IN PSecBuffer InputToken1,
    IN PSecBuffer InputToken2,
    OUT PSecBuffer OutputToken1,
    OUT PSecBuffer OutputToken2,
    OUT PULONG pContextAttr,
    OUT PTimeStamp ptsExpiry);

SECURITY_STATUS
NtlmGenerateChallengeMessage(
    IN PNTLMSSP_CONTEXT Context,
    IN PNTLMSSP_CREDENTIAL Credentials,
    IN UNICODE_STRING TargetName,
    IN ULONG MessageFlags,
    OUT PSecBuffer OutputToken);

SECURITY_STATUS
NtlmHandleChallengeMessage(
    IN ULONG_PTR hContext,
    IN ULONG ContextReq,
    IN PSecBuffer InputToken1,
    IN PSecBuffer InputToken2,
    IN OUT PSecBuffer OutputToken1,
    IN OUT PSecBuffer OutputToken2,
    OUT PULONG ContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PULONG NegotiateFlags);

SECURITY_STATUS
NtlmHandleAuthenticateMessage(
    IN ULONG_PTR hContext,
    IN ULONG fContextReq,
    IN PSecBuffer InputToken,
    OUT PSecBuffer OutputToken,
    OUT PULONG pContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PUCHAR pSessionKey,
    OUT PULONG pfUserFlags);

/* helper functions */

SECURITY_STATUS
NtlmBlobToUnicodeString(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PUNICODE_STRING OutputStr);

VOID
NtlmUnicodeStringToBlob(
    IN PVOID OutputBuffer,
    IN PUNICODE_STRING InStr,
    IN OUT PNTLM_BLOB OutputBlob,
    IN OUT PULONG_PTR OffSet);

