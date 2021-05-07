/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_BCRYPT_H
#define __WINE_BCRYPT_H

#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifdef __REACTOS__
#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif
#define STATUS_AUTH_TAG_MISMATCH         ((NTSTATUS) 0xC000A002)    //remove when ntstatus.h is winesynced
#else
#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define BCRYPT_ALGORITHM_NAME       L"AlgorithmName"
#define BCRYPT_AUTH_TAG_LENGTH      L"AuthTagLength"
#define BCRYPT_BLOCK_LENGTH         L"BlockLength"
#define BCRYPT_BLOCK_SIZE_LIST      L"BlockSizeList"
#define BCRYPT_CHAINING_MODE        L"ChainingMode"
#define BCRYPT_EFFECTIVE_KEY_LENGTH L"EffectiveKeyLength"
#define BCRYPT_HASH_BLOCK_LENGTH    L"HashBlockLength"
#define BCRYPT_HASH_LENGTH          L"HashDigestLength"
#define BCRYPT_HASH_OID_LIST        L"HashOIDList"
#define BCRYPT_KEY_LENGTH           L"KeyLength"
#define BCRYPT_KEY_LENGTHS          L"KeyLengths"
#define BCRYPT_KEY_OBJECT_LENGTH    L"KeyObjectLength"
#define BCRYPT_KEY_STRENGTH         L"KeyStrength"
#define BCRYPT_OBJECT_LENGTH        L"ObjectLength"
#define BCRYPT_PADDING_SCHEMES      L"PaddingSchemes"
#define BCRYPT_PROVIDER_HANDLE      L"ProviderHandle"
#define BCRYPT_SIGNATURE_LENGTH     L"SignatureLength"

#define BCRYPT_OPAQUE_KEY_BLOB      L"OpaqueKeyBlob"
#define BCRYPT_KEY_DATA_BLOB        L"KeyDataBlob"
#define BCRYPT_AES_WRAP_KEY_BLOB    L"Rfc3565KeyWrapBlob"
#define BCRYPT_ECCPUBLIC_BLOB       L"ECCPUBLICBLOB"
#define BCRYPT_ECCPRIVATE_BLOB      L"ECCPRIVATEBLOB"
#define BCRYPT_RSAPUBLIC_BLOB       L"RSAPUBLICBLOB"
#define BCRYPT_RSAPRIVATE_BLOB      L"RSAPRIVATEBLOB"
#define BCRYPT_DSA_PUBLIC_BLOB      L"DSAPUBLICBLOB"
#define BCRYPT_DSA_PRIVATE_BLOB     L"DSAPRIVATEBLOB"
#define BCRYPT_PUBLIC_KEY_BLOB      L"PUBLICBLOB"
#define BCRYPT_PRIVATE_KEY_BLOB     L"PRIVATEBLOB"
#define LEGACY_DSA_PUBLIC_BLOB      L"CAPIDSAPUBLICBLOB"
#define LEGACY_DSA_PRIVATE_BLOB     L"CAPIDSAPRIVATEBLOB"
#define LEGACY_DSA_V2_PUBLIC_BLOB   L"V2CAPIDSAPUBLICBLOB"
#define LEGACY_DSA_V2_PRIVATE_BLOB  L"V2CAPIDSAPRIVATEBLOB"

#define MS_PRIMITIVE_PROVIDER       L"Microsoft Primitive Provider"
#define MS_PLATFORM_CRYPTO_PROVIDER L"Microsoft Platform Crypto Provider"

