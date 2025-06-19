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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static HINSTANCE instance;

NTSTATUS WINAPI BCryptEnumAlgorithms(ULONG dwAlgOperations, ULONG *pAlgCount,
                                     BCRYPT_ALGORITHM_IDENTIFIER **ppAlgList, ULONG dwFlags)
{
    FIXME("%08x, %p, %p, %08x - stub\n", dwAlgOperations, pAlgCount, ppAlgList, dwFlags);

    *ppAlgList=NULL;
    *pAlgCount=0;

    return STATUS_NOT_IMPLEMENTED;
}

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
    hash->hmac      = alg->hmac;

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

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
        process_attach( NULL );
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        process_detach( NULL );
        break;
    }
    return TRUE;
}
