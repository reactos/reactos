/*
 * Copyright (C) 2005 Juan Lang
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
 */
#ifndef __WINE_SCHANNEL_H__
#define __WINE_SCHANNEL_H__

#include <wincrypt.h>

/* Package names */
#define UNISP_NAME_A "Microsoft Unified Security Protocol Provider"
#if defined(__GNUC__)
#define UNISP_NAME_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',\
 ' ','U','n','i','f','i','e','d',' ','S','e','c','u','r','i','t','y',' ',\
 'P','r','o','t','o','c','o','l',' ','P','r','o','v','i','d','e','r',0 }
#elif defined(_MSC_VER)
#define UNISP_NAME_W L"Microsoft Unified Security Protocol Provider"
#else
static const WCHAR UNISP_NAME_W[] = { 'M','i','c','r','o','s','o','f','t',
 ' ','U','n','i','f','i','e','d',' ','S','e','c','u','r','i','t','y',' ',
 'P','r','o','t','o','c','o','l',' ','P','r','o','v','i','d','e','r',0 };
#endif
#define UNISP_NAME WINELIB_NAME_AW(UNISP_NAME_)

#define SSL2SP_NAME_A   "Microsoft SSL 2.0"
#if defined(__GNUC__)
#define SSL2SP_NAME_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',\
 ' ','S','S','L',' ','2','.','0',0 }
#elif defined(_MSC_VER)
#define SSL2SP_NAME_W  L"Microsoft SSL 2.0"
#else
static const WCHAR SSL2SP_NAME_W[] = { 'M','i','c','r','o','s','o','f','t',
 ' ','S','S','L',' ','2','.','0',0 };
#endif
#define SSL2SP_NAME WINELIB_NAME_AW(SSL2SP_NAME_)

#define SSL3SP_NAME_A   "Microsoft SSL 3.0"
#if defined(__GNUC__)
#define SSL3SP_NAME_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',\
 ' ','S','S','L',' ','3','.','0',0 }
#elif defined(_MSC_VER)
#define SSL3SP_NAME_W  L"Microsoft SSL 3.0"
#else
static const WCHAR SSL3SP_NAME_W[] = { 'M','i','c','r','o','s','o','f','t',
 ' ','S','S','L',' ','3','.','0',0 };
#endif
#define SSL3SP_NAME WINELIB_NAME_AW(SSL3SP_NAME_)

#define TLS1SP_NAME_A   "Microsoft TLS 1.0"
#if defined(__GNUC__)
#define TLS1SP_NAME_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',\
 ' ','T','L','S',' ','1','.','0',0 }
#elif defined(_MSC_VER)
#define TLS1SP_NAME_W  L"Microsoft TLS 1.0"
#else
static const WCHAR TLS1SP_NAME_W[] = { 'M','i','c','r','o','s','o','f','t',
 ' ','T','L','S',' ','1','.','0',0 };
#endif
#define TLS1SP_NAME WINELIB_NAME_AW(TLS1SP_NAME_)

#define PCT1SP_NAME_A   "Microsoft PCT 1.0"
#if defined(__GNUC__)
#define PCT1SP_NAME_W (const WCHAR []){ 'M','i','c','r','o','s','o','f','t',\
 ' ','P','C','T',' ','1','.','0',0 }
#elif defined(_MSC_VER)
#define PCT1SP_NAME_W  L"Microsoft PCT 1.0"
#else
static const WCHAR PCT1SP_NAME_W[] = { 'M','i','c','r','o','s','o','f','t',
 ' ','P','C','T',' ','1','.','0',0 };
#endif
#define PCT1SP_NAME WINELIB_NAME_AW(PCT1SP_NAME_)

#define SCHANNEL_NAME_A "Schannel"
#if defined(__GNUC__)
#define SCHANNEL_NAME_W (const WCHAR []){ 'S','c','h','a','n','n','e','l',0 }
#elif defined(_MSC_VER)
#define SCHANNEL_NAME_W  L"Schannel"
#else
static const WCHAR SCHANNEL_NAME_W[] = { 'S','c','h','a','n','n','e','l',0 };
#endif
#define SCHANNEL_NAME WINELIB_NAME_AW(SCHANNEL_NAME_)

