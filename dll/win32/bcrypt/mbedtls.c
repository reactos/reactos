#include "precomp.h"

#ifdef SONAME_LIBMBEDTLS

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static void *libmbedtls_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(mbedtls_md_init);
MAKE_FUNCPTR(mbedtls_md_setup);
MAKE_FUNCPTR(mbedtls_md_update);
MAKE_FUNCPTR(mbedtls_md_starts);
MAKE_FUNCPTR(mbedtls_md_hmac_starts);
MAKE_FUNCPTR(mbedtls_md_finish);
MAKE_FUNCPTR(mbedtls_md_hmac_finish);
MAKE_FUNCPTR(mbedtls_md_free);
#undef MAKE_FUNCPTR

#define mbedtls_md_init                 pmbedtls_md_init
#define mbedtls_md_setup                pmbedtls_md_setup
#define mbedtls_md_update               pmbedtls_md_update
#define mbedtls_md_starts               pmbedtls_md_starts
#define mbedtls_md_finish               pmbedtls_md_finish
#define mbedtls_md_hmac_starts          pmbedtls_md_hmac_starts
#define mbedtls_md_hmac_finish          pmbedtls_md_hmac_finish
#define mbedtls_md_free                 pmbedtls_md_free

NTSTATUS process_attach( void *args )
{
    if (!(libmbedtls_handle = wine_dlopen( SONAME_LIBMBEDTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR( "failed to load libmbedtls, no support for crypto hashes\n" );
        return STATUS_DLL_NOT_FOUND;
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
    LOAD_FUNCPTR(mbedtls_md_starts)
    LOAD_FUNCPTR(mbedtls_md_finish)
    LOAD_FUNCPTR(mbedtls_md_hmac_starts)
    LOAD_FUNCPTR(mbedtls_md_hmac_finish)
    LOAD_FUNCPTR(mbedtls_md_free);
#undef LOAD_FUNCPTR

    return STATUS_SUCCESS;

fail:
    wine_dlclose( libmbedtls_handle, NULL, 0 );
    libmbedtls_handle = NULL;
    return STATUS_DLL_NOT_FOUND;
}

NTSTATUS process_detach( void *args )
{
    if (libmbedtls_handle)
    {
        wine_dlclose( libmbedtls_handle, NULL, 0 );
        libmbedtls_handle = NULL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS hash_init_internal( struct hash *hash, UCHAR *key, ULONG key_size )
{
    const mbedtls_md_info_t *md_info;
    mbedtls_md_type_t md_type;
    int ret;
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_init(&hash->u.hash_ctx);
    switch (hash->alg_id)
    {
    case ALG_ID_MD2:
        md_type = MBEDTLS_MD_MD2;
        break;

    case ALG_ID_MD4:
        md_type = MBEDTLS_MD_MD4;
        break;

    case ALG_ID_MD5:
        md_type = MBEDTLS_MD_MD5;
        break;

    case ALG_ID_SHA1:
        md_type = MBEDTLS_MD_SHA1;
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
        mbedtls_md_free(&hash->u.hash_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    if ((ret = mbedtls_md_setup(&hash->u.hash_ctx, md_info, hash->flags & HASH_FLAG_HMAC)) != 0)
    {
        mbedtls_md_free(&hash->u.hash_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    if (hash->flags & HASH_FLAG_HMAC)
    {
        mbedtls_md_hmac_starts(&hash->u.hash_ctx, key, key_size);
    }
    else
    {
        mbedtls_md_starts(&hash->u.hash_ctx);
    }

    return STATUS_SUCCESS;
}

NTSTATUS hash_init( struct hash *hash )
{
    return hash_init_internal( hash, NULL, 0 );
}

NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size )
{
    return hash_init_internal( hash, key, key_size );
}

NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_update(&hash->u.hash_ctx, input, size);

    return STATUS_SUCCESS;}

NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size )
{
    return hash_update( hash, input, size );
}

NTSTATUS hash_finish( struct hash *hash, UCHAR *output )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (hash->flags & HASH_FLAG_HMAC)
    {
        mbedtls_md_hmac_finish(&hash->u.hash_ctx, output);
    }
    else
    {
        mbedtls_md_finish(&hash->u.hash_ctx, output);
    }
    mbedtls_md_free(&hash->u.hash_ctx);

    return STATUS_SUCCESS;
}

NTSTATUS hmac_finish( struct hash *hash, UCHAR *output )
{
    return hash_finish( hash, output );
}

NTSTATUS key_symmetric_set_auth_data( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_vector_reset( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_encrypt_internal( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_decrypt_internal( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_get_tag( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_symmetric_destroy( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_generate( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_export( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_import( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_verify( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_sign( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_destroy( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_duplicate( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_decrypt( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_encrypt( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS key_asymmetric_derive_key( void *args )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBMBEDTLS */
