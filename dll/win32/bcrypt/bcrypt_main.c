/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#ifdef __REACTOS__
#define WIN32_NO_STATUS
#endif
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "wincrypt.h"
#include "winternl.h"
#include "bcrypt.h"

#include "symcrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

SYMCRYPT_ENVIRONMENT_DEFS( WindowsUsermodeWin8_1nLater );

#define MAGIC_DSS1 ('D' | ('S' << 8) | ('S' << 16) | ('1' << 24))
#define MAGIC_DSS2 ('D' | ('S' << 8) | ('S' << 16) | ('2' << 24))

#define MAGIC_ALG  (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
#define MAGIC_KEY  (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
#define MAGIC_SECRET (('S' << 24) | ('C' << 16) | ('R' << 8) | 'T')
struct object
{
    ULONG magic;
};

enum alg_id
{
    /* cipher */
    ALG_ID_3DES,
    ALG_ID_CHACHA20_POLY1305,
    ALG_ID_AES,
    ALG_ID_AES_GMAC,
    ALG_ID_RC4,

    /* hash */
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_SHA1,
    ALG_ID_MD5,
    ALG_ID_MD4,
    ALG_ID_MD2,

    /* asymmetric encryption */
    ALG_ID_RSA,

    /* secret agreement */
    ALG_ID_DH,
    ALG_ID_ECDH,
    ALG_ID_ECDH_P256,
    ALG_ID_ECDH_P384,
    ALG_ID_ECDH_P521,

    /* signature */
    ALG_ID_RSA_SIGN,
    ALG_ID_ECDSA,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
    ALG_ID_ECDSA_P521,
    ALG_ID_DSA,

    /* rng */
    ALG_ID_RNG,

    /* key derivation */
    ALG_ID_PBKDF2,
    ALG_ID_TLS1_1_KDF,
    ALG_ID_TLS1_2_KDF,
    ALG_ID_HKDF,
};

#define HASH_FLAG_HMAC      0x01
#define HASH_FLAG_REUSABLE  0x02
struct hash
{
    struct object hdr;
    enum alg_id   alg_id;
    ULONG         len;
    ULONG         flags;
    UCHAR        *secret;
    ULONG         secret_len;

    const SYMCRYPT_HASH       *desc;
    SYMCRYPT_HASH_STATE        state;
    SYMCRYPT_HMAC_EXPANDED_KEY hmac;
    SYMCRYPT_HMAC_STATE        hmac_state;
};

enum chain_mode
{
    CHAIN_MODE_CBC,
    CHAIN_MODE_ECB,
    CHAIN_MODE_CFB,
    CHAIN_MODE_CCM,
    CHAIN_MODE_GCM,
};

enum ecc_curve_id
{
    ECC_CURVE_INVALID = -1,
    ECC_CURVE_NONE,
    ECC_CURVE_25519,
    ECC_CURVE_BRAINPOOLP256R1,
    ECC_CURVE_BRAINPOOLP384R1,
    ECC_CURVE_P256R1,
    ECC_CURVE_P384R1,
    ECC_CURVE_P521R1,
};

struct algorithm
{
    struct object     hdr;
    enum alg_id       id;
    enum chain_mode   chain_mode;
    ULONG             flags;
    enum ecc_curve_id curve_id;
};

struct aes_key
{
    SYMCRYPT_AES_EXPANDED_KEY handle;
};

struct gcm_key
{
    SYMCRYPT_GCM_EXPANDED_KEY handle;
    SYMCRYPT_GCM_STATE        state;
};

struct hkdf_key
{
    SYMCRYPT_HKDF_EXPANDED_KEY handle;
    const SYMCRYPT_MAC        *hmac;
};

#define BLOCK_LENGTH_RC4    1
#define BLOCK_LENGTH_CHACHA20_POLY1305 1
#define BLOCK_LENGTH_3DES   8
#define BLOCK_LENGTH_AES   16

struct symmetric_key
{
    enum chain_mode  mode;
    ULONG            block_size;
    UCHAR           *secret;
    ULONG            secret_len;
    UCHAR            vector[BLOCK_LENGTH_AES];
    CRITICAL_SECTION cs;
    union
    {
        struct aes_key  aes;
        struct gcm_key  gcm;
        struct hkdf_key hkdf;
    };
};

struct rsa_key
{
    SYMCRYPT_RSAKEY *handle;
};

struct ecc_key
{
    SYMCRYPT_ECKEY   *handle;
    SYMCRYPT_ECURVE  *curve;
    enum ecc_curve_id curve_id;
};

struct dsa_key
{
    SYMCRYPT_DLKEY   *handle;
    SYMCRYPT_DLGROUP *group;
    UCHAR             seed[20];
    UINT32            count;
};

struct asymmetric_key
{
    ULONG bitlen;     /* key strength for ECC keys */
    union
    {
        struct rsa_key rsa;
        struct ecc_key ecc;
        struct dsa_key dsa;
    };
};

#define KEY_FLAG_PRIVATE    0x00000001
#define KEY_FLAG_FINALIZED  0x00000002

struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    ULONG         flags;
    union
    {
        struct symmetric_key s;
        struct asymmetric_key a;
    };
};

#define KEY_EXPORT_FLAG_PRIVATE     0x00000001
#define KEY_EXPORT_FLAG_RSA_FULL    0x00000002

struct secret
{
    struct object hdr;
    UCHAR        *derived_key;
    ULONG         derived_key_len;
};

void * SYMCRYPT_CALL SymCryptCallbackAlloc( SIZE_T size )
{
    return _aligned_malloc( size, SYMCRYPT_ASYM_ALIGN_VALUE );
}

