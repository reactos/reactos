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

#ifdef __UNUSED__
/* globals */
extern SECPKG_FUNCTION_TABLE NtLmPkgFuncTable; //functions we provide to LSA in SpLsaModeInitialize
extern PSECPKG_DLL_FUNCTIONS NtlmPkgDllFuncTable; //fuctions provided by LSA in SpInstanceInit
extern SECPKG_USER_FUNCTION_TABLE NtlmUmodeFuncTable; //fuctions we provide via SpUserModeInitialize
extern PLSA_SECPKG_FUNCTION_TABLE NtlmLsaFuncTable; // functions provided by LSA in SpInitialize
#endif

/* client configuraion flags CliLMLevel
 * (settings depends on (registry-Key) LMCompatibilityLevel) */
#define CLI_LMFLAG_USE_AUTH_LM     0x000000001
#define CLI_LMFLAG_USE_AUTH_NTLMv1 0x000000002
#define CLI_LMFLAG_USE_AUTH_NTLMv2 0x000000004
#define CLI_LMFLAG_USE_SSEC_NTLMv2 0x000000008
/* server configuration flags (SvrLMLevel)
 * (settings depends on (registry-Key) LMCompatibilityLevel) */
#define SVR_LMFLAG_ACCPT_AUTH_LM     0x000000001
#define SVR_LMFLAG_ACCPT_AUTH_NTLMv1 0x000000002
#define SVR_LMFLAG_ACCPT_AUTH_NTLMv2 0x000000004

#define NTLM_KEYEXCHANGE_KEY_LENGTH 16
#define NTLM_ENCRNDSESSION_KEY_LENGTH 16
#define NTLM_SEALINGKEY_LENGTH 16
#define NTLM_SIGNKEY_LENGTH 16

typedef struct _NTLMSSP_GLOBALS
{
    /* internal - use to read/write global state */
    CRITICAL_SECTION cs;

    HANDLE NtlmSystemSecurityToken;
    /* needed by client and server */
    EXT_STRING_A NbMachineNameOEM;
    EXT_STRING_A NbDomainNameOEM;

    /* should be readed from registry
     * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa\LMCompatibilityLevel */
    /* editable witht local security policy console */
    ULONG LMCompatibilityLevel;
} NTLMSSP_GLOBALS, *PNTLMSSP_GLOBALS;


/* MS-NLMP 3.2.1.1 */
typedef struct _NTLMSSP_GLOBALS_CLI
{
    /* internal - use to read/write global state */
    CRITICAL_SECTION cs;

    /* needed vars from MS-NLMP
     * activate if needed ... or move to context (cli) if needed
     * (ctx) means variable is _NTLMSSP_CONTEXT_CLI */
    /* The set of client configuration flags (section 2.2.2.5) that specify the
     * full set of capabilities of the client. */
    ULONG ClientConfigFlags;
    // ExportedSessionKey:
    /* NegFlg: (ctx) */
    // User: A string that indicates the name of the user.
    // UserDom: A string that indicates the name of the user's domain.
    //   The following NTLM configuration variables are internal to the client and impact all authenticated
    //   sessions:
    // NoLMResponseNTLMv1: A Boolean setting that controls using the NTLM response for the LM
    //   response to the server challenge when NTLMv1 authentication is used.<35>
    // ClientBlocked: A Boolean setting that disables the client from sending NTLM authenticate messages,
    //   as defined in section 2.2.1.3.<36>
    // ClientBlockExceptions: A list of server names that can use NTLM authentication.<37>
    // ClientRequire128bitEncryption: A Boolean setting that requires the client to use 128-bit
    //   encryption.<38>
    // The following variables are internal to the client and are maintained for the entire length of the
    //  authenticated session:
    // MaxLifetime: An integer that indicates the maximum lifetime for challenge/response pairs.<

    /* vars not in spec (MS-NLMP) */
    /* CliLMLevel - "expanded LMCompatiblilyLevel" */
    ULONG CliLMLevel;
} NTLMSSP_GLOBALS_CLI, *PNTLMSSP_GLOBALS_CLI;

