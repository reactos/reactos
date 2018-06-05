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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef HAVE_COMMONCRYPTO_COMMONCRYPTOR_H
#include <AvailabilityMacros.h>
#include <CommonCrypto/CommonCryptor.h>
#elif defined(HAVE_GNUTLS_CIPHER_INIT)
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/library.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static HINSTANCE instance;

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#if GNUTLS_VERSION_MAJOR < 3
#define GNUTLS_CIPHER_AES_192_CBC 92
#define GNUTLS_CIPHER_AES_128_GCM 93
#define GNUTLS_CIPHER_AES_256_GCM 94
#define GNUTLS_PK_ECC 4

typedef enum
{
    GNUTLS_ECC_CURVE_INVALID = 0,
    GNUTLS_ECC_CURVE_SECP224R1,
    GNUTLS_ECC_CURVE_SECP256R1,
    GNUTLS_ECC_CURVE_SECP384R1,
    GNUTLS_ECC_CURVE_SECP521R1,
} gnutls_ecc_curve_t;
#endif

/* Not present in gnutls version < 3.0 */
static int (*pgnutls_cipher_tag)(gnutls_cipher_hd_t handle, void * tag, size_t tag_size);
static int (*pgnutls_cipher_add_auth)(gnutls_cipher_hd_t handle, const void *ptext, size_t ptext_size);
static int (*pgnutls_pubkey_import_ecc_raw)(gnutls_pubkey_t key, gnutls_ecc_curve_t curve,
                                            const gnutls_datum_t *x, const gnutls_datum_t *y);
static gnutls_sign_algorithm_t (*pgnutls_pk_to_sign)(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash);
static int (*pgnutls_pubkey_verify_hash2)(gnutls_pubkey_t key, gnutls_sign_algorithm_t algo,
                                          unsigned int flags, const gnutls_datum_t *hash,
                                          const gnutls_datum_t *signature);

/* Not present in gnutls version < 2.11.0 */
static int (*pgnutls_pubkey_import_rsa_raw)(gnutls_pubkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_cipher_decrypt2);
MAKE_FUNCPTR(gnutls_cipher_deinit);
MAKE_FUNCPTR(gnutls_cipher_encrypt2);
MAKE_FUNCPTR(gnutls_cipher_init);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_pubkey_init);
MAKE_FUNCPTR(gnutls_pubkey_deinit);
#undef MAKE_FUNCPTR