void SYMCRYPT_CALL SymCryptCallbackFree( void *ptr )
{
    _aligned_free( ptr );
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptCallbackRandom( BYTE *buf, SIZE_T size )
{
    return RtlGenRandom( buf, size ) ? SYMCRYPT_NO_ERROR : SYMCRYPT_EXTERNAL_FAILURE;
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptRsaSignVerifyPct( const SYMCRYPT_RSAKEY *key )
{
    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptDsaPct( const SYMCRYPT_DLKEY *key )
{
    return SYMCRYPT_NO_ERROR;
}

void SYMCRYPT_CALL SymCryptRsaSelftest( void )
{
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptEcDsaPct( const SYMCRYPT_ECKEY *key )
{
    return SYMCRYPT_NO_ERROR;
}

void SYMCRYPT_CALL SymCryptDsaSelftest( void )
{
}

void SYMCRYPT_CALL SymCryptEcDsaSelftest( void )
{
}

void SYMCRYPT_CALL SymCryptDhSecretAgreementSelftest( void )
{
}

void SYMCRYPT_CALL SymCryptEcDhSecretAgreementSelftest( void )
{
}

UINT32 g_SymCryptFipsSelftestsPerformed;

static inline ULONG len_from_bitlen( ULONG bitlen )
{
    return (bitlen + 7) / 8;
}

NTSTATUS WINAPI BCryptAddContextFunction( ULONG table, const WCHAR *ctx, ULONG iface, const WCHAR *func, ULONG pos )
{
    FIXME( "%#lx, %s, %#lx, %s, %lu: stub\n", table, debugstr_w(ctx), iface, debugstr_w(func), pos );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptAddContextFunctionProvider( ULONG table, const WCHAR *ctx, ULONG iface, const WCHAR *func,
                                                  const WCHAR *provider, ULONG pos )
{
    FIXME( "%#lx, %s, %#lx, %s, %s, %lu: stub\n", table, debugstr_w(ctx), iface, debugstr_w(func),
           debugstr_w(provider), pos );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptRemoveContextFunction( ULONG table, const WCHAR *ctx, ULONG iface, const WCHAR *func )
{
    FIXME( "%#lx, %s, %#lx, %s: stub\n", table, debugstr_w(ctx), iface, debugstr_w(func) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptRemoveContextFunctionProvider( ULONG table, const WCHAR *ctx, ULONG iface, const WCHAR *func,
                                                     const WCHAR *provider )
{
    FIXME( "%#lx, %s, %#lx, %s, %s: stub\n", table, debugstr_w(ctx), iface, debugstr_w(func), debugstr_w(provider) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptEnumContextFunctions( ULONG table, const WCHAR *ctx, ULONG iface, ULONG *buflen,
                                            CRYPT_CONTEXT_FUNCTIONS **buffer )
{
    FIXME( "%#lx, %s, %#lx, %p, %p\n", table, debugstr_w(ctx), iface, buflen, buffer );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI BCryptFreeBuffer( void *buffer )
{
    free( buffer );
}

NTSTATUS WINAPI BCryptRegisterProvider( const WCHAR *provider, ULONG flags, CRYPT_PROVIDER_REG *reg )
{
    FIXME( "%s, %#lx, %p: stub\n", debugstr_w(provider), flags, reg );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptUnregisterProvider( const WCHAR *provider )
{
    FIXME( "%s: stub\n", debugstr_w(provider) );
    return STATUS_NOT_IMPLEMENTED;
}

#define MAX_HASH_OUTPUT_BYTES 64

/* ordered by class, keep in sync with enum alg_id */
static const struct
{
    const WCHAR *name;
    ULONG        class;
    ULONG        object_length;
    ULONG        hash_length;
    ULONG        hash_block_length;
    enum ecc_curve_id curve_id;
    enum chain_mode   chain_mode;
}
builtin_algorithms[] =
{
    { BCRYPT_3DES_ALGORITHM,       BCRYPT_CIPHER_INTERFACE,              522,    0,    0 },
    { BCRYPT_CHACHA20_POLY1305_ALGORITHM, BCRYPT_CIPHER_INTERFACE,       166,    0,    0 },
    { BCRYPT_AES_ALGORITHM,        BCRYPT_CIPHER_INTERFACE,              654,    0,    0 },
    { BCRYPT_AES_GMAC_ALGORITHM,   BCRYPT_CIPHER_INTERFACE,              654,    0,    0, 0, CHAIN_MODE_GCM },
    { BCRYPT_RC4_ALGORITHM,        BCRYPT_CIPHER_INTERFACE,              654,    0,    0 },
    { BCRYPT_SHA256_ALGORITHM,     BCRYPT_HASH_INTERFACE,                286,   32,   64 },
    { BCRYPT_SHA384_ALGORITHM,     BCRYPT_HASH_INTERFACE,                382,   48,  128 },
    { BCRYPT_SHA512_ALGORITHM,     BCRYPT_HASH_INTERFACE,                382,   64,  128 },
    { BCRYPT_SHA1_ALGORITHM,       BCRYPT_HASH_INTERFACE,                278,   20,   64 },
    { BCRYPT_MD5_ALGORITHM,        BCRYPT_HASH_INTERFACE,                274,   16,   64 },
    { BCRYPT_MD4_ALGORITHM,        BCRYPT_HASH_INTERFACE,                270,   16,   64 },
    { BCRYPT_MD2_ALGORITHM,        BCRYPT_HASH_INTERFACE,                270,   16,   16 },
    { BCRYPT_RSA_ALGORITHM,        BCRYPT_ASYMMETRIC_ENCRYPTION_INTERFACE, 0,      0,    0 },
    { BCRYPT_DH_ALGORITHM,         BCRYPT_SECRET_AGREEMENT_INTERFACE,      0,      0,    0 },
    { BCRYPT_ECDH_ALGORITHM,       BCRYPT_SECRET_AGREEMENT_INTERFACE,      0,      0,    0 },
    { BCRYPT_ECDH_P256_ALGORITHM,  BCRYPT_SECRET_AGREEMENT_INTERFACE,      0,      0,    0, ECC_CURVE_P256R1 },
    { BCRYPT_ECDH_P384_ALGORITHM,  BCRYPT_SECRET_AGREEMENT_INTERFACE,      0,      0,    0, ECC_CURVE_P384R1 },
    { BCRYPT_ECDH_P521_ALGORITHM,  BCRYPT_SECRET_AGREEMENT_INTERFACE,      0,      0,    0, ECC_CURVE_P521R1 },
    { BCRYPT_RSA_SIGN_ALGORITHM,   BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0 },
    { BCRYPT_ECDSA_ALGORITHM,      BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0 },
    { BCRYPT_ECDSA_P256_ALGORITHM, BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0, ECC_CURVE_P256R1 },
    { BCRYPT_ECDSA_P384_ALGORITHM, BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0, ECC_CURVE_P384R1 },
    { BCRYPT_ECDSA_P521_ALGORITHM, BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0, ECC_CURVE_P521R1 },
    { BCRYPT_DSA_ALGORITHM,        BCRYPT_SIGNATURE_INTERFACE,             0,      0,    0 },
    { BCRYPT_RNG_ALGORITHM,        BCRYPT_RNG_INTERFACE,                   0,      0,    0 },
    { BCRYPT_PBKDF2_ALGORITHM,     BCRYPT_KEY_DERIVATION_INTERFACE,      618,      0,    0 },
    { BCRYPT_TLS1_1_KDF_ALGORITHM, BCRYPT_KEY_DERIVATION_INTERFACE,      702,      0,    0 },
    { BCRYPT_TLS1_2_KDF_ALGORITHM, BCRYPT_KEY_DERIVATION_INTERFACE,      702,      0,    0 },
    { BCRYPT_HKDF_ALGORITHM,       BCRYPT_KEY_DERIVATION_INTERFACE,      702,      0,    0 },
};

static inline BOOL is_symmetric_alg( const struct algorithm *alg )
{
    return builtin_algorithms[alg->id].class == BCRYPT_CIPHER_INTERFACE
        || builtin_algorithms[alg->id].class == BCRYPT_KEY_DERIVATION_INTERFACE;
}

static inline BOOL is_symmetric_key( const struct key *key )
{
    return builtin_algorithms[key->alg_id].class == BCRYPT_CIPHER_INTERFACE
        || builtin_algorithms[key->alg_id].class == BCRYPT_KEY_DERIVATION_INTERFACE;
}

static BOOL match_operation_type( ULONG type, ULONG class )
{
    if (!type) return TRUE;
    switch (class)
    {
    case BCRYPT_CIPHER_INTERFACE:                return type & BCRYPT_CIPHER_OPERATION;
    case BCRYPT_HASH_INTERFACE:                  return type & BCRYPT_HASH_OPERATION;
    case BCRYPT_ASYMMETRIC_ENCRYPTION_INTERFACE: return type & BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION;
    case BCRYPT_SECRET_AGREEMENT_INTERFACE:      return type & BCRYPT_SECRET_AGREEMENT_OPERATION;
    case BCRYPT_SIGNATURE_INTERFACE:             return type & BCRYPT_SIGNATURE_OPERATION;
    case BCRYPT_RNG_INTERFACE:                   return type & BCRYPT_RNG_OPERATION;
    case BCRYPT_KEY_DERIVATION_INTERFACE:        return type & BCRYPT_KEY_DERIVATION_OPERATION;
    default: break;
    }
    return FALSE;
}

NTSTATUS WINAPI BCryptEnumAlgorithms( ULONG type, ULONG *ret_count, BCRYPT_ALGORITHM_IDENTIFIER **ret_list, ULONG flags )
{
    static const ULONG supported = BCRYPT_CIPHER_OPERATION |\
                                   BCRYPT_HASH_OPERATION |\
                                   BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION |\
                                   BCRYPT_SECRET_AGREEMENT_OPERATION |\
                                   BCRYPT_SIGNATURE_OPERATION |\
                                   BCRYPT_RNG_OPERATION |\
                                   BCRYPT_KEY_DERIVATION_OPERATION;
    BCRYPT_ALGORITHM_IDENTIFIER *list;
    ULONG i, j, count = 0;

    TRACE( "%#lx, %p, %p, %#lx\n", type, ret_count, ret_list, flags );

    if (!ret_count || !ret_list || (type & ~supported)) return STATUS_INVALID_PARAMETER;

    for (i = 0; i < ARRAY_SIZE( builtin_algorithms ); i++)
    {
        if (match_operation_type( type, builtin_algorithms[i].class )) count++;
    }

    if (!(list = malloc( count * sizeof(*list) ))) return STATUS_NO_MEMORY;

    for (i = 0, j = 0; i < ARRAY_SIZE( builtin_algorithms ); i++)
    {
        if (!match_operation_type( type, builtin_algorithms[i].class )) continue;
        list[j].pszName = (WCHAR *)builtin_algorithms[i].name;
        list[j].dwClass = builtin_algorithms[i].class;
        list[j].dwFlags = 0;
        j++;
    }

    *ret_count = count;
    *ret_list = list;
    return STATUS_SUCCESS;
}

static const struct algorithm pseudo_algorithms[] =
{
    {{ MAGIC_ALG }, ALG_ID_MD2 },
    {{ MAGIC_ALG }, ALG_ID_MD4 },
    {{ MAGIC_ALG }, ALG_ID_MD5 },
    {{ MAGIC_ALG }, ALG_ID_SHA1 },
    {{ MAGIC_ALG }, ALG_ID_SHA256 },
    {{ MAGIC_ALG }, ALG_ID_SHA384 },
    {{ MAGIC_ALG }, ALG_ID_SHA512 },
    {{ MAGIC_ALG }, ALG_ID_RC4 },
    {{ MAGIC_ALG }, ALG_ID_RNG },
    {{ MAGIC_ALG }, ALG_ID_MD5, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_SHA1, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_SHA256, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_SHA384, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_SHA512, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_RSA },
    {{ MAGIC_ALG }, ALG_ID_ECDSA },
    {{ 0 }}, /* AES_CMAC */
    {{ MAGIC_ALG }, ALG_ID_AES_GMAC, CHAIN_MODE_GCM },
    {{ MAGIC_ALG }, ALG_ID_MD2, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_MD4, 0, BCRYPT_ALG_HANDLE_HMAC_FLAG },
    {{ MAGIC_ALG }, ALG_ID_3DES, CHAIN_MODE_CBC },
    {{ MAGIC_ALG }, ALG_ID_3DES, CHAIN_MODE_ECB },
    {{ MAGIC_ALG }, ALG_ID_3DES, CHAIN_MODE_CFB },
    {{ 0 }}, /* 3DES_112_CBC */
    {{ 0 }}, /* 3DES_112_ECB */
    {{ 0 }}, /* 3DES_112_CFB */
    {{ MAGIC_ALG }, ALG_ID_AES, CHAIN_MODE_CBC },
    {{ MAGIC_ALG }, ALG_ID_AES, CHAIN_MODE_ECB },
    {{ MAGIC_ALG }, ALG_ID_AES, CHAIN_MODE_CFB },
    {{ MAGIC_ALG }, ALG_ID_AES, CHAIN_MODE_CCM },
    {{ MAGIC_ALG }, ALG_ID_AES, CHAIN_MODE_GCM },
    {{ 0 }}, /* DES_CBC */
    {{ 0 }}, /* DES_ECB */
    {{ 0 }}, /* DES_CFB */
    {{ 0 }}, /* DESX_CBC */
    {{ 0 }}, /* DESX_ECB */
    {{ 0 }}, /* DESX_CFB */
    {{ 0 }}, /* RC2_CBC */
    {{ 0 }}, /* RC2_ECB */
    {{ 0 }}, /* RC2_CFB */
    {{ MAGIC_ALG }, ALG_ID_DH },
    {{ MAGIC_ALG }, ALG_ID_ECDH },
    {{ MAGIC_ALG }, ALG_ID_ECDH_P256, 0, 0, ECC_CURVE_P256R1 },
    {{ MAGIC_ALG }, ALG_ID_ECDH_P384, 0, 0, ECC_CURVE_P384R1 },
    {{ MAGIC_ALG }, ALG_ID_ECDH_P521, 0, 0, ECC_CURVE_P521R1 },
    {{ MAGIC_ALG }, ALG_ID_DSA },
    {{ MAGIC_ALG }, ALG_ID_ECDSA_P256, 0, 0, ECC_CURVE_P256R1 },
    {{ MAGIC_ALG }, ALG_ID_ECDSA_P384, 0, 0, ECC_CURVE_P384R1 },
    {{ MAGIC_ALG }, ALG_ID_ECDSA_P521, 0, 0, ECC_CURVE_P521R1 },
    {{ MAGIC_ALG }, ALG_ID_RSA_SIGN },
    {{ 0 }}, /* CAPI_KDF */
    {{ MAGIC_ALG }, ALG_ID_PBKDF2 },
    {{ 0 }}, /* SP800108_CTR_HMAC */
    {{ 0 }}, /* SP80056A_CONCAT */
    {{ MAGIC_ALG }, ALG_ID_TLS1_1_KDF },
    {{ MAGIC_ALG }, ALG_ID_TLS1_2_KDF },
    {{ 0 }}, /* XTS_AES */
    {{ MAGIC_ALG }, ALG_ID_HKDF },
    {{ MAGIC_ALG }, ALG_ID_CHACHA20_POLY1305 },
};

/* Algorithm pseudo-handles are denoted by having the lowest bit set.
 * An aligned algorithm pointer will never have this bit set.
 */
static inline BOOL is_alg_pseudo_handle( BCRYPT_ALG_HANDLE handle )
{
    return (((ULONG_PTR)handle & 1) == 1);
}

static struct object *get_object( BCRYPT_HANDLE handle, ULONG magic )
{
    ULONG idx;

    if (!handle) return NULL;

    if (!is_alg_pseudo_handle( handle ))
    {
        struct object *obj = handle;
        if (magic && obj->magic != magic) return NULL;
        return obj;
    }

    idx = (ULONG_PTR)handle >> 4;
    if (idx >= ARRAY_SIZE(pseudo_algorithms) || !pseudo_algorithms[idx].hdr.magic)
    {
        FIXME( "pseudo-handle %p not supported\n", handle );
        return NULL;
    }
    return (struct object *)&pseudo_algorithms[idx];
}

static inline struct algorithm *get_alg_object( BCRYPT_ALG_HANDLE handle )
{
    return (struct algorithm *)get_object( handle, MAGIC_ALG );
}

static inline struct hash *get_hash_object( BCRYPT_HASH_HANDLE handle )
{
    return (struct hash *)get_object( handle, MAGIC_HASH );
}

static inline struct key *get_key_object( BCRYPT_KEY_HANDLE handle )
{
    return (struct key *)get_object( handle, MAGIC_KEY );
}

static inline struct secret *get_secret_object( BCRYPT_SECRET_HANDLE handle )
{
    return (struct secret *)get_object( handle, MAGIC_SECRET );
}

NTSTATUS WINAPI BCryptGenRandom( BCRYPT_ALG_HANDLE handle, UCHAR *buffer, ULONG count, ULONG flags )
{
    const DWORD supported_flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
    const struct algorithm *alg = get_alg_object( handle );

    TRACE("%p, %p, %lu, %#lx - semi-stub\n", handle, buffer, count, flags);

    if (!handle)
    {
        /* It's valid to call without an algorithm if BCRYPT_USE_SYSTEM_PREFERRED_RNG
         * is set. In this case the preferred system RNG is used.
         */
        if (!(flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
            return STATUS_INVALID_HANDLE;
    }
    else if (is_alg_pseudo_handle( handle ) && handle != BCRYPT_RNG_ALG_HANDLE)
    {
        FIXME( "pseudo-handle %p not supported\n", handle );
        return STATUS_NOT_IMPLEMENTED;
    }
    else if (!alg || alg->id != ALG_ID_RNG)
        return STATUS_INVALID_HANDLE;

    if (!buffer)
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags)
        FIXME("unsupported flags %#lx\n", flags & ~supported_flags);

    if (alg)
        FIXME("ignoring selected algorithm\n");

    if (!count) return STATUS_SUCCESS;

    if (alg || (flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
    {
        if (RtlGenRandom(buffer, count)) return STATUS_SUCCESS;
    }

    FIXME("called with unsupported parameters, returning error\n");
    return STATUS_NOT_IMPLEMENTED;
}

static struct algorithm *create_algorithm( enum alg_id id, DWORD flags )
{
    struct algorithm *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    ret->hdr.magic  = MAGIC_ALG;
    ret->id         = id;
    ret->flags      = flags;
    ret->curve_id   = builtin_algorithms[id].curve_id;
    ret->chain_mode = builtin_algorithms[id].chain_mode;
    return ret;
}

NTSTATUS WINAPI BCryptOpenAlgorithmProvider( BCRYPT_ALG_HANDLE *handle, const WCHAR *id, const WCHAR *implementation,
                                             DWORD flags )
{
    const DWORD supported_flags = BCRYPT_ALG_HANDLE_HMAC_FLAG | BCRYPT_HASH_REUSABLE_FLAG;
    struct algorithm *alg;
    enum alg_id alg_id;
    ULONG i;

    TRACE( "%p, %s, %s, %#lx\n", handle, wine_dbgstr_w(id), wine_dbgstr_w(implementation), flags );

    if (!handle || !id) return STATUS_INVALID_PARAMETER;
    if (flags & ~supported_flags)
    {
        FIXME( "unsupported flags %#lx\n", flags & ~supported_flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    for (i = 0; i < ARRAY_SIZE( builtin_algorithms ); i++)
    {
        if (!wcscmp( id, builtin_algorithms[i].name))
        {
            alg_id = i;
            break;
        }
    }
    if (i == ARRAY_SIZE( builtin_algorithms ))
    {
        FIXME( "algorithm %s not supported\n", debugstr_w(id) );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (implementation && wcscmp( implementation, MS_PRIMITIVE_PROVIDER ))
    {
        FIXME( "implementation %s not supported\n", debugstr_w(implementation) );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(alg = create_algorithm( alg_id, flags ))) return STATUS_NO_MEMORY;
    *handle = alg;

    TRACE( "returning handle %p\n", *handle );
    return STATUS_SUCCESS;
}

static void secure_zero( void *ptr, SIZE_T size )
{
    if (ptr) SecureZeroMemory( ptr, size );
}

static void destroy_object( struct object *obj )
{
    secure_zero( &obj->magic, sizeof(obj->magic) );
    free( obj );
}

NTSTATUS WINAPI BCryptCloseAlgorithmProvider( BCRYPT_ALG_HANDLE handle, DWORD flags )
{
    struct algorithm *alg = handle;

    TRACE( "%p, %#lx\n", handle, flags );

    if (!handle || is_alg_pseudo_handle( handle ) || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;

    destroy_object( &alg->hdr );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptGetFipsAlgorithmMode(BOOLEAN *enabled)
{
    FIXME("%p - semi-stub\n", enabled);

    if (!enabled)
        return STATUS_INVALID_PARAMETER;

    *enabled = FALSE;
    return STATUS_SUCCESS;
}

static NTSTATUS get_dword_property( UCHAR *buf, ULONG size, ULONG *ret_size, DWORD value )
{
    *ret_size = sizeof(value);
    if (size < sizeof(value)) return STATUS_BUFFER_TOO_SMALL;
    if (buf) *(DWORD *)buf = value;
    return STATUS_SUCCESS;
}

static NTSTATUS get_generic_alg_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_OBJECT_LENGTH ))
    {
        if (!builtin_algorithms[id].object_length) return STATUS_NOT_SUPPORTED;
        return get_dword_property( buf, size, ret_size, builtin_algorithms[id].object_length );
    }

    if (!wcscmp( prop, BCRYPT_HASH_LENGTH ))
    {
        if (!builtin_algorithms[id].hash_length) return STATUS_NOT_SUPPORTED;
        return get_dword_property( buf, size, ret_size, builtin_algorithms[id].hash_length );
    }

    if (!wcscmp( prop, BCRYPT_HASH_BLOCK_LENGTH ))
    {
        if (!builtin_algorithms[id].hash_block_length) return STATUS_NOT_SUPPORTED;
        return get_dword_property( buf, size, ret_size, builtin_algorithms[id].hash_block_length );
    }

    if (!wcscmp( prop, BCRYPT_ALGORITHM_NAME ))
    {
        *ret_size = (lstrlenW(builtin_algorithms[id].name) + 1) * sizeof(WCHAR);
        if (size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if(buf) memcpy(buf, builtin_algorithms[id].name, *ret_size);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_3des_property( enum chain_mode mode, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH ))
        return get_dword_property( buf, size, ret_size, BLOCK_LENGTH_3DES );

    if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
    {
        const WCHAR *str;
        switch (mode)
        {
        case CHAIN_MODE_CBC:
            str = BCRYPT_CHAIN_MODE_CBC; break;
        default:
            return STATUS_NOT_IMPLEMENTED;
        }

        *ret_size = (wcslen(str) + 1) * sizeof(WCHAR) * 2; /* quirk */
        if (size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        memcpy( buf, str, (wcslen(str) + 1) * sizeof(WCHAR) );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( prop, BCRYPT_KEY_LENGTHS ))
    {
        BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
        *ret_size = sizeof(*key_lengths);
        if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (key_lengths)
        {
            key_lengths->dwMinLength = 192;
            key_lengths->dwMaxLength = 192;
            key_lengths->dwIncrement = 0;
        }
        return STATUS_SUCCESS;
    }
    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_aes_property( enum chain_mode mode, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH ))
        return get_dword_property( buf, size, ret_size, BLOCK_LENGTH_AES );

    if (!wcscmp( prop, BCRYPT_MESSAGE_BLOCK_LENGTH ))
        return get_dword_property( buf, size, ret_size, 1 );

    if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
    {
        const WCHAR *str;
        switch (mode)
        {
        case CHAIN_MODE_ECB: str = BCRYPT_CHAIN_MODE_ECB; break;
        case CHAIN_MODE_CBC: str = BCRYPT_CHAIN_MODE_CBC; break;
        case CHAIN_MODE_GCM: str = BCRYPT_CHAIN_MODE_GCM; break;
        case CHAIN_MODE_CFB: str = BCRYPT_CHAIN_MODE_CFB; break;
        default: return STATUS_NOT_IMPLEMENTED;
        }

        *ret_size = (wcslen(str) + 1) * sizeof(WCHAR) * 2; /* quirk */
        if (size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        memcpy( buf, str, (wcslen(str) + 1) * sizeof(WCHAR) );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( prop, BCRYPT_KEY_LENGTHS ))
    {
        BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
        *ret_size = sizeof(*key_lengths);
        if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (key_lengths)
        {
            key_lengths->dwMinLength = 128;
            key_lengths->dwMaxLength = 256;
            key_lengths->dwIncrement = 64;
        }
        return STATUS_SUCCESS;
    }
    if (!wcscmp( prop, BCRYPT_AUTH_TAG_LENGTH ))
    {
        BCRYPT_AUTH_TAG_LENGTHS_STRUCT *tag_length = (void *)buf;
        if (mode != CHAIN_MODE_GCM) return STATUS_NOT_SUPPORTED;
        *ret_size = sizeof(*tag_length);
        if (tag_length && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (tag_length)
        {
            tag_length->dwMinLength = 12;
            tag_length->dwMaxLength = 16;
            tag_length->dwIncrement =  1;
        }
        return STATUS_SUCCESS;
    }

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_rc4_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH ))
        return get_dword_property( buf, size, ret_size, BLOCK_LENGTH_RC4 );

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_rsa_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_PADDING_SCHEMES ))
        return get_dword_property( buf, size, ret_size, BCRYPT_SUPPORTED_PAD_PKCS1_SIG | BCRYPT_SUPPORTED_PAD_OAEP );

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_dsa_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_PADDING_SCHEMES )) return STATUS_NOT_SUPPORTED;
    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_pbkdf2_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH )) return STATUS_NOT_SUPPORTED;
    if (!wcscmp( prop, BCRYPT_KEY_LENGTHS ))
    {
        BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
        *ret_size = sizeof(*key_lengths);
        if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (key_lengths)
        {
            key_lengths->dwMinLength = 0;
            key_lengths->dwMaxLength = 16384;
            key_lengths->dwIncrement = 8;
        }
        return STATUS_SUCCESS;
    }
    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_tls_kdf_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH )) return STATUS_NOT_SUPPORTED;
    if (!wcscmp( prop, BCRYPT_KEY_LENGTHS ))
    {
        BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
        *ret_size = sizeof(*key_lengths);
        if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (key_lengths)
        {
            key_lengths->dwMinLength = 0;
            key_lengths->dwMaxLength = 16384;
            key_lengths->dwIncrement = 8;
        }
        return STATUS_SUCCESS;
    }
    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_hkdf_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH )) return STATUS_NOT_SUPPORTED;
    if (!wcscmp( prop, BCRYPT_KEY_LENGTHS ))
    {
        BCRYPT_KEY_LENGTHS_STRUCT *key_lengths = (void *)buf;
        *ret_size = sizeof(*key_lengths);
        if (key_lengths && size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
        if (key_lengths)
        {
            key_lengths->dwMinLength = 0;
            key_lengths->dwMaxLength = 16384;
            key_lengths->dwIncrement = 8;
        }
        return STATUS_SUCCESS;
    }
    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_chacha20_poly1305_property( const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_BLOCK_LENGTH ))
        return get_dword_property( buf, size, ret_size, BLOCK_LENGTH_CHACHA20_POLY1305 );

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_alg_property( const struct algorithm *alg, const WCHAR *prop, UCHAR *buf, ULONG size,
                                  ULONG *ret_size )
{
    NTSTATUS status;

    if ((status = get_generic_alg_property( alg->id, prop, buf, size, ret_size )) != STATUS_NOT_IMPLEMENTED)
        return status;

    switch (alg->id)
    {
    case ALG_ID_3DES:
        return get_3des_property( alg->chain_mode, prop, buf, size, ret_size );

    case ALG_ID_CHACHA20_POLY1305:
        return get_chacha20_poly1305_property( prop, buf, size, ret_size );

    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        return get_aes_property( alg->chain_mode, prop, buf, size, ret_size );

    case ALG_ID_RC4:
        return get_rc4_property( prop, buf, size, ret_size );

    case ALG_ID_RSA:
        return get_rsa_property( prop, buf, size, ret_size );

    case ALG_ID_DSA:
        return get_dsa_property( prop, buf, size, ret_size );

    case ALG_ID_PBKDF2:
        return get_pbkdf2_property( prop, buf, size, ret_size );

    case ALG_ID_TLS1_1_KDF:
    case ALG_ID_TLS1_2_KDF:
        return get_tls_kdf_property( prop, buf, size, ret_size );

    case ALG_ID_HKDF:
        return get_hkdf_property( prop, buf, size, ret_size );

    default:
        break;
    }

    FIXME( "unsupported property %s algorithm %u\n", debugstr_w(prop), alg->id );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS set_alg_property( struct algorithm *alg, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    switch (alg->id)
    {
    case ALG_ID_3DES:
        if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
        {
            TRACE( "mode %s\n", debugstr_w((WCHAR *)value) );
            if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC ))
            {
                alg->chain_mode = CHAIN_MODE_CBC;
                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
                return STATUS_NOT_SUPPORTED;
            }
        }
        FIXME( "unsupported 3des algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_CHACHA20_POLY1305:
        if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
        {
            if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_NA )) return STATUS_SUCCESS;

            FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
            return STATUS_NOT_IMPLEMENTED;
        }
        FIXME( "unsupported CHACHA20_POLY1305 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_AES:
        if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
        {
            TRACE( "mode %s\n", debugstr_w((WCHAR *)value) );
            if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB ))
            {
                alg->chain_mode = CHAIN_MODE_ECB;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC ))
            {
                alg->chain_mode = CHAIN_MODE_CBC;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM ))
            {
                alg->chain_mode = CHAIN_MODE_GCM;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_CFB ))
            {
                alg->chain_mode = CHAIN_MODE_CFB;
                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
                return STATUS_NOT_IMPLEMENTED;
            }
        }
        else if (!wcscmp( (WCHAR *)prop, BCRYPT_MESSAGE_BLOCK_LENGTH ))
            return STATUS_INVALID_PARAMETER;

        FIXME( "unsupported aes algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_RC4:
        if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
        {
            TRACE( "mode %s\n", debugstr_w((WCHAR *)value) );
            if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_NA )) return STATUS_SUCCESS;

            FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
            return STATUS_NOT_IMPLEMENTED;
        }
        FIXME( "unsupported rc4 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_ECDH:
    case ALG_ID_ECDSA:
        if (!wcscmp( prop, BCRYPT_ECC_CURVE_NAME ))
        {
            TRACE( "curve %s\n", debugstr_w((const WCHAR *)value) );
            if (!wcscmp( (const WCHAR *)value, BCRYPT_ECC_CURVE_25519 ))
            {
                if (alg->id != ALG_ID_ECDH) return STATUS_NOT_SUPPORTED;
                alg->curve_id = ECC_CURVE_25519;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_ECC_CURVE_SECP256R1 ))
            {
                alg->curve_id = ECC_CURVE_P256R1;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_ECC_CURVE_SECP384R1 ))
            {
                alg->curve_id = ECC_CURVE_P384R1;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_ECC_CURVE_SECP521R1 ))
            {
                alg->curve_id = ECC_CURVE_P521R1;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_ECC_CURVE_BRAINPOOLP256R1 ))
            {
                alg->curve_id = ECC_CURVE_BRAINPOOLP256R1;
                return STATUS_SUCCESS;
            }
            else if (!wcscmp( (WCHAR *)value, BCRYPT_ECC_CURVE_BRAINPOOLP384R1 ))
            {
                alg->curve_id = ECC_CURVE_BRAINPOOLP384R1;
                return STATUS_SUCCESS;
            }
            FIXME( "unsupported curve %s\n", debugstr_w((WCHAR *)value) );
            return STATUS_NOT_IMPLEMENTED;
        }
        FIXME( "unsupported ECDH/ECDSA algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    default:
        FIXME( "unsupported algorithm %u\n", alg->id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS set_dh_parameters( struct key *key, const UCHAR *buf, ULONG size )
{
    BCRYPT_DH_PARAMETER_HEADER *hdr = (BCRYPT_DH_PARAMETER_HEADER *)buf;
    UCHAR *prime, *generator;
    ULONG key_size = len_from_bitlen( key->a.bitlen );

    if (key->alg_id != ALG_ID_DH || (key->flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    if (size < sizeof(*hdr) || hdr->dwMagic != BCRYPT_DH_PARAMETERS_MAGIC || hdr->cbKeyLength != key_size ||
        hdr->cbLength != sizeof(*hdr) + key_size * 2 || size != hdr->cbLength) return STATUS_INVALID_PARAMETER;

    if (!key->a.dsa.group && !(key->a.dsa.group = SymCryptDlgroupAllocate( key_size * 8, 0 )))
        return STATUS_NO_MEMORY;

    prime = (UCHAR *)(hdr + 1);
    generator = prime + key_size;

    if (SymCryptDlgroupSetValue( prime, key_size, NULL, 0, generator, key_size, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                                 NULL, NULL, 0, key->a.dsa.count, SYMCRYPT_DLGROUP_FIPS_NONE, key->a.dsa.group ))
        return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static const SYMCRYPT_MAC *get_hmac_from_hash( const WCHAR *alg )
{
    if (!alg) return NULL;
    if (!wcscmp( alg, BCRYPT_SHA1_ALGORITHM )) return SymCryptHmacSha1Algorithm;
    if (!wcscmp( alg, BCRYPT_SHA256_ALGORITHM )) return SymCryptHmacSha256Algorithm;
    if (!wcscmp( alg, BCRYPT_SHA384_ALGORITHM )) return SymCryptHmacSha384Algorithm;
    if (!wcscmp( alg, BCRYPT_SHA512_ALGORITHM )) return SymCryptHmacSha512Algorithm;

    FIXME( "hash algorithm %s not supported\n", debugstr_w(alg) );
    return NULL;
}

static NTSTATUS hkdf_finalize( struct key *key, UCHAR *salt, ULONG salt_len )
{
    SYMCRYPT_ERROR error;

    if (salt)
        error = SymCryptHkdfExpandKey( &key->s.hkdf.handle, key->s.hkdf.hmac, key->s.secret, key->s.secret_len, salt,
                                       salt_len );
    else
        error = SymCryptHkdfPrkExpandKey( &key->s.hkdf.handle, key->s.hkdf.hmac, key->s.secret, key->s.secret_len );

    if (error) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS set_key_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    if (!wcscmp( prop, BCRYPT_CHAINING_MODE ))
    {
        if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB ))
        {
            key->s.mode = CHAIN_MODE_ECB;
            return STATUS_SUCCESS;
        }
        else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC ))
        {
            key->s.mode = CHAIN_MODE_CBC;
            return STATUS_SUCCESS;
        }
        else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM ))
        {
            key->s.mode = CHAIN_MODE_GCM;
            return STATUS_SUCCESS;
        }
        else if (!wcscmp( (WCHAR *)value, BCRYPT_CHAIN_MODE_CFB ))
        {
            key->s.mode = CHAIN_MODE_CFB;
            return STATUS_SUCCESS;
        }
        else
        {
            FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
            return STATUS_NOT_IMPLEMENTED;
        }
    }
    else if (!wcscmp( prop, BCRYPT_INITIALIZATION_VECTOR ))
    {
        if (!is_symmetric_key( key )) return STATUS_INVALID_HANDLE;
        if (size != key->s.block_size || size > sizeof(key->s.vector)) return STATUS_INVALID_PARAMETER;

        EnterCriticalSection( &key->s.cs );
        memcpy( key->s.vector, value, size );
        LeaveCriticalSection( &key->s.cs );
        return STATUS_SUCCESS;
    }
    else if (!wcscmp( prop, BCRYPT_KEY_LENGTH ))
    {
        if (size < sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
        key->a.bitlen = *(DWORD*)value;
        return STATUS_SUCCESS;
    }
    else if (!wcscmp( prop, BCRYPT_DH_PARAMETERS ))
    {
        return set_dh_parameters( key, value, size );
    }
    else if (!wcscmp( prop, BCRYPT_HKDF_HASH_ALGORITHM ))
    {
        if (!(key->s.hkdf.hmac = get_hmac_from_hash( (const WCHAR *)value )))
            return STATUS_INVALID_PARAMETER;
        return STATUS_SUCCESS;
    }
    else if (!wcscmp( prop, BCRYPT_HKDF_SALT_AND_FINALIZE ))
    {
        return hkdf_finalize( key, value, size );
    }
    else if (!wcscmp( prop, BCRYPT_HKDF_PRK_AND_FINALIZE ))
    {
        return hkdf_finalize( key, NULL, 0 );
    }

    FIXME( "unsupported key property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptSetProperty( BCRYPT_HANDLE handle, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    struct object *object = get_object( handle, 0 );

    TRACE( "%p, %s, %p, %lu, %#lx\n", handle, debugstr_w(prop), value, size, flags );

    if (!handle) return STATUS_INVALID_HANDLE;
    if (is_alg_pseudo_handle( handle )) return STATUS_ACCESS_DENIED;

    switch (object->magic)
    {
    case MAGIC_ALG:
    {
        struct algorithm *alg = (struct algorithm *)object;
        return set_alg_property( alg, prop, value, size, flags );
    }
    case MAGIC_KEY:
    {
        struct key *key = (struct key *)object;
        return set_key_property( key, prop, value, size, flags );
    }
    default:
        WARN( "unknown magic %#lx\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

static NTSTATUS get_hash_property( const struct hash *hash, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    if ((status = get_generic_alg_property( hash->alg_id, prop, buf, size, ret_size )) == STATUS_NOT_IMPLEMENTED)
        FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return status;
}

static NTSTATUS get_dh_parameters( const struct key *key, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    BCRYPT_DH_PARAMETER_HEADER *hdr = (BCRYPT_DH_PARAMETER_HEADER *)buf;
    UCHAR *prime, *generator;
    UINT32 key_size = len_from_bitlen( key->a.bitlen ), required_size = sizeof(*hdr) + key_size * 2;

    if (!(key->flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    *ret_size = required_size;
    if (!buf) return STATUS_SUCCESS;
    else if (size < required_size) return STATUS_BUFFER_TOO_SMALL;

    prime = (UCHAR *)(hdr + 1);
    generator = prime + key_size;

    if (SymCryptDlgroupGetValue( key->a.dsa.group, prime, key_size, NULL, 0, generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, NULL, (BYTE *)key->a.dsa.seed,
                                 sizeof(key->a.dsa.seed), (UINT32 *)&key->a.dsa.count )) return STATUS_INTERNAL_ERROR;

    hdr->cbLength    = sizeof(*hdr) + key_size * 2;
    hdr->dwMagic     = BCRYPT_DH_PARAMETERS_MAGIC;
    hdr->cbKeyLength = key_size;

    return STATUS_SUCCESS;
}

static NTSTATUS get_key_property( const struct key *key, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!wcscmp( prop, BCRYPT_KEY_STRENGTH ))
    {
        DWORD value = is_symmetric_key (key ) ? key->s.block_size * 8 : key->a.bitlen;
        return get_dword_property( buf, size, ret_size, value );
    }

    switch (key->alg_id)
    {
    case ALG_ID_3DES:
        return get_3des_property( key->s.mode, prop, buf, size, ret_size );

    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        if (!wcscmp( prop, BCRYPT_AUTH_TAG_LENGTH )) return STATUS_NOT_SUPPORTED;
        return get_aes_property( key->s.mode, prop, buf, size, ret_size );

    case ALG_ID_DH:
        if (wcscmp( prop, BCRYPT_DH_PARAMETERS )) return STATUS_NOT_SUPPORTED;
        return get_dh_parameters( key, buf, size, ret_size );

    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        if (wcscmp( prop, BCRYPT_SIGNATURE_LENGTH )) return STATUS_NOT_SUPPORTED;
        return get_dword_property( buf, size, ret_size, key->a.bitlen / 8 );

    default:
        FIXME( "unsupported algorithm %u\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS WINAPI BCryptGetProperty( BCRYPT_HANDLE handle, const WCHAR *prop, UCHAR *buffer, ULONG count, ULONG *res,
                                   ULONG flags )
{
    const struct object *object = get_object( handle, 0 );

    TRACE( "%p, %s, %p, %lu, %p, %#lx\n", handle, wine_dbgstr_w(prop), buffer, count, res, flags );

    if (!object) return STATUS_INVALID_HANDLE;
    if (!prop || !res) return STATUS_INVALID_PARAMETER;

    switch (object->magic)
    {
    case MAGIC_ALG:
    {
        const struct algorithm *alg = (const struct algorithm *)object;
        return get_alg_property( alg, prop, buffer, count, res );
    }
    case MAGIC_KEY:
    {
        const struct key *key = (const struct key *)object;
        return get_key_property( key, prop, buffer, count, res );
    }
    case MAGIC_HASH:
    {
        const struct hash *hash = (const struct hash *)object;
        return get_hash_property( hash, prop, buffer, count, res );
    }
    default:
        WARN( "unknown magic %#lx\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

static void prepare_hash( struct hash *hash )
{
    if (hash->flags & HASH_FLAG_HMAC)
    {
        SymCryptHmacExpandKey( hash->desc, &hash->hmac, hash->secret, hash->secret_len );
        SymCryptHmacInit( &hash->hmac_state, &hash->hmac );
    }
    else SymCryptHashInit( hash->desc, &hash->state );
}

static const SYMCRYPT_HASH *get_hash_from_alg( enum alg_id alg_id )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:    return SymCryptMd2Algorithm;
    case ALG_ID_MD4:    return SymCryptMd4Algorithm;
    case ALG_ID_MD5:    return SymCryptMd5Algorithm;
    case ALG_ID_SHA1:   return SymCryptSha1Algorithm;
    case ALG_ID_SHA256: return SymCryptSha256Algorithm;
    case ALG_ID_SHA384: return SymCryptSha384Algorithm;
    case ALG_ID_SHA512: return SymCryptSha512Algorithm;
    default:
        FIXME( "unhandled algorithm %u\n", alg_id );
        return NULL;
    }
}

static NTSTATUS create_hash( const struct algorithm *alg, UCHAR *secret, ULONG secret_len, ULONG flags,
                             struct hash **ret_hash )
{
    struct hash *hash;
    const SYMCRYPT_HASH *desc = get_hash_from_alg( alg->id );

    if (secret && !(alg->flags & BCRYPT_ALG_HANDLE_HMAC_FLAG)) return STATUS_INVALID_PARAMETER;

    if (!desc) return STATUS_NOT_IMPLEMENTED;
    if (!(hash = calloc( 1, sizeof(*hash) ))) return STATUS_NO_MEMORY;
    hash->hdr.magic = MAGIC_HASH;
    hash->alg_id    = alg->id;
    hash->len       = builtin_algorithms[alg->id].hash_length;
    hash->desc      = desc;
    if (alg->flags & BCRYPT_ALG_HANDLE_HMAC_FLAG) hash->flags = HASH_FLAG_HMAC;
    if ((alg->flags & BCRYPT_HASH_REUSABLE_FLAG) || (flags & BCRYPT_HASH_REUSABLE_FLAG))
        hash->flags |= HASH_FLAG_REUSABLE;

    if (secret_len && !(hash->secret = malloc( secret_len )))
    {
        free( hash );
        return STATUS_NO_MEMORY;
    }
    memcpy( hash->secret, secret, secret_len );
    hash->secret_len = secret_len;

    prepare_hash( hash );
    *ret_hash = hash;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptCreateHash( BCRYPT_ALG_HANDLE handle, BCRYPT_HASH_HANDLE *ret_handle, UCHAR *object,
                                  ULONG object_len, UCHAR *secret, ULONG secret_len, ULONG flags )
{
    struct algorithm *alg = get_alg_object( handle );
    struct hash *hash;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %lu, %p, %lu, %#lx\n", handle, ret_handle, object, object_len, secret, secret_len, flags );
    if (flags & ~BCRYPT_HASH_REUSABLE_FLAG)
    {
        FIXME( "unimplemented flags %#lx\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }
    if (object) FIXME( "ignoring object buffer\n" );

    if (!alg) return STATUS_INVALID_HANDLE;
    if (!ret_handle) return STATUS_INVALID_PARAMETER;

    if ((status = create_hash( alg, secret, secret_len, flags, &hash ))) return status;
    *ret_handle = hash;

    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateHash( BCRYPT_HASH_HANDLE handle, BCRYPT_HASH_HANDLE *handle_copy, UCHAR *object,
                                     ULONG object_len, ULONG flags )
{
    const struct hash *hash_orig = get_hash_object( handle );
    struct hash *hash_copy;

    TRACE( "%p, %p, %p, %lu, %#lx\n", handle, handle_copy, object, object_len, flags );

    if (!hash_orig) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash_copy = malloc( sizeof(*hash_copy) ))) return STATUS_NO_MEMORY;

    memcpy( hash_copy, hash_orig, sizeof(*hash_orig) );
    if (hash_orig->secret && !(hash_copy->secret = malloc( hash_orig->secret_len )))
    {
        free( hash_copy );
        return STATUS_NO_MEMORY;
    }
    memcpy( hash_copy->secret, hash_orig->secret, hash_orig->secret_len );

    if (hash_orig->flags & HASH_FLAG_HMAC)
    {
        SymCryptHmacKeyCopy( &hash_orig->hmac, &hash_copy->hmac );
        SymCryptHmacStateCopy( &hash_orig->hmac_state, &hash_orig->hmac, &hash_copy->hmac_state );
    }
    else
        SymCryptHashStateCopy( hash_orig->desc, &hash_orig->state, &hash_copy->state );

    *handle_copy = hash_copy;
    TRACE( "returning handle %p\n", *handle_copy );
    return STATUS_SUCCESS;
}

static void destroy_hash( struct hash *hash )
{
    if (!hash) return;
    free( hash->secret );
    destroy_object( &hash->hdr );
}

NTSTATUS WINAPI BCryptDestroyHash( BCRYPT_HASH_HANDLE handle )
{
    struct hash *hash = get_hash_object( handle );

    TRACE( "%p\n", handle );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_PARAMETER;

    destroy_hash( hash );
    return STATUS_SUCCESS;
}

static void hash_data( struct hash *hash, UCHAR *input, ULONG size )
{
    if (hash->flags & HASH_FLAG_HMAC)
        SymCryptHmacAppend( &hash->hmac_state, input, size );
    else
        SymCryptHashAppend( hash->desc, &hash->state, input, size );
}

NTSTATUS WINAPI BCryptHashData( BCRYPT_HASH_HANDLE handle, UCHAR *input, ULONG size, ULONG flags )
{
    struct hash *hash = get_hash_object( handle );

    TRACE( "%p, %p, %lu, %#lx\n", handle, input, size, flags );

    if (!hash) return STATUS_INVALID_HANDLE;

    hash_data( hash, input, size );
    return STATUS_SUCCESS;
}

static void finish_hash( struct hash *hash, UCHAR *output )
{
    if (hash->flags & HASH_FLAG_HMAC)
        SymCryptHmacResult( &hash->hmac_state, output );
    else
        SymCryptHashResult( hash->desc, &hash->state, output, hash->len );

    if (hash->flags & HASH_FLAG_REUSABLE) prepare_hash( hash );
}

NTSTATUS WINAPI BCryptFinishHash( BCRYPT_HASH_HANDLE handle, UCHAR *output, ULONG size, ULONG flags )
{
    struct hash *hash = get_hash_object( handle );

    TRACE( "%p, %p, %lu, %#lx\n", handle, output, size, flags );

    if (!hash) return STATUS_INVALID_HANDLE;
    if (!output || size != hash->len) return STATUS_INVALID_PARAMETER;

    finish_hash( hash, output );
    return STATUS_SUCCESS;
}

static NTSTATUS hash_single( const struct algorithm *alg, UCHAR *secret, ULONG secret_len, UCHAR *input,
                             ULONG input_len, UCHAR *output )
{
    struct hash *hash;
    NTSTATUS status;

    if ((status = create_hash( alg, secret, secret_len, 0, &hash ))) return status;

    if (hash->flags & HASH_FLAG_HMAC)
        SymCryptHmac( &hash->hmac, input, input_len, output );
    else
        SymCryptHash( hash->desc, input, input_len, output, hash->len );

    destroy_hash( hash );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptHash( BCRYPT_ALG_HANDLE handle, UCHAR *secret, ULONG secret_len, UCHAR *input, ULONG input_len,
                            UCHAR *output, ULONG output_len )
{
    struct algorithm *alg = get_alg_object( handle );

    TRACE( "%p, %p, %lu, %p, %lu, %p, %lu\n", handle, secret, secret_len, input, input_len, output, output_len );

    if (!alg) return STATUS_INVALID_HANDLE;
    if (!output || output_len != builtin_algorithms[alg->id].hash_length) return STATUS_INVALID_PARAMETER;

    return hash_single( alg, secret, secret_len, input, input_len, output );
}

static void destroy_key( struct key *key )
{
    if (!key) return;
    if (is_symmetric_key( key ))
    {
        secure_zero( key->s.secret, key->s.secret_len );
        free( key->s.secret );
        secure_zero( key->s.vector, key->s.block_size );
        DeleteCriticalSection( &key->s.cs );
    }
    else
    {
        switch (key->alg_id)
        {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            if (key->a.rsa.handle) SymCryptRsakeyFree( key->a.rsa.handle );
            break;
        case ALG_ID_ECDH:
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDH_P521:
        case ALG_ID_ECDSA:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
        case ALG_ID_ECDSA_P521:
            if (key->a.ecc.handle) SymCryptEckeyFree( key->a.ecc.handle );
            if (key->a.ecc.curve) SymCryptEcurveFree( key->a.ecc.curve );
            break;
        case ALG_ID_DH:
        case ALG_ID_DSA:
            if (key->a.dsa.handle) SymCryptDlkeyFree( key->a.dsa.handle );
            if (key->a.dsa.group) SymCryptDlgroupFree( key->a.dsa.group );
            break;
        default:
            FIXME( "algorithm %u not handled\n", key->alg_id );
            break;
        }
    }

    secure_zero( &key->hdr.magic, sizeof(key->hdr.magic) );
    _aligned_free( key );
}

static NTSTATUS alloc_aes_key( struct key *key, enum chain_mode mode, ULONG block_size, const UCHAR *secret,
                               ULONG secret_len )
{
    key->s.mode       = mode;
    key->s.block_size = block_size;

    if (!(key->s.secret = malloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( key->s.secret, secret, secret_len );
    key->s.secret_len = secret_len;

    InitializeCriticalSection( &key->s.cs );

    switch (mode)
    {
    case CHAIN_MODE_ECB:
    case CHAIN_MODE_CBC:
    case CHAIN_MODE_CFB:
        if (SymCryptAesExpandKey( &key->s.aes.handle, secret, secret_len )) return STATUS_INTERNAL_ERROR;
        break;

    case CHAIN_MODE_GCM:
        if (SymCryptGcmExpandKey( &key->s.gcm.handle, SymCryptAesBlockCipher, secret, secret_len ))
            return STATUS_INTERNAL_ERROR;
        break;

    default:
        FIXME( "unhandled mode %u\n", mode );
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS alloc_key( enum alg_id alg_id, ULONG flags, struct key **ret_key )
{
    struct key *key;

    if (!(key = _aligned_malloc( sizeof(struct key), TYPE_ALIGNMENT(struct key) ))) return STATUS_NO_MEMORY;
    memset( key, 0, sizeof(*key) );
    key->hdr.magic = MAGIC_KEY;
    key->alg_id    = alg_id;
    key->flags     = flags;

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS validate_len_aes( const BCRYPT_KEY_LENGTHS_STRUCT *key_lengths, ULONG secret_len, ULONG *ret_len )
{
    ULONG size;

    if (secret_len > (size = key_lengths->dwMaxLength / 8))
    {
        WARN( "secret_len %lu exceeds key max length %lu, setting to maximum\n", secret_len, size );
        *ret_len = size;
        return STATUS_SUCCESS;
    }
    else if (secret_len < (size = key_lengths->dwMinLength / 8))
    {
        WARN( "secret_len %lu is less than minimum key length %lu\n", secret_len, size );
        return STATUS_INVALID_PARAMETER;
    }
    else if (key_lengths->dwIncrement && (secret_len * 8 - key_lengths->dwMinLength) % key_lengths->dwIncrement)
    {
        WARN( "secret_len %lu is not a valid key length\n", secret_len );
        return STATUS_INVALID_PARAMETER;
    }

    *ret_len = secret_len;
    return STATUS_SUCCESS;
}

static NTSTATUS generate_symmetric_key( const struct algorithm *alg, const UCHAR *secret, ULONG secret_len,
                                        struct key **ret_key )
{
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    struct key *key;
    NTSTATUS status;
    ULONG size;

    if ((status = get_alg_property( alg, BCRYPT_KEY_LENGTHS, (UCHAR *)&key_lengths, sizeof(key_lengths), &size )))
        return status;

    if ((status = alloc_key( alg->id, 0, &key ))) return status;

    switch (alg->id)
    {
    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        if ((status = validate_len_aes( &key_lengths, secret_len, &secret_len )) ||
            (status = alloc_aes_key( key, alg->chain_mode, BLOCK_LENGTH_AES, secret, secret_len )))
        {
            destroy_key( key );
            return status;
        }
        break;

    case ALG_ID_PBKDF2:
    case ALG_ID_TLS1_1_KDF:
    case ALG_ID_TLS1_2_KDF:
    case ALG_ID_HKDF:
        if (secret_len > key_lengths.dwMaxLength / 8 || secret_len < key_lengths.dwMinLength / 8)
        {
            destroy_key( key );
            return STATUS_INVALID_PARAMETER;
        }
        if (!(key->s.secret = malloc( secret_len )))
        {
            destroy_key( key );
            return STATUS_NO_MEMORY;
        }
        memcpy( key->s.secret, secret, secret_len );
        key->s.block_size = key->s.secret_len = secret_len;
        break;

    default:
        FIXME( "unhandled algorithm %u\n", alg->id );
        destroy_key( key );
        return STATUS_NOT_SUPPORTED;
    }

    *ret_key = key;
    return status;
}

NTSTATUS WINAPI BCryptGenerateSymmetricKey( BCRYPT_ALG_HANDLE handle, BCRYPT_KEY_HANDLE *ret_handle, UCHAR *object,
                                            ULONG object_len, UCHAR *secret, ULONG secret_len, ULONG flags )
{
    const struct algorithm *alg = get_alg_object( handle );
    struct key *key;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %lu, %p, %lu, %#lx\n", handle, ret_handle, object, object_len, secret, secret_len, flags );
    if (object) FIXME( "ignoring object buffer\n" );

    if (!alg) return STATUS_INVALID_HANDLE;

    if ((status = generate_symmetric_key( alg, secret, secret_len, &key ))) return status;
    *ret_handle = key;

    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

static NTSTATUS decrypt_aes_ecb( const struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                                 ULONG output_len, ULONG *ret_len, ULONG flags )
{
    ULONG offset = input_len, block_size = key->s.block_size;
    UCHAR buf[BLOCK_LENGTH_AES];
    NTSTATUS status = STATUS_SUCCESS;

    if (input_len & (block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;
    if (!output) return STATUS_SUCCESS;

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (output_len + block_size < *ret_len || input_len < block_size) return STATUS_BUFFER_TOO_SMALL;
        offset -= block_size;
    }
    else if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    SymCryptEcbDecrypt( SymCryptAesBlockCipher, &key->s.aes.handle, input, output, offset );

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        SymCryptEcbDecrypt( SymCryptAesBlockCipher, &key->s.aes.handle, input + offset, buf, block_size );

        if (buf[block_size - 1] <= block_size)
        {
            *ret_len -= buf[block_size - 1];
            if (output_len < *ret_len) status = STATUS_BUFFER_TOO_SMALL;
            else memcpy( output + offset, buf, block_size - buf[block_size - 1] );
        }
        else status = STATUS_UNSUCCESSFUL; /* FIXME: invalid padding */
    }

    return status;
}

static NTSTATUS decrypt_aes_gcm( struct key *key, const UCHAR *input, ULONG input_len,
                                 const BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *info, UCHAR *output, ULONG output_len,
                                 ULONG *ret_len )
{
    SYMCRYPT_ERROR error;

    if (!info || !info->pbNonce || !info->pbTag || info->cbTag < 12 || info->cbTag > 16)
        return STATUS_INVALID_PARAMETER;
    if (info->dwFlags & BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG) FIXME( "call chaining not implemented\n" );

    if (!output) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    EnterCriticalSection( &key->s.cs );

    SymCryptGcmInit( &key->s.gcm.state, &key->s.gcm.handle, info->pbNonce, info->cbNonce );
    SymCryptGcmAuthPart( &key->s.gcm.state, info->pbAuthData, info->cbAuthData );
    SymCryptGcmDecryptPart( &key->s.gcm.state, input, output, input_len );
    error = SymCryptGcmDecryptFinal( &key->s.gcm.state, info->pbTag, info->cbTag );

    LeaveCriticalSection( &key->s.cs );

    if (error == SYMCRYPT_AUTHENTICATION_FAILURE) return STATUS_AUTH_TAG_MISMATCH;
    if (error) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS decrypt_aes_vector( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *iv, UCHAR *output,
                                    ULONG output_len, ULONG *ret_len, ULONG flags )
{
    UCHAR buf[BLOCK_LENGTH_AES], *out;
    const UCHAR *in;
    ULONG bytes_left = input_len, block_size = key->s.block_size;
    NTSTATUS status = STATUS_SUCCESS;

    if (input_len & (block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;
    if (!output) return STATUS_SUCCESS;

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (output_len + block_size < *ret_len || input_len < block_size) return STATUS_BUFFER_TOO_SMALL;
        bytes_left -= block_size;
    }
    else if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    EnterCriticalSection( &key->s.cs );

    if (iv) memcpy( key->s.vector, iv, block_size );

    in = input;
    out = output;
    while (bytes_left >= block_size)
    {
        if (key->s.mode == CHAIN_MODE_CFB)
            SymCryptCfbDecrypt( SymCryptAesBlockCipher, 1, &key->s.aes.handle, key->s.vector, in, out, block_size );
        else
            SymCryptCbcDecrypt( SymCryptAesBlockCipher, &key->s.aes.handle, key->s.vector, in, out, block_size );

        bytes_left -= block_size;
        in += block_size;
        out += block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (key->s.mode == CHAIN_MODE_CFB)
            SymCryptCfbDecrypt( SymCryptAesBlockCipher, 1, &key->s.aes.handle, key->s.vector, in, buf, block_size );
        else
            SymCryptCbcDecrypt( SymCryptAesBlockCipher, &key->s.aes.handle, key->s.vector, in, buf, block_size );

        if (buf[block_size - 1] <= block_size)
        {
            *ret_len -= buf[block_size - 1];
            if (output_len < *ret_len) status = STATUS_BUFFER_TOO_SMALL;
            else memcpy( out, buf, block_size - buf[block_size - 1] );
        }
        else status = STATUS_UNSUCCESSFUL; /* FIXME: invalid padding */
    }

    if (iv) memcpy( iv, key->s.vector, block_size );

    LeaveCriticalSection( &key->s.cs );
    return status;
}

static NTSTATUS decrypt_symmetric( struct key *key, const UCHAR *input, ULONG input_len,
                                   const BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *info, UCHAR *iv, ULONG iv_len,
                                   UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    *ret_len = input_len;

    if (key->s.mode == CHAIN_MODE_GCM && (flags & BCRYPT_BLOCK_PADDING)) return STATUS_INVALID_PARAMETER;
    if (key->s.mode == CHAIN_MODE_ECB && iv) return STATUS_INVALID_PARAMETER;

    switch (key->alg_id)
    {
    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        switch (key->s.mode)
        {
        case CHAIN_MODE_ECB:
            return decrypt_aes_ecb( key, input, input_len, output, output_len, ret_len, flags );

        case CHAIN_MODE_GCM:
            return decrypt_aes_gcm( key, input, input_len, info, output, output_len, ret_len );

        default:
            if (iv && iv_len != key->s.block_size) return STATUS_INVALID_PARAMETER;
            return decrypt_aes_vector( key, input, input_len, iv, output, output_len, ret_len, flags );
        }
    default:
        FIXME( "unhandled algorithm %u\n", key->alg_id );
        break;
    }

    return STATUS_NOT_SUPPORTED;
}

static const SYMCRYPT_HASH *get_hash_from_str( const WCHAR *str )
{
    if (!str) return NULL;
    if (!wcscmp( str, BCRYPT_MD5_ALGORITHM ))    return SymCryptMd5Algorithm;
    if (!wcscmp( str, BCRYPT_SHA1_ALGORITHM ))   return SymCryptSha1Algorithm;
    if (!wcscmp( str, BCRYPT_SHA256_ALGORITHM )) return SymCryptSha256Algorithm;
    if (!wcscmp( str, BCRYPT_SHA384_ALGORITHM )) return SymCryptSha384Algorithm;
    if (!wcscmp( str, BCRYPT_SHA512_ALGORITHM )) return SymCryptSha512Algorithm;

    FIXME( "unhandled algorithm %s\n", debugstr_w(str) );
    return NULL;
}

static NTSTATUS decrypt_rsa( const struct key *key, const UCHAR *input, ULONG input_len, const void *padding,
                             UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    SIZE_T size = len_from_bitlen( key->a.bitlen );

    if (!flags || flags == BCRYPT_PAD_NONE)
    {
        if (input_len % size) return STATUS_INVALID_PARAMETER;

        *ret_len = size;
        if (output_len < size)
        {
            if (output) return STATUS_BUFFER_TOO_SMALL;
            return STATUS_SUCCESS;
        }
        if (SymCryptRsaRawDecrypt( key->a.rsa.handle, input, input_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, output,
                                   output_len )) return STATUS_INTERNAL_ERROR;
    }
    else if (flags == BCRYPT_PAD_PKCS1)
    {
        if (SymCryptRsaPkcs1Decrypt( key->a.rsa.handle, input, input_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, output,
                                     output_len, &size )) return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else if (flags == BCRYPT_PAD_OAEP)
    {
        const BCRYPT_OAEP_PADDING_INFO *pad = padding;
        const SYMCRYPT_HASH *hash_desc;

        if (!pad) return STATUS_INVALID_PARAMETER;
        hash_desc = get_hash_from_str( pad->pszAlgId );

        if (SymCryptRsaOaepDecrypt( key->a.rsa.handle, input, input_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, hash_desc,
                                    pad->pbLabel, pad->cbLabel, 0, output, output_len, &size ))
            return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else
    {
        FIXME( "unsupported flags %#lx", flags );
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDecrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len, void *padding, UCHAR *iv,
                               ULONG iv_len, UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = get_key_object( handle );
    NTSTATUS status;

    TRACE( "%p, %p, %lu, %p, %p, %lu, %p, %lu, %p, %#lx\n", handle, input, input_len, padding, iv, iv_len, output,
           output_len, ret_len, flags );

    if (!key) return STATUS_INVALID_HANDLE;

    if (is_symmetric_key( key ))
    {
        if (flags & ~BCRYPT_BLOCK_PADDING)
        {
            FIXME( "flags %#lx not supported\n", flags );
            return STATUS_NOT_IMPLEMENTED;
        }

        status = decrypt_symmetric( key, input, input_len, padding, iv, iv_len, output, output_len, ret_len, flags );
    }
    else
    {
        if (key->alg_id != ALG_ID_RSA) return STATUS_NOT_SUPPORTED;
        if (!(key->flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

        status = decrypt_rsa( key, input, input_len, padding, output, output_len, ret_len, flags );
    }
    return status;
}

/* AES Key Wrap Algorithm (RFC3394) */
static NTSTATUS unwrap_aes( const UCHAR *secret, ULONG secret_len, const UCHAR *cipher, ULONG cipher_len, UCHAR *plain )
{
    UCHAR a[8], *r, b[16];
    ULONG len = 16, t, i, n = cipher_len / 8;
    NTSTATUS status;
    int j;
    struct key *key;

    memcpy( a, cipher, 8 );
    r = plain;
    memcpy( r, cipher + 8, 8 * n );

    if ((status = alloc_key( ALG_ID_AES, 0, &key ))) return status;
    if ((status = alloc_aes_key( key, CHAIN_MODE_ECB, BLOCK_LENGTH_AES, secret, secret_len )))
    {
        destroy_key( key );
        return status;
    }

    for (j = 5; j >= 0; j--)
    {
        r = plain + (n - 1) * 8;
        for (i = n; i >= 1; i--)
        {
            memcpy( b, a, 8 );
            t = n * j + i;
            b[7] ^= t;
            b[6] ^= t >> 8;
            b[5] ^= t >> 16;
            b[4] ^= t >> 24;

            memcpy( b + 8, r, 8 );
            decrypt_aes_ecb( key, b, 16, b, 16, &len, 0 );
            memcpy( a, b, 8 );
            memcpy( r, b + 8, 8 );
            r -= 8;
        }
    }

    destroy_key( key );

    for (i = 0; i < 8; i++) if (a[i] != 0xa6) return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;
}

static NTSTATUS import_key( const struct algorithm *alg, const struct key *decrypt_key, const WCHAR *type,
                            const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    ULONG len;
    NTSTATUS status;

    if (decrypt_key && wcscmp( type, BCRYPT_AES_WRAP_KEY_BLOB ))
    {
        FIXME( "decryption of key not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!wcscmp( type, BCRYPT_KEY_DATA_BLOB ))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *header = (BCRYPT_KEY_DATA_BLOB_HEADER *)input;

        if (input_len < sizeof(BCRYPT_KEY_DATA_BLOB_HEADER)) return STATUS_BUFFER_TOO_SMALL;
        if (header->dwMagic != BCRYPT_KEY_DATA_BLOB_MAGIC) return STATUS_INVALID_PARAMETER;
        if (header->dwVersion != BCRYPT_KEY_DATA_BLOB_VERSION1)
        {
            FIXME( "unknown key data blob version %lu\n", header->dwVersion );
            return STATUS_INVALID_PARAMETER;
        }
        len = header->cbKeyData;
        if (len + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) > input_len) return STATUS_INVALID_PARAMETER;

        return generate_symmetric_key( alg, (UCHAR *)&header[1], len, ret_key );
    }
    else if (!wcscmp( type, BCRYPT_OPAQUE_KEY_BLOB ))
    {
        if (input_len < sizeof(len)) return STATUS_BUFFER_TOO_SMALL;
        len = *(ULONG *)input;
        if (len + sizeof(len) > input_len) return STATUS_INVALID_PARAMETER;

        return generate_symmetric_key( alg, input + sizeof(len), len, ret_key);
    }
    else if (!wcscmp( type, BCRYPT_AES_WRAP_KEY_BLOB ))
    {
        UCHAR output[32];

        if (!decrypt_key || input_len < 8) return STATUS_INVALID_PARAMETER;

        len = input_len - 8;
        if (len < BLOCK_LENGTH_AES || len > sizeof(output) || len & (BLOCK_LENGTH_AES - 1))
            return STATUS_INVALID_PARAMETER;

        if ((status = unwrap_aes( decrypt_key->s.secret, decrypt_key->s.secret_len, input, len, output )))
            return status;

        return generate_symmetric_key( alg, output, len, ret_key );
    }

    FIXME( "unsupported blob type %s\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptImportKey( BCRYPT_ALG_HANDLE handle, BCRYPT_KEY_HANDLE decrypt_key_handle, const WCHAR *type,
                                 BCRYPT_KEY_HANDLE *ret_handle, UCHAR *object, ULONG object_len, UCHAR *input,
                                 ULONG input_len, ULONG flags )
{
    const struct algorithm *alg = get_alg_object( handle );
    struct key *key;
    const struct key *decrypt_key = NULL;
    NTSTATUS status;

    TRACE( "%p, %p, %s, %p, %p, %lu, %p, %lu, %#lx\n", handle, decrypt_key_handle, debugstr_w(type), ret_handle,
           object, object_len, input, input_len, flags );

    if (!alg || !is_symmetric_alg( alg )) return STATUS_INVALID_HANDLE;
    if (!ret_handle || !type || !input) return STATUS_INVALID_PARAMETER;
    if (decrypt_key_handle && !(decrypt_key = get_key_object( decrypt_key_handle ))) return STATUS_INVALID_HANDLE;

    if ((status = import_key( alg, decrypt_key, type, input, input_len, &key ))) return status;
    *ret_handle = key;

    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

static NTSTATUS encrypt_aes_ecb( const struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                                 ULONG output_len, ULONG *ret_len, ULONG flags )
{
    ULONG bytes_left, offset, block_size = key->s.block_size;
    UCHAR buf[BLOCK_LENGTH_AES];

    if (flags & BCRYPT_BLOCK_PADDING) *ret_len = (input_len + block_size) & ~(block_size - 1);
    else if (input_len & (block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;

    if (!output) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    bytes_left = input_len % block_size;
    offset = input_len - bytes_left;

    SymCryptEcbEncrypt( SymCryptAesBlockCipher, &key->s.aes.handle, input, output, offset );

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        memcpy( buf, input + offset, bytes_left );
        memset( buf + bytes_left, block_size - bytes_left, block_size - bytes_left );

        SymCryptEcbEncrypt( SymCryptAesBlockCipher, &key->s.aes.handle, buf, output + offset, block_size );
    }

    return STATUS_SUCCESS;
}

static NTSTATUS encrypt_aes_gcm( struct key *key, const UCHAR *input, ULONG input_len,
                                 const BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *info, UCHAR *output, ULONG output_len,
                                 ULONG *ret_len )
{
    if (!info || !info->pbNonce || !info->pbTag || info->cbTag < 12 || info->cbTag > 16)
        return STATUS_INVALID_PARAMETER;
    if (info->dwFlags & BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG) FIXME( "call chaining not implemented\n" );

    if (input && !input_len) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    EnterCriticalSection( &key->s.cs );

    SymCryptGcmInit( &key->s.gcm.state, &key->s.gcm.handle, info->pbNonce, info->cbNonce );
    SymCryptGcmAuthPart( &key->s.gcm.state, info->pbAuthData, info->cbAuthData );
    SymCryptGcmEncryptPart( &key->s.gcm.state, input, output, input_len );
    SymCryptGcmEncryptFinal( &key->s.gcm.state, info->pbTag, info->cbTag );

    LeaveCriticalSection( &key->s.cs );
    return STATUS_SUCCESS;
}

static NTSTATUS encrypt_aes_vector( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *iv, UCHAR *output,
                                    ULONG output_len, ULONG *ret_len, ULONG flags )
{
    ULONG bytes_left = input_len, block_size = key->s.block_size;
    UCHAR buf[BLOCK_LENGTH_AES], *out;
    const UCHAR *in;

    if (flags & BCRYPT_BLOCK_PADDING) *ret_len = (input_len + block_size) & ~(block_size - 1);
    else if (input_len & (block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;

    if (!output) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    EnterCriticalSection( &key->s.cs );

    if (iv) memcpy( key->s.vector, iv, block_size );

    in = input;
    out = output;
    while (bytes_left >= block_size)
    {
        if (key->s.mode == CHAIN_MODE_CFB)
            SymCryptCfbEncrypt( SymCryptAesBlockCipher, 1, &key->s.aes.handle, key->s.vector, in, out, block_size );
        else
            SymCryptCbcEncrypt( SymCryptAesBlockCipher, &key->s.aes.handle, key->s.vector, in, out, block_size );

        bytes_left -= block_size;
        in += block_size;
        out += block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        memcpy( buf, in, bytes_left );
        memset( buf + bytes_left, block_size - bytes_left, block_size - bytes_left );

        if (key->s.mode == CHAIN_MODE_CFB)
            SymCryptCfbEncrypt( SymCryptAesBlockCipher, 1, &key->s.aes.handle, key->s.vector, buf, out, block_size );
        else
            SymCryptCbcEncrypt( SymCryptAesBlockCipher, &key->s.aes.handle, key->s.vector, buf, out, block_size );
    }

    if (iv) memcpy( iv, key->s.vector, block_size );

    LeaveCriticalSection( &key->s.cs );
    return STATUS_SUCCESS;
}

static NTSTATUS encrypt_symmetric( struct key *key, const UCHAR *input, ULONG input_len,
                                   const BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *info, UCHAR *iv, ULONG iv_len,
                                   UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    *ret_len = input_len;

    if (key->s.mode == CHAIN_MODE_ECB && iv) return STATUS_INVALID_PARAMETER;
    if (key->s.mode == CHAIN_MODE_GCM && (flags & BCRYPT_BLOCK_PADDING)) return STATUS_INVALID_PARAMETER;

    switch (key->alg_id)
    {
    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        switch (key->s.mode)
        {
        case CHAIN_MODE_ECB:
            return encrypt_aes_ecb( key, input, input_len, output, output_len, ret_len, flags );

        case CHAIN_MODE_GCM:
            return encrypt_aes_gcm( key, input, input_len, info, output, output_len, ret_len );

        default:
            if (iv && iv_len != key->s.block_size) return STATUS_INVALID_PARAMETER;
            return encrypt_aes_vector( key, input, input_len, iv, output, output_len, ret_len, flags );
        }
    default:
        FIXME( "unhandled algorithm %u\n", key->alg_id );
        break;
    }

    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS encrypt_rsa( const struct key *key, const UCHAR *input, ULONG input_len, const void *padding,
                             UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    SIZE_T size = len_from_bitlen( key->a.bitlen );

    *ret_len = size;
    if (output_len < size)
    {
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    if (!flags || flags == BCRYPT_PAD_NONE)
    {
        if (input_len % size) return STATUS_INVALID_PARAMETER;

        if (SymCryptRsaRawEncrypt( key->a.rsa.handle, input, input_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, output,
                                   output_len )) return STATUS_INTERNAL_ERROR;
    }
    else if (flags == BCRYPT_PAD_PKCS1)
    {
        if (SymCryptRsaPkcs1Encrypt( key->a.rsa.handle, input, input_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, output,
                                     output_len, &size )) return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else if (flags == BCRYPT_PAD_OAEP)
    {
        const BCRYPT_OAEP_PADDING_INFO *pad = padding;
        const SYMCRYPT_HASH *hash_desc = SymCryptSha1Algorithm;
        const BYTE *label = NULL;
        SIZE_T label_len = 0;

        if (pad)
        {
            hash_desc = get_hash_from_str( pad->pszAlgId );
            label = pad->pbLabel;
            label_len = pad->cbLabel;
        }
        if (SymCryptRsaOaepEncrypt( key->a.rsa.handle, input, input_len, hash_desc, label, label_len, 0,
                                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, output, output_len, &size ))
            return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else
    {
        FIXME( "unsupported flags %#lx", flags );
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptEncrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len, void *padding, UCHAR *iv,
                               ULONG iv_len, UCHAR *output, ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = get_key_object( handle );
    NTSTATUS status;

    TRACE( "%p, %p, %lu, %p, %p, %lu, %p, %lu, %p, %#lx\n", handle, input, input_len, padding, iv, iv_len, output,
           output_len, ret_len, flags );

    if (!key) return STATUS_INVALID_HANDLE;

    if (is_symmetric_key( key ))
    {
        if (flags & ~BCRYPT_BLOCK_PADDING)
        {
            FIXME( "flags %#lx not implemented\n", flags );
            return STATUS_NOT_IMPLEMENTED;
        }

        status = encrypt_symmetric( key, input, input_len, padding, iv, iv_len, output, output_len, ret_len, flags );
    }
    else
    {
        if (key->alg_id != ALG_ID_RSA) return STATUS_NOT_SUPPORTED;
        if (!(key->flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

        status = encrypt_rsa( key, input, input_len, padding, output, output_len, ret_len, flags );
    }
    return status;
}

/* AES Key Wrap Algorithm (RFC3394) */
static NTSTATUS wrap_aes( const UCHAR *secret, ULONG secret_len, const UCHAR *plain, ULONG plain_len, UCHAR *cipher )
{
    UCHAR *a, *r, b[16];
    ULONG len = 16, t, i, j, n = plain_len / 8;
    NTSTATUS status;
    struct key *key;

    a = cipher;
    r = cipher + 8;

    memset( a, 0xa6, 8 );
    memcpy( r, plain, 8 * n );

    if ((status = alloc_key( ALG_ID_AES, 0, &key ))) return status;
    if ((status = alloc_aes_key( key, CHAIN_MODE_ECB, BLOCK_LENGTH_AES, secret, secret_len )))
    {
        destroy_key( key );
        return status;
    }

    for (j = 0; j <= 5; j++)
    {
        r = cipher + 8;
        for (i = 1; i <= n; i++)
        {
            memcpy( b, a, 8 );
            memcpy( b + 8, r, 8 );
            encrypt_aes_ecb( key, b, 16, b, 16, &len, 0 );
            memcpy( a, b, 8 );
            t = n * j + i;
            a[7] ^= t;
            a[6] ^= t >> 8;
            a[5] ^= t >> 16;
            a[4] ^= t >> 24;
            memcpy( r, b + 8, 8 );
            r += 8;
        }
    }

    destroy_key( key );
    return STATUS_SUCCESS;
}

static NTSTATUS export_symmetric_key( const struct key *key, const struct key *encrypt_key, const WCHAR *type,
                                      UCHAR *output, ULONG output_len, ULONG *ret_len )
{
    NTSTATUS status;

    if (encrypt_key && wcscmp( type, BCRYPT_AES_WRAP_KEY_BLOB ))
    {
        FIXME( "encryption of key not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!wcscmp( type, BCRYPT_KEY_DATA_BLOB ))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *header = (BCRYPT_KEY_DATA_BLOB_HEADER *)output;
        ULONG size = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key->s.secret_len;

        *ret_len = size;
        if (output)
        {
            if (output_len < size) return STATUS_BUFFER_TOO_SMALL;
            header->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
            header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
            header->cbKeyData = key->s.secret_len;
            memcpy( &header[1], key->s.secret, key->s.secret_len );
        }
        return STATUS_SUCCESS;
    }
    else if (!wcscmp( type, BCRYPT_OPAQUE_KEY_BLOB ))
    {
        ULONG size = sizeof(ULONG) + key->s.secret_len;

        *ret_len = size;
        if (output)
        {
            if (output_len < size) return STATUS_BUFFER_TOO_SMALL;
            *(ULONG *)output = key->s.secret_len;
            memcpy( output + sizeof(ULONG), key->s.secret, key->s.secret_len );
        }
        return STATUS_SUCCESS;
    }
    else if (!wcscmp( type, BCRYPT_AES_WRAP_KEY_BLOB ))
    {
        ULONG size = key->s.secret_len + 8;

        if (!encrypt_key) return STATUS_INVALID_PARAMETER;

        *ret_len = size;
        if (output)
        {
            if (output_len < size) return STATUS_BUFFER_TOO_SMALL;
            if ((status = wrap_aes( encrypt_key->s.secret, encrypt_key->s.secret_len, key->s.secret,
                                    key->s.secret_len, output ))) return status;
        }
        return STATUS_SUCCESS;
    }

    FIXME( "unsupported blob type %s\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS export_rsa_key( const struct key *key, UCHAR *output, ULONG output_len, ULONG flags, ULONG *ret_len )
{
    BCRYPT_RSAKEY_BLOB *blob = (BCRYPT_RSAKEY_BLOB *)output;
#ifdef __REACTOS__
    BYTE *mod, *primes[2] = { 0 }, *exponents[2], *coefficient, *private_exp, *ptr = (BYTE *)(blob + 1);
#else
    BYTE *mod, *primes[2] = {}, *exponents[2], *coefficient, *private_exp, *ptr = (BYTE *)(blob + 1);
#endif
    UINT64 pubexp;
#ifdef __REACTOS__
    SIZE_T mod_size, pubexp_size = sizeof(pubexp), primes_sizes[2] = { 0 };
#else
    SIZE_T mod_size, pubexp_size = sizeof(pubexp), primes_sizes[2] = {};
#endif
    ULONG size, magic, num_primes = 0;

    pubexp_size = SymCryptRsakeySizeofPublicExponent( key->a.rsa.handle, 0 );
    mod_size = SymCryptRsakeySizeofModulus( key->a.rsa.handle );

    if (flags & KEY_EXPORT_FLAG_PRIVATE)
    {
        if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;
        primes_sizes[0] = SymCryptRsakeySizeofPrime( key->a.rsa.handle, 0 );
        primes_sizes[1] = SymCryptRsakeySizeofPrime( key->a.rsa.handle, 1 );
        num_primes = 2;
    }
    else primes_sizes[0] = primes_sizes[1] = 0;

    size = sizeof(*blob) + pubexp_size + mod_size + primes_sizes[0] + primes_sizes[1];
    if (flags & KEY_EXPORT_FLAG_RSA_FULL) size += primes_sizes[0] * 2 + primes_sizes[1] + mod_size;
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    if (flags & KEY_EXPORT_FLAG_RSA_FULL) magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
    else if (flags & KEY_EXPORT_FLAG_PRIVATE) magic = BCRYPT_RSAPRIVATE_MAGIC;
    else magic = BCRYPT_RSAPUBLIC_MAGIC;

    blob->Magic       = magic;
    blob->BitLength   = key->a.bitlen;
    blob->cbPublicExp = pubexp_size;
    blob->cbModulus   = mod_size;
    blob->cbPrime1    = primes_sizes[0];
    blob->cbPrime2    = primes_sizes[1];

    mod = ptr + pubexp_size;
    primes[0] = mod + mod_size;
    primes[1] = mod + mod_size + primes_sizes[0];

    if (SymCryptRsakeyGetValue( key->a.rsa.handle, mod, mod_size, &pubexp, 1, primes, primes_sizes, num_primes,
                                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0 )) return STATUS_INTERNAL_ERROR;

    memcpy( ptr, &pubexp, pubexp_size );

    if (flags & KEY_EXPORT_FLAG_RSA_FULL)
    {
        exponents[0] = primes[1]    + primes_sizes[1];
        exponents[1] = exponents[0] + primes_sizes[0];
        coefficient  = exponents[1] + primes_sizes[1];
        private_exp  = coefficient  + primes_sizes[0];

        if (SymCryptRsakeyGetCrtValue( key->a.rsa.handle, exponents, primes_sizes, 2, coefficient, primes_sizes[0],
                                       private_exp, mod_size, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0 ))
            return STATUS_INTERNAL_ERROR;
    }

    *ret_len = size;
    return STATUS_SUCCESS;
}

static DWORD get_ecc_magic( enum alg_id alg, BOOL private )
{
    switch (alg)
    {
    case ALG_ID_ECDH:       return private ? BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC  : BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC;
    case ALG_ID_ECDH_P256:  return private ? BCRYPT_ECDH_PRIVATE_P256_MAGIC     : BCRYPT_ECDH_PUBLIC_P256_MAGIC;
    case ALG_ID_ECDH_P384:  return private ? BCRYPT_ECDH_PRIVATE_P384_MAGIC     : BCRYPT_ECDH_PUBLIC_P384_MAGIC;
    case ALG_ID_ECDH_P521:  return private ? BCRYPT_ECDH_PRIVATE_P521_MAGIC     : BCRYPT_ECDH_PUBLIC_P521_MAGIC;

    case ALG_ID_ECDSA:      return private ? BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC : BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC;
    case ALG_ID_ECDSA_P256: return private ? BCRYPT_ECDSA_PRIVATE_P256_MAGIC    : BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    case ALG_ID_ECDSA_P384: return private ? BCRYPT_ECDSA_PRIVATE_P384_MAGIC    : BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
    case ALG_ID_ECDSA_P521: return private ? BCRYPT_ECDSA_PRIVATE_P521_MAGIC    : BCRYPT_ECDSA_PUBLIC_P521_MAGIC;
    default:
        FIXME( "unhandled algorithm %u\n", alg );
        return 0;
    }
}

static NTSTATUS export_ecc_key( const struct key *key, UCHAR *output, ULONG output_len, ULONG flags, ULONG *ret_len )
{
    BCRYPT_ECCKEY_BLOB *blob = (BCRYPT_ECCKEY_BLOB *)output;
    UINT32 pub_size = SymCryptEckeySizeofPublicKey( key->a.ecc.handle, SYMCRYPT_ECPOINT_FORMAT_XY ), priv_size = 0;
    UCHAR *pub = (UCHAR *)(blob + 1), *priv = NULL;
    ULONG size, key_size = len_from_bitlen( key->a.bitlen );

    size = sizeof(*blob) + key_size * 2;
    if (flags & KEY_EXPORT_FLAG_PRIVATE)
    {
        if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;
        priv_size = SymCryptEckeySizeofPrivateKey( key->a.ecc.handle );
        priv = pub + key_size * 2;
        size += key_size;
    }
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    blob->dwMagic = get_ecc_magic( key->alg_id, flags & KEY_EXPORT_FLAG_PRIVATE );
    blob->cbKey   = key_size;

    if (SymCryptEckeyGetValue( key->a.ecc.handle, priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                               SYMCRYPT_ECPOINT_FORMAT_XY, 0 )) return STATUS_INTERNAL_ERROR;
    *ret_len = size;
    return STATUS_SUCCESS;
}

static NTSTATUS export_dsa_key( const struct key *key, UCHAR *output, ULONG output_len, ULONG flags, ULONG *ret_len )
{
    BCRYPT_DSA_KEY_BLOB *blob = (BCRYPT_DSA_KEY_BLOB *)output;
    UINT32 key_size = len_from_bitlen( key->a.bitlen ), priv_size = 0;
    UCHAR *prime, *generator, *pub, *priv = NULL;
    ULONG size;

    size = sizeof(*blob) + key_size * 3;
    if (flags & KEY_EXPORT_FLAG_PRIVATE)
    {
        if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;
        priv_size = 20;
        size += priv_size;
    }
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    blob->dwMagic = (flags & KEY_EXPORT_FLAG_PRIVATE) ? BCRYPT_DSA_PRIVATE_MAGIC : BCRYPT_DSA_PUBLIC_MAGIC;
    blob->cbKey   = key_size;

    prime = (UCHAR *)(blob + 1);
    generator = prime + key_size;

    if (SymCryptDlgroupGetValue( key->a.dsa.group, prime, key_size, blob->q, sizeof(blob->q), generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, NULL, blob->Seed, sizeof(blob->Seed),
                                 (UINT32 *)blob->Count )) return STATUS_INTERNAL_ERROR;
    pub = generator + key_size;
    if (flags & KEY_EXPORT_FLAG_PRIVATE) priv = pub + key_size;

    if (SymCryptDlkeyGetValue( key->a.dsa.handle, priv, priv_size, pub, key_size, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                               0 )) return STATUS_INTERNAL_ERROR;
    *ret_len = size;
    return STATUS_SUCCESS;
}

static NTSTATUS export_legacy_dsa_key( const struct key *key, UCHAR *output, ULONG output_len, ULONG flags,
                                       ULONG *ret_len )
{
    BLOBHEADER *hdr = (BLOBHEADER *)output;
    DSSPUBKEY *dsskey = (DSSPUBKEY *)(hdr + 1);
    UINT32 key_size = len_from_bitlen( key->a.bitlen ), pub_size = 0, priv_size = 0;
    UCHAR *prime_p, *prime_q, *generator, *pub = NULL, *priv = NULL, *seed, *count;
    ULONG size;

    if ((flags & KEY_EXPORT_FLAG_PRIVATE) && !(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;

    size = sizeof(*hdr) + sizeof(*dsskey) + key_size * 2 + 20 /* q */ + 20 /* x or y */ + 20 /* seed */ + 4 /* count */;
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    hdr->bType    = (flags & KEY_EXPORT_FLAG_PRIVATE) ? PRIVATEKEYBLOB : PUBLICKEYBLOB;
    hdr->bVersion = 2;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_DSS_SIGN;

    dsskey->magic  = (flags & KEY_EXPORT_FLAG_PRIVATE) ? MAGIC_DSS2 : MAGIC_DSS1;
    dsskey->bitlen = key->a.bitlen;

    prime_p = (UCHAR *)(dsskey + 1);
    prime_q = prime_p + key_size;
    generator = prime_q + 20;
    seed = generator + key_size + 20;
    count = seed + 20;

    if (SymCryptDlgroupGetValue( key->a.dsa.group, prime_p, key_size, prime_q, 20, generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, NULL, seed, 20, (UINT32 *)count ))
        return STATUS_INTERNAL_ERROR;

    if (flags & KEY_EXPORT_FLAG_PRIVATE)
    {
        priv = generator + key_size;
        priv_size = 20;
    }
    else
    {
        pub = generator + key_size;
        pub_size = 20;
    }

    if (SymCryptDlkeyGetValue( key->a.dsa.handle, priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
                               0 )) return STATUS_INTERNAL_ERROR;
    *ret_len = size;
    return STATUS_SUCCESS;
}

static NTSTATUS export_dh_key( const struct key *key, UCHAR *output, ULONG output_len, ULONG flags, ULONG *ret_len )
{
    BCRYPT_DH_KEY_BLOB *blob = (BCRYPT_DH_KEY_BLOB *)output;
    UCHAR *modulus, *generator, *pub, *priv = NULL;
    ULONG size, priv_size = 0, key_size = len_from_bitlen( key->a.bitlen );

    size = sizeof(*blob) + key_size * 3;
    if (flags & KEY_EXPORT_FLAG_PRIVATE)
    {
        if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;
        priv_size = key_size;
        size += key_size;
    }
    if (output_len < size || !output)
    {
        *ret_len = size;
        if (output) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }

    blob->dwMagic = (flags & KEY_EXPORT_FLAG_PRIVATE) ? BCRYPT_DH_PRIVATE_MAGIC : BCRYPT_DH_PUBLIC_MAGIC;
    blob->cbKey   = key_size;

    modulus = (UCHAR *)(blob + 1);
    generator = modulus + key_size;

    if (SymCryptDlgroupGetValue( key->a.dsa.group, modulus, key_size, NULL, 0, generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, NULL, NULL, 0, 0 )) return STATUS_INTERNAL_ERROR;
    pub = generator + key_size;
    if (flags & KEY_EXPORT_FLAG_PRIVATE) priv = pub + key_size;

    if (SymCryptDlkeyGetValue( key->a.dsa.handle, priv, priv_size, pub, key_size,
                               SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0 )) return STATUS_INTERNAL_ERROR;
    *ret_len = size;
    return STATUS_SUCCESS;
}

static NTSTATUS export_asymmetric_key( const struct key *key, const WCHAR *type, UCHAR *output, ULONG output_len,
                                       ULONG *ret_len )
{
    if (!(key->flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    if (!wcscmp( type, BCRYPT_ECCPRIVATE_BLOB ))
    {
        return export_ecc_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_ECCPUBLIC_BLOB ))
    {
        return export_ecc_key( key, output, output_len, 0, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_DSA_PRIVATE_BLOB ))
    {
        return export_dsa_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_DSA_PUBLIC_BLOB ))
    {
        return export_dsa_key( key, output, output_len, 0, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_DH_PRIVATE_BLOB ))
    {
        return export_dh_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_DH_PUBLIC_BLOB ))
    {
        return export_dh_key( key, output, output_len, 0, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_RSAPRIVATE_BLOB ))
    {
        return export_rsa_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_RSAFULLPRIVATE_BLOB ))
    {
        return export_rsa_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE | KEY_EXPORT_FLAG_RSA_FULL, ret_len );
    }
    else if (!wcscmp( type, BCRYPT_RSAPUBLIC_BLOB ))
    {
        return export_rsa_key( key, output, output_len, 0, ret_len );
    }
    else if (!wcscmp( type, LEGACY_DSA_V2_PRIVATE_BLOB ))
    {
        return export_legacy_dsa_key( key, output, output_len, KEY_EXPORT_FLAG_PRIVATE, ret_len );
    }
    else if (!wcscmp( type, LEGACY_DSA_V2_PUBLIC_BLOB ))
    {
        return export_legacy_dsa_key( key, output, output_len, 0, ret_len );
    }

    FIXME( "unsupported blob type %s\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptExportKey( BCRYPT_KEY_HANDLE export_key_handle, BCRYPT_KEY_HANDLE encrypt_key_handle,
                                 const WCHAR *type, UCHAR *output, ULONG output_len, ULONG *size, ULONG flags )
{
    const struct key *key = get_key_object( export_key_handle ), *encrypt_key = NULL;

    TRACE( "%p, %p, %s, %p, %lu, %p, %#lx\n", export_key_handle, encrypt_key_handle, debugstr_w(type), output,
           output_len, size, flags );

    if (!key) return STATUS_INVALID_HANDLE;
    if (!type || !size) return STATUS_INVALID_PARAMETER;
    if (encrypt_key_handle && !(encrypt_key = get_key_object( encrypt_key_handle ))) return STATUS_INVALID_HANDLE;

    if (is_symmetric_key( key )) return export_symmetric_key( key, encrypt_key, type, output, output_len, size );
    return export_asymmetric_key( key, type, output, output_len, size );
}

static NTSTATUS alloc_rsa_key( struct key *key, ULONG bitlen )
{
    SYMCRYPT_RSA_PARAMS params;

    params.version = 1;
    params.nBitsOfModulus = bitlen;
    params.nPrimes = (key->flags & KEY_FLAG_PRIVATE) ? 2 : 0;
    params.nPubExp = 1;
    if (!(key->a.rsa.handle = SymCryptRsakeyAllocate( &params, 0 ))) return STATUS_NO_MEMORY;

    key->a.bitlen = bitlen;
    return STATUS_SUCCESS;
}

static NTSTATUS import_rsa_key( enum alg_id alg, const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    const BCRYPT_RSAKEY_BLOB *blob = (const BCRYPT_RSAKEY_BLOB *)input;
#ifdef __REACTOS__
    const UCHAR *mod, *primes[2] = { 0 }, *exp = (const UCHAR *)(blob + 1);
    SIZE_T primes_sizes[2] = { 0 };
#else
    const UCHAR *mod, *primes[2] = {}, *exp = (const UCHAR *)(blob + 1);
    SIZE_T primes_sizes[2] = {};
#endif
    UINT64 exp64 = 0;
    ULONG key_flags = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS, size, i, num_primes = 0;
    NTSTATUS status;
    struct key *key = NULL;

    if (input_len < sizeof(*blob)) return STATUS_INVALID_PARAMETER;

    if (alg != ALG_ID_RSA && alg != ALG_ID_RSA_SIGN) return STATUS_NOT_SUPPORTED;
    if (blob->Magic == BCRYPT_RSAPRIVATE_MAGIC || blob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC)
        key_flags = KEY_FLAG_PRIVATE;
    else if (blob->Magic != BCRYPT_RSAPUBLIC_MAGIC) return STATUS_NOT_SUPPORTED;

    if (blob->cbPublicExp > sizeof(exp64)) return NTE_BAD_DATA;
    for (i = 0; i < blob->cbPublicExp; i++) exp64 += exp[i] << ((blob->cbPublicExp - i - 1) * 8);
    if (exp64 == 1) return STATUS_INVALID_PARAMETER;

    size = sizeof(*blob) + blob->cbPublicExp + blob->cbModulus;
    if (key_flags & KEY_FLAG_PRIVATE) size += blob->cbPrime1 + blob->cbPrime2;
    if (blob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC) size += blob->cbPrime1 * 2 + blob->cbPrime2 + blob->cbModulus;
    if (size != input_len) return NTE_BAD_DATA;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_rsa_key( key, blob->cbModulus * 8 )))
    {
        destroy_key( key );
        return status;
    }

    if (alg == ALG_ID_RSA) set_flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT | SYMCRYPT_FLAG_RSAKEY_SIGN;
    else if (alg == ALG_ID_RSA_SIGN) set_flags |= SYMCRYPT_FLAG_RSAKEY_SIGN;

    mod = exp + blob->cbPublicExp;

    if (key_flags & KEY_FLAG_PRIVATE)
    {
        num_primes = 2;

        primes[0] = mod + blob->cbModulus;
        primes[1] = primes[0] + blob->cbPrime1;

        primes_sizes[0] = blob->cbPrime1;
        primes_sizes[1] = blob->cbPrime2;
    }

    if (SymCryptRsakeySetValue( mod, blob->cbModulus, &exp64, 1, primes, primes_sizes, num_primes,
                                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, set_flags, key->a.rsa.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS import_legacy_rsa_key( enum alg_id alg, const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    const BLOBHEADER *hdr = (const BLOBHEADER *)input;
    const RSAPUBKEY *rsakey = (RSAPUBKEY *)(hdr + 1);
#ifdef __REACTOS__
    const UCHAR *mod, *primes[2] = { 0 };
    SIZE_T primes_sizes[2] = { 0 };
#else
    const UCHAR *mod, *primes[2] = {};
    SIZE_T primes_sizes[2] = {};
#endif
    UINT64 exp64;
    ULONG key_size, key_flags = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS, size, num_primes = 0;
    NTSTATUS status;
    struct key *key = NULL;

    if (alg != ALG_ID_RSA && alg != ALG_ID_RSA_SIGN) return STATUS_NOT_SUPPORTED;
    if (input_len < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;

    if (hdr->bType == PRIVATEKEYBLOB) key_flags |= KEY_FLAG_PRIVATE;
    else if (hdr->bType != PUBLICKEYBLOB) return STATUS_NOT_SUPPORTED;
    if (hdr->bVersion != 2 || (hdr->aiKeyAlg != CALG_RSA_KEYX && hdr->aiKeyAlg != CALG_RSA_SIGN))
        return STATUS_NOT_SUPPORTED;

    size = sizeof(*hdr) + sizeof(*rsakey);
    if (input_len < size || rsakey->bitlen & 0xf) return STATUS_INVALID_PARAMETER;

    key_size = len_from_bitlen( rsakey->bitlen );
    size += key_size;
    if (key_flags & KEY_FLAG_PRIVATE) size += key_size; /* prime1 + prime2 */
    if (input_len < size || rsakey->pubexp == 1) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_rsa_key( key, rsakey->bitlen )))
    {
        destroy_key( key );
        return status;
    }

    if (alg == ALG_ID_RSA) set_flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT | SYMCRYPT_FLAG_RSAKEY_SIGN;
    else if (alg == ALG_ID_RSA_SIGN) set_flags |= SYMCRYPT_FLAG_RSAKEY_SIGN;

    exp64 = rsakey->pubexp;
    mod = (const UCHAR *)(rsakey + 1);
    if (key_flags & KEY_FLAG_PRIVATE)
    {
        num_primes = 2;

        primes[0] = mod + key_size;
        primes[1] = primes[0] + key_size / 2;

        primes_sizes[0] = key_size / 2;
        primes_sizes[1] = key_size / 2;
    }

    if (SymCryptRsakeySetValue( mod, key_size, &exp64, 1, primes, primes_sizes, num_primes,
                                SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, set_flags, key->a.rsa.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    *ret_key = key;
    return STATUS_SUCCESS;
}

static enum ecc_curve_id get_ecc_blob_curve( enum alg_id alg, const BCRYPT_ECCKEY_BLOB *blob, ULONG *size,
                                             ULONG *flags )
{
    enum ecc_curve_id curve_id = 0;

    *flags = 0;
    *size = blob->cbKey;

    switch (alg)
    {
    case ALG_ID_ECDH:
        if (blob->dwMagic == BCRYPT_ECDH_PRIVATE_GENERIC_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDH_PUBLIC_GENERIC_MAGIC) return ECC_CURVE_NONE;
        break;

    case ALG_ID_ECDH_P256:
        if (blob->dwMagic == BCRYPT_ECDH_PRIVATE_P256_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDH_PUBLIC_P256_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P256R1;
        *size = 32;
        break;

    case ALG_ID_ECDH_P384:
        if (blob->dwMagic == BCRYPT_ECDH_PRIVATE_P384_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDH_PUBLIC_P384_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P384R1;
        *size = 48;
        break;

    case ALG_ID_ECDH_P521:
        if (blob->dwMagic == BCRYPT_ECDH_PRIVATE_P521_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDH_PUBLIC_P521_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P521R1;
        *size = 66;
        break;

    case ALG_ID_ECDSA:
        if (blob->dwMagic == BCRYPT_ECDSA_PRIVATE_GENERIC_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDSA_PUBLIC_GENERIC_MAGIC) return ECC_CURVE_NONE;
        break;

    case ALG_ID_ECDSA_P256:
        if (blob->dwMagic == BCRYPT_ECDSA_PRIVATE_P256_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDSA_PUBLIC_P256_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P256R1;
        *size = 32;
        break;

    case ALG_ID_ECDSA_P384:
        if (blob->dwMagic == BCRYPT_ECDSA_PRIVATE_P384_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDSA_PUBLIC_P384_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P384R1;
        *size = 48;
        break;

    case ALG_ID_ECDSA_P521:
        if (blob->dwMagic == BCRYPT_ECDSA_PRIVATE_P521_MAGIC) *flags = KEY_FLAG_PRIVATE;
        else if (blob->dwMagic != BCRYPT_ECDSA_PUBLIC_P521_MAGIC) return ECC_CURVE_INVALID;
        curve_id = ECC_CURVE_P521R1;
        *size = 66;
        break;

    default:
        ERR( "unhandled algorithm %u\n", alg );
        return ECC_CURVE_INVALID;
    }
    return curve_id;
}

static UINT32 get_ecc_import_flags( enum alg_id alg )
{
    UINT32 flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION;

    switch (alg)
    {
    case ALG_ID_ECDH:
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDH_P384:
    case ALG_ID_ECDH_P521:
        flags |= SYMCRYPT_FLAG_ECKEY_ECDH;
        break;

    case ALG_ID_ECDSA:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_ECDSA_P521:
        flags |= SYMCRYPT_FLAG_ECKEY_ECDSA;
        break;

    default:
        FIXME( "unhandled algorithm %u\n", alg );
        break;
    }
    return flags;
}

static const BYTE brainpoolP256r1[] =
{
    /* version */
    0x01, 0x00, 0x00, 0x00,
    /* type */
    0x01, 0x00, 0x00, 0x00,
    /* algId */
    0x00, 0x00, 0x00, 0x00,
    /* cbFieldLength */
    0x20, 0x00, 0x00, 0x00,
    /* cbSubgroupOrder */
    0x20, 0x00, 0x00, 0x00,
    /* cbCofactor */
    0x01, 0x00, 0x00, 0x00,
    /* cbSeed */
    0x00, 0x00, 0x00, 0x00,
    /* P */
    0xa9, 0xfb, 0x57, 0xdb, 0xa1, 0xee, 0xa9, 0xbc,
    0x3e, 0x66, 0x0a, 0x90, 0x9d, 0x83, 0x8d, 0x72,
    0x6e, 0x3b, 0xf6, 0x23, 0xd5, 0x26, 0x20, 0x28,
    0x20, 0x13, 0x48, 0x1d, 0x1f, 0x6e, 0x53, 0x77,
    /* A */
    0x7d, 0x5a, 0x09, 0x75, 0xfc, 0x2c, 0x30, 0x57,
    0xee, 0xf6, 0x75, 0x30, 0x41, 0x7a, 0xff, 0xe7,
    0xfb, 0x80, 0x55, 0xc1, 0x26, 0xdc, 0x5c, 0x6c,
    0xe9, 0x4a, 0x4b, 0x44, 0xf3, 0x30, 0xb5, 0xd9,
    /* B */
    0x26, 0xdc, 0x5c, 0x6c, 0xe9, 0x4a, 0x4b, 0x44,
    0xf3, 0x30, 0xb5, 0xd9, 0xbb, 0xd7, 0x7c, 0xbf,
    0x95, 0x84, 0x16, 0x29, 0x5c, 0xf7, 0xe1, 0xce,
    0x6b, 0xcc, 0xdc, 0x18, 0xff, 0x8c, 0x07, 0xb6,
    /* Gx */
    0x8b, 0xd2, 0xae, 0xb9, 0xcb, 0x7e, 0x57, 0xcb,
    0x2c, 0x4b, 0x48, 0x2f, 0xfc, 0x81, 0xb7, 0xaf,
    0xb9, 0xde, 0x27, 0xe1, 0xe3, 0xbd, 0x23, 0xc2,
    0x3a, 0x44, 0x53, 0xbd, 0x9a, 0xce, 0x32, 0x62,
    /* Gy */
    0x54, 0x7e, 0xf8, 0x35, 0xc3, 0xda, 0xc4, 0xfd,
    0x97, 0xf8, 0x46, 0x1a, 0x14, 0x61, 0x1d, 0xc9,
    0xc2, 0x77, 0x45, 0x13, 0x2d, 0xed, 0x8e, 0x54,
    0x5c, 0x1d, 0x54, 0xc7, 0x2f, 0x04, 0x69, 0x97,
    /* n */
    0xa9, 0xfb, 0x57, 0xdb, 0xa1, 0xee, 0xa9, 0xbc,
    0x3e, 0x66, 0x0a, 0x90, 0x9d, 0x83, 0x8d, 0x71,
    0x8c, 0x39, 0x7a, 0xa3, 0xb5, 0x61, 0xa6, 0xf7,
    0x90, 0x1e, 0x0e, 0x82, 0x97, 0x48, 0x56, 0xa7,
    /* h */
    0x01
};
const SYMCRYPT_ECURVE_PARAMS *SymCryptEcurveParamsBrainpoolP256r1 = (const SYMCRYPT_ECURVE_PARAMS *)brainpoolP256r1;

static const BYTE brainpoolP384r1[] =
{
    /* version */
    0x01, 0x00, 0x00, 0x00,
    /* type */
    0x01, 0x00, 0x00, 0x00,
    /* algId */
    0x00, 0x00, 0x00, 0x00,
    /* cbFieldLength */
    0x30, 0x00, 0x00, 0x00,
    /* cbSubgroupOrder */
    0x30, 0x00, 0x00, 0x00,
    /* cbCofactor */
    0x01, 0x00, 0x00, 0x00,
    /* cbSeed */
    0x00, 0x00, 0x00, 0x00,
    /* P */
    0x8c, 0xb9, 0x1e, 0x82, 0xa3, 0x38, 0x6d, 0x28,
    0x0f, 0x5d, 0x6f, 0x7e, 0x50, 0xe6, 0x41, 0xdf,
    0x15, 0x2f, 0x71, 0x09, 0xed, 0x54, 0x56, 0xb4,
    0x12, 0xb1, 0xda, 0x19, 0x7f, 0xb7, 0x11, 0x23,
    0xac, 0xd3, 0xa7, 0x29, 0x90, 0x1d, 0x1a, 0x71,
    0x87, 0x47, 0x00, 0x13, 0x31, 0x07, 0xec, 0x53,
    /* A */
    0x7b, 0xc3, 0x82, 0xc6, 0x3d, 0x8c, 0x15, 0x0c,
    0x3c, 0x72, 0x08, 0x0a, 0xce, 0x05, 0xaf, 0xa0,
    0xc2, 0xbe, 0xa2, 0x8e, 0x4f, 0xb2, 0x27, 0x87,
    0x13, 0x91, 0x65, 0xef, 0xba, 0x91, 0xf9, 0x0f,
    0x8a, 0xa5, 0x81, 0x4a, 0x50, 0x3a, 0xd4, 0xeb,
    0x04, 0xa8, 0xc7, 0xdd, 0x22, 0xce, 0x28, 0x26,
    /* B */
    0x04, 0xa8, 0xc7, 0xdd, 0x22, 0xce, 0x28, 0x26,
    0x8b, 0x39, 0xb5, 0x54, 0x16, 0xf0, 0x44, 0x7c,
    0x2f, 0xb7, 0x7d, 0xe1, 0x07, 0xdc, 0xd2, 0xa6,
    0x2e, 0x88, 0x0e, 0xa5, 0x3e, 0xeb, 0x62, 0xd5,
    0x7c, 0xb4, 0x39, 0x02, 0x95, 0xdb, 0xc9, 0x94,
    0x3a, 0xb7, 0x86, 0x96, 0xfa, 0x50, 0x4c, 0x11,
    /* Gx */
    0x1d, 0x1c, 0x64, 0xf0, 0x68, 0xcf, 0x45, 0xff,
    0xa2, 0xa6, 0x3a, 0x81, 0xb7, 0xc1, 0x3f, 0x6b,
    0x88, 0x47, 0xa3, 0xe7, 0x7e, 0xf1, 0x4f, 0xe3,
    0xdb, 0x7f, 0xca, 0xfe, 0x0c, 0xbd, 0x10, 0xe8,
    0xe8, 0x26, 0xe0, 0x34, 0x36, 0xd6, 0x46, 0xaa,
    0xef, 0x87, 0xb2, 0xe2, 0x47, 0xd4, 0xaf, 0x1e,
    /* Gy */
    0x8a, 0xbe, 0x1d, 0x75, 0x20, 0xf9, 0xc2, 0xa4,
    0x5c, 0xb1, 0xeb, 0x8e, 0x95, 0xcf, 0xd5, 0x52,
    0x62, 0xb7, 0x0b, 0x29, 0xfe, 0xec, 0x58, 0x64,
    0xe1, 0x9c, 0x05, 0x4f, 0xf9, 0x91, 0x29, 0x28,
    0x0e, 0x46, 0x46, 0x21, 0x77, 0x91, 0x81, 0x11,
    0x42, 0x82, 0x03, 0x41, 0x26, 0x3c, 0x53, 0x15,
    /* n */
    0x8c, 0xb9, 0x1e, 0x82, 0xa3, 0x38, 0x6d, 0x28,
    0x0f, 0x5d, 0x6f, 0x7e, 0x50, 0xe6, 0x41, 0xdf,
    0x15, 0x2f, 0x71, 0x09, 0xed, 0x54, 0x56, 0xb3,
    0x1f, 0x16, 0x6e, 0x6c, 0xac, 0x04, 0x25, 0xa7,
    0xcf, 0x3a, 0xb6, 0xaf, 0x6b, 0x7f, 0xc3, 0x10,
    0x3b, 0x88, 0x32, 0x02, 0xe9, 0x04, 0x65, 0x65,
    /* h */
    0x01
};
const SYMCRYPT_ECURVE_PARAMS *SymCryptEcurveParamsBrainpoolP384r1 = (const SYMCRYPT_ECURVE_PARAMS *)brainpoolP384r1;

static SYMCRYPT_ECURVE *alloc_ecc_curve( enum ecc_curve_id curve_id )
{
    const SYMCRYPT_ECURVE_PARAMS *params;

    switch (curve_id)
    {
    case ECC_CURVE_25519:
        params = SymCryptEcurveParamsCurve25519;
        break;
    case ECC_CURVE_BRAINPOOLP256R1:
        params = SymCryptEcurveParamsBrainpoolP256r1;
        break;
    case ECC_CURVE_BRAINPOOLP384R1:
        params = SymCryptEcurveParamsBrainpoolP384r1;
        break;
    case ECC_CURVE_P256R1:
        params = SymCryptEcurveParamsNistP256;
        break;
    case ECC_CURVE_P384R1:
        params = SymCryptEcurveParamsNistP384;
        break;
    case ECC_CURVE_P521R1:
        params = SymCryptEcurveParamsNistP521;
        break;
    default:
        FIXME( "unhandled curve %u\n", curve_id );
        return NULL;
    }
    return SymCryptEcurveAllocate( params, 0 );
}

static ULONG curve_strength( enum ecc_curve_id curve_id )
{
    switch (curve_id)
    {
    case ECC_CURVE_25519:           return 253;
    case ECC_CURVE_BRAINPOOLP256R1:
    case ECC_CURVE_P256R1:          return 256;
    case ECC_CURVE_BRAINPOOLP384R1:
    case ECC_CURVE_P384R1:          return 384;
    case ECC_CURVE_P521R1:          return 521;
    default:
        FIXME( "unsupported curve %u\n", curve_id );
        return 0;
    }
}

static NTSTATUS alloc_ecc_key( struct key *key, enum ecc_curve_id curve_id )
{
    if (!(key->a.ecc.curve = alloc_ecc_curve( curve_id ))) return STATUS_NO_MEMORY;
    if (!(key->a.ecc.handle = SymCryptEckeyAllocate( key->a.ecc.curve ))) return STATUS_NO_MEMORY;
    key->a.ecc.curve_id = curve_id;
    key->a.bitlen = curve_strength( curve_id );
    return STATUS_SUCCESS;
}

static NTSTATUS import_ecc_key( enum alg_id alg, enum ecc_curve_id curve, const UCHAR *input, ULONG input_len,
                                struct key **ret_key )
{
    const BCRYPT_ECCKEY_BLOB *blob = (const BCRYPT_ECCKEY_BLOB *)input;
    UINT32 pub_size, priv_size = 0, set_flags = get_ecc_import_flags( alg );
    ULONG size, key_size, key_flags = 0;
    const UCHAR *pub = (UCHAR *)(blob + 1), *priv = NULL;
    enum ecc_curve_id blob_curve;
    NTSTATUS status;
    struct key *key = NULL;

    if (input_len < sizeof(*blob)) return STATUS_INVALID_PARAMETER;
    if (!(blob_curve = get_ecc_blob_curve( alg, blob, &key_size, &key_flags ))) blob_curve = curve;
    if (blob_curve == ECC_CURVE_INVALID) return STATUS_INVALID_PARAMETER;

    size = sizeof(*blob) + blob->cbKey * 2;
    if (key_flags & KEY_FLAG_PRIVATE)
    {
        priv = pub + blob->cbKey * 2;
        size += blob->cbKey;
    }
    if (blob->cbKey != key_size || input_len != size) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_ecc_key( key, curve )))
    {
        destroy_key( key );
        return status;
    }

    pub_size = SymCryptEckeySizeofPublicKey( key->a.ecc.handle, SYMCRYPT_ECPOINT_FORMAT_XY );
    if (key_flags & KEY_FLAG_PRIVATE) priv_size = SymCryptEckeySizeofPrivateKey( key->a.ecc.handle );

    if (SymCryptEckeySetValue( priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                               SYMCRYPT_ECPOINT_FORMAT_XY, set_flags, key->a.ecc.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS alloc_dsa_key( struct key *key, ULONG bitlen )
{
    if (!(key->a.dsa.group = SymCryptDlgroupAllocate( bitlen, 0 ))) return STATUS_NO_MEMORY;
    if (!(key->a.dsa.handle = SymCryptDlkeyAllocate( key->a.dsa.group ))) return STATUS_NO_MEMORY;
    key->a.bitlen = bitlen;
    return STATUS_SUCCESS;
}

static NTSTATUS import_dsa_key( enum alg_id alg, const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    const BCRYPT_DSA_KEY_BLOB *blob = (const BCRYPT_DSA_KEY_BLOB *)input;
    ULONG size, count, priv_size = 0, key_flags = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_DLKEY_DSA;
    const UCHAR *prime, *generator, *pub, *priv = NULL;
    struct key *key = NULL;
    NTSTATUS status;

    if (input_len < sizeof(*blob)) return STATUS_INVALID_PARAMETER;

    if (alg != ALG_ID_DSA) return STATUS_NOT_SUPPORTED;
    if (blob->dwMagic == BCRYPT_DSA_PRIVATE_MAGIC) key_flags = KEY_FLAG_PRIVATE;
    else if (blob->dwMagic != BCRYPT_DSA_PUBLIC_MAGIC) return STATUS_NOT_SUPPORTED;

    size = sizeof(*blob) + blob->cbKey * 3;
    if (key_flags & KEY_FLAG_PRIVATE) size += 20;
    if (input_len != size) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_dsa_key( key, blob->cbKey * 8 )))
    {
        destroy_key( key );
        return status;
    }

    prime = (const UCHAR *)(blob + 1);
    generator = prime + blob->cbKey;
    count = *(UINT32 *)blob->Count;

    if (SymCryptDlgroupSetValue( prime, blob->cbKey, blob->q, sizeof(blob->q), generator, blob->cbKey,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, NULL, blob->Seed, sizeof(blob->Seed),
                                 count, SYMCRYPT_DLGROUP_FIPS_NONE, key->a.dsa.group ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    pub = generator + blob->cbKey;
    if (key_flags & KEY_FLAG_PRIVATE)
    {
        priv = pub + blob->cbKey;
        priv_size = 20;
    }

    if (alg == ALG_ID_DH) set_flags |= SYMCRYPT_FLAG_DLKEY_DH;
    else if (alg == ALG_ID_DSA) set_flags |= SYMCRYPT_FLAG_DLKEY_DSA;

    if (SymCryptDlkeySetValue( priv, priv_size, pub, blob->cbKey, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, set_flags,
                               key->a.dsa.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    memcpy( key->a.dsa.seed, blob->Seed, sizeof(blob->Seed) );
    memcpy( &key->a.dsa.count, blob->Count, sizeof(blob->Count) );

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS import_legacy_dsa_key( enum alg_id alg, const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    const BLOBHEADER *hdr = (const BLOBHEADER *)input;
    const DSSPUBKEY *dsskey = (DSSPUBKEY *)(hdr + 1);
    UINT32 priv_size = 0, pub_size = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_DLKEY_DSA;
    const UCHAR *prime_p, *prime_q, *generator, *pub = NULL, *priv = NULL, *seed, *count;
    ULONG size, key_size, key_flags = 0;
    struct key *key = NULL;
    NTSTATUS status;

    if (alg != ALG_ID_DSA) return STATUS_NOT_SUPPORTED;
    if (input_len < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;

    if (hdr->bType == PRIVATEKEYBLOB) key_flags |= KEY_FLAG_PRIVATE;
    else if (hdr->bType != PUBLICKEYBLOB) return STATUS_NOT_SUPPORTED;
    if (hdr->bVersion != 2 || hdr->aiKeyAlg != CALG_DSS_SIGN) return STATUS_NOT_SUPPORTED;

    size = sizeof(*hdr) + sizeof(*dsskey);
    if (input_len < size || dsskey->bitlen & 0xf) return STATUS_INVALID_PARAMETER;

    key_size = len_from_bitlen( dsskey->bitlen );
    size += key_size * 2 + 20 /* q */ + 20 /* x or y */ + 20 /* seed */ + 4 /* count */;
    if (input_len != size) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_dsa_key( key, dsskey->bitlen )))
    {
        destroy_key( key );
        return status;
    }

    prime_p = (const UCHAR *)(dsskey + 1);
    prime_q = prime_p + key_size;
    generator = prime_q + 20;
    seed = generator + key_size + 20;
    count = seed + 20;

    if (SymCryptDlgroupSetValue( prime_p, key_size, prime_q, 20, generator, key_size,
                                 SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, NULL, seed, 20, *(UINT32 *)count,
                                 SYMCRYPT_DLGROUP_FIPS_NONE, key->a.dsa.group ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    if (key_flags & KEY_FLAG_PRIVATE)
    {
        priv = generator + key_size;
        priv_size = 20;
    }
    else
    {
        pub = generator + key_size;
        pub_size = 20;
    }

    if (SymCryptDlkeySetValue( priv, priv_size, pub, pub_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, set_flags,
                               key->a.dsa.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    memcpy( key->a.dsa.seed, seed, 20 );

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS import_dh_key( enum alg_id alg, const UCHAR *input, ULONG input_len, struct key **ret_key )
{
    const BCRYPT_DH_KEY_BLOB *blob = (const BCRYPT_DH_KEY_BLOB *)input;
    ULONG size, priv_size = 0, key_flags = 0, set_flags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_DLKEY_DH;
    const UCHAR *modulus, *generator, *pub, *priv = NULL;
    struct key *key = NULL;
    NTSTATUS status;

    if (input_len < sizeof(*blob)) return STATUS_INVALID_PARAMETER;

    if (alg != ALG_ID_DH) return STATUS_NOT_SUPPORTED;
    if (blob->dwMagic == BCRYPT_DH_PRIVATE_MAGIC) key_flags = KEY_FLAG_PRIVATE;
    else if (blob->dwMagic != BCRYPT_DH_PUBLIC_MAGIC) return STATUS_NOT_SUPPORTED;

    size = sizeof(*blob) + blob->cbKey * 3;
    if (key_flags & KEY_FLAG_PRIVATE) size += blob->cbKey;
    if (input_len != size) return STATUS_INVALID_PARAMETER;

    if ((status = alloc_key( alg, key_flags, &key )) || (status = alloc_dsa_key( key, blob->cbKey * 8 )))
    {
        destroy_key( key );
        return status;
    }

    modulus = (const UCHAR *)(blob + 1);
    generator = modulus + blob->cbKey;

    if (SymCryptDlgroupSetValue( modulus, blob->cbKey, NULL, 0, generator, blob->cbKey,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, NULL, NULL, 0, 0,
                                 SYMCRYPT_DLGROUP_FIPS_NONE, key->a.dsa.group ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    pub = generator + blob->cbKey;
    if (key_flags & KEY_FLAG_PRIVATE)
    {
        priv = pub + blob->cbKey;
        priv_size = blob->cbKey;
    }

    if (SymCryptDlkeySetValue( priv, priv_size, pub, blob->cbKey, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, set_flags,
                               key->a.dsa.handle ))
    {
        destroy_key( key );
        return STATUS_INTERNAL_ERROR;
    }

    *ret_key = key;
    return STATUS_SUCCESS;
}

static NTSTATUS import_key_pair( const struct algorithm *alg, const WCHAR *type, const UCHAR *input, ULONG input_len,
                                 struct key **ret_key )
{
    struct key *key;
    NTSTATUS status;

    if (!wcscmp( type, BCRYPT_ECCPUBLIC_BLOB ) || !wcscmp( type, BCRYPT_ECCPRIVATE_BLOB ))
    {
        status = import_ecc_key( alg->id, alg->curve_id, input, input_len, &key );
    }
    else if (!wcscmp( type, BCRYPT_RSAPUBLIC_BLOB ) || !wcscmp( type, BCRYPT_RSAPRIVATE_BLOB ) ||
             !wcscmp( type, BCRYPT_RSAFULLPRIVATE_BLOB ))
    {
        status = import_rsa_key( alg->id, input, input_len, &key );
    }
    else if (!wcscmp( type, LEGACY_RSAPRIVATE_BLOB ))
    {
        status = import_legacy_rsa_key( alg->id, input, input_len, &key );
    }
    else if (!wcscmp( type, BCRYPT_DSA_PUBLIC_BLOB ) || !wcscmp( type, BCRYPT_DSA_PRIVATE_BLOB ))
    {
        status = import_dsa_key( alg->id, input, input_len, &key );
    }
    else if (!wcscmp( type, BCRYPT_DH_PUBLIC_BLOB ) || !wcscmp( type, BCRYPT_DH_PRIVATE_BLOB ))
    {
        status = import_dh_key( alg->id, input, input_len, &key );
    }
    else if (!wcscmp( type, LEGACY_DSA_V2_PRIVATE_BLOB ) || !wcscmp( type, LEGACY_DSA_V2_PUBLIC_BLOB ))
    {
        status = import_legacy_dsa_key( alg->id, input, input_len, &key );
    }
    else
    {
        FIXME( "unsupported blob type %s\n", debugstr_w(type) );
        return STATUS_NOT_SUPPORTED;
    }

    if (!status)
    {
        key->flags |= KEY_FLAG_FINALIZED;
        *ret_key = key;
    }
    return status;
}

static const WCHAR *resolve_blob_type( const WCHAR *type, const UCHAR *input, ULONG input_len )
{
    const BCRYPT_KEY_BLOB *blob = (const BCRYPT_KEY_BLOB *)input;

    if (!type) return NULL;
    if (wcscmp( type, BCRYPT_PUBLIC_KEY_BLOB )) return type;
    if (input_len < sizeof(*blob)) return NULL;

    switch (blob->Magic)
    {
    case BCRYPT_ECDH_PUBLIC_P256_MAGIC:
    case BCRYPT_ECDH_PUBLIC_P384_MAGIC:
    case BCRYPT_ECDH_PUBLIC_P521_MAGIC:
    case BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
    case BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
    case BCRYPT_ECDSA_PUBLIC_P521_MAGIC:
        return BCRYPT_ECCPUBLIC_BLOB;

    case BCRYPT_RSAPUBLIC_MAGIC:
        return BCRYPT_RSAPUBLIC_BLOB;

    case BCRYPT_DSA_PUBLIC_MAGIC:
        return BCRYPT_DSA_PUBLIC_BLOB;

    default:
        FIXME( "unsupported key magic %#lx\n", blob->Magic );
        return NULL;
    }
}

NTSTATUS WINAPI BCryptImportKeyPair( BCRYPT_ALG_HANDLE handle, BCRYPT_KEY_HANDLE decrypt_key_handle, const WCHAR *type,
                                     BCRYPT_KEY_HANDLE *ret_handle, UCHAR *input, ULONG input_len, ULONG flags )
{
    const struct algorithm *alg = get_alg_object( handle );
    struct key *key;
    NTSTATUS status;

    TRACE( "%p, %p, %s, %p, %p, %lu, %#lx\n", handle, decrypt_key_handle, debugstr_w(type), ret_handle, input,
           input_len, flags );

    if (!alg) return STATUS_INVALID_HANDLE;
    if (!ret_handle || !input || !(type = resolve_blob_type( type, input, input_len ))) return STATUS_INVALID_PARAMETER;
    if (decrypt_key_handle)
    {
        FIXME( "decryption of key not yet supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = import_key_pair( alg, type, input, input_len, &key ))) return status;
    *ret_handle = key;

    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

static ULONG curve_size( enum ecc_curve_id curve_id )
{
    switch (curve_id)
    {
    case ECC_CURVE_25519:           return 255;
    case ECC_CURVE_BRAINPOOLP256R1:
    case ECC_CURVE_P256R1:          return 256;
    case ECC_CURVE_BRAINPOOLP384R1:
    case ECC_CURVE_P384R1:          return 384;
    case ECC_CURVE_P521R1:          return 521;
    default:
        FIXME( "unsupported curve %u\n", curve_id );
        return 0;
    }
}

static NTSTATUS generate_key_pair( const struct algorithm *alg, ULONG bitlen, ULONG flags, struct key **ret_key )
{
    NTSTATUS status;
    struct key *key;

    if ((status = alloc_key( alg->id, KEY_FLAG_PRIVATE, &key ))) return status;

    switch (alg->id)
    {
    case ALG_ID_DH:
        if (bitlen < 512)
        {
            destroy_key( key );
            return STATUS_INVALID_PARAMETER;
        }
        /* fall through */
    case ALG_ID_DSA:
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        key->a.bitlen = bitlen;
        break;

    case ALG_ID_ECDH:
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDH_P384:
    case ALG_ID_ECDH_P521:
    case ALG_ID_ECDSA:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_ECDSA_P521:
        if (bitlen && bitlen != curve_size( alg->curve_id ))
        {
            destroy_key( key );
            return STATUS_INVALID_PARAMETER;
        }
        key->a.ecc.curve_id = alg->curve_id;
        key->a.bitlen = curve_strength( key->a.ecc.curve_id );
        break;

    default:
        FIXME( "unhandled algorithm %u\n", alg->id );
        destroy_key( key );
        return STATUS_NOT_SUPPORTED;
    }

    *ret_key = key;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptGenerateKeyPair( BCRYPT_ALG_HANDLE handle, BCRYPT_KEY_HANDLE *ret_handle, ULONG bitlen,
                                       ULONG flags )
{
    const struct algorithm *alg = get_alg_object( handle );
    struct key *key;
    NTSTATUS status;

    TRACE( "%p, %p, %lu, %#lx\n", handle, ret_handle, bitlen, flags );

    if (!alg) return STATUS_INVALID_HANDLE;
    if (!ret_handle) return STATUS_INVALID_PARAMETER;

    if ((status = generate_key_pair( alg, bitlen, flags, &key ))) return status;

    *ret_handle = key;
    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

static NTSTATUS finalize_key_pair( struct key *key )
{
    UINT32 flags = SYMCRYPT_FLAG_KEY_NO_FIPS;
    NTSTATUS status;

    switch (key->alg_id)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
    {
        UINT64 exp = 65537;

        if (key->alg_id == ALG_ID_RSA) flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT | SYMCRYPT_FLAG_RSAKEY_SIGN;
        else flags |= SYMCRYPT_FLAG_RSAKEY_SIGN;

        if ((status = alloc_rsa_key( key, key->a.bitlen ))) return status;
        if (SymCryptRsakeyGenerate( key->a.rsa.handle, &exp, 1, flags )) return STATUS_INTERNAL_ERROR;
        break;
    }
    case ALG_ID_ECDH:
        if (!key->a.ecc.curve_id) return STATUS_INVALID_PARAMETER;
        /* fall through */
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDH_P384:
    case ALG_ID_ECDH_P521:
    {
        flags |= SYMCRYPT_FLAG_ECKEY_ECDH;
        if ((status = alloc_ecc_key( key, key->a.ecc.curve_id ))) return status;
        if (SymCryptEckeySetRandom( flags, key->a.ecc.handle )) return STATUS_INTERNAL_ERROR;
        break;
    }
    case ALG_ID_ECDSA:
        if (!key->a.ecc.curve_id) return STATUS_INVALID_PARAMETER;
        /* fall through */
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_ECDSA_P521:
    {
        flags |= SYMCRYPT_FLAG_ECKEY_ECDSA;
        if ((status = alloc_ecc_key( key, key->a.ecc.curve_id ))) return status;
        if (SymCryptEckeySetRandom( flags, key->a.ecc.handle )) return STATUS_INTERNAL_ERROR;
        break;
    }
    case ALG_ID_DH:
    case ALG_ID_DSA:
    {
        if (key->alg_id == ALG_ID_DSA) flags |= SYMCRYPT_FLAG_DLKEY_DSA;
        else flags |= SYMCRYPT_FLAG_DLKEY_DH;

        if (!(key->a.dsa.group)) /* no parameters have been imported */
        {
            if (!(key->a.dsa.group = SymCryptDlgroupAllocate( key->a.bitlen, 0 ))) return STATUS_NO_MEMORY;
            if (SymCryptDlgroupGenerate( NULL, SYMCRYPT_DLGROUP_FIPS_186_2, key->a.dsa.group ))
                return STATUS_INTERNAL_ERROR;
        }
        if (!(key->a.dsa.handle = SymCryptDlkeyAllocate( key->a.dsa.group ))) return STATUS_NO_MEMORY;
        if (SymCryptDlkeyGenerate( flags, key->a.dsa.handle )) return STATUS_INTERNAL_ERROR;
        break;
    }
    default:
        FIXME( "unhandled algorithm %u\n", key->alg_id );
        return STATUS_NOT_SUPPORTED;
    }

    key->flags |= KEY_FLAG_FINALIZED;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptFinalizeKeyPair( BCRYPT_KEY_HANDLE handle, ULONG flags )
{
    struct key *key = get_key_object( handle );

    TRACE( "%p, %#lx\n", key, flags );

    if (!key || key->flags & KEY_FLAG_FINALIZED) return STATUS_INVALID_HANDLE;

    return finalize_key_pair( key );
}

static NTSTATUS duplicate_key( const struct key *src, struct key **ret_key )
{
    struct key *dst;
    NTSTATUS status;

    switch (src->alg_id)
    {
    case ALG_ID_AES:
    case ALG_ID_AES_GMAC:
        if ((status = alloc_key( src->alg_id, src->flags, &dst ))) return status;
        if ((status = alloc_aes_key( dst, src->s.mode, src->s.block_size, src->s.secret, src->s.secret_len )))
        {
            destroy_key( dst );
            return status;
        }
        if (src->s.mode == CHAIN_MODE_GCM)
        {
            SymCryptGcmKeyCopy( &src->s.gcm.handle, &dst->s.gcm.handle );
            SymCryptGcmStateCopy( &src->s.gcm.state, &src->s.gcm.handle, &dst->s.gcm.state );
        }
        else SymCryptAesKeyCopy( &src->s.aes.handle, &dst->s.aes.handle );
        break;

    case ALG_ID_PBKDF2:
    case ALG_ID_TLS1_1_KDF:
    case ALG_ID_TLS1_2_KDF:
        if ((status = alloc_key( src->alg_id, src->flags, &dst ))) return status;
        dst->s.mode       = src->s.mode;
        dst->s.block_size = src->s.block_size;
        if (src->s.secret_len && !(dst->s.secret = malloc( src->s.secret_len )))
        {
            destroy_key( dst );
            return STATUS_NO_MEMORY;
        }
        memcpy( dst->s.secret, src->s.secret, src->s.secret_len );
        dst->s.secret_len = src->s.secret_len;
        InitializeCriticalSection( &dst->s.cs );
        break;

    default:
        FIXME( "algorithm %u not supported\n", src->alg_id );
        return STATUS_NOT_SUPPORTED;
    }

    *ret_key = dst;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateKey( BCRYPT_KEY_HANDLE handle, BCRYPT_KEY_HANDLE *handle_copy, UCHAR *object,
                                    ULONG object_len, ULONG flags )
{
    const struct key *key_orig = get_key_object( handle );
    struct key *key_copy;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %lu, %#lx\n", handle, handle_copy, object, object_len, flags );
    if (object) FIXME( "ignoring object buffer\n" );

    if (!key_orig) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;

    if ((status = duplicate_key( key_orig, &key_copy ))) return status;
    *handle_copy = key_copy;

    TRACE( "returning handle %p\n", *handle_copy );
    return STATUS_SUCCESS;
}

static NTSTATUS get_hash_oid( const WCHAR *str, const SYMCRYPT_OID **oid, UINT32 *oid_count )
{
    if (!str)
    {
        *oid = NULL;
        *oid_count = 0;
        return STATUS_SUCCESS;
    }
    if (!wcscmp( str, BCRYPT_MD5_ALGORITHM ))
    {
        *oid = SymCryptMd5OidList;
        *oid_count = ARRAY_SIZE( SymCryptMd5OidList );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( str, BCRYPT_SHA1_ALGORITHM ))
    {
        *oid = SymCryptSha1OidList;
        *oid_count = ARRAY_SIZE( SymCryptSha1OidList );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( str, BCRYPT_SHA256_ALGORITHM ))
    {
        *oid = SymCryptSha256OidList;
        *oid_count = ARRAY_SIZE( SymCryptSha256OidList );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( str, BCRYPT_SHA384_ALGORITHM ))
    {
        *oid = SymCryptSha384OidList;
        *oid_count = ARRAY_SIZE( SymCryptSha384OidList );
        return STATUS_SUCCESS;
    }
    if (!wcscmp( str, BCRYPT_SHA512_ALGORITHM ))
    {
        *oid = SymCryptSha512OidList;
        *oid_count = ARRAY_SIZE( SymCryptSha512OidList );
        return STATUS_SUCCESS;
    }
    FIXME( "unhandled algorithm %s\n", debugstr_w(str) );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS sign_hash_rsa( const struct key *key, const void *padding, const UCHAR *hash, ULONG hash_len,
                               UCHAR *signature, ULONG signature_len, ULONG *ret_len, ULONG flags )
{
    ULONG required_len = len_from_bitlen( key->a.bitlen );
    NTSTATUS status;

    if (!padding) return STATUS_INVALID_PARAMETER;
    if (signature_len < required_len)
    {
        *ret_len = required_len;
        if (signature) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }
    if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;

    if (flags == BCRYPT_PAD_PKCS1)
    {
        const BCRYPT_PKCS1_PADDING_INFO *pad = padding;
        UINT32 hash_oid_count, flags = 0;
        const SYMCRYPT_OID *hash_oid;
        SIZE_T size;

        if ((status = get_hash_oid( pad->pszAlgId, &hash_oid, &hash_oid_count ))) return status;
        if (!hash_oid) flags = SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1;

        if (SymCryptRsaPkcs1Sign( key->a.rsa.handle, hash, hash_len, hash_oid, hash_oid_count, flags,
                                  SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, signature, required_len, &size ))
            return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else if (flags == BCRYPT_PAD_PSS)
    {
        const BCRYPT_PSS_PADDING_INFO *pad = padding;
        const SYMCRYPT_HASH *hash_desc = get_hash_from_str( pad->pszAlgId );
        SIZE_T size;

        if (SymCryptRsaPssSign( key->a.rsa.handle, hash, hash_len, hash_desc, pad->cbSalt, 0,
                                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, signature, required_len, &size ))
            return STATUS_INTERNAL_ERROR;
        *ret_len = size;
    }
    else
    {
        FIXME( "unsupported flags %#lx", flags );
        if (padding) return STATUS_INVALID_PARAMETER;
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS sign_hash_ecc( const struct key *key, const UCHAR *hash, ULONG hash_len, UCHAR *signature,
                               ULONG signature_len, ULONG *ret_len )
{
    ULONG required_len = len_from_bitlen( key->a.bitlen ) * 2;

    if (signature_len < required_len)
    {
        *ret_len = required_len;
        if (signature) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }
    if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;

    if (SymCryptEcDsaSign( key->a.ecc.handle, hash, hash_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, signature,
                           required_len )) return STATUS_INTERNAL_ERROR;

    *ret_len = required_len;
    return STATUS_SUCCESS;
}

static NTSTATUS sign_hash_dsa( const struct key *key, const UCHAR *hash, ULONG hash_len, UCHAR *signature,
                               ULONG signature_len, ULONG *ret_len )
{
    ULONG required_len = hash_len * 2;

    if (signature_len < required_len)
    {
        *ret_len = required_len;
        if (signature) return STATUS_BUFFER_TOO_SMALL;
        return STATUS_SUCCESS;
    }
    if (!(key->flags & KEY_FLAG_PRIVATE)) return STATUS_INVALID_PARAMETER;

    if (SymCryptDsaSign( key->a.dsa.handle, hash, hash_len, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, signature,
                         required_len )) return STATUS_INTERNAL_ERROR;

    *ret_len = required_len;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptSignHash( BCRYPT_KEY_HANDLE handle, void *padding, UCHAR *hash, ULONG hash_len,
                                UCHAR *signature, ULONG signature_len, ULONG *ret_len, ULONG flags )
{
    const struct key *key = get_key_object( handle );

    TRACE( "%p, %p, %p, %lu, %p, %lu, %p, %#lx\n", handle, padding, hash, hash_len, signature, signature_len,
           ret_len, flags );

    if (!key) return STATUS_INVALID_HANDLE;

    switch (key->alg_id)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        return sign_hash_rsa( key, padding, hash, hash_len, signature, signature_len, ret_len, flags );

    case ALG_ID_ECDSA:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_ECDSA_P521:
        if (padding) return STATUS_INVALID_PARAMETER;
        return sign_hash_ecc( key, hash, hash_len, signature, signature_len, ret_len );

    case ALG_ID_DSA:
        return sign_hash_dsa( key, hash, hash_len, signature, signature_len, ret_len );

    default:
        FIXME( "unhandled algorithm %u\n", key->alg_id );
        break;
    }

    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS verify_signature_rsa( const struct key *key, const void *padding, const UCHAR *hash, ULONG hash_len,
                                      const UCHAR *signature, ULONG signature_len, ULONG flags )
{
    NTSTATUS status;
    SYMCRYPT_ERROR error;

    if (!padding || signature_len != len_from_bitlen( key->a.bitlen )) return STATUS_INVALID_PARAMETER;

    if (flags == BCRYPT_PAD_PKCS1)
    {
        const BCRYPT_PKCS1_PADDING_INFO *pad = padding;
        UINT32 flags = 0, hash_oid_count;
        const SYMCRYPT_OID *hash_oid;

        if (!pad) return STATUS_INVALID_PARAMETER;
        if ((status = get_hash_oid( pad->pszAlgId, &hash_oid, &hash_oid_count ))) return status;
        if (!hash_oid) flags = SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID;

        error = SymCryptRsaPkcs1Verify( key->a.rsa.handle, hash, hash_len, signature, signature_len,
                                        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, hash_oid, hash_oid_count, flags );
    }
    else if (flags == BCRYPT_PAD_PSS)
    {
        const BCRYPT_PSS_PADDING_INFO *pad = padding;
        const SYMCRYPT_HASH *hash_desc = get_hash_from_str( pad->pszAlgId );

        error = SymCryptRsaPssVerify( key->a.rsa.handle, hash, hash_len, signature, signature_len,
                                      SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, hash_desc, pad->cbSalt, 0 );
    }
    else
    {
        FIXME( "unsupported flags %#lx\n", flags );
        if (padding) return STATUS_INVALID_PARAMETER;
        return STATUS_NOT_SUPPORTED;
    }

    if (error == SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE) return STATUS_INVALID_SIGNATURE;
    if (error) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS verify_signature_ecc( const struct key *key, const UCHAR *hash, ULONG hash_len, const UCHAR *signature,
                                      ULONG signature_len, ULONG flags )
{
    SYMCRYPT_ERROR error;

    if (signature_len != len_from_bitlen( key->a.bitlen ) * 2) return STATUS_INVALID_PARAMETER;

    error = SymCryptEcDsaVerify( key->a.ecc.handle, hash, hash_len, signature, signature_len,
                                 SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0 );

    if (error == SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE) return STATUS_INVALID_SIGNATURE;
    if (error) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS verify_signature_dsa( const struct key *key, const UCHAR *hash, ULONG hash_len, const UCHAR *signature,
                                      ULONG signature_len, ULONG flags )
{
    SYMCRYPT_ERROR error;

    if (signature_len != hash_len * 2) return STATUS_INVALID_PARAMETER;

    error = SymCryptDsaVerify( key->a.dsa.handle, hash, hash_len, signature, signature_len,
                               SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0 );

    if (error == SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE) return STATUS_INVALID_SIGNATURE;
    if (error) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptVerifySignature( BCRYPT_KEY_HANDLE handle, void *padding, UCHAR *hash, ULONG hash_len,
                                       UCHAR *signature, ULONG signature_len, ULONG flags )
{
    const struct key *key = get_key_object( handle );

    TRACE( "%p, %p, %p, %lu, %p, %lu, %#lx\n", handle, padding, hash, hash_len, signature, signature_len, flags );

    if (!key) return STATUS_INVALID_HANDLE;
    if (!hash || !hash_len || !signature || !signature_len) return STATUS_INVALID_PARAMETER;

    switch (key->alg_id)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        return verify_signature_rsa( key, padding, hash, hash_len, signature, signature_len, flags );

    case ALG_ID_ECDSA:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_ECDSA_P521:
        return verify_signature_ecc( key, hash, hash_len, signature, signature_len, flags );

    case ALG_ID_DSA:
        return verify_signature_dsa( key, hash, hash_len, signature, signature_len, flags );

    default:
        FIXME( "unhandled algorithm %u\n", key->alg_id );
        break;
    }

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS WINAPI BCryptDestroyKey( BCRYPT_KEY_HANDLE handle )
{
    struct key *key = get_key_object( handle );

    TRACE( "%p\n", handle );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;

    destroy_key( key );
    return STATUS_SUCCESS;
}

#define HMAC_PAD_LEN 64
NTSTATUS WINAPI BCryptDeriveKeyCapi( BCRYPT_HASH_HANDLE handle, BCRYPT_ALG_HANDLE halg, UCHAR *key, ULONG keylen,
                                     ULONG flags )
{
    struct hash *hash = get_hash_object( handle );
    UCHAR buf[MAX_HASH_OUTPUT_BYTES * 2];

    TRACE( "%p, %p, %p, %lu, %#lx\n", handle, halg, key, keylen, flags );

    if (!hash) return STATUS_INVALID_HANDLE;
    if (!key || !keylen || keylen > hash->len * 2) return STATUS_INVALID_PARAMETER;
    if (halg)
    {
        FIXME( "algorithm handle not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    finish_hash( hash, buf );

    if (hash->len < keylen)
    {
        UCHAR pad1[HMAC_PAD_LEN], pad2[HMAC_PAD_LEN];
        ULONG i;

        for (i = 0; i < sizeof(pad1); i++)
        {
            pad1[i] = 0x36 ^ (i < hash->len ? buf[i] : 0);
            pad2[i] = 0x5c ^ (i < hash->len ? buf[i] : 0);
        }

        prepare_hash( hash );
        hash_data( hash, pad1, sizeof(pad1) );
        finish_hash( hash, buf );

        prepare_hash( hash );
        hash_data( hash, pad2, sizeof(pad2) );
        finish_hash( hash, buf + hash->len );
    }

    memcpy( key, buf, keylen );
    return STATUS_SUCCESS;
}

static void destroy_secret( struct secret *secret )
{
    if (!secret) return;
    secure_zero( secret->derived_key, secret->derived_key_len );
    free( secret->derived_key );
    destroy_object( &secret->hdr );
}

NTSTATUS WINAPI BCryptSecretAgreement( BCRYPT_KEY_HANDLE privkey_handle, BCRYPT_KEY_HANDLE pubkey_handle,
                                       BCRYPT_SECRET_HANDLE *ret_handle, ULONG flags )
{
    const struct key *privkey = get_key_object( privkey_handle );
    const struct key *pubkey = get_key_object( pubkey_handle );
    struct secret *secret;

    TRACE( "%p, %p, %p, %#lx\n", privkey_handle, pubkey_handle, ret_handle, flags );

    if (!privkey || !pubkey || privkey->alg_id != pubkey->alg_id || !(privkey->flags & KEY_FLAG_PRIVATE))
        return STATUS_INVALID_HANDLE;
    if (!ret_handle) return STATUS_INVALID_PARAMETER;

    if (!(secret = calloc( 1, sizeof(*secret) ))) return STATUS_NO_MEMORY;
    secret->hdr.magic = MAGIC_SECRET;
    secret->derived_key_len = len_from_bitlen( privkey->a.bitlen );
    if (!(secret->derived_key = malloc( secret->derived_key_len )))
    {
        free( secret );
        return STATUS_NO_MEMORY;
    }

    switch (privkey->alg_id)
    {
    case ALG_ID_DH:
        if (SymCryptDhSecretAgreement( privkey->a.dsa.handle, pubkey->a.dsa.handle,
                                       SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, secret->derived_key,
                                       secret->derived_key_len ))
        {
            destroy_secret( secret );
            return STATUS_INTERNAL_ERROR;
        }
        break;
    case ALG_ID_ECDH:
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDH_P384:
    case ALG_ID_ECDH_P521:
        if (SymCryptEcDhSecretAgreement( privkey->a.ecc.handle, pubkey->a.ecc.handle,
                                         SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, secret->derived_key,
                                         secret->derived_key_len ))
        {
            destroy_secret( secret );
            return STATUS_INTERNAL_ERROR;
        }
        break;
    default:
        FIXME( "algorithm %u not supported\n", privkey->alg_id );
        destroy_secret( secret );
        return STATUS_NOT_SUPPORTED;
    }

    *ret_handle = secret;
    TRACE( "returning handle %p\n", *ret_handle );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDestroySecret( BCRYPT_SECRET_HANDLE handle )
{
    struct secret *secret = get_secret_object( handle );

    TRACE( "%p\n", handle );

    if (!secret || secret->hdr.magic != MAGIC_SECRET) return STATUS_INVALID_HANDLE;

    destroy_secret( secret );
    return STATUS_SUCCESS;
}

static const SYMCRYPT_MAC *get_mac_from_alg( enum alg_id alg )
{
    switch (alg)
    {
    case ALG_ID_SHA1:   return SymCryptHmacSha1Algorithm;
    case ALG_ID_SHA256: return SymCryptHmacSha256Algorithm;
    case ALG_ID_SHA384: return SymCryptHmacSha384Algorithm;
    case ALG_ID_SHA512: return SymCryptHmacSha512Algorithm;
    default:
        FIXME( "hash algorithm %u not supported\n", alg );
        return NULL;
    }
}

NTSTATUS WINAPI BCryptDeriveKeyPBKDF2( BCRYPT_ALG_HANDLE handle, UCHAR *pwd, ULONG pwd_len, UCHAR *salt, ULONG salt_len,
                                       ULONGLONG iterations, UCHAR *key, ULONG key_len, ULONG flags )
{
    const struct algorithm *alg = get_alg_object( handle );
    const SYMCRYPT_MAC *mac;

    TRACE( "%p, %p, %lu, %p, %lu, %s, %p, %lu, %#lx\n", handle, pwd, pwd_len, salt, salt_len,
           wine_dbgstr_longlong(iterations), key, key_len, flags );

    if (!alg || !(alg->flags & BCRYPT_ALG_HANDLE_HMAC_FLAG)) return STATUS_INVALID_HANDLE;
    if (!iterations) return STATUS_INVALID_PARAMETER;

    if (!(mac = get_mac_from_alg( alg->id ))) return STATUS_NOT_SUPPORTED;

    if (SymCryptPbkdf2( mac, pwd, pwd_len, salt, salt_len, iterations, key, key_len )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static const struct algorithm *get_hash_from_desc( const BCryptBufferDesc *desc )
{
    const struct algorithm *alg = get_alg_object( BCRYPT_SHA1_ALG_HANDLE );
    ULONG i;

    if (!desc) return alg;
    for (i = 0; i < desc->cBuffers; i++)
    {
        if (desc->pBuffers[i].BufferType == KDF_HASH_ALGORITHM)
        {
            const WCHAR *str = desc->pBuffers[i].pvBuffer;

            if (!wcscmp( str, BCRYPT_SHA1_ALGORITHM )) alg = get_alg_object( BCRYPT_SHA1_ALG_HANDLE );
            else if (!wcscmp( str, BCRYPT_SHA256_ALGORITHM )) alg = get_alg_object( BCRYPT_SHA256_ALG_HANDLE );
            else if (!wcscmp( str, BCRYPT_SHA384_ALGORITHM )) alg = get_alg_object( BCRYPT_SHA384_ALG_HANDLE );
            else if (!wcscmp( str, BCRYPT_SHA512_ALGORITHM )) alg = get_alg_object( BCRYPT_SHA512_ALG_HANDLE );
            else
            {
                FIXME( "hash %s not suported\n", debugstr_w(str) );
                return NULL;
            }
        }
        else if (desc->pBuffers[i].BufferType == KDF_SECRET_PREPEND) continue;
        else FIXME( "buffer type %lu not supported\n", desc->pBuffers[i].BufferType );
    }
    return alg;
}

static NTSTATUS derive_key_hash( const struct secret *secret, const BCryptBufferDesc *desc, UCHAR *output,
                                 ULONG output_len, ULONG *ret_len )
{
    ULONG derived_key_len = secret->derived_key_len, secret_len = 0;
    ULONG i, buffer_count = desc ? desc->cBuffers : 0;
    UCHAR *derived_key = secret->derived_key;
    const struct algorithm *alg;
    ULONG len;
    UCHAR hash[MAX_HASH_OUTPUT_BYTES];
    NTSTATUS status;

    if (!(alg = get_hash_from_desc( desc ))) return STATUS_NOT_SUPPORTED;
    len = builtin_algorithms[alg->id].hash_length;

    if (!output)
    {
        *ret_len = len;
        return STATUS_SUCCESS;
    }

    for (i = 0; i < buffer_count; i++)
        if (desc->pBuffers[i].BufferType == KDF_SECRET_PREPEND) secret_len += desc->pBuffers[i].cbBuffer;

    if (secret_len)
    {
        if (!(derived_key = malloc( secret_len + derived_key_len ))) return STATUS_NO_MEMORY;

        derived_key_len = 0;
        for (i = 0; i < buffer_count; i++)
        {
            if (desc->pBuffers[i].BufferType != KDF_SECRET_PREPEND) continue;
            memcpy( derived_key + derived_key_len, desc->pBuffers[i].pvBuffer, desc->pBuffers[i].cbBuffer );
            derived_key_len += desc->pBuffers[i].cbBuffer;
        }

        memcpy( derived_key + derived_key_len, secret->derived_key, secret->derived_key_len );
        derived_key_len += secret->derived_key_len;
    }

    status = hash_single( alg, NULL, 0, derived_key, derived_key_len, hash );
    if (derived_key != secret->derived_key) free( derived_key );
    if (status) return status;

    len = min( len, output_len );
    memcpy( output, hash, len );

    *ret_len = len;
    return STATUS_SUCCESS;
}

static NTSTATUS derive_key_raw( const struct secret *secret, UCHAR *output, ULONG output_len, ULONG *ret_len )
{
    ULONG i, len = secret->derived_key_len;

    if (!output)
    {
        *ret_len = len;
        return STATUS_SUCCESS;
    }

    len = min( len, output_len );
    for (i = 0; i < len; i++) output[i] = secret->derived_key[len - i - 1];

    *ret_len = len;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDeriveKey( BCRYPT_SECRET_HANDLE handle, const WCHAR *kdf, BCryptBufferDesc *desc, UCHAR *output,
                                 ULONG output_len, ULONG *ret_len, ULONG flags )
{
    const struct secret *secret = get_secret_object( handle );

    TRACE( "%p, %s, %p, %p, %lu, %p, %#lx\n", secret, debugstr_w(kdf), desc, output, output_len, ret_len, flags );
    if (flags) FIXME( "ignoring flags %#lx\n", flags );

    if (!secret) return STATUS_INVALID_HANDLE;
    if (!kdf || !ret_len) return STATUS_INVALID_PARAMETER;

    if (!wcscmp( kdf, BCRYPT_KDF_HASH ))
    {
        return derive_key_hash( secret, desc, output, output_len, ret_len );
    }
    if (!wcscmp( kdf, BCRYPT_KDF_RAW_SECRET ))
    {
        return derive_key_raw( secret, output, output_len, ret_len );
    }

    FIXME( "kdf %s not supportedi\n", debugstr_w(kdf) );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS key_derivation_pbkdf2( const struct key *key, BCryptBufferDesc *desc, UCHAR *output, ULONG output_len,
                                       ULONG *ret_len )
{
    const SYMCRYPT_MAC *mac = NULL;
    UINT64 iterations = 10000;
    ULONG salt_len = 0, i;
    const UCHAR *salt = NULL;

    if (desc)
    {
        for (i = 0; i < desc->cBuffers; i++)
        {
            switch (desc->pBuffers[i].BufferType)
            {
            case KDF_HASH_ALGORITHM:
                mac = get_hmac_from_hash( desc->pBuffers[i].pvBuffer );
                break;
            case KDF_SALT:
                salt = desc->pBuffers[i].pvBuffer;
                salt_len = desc->pBuffers[i].cbBuffer;
                break;
            case KDF_ITERATION_COUNT:
                if (desc->pBuffers[i].cbBuffer != sizeof(iterations)) return STATUS_INVALID_PARAMETER;
                iterations = *(UINT64 *)desc->pBuffers[i].pvBuffer;
                break;
            default:
                WARN( "unexpected buffer type %lu\n", desc->pBuffers[i].BufferType );
                break;
            }
        }
    }
    if (!mac) return STATUS_INVALID_PARAMETER;

    if (SymCryptPbkdf2( mac, key->s.secret, key->s.secret_len, salt, salt_len, iterations, output, output_len ))
        return STATUS_INTERNAL_ERROR;

    *ret_len = output_len;
    return STATUS_SUCCESS;
}

static NTSTATUS key_derivation_tls_prf( const struct key *key, BCryptBufferDesc *desc, UCHAR *output, ULONG output_len,
                                        ULONG *ret_len, BOOL tls1_2 )
{
    const SYMCRYPT_MAC *mac = NULL;
    ULONG label_len = 0, seed_len = 0, i;
    const UCHAR *label = NULL, *seed = NULL;
    SYMCRYPT_ERROR error;

    if (desc)
    {
        for (i = 0; i < desc->cBuffers; i++)
        {
            switch (desc->pBuffers[i].BufferType)
            {
            case KDF_TLS_PRF_LABEL:
                label = desc->pBuffers[i].pvBuffer;
                label_len = desc->pBuffers[i].cbBuffer;
                break;
            case KDF_TLS_PRF_SEED:
                seed = desc->pBuffers[i].pvBuffer;
                seed_len = desc->pBuffers[i].cbBuffer;
                break;
            case KDF_HASH_ALGORITHM:
                mac = get_hmac_from_hash( desc->pBuffers[i].pvBuffer );
                break;
            default:
                WARN( "unexpected buffer type %lu\n", desc->pBuffers[i].BufferType );
                break;
            }
        }
    }
    if (!seed || (tls1_2 && !mac)) return STATUS_INVALID_PARAMETER;

    if (tls1_2)
        error = SymCryptTlsPrf1_2( mac, key->s.secret, key->s.secret_len, label, label_len, seed, seed_len,
                                   output, output_len );
    else
        error = SymCryptTlsPrf1_1( key->s.secret, key->s.secret_len, label, label_len, seed, seed_len, output,
                                   output_len );
    if (error) return STATUS_INTERNAL_ERROR;

    *ret_len = output_len;
    return STATUS_SUCCESS;
}

static NTSTATUS key_derivation_hkdf( const struct key *key, BCryptBufferDesc *desc, UCHAR *output, ULONG output_len,
                                     ULONG *ret_len )
{
    const UCHAR *info = NULL;
    ULONG i, info_len = 0;

    if (desc)
    {
        for (i = 0; i < desc->cBuffers; i++)
        {
            switch (desc->pBuffers[i].BufferType)
            {
            case KDF_HKDF_INFO:
                info = desc->pBuffers[i].pvBuffer;
                info_len = desc->pBuffers[i].cbBuffer;
                break;
            default:
                WARN( "unexpected buffer type %lu\n", desc->pBuffers[i].BufferType );
                break;
            }
        }
    }

    if (SymCryptHkdfDerive( &key->s.hkdf.handle, info, info_len, output, output_len )) return STATUS_INTERNAL_ERROR;

    *ret_len = output_len;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptKeyDerivation( BCRYPT_KEY_HANDLE handle, BCryptBufferDesc *desc, UCHAR *output, ULONG output_len,
                                     ULONG *ret_len, ULONG flags )
{
    const struct key *key = get_key_object( handle );
    NTSTATUS status;

    TRACE( "%p, %p, %p, %lu, %p, %#lx\n", key, desc, output, output_len, ret_len, flags );

    if (!key || !output || !ret_len) return STATUS_INVALID_PARAMETER;

    switch (key->alg_id)
    {
    case ALG_ID_PBKDF2:
        status = key_derivation_pbkdf2( key, desc, output, output_len, ret_len );
        break;
    case ALG_ID_TLS1_1_KDF:
        status = key_derivation_tls_prf( key, desc, output, output_len, ret_len, FALSE );
        break;
    case ALG_ID_TLS1_2_KDF:
        status = key_derivation_tls_prf( key, desc, output, output_len, ret_len, TRUE );
        break;
    case ALG_ID_HKDF:
        status = key_derivation_hkdf( key, desc, output, output_len, ret_len );
        break;
    default:
        FIXME( "unsupported algorithm %u\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return status;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        SymCryptInit();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        break;
    }
    return TRUE;
}