#define BCRYPT_3DES_ALGORITHM       L"3DES"
#define BCRYPT_AES_ALGORITHM        L"AES"
#define BCRYPT_DES_ALGORITHM        L"DES"
#define BCRYPT_DSA_ALGORITHM        L"DSA"
#define BCRYPT_ECDH_P256_ALGORITHM  L"ECDH_P256"
#define BCRYPT_ECDSA_P256_ALGORITHM L"ECDSA_P256"
#define BCRYPT_ECDSA_P384_ALGORITHM L"ECDSA_P384"
#define BCRYPT_ECDSA_P521_ALGORITHM L"ECDSA_P521"
#define BCRYPT_MD2_ALGORITHM        L"MD2"
#define BCRYPT_MD4_ALGORITHM        L"MD4"
#define BCRYPT_MD5_ALGORITHM        L"MD5"
#define BCRYPT_RC2_ALGORITHM        L"RC2"
#define BCRYPT_RC4_ALGORITHM        L"RC4"
#define BCRYPT_RNG_ALGORITHM        L"RNG"
#define BCRYPT_RSA_ALGORITHM        L"RSA"
#define BCRYPT_RSA_SIGN_ALGORITHM   L"RSA_SIGN"
#define BCRYPT_SHA1_ALGORITHM       L"SHA1"
#define BCRYPT_SHA256_ALGORITHM     L"SHA256"
#define BCRYPT_SHA384_ALGORITHM     L"SHA384"
#define BCRYPT_SHA512_ALGORITHM     L"SHA512"

#define BCRYPT_CHAIN_MODE_NA        L"ChainingModeN/A"
#define BCRYPT_CHAIN_MODE_CBC       L"ChainingModeCBC"
#define BCRYPT_CHAIN_MODE_ECB       L"ChainingModeECB"
#define BCRYPT_CHAIN_MODE_CFB       L"ChainingModeCFB"
#define BCRYPT_CHAIN_MODE_CCM       L"ChainingModeCCM"
#define BCRYPT_CHAIN_MODE_GCM       L"ChainingModeGCM"

