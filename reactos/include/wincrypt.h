/*
 * Copyright (C) 2002 Travis Michielsen
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINCRYPT_H
#define __WINE_WINCRYPT_H

/* some typedefs for function parameters */
typedef unsigned int ALG_ID;
typedef unsigned long HCRYPTPROV;
typedef unsigned long HCRYPTKEY;
typedef unsigned long HCRYPTHASH;
typedef void *HCERTSTORE;

/* CSP Structs */

typedef struct _CRYPTOAPI_BLOB {
  DWORD    cbData;
  BYTE*    pbData;
} CRYPT_INTEGER_BLOB,  *PCRYPT_INTEGER_BLOB,
  CRYPT_UINT_BLOB,     *PCRYPT_UINT_BLOB,
  CRYPT_OBJID_BLOB,    *PCRYPT_OBJID_BLOB,
  CERT_NAME_BLOB,      *PCERT_NAME_BLOB,
  CERT_RDN_VALUE_BLOB, *PCERT_RDN_VALUE_BLOB,
  CERT_BLOB,           *PCERT_BLOB,
  CRL_BLOB,            *PCRL_BLOB,
  DATA_BLOB,           *PDATA_BLOB,
  CRYPT_DATA_BLOB,     *PCRYPT_DATA_BLOB,
  CRYPT_HASH_BLOB,     *PCRYPT_HASH_BLOB,
  CRYPT_DIGEST_BLOB,   *PCRYPT_DIGEST_BLOB,
  CRYPT_DER_BLOB,      *PCRYPT_DER_BLOB,
  CRYPT_ATTR_BLOB,     *PCRYPT_ATTR_BLOB;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
  LPSTR            pszObjId;
  CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;

typedef struct _CRYPT_ATTRIBUTE_TYPE_VALUE {
  LPSTR               pszObjId;
  CRYPT_OBJID_BLOB    Value;
} CRYPT_ATTRIBUTE_TYPE_VALUE, *PCRYPT_ATTRIBUTE_TYPE_VALUE;

typedef struct _PUBLICKEYSTRUC {
    BYTE   bType;
    BYTE   bVersion;
    WORD   reserved;
    ALG_ID aiKeyAlg;
} BLOBHEADER, PUBLICKEYSTRUC;