/* MS-NLMP 3.2.1.1 */
typedef struct _NTLMSSP_GLOBALS_SVR
{
    /* internal - use to read/write global state */
    CRITICAL_SECTION cs;

    /* needed vars from MS-NLMP
     * activate if needed ... or move to context (svr) if needed
     * (ctx) means variable is _NTLMSSP_CONTEXT_SVR */
    //Additionally, the server maintains the following:
    /* The set of server configuration flags (section 2.2.2.5) that specify the
       full set of capabilities of the server. */
    ULONG CfgFlg;
    //DnsDomainName: A string that indicates the fully qualified domain name (FQDN) of the server's
    //domain.
    //DnsForestName: A string that indicates the FQDN of the server's forest. The DnsForestName is
    //NULL on machines that are not domain joined.
    /* MS-NLMP: A string that indicates the FQDN of the server. */
    EXT_STRING_W DnsMachineName;
    EXT_STRING_A DnsMachineNameOEM;
    /* MS-NLMP: A string that indicates the NetBIOS
     * name of the server's domain. */
    EXT_STRING_W NbDomainName;
    /* MS-NLMP: A string that indicates the NetBIOS
     * machine name of the server. */
    EXT_STRING_W NbMachineName;
    //The following NTLM server configuration variables are internal to the client and impact all
    //authenticated sessions:
    //ServerBlock: A Boolean setting that disables the server from generating challenges and responding
    //to NTLM_NEGOTIATE messages.<59>
    //ServerRequire128bitEncryption: A Boolean setting that requires the server to use 128-bit
    //encryption.<60>

    /* SvrLMLevel - "expanded LMCompatiblilyLevel" */
    ULONG SvrLMLevel;
    /* lmJoinState - is this computer a domain member? */
    NETSETUP_JOIN_STATUS lmJoinState;
} NTLMSSP_GLOBALS_SVR, *PNTLMSSP_GLOBALS_SVR;

typedef enum _NTLM_MODE
{
    NtlmUnknownMode = 0,
    NtlmLsaMode = 1,
    NtlmUserMode
} NTLM_MODE, *PNTLM_MODE;

extern NTLM_MODE NtlmMode;

#define inLsaMode (NtlmMode == NtlmLsaMode)
#define inUserMode (NtlmMode == NtlmUserMode)

#ifdef __UNUSED__
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
#endif

#define NTLM_DEFAULT_TIMEOUT (5*60*1000) //context valid for 5 mins
#define NTLM_MAX_BUF 1904
#define NTLM_CRED_NULLSESSION SECPKG_CRED_RESERVED

typedef struct _NTLMSSP_CREDENTIAL
{
    LIST_ENTRY Entry;
    ULONG RefCount;
    ULONG UseFlags;
    EXT_STRING UserNameW;
    EXT_STRING PasswordW;
    EXT_STRING DomainNameW;
    ULONG ProcId;
    HANDLE SecToken;
    LUID LogonId;
    TimeStamp ExpirationTime;
} NTLMSSP_CREDENTIAL, *PNTLMSSP_CREDENTIAL;

typedef enum {
    Idle,
    NegotiateSent,
    ChallengeSent,
    AuthenticateSent,
    Authenticated,
    PassedToService
} NTLMSSP_CONTEXT_STATE, *PNTLMSSP_CONTEXT_STATE;

/* context (client + server) base */
typedef struct _NTLMSSP_CONTEXT_HDR
{
    LIST_ENTRY Entry;
    BOOL isServer;
    ULONG RefCount;
    LARGE_INTEGER StartTime;
    ULONG ProcId;
    ULONG Timeout;
    NTLMSSP_CONTEXT_STATE State;
} NTLMSSP_CONTEXT_HDR, *PNTLMSSP_CONTEXT_HDR;

/* context - message support (client + server) */
typedef struct _NTLMSSP_CONTEXT_MSG
{
    /* MS-NLSP 3.4.1 - variables */
    /* ClientHandle (Public): The handle to a key state structure corresponding to the current state of
       the ClientSealingKey. */
    rc4_key ClientHandle;
    /* ServerHandle (Public): The handle to a key state structure corresponding to the current state of
      the ServerSealingKey. */
    rc4_key ServerHandle;
    ULONG ClientSeqNum;
    ULONG ServerSeqNum;
    UCHAR ClientSigningKey[NTLM_SIGNKEY_LENGTH];
    UCHAR ClientSealingKey[NTLM_SEALINGKEY_LENGTH];
    UCHAR ServerSigningKey[NTLM_SIGNKEY_LENGTH];
    UCHAR ServerSealingKey[NTLM_SEALINGKEY_LENGTH];

    /* not in spec or unknown whether they are... */
    /* message support  */
    UCHAR MessageIntegrityCheck[16];
} NTLMSSP_CONTEXT_MSG, *PNTLMSSP_CONTEXT_MSG;

typedef struct _NTLMSSP_CONTEXT_CLI
{
    NTLMSSP_CONTEXT_HDR hdr;
    // MS-NLSP 3.1.1.1 (see also _NTLMSSP_GLOBALS_CLI)
    /* The set of configuration flags (section 2.2.2.5) that
     * specifies the negotiated capabilities of the client and
     * server for the current NTLM session. */
    ULONG NegFlg;
    // TODO ... rename according to spec
    BOOL isLocal;
    PNTLMSSP_CREDENTIAL Credential;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
    UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    HANDLE ClientToken;
    
    NTLMSSP_CONTEXT_MSG msg;

    /* extra flags (not directly in spec) */
    BOOL UseNTLMv2;

} NTLMSSP_CONTEXT_CLI, *PNTLMSSP_CONTEXT_CLI;