#define SCH_CRED_V1           1
#define SCH_CRED_V2           2
#define SCH_CRED_VERSION      2
#define SCH_CRED_V3           3
#define SCHANNEL_CRED_VERSION 4

#define SCHANNEL_RENEGOTIATE 0
#define SCHANNEL_SHUTDOWN    1
#define SCHANNEL_ALERT       2
#define SCHANNEL_SESSION     3

#define SP_PROT_ALL           0xffffffff
#define SP_PROT_UNI_CLIENT    0x80000000
#define SP_PROT_UNI_SERVER    0x40000000
#define SP_PROT_DTLS1_2_SERVER 0x00040000
#define SP_PROT_DTLS1_2_CLIENT 0x00080000
#define SP_PROT_DTLS_SERVER    0x00010000
#define SP_PROT_DTLS_CLIENT    0x00020000
#define SP_PROT_DTLS1_0_SERVER SP_PROT_DTLS_SERVER
#define SP_PROT_DTLS1_0_CLIENT SP_PROT_DTLS_CLIENT
#define SP_PROT_TLS1_3_SERVER 0x00001000
#define SP_PROT_TLS1_3_CLIENT 0x00002000
#define SP_PROT_TLS1_2_CLIENT 0x00000800
#define SP_PROT_TLS1_2_SERVER 0x00000400
#define SP_PROT_TLS1_1_CLIENT 0x00000200
#define SP_PROT_TLS1_1_SERVER 0x00000100
#define SP_PROT_TLS1_0_CLIENT SP_PROT_TLS1_CLIENT
#define SP_PROT_TLS1_0_SERVER SP_PROT_TLS1_SERVER
#define SP_PROT_TLS1_CLIENT   0x00000080
#define SP_PROT_TLS1_SERVER   0x00000040
#define SP_PROT_SSL3_CLIENT   0x00000020
#define SP_PROT_SSL3_SERVER   0x00000010
#define SP_PROT_SSL2_CLIENT   0x00000008
#define SP_PROT_SSL2_SERVER   0x00000004
#define SP_PROT_PCT1_CLIENT   0x00000002
#define SP_PROT_PCT1_SERVER   0x00000001
#define SP_PROT_NONE          0x00000000

#define SP_PROT_UNI                (SP_PROT_UNI_CLIENT | SP_PROT_UNI_SERVER)
#define SP_PROT_DTLS               (SP_PROT_DTLS_SERVER | SP_PROT_DTLS_CLIENT)
#define SP_PROT_DTLS1_0            (SP_PROT_DTLS1_0_SERVER | SP_PROT_DTLS1_0_CLIENT)
#define SP_PROT_DTLS1_2            (SP_PROT_DTLS1_2_SERVER | SP_PROT_DTLS1_2_CLIENT)
#define SP_PROT_TLS1_3             (SP_PROT_TLS1_3_CLIENT | SP_PROT_TLS1_3_SERVER)
#define SP_PROT_TLS1_2             (SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_2_SERVER)
#define SP_PROT_TLS1_1             (SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_1_SERVER)
#define SP_PROT_TLS1_0             (SP_PROT_TLS1_0_CLIENT | SP_PROT_TLS1_0_SERVER)
#define SP_PROT_TLS1               (SP_PROT_TLS1_CLIENT | SP_PROT_TLS1_SERVER)
#define SP_PROT_SSL3               (SP_PROT_SSL3_CLIENT | SP_PROT_SSL3_SERVER)
#define SP_PROT_SSL2               (SP_PROT_SSL2_CLIENT | SP_PROT_SSL2_SERVER)
#define SP_PROT_PCT1               (SP_PROT_PCT1_CLIENT | SP_PROT_PCT1_SERVER)

