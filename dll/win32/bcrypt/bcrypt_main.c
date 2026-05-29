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

#include <wine/config.h>
#include <wine/port.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ntsecapi.h>
#include <bcrypt.h>

#include <wine/debug.h>
#include <wine/unicode.h>
#include <wine/library.h>

#ifdef SONAME_LIBMBEDTLS
#include <mbedtls/md.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/aes.h>
#include <mbedtls/des.h>
#include <mbedtls/gcm.h>
#include <mbedtls/ccm.h>
#include <mbedtls/rsa.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static HINSTANCE instance;

#if defined(HAVE_GNUTLS_HASH) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H)
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_hash);
MAKE_FUNCPTR(gnutls_hash_deinit);
MAKE_FUNCPTR(gnutls_hash_init);
MAKE_FUNCPTR(gnutls_hmac);
MAKE_FUNCPTR(gnutls_hmac_deinit);
MAKE_FUNCPTR(gnutls_hmac_init);
MAKE_FUNCPTR(gnutls_perror);
#undef MAKE_FUNCPTR

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

static BOOL gnutls_initialize(void)
{
    int ret;

    if (!(libgnutls_handle = wine_dlopen( SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for crypto hashes\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libgnutls_handle, #f, NULL, 0 ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_hash);
    LOAD_FUNCPTR(gnutls_hash_deinit);
    LOAD_FUNCPTR(gnutls_hash_init);
    LOAD_FUNCPTR(gnutls_hmac);
    LOAD_FUNCPTR(gnutls_hmac_deinit);
    LOAD_FUNCPTR(gnutls_hmac_init);
    LOAD_FUNCPTR(gnutls_perror)
#undef LOAD_FUNCPTR

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
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
#elif defined(SONAME_LIBMBEDTLS) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H) && !defined(__REACTOS__)
WINE_DECLARE_DEBUG_CHANNEL(winediag);

void *libmbedtls_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(mbedtls_md_init);
MAKE_FUNCPTR(mbedtls_md_setup);
MAKE_FUNCPTR(mbedtls_md_update);
MAKE_FUNCPTR(mbedtls_md_hmac_starts);
MAKE_FUNCPTR(mbedtls_md_hmac_finish);
MAKE_FUNCPTR(mbedtls_md_hmac_reset);
MAKE_FUNCPTR(mbedtls_md_free);
MAKE_FUNCPTR(mbedtls_md5_init);
MAKE_FUNCPTR(mbedtls_md5_starts);
MAKE_FUNCPTR(mbedtls_md5_update);
MAKE_FUNCPTR(mbedtls_md5_finish);
MAKE_FUNCPTR(mbedtls_md5_free);
MAKE_FUNCPTR(mbedtls_sha1_init);
MAKE_FUNCPTR(mbedtls_sha1_starts);
MAKE_FUNCPTR(mbedtls_sha1_update);
MAKE_FUNCPTR(mbedtls_sha1_finish);
MAKE_FUNCPTR(mbedtls_sha1_free);
MAKE_FUNCPTR(mbedtls_sha256_init);
MAKE_FUNCPTR(mbedtls_sha256_starts);
MAKE_FUNCPTR(mbedtls_sha256_update);
MAKE_FUNCPTR(mbedtls_sha256_finish);
MAKE_FUNCPTR(mbedtls_sha256_free);
MAKE_FUNCPTR(mbedtls_sha512_init);
MAKE_FUNCPTR(mbedtls_sha512_starts);
MAKE_FUNCPTR(mbedtls_sha512_update);
MAKE_FUNCPTR(mbedtls_sha512_finish);
MAKE_FUNCPTR(mbedtls_sha512_free);
#undef MAKE_FUNCPTR

#define mbedtls_md_init             pmbedtls_md_init
#define mbedtls_md_setup            pmbedtls_md_setup
#define mbedtls_md_update           pmbedtls_md_update
#define mbedtls_md_hmac_starts      pmbedtls_md_hmac_starts
#define mbedtls_md_hmac_finish      pmbedtls_md_hmac_finish
#define mbedtls_md_hmac_reset       pmbedtls_md_hmac_reset
#define mbedtls_md_free             pmbedtls_md_free
#define mbedtls_md5_init            pmbedtls_md5_init
#define mbedtls_md5_starts          pmbedtls_md5_starts
#define mbedtls_md5_update          pmbedtls_md5_update
#define mbedtls_md5_finish          pmbedtls_md5_finish
#define mbedtls_md5_free            pmbedtls_md5_free
#define mbedtls_sha1_init           pmbedtls_sha1_init
#define mbedtls_sha1_starts         pmbedtls_sha1_starts
#define mbedtls_sha1_update         pmbedtls_sha1_update
#define mbedtls_sha1_finish         pmbedtls_sha1_finish
#define mbedtls_sha1_free           pmbedtls_sha1_free
#define mbedtls_sha256_init         pmbedtls_sha256_init
#define mbedtls_sha256_starts       pmbedtls_sha256_starts
#define mbedtls_sha256_update       pmbedtls_sha256_update
#define mbedtls_sha256_finish       pmbedtls_sha256_finish
#define mbedtls_sha256_free         pmbedtls_sha256_free
#define mbedtls_sha512_init         pmbedtls_sha512_init
#define mbedtls_sha512_starts       pmbedtls_sha512_starts
#define mbedtls_sha512_update       pmbedtls_sha512_update
#define mbedtls_sha512_finish       pmbedtls_sha512_finish
#define mbedtls_sha512_free         pmbedtls_sha512_free

static BOOL mbedtls_initialize(void)
{
    if (!(libmbedtls_handle = wine_dlopen( SONAME_LIBMBEDTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libmbedtls, no support for crypto hashes\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libmbedtls_handle, #f, NULL, 0 ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(mbedtls_md_init)
    LOAD_FUNCPTR(mbedtls_md_setup)
    LOAD_FUNCPTR(mbedtls_md_update)
    LOAD_FUNCPTR(mbedtls_md_hmac_starts)
    LOAD_FUNCPTR(mbedtls_md_hmac_finish)
    LOAD_FUNCPTR(mbedtls_md_hmac_reset)
    LOAD_FUNCPTR(mbedtls_md_free);
    LOAD_FUNCPTR(mbedtls_md5_init)
    LOAD_FUNCPTR(mbedtls_md5_starts)
    LOAD_FUNCPTR(mbedtls_md5_update)
    LOAD_FUNCPTR(mbedtls_md5_finish)
    LOAD_FUNCPTR(mbedtls_md5_free);
    LOAD_FUNCPTR(mbedtls_sha1_init)
    LOAD_FUNCPTR(mbedtls_sha1_starts)
    LOAD_FUNCPTR(mbedtls_sha1_update)
    LOAD_FUNCPTR(mbedtls_sha1_finish)
    LOAD_FUNCPTR(mbedtls_sha1_free);
    LOAD_FUNCPTR(mbedtls_sha256_init)
    LOAD_FUNCPTR(mbedtls_sha256_starts)
    LOAD_FUNCPTR(mbedtls_sha256_update)
    LOAD_FUNCPTR(mbedtls_sha256_finish)
    LOAD_FUNCPTR(mbedtls_sha256_free);
    LOAD_FUNCPTR(mbedtls_sha512_init)
    LOAD_FUNCPTR(mbedtls_sha512_starts)
    LOAD_FUNCPTR(mbedtls_sha512_update)
    LOAD_FUNCPTR(mbedtls_sha512_finish)
    LOAD_FUNCPTR(mbedtls_sha512_free);
#undef LOAD_FUNCPTR

    return TRUE;

fail:
    wine_dlclose( libmbedtls_handle, NULL, 0 );
    libmbedtls_handle = NULL;
    return FALSE;
}

static void mbedtls_uninitialize(void)
{
    wine_dlclose( libmbedtls_handle, NULL, 0 );
    libmbedtls_handle = NULL;
}
#endif /* SONAME_LIBMBEDTLS && !HAVE_COMMONCRYPTO_COMMONDIGEST_H && !__REACTOS__ */

NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG dwAlgOperations, ULONG *pAlgCount,
                                     BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags)
{
    FIXME("%08x, %p, %p, %08x - stub\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags);

    *ppAlgList=NULL;
    *pAlgCount=0;

    return STATUS_NOT_IMPLEMENTED;
}

#define MAGIC_ALG    (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH   (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
#define MAGIC_KEY    (('K' << 24) | ('E' << 16) | ('Y' << 8) | '0')
#define MAGIC_KPAIR  (('K' << 24) | ('P' << 16) | ('A' << 8) | 'R')
#define MAGIC_SECRET (('S' << 24) | ('E' << 16) | ('C' << 8) | 'T')
struct object
{
    ULONG magic;
};

enum alg_id
{
    ALG_ID_MD5,
    ALG_ID_RNG,
    ALG_ID_SHA1,
    ALG_ID_SHA224,
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512,
    ALG_ID_ECDSA_P256,
    ALG_ID_ECDSA_P384,
    ALG_ID_AES,
    ALG_ID_RSA,
    ALG_ID_ECDH_P256,
    ALG_ID_ECDH_P384,
    ALG_ID_3DES,
};

static const struct {
    ULONG hash_length;
    const WCHAR *alg_name;
} alg_props[] = {
    /* ALG_ID_MD5        */ { 16, BCRYPT_MD5_ALGORITHM },
    /* ALG_ID_RNG        */ {  0, BCRYPT_RNG_ALGORITHM },
    /* ALG_ID_SHA1       */ { 20, BCRYPT_SHA1_ALGORITHM },
    /* ALG_ID_SHA224     */ { 28, BCRYPT_SHA224_ALGORITHM },
    /* ALG_ID_SHA256     */ { 32, BCRYPT_SHA256_ALGORITHM },
    /* ALG_ID_SHA384     */ { 48, BCRYPT_SHA384_ALGORITHM },
    /* ALG_ID_SHA512     */ { 64, BCRYPT_SHA512_ALGORITHM },
    /* ALG_ID_ECDSA_P256 */ {  0, BCRYPT_ECDSA_P256_ALGORITHM },
    /* ALG_ID_ECDSA_P384 */ {  0, BCRYPT_ECDSA_P384_ALGORITHM },
    /* ALG_ID_AES        */ {  0, BCRYPT_AES_ALGORITHM },
    /* ALG_ID_RSA        */ {  0, BCRYPT_RSA_ALGORITHM },
    /* ALG_ID_ECDH_P256  */ {  0, BCRYPT_ECDH_P256_ALGORITHM },
    /* ALG_ID_ECDH_P384  */ {  0, BCRYPT_ECDH_P384_ALGORITHM },
    /* ALG_ID_3DES       */ {  0, BCRYPT_3DES_ALGORITHM },
};

struct algorithm
{
    struct object hdr;
    enum alg_id   id;
    BOOL hmac;
};

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
    struct algorithm *alg;
    enum alg_id alg_id;

    const DWORD supported_flags = BCRYPT_ALG_HANDLE_HMAC_FLAG;

    TRACE( "%p, %s, %s, %08x\n", handle, wine_dbgstr_w(id), wine_dbgstr_w(implementation), flags );

    if (!handle || !id) return STATUS_INVALID_PARAMETER;
    if (flags & ~supported_flags)
    {
        FIXME( "unsupported flags %08x\n", flags & ~supported_flags);
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( id, BCRYPT_SHA1_ALGORITHM )) alg_id = ALG_ID_SHA1;
    else if (!strcmpW( id, BCRYPT_MD5_ALGORITHM )) alg_id = ALG_ID_MD5;
    else if (!strcmpW( id, BCRYPT_RNG_ALGORITHM )) alg_id = ALG_ID_RNG;
    else if (!strcmpW( id, BCRYPT_SHA224_ALGORITHM )) alg_id = ALG_ID_SHA224;
    else if (!strcmpW( id, BCRYPT_SHA256_ALGORITHM )) alg_id = ALG_ID_SHA256;
    else if (!strcmpW( id, BCRYPT_SHA384_ALGORITHM )) alg_id = ALG_ID_SHA384;
    else if (!strcmpW( id, BCRYPT_SHA512_ALGORITHM )) alg_id = ALG_ID_SHA512;
    else if (!strcmpW( id, BCRYPT_ECDSA_P256_ALGORITHM )) alg_id = ALG_ID_ECDSA_P256;
    else if (!strcmpW( id, BCRYPT_ECDSA_P384_ALGORITHM )) alg_id = ALG_ID_ECDSA_P384;
    else if (!strcmpW( id, BCRYPT_AES_ALGORITHM )) alg_id = ALG_ID_AES;
    else if (!strcmpW( id, BCRYPT_RSA_ALGORITHM )) alg_id = ALG_ID_RSA;
    else if (!strcmpW( id, BCRYPT_3DES_ALGORITHM )) alg_id = ALG_ID_3DES;
    else if (!strcmpW( id, BCRYPT_ECDH_P256_ALGORITHM )) alg_id = ALG_ID_ECDH_P256;
    else if (!strcmpW( id, BCRYPT_ECDH_P384_ALGORITHM )) alg_id = ALG_ID_ECDH_P384;
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

    if (!(alg = HeapAlloc( GetProcessHeap(), 0, sizeof(*alg) ))) return STATUS_NO_MEMORY;
    alg->hdr.magic = MAGIC_ALG;
    alg->id        = alg_id;
    alg->hmac      = flags & BCRYPT_ALG_HANDLE_HMAC_FLAG;

    *handle = alg;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptCloseAlgorithmProvider( BCRYPT_ALG_HANDLE handle, DWORD flags )
{
    struct algorithm *alg = handle;

    TRACE( "%p, %08x\n", handle, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    HeapFree( GetProcessHeap(), 0, alg );
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

#ifdef HAVE_COMMONCRYPTO_COMMONDIGEST_H
struct hash
{
    struct object hdr;
    enum alg_id   alg_id;
    BOOL hmac;
    union
    {
        CC_MD5_CTX    md5_ctx;
        CC_SHA1_CTX   sha1_ctx;
        CC_SHA256_CTX sha256_ctx;
        CC_SHA512_CTX sha512_ctx;
        CCHmacContext hmac_ctx;
    } u;
};

static NTSTATUS hash_init( struct hash *hash )
{
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        CC_MD5_Init( &hash->u.md5_ctx );
        break;

    case ALG_ID_SHA1:
        CC_SHA1_Init( &hash->u.sha1_ctx );
        break;

    case ALG_ID_SHA256:
        CC_SHA256_Init( &hash->u.sha256_ctx );
        break;

    case ALG_ID_SHA384:
        CC_SHA384_Init( &hash->u.sha512_ctx );
        break;

    case ALG_ID_SHA512:
        CC_SHA512_Init( &hash->u.sha512_ctx );
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size )
{
    CCHmacAlgorithm cc_algorithm;
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        cc_algorithm = kCCHmacAlgMD5;
        break;

    case ALG_ID_SHA1:
        cc_algorithm = kCCHmacAlgSHA1;
        break;

    case ALG_ID_SHA256:
        cc_algorithm = kCCHmacAlgSHA256;
        break;

    case ALG_ID_SHA384:
        cc_algorithm = kCCHmacAlgSHA384;
        break;

    case ALG_ID_SHA512:
        cc_algorithm = kCCHmacAlgSHA512;
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    CCHmacInit( &hash->u.hmac_ctx, cc_algorithm, key, key_size );
    return STATUS_SUCCESS;
}


static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        CC_MD5_Update( &hash->u.md5_ctx, input, size );
        break;

    case ALG_ID_SHA1:
        CC_SHA1_Update( &hash->u.sha1_ctx, input, size );
        break;

    case ALG_ID_SHA256:
        CC_SHA256_Update( &hash->u.sha256_ctx, input, size );
        break;

    case ALG_ID_SHA384:
        CC_SHA384_Update( &hash->u.sha512_ctx, input, size );
        break;

    case ALG_ID_SHA512:
        CC_SHA512_Update( &hash->u.sha512_ctx, input, size );
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size )
{
    CCHmacUpdate( &hash->u.hmac_ctx, input, size );
    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        CC_MD5_Final( output, &hash->u.md5_ctx );
        break;

    case ALG_ID_SHA1:
        CC_SHA1_Final( output, &hash->u.sha1_ctx );
        break;

    case ALG_ID_SHA256:
        CC_SHA256_Final( output, &hash->u.sha256_ctx );
        break;

    case ALG_ID_SHA384:
        CC_SHA384_Final( output, &hash->u.sha512_ctx );
        break;

    case ALG_ID_SHA512:
        CC_SHA512_Final( output, &hash->u.sha512_ctx );
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        break;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    CCHmacFinal( &hash->u.hmac_ctx, output );

    return STATUS_SUCCESS;
}
#elif defined(HAVE_GNUTLS_HASH)
struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    BOOL hmac;
    union
    {
        gnutls_hash_hd_t hash_handle;
        gnutls_hmac_hd_t hmac_handle;
    } u;
};

static NTSTATUS hash_init( struct hash *hash )
{
    gnutls_digest_algorithm_t alg;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        alg = GNUTLS_DIG_MD5;
        break;
    case ALG_ID_SHA1:
        alg = GNUTLS_DIG_SHA1;
        break;

    case ALG_ID_SHA256:
        alg = GNUTLS_DIG_SHA256;
        break;

    case ALG_ID_SHA384:
        alg = GNUTLS_DIG_SHA384;
        break;

    case ALG_ID_SHA512:
        alg = GNUTLS_DIG_SHA512;
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (pgnutls_hash_init( &hash->u.hash_handle, alg )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size )
{
    gnutls_mac_algorithm_t alg;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        alg = GNUTLS_MAC_MD5;
        break;
    case ALG_ID_SHA1:
        alg = GNUTLS_MAC_SHA1;
        break;

    case ALG_ID_SHA256:
        alg = GNUTLS_MAC_SHA256;
        break;

    case ALG_ID_SHA384:
        alg = GNUTLS_MAC_SHA384;
        break;

    case ALG_ID_SHA512:
        alg = GNUTLS_MAC_SHA512;
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (pgnutls_hmac_init( &hash->u.hmac_handle, alg, key, key_size )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
    if (pgnutls_hash( hash->u.hash_handle, input, size )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size )
{
    if (pgnutls_hmac( hash->u.hmac_handle, input, size )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    pgnutls_hash_deinit( hash->u.hash_handle, output );
    return STATUS_SUCCESS;
}

static NTSTATUS hmac_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    pgnutls_hmac_deinit( hash->u.hmac_handle, output );
    return STATUS_SUCCESS;
}
#elif defined(SONAME_LIBMBEDTLS)
struct hash
{
    struct object hdr;
    BOOL hmac;
    BOOL reusable;
    enum alg_id   alg_id;
    UCHAR *hmac_key;
    ULONG  hmac_key_len;
    union
    {
        mbedtls_md5_context    md5_ctx;
        mbedtls_sha1_context   sha1_ctx;
        mbedtls_sha256_context sha256_ctx;
        mbedtls_sha512_context sha512_ctx;
        mbedtls_md_context_t   hmac_ctx;
    } u;
};

static NTSTATUS hash_init( struct hash *hash )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        mbedtls_md5_init(&hash->u.md5_ctx);
        mbedtls_md5_starts(&hash->u.md5_ctx);
        break;

    case ALG_ID_SHA1:
        mbedtls_sha1_init(&hash->u.sha1_ctx);
        mbedtls_sha1_starts(&hash->u.sha1_ctx);
        break;

    case ALG_ID_SHA224:
        mbedtls_sha256_init(&hash->u.sha256_ctx);
        mbedtls_sha256_starts(&hash->u.sha256_ctx, TRUE);
        break;

    case ALG_ID_SHA256:
        mbedtls_sha256_init(&hash->u.sha256_ctx);
        mbedtls_sha256_starts(&hash->u.sha256_ctx, FALSE);
        break;

    case ALG_ID_SHA384:
    case ALG_ID_SHA512:
        mbedtls_sha512_init(&hash->u.sha512_ctx);
        mbedtls_sha512_starts(&hash->u.sha512_ctx, hash->alg_id==ALG_ID_SHA384);
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size )
{
    const mbedtls_md_info_t *md_info;
    mbedtls_md_type_t md_type;
    int ret;
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_init(&hash->u.hmac_ctx);
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        md_type = MBEDTLS_MD_MD5;
        break;

    case ALG_ID_SHA1:
        md_type = MBEDTLS_MD_SHA1;
        break;

    case ALG_ID_SHA224:
        md_type = MBEDTLS_MD_SHA224;
        break;

    case ALG_ID_SHA256:
        md_type = MBEDTLS_MD_SHA256;
        break;

    case ALG_ID_SHA384:
        md_type = MBEDTLS_MD_SHA384;
        break;

    case ALG_ID_SHA512:
        md_type = MBEDTLS_MD_SHA512;
        break;

    default:
        ERR("unhandled id %u\n", hash->alg_id);
        return STATUS_NOT_IMPLEMENTED;
    }
    if ((md_info = mbedtls_md_info_from_type(md_type)) == NULL)
    {
        mbedtls_md_free(&hash->u.hmac_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    if ((ret = mbedtls_md_setup(&hash->u.hmac_ctx, md_info, 1)) != 0)
    {
        mbedtls_md_free(&hash->u.hmac_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    mbedtls_md_hmac_starts(&hash->u.hmac_ctx, key, key_size);

    if (hash->reusable && key_size > 0)
    {
        hash->hmac_key = HeapAlloc(GetProcessHeap(), 0, key_size);
        if (!hash->hmac_key)
        {
            mbedtls_md_free(&hash->u.hmac_ctx);
            return STATUS_NO_MEMORY;
        }
        memcpy(hash->hmac_key, key, key_size);
        hash->hmac_key_len = key_size;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        mbedtls_md5_update(&hash->u.md5_ctx, input, size);
        break;

    case ALG_ID_SHA1:
        mbedtls_sha1_update(&hash->u.sha1_ctx, input, size);
        break;

    case ALG_ID_SHA224:
    case ALG_ID_SHA256:
        mbedtls_sha256_update(&hash->u.sha256_ctx, input, size);
        break;

    case ALG_ID_SHA384:
    case ALG_ID_SHA512:
        mbedtls_sha512_update(&hash->u.sha512_ctx, input, size);
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_update(&hash->u.hmac_ctx, input, size);

    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_MD5:
        mbedtls_md5_finish(&hash->u.md5_ctx, output);
        if (hash->reusable) mbedtls_md5_starts(&hash->u.md5_ctx);
        else                mbedtls_md5_free(&hash->u.md5_ctx);
        break;

    case ALG_ID_SHA1:
        mbedtls_sha1_finish(&hash->u.sha1_ctx, output);
        if (hash->reusable) mbedtls_sha1_starts(&hash->u.sha1_ctx);
        else                mbedtls_sha1_free(&hash->u.sha1_ctx);
        break;

    case ALG_ID_SHA224:
        mbedtls_sha256_finish(&hash->u.sha256_ctx, output);
        if (hash->reusable) mbedtls_sha256_starts(&hash->u.sha256_ctx, TRUE);
        else                mbedtls_sha256_free(&hash->u.sha256_ctx);
        break;

    case ALG_ID_SHA256:
        mbedtls_sha256_finish(&hash->u.sha256_ctx, output);
        if (hash->reusable) mbedtls_sha256_starts(&hash->u.sha256_ctx, FALSE);
        else                mbedtls_sha256_free(&hash->u.sha256_ctx);
        break;

    case ALG_ID_SHA384:
    case ALG_ID_SHA512:
        mbedtls_sha512_finish(&hash->u.sha512_ctx, output);
        if (hash->reusable) mbedtls_sha512_starts(&hash->u.sha512_ctx, hash->alg_id == ALG_ID_SHA384);
        else                mbedtls_sha512_free(&hash->u.sha512_ctx);
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS hmac_finish( struct hash *hash, UCHAR *output, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_hmac_finish(&hash->u.hmac_ctx, output);
    if (hash->reusable)
        mbedtls_md_hmac_reset(&hash->u.hmac_ctx);
    else
        mbedtls_md_free(&hash->u.hmac_ctx);

    return STATUS_SUCCESS;
}

static void hash_free_contexts( struct hash *hash )
{
    if (hash->hmac)
    {
        mbedtls_md_free(&hash->u.hmac_ctx);
    }
    else
    {
        switch (hash->alg_id)
        {
        case ALG_ID_MD5:    mbedtls_md5_free(&hash->u.md5_ctx);       break;
        case ALG_ID_SHA1:   mbedtls_sha1_free(&hash->u.sha1_ctx);     break;
        case ALG_ID_SHA224:
        case ALG_ID_SHA256: mbedtls_sha256_free(&hash->u.sha256_ctx); break;
        case ALG_ID_SHA384:
        case ALG_ID_SHA512: mbedtls_sha512_free(&hash->u.sha512_ctx); break;
        default: break;
        }
    }
    if (hash->hmac_key)
    {
        HeapFree(GetProcessHeap(), 0, hash->hmac_key);
        hash->hmac_key = NULL;
    }
}

/* ---- AES symmetric key support ---- */

#define AES_BLOCK_SIZE 16

struct key
{
    struct object hdr;
    enum alg_id   alg_id;
    ULONG         block_size;
    ULONG         key_len;              /* key length in bytes */
    WCHAR         chaining_mode[64];
    UCHAR         key_data[32];         /* raw key bytes, up to 256-bit */
    mbedtls_aes_context  aes_enc;
    mbedtls_aes_context  aes_dec;
    mbedtls_gcm_context  gcm;
    BOOL                 gcm_key_set;
    mbedtls_ccm_context  ccm;
    BOOL                 ccm_key_set;
    mbedtls_des3_context des3_enc;
    mbedtls_des3_context des3_dec;
};

/* RSA/ECDH asymmetric key pair */
struct key_pair
{
    struct object    hdr;
    enum alg_id      alg_id;
    ULONG            bitlen;
    BOOL             finalized;
    BOOL             has_private;
    union {
        mbedtls_rsa_context   rsa;
        mbedtls_ecdh_context  ecdh;
        mbedtls_ecdsa_context ecdsa;
    } u;
};

/* ECDH shared secret (result of BCryptSecretAgreement) */
struct secret
{
    struct object hdr;
    enum alg_id   alg_id;   /* ECDH algorithm that produced this secret */
    ULONG         secret_len;
    UCHAR         secret[66]; /* up to 66 bytes for P-521; 32 for P-256, 48 for P-384 */
};

/* Simple RNG shim for mbedTLS (uses Windows RtlGenRandom) */
static int rng_cb( void *p_rng, unsigned char *output, size_t len )
{
    (void)p_rng;
    return RtlGenRandom( output, (ULONG)len ) ? 0 : -1;
}

static NTSTATUS key_init( struct key *key, enum alg_id alg_id, const UCHAR *secret, ULONG secretlen )
{
    key->hdr.magic  = MAGIC_KEY;
    key->alg_id     = alg_id;
    key->key_len    = secretlen;
    strcpyW(key->chaining_mode, BCRYPT_CHAIN_MODE_CBC);

    if (alg_id == ALG_ID_AES)
    {
        if (secretlen != 16 && secretlen != 24 && secretlen != 32) return STATUS_INVALID_PARAMETER;
        memcpy(key->key_data, secret, secretlen);
        key->block_size = AES_BLOCK_SIZE;

        mbedtls_aes_init(&key->aes_enc);
        mbedtls_aes_init(&key->aes_dec);
        mbedtls_gcm_init(&key->gcm);
        key->gcm_key_set = FALSE;
        mbedtls_ccm_init(&key->ccm);
        key->ccm_key_set = FALSE;

        if (mbedtls_aes_setkey_enc(&key->aes_enc, secret, secretlen * 8) != 0 ||
            mbedtls_aes_setkey_dec(&key->aes_dec, secret, secretlen * 8) != 0)
        {
            mbedtls_aes_free(&key->aes_enc);
            mbedtls_aes_free(&key->aes_dec);
            mbedtls_gcm_free(&key->gcm);
            mbedtls_ccm_free(&key->ccm);
            return STATUS_INTERNAL_ERROR;
        }
        return STATUS_SUCCESS;
    }

    if (alg_id == ALG_ID_3DES)
    {
        /* 3DES: 16 bytes (2-key) or 24 bytes (3-key) */
        if (secretlen != 16 && secretlen != 24) return STATUS_INVALID_PARAMETER;
        memcpy(key->key_data, secret, secretlen);
        key->block_size = 8; /* DES block */

        mbedtls_des3_init(&key->des3_enc);
        mbedtls_des3_init(&key->des3_dec);

        if (secretlen == 24)
        {
            if (mbedtls_des3_set3key_enc(&key->des3_enc, secret) != 0 ||
                mbedtls_des3_set3key_dec(&key->des3_dec, secret) != 0)
            {
                mbedtls_des3_free(&key->des3_enc);
                mbedtls_des3_free(&key->des3_dec);
                return STATUS_INTERNAL_ERROR;
            }
        }
        else /* 16 bytes = 2-key 3DES */
        {
            if (mbedtls_des3_set2key_enc(&key->des3_enc, secret) != 0 ||
                mbedtls_des3_set2key_dec(&key->des3_dec, secret) != 0)
            {
                mbedtls_des3_free(&key->des3_enc);
                mbedtls_des3_free(&key->des3_dec);
                return STATUS_INTERNAL_ERROR;
            }
        }
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

static void key_free( struct key *key )
{
    if (key->alg_id == ALG_ID_AES)
    {
        mbedtls_aes_free(&key->aes_enc);
        mbedtls_aes_free(&key->aes_dec);
        mbedtls_gcm_free(&key->gcm);
        mbedtls_ccm_free(&key->ccm);
    }
    else if (key->alg_id == ALG_ID_3DES)
    {
        mbedtls_des3_free(&key->des3_enc);
        mbedtls_des3_free(&key->des3_dec);
    }
    HeapFree(GetProcessHeap(), 0, key);
}

#endif /* SONAME_LIBMBEDTLS */

#define OBJECT_LENGTH_MD5       274
#define OBJECT_LENGTH_SHA1      278
#define OBJECT_LENGTH_SHA256    286
#define OBJECT_LENGTH_SHA384    382
#define OBJECT_LENGTH_SHA512    382

static NTSTATUS generic_alg_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    if (!strcmpW( prop, BCRYPT_HASH_LENGTH ))
    {
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

static NTSTATUS get_alg_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;
    ULONG value;

    status = generic_alg_property( id, prop, buf, size, ret_size );
    if (status != STATUS_NOT_IMPLEMENTED)
        return status;

    switch (id)
    {
    case ALG_ID_MD5:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_MD5;
            break;
        }
        FIXME( "unsupported md5 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_RNG:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH )) return STATUS_NOT_SUPPORTED;
        FIXME( "unsupported rng algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_SHA1:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA1;
            break;
        }
        FIXME( "unsupported sha1 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_SHA224:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA256; /* SHA-224 uses same context as SHA-256 */
            break;
        }
        FIXME( "unsupported sha224 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_SHA256:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA256;
            break;
        }
        FIXME( "unsupported sha256 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_SHA384:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA384;
            break;
        }
        FIXME( "unsupported sha384 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_SHA512:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA512;
            break;
        }
        FIXME( "unsupported sha512 algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_AES:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            /* size of struct key, returned so callers can allocate the object buffer */
            value = 512; /* conservative upper bound including mbedtls contexts */
            break;
        }
        if (!strcmpW( prop, BCRYPT_BLOCK_LENGTH ))
        {
            value = 16;
            break;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTHS ))
        {
            BCRYPT_KEY_LENGTHS_STRUCT *kl = (BCRYPT_KEY_LENGTHS_STRUCT *)buf;
            *ret_size = sizeof(*kl);
            if (size < sizeof(*kl)) return STATUS_BUFFER_TOO_SMALL;
            if (kl)
            {
                kl->dwMinLength = 128;
                kl->dwMaxLength = 256;
                kl->dwIncrement = 64;
            }
            return STATUS_SUCCESS;
        }
        FIXME( "unsupported AES algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_3DES:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = 512;
            break;
        }
        if (!strcmpW( prop, BCRYPT_BLOCK_LENGTH ))
        {
            value = 8; /* DES block = 8 bytes */
            break;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTHS ))
        {
            BCRYPT_KEY_LENGTHS_STRUCT *kl = (BCRYPT_KEY_LENGTHS_STRUCT *)buf;
            *ret_size = sizeof(*kl);
            if (size < sizeof(*kl)) return STATUS_BUFFER_TOO_SMALL;
            if (kl)
            {
                kl->dwMinLength = 112; /* 2-key 3DES effective bits */
                kl->dwMaxLength = 168; /* 3-key 3DES effective bits */
                kl->dwIncrement = 56;
            }
            return STATUS_SUCCESS;
        }
        FIXME( "unsupported 3DES algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_RSA:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = 512; /* struct key_pair upper bound */
            break;
        }
        FIXME( "unsupported RSA algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = 512;
            break;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTH ))
        {
            value = (id == ALG_ID_ECDSA_P256) ? 256 : 384;
            break;
        }
        FIXME( "unsupported ECDSA algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDH_P384:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = 512; /* struct key_pair upper bound */
            break;
        }
        if (!strcmpW( prop, BCRYPT_KEY_LENGTH ))
        {
            value = (id == ALG_ID_ECDH_P256) ? 256 : 384;
            break;
        }
        FIXME( "unsupported ECDH algorithm property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

    default:
        FIXME( "unsupported algorithm %u\n", id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (size < sizeof(ULONG))
    {
        *ret_size = sizeof(ULONG);
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (buf) *(ULONG *)buf = value;
    *ret_size = sizeof(ULONG);

    return STATUS_SUCCESS;
}

static NTSTATUS get_hash_property( enum alg_id id, const WCHAR *prop, UCHAR *buf, ULONG size, ULONG *ret_size )
{
    NTSTATUS status;

    status = generic_alg_property( id, prop, buf, size, ret_size );
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
        return get_alg_property( alg->id, prop, buffer, count, res );
    }
    case MAGIC_HASH:
    {
        const struct hash *hash = (const struct hash *)object;
        return get_hash_property( hash->alg_id, prop, buffer, count, res );
    }
    case MAGIC_KEY:
    {
#ifdef SONAME_LIBMBEDTLS
        const struct key *key = (const struct key *)object;
        if (!strcmpW( prop, BCRYPT_KEY_LENGTH ))
        {
            if (count < sizeof(ULONG)) { *res = sizeof(ULONG); return STATUS_BUFFER_TOO_SMALL; }
            if (buffer) *(ULONG *)buffer = key->key_len * 8;
            *res = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_BLOCK_LENGTH ))
        {
            if (count < sizeof(ULONG)) { *res = sizeof(ULONG); return STATUS_BUFFER_TOO_SMALL; }
            if (buffer) *(ULONG *)buffer = key->block_size;
            *res = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            ULONG sz = (strlenW(key->chaining_mode) + 1) * sizeof(WCHAR);
            *res = sz;
            if (count < sz) return STATUS_BUFFER_TOO_SMALL;
            if (buffer) memcpy(buffer, key->chaining_mode, sz);
            return STATUS_SUCCESS;
        }
        FIXME( "unsupported key property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;
#else
        FIXME( "key object not supported without mbedTLS\n" );
        return STATUS_NOT_IMPLEMENTED;
#endif
    }
    case MAGIC_KPAIR:
    {
#ifdef SONAME_LIBMBEDTLS
        const struct key_pair *kp = (const struct key_pair *)object;
        if (!strcmpW( prop, BCRYPT_KEY_LENGTH ))
        {
            if (count < sizeof(ULONG)) { *res = sizeof(ULONG); return STATUS_BUFFER_TOO_SMALL; }
            if (buffer) *(ULONG *)buffer = kp->bitlen;
            *res = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
        if (!strcmpW( prop, BCRYPT_KEY_STRENGTH ))
        {
            if (count < sizeof(ULONG)) { *res = sizeof(ULONG); return STATUS_BUFFER_TOO_SMALL; }
            if (buffer) *(ULONG *)buffer = kp->bitlen;
            *res = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
        FIXME( "unsupported key-pair property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;
#else
        return STATUS_NOT_IMPLEMENTED;
#endif
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
    struct hash *hash;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n", algorithm, handle, object, objectlen,
           secret, secretlen, flags );

    if (flags & ~BCRYPT_HASH_REUSABLE_FLAG)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*hash) ))) return STATUS_NO_MEMORY;
    hash->hdr.magic = MAGIC_HASH;
    hash->alg_id    = alg->id;
    hash->hmac      = alg->hmac;
#ifdef SONAME_LIBMBEDTLS
    hash->reusable  = (flags & BCRYPT_HASH_REUSABLE_FLAG) != 0;
#endif

    if (hash->hmac)
    {
        status = hmac_init( hash, secret, secretlen );
    }
    else
    {
        status = hash_init( hash );
    }

    if (status != STATUS_SUCCESS)
    {
        HeapFree( GetProcessHeap(), 0, hash );
        return status;
    }

    *handle = hash;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDestroyHash( BCRYPT_HASH_HANDLE handle )
{
    struct hash *hash = handle;

    TRACE( "%p\n", handle );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    hash->hdr.magic = 0;
#ifdef SONAME_LIBMBEDTLS
    hash_free_contexts( hash );
#endif
    HeapFree( GetProcessHeap(), 0, hash );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptHashData( BCRYPT_HASH_HANDLE handle, UCHAR *input, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, input, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!input) return STATUS_SUCCESS;

    if (hash->hmac)
    {
        return hmac_update( hash, input, size );
    }
    else
    {
        return hash_update( hash, input, size );
    }
}

NTSTATUS WINAPI BCryptFinishHash( BCRYPT_HASH_HANDLE handle, UCHAR *output, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, output, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!output) return STATUS_INVALID_PARAMETER;

    if (hash->hmac)
    {
        return hmac_finish( hash, output, size );
    }
    else
    {
        return hash_finish( hash, output, size );
    }
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

/* Supported algorithm list for BCryptEnumAlgorithms */
static const struct
{
    ULONG        class_flag;
    const WCHAR *name;
} supported_algs[] =
{
    { BCRYPT_HASH_OPERATION,      BCRYPT_MD5_ALGORITHM    },
    { BCRYPT_HASH_OPERATION,      BCRYPT_SHA1_ALGORITHM   },
    { BCRYPT_HASH_OPERATION,      BCRYPT_SHA224_ALGORITHM },
    { BCRYPT_HASH_OPERATION,      BCRYPT_SHA256_ALGORITHM },
    { BCRYPT_HASH_OPERATION,      BCRYPT_SHA384_ALGORITHM },
    { BCRYPT_HASH_OPERATION,      BCRYPT_SHA512_ALGORITHM },
    { BCRYPT_CIPHER_OPERATION,    BCRYPT_AES_ALGORITHM    },
    { BCRYPT_CIPHER_OPERATION,    BCRYPT_3DES_ALGORITHM   },
    { BCRYPT_RNG_OPERATION,       BCRYPT_RNG_ALGORITHM    },
    { BCRYPT_ASYMMETRIC_ENCRYPTION_OPERATION | BCRYPT_SIGNATURE_OPERATION, BCRYPT_RSA_ALGORITHM },
    { BCRYPT_SIGNATURE_OPERATION,              BCRYPT_ECDSA_P256_ALGORITHM },
    { BCRYPT_SIGNATURE_OPERATION,              BCRYPT_ECDSA_P384_ALGORITHM },
    { BCRYPT_SECRET_AGREEMENT_OPERATION,       BCRYPT_ECDH_P256_ALGORITHM  },
    { BCRYPT_SECRET_AGREEMENT_OPERATION,       BCRYPT_ECDH_P384_ALGORITHM  },
};

NTSTATUS WINAPI BCryptEnumAlgorithms( ULONG dwAlgOperations, ULONG *pAlgCount,
                                      BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags )
{
    BCRYPT_ALGORITHM_IDENTIFIER *list;
    ULONG i, count = 0;

    TRACE( "%08x, %p, %p, %08x\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags );

    if (!pAlgCount || !ppAlgList) return STATUS_INVALID_PARAMETER;

    /* count matching entries */
    for (i = 0; i < ARRAY_SIZE(supported_algs); i++)
    {
        if (!dwAlgOperations || (supported_algs[i].class_flag & dwAlgOperations))
            count++;
    }

    list = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*list) );
    if (!list) return STATUS_NO_MEMORY;

    count = 0;
    for (i = 0; i < ARRAY_SIZE(supported_algs); i++)
    {
        if (!dwAlgOperations || (supported_algs[i].class_flag & dwAlgOperations))
        {
            list[count].pszName  = (LPWSTR)supported_algs[i].name;
            list[count].dwClass  = supported_algs[i].class_flag;
            list[count].dwFlags  = 0;
            count++;
        }
    }

    *pAlgCount = count;
    *ppAlgList = list;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateKey( BCRYPT_KEY_HANDLE handle, BCRYPT_KEY_HANDLE *handle_copy,
                                     UCHAR *object, ULONG objectlen, ULONG flags )
{
    TRACE( "%p, %p, %p, %u, %08x\n", handle, handle_copy, object, objectlen, flags );

    if (!handle_copy) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    {
        struct object *obj = handle;
        if (!obj) return STATUS_INVALID_HANDLE;

        if (obj->magic == MAGIC_KEY)
        {
            struct key *orig = (struct key *)obj;
            struct key *copy = HeapAlloc( GetProcessHeap(), 0, sizeof(*copy) );
            if (!copy) return STATUS_NO_MEMORY;
            memcpy( copy, orig, sizeof(*copy) );
            /* Re-initialize mbedTLS contexts from saved key data */
            if (orig->alg_id == ALG_ID_AES)
            {
                mbedtls_aes_init(&copy->aes_enc);
                mbedtls_aes_init(&copy->aes_dec);
                mbedtls_gcm_init(&copy->gcm);
                copy->gcm_key_set = FALSE;
                mbedtls_aes_setkey_enc(&copy->aes_enc, copy->key_data, copy->key_len * 8);
                mbedtls_aes_setkey_dec(&copy->aes_dec, copy->key_data, copy->key_len * 8);
                if (orig->gcm_key_set)
                {
                    if (mbedtls_gcm_setkey(&copy->gcm, MBEDTLS_CIPHER_ID_AES,
                                            copy->key_data, copy->key_len * 8) == 0)
                        copy->gcm_key_set = TRUE;
                }
            }
            else if (orig->alg_id == ALG_ID_3DES)
            {
                mbedtls_des3_init(&copy->des3_enc);
                mbedtls_des3_init(&copy->des3_dec);
                if (orig->key_len == 24)
                {
                    mbedtls_des3_set3key_enc(&copy->des3_enc, copy->key_data);
                    mbedtls_des3_set3key_dec(&copy->des3_dec, copy->key_data);
                }
                else
                {
                    mbedtls_des3_set2key_enc(&copy->des3_enc, copy->key_data);
                    mbedtls_des3_set2key_dec(&copy->des3_dec, copy->key_data);
                }
            }
            *handle_copy = copy;
            return STATUS_SUCCESS;
        }
    }
#endif
    FIXME( "unsupported handle type\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptGenerateKeyPair( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE *handle,
                                        ULONG length, ULONG flags )
{
    struct algorithm *alg = algorithm;

    TRACE( "%p, %p, %u, %08x\n", algorithm, handle, length, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!handle) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    if (alg->id == ALG_ID_RSA)
    {
        struct key_pair *kp;
        if (length < 512 || length > 16384 || (length % 64))
            return STATUS_INVALID_PARAMETER;

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic  = MAGIC_KPAIR;
        kp->alg_id     = ALG_ID_RSA;
        kp->bitlen     = length;
        kp->finalized  = FALSE;
        kp->has_private= FALSE;
        mbedtls_rsa_init( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, 0 );

        *handle = kp;
        return STATUS_SUCCESS;
    }
    if (alg->id == ALG_ID_ECDH_P256 || alg->id == ALG_ID_ECDH_P384)
    {
        struct key_pair *kp;
        ULONG expected_bits = (alg->id == ALG_ID_ECDH_P256) ? 256 : 384;

        if (length != 0 && length != expected_bits)
            return STATUS_INVALID_PARAMETER;

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic  = MAGIC_KPAIR;
        kp->alg_id     = alg->id;
        kp->bitlen     = expected_bits;
        kp->finalized  = FALSE;
        kp->has_private= FALSE;
        mbedtls_ecdh_init( &kp->u.ecdh );

        *handle = kp;
        return STATUS_SUCCESS;
    }
    if (alg->id == ALG_ID_ECDSA_P256 || alg->id == ALG_ID_ECDSA_P384)
    {
        struct key_pair *kp;
        ULONG expected_bits = (alg->id == ALG_ID_ECDSA_P256) ? 256 : 384;

        if (length != 0 && length != expected_bits)
            return STATUS_INVALID_PARAMETER;

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic  = MAGIC_KPAIR;
        kp->alg_id     = alg->id;
        kp->bitlen     = expected_bits;
        kp->finalized  = FALSE;
        kp->has_private= FALSE;
        mbedtls_ecdsa_init( &kp->u.ecdsa );

        *handle = kp;
        return STATUS_SUCCESS;
    }
#endif
    FIXME( "algorithm %u key pair generation not supported\n", alg->id );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptFinalizeKeyPair( BCRYPT_KEY_HANDLE handle, ULONG flags )
{
    TRACE( "%p, %08x\n", handle, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct key_pair *kp = handle;
        if (!kp || kp->hdr.magic != MAGIC_KPAIR) return STATUS_INVALID_HANDLE;
        if (kp->finalized) return STATUS_SUCCESS;

        if (kp->alg_id == ALG_ID_RSA)
        {
            if (mbedtls_rsa_gen_key( &kp->u.rsa, rng_cb, NULL, kp->bitlen, 65537 ) != 0)
                return STATUS_INTERNAL_ERROR;
            kp->finalized  = TRUE;
            kp->has_private= TRUE;
            return STATUS_SUCCESS;
        }
        if (kp->alg_id == ALG_ID_ECDH_P256 || kp->alg_id == ALG_ID_ECDH_P384)
        {
            mbedtls_ecp_group_id grp_id = (kp->alg_id == ALG_ID_ECDH_P256)
                                          ? MBEDTLS_ECP_DP_SECP256R1
                                          : MBEDTLS_ECP_DP_SECP384R1;
            if (mbedtls_ecdh_setup( &kp->u.ecdh, grp_id ) != 0)
                return STATUS_INTERNAL_ERROR;
            if (mbedtls_ecdh_gen_public( &kp->u.ecdh.grp, &kp->u.ecdh.d, &kp->u.ecdh.Q,
                                          rng_cb, NULL ) != 0)
                return STATUS_INTERNAL_ERROR;
            kp->finalized  = TRUE;
            kp->has_private= TRUE;
            return STATUS_SUCCESS;
        }
        if (kp->alg_id == ALG_ID_ECDSA_P256 || kp->alg_id == ALG_ID_ECDSA_P384)
        {
            mbedtls_ecp_group_id grp_id = (kp->alg_id == ALG_ID_ECDSA_P256)
                                          ? MBEDTLS_ECP_DP_SECP256R1
                                          : MBEDTLS_ECP_DP_SECP384R1;
            if (mbedtls_ecdsa_genkey( &kp->u.ecdsa, grp_id, rng_cb, NULL ) != 0)
                return STATUS_INTERNAL_ERROR;
            kp->finalized  = TRUE;
            kp->has_private= TRUE;
            return STATUS_SUCCESS;
        }
    }
#endif
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptImportKeyPair( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE decrypt_key,
                                      LPCWSTR type, BCRYPT_KEY_HANDLE *handle,
                                      UCHAR *input, ULONG inputlen, ULONG flags )
{
    struct algorithm *alg = algorithm;

    TRACE( "%p, %p, %s, %p, %p, %u, %08x\n", algorithm, decrypt_key, debugstr_w(type),
           handle, input, inputlen, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!type || !handle || !input) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    if (alg->id == ALG_ID_RSA &&
        (!strcmpW( type, BCRYPT_RSAPUBLIC_BLOB ) || !strcmpW( type, BCRYPT_RSAPRIVATE_BLOB )))
    {
        const BCRYPT_RSAKEY_BLOB *hdr = (const BCRYPT_RSAKEY_BLOB *)input;
        BOOL is_public = !strcmpW( type, BCRYPT_RSAPUBLIC_BLOB );
        const UCHAR *p = (const UCHAR *)(hdr + 1);
        mbedtls_mpi N, E, D, P, Q;
        struct key_pair *kp;
        int ret;

        if (inputlen < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;
        if (is_public  && hdr->Magic != BCRYPT_RSAPUBLIC_MAGIC)  return STATUS_INVALID_PARAMETER;
        if (!is_public && hdr->Magic != BCRYPT_RSAPRIVATE_MAGIC) return STATUS_INVALID_PARAMETER;

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic  = MAGIC_KPAIR;
        kp->alg_id     = ALG_ID_RSA;
        kp->bitlen     = hdr->BitLength;
        kp->finalized  = TRUE;
        kp->has_private= !is_public;
        mbedtls_rsa_init( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, 0 );

        mbedtls_mpi_init(&N); mbedtls_mpi_init(&E);
        mbedtls_mpi_init(&D); mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q);

        /* Public exponent (big-endian) */
        mbedtls_mpi_read_binary(&E, p, hdr->cbPublicExp); p += hdr->cbPublicExp;
        /* Modulus */
        mbedtls_mpi_read_binary(&N, p, hdr->cbModulus);   p += hdr->cbModulus;

        if (!is_public && hdr->cbPrime1 && hdr->cbPrime2)
        {
            mbedtls_mpi_read_binary(&P, p, hdr->cbPrime1); p += hdr->cbPrime1;
            mbedtls_mpi_read_binary(&Q, p, hdr->cbPrime2);
        }

        ret = mbedtls_rsa_import( &kp->u.rsa, &N, is_public ? NULL : &P,
                                   is_public ? NULL : &Q, NULL, &E );
        if (ret == 0 && !is_public)
            ret = mbedtls_rsa_complete( &kp->u.rsa );

        mbedtls_mpi_free(&N); mbedtls_mpi_free(&E);
        mbedtls_mpi_free(&D); mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);

        if (ret != 0)
        {
            mbedtls_rsa_free( &kp->u.rsa );
            HeapFree( GetProcessHeap(), 0, kp );
            return STATUS_INVALID_PARAMETER;
        }

        *handle = kp;
        return STATUS_SUCCESS;
    }
#endif
#ifdef SONAME_LIBMBEDTLS
    if ((alg->id == ALG_ID_ECDSA_P256 || alg->id == ALG_ID_ECDSA_P384) &&
        (!strcmpW( type, BCRYPT_ECCPUBLIC_BLOB ) || !strcmpW( type, BCRYPT_ECCPRIVATE_BLOB )))
    {
        const BCRYPT_ECCKEY_BLOB *hdr = (const BCRYPT_ECCKEY_BLOB *)input;
        BOOL is_public = !strcmpW( type, BCRYPT_ECCPUBLIC_BLOB );
        ULONG expected_magic_pub, expected_magic_priv, coord_size;
        struct key_pair *kp;

        if (alg->id == ALG_ID_ECDSA_P256)
        {
            expected_magic_pub  = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
            expected_magic_priv = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
            coord_size = 32;
        }
        else
        {
            expected_magic_pub  = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
            expected_magic_priv = BCRYPT_ECDSA_PRIVATE_P384_MAGIC;
            coord_size = 48;
        }

        if (inputlen < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;
        if (is_public  && hdr->dwMagic != expected_magic_pub)  return STATUS_INVALID_PARAMETER;
        if (!is_public && hdr->dwMagic != expected_magic_priv) return STATUS_INVALID_PARAMETER;
        if (hdr->cbKey != coord_size) return STATUS_INVALID_PARAMETER;

        {
            ULONG min_len = sizeof(*hdr) + 2 * coord_size + (is_public ? 0 : coord_size);
            if (inputlen < min_len) return STATUS_INVALID_PARAMETER;
        }

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic   = MAGIC_KPAIR;
        kp->alg_id      = alg->id;
        kp->bitlen      = coord_size * 8;
        kp->finalized   = TRUE;
        kp->has_private = !is_public;
        mbedtls_ecdsa_init( &kp->u.ecdsa );

        if (mbedtls_ecp_group_load( &kp->u.ecdsa.grp,
                                     (alg->id == ALG_ID_ECDSA_P256)
                                      ? MBEDTLS_ECP_DP_SECP256R1
                                      : MBEDTLS_ECP_DP_SECP384R1 ) != 0)
        {
            mbedtls_ecdsa_free( &kp->u.ecdsa );
            HeapFree( GetProcessHeap(), 0, kp );
            return STATUS_INTERNAL_ERROR;
        }

        {
            const UCHAR *p = (const UCHAR *)(hdr + 1);
            UCHAR point_buf[1 + 2 * 48];
            point_buf[0] = 0x04;
            memcpy( point_buf + 1,              p,              coord_size );
            memcpy( point_buf + 1 + coord_size, p + coord_size, coord_size );

            if (mbedtls_ecp_point_read_binary( &kp->u.ecdsa.grp, &kp->u.ecdsa.Q,
                                                point_buf, 1 + 2 * coord_size ) != 0)
            {
                mbedtls_ecdsa_free( &kp->u.ecdsa );
                HeapFree( GetProcessHeap(), 0, kp );
                return STATUS_INVALID_PARAMETER;
            }

            if (!is_public)
            {
                p += 2 * coord_size;
                if (mbedtls_mpi_read_binary( &kp->u.ecdsa.d, p, coord_size ) != 0)
                {
                    mbedtls_ecdsa_free( &kp->u.ecdsa );
                    HeapFree( GetProcessHeap(), 0, kp );
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }

        *handle = kp;
        return STATUS_SUCCESS;
    }
    if ((alg->id == ALG_ID_ECDH_P256 || alg->id == ALG_ID_ECDH_P384) &&
        (!strcmpW( type, BCRYPT_ECCPUBLIC_BLOB ) || !strcmpW( type, BCRYPT_ECCPRIVATE_BLOB )))
    {
        const BCRYPT_ECCKEY_BLOB *hdr = (const BCRYPT_ECCKEY_BLOB *)input;
        BOOL is_public = !strcmpW( type, BCRYPT_ECCPUBLIC_BLOB );
        mbedtls_ecp_group_id grp_id;
        ULONG expected_magic_pub, expected_magic_priv, coord_size;
        struct key_pair *kp;

        if (alg->id == ALG_ID_ECDH_P256)
        {
            grp_id           = MBEDTLS_ECP_DP_SECP256R1;
            expected_magic_pub  = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
            expected_magic_priv = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
            coord_size       = 32;
        }
        else
        {
            grp_id           = MBEDTLS_ECP_DP_SECP384R1;
            expected_magic_pub  = BCRYPT_ECDH_PUBLIC_P384_MAGIC;
            expected_magic_priv = BCRYPT_ECDH_PRIVATE_P384_MAGIC;
            coord_size       = 48;
        }

        if (inputlen < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;
        if (is_public  && hdr->dwMagic != expected_magic_pub)  return STATUS_INVALID_PARAMETER;
        if (!is_public && hdr->dwMagic != expected_magic_priv) return STATUS_INVALID_PARAMETER;
        if (hdr->cbKey != coord_size) return STATUS_INVALID_PARAMETER;

        /* Public blob: X (cbKey) + Y (cbKey); Private blob: X + Y + d */
        {
            ULONG min_len = sizeof(*hdr) + 2 * coord_size + (is_public ? 0 : coord_size);
            if (inputlen < min_len) return STATUS_INVALID_PARAMETER;
        }

        kp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*kp) );
        if (!kp) return STATUS_NO_MEMORY;

        kp->hdr.magic   = MAGIC_KPAIR;
        kp->alg_id      = alg->id;
        kp->bitlen      = coord_size * 8;
        kp->finalized   = TRUE;
        kp->has_private = !is_public;
        mbedtls_ecdh_init( &kp->u.ecdh );

        if (mbedtls_ecdh_setup( &kp->u.ecdh, grp_id ) != 0)
        {
            HeapFree( GetProcessHeap(), 0, kp );
            return STATUS_INTERNAL_ERROR;
        }

        {
            const UCHAR *p = (const UCHAR *)(hdr + 1);
            /* Read uncompressed point: 0x04 || X || Y */
            UCHAR point_buf[1 + 2 * 48]; /* up to P-384 */
            point_buf[0] = 0x04;
            memcpy( point_buf + 1,               p,              coord_size );
            memcpy( point_buf + 1 + coord_size,  p + coord_size, coord_size );

            if (mbedtls_ecp_point_read_binary( &kp->u.ecdh.grp, &kp->u.ecdh.Q,
                                                point_buf, 1 + 2 * coord_size ) != 0)
            {
                mbedtls_ecdh_free( &kp->u.ecdh );
                HeapFree( GetProcessHeap(), 0, kp );
                return STATUS_INVALID_PARAMETER;
            }

            if (!is_public)
            {
                p += 2 * coord_size;
                if (mbedtls_mpi_read_binary( &kp->u.ecdh.d, p, coord_size ) != 0)
                {
                    mbedtls_ecdh_free( &kp->u.ecdh );
                    HeapFree( GetProcessHeap(), 0, kp );
                    return STATUS_INVALID_PARAMETER;
                }
            }
        }

        *handle = kp;
        return STATUS_SUCCESS;
    }
#endif
    FIXME( "key pair type %s not supported\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptSignHash( BCRYPT_KEY_HANDLE handle, void *padding_info,
                                 UCHAR *hash, ULONG hashlen,
                                 UCHAR *sig, ULONG siglen, ULONG *retlen, ULONG flags )
{
    TRACE( "%p, %p, %p, %u, %p, %u, %p, %08x\n",
           handle, padding_info, hash, hashlen, sig, siglen, retlen, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct key_pair *kp = handle;

        if (!kp || kp->hdr.magic != MAGIC_KPAIR) return STATUS_INVALID_HANDLE;
        if (!kp->has_private) return STATUS_INVALID_PARAMETER;
        if (!hash || !hashlen) return STATUS_INVALID_PARAMETER;

        if (kp->alg_id == ALG_ID_RSA)
        {
            mbedtls_md_type_t md = MBEDTLS_MD_NONE;
            ULONG key_bytes;
            int ret;

            if (!(flags & (BCRYPT_PAD_PKCS1 | BCRYPT_PAD_PSS)))
            {
                FIXME("RSA: unsupported padding flags %08x\n", flags);
                return STATUS_NOT_IMPLEMENTED;
            }

            key_bytes = (ULONG)mbedtls_rsa_get_len( &kp->u.rsa );
            if (retlen) *retlen = key_bytes;
            if (!sig) return STATUS_SUCCESS;
            if (siglen < key_bytes) return STATUS_BUFFER_TOO_SMALL;

            if (flags & BCRYPT_PAD_PKCS1)
            {
                BCRYPT_PKCS1_PADDING_INFO *pkcs1 = padding_info;
                if (pkcs1 && pkcs1->pszAlgId)
                {
                    if      (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA1_ALGORITHM   )) md = MBEDTLS_MD_SHA1;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_MD5_ALGORITHM    )) md = MBEDTLS_MD_MD5;
                }
                if (md == MBEDTLS_MD_NONE)
                {
                    if      (hashlen == 20) md = MBEDTLS_MD_SHA1;
                    else if (hashlen == 32) md = MBEDTLS_MD_SHA256;
                    else if (hashlen == 48) md = MBEDTLS_MD_SHA384;
                    else if (hashlen == 64) md = MBEDTLS_MD_SHA512;
                    else if (hashlen == 16) md = MBEDTLS_MD_MD5;
                }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE );
                ret = mbedtls_rsa_rsassa_pkcs1_v15_sign( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PRIVATE, md, hashlen, hash, sig );
            }
            else /* BCRYPT_PAD_PSS */
            {
                BCRYPT_PSS_PADDING_INFO *pss = padding_info;
                if (pss && pss->pszAlgId)
                {
                    if      (!strcmpW( pss->pszAlgId, BCRYPT_SHA1_ALGORITHM   )) md = MBEDTLS_MD_SHA1;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                }
                if (md == MBEDTLS_MD_NONE)
                {
                    if      (hashlen == 20) md = MBEDTLS_MD_SHA1;
                    else if (hashlen == 32) md = MBEDTLS_MD_SHA256;
                    else if (hashlen == 48) md = MBEDTLS_MD_SHA384;
                    else if (hashlen == 64) md = MBEDTLS_MD_SHA512;
                }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V21, md );
                ret = mbedtls_rsa_rsassa_pss_sign( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PRIVATE, md, hashlen, hash, sig );
            }

            if (ret != 0)
            {
                WARN( "RSA sign failed: -0x%04x\n", -ret );
                return STATUS_INTERNAL_ERROR;
            }
            return STATUS_SUCCESS;
        }

        if (kp->alg_id == ALG_ID_ECDSA_P256 || kp->alg_id == ALG_ID_ECDSA_P384)
        {
            ULONG coord_size = (kp->alg_id == ALG_ID_ECDSA_P256) ? 32 : 48;
            ULONG sig_size   = 2 * coord_size;
            mbedtls_mpi r, s;
            int ret;

            if (retlen) *retlen = sig_size;
            if (!sig) return STATUS_SUCCESS;
            if (siglen < sig_size) return STATUS_BUFFER_TOO_SMALL;

            mbedtls_mpi_init( &r );
            mbedtls_mpi_init( &s );
            ret = mbedtls_ecdsa_sign( &kp->u.ecdsa.grp, &r, &s, &kp->u.ecdsa.d,
                                       hash, hashlen, rng_cb, NULL );
            if (ret == 0)
            {
                ret  = mbedtls_mpi_write_binary( &r, sig,             coord_size );
                ret |= mbedtls_mpi_write_binary( &s, sig + coord_size, coord_size );
            }
            mbedtls_mpi_free( &r );
            mbedtls_mpi_free( &s );
            return (ret == 0) ? STATUS_SUCCESS : STATUS_INTERNAL_ERROR;
        }

        FIXME( "algorithm %u not supported for signing\n", kp->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
#endif
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptVerifySignature( BCRYPT_KEY_HANDLE handle, void *padding_info,
                                        UCHAR *hash, ULONG hashlen,
                                        UCHAR *sig, ULONG siglen, ULONG flags )
{
    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n",
           handle, padding_info, hash, hashlen, sig, siglen, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct key_pair *kp = handle;

        if (!kp || kp->hdr.magic != MAGIC_KPAIR) return STATUS_INVALID_HANDLE;
        if (!hash || !hashlen || !sig || !siglen) return STATUS_INVALID_PARAMETER;

        if (kp->alg_id == ALG_ID_RSA)
        {
            mbedtls_md_type_t md = MBEDTLS_MD_NONE;
            int ret;

            if (!(flags & (BCRYPT_PAD_PKCS1 | BCRYPT_PAD_PSS)))
            {
                FIXME("RSA: unsupported padding flags %08x\n", flags);
                return STATUS_NOT_IMPLEMENTED;
            }

            if (flags & BCRYPT_PAD_PKCS1)
            {
                BCRYPT_PKCS1_PADDING_INFO *pkcs1 = padding_info;
                if (pkcs1 && pkcs1->pszAlgId)
                {
                    if      (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA1_ALGORITHM   )) md = MBEDTLS_MD_SHA1;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                    else if (!strcmpW( pkcs1->pszAlgId, BCRYPT_MD5_ALGORITHM    )) md = MBEDTLS_MD_MD5;
                }
                if (md == MBEDTLS_MD_NONE)
                {
                    if      (hashlen == 20) md = MBEDTLS_MD_SHA1;
                    else if (hashlen == 32) md = MBEDTLS_MD_SHA256;
                    else if (hashlen == 48) md = MBEDTLS_MD_SHA384;
                    else if (hashlen == 64) md = MBEDTLS_MD_SHA512;
                    else if (hashlen == 16) md = MBEDTLS_MD_MD5;
                }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE );
                ret = mbedtls_rsa_rsassa_pkcs1_v15_verify( &kp->u.rsa, NULL, NULL,
                    MBEDTLS_RSA_PUBLIC, md, hashlen, hash, sig );
            }
            else /* BCRYPT_PAD_PSS */
            {
                BCRYPT_PSS_PADDING_INFO *pss = padding_info;
                if (pss && pss->pszAlgId)
                {
                    if      (!strcmpW( pss->pszAlgId, BCRYPT_SHA1_ALGORITHM   )) md = MBEDTLS_MD_SHA1;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( pss->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                }
                if (md == MBEDTLS_MD_NONE)
                {
                    if      (hashlen == 20) md = MBEDTLS_MD_SHA1;
                    else if (hashlen == 32) md = MBEDTLS_MD_SHA256;
                    else if (hashlen == 48) md = MBEDTLS_MD_SHA384;
                    else if (hashlen == 64) md = MBEDTLS_MD_SHA512;
                }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V21, md );
                ret = mbedtls_rsa_rsassa_pss_verify( &kp->u.rsa, NULL, NULL,
                    MBEDTLS_RSA_PUBLIC, md, hashlen, hash, sig );
            }

            if (ret == MBEDTLS_ERR_RSA_VERIFY_FAILED) return STATUS_INVALID_SIGNATURE;
            if (ret != 0)
            {
                WARN( "RSA verify failed: -0x%04x\n", -ret );
                return STATUS_INTERNAL_ERROR;
            }
            return STATUS_SUCCESS;
        }

        if (kp->alg_id == ALG_ID_ECDSA_P256 || kp->alg_id == ALG_ID_ECDSA_P384)
        {
            ULONG coord_size = (kp->alg_id == ALG_ID_ECDSA_P256) ? 32 : 48;
            ULONG sig_size   = 2 * coord_size;
            mbedtls_mpi r, s;
            int ret;

            if (siglen != sig_size) return STATUS_INVALID_PARAMETER;

            mbedtls_mpi_init( &r );
            mbedtls_mpi_init( &s );
            mbedtls_mpi_read_binary( &r, sig,             coord_size );
            mbedtls_mpi_read_binary( &s, sig + coord_size, coord_size );
            ret = mbedtls_ecdsa_verify( &kp->u.ecdsa.grp, hash, hashlen,
                                         &kp->u.ecdsa.Q, &r, &s );
            mbedtls_mpi_free( &r );
            mbedtls_mpi_free( &s );
            if (ret == MBEDTLS_ERR_ECP_VERIFY_FAILED) return STATUS_INVALID_SIGNATURE;
            if (ret != 0) return STATUS_INTERNAL_ERROR;
            return STATUS_SUCCESS;
        }

        FIXME( "algorithm %u not supported for verify\n", kp->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
#endif
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptFreeBuffer( PVOID buffer )
{
    TRACE( "%p\n", buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptDuplicateHash( BCRYPT_HASH_HANDLE handle, BCRYPT_HASH_HANDLE *handle_copy,
                                     UCHAR *object, ULONG objectlen, ULONG flags )
{
    struct hash *hash_orig = handle;
    struct hash *hash_copy;

    TRACE( "%p, %p, %p, %u, %08x\n", handle, handle_copy, object, objectlen, flags );

    if (!hash_orig || hash_orig->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!handle_copy) return STATUS_INVALID_PARAMETER;

    if (!(hash_copy = HeapAlloc( GetProcessHeap(), 0, sizeof(*hash_copy) )))
        return STATUS_NO_MEMORY;

    memcpy( hash_copy, hash_orig, sizeof(*hash_copy) );

#ifdef SONAME_LIBMBEDTLS
    /* Deep-copy the HMAC key buffer if present */
    hash_copy->hmac_key = NULL;
    if (hash_orig->hmac_key && hash_orig->hmac_key_len > 0)
    {
        hash_copy->hmac_key = HeapAlloc(GetProcessHeap(), 0, hash_orig->hmac_key_len);
        if (!hash_copy->hmac_key)
        {
            HeapFree(GetProcessHeap(), 0, hash_copy);
            return STATUS_NO_MEMORY;
        }
        memcpy(hash_copy->hmac_key, hash_orig->hmac_key, hash_orig->hmac_key_len);
    }
#endif

    *handle_copy = hash_copy;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptSetProperty( BCRYPT_HANDLE handle, LPCWSTR prop, UCHAR *value, ULONG size, ULONG flags )
{
    struct object *object = handle;

    TRACE( "%p, %s, %p, %u, %08x\n", handle, debugstr_w(prop), value, size, flags );

    if (!object) return STATUS_INVALID_HANDLE;

    switch (object->magic)
    {
    case MAGIC_ALG:
        FIXME( "setting property %s on algorithm not supported\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;

#ifdef SONAME_LIBMBEDTLS
    case MAGIC_KEY:
    {
        struct key *key = (struct key *)object;
        if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
        {
            if (!value || size < sizeof(WCHAR)) return STATUS_INVALID_PARAMETER;
            lstrcpynW( key->chaining_mode, (const WCHAR *)value,
                       sizeof(key->chaining_mode) / sizeof(WCHAR) );
            /* Lazily initialize GCM/CCM key when switching mode (AES only) */
            if (key->alg_id == ALG_ID_AES)
            {
                if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_GCM ) && !key->gcm_key_set)
                {
                    mbedtls_gcm_free(&key->gcm);
                    mbedtls_gcm_init(&key->gcm);
                    if (mbedtls_gcm_setkey(&key->gcm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) == 0)
                        key->gcm_key_set = TRUE;
                }
                else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CCM ) && !key->ccm_key_set)
                {
                    mbedtls_ccm_free(&key->ccm);
                    mbedtls_ccm_init(&key->ccm);
                    if (mbedtls_ccm_setkey(&key->ccm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) == 0)
                        key->ccm_key_set = TRUE;
                }
            }
            return STATUS_SUCCESS;
        }
        FIXME( "unsupported key property %s\n", debugstr_w(prop) );
        return STATUS_NOT_IMPLEMENTED;
    }
#endif

    default:
        WARN( "unknown magic %08x\n", object->magic );
        return STATUS_INVALID_HANDLE;
    }
}

NTSTATUS WINAPI BCryptGenerateSymmetricKey( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE *handle,
                                            UCHAR *object, ULONG objectlen,
                                            UCHAR *secret, ULONG secretlen, ULONG flags )
{
    struct algorithm *alg = algorithm;
    NTSTATUS status;

    TRACE( "%p, %p, %p, %u, %p, %u, %08x\n", algorithm, handle, object, objectlen,
           secret, secretlen, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!handle || !secret) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    {
        struct key *key;
        if (!(key = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*key) )))
            return STATUS_NO_MEMORY;

        status = key_init( key, alg->id, secret, secretlen );
        if (status != STATUS_SUCCESS)
        {
            HeapFree( GetProcessHeap(), 0, key );
            return status;
        }
        *handle = key;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "BCryptGenerateSymmetricKey not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS WINAPI BCryptDestroyKey( BCRYPT_KEY_HANDLE handle )
{
    struct object *obj = handle;

    TRACE( "%p\n", handle );

    if (!obj) return STATUS_INVALID_HANDLE;

#ifdef SONAME_LIBMBEDTLS
    if (obj->magic == MAGIC_KEY)
    {
        struct key *key = (struct key *)obj;
        key->hdr.magic = 0;
        key_free( key );
        return STATUS_SUCCESS;
    }
    if (obj->magic == MAGIC_KPAIR)
    {
        struct key_pair *kp = (struct key_pair *)obj;
        kp->hdr.magic = 0;
        if (kp->alg_id == ALG_ID_RSA)
            mbedtls_rsa_free( &kp->u.rsa );
        else if (kp->alg_id == ALG_ID_ECDSA_P256 || kp->alg_id == ALG_ID_ECDSA_P384)
            mbedtls_ecdsa_free( &kp->u.ecdsa );
        else
            mbedtls_ecdh_free( &kp->u.ecdh );
        HeapFree( GetProcessHeap(), 0, kp );
        return STATUS_SUCCESS;
    }
#endif
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI BCryptEncrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG inputlen,
                                VOID *padding, UCHAR *iv, ULONG ivlen,
                                UCHAR *output, ULONG outputlen, ULONG *retlen, ULONG flags )
{
    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n",
           handle, input, inputlen, padding, iv, ivlen, output, outputlen, retlen, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct object *obj = (struct object *)handle;
        struct key *key;
        BOOL use_padding;
        ULONG pad_len, full_len, required, rem;
        UCHAR iv_buf[AES_BLOCK_SIZE] = {0};
        UCHAR pad_block[AES_BLOCK_SIZE];

        if (!obj) return STATUS_INVALID_HANDLE;

        /* RSA asymmetric encryption using public key */
        if (obj->magic == MAGIC_KPAIR)
        {
            struct key_pair *kp = (struct key_pair *)obj;
            ULONG key_bytes;
            int ret;

            if (kp->alg_id != ALG_ID_RSA) return STATUS_NOT_IMPLEMENTED;
            if (!kp->finalized) return STATUS_INVALID_HANDLE;

            key_bytes = (ULONG)mbedtls_rsa_get_len( &kp->u.rsa );
            if (retlen) *retlen = key_bytes;
            if (!output) return STATUS_SUCCESS;
            if (outputlen < key_bytes) return STATUS_BUFFER_TOO_SMALL;

            if (flags & BCRYPT_PAD_OAEP)
            {
                BCRYPT_OAEP_PADDING_INFO *oaep = (BCRYPT_OAEP_PADDING_INFO *)padding;
                mbedtls_md_type_t md = MBEDTLS_MD_SHA1;
                const unsigned char *label = NULL;
                size_t label_len = 0;

                if (oaep && oaep->pszAlgId)
                {
                    if      (!strcmpW( oaep->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( oaep->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( oaep->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                }
                if (oaep) { label = oaep->pbLabel; label_len = oaep->cbLabel; }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V21, md );
                ret = mbedtls_rsa_rsaes_oaep_encrypt( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PUBLIC, label, label_len, inputlen, input, output );
            }
            else if (flags & BCRYPT_PAD_PKCS1)
            {
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE );
                ret = mbedtls_rsa_rsaes_pkcs1_v15_encrypt( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PUBLIC, inputlen, input, output );
            }
            else
            {
                FIXME( "unsupported RSA padding flags %08x\n", flags );
                return STATUS_NOT_SUPPORTED;
            }

            if (ret != 0)
            {
                WARN( "RSA encrypt failed: -0x%04x\n", -ret );
                return STATUS_UNSUCCESSFUL;
            }
            return STATUS_SUCCESS;
        }

        key = (struct key *)obj;
        use_padding = (flags & BCRYPT_BLOCK_PADDING) != 0;

        if (key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
        if (key->alg_id != ALG_ID_AES && key->alg_id != ALG_ID_3DES)
        {
            FIXME( "algorithm %u not supported for encrypt\n", key->alg_id );
            return STATUS_NOT_IMPLEMENTED;
        }
        if (!input && inputlen) return STATUS_INVALID_PARAMETER;

        {
            ULONG blk = key->block_size;
            if (use_padding)
            {
                rem = inputlen % blk;
                pad_len = rem ? (blk - rem) : blk;
                required = inputlen + pad_len;
            }
            else
            {
                if (inputlen % blk) return STATUS_INVALID_BUFFER_SIZE;
                required = inputlen;
                pad_len  = 0;
                rem      = 0;
            }

            if (retlen) *retlen = required;
            if (!output) return STATUS_SUCCESS;
            if (outputlen < required) return STATUS_BUFFER_TOO_SMALL;

            if (iv && ivlen >= blk) memcpy( iv_buf, iv, blk );

            full_len = (inputlen / blk) * blk;

            if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CBC ))
            {
                if (key->alg_id == ALG_ID_AES)
                {
                    if (full_len > 0)
                        mbedtls_aes_crypt_cbc( &key->aes_enc, MBEDTLS_AES_ENCRYPT, full_len,
                                               iv_buf, input, output );
                    if (use_padding)
                    {
                        rem = inputlen - full_len;
                        memcpy( pad_block, input + full_len, rem );
                        memset( pad_block + rem, (UCHAR)pad_len, pad_len );
                        mbedtls_aes_crypt_cbc( &key->aes_enc, MBEDTLS_AES_ENCRYPT, blk,
                                               iv_buf, pad_block, output + full_len );
                    }
                }
                else /* 3DES */
                {
                    if (full_len > 0)
                        mbedtls_des3_crypt_cbc( &key->des3_enc, MBEDTLS_DES_ENCRYPT, full_len,
                                                iv_buf, input, output );
                    if (use_padding)
                    {
                        rem = inputlen - full_len;
                        memcpy( pad_block, input + full_len, rem );
                        memset( pad_block + rem, (UCHAR)pad_len, pad_len );
                        mbedtls_des3_crypt_cbc( &key->des3_enc, MBEDTLS_DES_ENCRYPT, blk,
                                                iv_buf, pad_block, output + full_len );
                    }
                }
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_ECB ))
            {
                ULONG i;
                if (key->alg_id == ALG_ID_AES)
                {
                    for (i = 0; i < full_len; i += blk)
                        mbedtls_aes_crypt_ecb( &key->aes_enc, MBEDTLS_AES_ENCRYPT, input + i, output + i );
                    if (use_padding)
                    {
                        rem = inputlen - full_len;
                        memcpy( pad_block, input + full_len, rem );
                        memset( pad_block + rem, (UCHAR)pad_len, pad_len );
                        mbedtls_aes_crypt_ecb( &key->aes_enc, MBEDTLS_AES_ENCRYPT, pad_block, output + full_len );
                    }
                }
                else /* 3DES */
                {
                    for (i = 0; i < full_len; i += blk)
                        mbedtls_des3_crypt_ecb( &key->des3_enc, input + i, output + i );
                    if (use_padding)
                    {
                        rem = inputlen - full_len;
                        memcpy( pad_block, input + full_len, rem );
                        memset( pad_block + rem, (UCHAR)pad_len, pad_len );
                        mbedtls_des3_crypt_ecb( &key->des3_enc, pad_block, output + full_len );
                    }
                }
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CFB ))
            {
                size_t iv_offset = 0;
                if (key->alg_id != ALG_ID_AES) return STATUS_NOT_IMPLEMENTED;
                if (!iv || ivlen < AES_BLOCK_SIZE) return STATUS_INVALID_PARAMETER;
                if (retlen) *retlen = inputlen;
                if (!output) return STATUS_SUCCESS;
                if (outputlen < inputlen) return STATUS_BUFFER_TOO_SMALL;
                /* CFB128: uses encryption key context for both directions */
                mbedtls_aes_crypt_cfb128(&key->aes_enc, MBEDTLS_AES_ENCRYPT, inputlen,
                                          &iv_offset, iv_buf, input, output);
                /* update IV with last 16 bytes of ciphertext */
                if (inputlen >= AES_BLOCK_SIZE)
                    memcpy(iv, output + inputlen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
                return STATUS_SUCCESS;
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_GCM ))
            {
                BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth = (BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *)padding;
                if (!auth) return STATUS_INVALID_PARAMETER;
                if (key->alg_id != ALG_ID_AES) return STATUS_NOT_IMPLEMENTED;

                if (!key->gcm_key_set)
                {
                    mbedtls_gcm_free(&key->gcm);
                    mbedtls_gcm_init(&key->gcm);
                    if (mbedtls_gcm_setkey(&key->gcm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) != 0)
                        return STATUS_INTERNAL_ERROR;
                    key->gcm_key_set = TRUE;
                }

                if (retlen) *retlen = inputlen;
                if (!output) return STATUS_SUCCESS;
                if (outputlen < inputlen) return STATUS_BUFFER_TOO_SMALL;

                if (mbedtls_gcm_crypt_and_tag(&key->gcm, MBEDTLS_GCM_ENCRYPT, inputlen,
                                               auth->pbNonce, auth->cbNonce,
                                               auth->pbAuthData, auth->cbAuthData,
                                               input, output,
                                               auth->cbTag, auth->pbTag) != 0)
                    return STATUS_INTERNAL_ERROR;

                return STATUS_SUCCESS;
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CCM ))
            {
                BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth = (BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *)padding;
                if (!auth) return STATUS_INVALID_PARAMETER;
                if (key->alg_id != ALG_ID_AES) return STATUS_NOT_IMPLEMENTED;

                if (!key->ccm_key_set)
                {
                    mbedtls_ccm_free(&key->ccm);
                    mbedtls_ccm_init(&key->ccm);
                    if (mbedtls_ccm_setkey(&key->ccm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) != 0)
                        return STATUS_INTERNAL_ERROR;
                    key->ccm_key_set = TRUE;
                }

                if (retlen) *retlen = inputlen;
                if (!output) return STATUS_SUCCESS;
                if (outputlen < inputlen) return STATUS_BUFFER_TOO_SMALL;

                if (mbedtls_ccm_encrypt_and_tag(&key->ccm, inputlen,
                                                 auth->pbNonce, auth->cbNonce,
                                                 auth->pbAuthData, auth->cbAuthData,
                                                 input, output,
                                                 auth->pbTag, auth->cbTag) != 0)
                    return STATUS_INTERNAL_ERROR;

                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "chaining mode %s not supported\n", debugstr_w(key->chaining_mode) );
                return STATUS_NOT_IMPLEMENTED;
            }

            /* update caller's IV to last ciphertext block (CBC/ECB) */
            if (iv && ivlen >= blk && required > 0)
                memcpy( iv, output + required - blk, blk );
        }

        return STATUS_SUCCESS;
    }
#else
    FIXME( "BCryptEncrypt not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS WINAPI BCryptDecrypt( BCRYPT_KEY_HANDLE handle, UCHAR *input, ULONG inputlen,
                                VOID *padding, UCHAR *iv, ULONG ivlen,
                                UCHAR *output, ULONG outputlen, ULONG *retlen, ULONG flags )
{
    TRACE( "%p, %p, %u, %p, %p, %u, %p, %u, %p, %08x\n",
           handle, input, inputlen, padding, iv, ivlen, output, outputlen, retlen, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct object *obj = (struct object *)handle;
        struct key *key;
        BOOL use_padding;
        UCHAR iv_buf[AES_BLOCK_SIZE] = {0};
        UCHAR *plain;
        ULONG plain_len;

        if (!obj) return STATUS_INVALID_HANDLE;

        /* RSA asymmetric decryption using private key */
        if (obj->magic == MAGIC_KPAIR)
        {
            struct key_pair *kp = (struct key_pair *)obj;
            size_t olen = 0;
            ULONG key_bytes;
            int ret;

            if (kp->alg_id != ALG_ID_RSA) return STATUS_NOT_IMPLEMENTED;
            if (!kp->finalized || !kp->has_private) return STATUS_INVALID_HANDLE;
            if (!input || !inputlen) return STATUS_INVALID_PARAMETER;

            key_bytes = (ULONG)mbedtls_rsa_get_len( &kp->u.rsa );
            /* size estimate for query (without output) */
            if (!output)
            {
                if (retlen) *retlen = key_bytes;
                return STATUS_SUCCESS;
            }

            if (flags & BCRYPT_PAD_OAEP)
            {
                BCRYPT_OAEP_PADDING_INFO *oaep = (BCRYPT_OAEP_PADDING_INFO *)padding;
                mbedtls_md_type_t md = MBEDTLS_MD_SHA1;
                const unsigned char *label = NULL;
                size_t label_len = 0;

                if (oaep && oaep->pszAlgId)
                {
                    if      (!strcmpW( oaep->pszAlgId, BCRYPT_SHA256_ALGORITHM )) md = MBEDTLS_MD_SHA256;
                    else if (!strcmpW( oaep->pszAlgId, BCRYPT_SHA384_ALGORITHM )) md = MBEDTLS_MD_SHA384;
                    else if (!strcmpW( oaep->pszAlgId, BCRYPT_SHA512_ALGORITHM )) md = MBEDTLS_MD_SHA512;
                }
                if (oaep) { label = oaep->pbLabel; label_len = oaep->cbLabel; }
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V21, md );
                ret = mbedtls_rsa_rsaes_oaep_decrypt( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PRIVATE, label, label_len,
                    &olen, input, output, outputlen );
            }
            else if (flags & BCRYPT_PAD_PKCS1)
            {
                mbedtls_rsa_set_padding( &kp->u.rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE );
                ret = mbedtls_rsa_rsaes_pkcs1_v15_decrypt( &kp->u.rsa, rng_cb, NULL,
                    MBEDTLS_RSA_PRIVATE, &olen, input, output, outputlen );
            }
            else
            {
                FIXME( "unsupported RSA padding flags %08x\n", flags );
                return STATUS_NOT_SUPPORTED;
            }

            if (ret != 0)
            {
                WARN( "RSA decrypt failed: -0x%04x\n", -ret );
                return STATUS_UNSUCCESSFUL;
            }
            if (retlen) *retlen = (ULONG)olen;
            return STATUS_SUCCESS;
        }

        key = (struct key *)obj;
        use_padding = (flags & BCRYPT_BLOCK_PADDING) != 0;

        if (key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;
        if (key->alg_id != ALG_ID_AES && key->alg_id != ALG_ID_3DES)
        {
            FIXME( "algorithm %u not supported for decrypt\n", key->alg_id );
            return STATUS_NOT_IMPLEMENTED;
        }
        if (!input || !inputlen) return STATUS_INVALID_PARAMETER;

        {
            ULONG blk = key->block_size;

            if (inputlen % blk) return STATUS_INVALID_BUFFER_SIZE;

            if (iv && ivlen >= blk) memcpy( iv_buf, iv, blk );

            /* decrypt into a temporary buffer when padding removal is needed */
            if (use_padding)
            {
                plain = HeapAlloc( GetProcessHeap(), 0, inputlen );
                if (!plain) return STATUS_NO_MEMORY;
            }
            else
            {
                if (retlen) *retlen = inputlen;
                if (!output) return STATUS_SUCCESS;
                if (outputlen < inputlen) return STATUS_BUFFER_TOO_SMALL;
                plain = output;
            }

            if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CBC ))
            {
                if (key->alg_id == ALG_ID_AES)
                    mbedtls_aes_crypt_cbc( &key->aes_dec, MBEDTLS_AES_DECRYPT, inputlen,
                                           iv_buf, input, plain );
                else
                    mbedtls_des3_crypt_cbc( &key->des3_dec, MBEDTLS_DES_DECRYPT, inputlen,
                                            iv_buf, input, plain );
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_ECB ))
            {
                ULONG i;
                if (key->alg_id == ALG_ID_AES)
                {
                    for (i = 0; i < inputlen; i += blk)
                        mbedtls_aes_crypt_ecb( &key->aes_dec, MBEDTLS_AES_DECRYPT, input + i, plain + i );
                }
                else
                {
                    for (i = 0; i < inputlen; i += blk)
                        mbedtls_des3_crypt_ecb( &key->des3_dec, input + i, plain + i );
                }
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CFB ))
            {
                size_t iv_offset = 0;
                if (key->alg_id != ALG_ID_AES) { if (use_padding) HeapFree(GetProcessHeap(), 0, plain); return STATUS_NOT_IMPLEMENTED; }
                if (!iv || ivlen < AES_BLOCK_SIZE) { if (use_padding) HeapFree(GetProcessHeap(), 0, plain); return STATUS_INVALID_PARAMETER; }
                if (retlen) *retlen = inputlen;
                if (!output) { if (use_padding) HeapFree(GetProcessHeap(), 0, plain); return STATUS_SUCCESS; }
                if (outputlen < inputlen) { if (use_padding) HeapFree(GetProcessHeap(), 0, plain); return STATUS_BUFFER_TOO_SMALL; }
                /* CFB128 decryption: save last ciphertext block for IV update before decrypting */
                UCHAR next_iv[AES_BLOCK_SIZE];
                if (inputlen >= AES_BLOCK_SIZE)
                    memcpy(next_iv, input + inputlen - AES_BLOCK_SIZE, AES_BLOCK_SIZE);
                mbedtls_aes_crypt_cfb128(&key->aes_enc, MBEDTLS_AES_DECRYPT, inputlen,
                                          &iv_offset, iv_buf, input, output);
                if (inputlen >= AES_BLOCK_SIZE)
                    memcpy(iv, next_iv, AES_BLOCK_SIZE);
                if (use_padding) HeapFree(GetProcessHeap(), 0, plain);
                return STATUS_SUCCESS;
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_GCM ))
            {
                BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth = (BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *)padding;
                if (!auth) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_INVALID_PARAMETER; }
                if (key->alg_id != ALG_ID_AES) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_NOT_IMPLEMENTED; }

                if (!key->gcm_key_set)
                {
                    mbedtls_gcm_free(&key->gcm);
                    mbedtls_gcm_init(&key->gcm);
                    if (mbedtls_gcm_setkey(&key->gcm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) != 0)
                    {
                        if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                        return STATUS_INTERNAL_ERROR;
                    }
                    key->gcm_key_set = TRUE;
                }

                if (retlen) *retlen = inputlen;
                if (!output) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_SUCCESS; }
                if (outputlen < inputlen) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_BUFFER_TOO_SMALL; }

                if (mbedtls_gcm_auth_decrypt(&key->gcm, inputlen,
                                              auth->pbNonce, auth->cbNonce,
                                              auth->pbAuthData, auth->cbAuthData,
                                              auth->pbTag, auth->cbTag,
                                              input, output) != 0)
                {
                    if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                    return STATUS_UNSUCCESSFUL;
                }
                if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                return STATUS_SUCCESS;
            }
            else if (!strcmpW( key->chaining_mode, BCRYPT_CHAIN_MODE_CCM ))
            {
                BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *auth = (BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO *)padding;
                if (!auth) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_INVALID_PARAMETER; }
                if (key->alg_id != ALG_ID_AES) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_NOT_IMPLEMENTED; }

                if (!key->ccm_key_set)
                {
                    mbedtls_ccm_free(&key->ccm);
                    mbedtls_ccm_init(&key->ccm);
                    if (mbedtls_ccm_setkey(&key->ccm, MBEDTLS_CIPHER_ID_AES,
                                            key->key_data, key->key_len * 8) != 0)
                    {
                        if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                        return STATUS_INTERNAL_ERROR;
                    }
                    key->ccm_key_set = TRUE;
                }

                if (retlen) *retlen = inputlen;
                if (!output) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_SUCCESS; }
                if (outputlen < inputlen) { if (use_padding) HeapFree( GetProcessHeap(), 0, plain ); return STATUS_BUFFER_TOO_SMALL; }

                if (mbedtls_ccm_auth_decrypt(&key->ccm, inputlen,
                                              auth->pbNonce, auth->cbNonce,
                                              auth->pbAuthData, auth->cbAuthData,
                                              input, output,
                                              auth->pbTag, auth->cbTag) != 0)
                {
                    if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                    return STATUS_UNSUCCESSFUL;
                }
                if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                return STATUS_SUCCESS;
            }
            else
            {
                FIXME( "chaining mode %s not supported\n", debugstr_w(key->chaining_mode) );
                if (use_padding) HeapFree( GetProcessHeap(), 0, plain );
                return STATUS_NOT_IMPLEMENTED;
            }

            if (use_padding)
            {
                UCHAR pad_val = plain[inputlen - 1];
                ULONG i;

                if (pad_val == 0 || pad_val > blk)
                {
                    HeapFree( GetProcessHeap(), 0, plain );
                    return STATUS_UNSUCCESSFUL;
                }
                for (i = inputlen - pad_val; i < inputlen; i++)
                {
                    if (plain[i] != pad_val)
                    {
                        HeapFree( GetProcessHeap(), 0, plain );
                        return STATUS_UNSUCCESSFUL;
                    }
                }
                plain_len = inputlen - pad_val;

                if (retlen) *retlen = plain_len;
                if (!output)
                {
                    HeapFree( GetProcessHeap(), 0, plain );
                    return STATUS_SUCCESS;
                }
                if (outputlen < plain_len)
                {
                    HeapFree( GetProcessHeap(), 0, plain );
                    return STATUS_BUFFER_TOO_SMALL;
                }
                memcpy( output, plain, plain_len );
                HeapFree( GetProcessHeap(), 0, plain );
            }

            /* update caller's IV to last ciphertext block */
            if (iv && ivlen >= blk)
                memcpy( iv, input + inputlen - blk, blk );
        }

        return STATUS_SUCCESS;
    }
#else
    FIXME( "BCryptDecrypt not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS WINAPI BCryptImportKey( BCRYPT_ALG_HANDLE algorithm, BCRYPT_KEY_HANDLE decrypt_key,
                                  LPCWSTR type, BCRYPT_KEY_HANDLE *handle,
                                  UCHAR *object, ULONG objectlen,
                                  UCHAR *input, ULONG inputlen, ULONG flags )
{
    struct algorithm *alg = algorithm;

    TRACE( "%p, %p, %s, %p, %p, %u, %p, %u, %08x\n", algorithm, decrypt_key,
           debugstr_w(type), handle, object, objectlen, input, inputlen, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!type || !handle || !input) return STATUS_INVALID_PARAMETER;
    if (decrypt_key)
    {
        FIXME( "decrypt_key not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( type, BCRYPT_KEY_DATA_BLOB ))
    {
#ifdef SONAME_LIBMBEDTLS
        const BCRYPT_KEY_DATA_BLOB_HEADER *hdr = (const BCRYPT_KEY_DATA_BLOB_HEADER *)input;
        struct key *key;
        NTSTATUS status;

        if (inputlen < sizeof(*hdr)) return STATUS_INVALID_PARAMETER;
        if (hdr->dwMagic   != BCRYPT_KEY_DATA_BLOB_MAGIC)   return STATUS_INVALID_PARAMETER;
        if (hdr->dwVersion != BCRYPT_KEY_DATA_BLOB_VERSION1) return STATUS_NOT_SUPPORTED;
        if (inputlen < sizeof(*hdr) + hdr->cbKeyData)        return STATUS_INVALID_PARAMETER;

        if (!(key = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*key) )))
            return STATUS_NO_MEMORY;

        status = key_init( key, alg->id, (const UCHAR *)(hdr + 1), hdr->cbKeyData );
        if (status != STATUS_SUCCESS)
        {
            HeapFree( GetProcessHeap(), 0, key );
            return status;
        }
        *handle = key;
        return STATUS_SUCCESS;
#else
        FIXME( "BCRYPT_KEY_DATA_BLOB not supported without mbedTLS\n" );
        return STATUS_NOT_IMPLEMENTED;
#endif
    }

    FIXME( "key blob type %s not supported\n", debugstr_w(type) );
    return STATUS_NOT_IMPLEMENTED;
}

#ifdef SONAME_LIBMBEDTLS
static NTSTATUS export_rsa_key( struct key_pair *kp, LPCWSTR type,
                                  UCHAR *output, ULONG outputlen, ULONG *retlen );
static NTSTATUS export_ecc_key( struct key_pair *kp, LPCWSTR type,
                                  UCHAR *output, ULONG outputlen, ULONG *retlen );
#endif

NTSTATUS WINAPI BCryptExportKey( BCRYPT_KEY_HANDLE handle, BCRYPT_KEY_HANDLE encrypt_key,
                                  LPCWSTR type, UCHAR *output, ULONG outputlen,
                                  ULONG *retlen, ULONG flags )
{
    TRACE( "%p, %p, %s, %p, %u, %p, %08x\n", handle, encrypt_key,
           debugstr_w(type), output, outputlen, retlen, flags );

    if (encrypt_key)
    {
        FIXME( "encrypt_key not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

#ifdef SONAME_LIBMBEDTLS
    {
        struct object *obj = handle;
        if (!obj) return STATUS_INVALID_HANDLE;

        /* Dispatch to appropriate export for key pair handles */
        if (obj->magic == MAGIC_KPAIR)
        {
            struct key_pair *kp = (struct key_pair *)obj;
            if (!strcmpW( type, BCRYPT_RSAPUBLIC_BLOB ) || !strcmpW( type, BCRYPT_RSAPRIVATE_BLOB ))
                return export_rsa_key( kp, type, output, outputlen, retlen );
            if (!strcmpW( type, BCRYPT_ECCPUBLIC_BLOB ) || !strcmpW( type, BCRYPT_ECCPRIVATE_BLOB ))
                return export_ecc_key( kp, type, output, outputlen, retlen );
            FIXME( "unsupported key pair export type %s\n", debugstr_w(type) );
            return STATUS_NOT_IMPLEMENTED;
        }

        {
            struct key *key = (struct key *)obj;
            if (!key || key->hdr.magic != MAGIC_KEY) return STATUS_INVALID_HANDLE;

            if (!strcmpW( type, BCRYPT_KEY_DATA_BLOB ))
            {
                ULONG required = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key->key_len;
                BCRYPT_KEY_DATA_BLOB_HEADER *hdr = (BCRYPT_KEY_DATA_BLOB_HEADER *)output;

                if (retlen) *retlen = required;
                if (!output) return STATUS_SUCCESS;
                if (outputlen < required) return STATUS_BUFFER_TOO_SMALL;

                hdr->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
                hdr->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
                hdr->cbKeyData = key->key_len;
                memcpy( hdr + 1, key->key_data, key->key_len );
                return STATUS_SUCCESS;
            }

            FIXME( "key blob type %s not supported\n", debugstr_w(type) );
            return STATUS_NOT_IMPLEMENTED;
        }
    }
#else
    return STATUS_INVALID_HANDLE;
#endif
}

/* BCryptExportKey extension for RSA key pairs (called from BCryptExportKey dispatch below) */
#ifdef SONAME_LIBMBEDTLS
static NTSTATUS export_rsa_key( struct key_pair *kp, LPCWSTR type,
                                  UCHAR *output, ULONG outputlen, ULONG *retlen )
{
    BCRYPT_RSAKEY_BLOB *hdr;
    mbedtls_mpi N, E, P, Q;
    ULONG cbE, cbN, cbP = 0, cbQ = 0, required;
    BOOL is_public = !strcmpW( type, BCRYPT_RSAPUBLIC_BLOB );
    UCHAR *p;

    if (!is_public && !kp->has_private) return STATUS_INVALID_PARAMETER;

    mbedtls_mpi_init(&N); mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&P); mbedtls_mpi_init(&Q);

    mbedtls_rsa_export( &kp->u.rsa, &N, is_public ? NULL : &P,
                         is_public ? NULL : &Q, NULL, &E );

    cbE = (ULONG)mbedtls_mpi_size(&E);
    cbN = (ULONG)mbedtls_mpi_size(&N);
    if (!is_public)
    {
        cbP = (ULONG)mbedtls_mpi_size(&P);
        cbQ = (ULONG)mbedtls_mpi_size(&Q);
    }

    required = sizeof(*hdr) + cbE + cbN + cbP + cbQ;
    if (retlen) *retlen = required;
    if (!output)
    {
        mbedtls_mpi_free(&N); mbedtls_mpi_free(&E);
        mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
        return STATUS_SUCCESS;
    }
    if (outputlen < required)
    {
        mbedtls_mpi_free(&N); mbedtls_mpi_free(&E);
        mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
        return STATUS_BUFFER_TOO_SMALL;
    }

    hdr = (BCRYPT_RSAKEY_BLOB *)output;
    hdr->Magic       = is_public ? BCRYPT_RSAPUBLIC_MAGIC : BCRYPT_RSAPRIVATE_MAGIC;
    hdr->BitLength   = kp->bitlen;
    hdr->cbPublicExp = cbE;
    hdr->cbModulus   = cbN;
    hdr->cbPrime1    = cbP;
    hdr->cbPrime2    = cbQ;
    p = (UCHAR *)(hdr + 1);

    mbedtls_mpi_write_binary(&E, p, cbE); p += cbE;
    mbedtls_mpi_write_binary(&N, p, cbN); p += cbN;
    if (!is_public)
    {
        mbedtls_mpi_write_binary(&P, p, cbP); p += cbP;
        mbedtls_mpi_write_binary(&Q, p, cbQ);
    }

    mbedtls_mpi_free(&N); mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&P); mbedtls_mpi_free(&Q);
    return STATUS_SUCCESS;
}
#endif

NTSTATUS WINAPI BCryptSecretAgreement( BCRYPT_KEY_HANDLE priv_handle, BCRYPT_KEY_HANDLE pub_handle,
                                        BCRYPT_SECRET_HANDLE *secret_handle, ULONG flags )
{
    TRACE( "%p, %p, %p, %08x\n", priv_handle, pub_handle, secret_handle, flags );

    if (!secret_handle) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    {
        struct key_pair *priv = priv_handle;
        struct key_pair *pub  = pub_handle;
        struct secret *sec;
        mbedtls_mpi z;
        size_t secret_len;
        int ret;

        if (!priv || priv->hdr.magic != MAGIC_KPAIR) return STATUS_INVALID_HANDLE;
        if (!pub  || pub->hdr.magic  != MAGIC_KPAIR) return STATUS_INVALID_HANDLE;
        if (!priv->has_private) return STATUS_INVALID_PARAMETER;
        if (priv->alg_id != pub->alg_id) return STATUS_INVALID_PARAMETER;

        if (priv->alg_id != ALG_ID_ECDH_P256 && priv->alg_id != ALG_ID_ECDH_P384)
        {
            FIXME( "BCryptSecretAgreement not implemented for algorithm %u\n", priv->alg_id );
            return STATUS_NOT_IMPLEMENTED;
        }

        sec = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sec) );
        if (!sec) return STATUS_NO_MEMORY;

        sec->hdr.magic = MAGIC_SECRET;
        sec->alg_id    = priv->alg_id;
        secret_len     = (priv->alg_id == ALG_ID_ECDH_P256) ? 32 : 48;

        mbedtls_mpi_init( &z );
        ret = mbedtls_ecdh_compute_shared( &priv->u.ecdh.grp, &z,
                                            &pub->u.ecdh.Q, &priv->u.ecdh.d,
                                            rng_cb, NULL );
        if (ret != 0)
        {
            mbedtls_mpi_free( &z );
            HeapFree( GetProcessHeap(), 0, sec );
            return STATUS_INTERNAL_ERROR;
        }

        mbedtls_mpi_write_binary( &z, sec->secret, secret_len );
        sec->secret_len = (ULONG)secret_len;
        mbedtls_mpi_free( &z );

        *secret_handle = sec;
        return STATUS_SUCCESS;
    }
#endif
    FIXME( "BCryptSecretAgreement not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptDeriveKey( BCRYPT_SECRET_HANDLE secret_handle, LPCWSTR kdf,
                                   BCryptBufferDesc *params, UCHAR *derived, ULONG derived_len,
                                   ULONG *result_len, ULONG flags )
{
    TRACE( "%p, %s, %p, %p, %u, %p, %08x\n",
           secret_handle, wine_dbgstr_w(kdf), params, derived, derived_len, result_len, flags );

#ifdef SONAME_LIBMBEDTLS
    {
        struct secret *sec = secret_handle;
        if (!sec || sec->hdr.magic != MAGIC_SECRET) return STATUS_INVALID_HANDLE;
        if (!kdf) return STATUS_INVALID_PARAMETER;

        if (!strcmpW( kdf, BCRYPT_KDF_RAW_SECRET ))
        {
            /* Return the raw shared secret */
            if (result_len) *result_len = sec->secret_len;
            if (!derived) return STATUS_SUCCESS;
            if (derived_len < sec->secret_len) return STATUS_BUFFER_TOO_SMALL;
            memcpy( derived, sec->secret, sec->secret_len );
            return STATUS_SUCCESS;
        }

        if (!strcmpW( kdf, BCRYPT_KDF_HASH ))
        {
            /* Hash-based KDF: hash the shared secret */
            mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256; /* default */
            const mbedtls_md_info_t *md_info;
            mbedtls_md_context_t md_ctx;
            ULONG hash_len;

            /* Check params for KDF_HASH_ALGORITHM buffer */
            if (params)
            {
                ULONG i;
                for (i = 0; i < params->cBuffers; i++)
                {
                    if (params->pBuffers[i].BufferType == KDF_HASH_ALGORITHM && params->pBuffers[i].pvBuffer)
                    {
                        const WCHAR *alg_name = (const WCHAR *)params->pBuffers[i].pvBuffer;
                        if      (!strcmpW( alg_name, BCRYPT_SHA1_ALGORITHM   )) md_type = MBEDTLS_MD_SHA1;
                        else if (!strcmpW( alg_name, BCRYPT_SHA224_ALGORITHM )) md_type = MBEDTLS_MD_SHA224;
                        else if (!strcmpW( alg_name, BCRYPT_SHA256_ALGORITHM )) md_type = MBEDTLS_MD_SHA256;
                        else if (!strcmpW( alg_name, BCRYPT_SHA384_ALGORITHM )) md_type = MBEDTLS_MD_SHA384;
                        else if (!strcmpW( alg_name, BCRYPT_SHA512_ALGORITHM )) md_type = MBEDTLS_MD_SHA512;
                        else if (!strcmpW( alg_name, BCRYPT_MD5_ALGORITHM    )) md_type = MBEDTLS_MD_MD5;
                    }
                }
            }

            md_info = mbedtls_md_info_from_type( md_type );
            if (!md_info) return STATUS_NOT_IMPLEMENTED;

            hash_len = (ULONG)mbedtls_md_get_size( md_info );
            if (result_len) *result_len = hash_len;
            if (!derived) return STATUS_SUCCESS;
            if (derived_len < hash_len) return STATUS_BUFFER_TOO_SMALL;

            mbedtls_md_init( &md_ctx );
            if (mbedtls_md_setup( &md_ctx, md_info, 0 ) != 0)
            {
                mbedtls_md_free( &md_ctx );
                return STATUS_INTERNAL_ERROR;
            }
            mbedtls_md_starts( &md_ctx );
            mbedtls_md_update( &md_ctx, sec->secret, sec->secret_len );
            mbedtls_md_finish( &md_ctx, derived );
            mbedtls_md_free( &md_ctx );

            return STATUS_SUCCESS;
        }

        if (!strcmpW( kdf, BCRYPT_KDF_HMAC ))
        {
            /* HMAC-based KDF: HMAC(key, secret) */
            mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
            const mbedtls_md_info_t *md_info;
            mbedtls_md_context_t md_ctx;
            const UCHAR *hmac_key = NULL;
            ULONG hmac_key_len = 0;
            ULONG hash_len;

            if (params)
            {
                ULONG i;
                for (i = 0; i < params->cBuffers; i++)
                {
                    if (params->pBuffers[i].BufferType == KDF_HASH_ALGORITHM && params->pBuffers[i].pvBuffer)
                    {
                        const WCHAR *alg_name = (const WCHAR *)params->pBuffers[i].pvBuffer;
                        if      (!strcmpW( alg_name, BCRYPT_SHA1_ALGORITHM   )) md_type = MBEDTLS_MD_SHA1;
                        else if (!strcmpW( alg_name, BCRYPT_SHA224_ALGORITHM )) md_type = MBEDTLS_MD_SHA224;
                        else if (!strcmpW( alg_name, BCRYPT_SHA256_ALGORITHM )) md_type = MBEDTLS_MD_SHA256;
                        else if (!strcmpW( alg_name, BCRYPT_SHA384_ALGORITHM )) md_type = MBEDTLS_MD_SHA384;
                        else if (!strcmpW( alg_name, BCRYPT_SHA512_ALGORITHM )) md_type = MBEDTLS_MD_SHA512;
                        else if (!strcmpW( alg_name, BCRYPT_MD5_ALGORITHM    )) md_type = MBEDTLS_MD_MD5;
                    }
                    else if (params->pBuffers[i].BufferType == KDF_HMAC_KEY && params->pBuffers[i].pvBuffer)
                    {
                        hmac_key     = (const UCHAR *)params->pBuffers[i].pvBuffer;
                        hmac_key_len = params->pBuffers[i].cbBuffer;
                    }
                }
            }

            md_info = mbedtls_md_info_from_type( md_type );
            if (!md_info) return STATUS_NOT_IMPLEMENTED;

            hash_len = (ULONG)mbedtls_md_get_size( md_info );
            if (result_len) *result_len = hash_len;
            if (!derived) return STATUS_SUCCESS;
            if (derived_len < hash_len) return STATUS_BUFFER_TOO_SMALL;

            /* Use shared secret as HMAC key if no explicit key provided */
            if (!hmac_key) { hmac_key = sec->secret; hmac_key_len = sec->secret_len; }

            mbedtls_md_init( &md_ctx );
            if (mbedtls_md_setup( &md_ctx, md_info, 1 /* hmac */ ) != 0)
            {
                mbedtls_md_free( &md_ctx );
                return STATUS_INTERNAL_ERROR;
            }
            mbedtls_md_hmac_starts( &md_ctx, hmac_key, hmac_key_len );
            mbedtls_md_hmac_update( &md_ctx, sec->secret, sec->secret_len );
            mbedtls_md_hmac_finish( &md_ctx, derived );
            mbedtls_md_free( &md_ctx );

            return STATUS_SUCCESS;
        }

        FIXME( "KDF %s not supported\n", wine_dbgstr_w(kdf) );
        return STATUS_NOT_IMPLEMENTED;
    }
#endif
    FIXME( "BCryptDeriveKey not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI BCryptDeriveKeyPBKDF2( BCRYPT_ALG_HANDLE algorithm, PUCHAR password, ULONG password_len,
                                        PUCHAR salt, ULONG salt_len, ULONGLONG iterations,
                                        PUCHAR key, ULONG key_len, ULONG flags )
{
    struct algorithm *alg = algorithm;

    TRACE( "%p, %p, %u, %p, %u, %I64u, %p, %u, %08x\n",
           algorithm, password, password_len, salt, salt_len, iterations, key, key_len, flags );

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (!key || !key_len) return STATUS_INVALID_PARAMETER;

#ifdef SONAME_LIBMBEDTLS
    {
        const mbedtls_md_info_t *md_info;
        mbedtls_md_type_t md_type;
        ULONG hash_len, block_count, i, j;
        UCHAR *T = NULL, *U = NULL, *salt_int = NULL;
        ULONG salt_int_len;
        NTSTATUS status = STATUS_SUCCESS;

        switch (alg->id)
        {
        case ALG_ID_MD5:    md_type = MBEDTLS_MD_MD5;    break;
        case ALG_ID_SHA1:   md_type = MBEDTLS_MD_SHA1;   break;
        case ALG_ID_SHA224: md_type = MBEDTLS_MD_SHA224; break;
        case ALG_ID_SHA256: md_type = MBEDTLS_MD_SHA256; break;
        case ALG_ID_SHA384: md_type = MBEDTLS_MD_SHA384; break;
        case ALG_ID_SHA512: md_type = MBEDTLS_MD_SHA512; break;
        default:
            FIXME( "unsupported algorithm %u for PBKDF2\n", alg->id );
            return STATUS_NOT_IMPLEMENTED;
        }

        md_info = mbedtls_md_info_from_type( md_type );
        if (!md_info) return STATUS_NOT_IMPLEMENTED;

        hash_len = (ULONG)mbedtls_md_get_size( md_info );
        block_count = (key_len + hash_len - 1) / hash_len;

        T = HeapAlloc( GetProcessHeap(), 0, hash_len );
        U = HeapAlloc( GetProcessHeap(), 0, hash_len );
        /* salt || INT(i): 4 extra bytes for the block counter */
        salt_int_len = salt_len + 4;
        salt_int = HeapAlloc( GetProcessHeap(), 0, salt_int_len );

        if (!T || !U || !salt_int)
        {
            status = STATUS_NO_MEMORY;
            goto pbkdf2_done;
        }

        if (salt && salt_len) memcpy( salt_int, salt, salt_len );

        for (i = 1; i <= block_count; i++)
        {
            ULONGLONG it;
            ULONG copy_len;

            /* Append big-endian block index */
            salt_int[salt_len    ] = (UCHAR)((i >> 24) & 0xFF);
            salt_int[salt_len + 1] = (UCHAR)((i >> 16) & 0xFF);
            salt_int[salt_len + 2] = (UCHAR)((i >>  8) & 0xFF);
            salt_int[salt_len + 3] = (UCHAR)( i        & 0xFF);

            /* U_1 = HMAC(password, salt || INT(i)) */
            if (mbedtls_md_hmac( md_info, password, password_len,
                                  salt_int, salt_int_len, U ) != 0)
            {
                status = STATUS_INTERNAL_ERROR;
                goto pbkdf2_done;
            }
            memcpy( T, U, hash_len );

            /* U_2 .. U_c */
            for (it = 1; it < iterations; it++)
            {
                ULONG k;
                if (mbedtls_md_hmac( md_info, password, password_len, U, hash_len, U ) != 0)
                {
                    status = STATUS_INTERNAL_ERROR;
                    goto pbkdf2_done;
                }
                for (k = 0; k < hash_len; k++)
                    T[k] ^= U[k];
            }

            /* Copy T_i into output */
            copy_len = min( hash_len, key_len - (i - 1) * hash_len );
            memcpy( key + (i - 1) * hash_len, T, copy_len );
        }

pbkdf2_done:
        if (T) HeapFree( GetProcessHeap(), 0, T );
        if (U) HeapFree( GetProcessHeap(), 0, U );
        if (salt_int) HeapFree( GetProcessHeap(), 0, salt_int );
        return status;
    }
#else
    FIXME( "BCryptDeriveKeyPBKDF2 not supported without mbedTLS\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS WINAPI BCryptDestroySecret( BCRYPT_SECRET_HANDLE handle )
{
    struct secret *sec = handle;

    TRACE( "%p\n", handle );

    if (!sec || sec->hdr.magic != MAGIC_SECRET) return STATUS_INVALID_HANDLE;
    sec->hdr.magic = 0;
    /* Wipe secret material */
    SecureZeroMemory( sec->secret, sizeof(sec->secret) );
    HeapFree( GetProcessHeap(), 0, sec );
    return STATUS_SUCCESS;
}

/* ECDH ECC key export helper */
#ifdef SONAME_LIBMBEDTLS
static NTSTATUS export_ecc_key( struct key_pair *kp, LPCWSTR type,
                                  UCHAR *output, ULONG outputlen, ULONG *retlen )
{
    BCRYPT_ECCKEY_BLOB *hdr;
    ULONG coord_size, pub_magic, priv_magic, required;
    BOOL is_public = !strcmpW( type, BCRYPT_ECCPUBLIC_BLOB );
    UCHAR point_buf[1 + 2 * 48];
    size_t point_len = 0;
    UCHAR *p;

    if (kp->alg_id == ALG_ID_ECDH_P256)
    {
        coord_size  = 32;
        pub_magic   = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
        priv_magic  = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
    }
    else if (kp->alg_id == ALG_ID_ECDH_P384)
    {
        coord_size  = 48;
        pub_magic   = BCRYPT_ECDH_PUBLIC_P384_MAGIC;
        priv_magic  = BCRYPT_ECDH_PRIVATE_P384_MAGIC;
    }
    else if (kp->alg_id == ALG_ID_ECDSA_P256)
    {
        coord_size  = 32;
        pub_magic   = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
        priv_magic  = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    }
    else if (kp->alg_id == ALG_ID_ECDSA_P384)
    {
        coord_size  = 48;
        pub_magic   = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
        priv_magic  = BCRYPT_ECDSA_PRIVATE_P384_MAGIC;
    }
    else
    {
        FIXME( "unsupported ECC algorithm %u for export\n", kp->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!is_public && !kp->has_private) return STATUS_INVALID_PARAMETER;

    /* Export public point as uncompressed — use ecdsa or ecdh field as appropriate */
    {
        mbedtls_ecp_group *grp;
        mbedtls_ecp_point *Q;
        mbedtls_mpi       *d;

        if (kp->alg_id == ALG_ID_ECDSA_P256 || kp->alg_id == ALG_ID_ECDSA_P384)
        {
            grp = &kp->u.ecdsa.grp;
            Q   = &kp->u.ecdsa.Q;
            d   = &kp->u.ecdsa.d;
        }
        else
        {
            grp = &kp->u.ecdh.grp;
            Q   = &kp->u.ecdh.Q;
            d   = &kp->u.ecdh.d;
        }

        if (mbedtls_ecp_point_write_binary( grp, Q,
                                             MBEDTLS_ECP_PF_UNCOMPRESSED,
                                             &point_len, point_buf, sizeof(point_buf) ) != 0)
            return STATUS_INTERNAL_ERROR;

        required = sizeof(*hdr) + 2 * coord_size + (is_public ? 0 : coord_size);
        if (retlen) *retlen = required;
        if (!output) return STATUS_SUCCESS;
        if (outputlen < required) return STATUS_BUFFER_TOO_SMALL;

        hdr = (BCRYPT_ECCKEY_BLOB *)output;
        hdr->dwMagic = is_public ? pub_magic : priv_magic;
        hdr->cbKey   = coord_size;
        p = (UCHAR *)(hdr + 1);

        /* X then Y (skip the 0x04 prefix byte) */
        memcpy( p,              point_buf + 1,              coord_size );
        memcpy( p + coord_size, point_buf + 1 + coord_size, coord_size );
        p += 2 * coord_size;

        if (!is_public)
        {
            if (mbedtls_mpi_write_binary( d, p, coord_size ) != 0)
                return STATUS_INTERNAL_ERROR;
        }
    }

    return STATUS_SUCCESS;
}
#endif

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
#if defined(HAVE_GNUTLS_HASH) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H)
        gnutls_initialize();
#elif defined(SONAME_LIBMBEDTLS) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H) && !defined(__REACTOS__)
        mbedtls_initialize();
#endif
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
#if defined(HAVE_GNUTLS_HASH) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H)
        gnutls_uninitialize();
#elif defined(SONAME_LIBMBEDTLS) && !defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H) && !defined(__REACTOS__)
        mbedtls_uninitialize();
#endif
        break;
    }
    return TRUE;
}
