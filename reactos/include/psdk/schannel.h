#ifndef _SCHANNEL_H
#define _SCHANNEL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHANNEL_NAME_A "Schannel"
#define SCHANNEL_NAME_W L"Schannel"
#ifdef UNICODE
#define SCHANNEL_NAME SCHANNEL_NAME_W
#else
#define SCHANNEL_NAME SCHANNEL_NAME_A
#endif

#define SCH_CRED_V1           1
#define SCH_CRED_V2           2
#define SCH_CRED_VERSION      2
#define SCH_CRED_V3           3
#define SCHANNEL_CRED_VERSION 4

#define SCHANNEL_RENEGOTIATE 0
#define SCHANNEL_SHUTDOWN    1
#define SCHANNEL_ALERT       2
#define SCHANNEL_SESSION     3

#define SP_PROT_TLS1_CLIENT 128
#define SP_PROT_TLS1_SERVER 64
#define SP_PROT_SSL3_CLIENT 32
#define SP_PROT_SSL3_SERVER 16
#define SP_PROT_SSL2_CLIENT 8
#define SP_PROT_SSL2_SERVER 4
#define SP_PROT_PCT1_CLIENT 2
#define SP_PROT_PCT1_SERVER 1

#define SP_PROT_TLS1 (SP_PROT_TLS1_CLIENT | SP_PROT_TLS1_SERVER)
#define SP_PROT_SSL3 (SP_PROT_SSL3_CLIENT | SP_PROT_SSL3_SERVER)
#define SP_PROT_SSL2 (SP_PROT_SSL2_CLIENT | SP_PROT_SSL2_SERVER)
#define SP_PROT_PCT1 (SP_PROT_PCT1_CLIENT | SP_PROT_PCT1_SERVER)

#define SCH_CRED_NO_SYSTEM_MAPPER                    2
#define SCH_CRED_NO_SERVERNAME_CHECK                 4
#define SCH_CRED_MANUAL_CRED_VALIDATION              8
#define SCH_CRED_NO_DEFAULT_CREDS                    16
#define SCH_CRED_AUTO_CRED_VALIDATION                32
#define SCH_CRED_USE_DEFAULT_CREDS                   64
#define SCH_CRED_REVOCATION_CHECK_CHAIN_END_CERT     256
#define SCH_CRED_REVOCATION_CHECK_CHAIN              512
#define SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 1024
#define SCH_CRED_IGNORE_NO_REVOCATION_CHECK          2048
#define SCH_CRED_IGNORE_REVOCATION_OFFLINE           4096

#define SECPKG_ATTR_ISSUER_LIST         0x50
#define SECPKG_ATTR_REMOTE_CRED         0x51
#define SECPKG_ATTR_LOCAL_CRED          0x52
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT 0x53
#define SECPKG_ATTR_LOCAL_CERT_CONTEXT  0x54
#define SECPKG_ATTR_ROOT_STORE          0x55
#define SECPKG_ATTR_SUPPORTED_ALGS      0x56
#define SECPKG_ATTR_CIPHER_STRENGTHS    0x57
#define SECPKG_ATTR_SUPPORTED_PROTOCOLS 0x58
#define SECPKG_ATTR_ISSUER_LIST_EX      0x59
#define SECPKG_ATTR_CONNECTION_INFO     0x5a
#define SECPKG_ATTR_EAP_KEY_BLOCK       0x5b
#define SECPKG_ATTR_MAPPED_CRED_ATTR    0x5c
#define SECPKG_ATTR_SESSION_INFO        0x5d
#define SECPKG_ATTR_APP_DATA            0x5e

#define UNISP_RPC_ID 14

struct _HMAPPER;

typedef struct _SCHANNEL_CRED
{
    DWORD dwVersion;
    DWORD cCreds;
    PCCERT_CONTEXT *paCred;
    HCERTSTORE hRootStore;
    DWORD cMappers;
    struct _HMAPPER **aphMappers;
    DWORD cSupportedAlgs;
    ALG_ID *palgSupportedAlgs;
    DWORD grbitEnabledProtocols;
    DWORD dwMinimumCipherStrength;
    DWORD dwMaximumCipherStrength;
    DWORD dwSessionLength;
    DWORD dwFlags;
    DWORD reserved;
} SCHANNEL_CRED, *PSCHANNEL_CRED;

typedef struct _SecPkgCred_SupportedAlgs
{
    DWORD cSupportedAlgs;
    ALG_ID *palgSupportedAlgs;
} SecPkgCred_SupportedAlgs, *PSecPkgCred_SupportedAlgs;

typedef struct _SecPkgCred_CipherStrengths
{
    DWORD dwMinimumCipherStrength;
    DWORD dwMaximumCipherStrength;
} SecPkgCred_CipherStrengths, *PSecPkgCred_CipherStrengths;

typedef struct _SecPkgCred_SupportedProtocols
{
    DWORD grbitProtocol;
} SecPkgCred_SupportedProtocols, *PSecPkgCred_SupportedProtocols;

typedef struct _SecPkgContext_IssuerListInfoEx
{
    PCERT_NAME_BLOB aIssuers;
    DWORD cIssuers;
} SecPkgContext_IssuerListInfoEx, *PSecPkgContext_IssuerListInfoEx;

typedef struct _SecPkgContext_ConnectionInfo
{
    DWORD dwProtocol;
    ALG_ID aiCipher;
    DWORD dwCipherStrength;
    ALG_ID aiHash;
    DWORD dwHashStrength;
    ALG_ID aiExch;
    DWORD dwExchStrength;
} SecPkgContext_ConnectionInfo, *PSecPkgContext_ConnectionInfo;

#ifdef __cplusplus
}
#endif

#endif /* _SCHANNEL_H */