#define SP_PROT_SSL3TLS1_CLIENTS   (SP_PROT_SSL3_CLIENT | SP_PROT_TLS1_CLIENT)
#define SP_PROT_SSL3TLS1_SERVERS   (SP_PROT_SSL3_SERVER | SP_PROT_TLS1_SERVER)
#define SP_PROT_SSL3TLS1_X_CLIENTS (SP_PROT_SSL3_CLIENT | SP_PROT_TLS1_X_CLIENT)
#define SP_PROT_SSL3TLS1_X_SERVERS (SP_PROT_SSL3_SERVER | SP_PROT_TLS1_X_SERVER)
#define SP_PROT_TLS1_X_CLIENT      ( SP_PROT_TLS1_0_CLIENT \
                                   | SP_PROT_TLS1_1_CLIENT \
                                   | SP_PROT_TLS1_2_CLIENT \
                                   | SP_PROT_TLS1_3_CLIENT )
#define SP_PROT_TLS1_X_SERVER      ( SP_PROT_TLS1_0_SERVER \
                                   | SP_PROT_TLS1_1_SERVER \
                                   | SP_PROT_TLS1_2_SERVER \
                                   | SP_PROT_TLS1_3_SERVER )
#define SP_PROT_TLS1_1PLUS_CLIENT  ( SP_PROT_TLS1_1_CLIENT \
                                   | SP_PROT_TLS1_2_CLIENT \
                                   | SP_PROT_TLS1_3_CLIENT)
#define SP_PROT_TLS1_1PLUS_SERVER  ( SP_PROT_TLS1_1_SERVER \
                                   | SP_PROT_TLS1_2_SERVER \
                                   | SP_PROT_TLS1_3_SERVER )
#define SP_PROT_DTLS1_X_SERVER     ( SP_PROT_DTLS1_0_SERVER \
                                   | SP_PROT_DTLS1_2_SERVER )
#define SP_PROT_DTLS1_X_CLIENT     ( SP_PROT_DTLS1_0_CLIENT \
                                   | SP_PROT_DTLS1_2_CLIENT )
#define SP_PROT_DTLS1_X            ( SP_PROT_DTLS1_X_SERVER \
                                   | SP_PROT_DTLS1_X_CLIENT )
#define SP_PROT_CLIENTS            (SP_PROT_PCT1_CLIENT | SP_PROT_SSL2_CLIENT | SP_PROT_SSL3_CLIENT \
                                  | SP_PROT_TLS1_CLIENT | SP_PROT_UNI_CLIENT)
#define SP_PROT_SERVERS            (SP_PROT_PCT1_SERVER | SP_PROT_SSL2_SERVER | SP_PROT_SSL3_SERVER \
                                  | SP_PROT_TLS1_SERVER | SP_PROT_UNI_SERVER)
#define SP_PROT_X_CLIENTS          ( SP_PROT_CLIENTS \
                                   | SP_PROT_TLS1_X_CLIENT \
                                   | SP_PROT_DTLS1_X_CLIENT)
#define SP_PROT_X_SERVERS          ( SP_PROT_SERVERS \
                                   | SP_PROT_TLS1_X_SERVER \
                                   | SP_PROT_DTLS1_X_SERVER)

#define SP_PROT_SSL3TLS1           (SP_PROT_SSL3 | SP_PROT_TLS1)
#define SP_PROT_SSL3TLS1_X         (SP_PROT_SSL3 | SP_PROT_TLS1_X)
#define SP_PROT_TLS1_X             (SP_PROT_TLS1_X_CLIENT | SP_PROT_TLS1_X_SERVER)
#define SP_PROT_TLS1_1PLUS         (SP_PROT_TLS1_1PLUS_CLIENT | SP_PROT_TLS1_1PLUS_SERVER)

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
    DWORD dwSessionLifespan;
    DWORD dwFlags;
    DWORD dwCredFormat;
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

#endif /* __WINE_SCHANNEL_H__ */