typedef struct _NTLMSSP_CONTEXT_SVR
{
    NTLMSSP_CONTEXT_HDR hdr;
    // MS-NLSP 3.2.1.1 (see also _NTLMSSP_GLOBALS_SVR)
    /* The server maintains all of the variables that the client does
     * (section 3.1.1.1) except the ClientConfigFlags.*/
    // NTLMSSP_CONTEXT_CLI cli;
    ULONG cli_NegFlg;
    NTLMSSP_CONTEXT_MSG cli_msg;

    // TODO ... rename according to spec
    BOOL isLocal;
    /* FIXME: These flags are only assigned, never used ... remove? */
    ULONG ISCRetContextFlags;
    ULONG ASCRetContextFlags;
    PNTLMSSP_CREDENTIAL Credential;
    UCHAR SessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    HANDLE ClientToken;

    /* generated in GenerateChallenge, needed when
     * AUTHENTICATE_MESSAGE is processed */
    UCHAR ServerChallenge[MSV1_0_CHALLENGE_LENGTH];

} NTLMSSP_CONTEXT_SVR, *PNTLMSSP_CONTEXT_SVR;

/* private functions */

/* ntlmssp.c */
BOOL
NtlmInitializeGlobals(VOID);

VOID
NtlmTerminateGlobals(VOID);

PNTLMSSP_GLOBALS lockGlobals(VOID);
PNTLMSSP_GLOBALS_CLI lockGlobalsCli(VOID);
PNTLMSSP_GLOBALS_SVR lockGlobalsSvr(VOID);

VOID unlockGlobals(IN OUT PNTLMSSP_GLOBALS* g);
VOID unlockGlobalsCli(IN OUT PNTLMSSP_GLOBALS_CLI* g);
VOID unlockGlobalsSvr(IN OUT PNTLMSSP_GLOBALS_SVR* g);

/* globals read only, no lock needed */
PNTLMSSP_GLOBALS getGlobals(VOID);
PNTLMSSP_GLOBALS_CLI getGlobalsCli(VOID);
PNTLMSSP_GLOBALS_SVR getGlobalsSvr(VOID);

/* context.c */

NTSTATUS
NtlmContextInitialize(VOID);

VOID
NtlmContextTerminate(VOID);

PNTLMSSP_CONTEXT_CLI
NtlmAllocateContextCli(VOID);
PNTLMSSP_CONTEXT_SVR
NtlmAllocateContextSvr(VOID);

PNTLMSSP_CONTEXT_HDR
NtlmReferenceContextHdr(IN ULONG_PTR Handle);
PNTLMSSP_CONTEXT_MSG
NtlmReferenceContextMsg(
    IN ULONG_PTR Handle,
    IN BOOL isSending,
    OUT PULONG pNegFlg,
    OUT prc4_key* pSealHandle,
    OUT PBYTE* pSignKey,
    OUT PULONG* pSeqNum);
PNTLMSSP_CONTEXT_CLI
NtlmReferenceContextCli(IN ULONG_PTR Handle);
PNTLMSSP_CONTEXT_SVR
NtlmReferenceContextSvr(IN ULONG_PTR Handle);

VOID
NtlmDereferenceContext(IN ULONG_PTR Handle);

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

void*
NtlmAllocate(
    IN size_t Size);

void
NtlmFree(
    IN void* Buffer);

void
NtlmInit(
    _In_ NTLM_MODE mode);

void
NtlmFini(void);

#ifdef __UNUSED__
BOOLEAN
NtlmHasIntervalElapsed(
    IN LARGE_INTEGER Start,
    IN LONG Timeout);
#endif

BOOLEAN
NtlmGetSecBuffer(
    IN OPTIONAL PSecBufferDesc pInputDesc,
    IN ULONG BufferIndex,
    OUT PSecBuffer *pOutBuffer,
    IN BOOLEAN Output);

BOOLEAN
NtlmGetSecBufferType(
    _In_ OPTIONAL PSecBufferDesc pInputDesc,
    _In_ ULONG BufferType,
    _In_ ULONG BufferIndex,
    _In_ BOOLEAN OutputBuffer,
    _Out_ PSecBuffer *pOutBuffer);

/* debug.c */

void
NtlmPrintNegotiateFlags(ULONG Flags);

void
NtlmPrintHexDump(PBYTE buffer, DWORD length);

void
NtlmPrintAvPairs(
    IN PEXT_DATA pAvData);

void
PrintSignSealKeyInfo(PNTLMSSP_CONTEXT_MSG msg);

#endif
