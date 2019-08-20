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
* [MS-NLMP]  v20110504 for more details */

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

BOOLEAN
NTOWFv1(
    LPCWSTR password,
    PUCHAR result);

BOOL
NTOWFv2(
    IN LPCWSTR password,
    IN LPCWSTR user,
    IN LPCWSTR domain,
    OUT UCHAR result[16]);

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
    IN ULONG NegFlg,
    IN UCHAR SessionBaseKey[MSV1_0_USER_SESSION_KEY_LENGTH],
    IN UCHAR* LmChallengeResponse,
    IN ULONG LmChallengeResponseLen,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_RESPONSE_LENGTH],
    OUT UCHAR KeyExchangeKey[NTLM_KEYEXCHANGE_KEY_LENGTH]);

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

BOOL
CliComputeResponse(
    IN ULONG NegFlg,
    IN ULONG Challenge_NegFlg,
    IN PEXT_STRING_W pUserNameW,
    IN PEXT_STRING_W pPasswordW,
    IN PEXT_STRING_W pDomainNameW,
    IN PEXT_STRING_W pServerName,
    IN UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TargetInfoTimeStamp,
    IN OUT PNTLMSSP_CONTEXT_MSG ctxmsg,
    IN OUT PNTLM_DATABUF pNtChallengeResponseData,
    OUT PEXT_DATA pLmChallengeResponseData,
    IN OUT PEXT_DATA EncryptedRandomSessionKey);

BOOL
CliComputeKeys(
    IN ULONG ChallengeMsg_NegFlg,
    IN PUSER_SESSION_KEY pSessionBaseKey,
    IN PEXT_DATA pLmChallengeResponseData,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ResponseKeyLM[MSV1_0_RESPONSE_LENGTH],
    OUT UCHAR ExportedSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH],
    OUT PEXT_DATA pEncryptedRandomSessionKey,
    OUT PNTLMSSP_CONTEXT_MSG ctxmsg);

/* avl functions */

#define NtlmAvlAlloc NtlmDataBufAlloc
#define NtlmAvlFree NtlmDataBufFree

BOOL
NtlmAvlGet(
    IN PNTLM_DATABUF pAvData,
    IN MSV1_0_AVID AvId,
    OUT PVOID* pData,
    OUT PULONG plen);
//ULONG
//NtlmAvlLen(
//    IN PMSV1_0_AV_PAIR pAvList,
//    IN LONG cAvList);

BOOL
NtlmAvlAdd(
    IN PNTLM_DATABUF pAvData,
    IN MSV1_0_AVID AvId,
    IN void* data,
    IN ULONG len);

ULONG
NtlmAvlSize(
    IN ULONG Pairs,
    IN ULONG PairsLen);

/* client message functions */
SECURITY_STATUS
CliGenerateNegotiateMessage(
    IN PNTLMSSP_CONTEXT_CLI context,
    IN ULONG ContextReq,
    OUT PSecBuffer OutputToken);

SECURITY_STATUS
CliGenerateAuthenticationMessage(
    IN ULONG_PTR hContext,
    IN ULONG ISCContextReq,
    IN PSecBuffer InputToken1,
    IN PSecBuffer InputToken2,
    IN OUT PSecBuffer OutputToken1,
    IN OUT PSecBuffer OutputToken2,
    OUT PULONG pISCContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PULONG NegotiateFlags);

/* server message functions */

/* ??? message functions */
SECURITY_STATUS
SvrHandleNegotiateMessage(
    IN ULONG_PTR hCredential,
    IN OUT PULONG_PTR phContext,
    IN ULONG ASCContextReq,
    IN PSecBuffer InputToken1,
    IN PSecBuffer InputToken2,
    OUT PSecBuffer OutputToken1,
    OUT PSecBuffer OutputToken2,
    OUT PULONG pASCContextAttr,
    OUT PTimeStamp ptsExpiry);