static int compat_gnutls_cipher_tag(gnutls_cipher_hd_t handle, void *tag, size_t tag_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_cipher_add_auth(gnutls_cipher_hd_t handle, const void *ptext, size_t ptext_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_pubkey_import_ecc_raw(gnutls_pubkey_t key, gnutls_ecc_curve_t curve,
                                               const gnutls_datum_t *x, const gnutls_datum_t *y)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static gnutls_sign_algorithm_t compat_gnutls_pk_to_sign(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash)
{
    return GNUTLS_SIGN_UNKNOWN;
}

static int compat_gnutls_pubkey_verify_hash2(gnutls_pubkey_t key, gnutls_sign_algorithm_t algo,
                                             unsigned int flags, const gnutls_datum_t *hash,
                                             const gnutls_datum_t *signature)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_pubkey_import_rsa_raw(gnutls_pubkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

static BOOL gnutls_initialize(void)
{
    int ret;

    if (!(libgnutls_handle = wine_dlopen( SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for encryption\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libgnutls_handle, #f, NULL, 0 ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_cipher_decrypt2)
    LOAD_FUNCPTR(gnutls_cipher_deinit)
    LOAD_FUNCPTR(gnutls_cipher_encrypt2)
    LOAD_FUNCPTR(gnutls_cipher_init)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_pubkey_init);
    LOAD_FUNCPTR(gnutls_pubkey_deinit);
#undef LOAD_FUNCPTR

    if (!(pgnutls_cipher_tag = wine_dlsym( libgnutls_handle, "gnutls_cipher_tag", NULL, 0 )))
    {
        WARN("gnutls_cipher_tag not found\n");
        pgnutls_cipher_tag = compat_gnutls_cipher_tag;
    }
    if (!(pgnutls_cipher_add_auth = wine_dlsym( libgnutls_handle, "gnutls_cipher_add_auth", NULL, 0 )))
    {
        WARN("gnutls_cipher_add_auth not found\n");
        pgnutls_cipher_add_auth = compat_gnutls_cipher_add_auth;
    }

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
    }
    if (!(pgnutls_pubkey_import_ecc_raw = wine_dlsym( libgnutls_handle, "gnutls_pubkey_import_ecc_raw", NULL, 0 )))
    {
        WARN("gnutls_pubkey_import_ecc_raw not found\n");
        pgnutls_pubkey_import_ecc_raw = compat_gnutls_pubkey_import_ecc_raw;
    }
    if (!(pgnutls_pk_to_sign = wine_dlsym( libgnutls_handle, "gnutls_pk_to_sign", NULL, 0 )))
    {
        WARN("gnutls_pk_to_sign not found\n");
        pgnutls_pk_to_sign = compat_gnutls_pk_to_sign;
    }
    if (!(pgnutls_pubkey_verify_hash2 = wine_dlsym( libgnutls_handle, "gnutls_pubkey_verify_hash2", NULL, 0 )))
    {
        WARN("gnutls_pubkey_verify_hash2 not found\n");
        pgnutls_pubkey_verify_hash2 = compat_gnutls_pubkey_verify_hash2;
    }
    if (!(pgnutls_pubkey_import_rsa_raw = wine_dlsym( libgnutls_handle, "gnutls_pubkey_import_rsa_raw", NULL, 0 )))
    {
        WARN("gnutls_pubkey_import_rsa_raw not found\n");
        pgnutls_pubkey_import_rsa_raw = compat_gnutls_pubkey_import_rsa_raw;
    }

    if (TRACE_ON( bcrypt ))
    {
        pgnutls_global_set_log_level( 4 );
        pgnutls_global_set_log_function( gnutls_log );
    }

    return TRUE;

fail:
    wine_dlclose( libgnutls_handle, NULL, 0 );
    libgnutls_handle = NULL;
    return FALSE;
}

static void gnutls_uninitialize(void)
{
    pgnutls_global_deinit();
    wine_dlclose( libgnutls_handle, NULL, 0 );
    libgnutls_handle = NULL;
}
#endif /* HAVE_GNUTLS_CIPHER_INIT && !HAVE_COMMONCRYPTO_COMMONCRYPTOR_H */

NTSTATUS WINAPI BCryptAddContextFunction(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, ULONG pos)
{
    FIXME("%08x, %s, %08x, %s, %u: stub\n", table, debugstr_w(context), iface, debugstr_w(function), pos);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptAddContextFunctionProvider(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, LPCWSTR provider, ULONG pos)
{
    FIXME("%08x, %s, %08x, %s, %s, %u: stub\n", table, debugstr_w(context), iface, debugstr_w(function), debugstr_w(provider), pos);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptRemoveContextFunction(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function)
{
    FIXME("%08x, %s, %08x, %s: stub\n", table, debugstr_w(context), iface, debugstr_w(function));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptRemoveContextFunctionProvider(ULONG table, LPCWSTR context, ULONG iface, LPCWSTR function, LPCWSTR provider)
{
    FIXME("%08x, %s, %08x, %s, %s: stub\n", table, debugstr_w(context), iface, debugstr_w(function), debugstr_w(provider));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptRegisterProvider(LPCWSTR provider, ULONG flags, PCRYPT_PROVIDER_REG reg)
{
    FIXME("%s, %08x, %p: stub\n", debugstr_w(provider), flags, reg);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptUnregisterProvider(LPCWSTR provider)
{
    FIXME("%s: stub\n", debugstr_w(provider));
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG dwAlgOperations, ULONG *pAlgCount,
                                     BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags)
{
    FIXME("%08x, %p, %p, %08x - stub\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags);

    *ppAlgList=NULL;
    *pAlgCount=0;

    return STATUS_NOT_IMPLEMENTED;
}

#define MAGIC_ALG  (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
#define MAGIC_KEY  (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
struct object
{
    ULONG magic;
};

enum alg_id
{
    ALG_ID_AES,
    ALG_ID_MD2,
    ALG_ID_MD4,
    ALG_ID_MD5,
    ALG_ID_RNG,
    ALG_ID_RSA,
    ALG_ID_SHA1,
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
};

enum mode_id
{
    MODE_ID_ECB,
    MODE_ID_CBC,
    MODE_ID_GCM
};

#define MAX_HASH_OUTPUT_BYTES 64
#define MAX_HASH_BLOCK_BITS 1024

static const struct {
    ULONG object_length;
    ULONG hash_length;
    ULONG block_bits;
    const WCHAR *alg_name;
    BOOL symmetric;
} alg_props[] = {
    /* ALG_ID_AES    */ {  654,    0,    0, BCRYPT_AES_ALGORITHM,    TRUE },
    /* ALG_ID_MD2    */ {  270,   16,  128, BCRYPT_MD2_ALGORITHM,    FALSE },
    /* ALG_ID_MD4    */ {  270,   16,  512, BCRYPT_MD4_ALGORITHM,    FALSE },
    /* ALG_ID_MD5    */ {  274,   16,  512, BCRYPT_MD5_ALGORITHM,    FALSE },
    /* ALG_ID_RNG    */ {    0,    0,    0, BCRYPT_RNG_ALGORITHM,    FALSE },
    /* ALG_ID_RSA    */ {    0,    0,    0, BCRYPT_RSA_ALGORITHM,    FALSE },
    /* ALG_ID_SHA1   */ {  278,   20,  512, BCRYPT_SHA1_ALGORITHM,   FALSE },
    /* ALG_ID_SHA256 */ {  286,   32,  512, BCRYPT_SHA256_ALGORITHM, FALSE },
    /* ALG_ID_SHA384 */ {  382,   48, 1024, BCRYPT_SHA384_ALGORITHM, FALSE },
    /* ALG_ID_SHA512 */ {  382,   64, 1024, BCRYPT_SHA512_ALGORITHM, FALSE },
    /* ALG_ID_ECDSA_P256 */ { 0,   0,     0, BCRYPT_ECDSA_P256_ALGORITHM, FALSE  },
    /* ALG_ID_ECDSA_P384 */ { 0,   0,     0, BCRYPT_ECDSA_P384_ALGORITHM, FALSE  },
};

struct algorithm
{
    struct object hdr;
    enum alg_id   id;
    enum mode_id  mode;
    BOOL hmac;
};

struct key;
static NTSTATUS set_key_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags );

NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE handle, UCHAR *buffer, ULONG count, ULONG flags)
{
    const DWORD supported_flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
    struct algorithm *algorithm = handle;

    TRACE("%p, %p, %u, %08x - semi-stub\n", handle, buffer, count, flags);

    if (!algorithm)
    {
        /* It's valid to call without an algorithm if BCRYPT_USE_SYSTEM_PREFERRED_RNG
         * is set. In this case the preferred system RNG is used.
         */
        if (!(flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
            return STATUS_INVALID_HANDLE;
    }
    else if (algorithm->hdr.magic != MAGIC_ALG || algorithm->id != ALG_ID_RNG)
        return STATUS_INVALID_HANDLE;

    if (!buffer)
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags)
        FIXME("unsupported flags %08x\n", flags & ~supported_flags);

    if (algorithm)
        FIXME("ignoring selected algorithm\n");

    /* When zero bytes are requested the function returns success too. */
    if (!count)
        return STATUS_SUCCESS;

    if (algorithm || (flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
    {
        if (RtlGenRandom(buffer, count))
            return STATUS_SUCCESS;
    }

    FIXME("called with unsupported parameters, returning error\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptOpenAlgorithmProvider( BCRYPT_ALG_HANDLE *handle, LPCWSTR id, LPCWSTR implementation, DWORD flags )
{
    const DWORD supported_flags = BCRYPT_ALG_HANDLE_HMAC_FLAG;
    struct algorithm *alg;
    enum alg_id alg_id;

    TRACE( "%p, %s, %s, %08x\n", handle, wine_dbgstr_w(id), wine_dbgstr_w(implementation), flags );

    if (!handle || !id) return STATUS_INVALID_PARAMETER;
    if (flags & ~supported_flags)
    {
        FIXME( "unsupported flags %08x\n", flags & ~supported_flags);
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( id, BCRYPT_AES_ALGORITHM )) alg_id = ALG_ID_AES;
    else if (!strcmpW( id, BCRYPT_MD2_ALGORITHM )) alg_id = ALG_ID_MD2;
    else if (!strcmpW( id, BCRYPT_MD4_ALGORITHM )) alg_id = ALG_ID_MD4;
    else if (!strcmpW( id, BCRYPT_MD5_ALGORITHM )) alg_id = ALG_ID_MD5;
    else if (!strcmpW( id, BCRYPT_RNG_ALGORITHM )) alg_id = ALG_ID_RNG;
    else if (!strcmpW( id, BCRYPT_RSA_ALGORITHM )) alg_id = ALG_ID_RSA;
    else if (!strcmpW( id, BCRYPT_SHA1_ALGORITHM )) alg_id = ALG_ID_SHA1;
    else if (!strcmpW( id, BCRYPT_SHA256_ALGORITHM )) alg_id = ALG_ID_SHA256;
    else if (!strcmpW( id, BCRYPT_SHA384_ALGORITHM )) alg_id = ALG_ID_SHA384;
    else if (!strcmpW( id, BCRYPT_SHA512_ALGORITHM )) alg_id = ALG_ID_SHA512;
    else if (!strcmpW( id, BCRYPT_ECDSA_P256_ALGORITHM )) alg_id = ALG_ID_ECDSA_P256;
    else if (!strcmpW( id, BCRYPT_ECDSA_P384_ALGORITHM )) alg_id = ALG_ID_ECDSA_P384;
    else
    {
        FIXME( "algorithm %s not supported\n", debugstr_w(id) );
        return STATUS_NOT_IMPLEMENTED;
    }
    if (implementation && strcmpW( implementation, MS_PRIMITIVE_PROVIDER ))
    {
        FIXME( "implementation %s not supported\n", debugstr_w(implementation) );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(alg = heap_alloc( sizeof(*alg) ))) return STATUS_NO_MEMORY;
    alg->hdr.magic = MAGIC_ALG;
    alg->id        = alg_id;
    alg->mode      = MODE_ID_CBC;
    alg->hmac      = flags & BCRYPT_ALG_HANDLE_HMAC_FLAG;

    *handle = alg;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptCloseAlgorithmProvider( BCRYPT_ALG_HANDLE handle, DWORD flags )
{
    struct algorithm *alg = handle;

    TRACE( "%p, %08x\n", handle, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    heap_free( alg );
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

struct hash_impl
{
    union
    {
        MD2_CTX md2;
        MD4_CTX md4;
        MD5_CTX md5;
        SHA_CTX sha1;
        SHA256_CTX sha256;
        SHA512_CTX sha512;
    } u;
};

static NTSTATUS hash_init( struct hash_impl *hash, enum alg_id alg_id )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_init( &hash->u.md2 );
        break;

    case ALG_ID_MD4:
        MD4Init( &hash->u.md4 );
        break;

    case ALG_ID_MD5:
        MD5Init( &hash->u.md5 );
        break;

    case ALG_ID_SHA1:
        A_SHAInit( &hash->u.sha1 );
        break;

    case ALG_ID_SHA256:
        sha256_init( &hash->u.sha256 );
        break;

    case ALG_ID_SHA384:
        sha384_init( &hash->u.sha512 );
        break;

    case ALG_ID_SHA512:
        sha512_init( &hash->u.sha512 );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hash_update( struct hash_impl *hash, enum alg_id alg_id,
                             UCHAR *input, ULONG size )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_update( &hash->u.md2, input, size );
        break;

    case ALG_ID_MD4:
        MD4Update( &hash->u.md4, input, size );
        break;

    case ALG_ID_MD5:
        MD5Update( &hash->u.md5, input, size );
        break;

    case ALG_ID_SHA1:
        A_SHAUpdate( &hash->u.sha1, input, size );
        break;

    case ALG_ID_SHA256:
        sha256_update( &hash->u.sha256, input, size );
        break;

    case ALG_ID_SHA384:
        sha384_update( &hash->u.sha512, input, size );
        break;

    case ALG_ID_SHA512:
        sha512_update( &hash->u.sha512, input, size );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash_impl *hash, enum alg_id alg_id,
                             UCHAR *output, ULONG size )
{
    switch (alg_id)
    {
    case ALG_ID_MD2:
        md2_finalize( &hash->u.md2, output );
        break;

    case ALG_ID_MD4:
        MD4Final( &hash->u.md4 );
        memcpy( output, hash->u.md4.digest, 16 );
        break;

    case ALG_ID_MD5:
        MD5Final( &hash->u.md5 );
        memcpy( output, hash->u.md5.digest, 16 );
        break;

    case ALG_ID_SHA1:
        A_SHAFinal( &hash->u.sha1, (ULONG *)output );
        break;

    case ALG_ID_SHA256:
        sha256_finalize( &hash->u.sha256, output );
        break;

    case ALG_ID_SHA384:
        sha384_finalize( &hash->u.sha512, output );
        break;

    case ALG_ID_SHA512:
        sha512_finalize( &hash->u.sha512, output );
        break;

    default:
        ERR( "unhandled id %u\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    BOOL             hmac;
    struct hash_impl outer;
    struct hash_impl inner;
};

#define BLOCK_LENGTH_AES        16

static NTSTATUS generic_alg_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
    {
        if (!alg_props[id].object_length)
            return STATUS_NOT_SUPPORTED;
        *ret_size = sizeof(ULONG);
        if (size < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;
        if (buf)
            *(ULONG *)buf = alg_props[id].object_length;
        return STATUS_SUCCESS;
    }

    if (!strcmpW( prop, BCRYPT_HASH_LENGTH ))
    {
        if (!alg_props[id].hash_length)
            return STATUS_NOT_SUPPORTED;
        *ret_size = sizeof(ULONG);
        if (size < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;
        if(buf)
            *(ULONG*)buf = alg_props[id].hash_length;
        return STATUS_SUCCESS;
    }

    if (!strcmpW( prop, BCRYPT_ALGORITHM_NAME ))
    {
        *ret_size = (strlenW(alg_props[id].alg_name)+1)*sizeof(WCHAR);
        if (size < *ret_size)
            return STATUS_BUFFER_TOO_SMALL;
        if(buf)
            memcpy(buf, alg_props[id].alg_name, *ret_size);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS get_alg_property( const struct algorithm *alg, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    status = generic_alg_property( alg->id, prop, buf, size, ret_size );
    if (status != STATUS_NOT_IMPLEMENTED)
        return status;

    switch (alg->id)
    {
    case ALG_ID_AES:
        if (!strcmpW( prop, BCRYPT_BLOCK_LENGTH ))
        {
            *ret_size = sizeof(ULONG);
            if (size < sizeof(ULONG))
                return STATUS_BUFFER_TOO_SMALL;
            if (buf)
                *(ULONG *)buf = BLOCK_LENGTH_AES;
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            const WCHAR *mode;
            switch (alg->mode)
            {
                case MODE_ID_ECB: mode = BCRYPT_CHAIN_MODE_ECB; break;
                case MODE_ID_CBC: mode = BCRYPT_CHAIN_MODE_CBC; break;
                case MODE_ID_GCM: mode = BCRYPT_CHAIN_MODE_GCM; break;
                default: return STATUS_NOT_IMPLEMENTED;
            }

            *ret_size = 64;
            if (size < *ret_size) return STATUS_BUFFER_TOO_SMALL;
            memcpy( buf, mode, (strlenW(mode) + 1) * sizeof(WCHAR) );
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTHS ))
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
        if (!strcmpW( prop, BCRYPT_AUTH_TAG_LENGTH ))
        {
            BCRYPT_AUTH_TAG_LENGTHS_STRUCT *tag_length = (void *)buf;
            if (alg->mode != MODE_ID_GCM) return STATUS_NOT_SUPPORTED;
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
        break;

    default:
        break;
    }

    FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS set_alg_property( struct algorithm *alg, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    switch (alg->id)
    {
    case ALG_ID_AES:
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB, size ))
            {
                alg->mode = MODE_ID_ECB;
                return STATUS_SUCCESS;
            }
            else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC, size ))
            {
                alg->mode = MODE_ID_CBC;
                return STATUS_SUCCESS;
            }
            else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM, size ))
            {
                alg->mode = MODE_ID_GCM;
                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "unsupported mode %s\n", debugstr_wn( (WCHAR *)value, size ) );
                return STATUS_NOT_IMPLEMENTED;
            }
        }
        FIXME( "unsupported aes algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    default:
        FIXME( "unsupported algorithm %u\n", alg->id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS get_hash_property( const struct hash *hash, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    status = generic_alg_property( hash->alg_id, prop, buf, size, ret_size );
    if (status == STATUS_NOT_IMPLEMENTED)
        FIXME( "unsupported property %s\n", debugstr_w(prop) );
    return status;
}

NTSTATUS WINAPI BCryptGetProperty( BCRYPT_HANDLE handle, LPCWSTR prop, UCHAR *buffer, ULONG count, ULONG *res, ULONG flags )
{
    struct object *object = handle;

    TRACE( "%p, %s, %p, %u, %p, %08x\n", handle, wine_dbgstr_w(prop), buffer, count, res, flags );

    if (!object) return STATUS_INVALID_HANDLE;
    if (!prop || !res) return STATUS_INVALID_PARAMETER;

    switch (object->magic)
    {
    case MAGIC_ALG:
    {
        const struct algorithm *alg = (const struct algorithm *)object;
        return get_alg_property( alg, prop, buffer, count, res );
    }
    case MAGIC_HASH:
    {
        const struct hash *hash = (const struct hash *)object;
        return get_hash_property( hash, prop, buffer, count, res );
    }
    default:
        WARN( "unknown magic %08x\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

NTSTATUS WINAPI BCryptSetProperty( BCRYPT_HANDLE handle, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    struct object *object = handle;

    TRACE( "%p, %s, %p, %u, %08x\n", handle, debugstr_w(prop), value, size, flags );

    if (!object) return STATUS_INVALID_HANDLE;

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
        WARN( "unknown magic %08x\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

NTSTATUS WINAPI BCryptCreateHash( BCRYPT_ALG_HANDLE algorithm, BCRYPT_HASH_HANDLE *handle, UCHAR *object, ULONG objectlen,
                                  UCHAR *secret, ULONG secretlen, ULONG flags )
{
    struct algorithm *alg = algorithm;
    UCHAR buffer[MAX_HASH_BLOCK_BITS / 8] = {0};
    struct hash *hash;
    int block_bytes;
    NTSTATUS status;
    int i;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x - stub\n", algorithm, handle, object, objectlen,
           secret, secretlen, flags );
    if (flags)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash = heap_alloc( sizeof(*hash) ))) return STATUS_NO_MEMORY;
    hash->hdr.magic = MAGIC_HASH;
    hash->alg_id    = alg->id;
    hash->hmac      = alg->hmac;

    /* initialize hash */
    if ((status = hash_init( &hash->inner, hash->alg_id ))) goto end;
    if (!hash->hmac) goto end;

    /* initialize hmac */
    if ((status = hash_init( &hash->outer, hash->alg_id ))) goto end;
    block_bytes = alg_props[hash->alg_id].block_bits / 8;
    if (secretlen > block_bytes)
    {
        struct hash_impl temp;
        if ((status = hash_init( &temp, hash->alg_id ))) goto end;
        if ((status = hash_update( &temp, hash->alg_id, secret, secretlen ))) goto end;
        if ((status = hash_finish( &temp, hash->alg_id, buffer,
                                   alg_props[hash->alg_id].hash_length ))) goto end;
    }
    else
    {
        memcpy( buffer, secret, secretlen );
    }
    for (i = 0; i < block_bytes; i++) buffer[i] ^= 0x5c;
    if ((status = hash_update( &hash->outer, hash->alg_id, buffer, block_bytes ))) goto end;
    for (i = 0; i < block_bytes; i++) buffer[i] ^= (0x5c ^ 0x36);
    status = hash_update( &hash->inner, hash->alg_id, buffer, block_bytes );

end:
    if (status != STATUS_SUCCESS)
    {
        heap_free( hash );
        return status;
    }

    *handle = hash;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateHash( BCRYPT_HASH_HANDLE handle, BCRYPT_HASH_HANDLE *handle_copy,
                                     UCHAR *object, ULONG objectlen, ULONG flags )
{
    struct hash *hash_orig = handle;
    struct hash *hash_copy;

    TRACE( "%p, %p, %p, %u, %u\n", handle, handle_copy, object, objectlen, flags );

    if (!hash_orig || hash_orig->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash_copy = heap_alloc( sizeof(*hash_copy) )))
        return STATUS_NO_MEMORY;

    memcpy( hash_copy, hash_orig, sizeof(*hash_orig) );

    *handle_copy = hash_copy;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDestroyHash( BCRYPT_HASH_HANDLE handle )
{
    struct hash *hash = handle;

    TRACE( "%p\n", handle );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    heap_free( hash );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptHashData( BCRYPT_HASH_HANDLE handle, UCHAR *input, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, input, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!input) return STATUS_SUCCESS;

    return hash_update( &hash->inner, hash->alg_id, input, size );
}

NTSTATUS WINAPI BCryptFinishHash( BCRYPT_HASH_HANDLE handle, UCHAR *output, ULONG size, ULONG flags )
{
    UCHAR buffer[MAX_HASH_OUTPUT_BYTES];
    struct hash *hash = handle;
    NTSTATUS status;
    int hash_length;

    TRACE( "%p, %p, %u, %08x\n", handle, output, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!output) return STATUS_INVALID_PARAMETER;

    if (!hash->hmac)
        return hash_finish( &hash->inner, hash->alg_id, output, size );

    hash_length = alg_props[hash->alg_id].hash_length;
    if ((status = hash_finish( &hash->inner, hash->alg_id, buffer, hash_length ))) return status;
    if ((status = hash_update( &hash->outer, hash->alg_id, buffer, hash_length ))) return status;
    return hash_finish( &hash->outer, hash->alg_id, output, size );
}

NTSTATUS WINAPI BCryptHash( BCRYPT_ALG_HANDLE algorithm, UCHAR *secret, ULONG secretlen,
                            UCHAR *input, ULONG inputlen, UCHAR *output, ULONG outputlen )
{
    NTSTATUS status;
    BCRYPT_HASH_HANDLE handle;

    TRACE( "%p, %p, %u, %p, %u, %p, %u\n", algorithm, secret, secretlen,
           input, inputlen, output, outputlen );

    status = BCryptCreateHash( algorithm, &handle, NULL, 0, secret, secretlen, 0);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = BCryptHashData( handle, input, inputlen, 0 );
    if (status != STATUS_SUCCESS)
    {
        BCryptDestroyHash( handle );
        return status;
    }

    status = BCryptFinishHash( handle, output, outputlen, 0 );
    if (status != STATUS_SUCCESS)
    {
        BCryptDestroyHash( handle );
        return status;
    }

    return BCryptDestroyHash( handle );
}

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
struct key_symmetric
{
    enum mode_id       mode;
    ULONG              block_size;
    gnutls_cipher_hd_t handle;
    UCHAR             *secret;
    ULONG              secret_len;
};

struct key_asymmetric
{
    UCHAR             *pubkey;
    ULONG              pubkey_len;
};

struct key
{
    struct object      hdr;
    enum alg_id        alg_id;
    union
    {
        struct key_symmetric s;
        struct key_asymmetric a;
    } u;
};

#elif defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
struct key_symmetric
{
    enum mode_id   mode;
    ULONG          block_size;
    CCCryptorRef   ref_encrypt;
    CCCryptorRef   ref_decrypt;
    UCHAR         *secret;
    ULONG          secret_len;
};

struct key_asymmetric
{
    UCHAR             *pubkey;
    ULONG              pubkey_len;
};

struct key
{
    struct object  hdr;
    enum alg_id    alg_id;
    union
    {
        struct key_symmetric s;
        struct key_asymmetric a;
    } u;
};
#else
struct key_symmetric
{
    enum mode_id  mode;
    ULONG         block_size;
};

struct key
{
    struct object  hdr;
    union
    {
        struct key_symmetric s;
    } u;
};
#endif

#if defined(HAVE_GNUTLS_CIPHER_INIT) || defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
static inline BOOL key_is_symmetric( struct key *key )
{
    return alg_props[key->alg_id].symmetric;
}

static inline BOOL key_is_asymmetric( struct key *key )
{
    return !alg_props[key->alg_id].symmetric;
}

static NTSTATUS key_symmetric_get_mode( struct key *key, enum mode_id *mode )
{
    *mode = key->u.s.mode;
    return STATUS_SUCCESS;
}

static NTSTATUS key_symmetric_get_blocksize( struct key *key, ULONG *size )
{
    *size = key->u.s.block_size;
    return STATUS_SUCCESS;
}

static ULONG get_block_size( struct algorithm *alg )
{
    ULONG ret = 0, size = sizeof(ret);
    get_alg_property( alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&ret, sizeof(ret), &size );
    return ret;
}
static NTSTATUS key_export( struct key *key, const WCHAR *type, UCHAR *output, ULONG output_len, ULONG *size )
{
    if (!strcmpW( type, BCRYPT_KEY_DATA_BLOB ))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *header = (BCRYPT_KEY_DATA_BLOB_HEADER *)output;
        ULONG req_size = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key->u.s.secret_len;

        *size = req_size;
        if (output_len < req_size) return STATUS_BUFFER_TOO_SMALL;

        header->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
        header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
        header->cbKeyData = key->u.s.secret_len;
        memcpy( &header[1], key->u.s.secret, key->u.s.secret_len );
        return STATUS_SUCCESS;
    }

    FIXME( "unsupported key type %s\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}
#else
static inline BOOL key_is_symmetric( struct key *key )
{
    ERR( "support for keys not available at build time\n" );
    return FALSE;
}

static inline BOOL key_is_asymmetric( struct key *key )
{
    ERR( "support for keys not available at build time\n" );
    return FALSE;
}

static NTSTATUS key_symmetric_get_mode( struct key *key, enum mode_id *mode )
{
    *mode = key->u.s.mode;
    return STATUS_SUCCESS;
}

static NTSTATUS key_symmetric_get_blocksize( struct key *key, ULONG *size )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}
#endif

#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
static NTSTATUS key_symmetric_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    UCHAR *buffer;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (alg->id)
    {
    case ALG_ID_AES:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->u.s.block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(buffer = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, secret, secret_len );

    key->alg_id         = alg->id;
    key->u.s.mode       = alg->mode;
    key->u.s.handle     = 0;        /* initialized on first use */
    key->u.s.secret     = buffer;
    key->u.s.secret_len = secret_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_asymmetric_init( struct key *key, struct algorithm *alg, const UCHAR *pubkey, ULONG pubkey_len )
{
    UCHAR *buffer;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (alg->id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_RSA:
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(buffer = heap_alloc( pubkey_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, pubkey, pubkey_len );

    key->alg_id         = alg->id;
    key->u.a.pubkey     = buffer;
    key->u.a.pubkey_len = pubkey_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_duplicate( struct key *key_orig, struct key *key_copy )
{
    UCHAR *buffer;

    key_copy->hdr               = key_orig->hdr;
    key_copy->alg_id            = key_orig->alg_id;

    if (key_is_symmetric(key_orig))
    {
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, key_orig->u.s.secret_len ))) return STATUS_NO_MEMORY;
        memcpy( buffer, key_orig->u.s.secret, key_orig->u.s.secret_len );

        key_copy->u.s.mode          = key_orig->u.s.mode;
        key_copy->u.s.block_size    = key_orig->u.s.block_size;
        key_copy->u.s.handle        = NULL;
        key_copy->u.s.secret        = buffer;
        key_copy->u.s.secret_len    = key_orig->u.s.secret_len;
    }
    else
    {
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, key_orig->u.a.pubkey_len ))) return STATUS_NO_MEMORY;
        memcpy( buffer, key_orig->u.a.pubkey, key_orig->u.a.pubkey_len );

        key_copy->u.a.pubkey      = buffer;
        key_copy->u.a.pubkey_len  = key_orig->u.a.pubkey_len;

        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS set_key_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
    {
        if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB, size ))
        {
            key->u.s.mode = MODE_ID_ECB;
            return STATUS_SUCCESS;
        }
        else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC, size ))
        {
            key->u.s.mode = MODE_ID_CBC;
            return STATUS_SUCCESS;
        }
        else if (!strncmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM, size ))
        {
            key->u.s.mode = MODE_ID_GCM;
            return STATUS_SUCCESS;
        }
        else
        {
            FIXME( "unsupported mode %s\n", debugstr_wn( (WCHAR *)value, size ) );
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    FIXME( "unsupported key property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static gnutls_cipher_algorithm_t get_gnutls_cipher( const struct key *key )
{
    switch (key->alg_id)
    {
    case ALG_ID_AES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
            case MODE_ID_GCM:
                if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_GCM;
                if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_GCM;
                break;
            case MODE_ID_ECB: /* can be emulated with CBC + empty IV */
            case MODE_ID_CBC:
                if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_CBC;
                if (key->u.s.secret_len == 24) return GNUTLS_CIPHER_AES_192_CBC;
                if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_CBC;
                break;
            default:
                break;
        }
        FIXME( "aes mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return GNUTLS_CIPHER_UNKNOWN;
    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return GNUTLS_CIPHER_UNKNOWN;
    }
}

static NTSTATUS key_symmetric_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    static const UCHAR zero_iv[16];
    gnutls_cipher_algorithm_t cipher;
    gnutls_datum_t secret, vector;
    int ret;

    if (key->u.s.handle)
    {
        pgnutls_cipher_deinit( key->u.s.handle );
        key->u.s.handle = NULL;
    }

    if ((cipher = get_gnutls_cipher( key )) == GNUTLS_CIPHER_UNKNOWN)
        return STATUS_NOT_SUPPORTED;

    if (!iv)
    {
        iv      = (UCHAR *)zero_iv;
        iv_len  = sizeof(zero_iv);
    }

    secret.data = key->u.s.secret;
    secret.size = key->u.s.secret_len;
    vector.data = iv;
    vector.size = iv_len;

    if ((ret = pgnutls_cipher_init( &key->u.s.handle, cipher, &secret, &vector )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    int ret;

    if ((ret = pgnutls_cipher_add_auth( key->u.s.handle, auth_data, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    int ret;

    if ((ret = pgnutls_cipher_encrypt2( key->u.s.handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    int ret;

    if ((ret = pgnutls_cipher_decrypt2( key->u.s.handle, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    int ret;

    if ((ret = pgnutls_cipher_tag( key->u.s.handle, tag, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

struct buffer
{
    BYTE *buffer;
    DWORD length;
    DWORD pos;
    BOOL error;
};

static void buffer_init( struct buffer *buffer )
{
    buffer->buffer = NULL;
    buffer->length = 0;
    buffer->pos = 0;
    buffer->error = FALSE;
}

static void buffer_free( struct buffer *buffer )
{
    HeapFree( GetProcessHeap(), 0, buffer->buffer );
}

static void buffer_append( struct buffer *buffer, BYTE *data, DWORD len )
{
    if (!len) return;

    if (buffer->pos + len > buffer->length)
    {
        DWORD new_length = max( max( buffer->pos + len, buffer->length * 2 ), 64 );
        BYTE *new_buffer;

        if (buffer->buffer)
            new_buffer = HeapReAlloc( GetProcessHeap(), 0, buffer->buffer, new_length );
        else
            new_buffer = HeapAlloc( GetProcessHeap(), 0, new_length );

        if (!new_buffer)
        {
            ERR( "out of memory\n" );
            buffer->error = TRUE;
            return;
        }

        buffer->buffer = new_buffer;
        buffer->length = new_length;
    }

    memcpy( &buffer->buffer[buffer->pos], data, len );
    buffer->pos += len;
}

static void buffer_append_byte( struct buffer *buffer, BYTE value )
{
    buffer_append( buffer, &value, sizeof(value) );
}

static void buffer_append_asn1_length( struct buffer *buffer, DWORD length )
{
    DWORD num_bytes;

    if (length < 128)
    {
        buffer_append_byte( buffer, length );
        return;
    }

    if (length <= 0xff) num_bytes = 1;
    else if (length <= 0xffff) num_bytes = 2;
    else if (length <= 0xffffff) num_bytes = 3;
    else num_bytes = 4;

    buffer_append_byte( buffer, 0x80 | num_bytes );
    while (num_bytes--)
        buffer_append_byte( buffer, length >> (num_bytes * 8) );
}

static void buffer_append_asn1_integer( struct buffer *buffer, BYTE *data, DWORD len )
{
    DWORD leading_zero = (*data & 0x80) != 0;

    buffer_append_byte( buffer, 0x02 );  /* tag */
    buffer_append_asn1_length( buffer, len + leading_zero );
    if (leading_zero) buffer_append_byte( buffer, 0 );
    buffer_append( buffer, data, len );
}

static void buffer_append_asn1_sequence( struct buffer *buffer, struct buffer *content )
{
    if (content->error)
    {
        buffer->error = TRUE;
        return;
    }

    buffer_append_byte( buffer, 0x30 );  /* tag */
    buffer_append_asn1_length( buffer, content->pos );
    buffer_append( buffer, content->buffer, content->pos );
}

static void buffer_append_asn1_r_s( struct buffer *buffer, BYTE *r, DWORD r_len, BYTE *s, DWORD s_len )
{
    struct buffer value;

    buffer_init( &value );
    buffer_append_asn1_integer( &value, r, r_len );
    buffer_append_asn1_integer( &value, s, s_len );
    buffer_append_asn1_sequence( buffer, &value );
    buffer_free( &value );
}

static NTSTATUS import_gnutls_pubkey_ecc( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y;
    int ret;

    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256: curve = GNUTLS_ECC_CURVE_SECP256R1; break;
        case ALG_ID_ECDSA_P384: curve = GNUTLS_ECC_CURVE_SECP384R1; break;

        default:
            FIXME( "Algorithm %d not yet supported\n", key->alg_id );
            return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)key->u.a.pubkey;
    x.data = key->u.a.pubkey + sizeof(*ecc_blob);
    x.size = ecc_blob->cbKey;
    y.data = key->u.a.pubkey + sizeof(*ecc_blob) + ecc_blob->cbKey;
    y.size = ecc_blob->cbKey;

    if ((ret = pgnutls_pubkey_import_ecc_raw( *gnutls_key, curve, &x, &y )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_rsa( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    gnutls_datum_t m, e;
    int ret;

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    rsa_blob = (BCRYPT_RSAKEY_BLOB *)key->u.a.pubkey;
    e.data = key->u.a.pubkey + sizeof(*rsa_blob);
    e.size = rsa_blob->cbPublicExp;
    m.data = key->u.a.pubkey + sizeof(*rsa_blob) + rsa_blob->cbPublicExp;
    m.size = rsa_blob->cbModulus;

    if ((ret = pgnutls_pubkey_import_rsa_raw( *gnutls_key, &m, &e )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey( struct key *key,  gnutls_pubkey_t *gnutls_key)
{
    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            return import_gnutls_pubkey_ecc( key, gnutls_key );
        case ALG_ID_RSA:
            return import_gnutls_pubkey_rsa( key, gnutls_key );

        default:
            FIXME("Algorithm %d not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS prepare_gnutls_signature_ecc( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    struct buffer buffer;
    DWORD r_len = signature_len / 2;
    DWORD s_len = r_len;
    BYTE *r = signature;
    BYTE *s = signature + r_len;

    buffer_init( &buffer );
    buffer_append_asn1_r_s( &buffer, r, r_len, s, s_len );
    if (buffer.error)
    {
        buffer_free( &buffer );
        return STATUS_NO_MEMORY;
    }

    gnutls_signature->data = buffer.buffer;
    gnutls_signature->size = buffer.pos;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature_rsa( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    gnutls_signature->data = signature;
    gnutls_signature->size = signature_len;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature( struct key *key, UCHAR *signature, ULONG signature_len,
                                          gnutls_datum_t *gnutls_signature )
{
    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            return prepare_gnutls_signature_ecc( key, signature, signature_len, gnutls_signature );
        case ALG_ID_RSA:
            return prepare_gnutls_signature_rsa( key, signature, signature_len, gnutls_signature );

        default:
            FIXME( "Algorithm %d not yet supported\n", key->alg_id );
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                       UCHAR *signature, ULONG signature_len, DWORD flags )
{
    gnutls_digest_algorithm_t hash_algo;
    gnutls_sign_algorithm_t sign_algo;
    gnutls_datum_t gnutls_hash, gnutls_signature;
    gnutls_pk_algorithm_t pk_algo;
    gnutls_pubkey_t gnutls_key;
    NTSTATUS status;
    int ret;

    if (key->alg_id == ALG_ID_RSA)
    {
        BCRYPT_PKCS1_PADDING_INFO *pinfo = (BCRYPT_PKCS1_PADDING_INFO *)padding;

        if (!(flags & BCRYPT_PAD_PKCS1) || !pinfo) return STATUS_INVALID_PARAMETER;
        if (!pinfo->pszAlgId) return STATUS_INVALID_SIGNATURE;

        if (!strcmpW( pinfo->pszAlgId, BCRYPT_SHA1_ALGORITHM )) hash_algo = GNUTLS_DIG_SHA1;
        else if (!strcmpW( pinfo->pszAlgId, BCRYPT_SHA256_ALGORITHM )) hash_algo = GNUTLS_DIG_SHA256;
        else if (!strcmpW( pinfo->pszAlgId, BCRYPT_SHA384_ALGORITHM )) hash_algo = GNUTLS_DIG_SHA384;
        else if (!strcmpW( pinfo->pszAlgId, BCRYPT_SHA512_ALGORITHM )) hash_algo = GNUTLS_DIG_SHA512;
        else
        {
            FIXME( "Hash algorithm %s not supported\n", debugstr_w(pinfo->pszAlgId) );
            return STATUS_NOT_SUPPORTED;
        }
    }
    else
    {
        if (flags)
            FIXME( "Flags %08x not supported\n", flags );

        /* only the hash size must match, not the actual hash function */
        switch (hash_len)
        {
            case 32: hash_algo = GNUTLS_DIG_SHA256; break;
            case 48: hash_algo = GNUTLS_DIG_SHA384; break;

            default:
                FIXME( "Hash size %u not yet supported\n", hash_len );
                return STATUS_INVALID_SIGNATURE;
        }
    }

    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            pk_algo = GNUTLS_PK_ECC;
            break;
        case ALG_ID_RSA:
            pk_algo = GNUTLS_PK_RSA;
            break;

        default:
            FIXME( "Algorithm %d not yet supported\n", key->alg_id );
            return STATUS_NOT_IMPLEMENTED;
    }

    if ((sign_algo = pgnutls_pk_to_sign( pk_algo, hash_algo )) == GNUTLS_SIGN_UNKNOWN)
    {
        FIXME("Gnutls does not support algorithm %d with hash len %u\n", key->alg_id, hash_len);
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = import_gnutls_pubkey( key, &gnutls_key )))
        return status;

    if ((status = prepare_gnutls_signature( key, signature, signature_len, &gnutls_signature )))
    {
        pgnutls_pubkey_deinit( gnutls_key );
        return status;
    }

    gnutls_hash.data = hash;
    gnutls_hash.size = hash_len;
    ret = pgnutls_pubkey_verify_hash2( gnutls_key, sign_algo, 0, &gnutls_hash, &gnutls_signature );

    if (gnutls_signature.data != signature)
        HeapFree( GetProcessHeap(), 0, gnutls_signature.data );
    pgnutls_pubkey_deinit( gnutls_key );
    return (ret < 0) ? STATUS_INVALID_SIGNATURE : STATUS_SUCCESS;
}

static NTSTATUS key_destroy( struct key *key )
{
    if(key_is_symmetric(key))
    {
        if (key->u.s.handle) pgnutls_cipher_deinit( key->u.s.handle );
        heap_free( key->u.s.secret );
    }
    else
        heap_free( key->u.a.pubkey );
    heap_free( key );
    return STATUS_SUCCESS;
}
#elif defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
static NTSTATUS key_symmetric_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    UCHAR *buffer;

    switch (alg->id)
    {
    case ALG_ID_AES:
        switch (alg->mode)
        {
            case MODE_ID_CBC:
                break;
            default:
                FIXME( "mode %u not supported\n", alg->mode );
                return STATUS_NOT_SUPPORTED;
        }
        break;

    default:
        FIXME( "algorithm %u not supported\n", alg->id );
        return STATUS_NOT_SUPPORTED;
    }

    if (!(key->u.s.block_size = get_block_size( alg ))) return STATUS_INVALID_PARAMETER;
    if (!(buffer = heap_alloc( secret_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, secret, secret_len );

    key->alg_id          = alg->id;
    key->u.s.mode        = alg->mode;
    key->u.s.ref_encrypt = NULL;        /* initialized on first use */
    key->u.s.ref_decrypt = NULL;
    key->u.s.secret      = buffer;
    key->u.s.secret_len  = secret_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_asymmetric_init( struct key *key, struct algorithm *alg, const UCHAR *pubkey, ULONG pubkey_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_duplicate( struct key *key_orig, struct key *key_copy )
{
    UCHAR *buffer;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, key_orig->u.s.secret_len ))) return STATUS_NO_MEMORY;
    memcpy( buffer, key_orig->u.s.secret, key_orig->u.s.secret_len );

    key_copy->hdr               = key_orig->hdr;
    key_copy->alg_id            = key_orig->alg_id;
    key_copy->u.s.mode          = key_orig->u.s.mode;
    key_copy->u.s.block_size    = key_orig->u.s.block_size;
    key_copy->u.s.ref_encrypt   = NULL;
    key_copy->u.s.ref_decrypt   = NULL;
    key_copy->u.s.secret        = buffer;
    key_copy->u.s.secret_len    = key_orig->u.s.secret_len;

    return STATUS_SUCCESS;
}

static NTSTATUS set_key_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_symmetric_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    CCCryptorStatus status;

    if (key->u.s.ref_encrypt)
    {
        CCCryptorRelease( key->u.s.ref_encrypt );
        key->u.s.ref_encrypt = NULL;
    }
    if (key->u.s.ref_decrypt)
    {
        CCCryptorRelease( key->u.s.ref_decrypt );
        key->u.s.ref_decrypt = NULL;
    }

    if ((status = CCCryptorCreateWithMode( kCCEncrypt, kCCModeCBC, kCCAlgorithmAES128, ccNoPadding, iv,
                                           key->u.s.secret, key->u.s.secret_len, NULL, 0, 0, 0, &key->u.s.ref_encrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    if ((status = CCCryptorCreateWithMode( kCCDecrypt, kCCModeCBC, kCCAlgorithmAES128, ccNoPadding, iv,
                                           key->u.s.secret, key->u.s.secret_len, NULL, 0, 0, 0, &key->u.s.ref_decrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        CCCryptorRelease( key->u.s.ref_encrypt );
        key->u.s.ref_encrypt = NULL;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    CCCryptorStatus status;

    if ((status = CCCryptorUpdate( key->u.s.ref_encrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    CCCryptorStatus status;

    if ((status = CCCryptorUpdate( key->u.s.ref_decrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                       UCHAR *signature, ULONG signature_len, DWORD flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_destroy( struct key *key )
{
    if (key->u.s.ref_encrypt) CCCryptorRelease( key->u.s.ref_encrypt );
    if (key->u.s.ref_decrypt) CCCryptorRelease( key->u.s.ref_decrypt );
    heap_free( key->u.s.secret );
    heap_free( key );
    return STATUS_SUCCESS;
}
#else
static NTSTATUS key_symmetric_init( struct key *key, struct algorithm *alg, const UCHAR *secret, ULONG secret_len )
{
    ERR( "support for keys not available at build time\n" );
    key->u.s.mode = MODE_ID_CBC;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_asymmetric_init( struct key *key, struct algorithm *alg, const UCHAR *pubkey, ULONG pubkey_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_duplicate( struct key *key_orig, struct key *key_copy )
{
    ERR( "support for keys not available at build time\n" );
    key_copy->u.s.mode = MODE_ID_CBC;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS set_key_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_symmetric_set_params( struct key *key, UCHAR *iv, ULONG iv_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len  )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output,
                             ULONG output_len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                       UCHAR *signature, ULONG signature_len, DWORD flags )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_destroy( struct key *key )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS key_export( struct key *key, const WCHAR *type, UCHAR *output, ULONG output_len, ULONG *size )
{
    ERR( "support for keys not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}
#endif

NTSTATUS WINAPI BCryptGenerateSymmetricKey( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE *handle,
                                            UCHAR *object, ULONG object_len, UCHAR *secret, ULONG secret_len,
                                            ULONG flags )
{
    struct algorithm *alg = algorithm;
    struct key *key;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n", algorithm, handle, object, object_len, secret, secret_len, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );


    if (!(key = heap_alloc( sizeof(*key) )))
    {
        *handle = NULL;
        return STATUS_NO_MEMORY;
    }

    key->hdr.magic = MAGIC_KEY;

    if ((status = key_symmetric_init( key, alg, secret, secret_len )))
    {
        heap_free( key );
        *handle = NULL;
        return status;
    }

    *handle = key;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptImportKey(BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE decrypt_key, LPCWSTR type,
                                BCRYPT_KEY_HANDLE *key, PUCHAR object, ULONG object_len, PUCHAR input,
                                ULONG input_len, ULONG flags)
{
    struct algorithm *alg = algorithm;

    TRACE("%p, %p, %s, %p, %p, %u, %p, %u, %u\n", algorithm, decrypt_key, debugstr_w(type), key, object,
          object_len, input, input_len, flags);

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!key || !type || !input) return STATUS_INVALID_PARAMETER;

    if (decrypt_key)
    {
        FIXME("Decrypting of key not yet supported\n");
        return STATUS_NO_MEMORY;
    }

    if (!strcmpW(type, BCRYPT_KEY_DATA_BLOB))
    {
        BCRYPT_KEY_DATA_BLOB_HEADER *key_header = (BCRYPT_KEY_DATA_BLOB_HEADER*)input;

        if (input_len < sizeof(BCRYPT_KEY_DATA_BLOB_HEADER))
            return STATUS_BUFFER_TOO_SMALL;

        if (key_header->dwMagic != BCRYPT_KEY_DATA_BLOB_MAGIC)
            return STATUS_INVALID_PARAMETER;

        if (key_header->dwVersion != BCRYPT_KEY_DATA_BLOB_VERSION1)
        {
            FIXME("Unknown key data blob version: %d\n", key_header->dwVersion);
            return STATUS_INVALID_PARAMETER;
        }

        if (key_header->cbKeyData + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) > input_len)
            return STATUS_INVALID_PARAMETER;

        return BCryptGenerateSymmetricKey(algorithm, key, object, object_len, (UCHAR*)&key_header[1], key_header->cbKeyData, 0);
    }

    FIXME("Unsupported key type: %s\n", debugstr_w(type));
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS WINAPI BCryptExportKey(BCRYPT_KEY_HANDLE export_key, BCRYPT_KEY_HANDLE encrypt_key, LPCWSTR type,
                                PUCHAR output, ULONG output_len, ULONG *size, ULONG flags)
{
    struct key *key = export_key;

    TRACE("%p, %p, %s, %p, %u, %p, %u\n", key, encrypt_key, debugstr_w(type), output, output_len, size, flags);

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (!output || !output_len || !size) return STATUS_INVALID_PARAMETER;

    if (encrypt_key)
    {
        FIXME("Encryption of key not yet supported\n");
        return STATUS_NO_MEMORY;
    }

    return key_export( key, type, output, output_len, size );
}

NTSTATUS WINAPI BCryptDuplicateKey( BCRYPT_KEY_HANDLE handle, BCRYPT_KEY_HANDLE *handle_copy,
                                    UCHAR *object, ULONG object_len, ULONG flags )
{
    struct key *key_orig = handle;
    struct key *key_copy;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %u, %08x\n", handle, handle_copy, object, object_len, flags );

    if (!key_orig || key_orig->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;
    if (!(key_copy = HeapAlloc( GetProcessHeap(), 0, sizeof(*key_copy) )))
    {
        *handle_copy = NULL;
        return STATUS_NO_MEMORY;
    }

    if ((status = key_duplicate( key_orig, key_copy )))
    {
        HeapFree( GetProcessHeap(), 0, key_copy );
        *handle_copy = NULL;
        return status;
    }

    *handle_copy = key_copy;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptImportKeyPair( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE decrypt_key, const WCHAR *type,
                                     BCRYPT_KEY_HANDLE *ret_key, UCHAR *input, ULONG input_len, ULONG flags )
{
    struct algorithm *alg = algorithm;
    NTSTATUS status;
    struct key *key;

    TRACE( "%p, %p, %s, %p, %p, %u, %u\n", algorithm, decrypt_key, debugstr_w(type), ret_key, input, input_len, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!ret_key || !type || !input) return STATUS_INVALID_PARAMETER;

    *ret_key = NULL;

    if (decrypt_key)
    {
        FIXME( "decrypting of key not yet supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( type, BCRYPT_ECCPUBLIC_BLOB ))
    {
        BCRYPT_ECCKEY_BLOB *ecc_blob = (BCRYPT_ECCKEY_BLOB *)input;
        DWORD key_size, magic;

        if (input_len < sizeof(*ecc_blob))
            return STATUS_INVALID_PARAMETER;

        switch (alg->id)
        {
            case ALG_ID_ECDSA_P256:
                key_size = 32;
                magic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
                break;
            case ALG_ID_ECDSA_P384:
                key_size = 48;
                magic = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
                break;

            default:
                FIXME("Algorithm %d does not yet support importing blob of type: %s\n", alg->id, debugstr_w(type));
                return STATUS_NOT_SUPPORTED;
        }

        if (ecc_blob->dwMagic != magic)
            return STATUS_NOT_SUPPORTED;

        if (ecc_blob->cbKey != key_size)
            return STATUS_INVALID_PARAMETER;

        if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) )))
            return STATUS_NO_MEMORY;

        key->hdr.magic = MAGIC_KEY;
        if ((status = key_asymmetric_init( key, alg, (BYTE *)ecc_blob, sizeof(*ecc_blob) + ecc_blob->cbKey * 2 )))
        {
            HeapFree( GetProcessHeap(), 0, key );
            return status;
        }

        *ret_key = key;
        return STATUS_SUCCESS;
    }
    else if (!strcmpW( type, BCRYPT_RSAPUBLIC_BLOB ))
    {
        BCRYPT_RSAKEY_BLOB *rsa_blob = (BCRYPT_RSAKEY_BLOB *)input;

        if (input_len < sizeof(*rsa_blob))
            return STATUS_INVALID_PARAMETER;

        if (alg->id != ALG_ID_RSA)
            return STATUS_NOT_SUPPORTED;

        if (rsa_blob->Magic != BCRYPT_RSAPUBLIC_MAGIC)
            return STATUS_NOT_SUPPORTED;

        if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) )))
            return STATUS_NO_MEMORY;

        key->hdr.magic = MAGIC_KEY;
        if ((status = key_asymmetric_init( key, alg, (BYTE *)rsa_blob,
                sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus )))
        {
            HeapFree( GetProcessHeap(), 0, key );
            return status;
        }

        *ret_key = key;
        return STATUS_SUCCESS;
    }

    FIXME( "unsupported key type %s\n", debugstr_w(type) );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS WINAPI BCryptVerifySignature( BCRYPT_KEY_HANDLE handle, void *padding, UCHAR *hash, ULONG hash_len,
                                       UCHAR *signature, ULONG signature_len, ULONG flags )
{
    struct key *key = handle;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n", handle, padding, hash,
           hash_len, signature, signature_len, flags );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (!hash || !hash_len || !signature || !signature_len) return STATUS_INVALID_PARAMETER;
    if (!key_is_asymmetric(key)) return STATUS_NOT_SUPPORTED;

    return key_asymmetric_verify( key, padding, hash, hash_len, signature, signature_len, flags );
}

NTSTATUS WINAPI BCryptDestroyKey( BCRYPT_KEY_HANDLE handle )
{
    struct key *key = handle;

    TRACE( "%p\n", handle );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    return key_destroy( key );
}

NTSTATUS WINAPI BCryptEncrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len,
                               void *padding, UCHAR *iv, ULONG iv_len, UCHAR *output,
                               ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = handle;
    ULONG bytes_left = input_len;
    UCHAR *buf, *src, *dst;
    enum mode_id mode;
    ULONG block_size;
    NTSTATUS status;

    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n", handle, input, input_len,
           padding, iv, iv_len, output, output_len, ret_len, flags );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;

    if (!key_is_symmetric(key))
    {
        FIXME( "encryption with asymmetric keys not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (flags & ~BCRYPT_BLOCK_PADDING)
    {
        FIXME( "flags %08x not implemented\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = key_symmetric_get_mode( key, &mode ))) return status;

    if (mode == MODE_ID_GCM)
    {
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth_info = padding;

        if (!auth_info) return STATUS_INVALID_PARAMETER;
        if (!auth_info->pbNonce) return STATUS_INVALID_PARAMETER;
        if (!auth_info->pbTag) return STATUS_INVALID_PARAMETER;
        if (auth_info->cbTag < 12 || auth_info->cbTag > 16) return STATUS_INVALID_PARAMETER;
        if (auth_info->dwFlags & BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG)
            FIXME( "call chaining not implemented\n" );

        if ((status = key_symmetric_set_params( key, auth_info->pbNonce, auth_info->cbNonce )))
            return status;

        *ret_len = input_len;
        if (flags & BCRYPT_BLOCK_PADDING) return STATUS_INVALID_PARAMETER;
        if (input && !output) return STATUS_SUCCESS;
        if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

        if (auth_info->pbAuthData && (status = key_symmetric_set_auth_data( key, auth_info->pbAuthData, auth_info->cbAuthData )))
            return status;
        if ((status = key_symmetric_encrypt( key, input, input_len, output, output_len )))
            return status;

        return key_get_tag( key, auth_info->pbTag, auth_info->cbTag );
    }

    if ((status = key_symmetric_set_params( key, iv, iv_len ))) return status;
    if ((status = key_symmetric_get_blocksize( key, &block_size ))) return status;

    *ret_len = input_len;

    if (flags & BCRYPT_BLOCK_PADDING)
        *ret_len = (input_len + block_size) & ~(block_size - 1);
    else if (input_len & (block_size - 1))
        return STATUS_INVALID_BUFFER_SIZE;

    if (!output) return STATUS_SUCCESS;
    if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

    if (mode == MODE_ID_ECB && iv)
        return STATUS_INVALID_PARAMETER;

    src = input;
    dst = output;
    while (bytes_left >= block_size)
    {
        if ((status = key_symmetric_encrypt( key, src, block_size, dst, block_size ))) return status;
        if (mode == MODE_ID_ECB && (status = key_symmetric_set_params( key, iv, iv_len ))) return status;
        bytes_left -= block_size;
        src += block_size;
        dst += block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (!(buf = heap_alloc( block_size ))) return STATUS_NO_MEMORY;
        memcpy( buf, src, bytes_left );
        memset( buf + bytes_left, block_size - bytes_left, block_size - bytes_left );
        status = key_symmetric_encrypt( key, buf, block_size, dst, block_size );
        heap_free( buf );
    }

    return status;
}

NTSTATUS WINAPI BCryptDecrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG input_len,
                               void *padding, UCHAR *iv, ULONG iv_len, UCHAR *output,
                               ULONG output_len, ULONG *ret_len, ULONG flags )
{
    struct key *key = handle;
    ULONG bytes_left = input_len;
    UCHAR *buf, *src, *dst;
    NTSTATUS status;

    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n", handle, input, input_len,
           padding, iv, iv_len, output, output_len, ret_len, flags );

    if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
    if (flags & ~BCRYPT_BLOCK_PADDING)
    {
        FIXME( "flags %08x not supported\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (key->u.s.mode == MODE_ID_GCM)
    {
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth_info = padding;
        UCHAR tag[16];

        if (!auth_info) return STATUS_INVALID_PARAMETER;
        if (!auth_info->pbNonce) return STATUS_INVALID_PARAMETER;
        if (!auth_info->pbTag) return STATUS_INVALID_PARAMETER;
        if (auth_info->cbTag < 12 || auth_info->cbTag > 16) return STATUS_INVALID_PARAMETER;

        if ((status = key_symmetric_set_params( key, auth_info->pbNonce, auth_info->cbNonce )))
            return status;

        *ret_len = input_len;
        if (flags & BCRYPT_BLOCK_PADDING) return STATUS_INVALID_PARAMETER;
        if (!output) return STATUS_SUCCESS;
        if (output_len < *ret_len) return STATUS_BUFFER_TOO_SMALL;

        if (auth_info->pbAuthData && (status = key_symmetric_set_auth_data( key, auth_info->pbAuthData, auth_info->cbAuthData )))
            return status;
        if ((status = key_decrypt( key, input, input_len, output, output_len )))
            return status;

        if ((status = key_get_tag( key, tag, sizeof(tag) )))
            return status;
        if (memcmp( tag, auth_info->pbTag, auth_info->cbTag ))
            return STATUS_AUTH_TAG_MISMATCH;

        return STATUS_SUCCESS;
    }

    if ((status = key_symmetric_set_params( key, iv, iv_len ))) return status;

    *ret_len = input_len;

    if (input_len & (key->u.s.block_size - 1)) return STATUS_INVALID_BUFFER_SIZE;
    if (!output) return STATUS_SUCCESS;
    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (output_len + key->u.s.block_size < *ret_len) return STATUS_BUFFER_TOO_SMALL;
        if (input_len < key->u.s.block_size) return STATUS_BUFFER_TOO_SMALL;
        bytes_left -= key->u.s.block_size;
    }
    else if (output_len < *ret_len)
        return STATUS_BUFFER_TOO_SMALL;

    if (key->u.s.mode == MODE_ID_ECB && iv)
        return STATUS_INVALID_PARAMETER;

    src = input;
    dst = output;
    while (bytes_left >= key->u.s.block_size)
    {
        if ((status = key_decrypt( key, src, key->u.s.block_size, dst, key->u.s.block_size ))) return status;
        if (key->u.s.mode == MODE_ID_ECB && (status = key_symmetric_set_params( key, iv, iv_len ))) return status;
        bytes_left -= key->u.s.block_size;
        src += key->u.s.block_size;
        dst += key->u.s.block_size;
    }

    if (flags & BCRYPT_BLOCK_PADDING)
    {
        if (!(buf = heap_alloc( key->u.s.block_size ))) return STATUS_NO_MEMORY;
        status = key_decrypt( key, src, key->u.s.block_size, buf, key->u.s.block_size );
        if (!status && buf[ key->u.s.block_size - 1 ] <= key->u.s.block_size)
        {
            *ret_len -= buf[ key->u.s.block_size - 1 ];
            if (output_len < *ret_len) status = STATUS_BUFFER_TOO_SMALL;
            else memcpy( dst, buf, key->u.s.block_size - buf[ key->u.s.block_size - 1 ] );
        }
        else
            status = STATUS_UNSUCCESSFUL; /* FIXME: invalid padding */
        heap_free( buf );
    }

    return status;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
        gnutls_initialize();
#endif
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
#if defined(HAVE_GNUTLS_CIPHER_INIT) && !defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H)
        gnutls_uninitialize();
#endif
        break;
    }
    return TRUE;
}