typedef struct _CRYPT_BIT_BLOB {
    DWORD cbData;
    BYTE  *pbData;
    DWORD cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;

typedef struct _CERT_PUBLIC_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER Algorithm;
    CRYPT_BIT_BLOB             PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;

typedef struct _CERT_EXTENSION {
    LPSTR               pszObjId;
    WINBOOL                fCritical;
    CRYPT_OBJID_BLOB    Value;
} CERT_EXTENSION, *PCERT_EXTENSION;

typedef struct _CERT_INFO {
    DWORD                      dwVersion;
    CRYPT_INTEGER_BLOB         SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB             Issuer;
    FILETIME                   NotBefore;
    FILETIME                   NotAfter;
    CERT_NAME_BLOB             Subject;
    CERT_PUBLIC_KEY_INFO       SubjectPublicKeyInfo;
    CRYPT_BIT_BLOB             IssuerUniqueId;
    CRYPT_BIT_BLOB             SubjectUniqueId;
    DWORD                      cExtension;
    PCERT_EXTENSION            rgExtension;
} CERT_INFO, *PCERT_INFO;

typedef struct _CERT_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE       *pbCertEncoded;
    DWORD      cbCertEncoded;
    PCERT_INFO pCertInfo;
    HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

typedef struct _VTableProvStruc {
    DWORD    Version;
    FARPROC  pFuncVerifyImage;
    FARPROC  pFuncReturnhWnd;
    DWORD    dwProvType;
    BYTE    *pbContextInfo;
    DWORD    cbContextInfo;
    LPSTR    pszProvName;
} VTableProvStruc, *PVTableProvStruc;

/* Algorithm IDs */

#define GET_ALG_CLASS(x)                (x & (7 << 13))
#define GET_ALG_TYPE(x)                 (x & (15 << 9))
#define GET_ALG_SID(x)                  (x & (511))

/* Algorithm Classes */
#define ALG_CLASS_ANY                   (0)
#define ALG_CLASS_SIGNATURE             (1 << 13)
#define ALG_CLASS_MSG_ENCRYPT           (2 << 13)
#define ALG_CLASS_DATA_ENCRYPT          (3 << 13)
#define ALG_CLASS_HASH                  (4 << 13)
#define ALG_CLASS_KEY_EXCHANGE          (5 << 13)
/* Algorithm types */
#define ALG_TYPE_ANY                    (0)
#define ALG_TYPE_DSS                    (1 << 9)
#define ALG_TYPE_RSA                    (2 << 9)
#define ALG_TYPE_BLOCK                  (3 << 9)
#define ALG_TYPE_STREAM                 (4 << 9)

/* SIDs */
#define ALG_SID_ANY                     (0)
/* RSA SIDs */
#define ALG_SID_RSA_ANY                 0
#define ALG_SID_RSA_PKCS                1
#define ALG_SID_RSA_MSATWORK            2
#define ALG_SID_RSA_ENTRUST             3
#define ALG_SID_RSA_PGP                 4
/* DSS SIDs */
#define ALG_SID_DSS_ANY                 0
#define ALG_SID_DSS_PKCS                1
#define ALG_SID_DSS_DMS                 2

/* DES SIDs */
#define ALG_SID_DES                     1
#define ALG_SID_3DES			3
#define ALG_SID_DESX			4
#define ALG_SID_IDEA			5
#define ALG_SID_CAST			6
#define ALG_SID_SAFERSK64		7
#define ALD_SID_SAFERSK128		8
/* RC2 SIDs */
#define ALG_SID_RC4                     1
#define ALG_SID_RC2                     2
#define ALG_SID_SEAL                    2
/* Hash SIDs */
#define ALG_SID_MD2                     1
#define ALG_SID_MD4                     2
#define ALG_SID_MD5                     3
#define ALG_SID_SHA                     4
#define ALG_SID_MAC                     5
#define ALG_SID_RIPEMD			6
#define ALG_SID_RIPEMD160		7
#define ALG_SID_SSL3SHAMD5		8

/* Algorithm Definitions */
#define CALG_MD2        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD2)
#define CALG_MD4        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD4)
#define CALG_MD5        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MD5)
#define CALG_SHA        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_SHA)
#define CALG_MAC        (ALG_CLASS_HASH         | ALG_TYPE_ANY    | ALG_SID_MAC)
#define CALG_RSA_SIGN   (ALG_CLASS_SIGNATURE    | ALG_TYPE_RSA    | ALG_SID_RSA_ANY)
#define CALG_DSS_SIGN   (ALG_CLASS_SIGNATURE    | ALG_TYPE_DSS    | ALG_SID_DSS_ANY)
#define CALG_RSA_KEYX   (ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA    | ALG_SID_RSA_ANY)
#define CALG_DES        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK  | ALG_SID_DES)
#define CALG_RC2        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK  | ALG_SID_RC2)
#define CALG_RC4        (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_RC4)
#define CALG_SEAL       (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_SEAL)

/* Provider names */
#define MS_DEF_PROV_A "Microsoft Base Cryptographic Provider v1.0"
#define MS_DEF_PROV_W L"Microsoft Base Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_A "Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_W L"Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_STRONG_PROV_A "Microsoft Strong Cryptographic Provider"
#define MS_STRONG_PROV_W L"Microsoft Strong Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_A "Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SIG_PROV_W L"Microsoft RSA Signature Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_A "Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_RSA_SCHANNEL_PROV_W L"Microsoft RSA SChannel Cryptographic Provider"
#define MS_DEF_DSS_PROV_A "Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_PROV_W L"Microsoft Base DSS Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_A "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DSS_DH_PROV_W L"Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_A "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_ENH_DSS_DH_PROV_W L"Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_A "Microsoft DH SChannel Cryptographic Provider"
#define MS_DEF_DH_SCHANNEL_PROV_W L"Microsoft DH SChannel Cryptographic Provider"
#define MS_SCARD_PROV_A "Microsoft Base Smart Card Cryptographic Provider"
#define MS_SCARD_PROV_W L"Microsoft Base Smart Card Cryptographic Provider"

/* Key Specs*/
#define AT_KEYEXCHANGE          1
#define AT_SIGNATURE            2

/* Provider Types */
#define PROV_RSA_FULL             1
#define PROV_RSA_SIG              2
#define PROV_DSS                  3
#define PROV_FORTEZZA             4
#define PROV_MS_EXCHANGE          5
#define PROV_SSL                  6
#define PROV_RSA_SCHANNEL         12
#define PROV_DSS_DH               13
#define PROV_EC_ECDSA_SIG         14
#define PROV_EC_ECNRA_SIG         15
#define PROV_EC_ECDSA_FULL        16
#define PROV_EC_ECNRA_FULL        17
#define PROV_DH_SCHANNEL          18
#define PROV_SPYRUS_LYNKS         20
#define PROV_RNG                  21
#define PROV_INTEL_SEC            22
#define PROV_REPLACE_OWF          23
#define PROV_RSA_AES              24

/* FLAGS Section */

/* Provider Parameters */
#define PP_ENUMALGS             1
#define PP_ENUMCONTAINERS       2
#define PP_IMPTYPE              3
#define PP_NAME                 4
#define PP_VERSION              5
#define PP_CONTAINER            6

#define CRYPT_FIRST             1
#define CRYPT_NEXT              2

#define CRYPT_IMPL_HARDWARE     1
#define CRYPT_IMPL_SOFTWARE     2
#define CRYPT_IMPL_MIXED        3
#define CRYPT_IMPL_UNKNOWN      4

/* CryptAcquireContext */
#define CRYPT_VERIFYCONTEXT       0xF0000000
#define CRYPT_NEWKEYSET           0x00000008
#define CRYPT_MACHINE_KEYSET      0x00000000
#define CRYPT_DELETEKEYSET        0x00000010
#define CRYPT_SILENT              0x00000000

/* Crypt{Get|Set}Provider */
#define CRYPT_MACHINE_DEFAULT     0x00000001
#define CRYPT_USER_DEFAULT        0x00000002
#define CRYPT_DELETE_DEFAULT      0x00000004

/* Crypt{Get/Set}ProvParam */
#define PP_CLIENT_HWND          1
#define PP_ENUMALGS             1
#define PP_ENUMCONTAINERS       2
#define PP_IMPTYPE              3
#define PP_NAME                 4
#define PP_VERSION              5
#define PP_CONTAINER            6
#define PP_CHANGE_PASSWORD      7
#define PP_KEYSET_SEC_DESCR     8
#define PP_KEY_TYPE_SUBTYPE     10
#define PP_CONTEXT_INFO         11
#define PP_KEYEXCHANGE_KEYSIZE  12
#define PP_SIGNATURE_KEYSIZE    13
#define PP_KEYEXCHANGE_ALG      14
#define PP_SIGNATURE_ALG        15
#define PP_PROVTYPE             16
#define PP_KEYSTORAGE           17
#define PP_SYM_KEYSIZE          19
#define PP_SESSION_KEYSIZE      20
#define PP_UI_PROMPT            21
#define PP_ENUMALGS_EX          22
#define PP_DELETEKEY            24
#define PP_ENUMMANDROOTS        25
#define PP_ENUMELECTROOTS       26
#define PP_KEYSET_TYPE          27
#define PP_ADMIN_PIN            31
#define PP_KEYEXCHANGE_PIN      32
#define PP_SIGNATURE_PIN        33
#define PP_SIG_KEYSIZE_INC      34
#define PP_KEYX_KEYSIZE_INC     35
#define PP_UNIQUE_CONTAINER     36
#define PP_SGC_INFO             37
#define PP_USE_HARDWARE_RNG     38
#define PP_KEYSPEC              39
#define PP_ENUMEX_SIGNING_PROT  40

/* CryptSignHash/CryptVerifySignature */
#define CRYPT_NOHASHOID         0x00000001
#define CRYPT_TYPE2_FORMAT      0x00000002
#define CRYPT_X931_FORMAT       0x00000004

/* Crypt*Key */
#define CRYPT_EXPORTABLE        0x00000001
#define CRYPT_USER_PROTECTED    0x00000002
#define CRYPT_CREATE_SALT       0x00000004
#define CRYPT_UPDATE_KEY        0x00000008
#define CRYPT_NO_SALT           0x00000010
#define CRYPT_PREGEN            0x00000040
#define CRYPT_ARCHIVABLE        0x00004000
#define CRYPT_SSL2_FALLBACK     0x00000002
#define CRYPT_DESTROYKEY        0x00000004
#define CRYPT_OAEP              0x00000040

/* Blob Types */
#define SIMPLEBLOB              0x1
#define PUBLICKEYBLOB           0x6
#define PRIVATEKEYBLOB          0x7
#define PLAINTEXTKEYBLOB        0x8
#define OPAQUEKEYBLOB           0x9
#define PUBLICKEYBLOBEX         0xA
#define SYMMETRICWRAPKEYBLOB    0xB

/* function declarations */
/* advapi32.dll */
WINBOOL STDCALL CryptAcquireContextA(HCRYPTPROV *phProv, LPCSTR pszContainer,
				   LPCSTR pszProvider, DWORD dwProvType,
				   DWORD dwFlags);
WINBOOL STDCALL CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
WINBOOL STDCALL CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer);
WINBOOL STDCALL CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags);
WINBOOL STDCALL CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash);
WINBOOL STDCALL CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
WINBOOL STDCALL CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey);
WINBOOL STDCALL CryptDestroyHash (HCRYPTHASH hHash);
WINBOOL STDCALL CryptDestroyKey (HCRYPTKEY hKey);
WINBOOL STDCALL CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey);
WINBOOL STDCALL CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash);
WINBOOL STDCALL CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen);
WINBOOL STDCALL CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName);
WINBOOL STDCALL CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName);
WINBOOL STDCALL CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName);
WINBOOL STDCALL CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName);
WINBOOL STDCALL CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen);
WINBOOL STDCALL CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey);
WINBOOL STDCALL CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
WINBOOL STDCALL CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
WINBOOL STDCALL CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags);
WINBOOL STDCALL CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName);
WINBOOL STDCALL CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName);
WINBOOL STDCALL CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey);
WINBOOL STDCALL CryptHashData (HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags);
WINBOOL STDCALL CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags);
WINBOOL STDCALL CryptImportKey (HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey);
WINBOOL STDCALL CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags);
WINBOOL STDCALL CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
WINBOOL STDCALL CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen);
WINBOOL STDCALL CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
WINBOOL STDCALL CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
WINBOOL STDCALL CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType);
WINBOOL STDCALL CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType);
WINBOOL STDCALL CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags);
WINBOOL STDCALL CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags);
WINBOOL STDCALL CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags);
WINBOOL STDCALL CryptVerifySignatureA (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags);
WINBOOL STDCALL CryptVerifySignatureW (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags);
#ifndef _DISABLE_TIDENTS
#ifdef UNICODE
#define CryptAcquireContext CryptAcquireContextW
#define CryptEnumProviders CryptEnumProvidersW
#define CryptEnumProviderTypes CryptEnumProviderTypesW
#define CryptGetDefaultProvider CryptGetDefaultProviderW
#define CryptSignHash CryptSignHashW
#define CryptSetProvider CryptSetProviderW
#define CryptSetProviderEx CryptSetProviderExW
#define CryptVerifySignature CryptVerifySignatureW
#define MS_DEF_PROV MS_DEF_PROV_W
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_W
#define MS_STRONG_PROV MS_STRONG_PROV_W
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_W
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_W
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_W
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_W
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_W
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_W
#define MS_SCARD_PROV MS_SCARD_PROV_W
#else
#define CryptAcquireContext CryptAcquireContextA
#define CryptEnumProviders CryptEnumProvidersA
#define CryptEnumProviderTypes CryptEnumProviderTypesA
#define CryptGetDefaultProvider CryptGetDefaultProviderA
#define CryptSignHash CryptSignHashA
#define CryptSetProvider CryptSetProviderA
#define CryptSetProviderEx CryptSetProviderExA
#define CryptVerifySignature CryptVerifySignatureA
#define MS_DEF_PROV MS_DEF_PROV_A
#define MS_ENHANCED_PROV MS_ENHANCED_PROV_A
#define MS_STRONG_PROV MS_STRONG_PROV_A
#define MS_DEF_RSA_SIG_PROV MS_DEF_RSA_SIG_PROV_A
#define MS_DEF_RSA_SCHANNEL_PROV MS_DEF_RSA_SCHANNEL_PROV_A
#define MS_DEF_DSS_PROV MS_DEF_DSS_PROV_A
#define MS_DEF_DSS_DH_PROV MS_DEF_DSS_DH_PROV_A
#define MS_ENH_DSS_DH_PROV MS_ENH_DSS_DH_PROV_A
#define MS_DEF_DH_SCHANNEL_PROV MS_DEF_DH_SCHANNEL_PROV_A
#define MS_SCARD_PROV MS_SCARD_PROV_A
#endif
#endif

#endif