SECURITY_STATUS
CliGenerateChallengeMessage(
    IN PNTLMSSP_CONTEXT_SVR Context,
    IN PNTLMSSP_CREDENTIAL Credentials,
    IN ULONG ASCContextReq,
    IN EXT_STRING TargetName,
    IN ULONG MessageFlags,
    OUT PSecBuffer OutputToken);

SECURITY_STATUS
SvrHandleAuthenticateMessage(
    IN ULONG_PTR hContext,
    IN ULONG ASCContextReq,
    IN PSecBuffer InputToken,
    OUT PSecBuffer OutputToken,
    OUT PULONG pASCContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PUCHAR pSessionKey,
    OUT PULONG pfUserFlags);

/* helper functions */

/* makes only a reference to blob - do not free! */
/* do not use for strings ... its not null terminated! */
SECURITY_STATUS
NtlmBlobToExtDataRef(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_DATA OutputStr);
SECURITY_STATUS
NtlmCopyBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    OUT PVOID dst,
    IN ULONG len);

/* copys a NTLM message blob into a string */
SECURITY_STATUS
NtlmCreateExtWStrFromBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_STRING_W OutputStr);
SECURITY_STATUS
NtlmCreateExtAStrFromBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_STRING_A OutputStr);

VOID
NtlmExtStringToBlob(
    IN PVOID OutputBuffer,
    IN PEXT_STRING InStr,
    IN OUT PNTLM_BLOB OutputBlob,
    IN OUT PULONG_PTR OffSet);
#define NtlmUnicodeStringToBlob(buf, str, out, ofs) NtlmExtStringToBlob(buf, (PEXT_STRING)str, out, ofs)

VOID
NtlmWriteDataBufToBlob(
    IN PVOID OutputBuffer,
    IN PNTLM_DATABUF pDataBuf,
    IN OUT PNTLM_BLOB OutputBlob,
    IN OUT PULONG_PTR OffSet);

VOID
NtlmAppendToBlob(IN void* buffer,
                 IN ULONG len,
                 IN OUT PNTLM_BLOB OutputBlob,
                 IN OUT PULONG_PTR OffSet);

VOID
NtlmStructWriteStrA(
    IN void* dataStart,
    IN ULONG dataSize,
    OUT PCHAR* pDataFieldA,
    IN const char* dataA,
    IN OUT PBYTE* pOffset);
VOID
NtlmStructWriteStrW(
    IN void* dataStart,
    IN ULONG dataSize,
    OUT PWCHAR* pDataFieldW,
    IN const WCHAR* dataW,
    IN OUT PBYTE* pOffset);

/* calculations */
BOOL
CliComputeResponseNTLMv2(
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN PEXT_STRING_W userdom,
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_RESPONSE_LENGTH],
    IN UCHAR ResponseKeyNT[MSV1_0_NTLM3_RESPONSE_LENGTH],
    IN PEXT_STRING_W ServerName,
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN ULONGLONG TimeStamp,
    IN OUT PNTLM_DATABUF pNtChallengeResponse,
    OUT PLM2_RESPONSE pLmChallengeResponse,
    OUT PUSER_SESSION_KEY SessionBaseKey);

BOOL
CliComputeResponseNTLMv1(
    IN ULONG NegFlg,
    IN PEXT_STRING_W user,
    IN PEXT_STRING_W passwd,
    IN UCHAR ResponseKeyLM[MSV1_0_NTLM3_RESPONSE_LENGTH],
    IN UCHAR ResponseKeyNT[MSV1_0_NTLM3_RESPONSE_LENGTH],
    IN UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH],
    IN UCHAR ClientChallenge[MSV1_0_CHALLENGE_LENGTH],
    OUT UCHAR NtChallengeResponse[MSV1_0_RESPONSE_LENGTH],
    OUT UCHAR LmChallengeResponse[MSV1_0_RESPONSE_LENGTH]);

VOID
RC4Init(
    OUT PHANDLE pClientHandle,
    IN UCHAR* Key);

