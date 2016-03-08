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
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
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

NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE algorithm, UCHAR *buffer, ULONG count, ULONG flags)
{
    const DWORD supported_flags = BCRYPT_USE_SYSTEM_PREFERRED_RNG;
    TRACE("%p, %p, %u, %08x - semi-stub\n", algorithm, buffer, count, flags);

    if (!algorithm)
    {
        /* It's valid to call without an algorithm if BCRYPT_USE_SYSTEM_PREFERRED_RNG
         * is set. In this case the preferred system RNG is used.
         */
        if (!(flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG))
            return STATUS_INVALID_HANDLE;
    }
    if (!buffer)
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags)
        FIXME("unsupported flags %08x\n", flags & ~supported_flags);

    if (algorithm)
        FIXME("ignoring selected algorithm\n");

    /* When zero bytes are requested the function returns success too. */
    if (!count)
        return STATUS_SUCCESS;

    if (flags & BCRYPT_USE_SYSTEM_PREFERRED_RNG)
    {
        if (RtlGenRandom(buffer, count))
            return STATUS_SUCCESS;
    }

    FIXME("called with unsupported parameters, returning error\n");
    return STATUS_NOT_IMPLEMENTED;
}

#define MAGIC_ALG  (('A' << 24) | ('L' << 16) | ('G' << 8) | '0')
#define MAGIC_HASH (('H' << 24) | ('A' << 16) | ('S' << 8) | 'H')
struct object
{
    ULONG magic;
};

enum alg_id
{
    ALG_ID_SHA1,
    ALG_ID_SHA256,
    ALG_ID_SHA384,
    ALG_ID_SHA512
};

static const struct {
    ULONG hash_length;
    const WCHAR *alg_name;
} alg_props[] = {
    /* ALG_ID_SHA1   */ { 20, BCRYPT_SHA1_ALGORITHM },
    /* ALG_ID_SHA256 */ { 32, BCRYPT_SHA256_ALGORITHM },
    /* ALG_ID_SHA384 */ { 48, BCRYPT_SHA384_ALGORITHM },
    /* ALG_ID_SHA512 */ { 64, BCRYPT_SHA512_ALGORITHM }
};

struct algorithm
{
    struct object hdr;
    enum alg_id   id;
};

NTSTATUS WINAPI BCryptOpenAlgorithmProvider( BCRYPT_ALG_HANDLE *handle, LPCWSTR id, LPCWSTR implementation, DWORD flags )
{
    struct algorithm *alg;
    enum alg_id alg_id;

    TRACE( "%p, %s, %s, %08x\n", handle, wine_dbgstr_w(id), wine_dbgstr_w(implementation), flags );

    if (!handle || !id) return STATUS_INVALID_PARAMETER;
    if (flags)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!strcmpW( id, BCRYPT_SHA1_ALGORITHM )) alg_id = ALG_ID_SHA1;
    else if (!strcmpW( id, BCRYPT_SHA256_ALGORITHM )) alg_id = ALG_ID_SHA256;
    else if (!strcmpW( id, BCRYPT_SHA384_ALGORITHM )) alg_id = ALG_ID_SHA384;
    else if (!strcmpW( id, BCRYPT_SHA512_ALGORITHM )) alg_id = ALG_ID_SHA512;
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
    union
    {
        CC_SHA1_CTX   sha1_ctx;
        CC_SHA256_CTX sha256_ctx;
        CC_SHA512_CTX sha512_ctx;
    } u;
};

static NTSTATUS hash_init( struct hash *hash )
{
    switch (hash->alg_id)
    {
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

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
    switch (hash->alg_id)
    {
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

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    switch (hash->alg_id)
    {
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
#elif defined(HAVE_GNUTLS_HASH)
struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    gnutls_hash_hd_t handle;
};

static NTSTATUS hash_init( struct hash *hash )
{
    gnutls_digest_algorithm_t alg;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (hash->alg_id)
    {
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

    if (pgnutls_hash_init( &hash->handle, alg )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
    if (pgnutls_hash( hash->handle, input, size )) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    pgnutls_hash_deinit( hash->handle, output );
    return STATUS_SUCCESS;
}
#elif defined(SONAME_LIBMBEDTLS)
struct hash
{
    struct object    hdr;
    enum alg_id      alg_id;
    union
    {
        mbedtls_sha1_context   sha1_ctx;
        mbedtls_sha256_context sha256_ctx;
        mbedtls_sha512_context sha512_ctx;
    } u;
};

static NTSTATUS hash_init( struct hash *hash )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_SHA1:
        mbedtls_sha1_init(&hash->u.sha1_ctx);
        mbedtls_sha1_starts(&hash->u.sha1_ctx);
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

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_SHA1:
        mbedtls_sha1_update(&hash->u.sha1_ctx, input, size);
        break;

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

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    switch (hash->alg_id)
    {
    case ALG_ID_SHA1:
        mbedtls_sha1_finish(&hash->u.sha1_ctx, output);
        mbedtls_sha1_free(&hash->u.sha1_ctx);
        break;

    case ALG_ID_SHA256:
        mbedtls_sha256_finish(&hash->u.sha256_ctx, output);
        mbedtls_sha256_free(&hash->u.sha256_ctx);
        break;

    case ALG_ID_SHA384:
    case ALG_ID_SHA512:
        mbedtls_sha512_finish(&hash->u.sha512_ctx, output);
        mbedtls_sha512_free(&hash->u.sha512_ctx);
        break;

    default:
        ERR( "unhandled id %u\n", hash->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}
#else
struct hash
{
    struct object hdr;
    enum alg_id   alg_id;
};

static NTSTATUS hash_init( struct hash *hash )
{
    ERR( "support for hashes not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
    ERR( "support for hashes not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS hash_finish( struct hash *hash, UCHAR *output, ULONG size )
{
    ERR( "support for hashes not available at build time\n" );
    return STATUS_NOT_IMPLEMENTED;
}
#endif

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
    case ALG_ID_SHA1:
        if (!strcmpW( prop, BCRYPT_OBJECT_LENGTH ))
        {
            value = OBJECT_LENGTH_SHA1;
            break;
        }
        FIXME( "unsupported sha1 algorithm property %s\n", debugstr_w(prop) );
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

    TRACE( "%p, %p, %p, %u, %p, %u, %08x - stub\n", algorithm, handle, object, objectlen,
           secret, secretlen, flags );
    if (flags)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!alg || alg->hdr.magic != MAGIC_ALG) return STATUS_INVALID_HANDLE;
    if (object) FIXME( "ignoring object buffer\n" );

    if (!(hash = HeapAlloc( GetProcessHeap(), 0, sizeof(*hash) ))) return STATUS_NO_MEMORY;
    hash->hdr.magic = MAGIC_HASH;
    hash->alg_id    = alg->id;
    if ((status = hash_init( hash )) != STATUS_SUCCESS)
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
    HeapFree( GetProcessHeap(), 0, hash );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BCryptHashData( BCRYPT_HASH_HANDLE handle, UCHAR *input, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, input, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!input) return STATUS_INVALID_PARAMETER;

    return hash_update( hash, input, size );
}

NTSTATUS WINAPI BCryptFinishHash( BCRYPT_HASH_HANDLE handle, UCHAR *output, ULONG size, ULONG flags )
{
    struct hash *hash = handle;

    TRACE( "%p, %p, %u, %08x\n", handle, output, size, flags );

    if (!hash || hash->hdr.magic != MAGIC_HASH) return STATUS_INVALID_HANDLE;
    if (!output) return STATUS_INVALID_PARAMETER;

    return hash_finish( hash, output, size );
}

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