#define BCRYPT_KDF_HASH             L"HASH"
#define BCRYPT_KDF_HMAC             L"HMAC"
#define BCRYPT_KDF_TLS_PRF          L"TLS_PRF"
#define BCRYPT_KDF_SP80056A_CONCAT  L"SP800_56A_CONCAT"
#define BCRYPT_KDF_RAW_SECRET       L"TRUNCATE"
#else
static const WCHAR BCRYPT_ALGORITHM_NAME[] = {'A','l','g','o','r','i','t','h','m','N','a','m','e',0};
static const WCHAR BCRYPT_AUTH_TAG_LENGTH[] = {'A','u','t','h','T','a','g','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_BLOCK_LENGTH[] = {'B','l','o','c','k','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_BLOCK_SIZE_LIST[] = {'B','l','o','c','k','S','i','z','e','L','i','s','t',0};
static const WCHAR BCRYPT_CHAINING_MODE[] = {'C','h','a','i','n','i','n','g','M','o','d','e',0};
static const WCHAR BCRYPT_EFFECTIVE_KEY_LENGTH[] = {'E','f','f','e','c','t','i','v','e','K','e','y','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_HASH_BLOCK_LENGTH[] = {'H','a','s','h','B','l','o','c','k','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_HASH_LENGTH[] = {'H','a','s','h','D','i','g','e','s','t','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_HASH_OID_LIST[] = {'H','a','s','h','O','I','D','L','i','s','t',0};
static const WCHAR BCRYPT_KEY_LENGTH[] = {'K','e','y','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_KEY_LENGTHS[] = {'K','e','y','L','e','n','g','t','h','s',0};
static const WCHAR BCRYPT_KEY_OBJECT_LENGTH[] = {'K','e','y','O','b','j','e','c','t','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_KEY_STRENGTH[] = {'K','e','y','S','t','r','e','n','g','t','h',0};
static const WCHAR BCRYPT_OBJECT_LENGTH[] = {'O','b','j','e','c','t','L','e','n','g','t','h',0};
static const WCHAR BCRYPT_PADDING_SCHEMES[] = {'P','a','d','d','i','n','g','S','c','h','e','m','e','s',0};
static const WCHAR BCRYPT_PROVIDER_HANDLE[] = {'P','r','o','v','i','d','e','r','H','a','n','d','l','e',0};
static const WCHAR BCRYPT_SIGNATURE_LENGTH[] = {'S','i','g','n','a','t','u','r','e','L','e','n','g','t','h',0};

static const WCHAR BCRYPT_OPAQUE_KEY_BLOB[] = {'O','p','a','q','u','e','K','e','y','B','l','o','b',0};
static const WCHAR BCRYPT_KEY_DATA_BLOB[] = {'K','e','y','D','a','t','a','B','l','o','b',0};
static const WCHAR BCRYPT_AES_WRAP_KEY_BLOB[] = {'R','f','c','3','5','6','5','K','e','y','W','r','a','p','B','l','o','b',0};
static const WCHAR BCRYPT_ECCPUBLIC_BLOB[] = {'E','C','C','P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR BCRYPT_ECCPRIVATE_BLOB[] = {'E','C','C','P','R','I','V','A','T','E','B','L','O','B',0};
static const WCHAR BCRYPT_RSAPUBLIC_BLOB[] = {'R','S','A','P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR BCRYPT_RSAPRIVATE_BLOB[] = {'R','S','A','P','R','I','V','A','T','E','B','L','O','B',0};
static const WCHAR BCRYPT_DSA_PUBLIC_BLOB[] = {'D','S','A','P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR BCRYPT_DSA_PRIVATE_BLOB[] = {'D','S','A','P','R','I','V','A','T','E','B','L','O','B',0};
static const WCHAR BCRYPT_PUBLIC_KEY_BLOB[] = {'P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR BCRYPT_PRIVATE_KEY_BLOB[] = {'P','R','I','V','A','T','E','B','L','O','B',0};
static const WCHAR LEGACY_DSA_PUBLIC_BLOB[] = {'C','A','P','I','D','S','A','P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR LEGACY_DSA_PRIVATE_BLOB[] = {'C','A','P','I','D','S','A','P','R','I','V','A','T','E','B','L','O','B',0};
static const WCHAR LEGACY_DSA_V2_PUBLIC_BLOB[] = {'V','2','C','A','P','I','D','S','A','P','U','B','L','I','C','B','L','O','B',0};
static const WCHAR LEGACY_DSA_V2_PRIVATE_BLOB[] = {'V','2','C','A','P','I','D','S','A','P','R','I','V','A','T','E','B','L','O','B',0};

static const WCHAR MS_PRIMITIVE_PROVIDER[] = \
{'M','i','c','r','o','s','o','f','t',' ','P','r','i','m','i','t','i','v','e',' ','P','r','o','v','i','d','e','r',0};
static const WCHAR MS_PLATFORM_CRYPTO_PROVIDER[] = \
{'M','i','c','r','o','s','o','f','t',' ','P','l','a','t','f','o','r','m',' ','C','r','y','p','t','o',' ','P','r','o','v','i','d','e','r',0};

static const WCHAR BCRYPT_3DES_ALGORITHM[] = {'3','D','E','S',0};
static const WCHAR BCRYPT_AES_ALGORITHM[] = {'A','E','S',0};
static const WCHAR BCRYPT_DES_ALGORITHM[] = {'D','E','S',0};
static const WCHAR BCRYPT_DSA_ALGORITHM[] = {'D','S','A',0};
static const WCHAR BCRYPT_ECDH_P256_ALGORITHM[] = {'E','C','D','H','_','P','2','5','6',0};
static const WCHAR BCRYPT_ECDSA_P256_ALGORITHM[] = {'E','C','D','S','A','_','P','2','5','6',0};
static const WCHAR BCRYPT_ECDSA_P384_ALGORITHM[] = {'E','C','D','S','A','_','P','3','8','4',0};
static const WCHAR BCRYPT_ECDSA_P521_ALGORITHM[] = {'E','C','D','S','A','_','P','5','2','1',0};
static const WCHAR BCRYPT_MD2_ALGORITHM[] = {'M','D','2',0};
static const WCHAR BCRYPT_MD4_ALGORITHM[] = {'M','D','4',0};
static const WCHAR BCRYPT_MD5_ALGORITHM[] = {'M','D','5',0};
static const WCHAR BCRYPT_RC2_ALGORITHM[] = {'R','C','2',0};
static const WCHAR BCRYPT_RC4_ALGORITHM[] = {'R','C','4',0};
static const WCHAR BCRYPT_RNG_ALGORITHM[] = {'R','N','G',0};
static const WCHAR BCRYPT_RSA_ALGORITHM[] = {'R','S','A',0};
static const WCHAR BCRYPT_RSA_SIGN_ALGORITHM[] = {'R','S','A','_','S','I','G','N',0};
static const WCHAR BCRYPT_SHA1_ALGORITHM[] = {'S','H','A','1',0};
static const WCHAR BCRYPT_SHA256_ALGORITHM[] = {'S','H','A','2','5','6',0};
static const WCHAR BCRYPT_SHA384_ALGORITHM[] = {'S','H','A','3','8','4',0};
static const WCHAR BCRYPT_SHA512_ALGORITHM[] = {'S','H','A','5','1','2',0};

static const WCHAR BCRYPT_CHAIN_MODE_NA[] = {'C','h','a','i','n','i','n','g','M','o','d','e','N','/','A',0};
static const WCHAR BCRYPT_CHAIN_MODE_CBC[] = {'C','h','a','i','n','i','n','g','M','o','d','e','C','B','C',0};
static const WCHAR BCRYPT_CHAIN_MODE_ECB[] = {'C','h','a','i','n','i','n','g','M','o','d','e','E','C','B',0};
static const WCHAR BCRYPT_CHAIN_MODE_CFB[] = {'C','h','a','i','n','i','n','g','M','o','d','e','C','F','B',0};
static const WCHAR BCRYPT_CHAIN_MODE_CCM[] = {'C','h','a','i','n','i','n','g','M','o','d','e','C','C','M',0};
static const WCHAR BCRYPT_CHAIN_MODE_GCM[] = {'C','h','a','i','n','i','n','g','M','o','d','e','G','C','M',0};

static const WCHAR BCRYPT_KDF_HASH[] = {'H','A','S','H',0};
static const WCHAR BCRYPT_KDF_HMAC[] = {'H','M','A','C',0};
static const WCHAR BCRYPT_KDF_TLS_PRF[] = {'T','L','S','_','P','R','F',0};
static const WCHAR BCRYPT_KDF_SP80056A_CONCAT[] = {'S','P','8','0','0','_','5','6','A','_','C','O','N','C','A','T',0};
static const WCHAR BCRYPT_KDF_RAW_SECRET[] = {'T','R','U','N','C','A','T','E',0};
#endif

#define BCRYPT_ECDSA_PUBLIC_P256_MAGIC  0x31534345
#define BCRYPT_ECDSA_PRIVATE_P256_MAGIC 0x32534345
#define BCRYPT_ECDSA_PUBLIC_P384_MAGIC  0x33534345
#define BCRYPT_ECDSA_PRIVATE_P384_MAGIC 0x34534345
#define BCRYPT_ECDSA_PUBLIC_P521_MAGIC  0x35534345
#define BCRYPT_ECDSA_PRIVATE_P521_MAGIC 0x36534345

#define BCRYPT_ECDH_PUBLIC_P256_MAGIC  0x314b4345
#define BCRYPT_ECDH_PRIVATE_P256_MAGIC 0x324b4345
#define BCRYPT_ECDH_PUBLIC_P384_MAGIC  0x334b4345
#define BCRYPT_ECDH_PRIVATE_P384_MAGIC 0x344b4345
#define BCRYPT_ECDH_PUBLIC_P521_MAGIC  0x354b4345
#define BCRYPT_ECDH_PRIVATE_P521_MAGIC 0x364b4345

#define BCRYPT_CIPHER_OPERATION                 0x00000001
#define BCRYPT_HASH_OPERATION                   0x00000002
#define BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION  0x00000004
#define BCRYPT_SECRET_AGREEMENT_OPERATION       0x00000008
#define BCRYPT_SIGNATURE_OPERATION              0x00000010
#define BCRYPT_RNG_OPERATION                    0x00000020

#define BCRYPT_CIPHER_INTERFACE                 0x00000001
#define BCRYPT_HASH_INTERFACE                   0x00000002
#define BCRYPT_ASYMMETRIC_ENCRYPTION_INTERFACE  0x00000003
#define BCRYPT_SECRET_AGREEMENT_INTERFACE       0x00000004
#define BCRYPT_SIGNATURE_INTERFACE              0x00000005
#define BCRYPT_RNG_INTERFACE                    0x00000006
#define BCRYPT_KEY_DERIVATION_INTERFACE         0x00000007

#define BCRYPT_SUPPORTED_PAD_ROUTER     0x00000001
#define BCRYPT_SUPPORTED_PAD_PKCS1_ENC  0x00000002
#define BCRYPT_SUPPORTED_PAD_PKCS1_SIG  0x00000004
#define BCRYPT_SUPPORTED_PAD_OAEP       0x00000008
#define BCRYPT_SUPPORTED_PAD_PSS        0x00000010

typedef struct _BCRYPT_ALGORITHM_IDENTIFIER
{
    LPWSTR pszName;
    ULONG  dwClass;
    ULONG  dwFlags;
} BCRYPT_ALGORITHM_IDENTIFIER;

typedef struct __BCRYPT_KEY_LENGTHS_STRUCT
{
    ULONG dwMinLength;
    ULONG dwMaxLength;
    ULONG dwIncrement;
} BCRYPT_KEY_LENGTHS_STRUCT, BCRYPT_AUTH_TAG_LENGTHS_STRUCT;

typedef struct _BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO
{
    ULONG cbSize;
    ULONG dwInfoVersion;
    UCHAR *pbNonce;
    ULONG cbNonce;
    UCHAR *pbAuthData;
    ULONG cbAuthData;
    UCHAR *pbTag;
    ULONG cbTag;
    UCHAR *pbMacContext;
    ULONG cbMacContext;
    ULONG cbAAD;
    ULONGLONG cbData;
    ULONG dwFlags;
} BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO, *PBCRYPT_AUTHENTICATED_CIPHER_MODE_INFO;

typedef struct _BCRYPT_ECCKEY_BLOB
{
    ULONG dwMagic;
    ULONG cbKey;
} BCRYPT_ECCKEY_BLOB, *PBCRYPT_ECCKEY_BLOB;

#define BCRYPT_RSAPUBLIC_MAGIC      0x31415352
#define BCRYPT_RSAPRIVATE_MAGIC     0x32415352
#define BCRYPT_RSAFULLPRIVATE_MAGIC 0x33415352

typedef struct _BCRYPT_RSAKEY_BLOB
{
    ULONG Magic;
    ULONG BitLength;
    ULONG cbPublicExp;
    ULONG cbModulus;
    ULONG cbPrime1;
    ULONG cbPrime2;
} BCRYPT_RSAKEY_BLOB;

typedef struct _BCRYPT_PKCS1_PADDING_INFO
{
    LPCWSTR pszAlgId;
} BCRYPT_PKCS1_PADDING_INFO;

#define BCRYPT_PAD_NONE                     0x00000001
#define BCRYPT_PAD_PKCS1                    0x00000002
#define BCRYPT_PAD_OAEP                     0x00000004
#define BCRYPT_PAD_PSS                      0x00000008
#define BCRYPT_PAD_PKCS1_OPTIONAL_HASH_OID  0x00000010

#define BCRYPT_DSA_PUBLIC_MAGIC     0x42505344
#define BCRYPT_DSA_PRIVATE_MAGIC    0x56505344

typedef struct _BCRYPT_DSA_KEY_BLOB
{
    ULONG dwMagic;
    ULONG cbKey;
    UCHAR Count[4];
    UCHAR Seed[20];
    UCHAR q[20];
} BCRYPT_DSA_KEY_BLOB, *PBCRYPT_DSA_KEY_BLOB;

#define BCRYPT_DSA_PUBLIC_MAGIC_V2  0x32425044
#define BCRYPT_DSA_PRIVATE_MAGIC_V2 0x32565044

typedef enum
{
    DSA_HASH_ALGORITHM_SHA1,
    DSA_HASH_ALGORITHM_SHA256,
    DSA_HASH_ALGORITHM_SHA512
} HASHALGORITHM_ENUM;

typedef enum
{
    DSA_FIPS186_2,
    DSA_FIPS186_3
} DSAFIPSVERSION_ENUM;

typedef struct _BCRYPT_DSA_KEY_BLOB_V2
{
    ULONG               dwMagic;
    ULONG               cbKey;
    HASHALGORITHM_ENUM  hashAlgorithm;
    DSAFIPSVERSION_ENUM standardVersion;
    ULONG               cbSeedLength;
    ULONG               cbGroupSize;
    UCHAR               Count[4];
} BCRYPT_DSA_KEY_BLOB_V2, *PBCRYPT_DSA_KEY_BLOB_V2;

#define BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO_VERSION 1

#define BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG 0x00000001
#define BCRYPT_AUTH_MODE_IN_PROGRESS_FLAG 0x00000002

typedef struct _CRYPT_INTERFACE_REG
{
    ULONG dwInterface;
    ULONG dwFlags;
    ULONG cFunctions;
    PWSTR *rgpszFunctions;
} CRYPT_INTERFACE_REG, *PCRYPT_INTERFACE_REG;

typedef struct _CRYPT_IMAGE_REG
{
    PWSTR pszImage;
    ULONG cInterfaces;
    PCRYPT_INTERFACE_REG *rgpInterfaces;
} CRYPT_IMAGE_REG, *PCRYPT_IMAGE_REG;

typedef struct _CRYPT_PROVIDER_REG
{
    ULONG cAliases;
    PWSTR *rgpszAliases;
    PCRYPT_IMAGE_REG pUM;
    PCRYPT_IMAGE_REG pKM;
} CRYPT_PROVIDER_REG, *PCRYPT_PROVIDER_REG;

typedef struct _BCRYPT_KEY_DATA_BLOB_HEADER
{
    ULONG dwMagic;
    ULONG dwVersion;
    ULONG cbKeyData;
} BCRYPT_KEY_DATA_BLOB_HEADER, *PBCRYPT_KEY_DATA_BLOB_HEADER;

#define KDF_HASH_ALGORITHM 0x00000000
#define KDF_SECRET_PREPEND 0x00000001
#define KDF_SECRET_APPEND  0x00000002

typedef struct _BCryptBuffer
{
    ULONG cbBuffer;
    ULONG BufferType;
    void  *pvBuffer;
} BCryptBuffer, *PBCryptBuffer;

#define BCRYPTBUFFER_VERSION        0

typedef struct _BCryptBufferDesc
{
    ULONG ulVersion;
    ULONG cBuffers;
    PBCryptBuffer pBuffers;
} BCryptBufferDesc, *PBCryptBufferDesc;

#define BCRYPT_KEY_DATA_BLOB_MAGIC    0x4d42444b
#define BCRYPT_KEY_DATA_BLOB_VERSION1 1

typedef PVOID BCRYPT_ALG_HANDLE;
typedef PVOID BCRYPT_KEY_HANDLE;
typedef PVOID BCRYPT_HANDLE;
typedef PVOID BCRYPT_HASH_HANDLE;
typedef PVOID BCRYPT_SECRET_HANDLE;

/* Flags for BCryptGenRandom */
#define BCRYPT_RNG_USE_ENTROPY_IN_BUFFER 0x00000001
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG  0x00000002

/* Flags for BCryptOpenAlgorithmProvider */
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008

/* Flags for BCryptEncrypt/BCryptDecrypt */
#define BCRYPT_BLOCK_PADDING        0x00000001

/* Flags for BCryptCreateHash */
#define BCRYPT_HASH_REUSABLE_FLAG   0x00000020

#define CRYPT_LOCAL     0x00000001
#define CRYPT_DOMAIN    0x00000002

typedef struct _CRYPT_CONTEXT_FUNCTIONS
{
    ULONG cFunctions;
    WCHAR **rgpszFunctions;
} CRYPT_CONTEXT_FUNCTIONS, *PCRYPT_CONTEXT_FUNCTIONS;

NTSTATUS WINAPI BCryptAddContextFunction(ULONG, LPCWSTR, ULONG, LPCWSTR, ULONG);
NTSTATUS WINAPI BCryptCloseAlgorithmProvider(BCRYPT_ALG_HANDLE, ULONG);
NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDecrypt(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptDeriveKey(BCRYPT_SECRET_HANDLE, LPCWSTR, BCryptBufferDesc*, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptDeriveKeyCapi(BCRYPT_HASH_HANDLE, BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDeriveKeyPBKDF2(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, ULONGLONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptDestroyHash(BCRYPT_HASH_HANDLE);
NTSTATUS WINAPI BCryptDestroyKey(BCRYPT_KEY_HANDLE);
NTSTATUS WINAPI BCryptDestroySecret(BCRYPT_SECRET_HANDLE);
NTSTATUS WINAPI BCryptDuplicateHash(BCRYPT_HASH_HANDLE, BCRYPT_HASH_HANDLE *, UCHAR *, ULONG, ULONG);
NTSTATUS WINAPI BCryptDuplicateKey(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptEncrypt(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG, ULONG *, BCRYPT_ALGORITHM_IDENTIFIER **, ULONG);
NTSTATUS WINAPI BCryptEnumContextFunctions(ULONG, const WCHAR *, ULONG, ULONG *, CRYPT_CONTEXT_FUNCTIONS **);
NTSTATUS WINAPI BCryptExportKey(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptFinalizeKeyPair(BCRYPT_KEY_HANDLE, ULONG);
NTSTATUS WINAPI BCryptFinishHash(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
void     WINAPI BCryptFreeBuffer(void *);
NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenerateKeyPair(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, ULONG, ULONG);
NTSTATUS WINAPI BCryptGenerateSymmetricKey(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *);
NTSTATUS WINAPI BCryptGetProperty(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptHash(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, PUCHAR, ULONG, PUCHAR, ULONG);
NTSTATUS WINAPI BCryptHashData(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptImportKey(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptImportKeyPair(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, UCHAR *, ULONG, ULONG);
NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);
NTSTATUS WINAPI BCryptRemoveContextFunction(ULONG, LPCWSTR, ULONG, LPCWSTR);
NTSTATUS WINAPI BCryptSecretAgreement(BCRYPT_KEY_HANDLE, BCRYPT_KEY_HANDLE, BCRYPT_SECRET_HANDLE *, ULONG);
NTSTATUS WINAPI BCryptSetProperty(BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG);
NTSTATUS WINAPI BCryptSignHash(BCRYPT_KEY_HANDLE, void *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG *, ULONG);
NTSTATUS WINAPI BCryptVerifySignature(BCRYPT_KEY_HANDLE, void *, UCHAR *, ULONG, UCHAR *, ULONG, ULONG);

#endif  /* __WINE_BCRYPT_H */
