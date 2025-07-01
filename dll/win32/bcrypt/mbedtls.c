#include "precomp.h"

#ifdef SONAME_LIBMBEDTLS

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static void *libmbedtls_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(mbedtls_md_init);
MAKE_FUNCPTR(mbedtls_md_info_from_type);
MAKE_FUNCPTR(mbedtls_md_get_size);
MAKE_FUNCPTR(mbedtls_md_setup);
MAKE_FUNCPTR(mbedtls_md_update);
MAKE_FUNCPTR(mbedtls_md_starts);
MAKE_FUNCPTR(mbedtls_md_finish);
MAKE_FUNCPTR(mbedtls_md_hmac_update);
MAKE_FUNCPTR(mbedtls_md_hmac_starts);
MAKE_FUNCPTR(mbedtls_md_hmac_finish);
MAKE_FUNCPTR(mbedtls_md_free);
MAKE_FUNCPTR(mbedtls_cipher_init);
MAKE_FUNCPTR(mbedtls_cipher_setup);
MAKE_FUNCPTR(mbedtls_cipher_info_from_type);
MAKE_FUNCPTR(mbedtls_cipher_setkey);
MAKE_FUNCPTR(mbedtls_cipher_set_iv);
MAKE_FUNCPTR(mbedtls_cipher_set_padding_mode);
MAKE_FUNCPTR(mbedtls_cipher_update);
MAKE_FUNCPTR(mbedtls_cipher_update_ad);
MAKE_FUNCPTR(mbedtls_cipher_check_tag);
MAKE_FUNCPTR(mbedtls_cipher_write_tag);
MAKE_FUNCPTR(mbedtls_cipher_finish);
MAKE_FUNCPTR(mbedtls_cipher_free);
#undef MAKE_FUNCPTR

#define mbedtls_md_init                 pmbedtls_md_init
#define mbedtls_md_info_from_type       pmbedtls_md_info_from_type
#define mbedtls_md_get_size             pmbedtls_md_get_size
#define mbedtls_md_setup                pmbedtls_md_setup
#define mbedtls_md_update               pmbedtls_md_update
#define mbedtls_md_starts               pmbedtls_md_starts
#define mbedtls_md_finish               pmbedtls_md_finish
#define mbedtls_md_hmac_update          pmbedtls_md_hmac_update
#define mbedtls_md_hmac_starts          pmbedtls_md_hmac_starts
#define mbedtls_md_hmac_finish          pmbedtls_md_hmac_finish
#define mbedtls_md_free                 pmbedtls_md_free
#define mbedtls_cipher_init             pmbedtls_cipher_init
#define mbedtls_cipher_setup            pmbedtls_cipher_setup
#define mbedtls_cipher_info_from_type   pmbedtls_cipher_info_from_type
#define mbedtls_cipher_setkey           pmbedtls_cipher_setkey
#define mbedtls_cipher_set_iv           pmbedtls_cipher_set_iv
#define mbedtls_cipher_set_padding_mode pmbedtls_cipher_set_padding_mode
#define mbedtls_cipher_update           pmbedtls_cipher_update
#define mbedtls_cipher_update_ad        pmbedtls_cipher_update_ad
#define mbedtls_cipher_check_tag        pmbedtls_cipher_check_tag
#define mbedtls_cipher_write_tag        pmbedtls_cipher_write_tag
#define mbedtls_cipher_finish           pmbedtls_cipher_finish
#define mbedtls_cipher_free             pmbedtls_cipher_free

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
    LOAD_FUNCPTR(mbedtls_md_info_from_type)
    LOAD_FUNCPTR(mbedtls_md_get_size)
    LOAD_FUNCPTR(mbedtls_md_setup)
    LOAD_FUNCPTR(mbedtls_md_update)
    LOAD_FUNCPTR(mbedtls_md_starts)
    LOAD_FUNCPTR(mbedtls_md_finish)
    LOAD_FUNCPTR(mbedtls_md_hmac_update)
    LOAD_FUNCPTR(mbedtls_md_hmac_starts)
    LOAD_FUNCPTR(mbedtls_md_hmac_finish)
    LOAD_FUNCPTR(mbedtls_md_free);
    LOAD_FUNCPTR(mbedtls_cipher_init)
    LOAD_FUNCPTR(mbedtls_cipher_setup)
    LOAD_FUNCPTR(mbedtls_cipher_info_from_type)
    LOAD_FUNCPTR(mbedtls_cipher_setkey)
    LOAD_FUNCPTR(mbedtls_cipher_set_iv)
    LOAD_FUNCPTR(mbedtls_cipher_set_padding_mode);
    LOAD_FUNCPTR(mbedtls_cipher_update);
    LOAD_FUNCPTR(mbedtls_cipher_update_ad);
    LOAD_FUNCPTR(mbedtls_cipher_check_tag);
    LOAD_FUNCPTR(mbedtls_cipher_write_tag);
    LOAD_FUNCPTR(mbedtls_cipher_finish);
    LOAD_FUNCPTR(mbedtls_cipher_free);
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

NTSTATUS hash_init( struct hash *hash )
{
    const mbedtls_md_info_t *md_info;
    mbedtls_md_type_t md_type;
    int ret;
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    mbedtls_md_init(&hash->hash_ctx);
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
        mbedtls_md_free(&hash->hash_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    if ((ret = mbedtls_md_setup(&hash->hash_ctx, md_info, hash->flags & HASH_FLAG_HMAC)) != 0)
    {
        mbedtls_md_free(&hash->hash_ctx);
        return STATUS_INTERNAL_ERROR;
    }

    if (hash->flags & HASH_FLAG_HMAC)
        mbedtls_md_hmac_starts(&hash->hash_ctx, hash->secret, hash->secret_len);
    else
        mbedtls_md_starts(&hash->hash_ctx);

    return STATUS_SUCCESS;
}

NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (hash->flags & HASH_FLAG_HMAC)
        mbedtls_md_hmac_update(&hash->hash_ctx, input, size);
    else
        mbedtls_md_update(&hash->hash_ctx, input, size);

    return STATUS_SUCCESS;
}

NTSTATUS hash_finish( struct hash *hash, UCHAR *output )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (hash->flags & HASH_FLAG_HMAC)
        mbedtls_md_hmac_finish(&hash->hash_ctx, output);
    else
        mbedtls_md_finish(&hash->hash_ctx, output);

    mbedtls_md_free(&hash->hash_ctx);

    return STATUS_SUCCESS;
}

NTSTATUS hash_get_size( struct hash *hash, ULONG *output )
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (!hash->hash_ctx.md_info) return STATUS_INTERNAL_ERROR;
    if (!output) return STATUS_INTERNAL_ERROR;
    *output = mbedtls_md_get_size(hash->hash_ctx.md_info);

    return STATUS_SUCCESS;
}

typedef struct {
	unsigned char *data;
	unsigned int size;
} mbedtls_datum_t;

union key_data
{
    mbedtls_cipher_context_t cipher_ctx;
    struct
    {
        mbedtls_pk_context  privkey;
        mbedtls_pk_context  pubkey;
        mbedtls_dhm_context dh_params;
    } a;
};
C_ASSERT( sizeof(union key_data) <= sizeof(((struct key *)0)->private) );

static union key_data *key_data( struct key *key )
{
    return (union key_data *)key->private;
}

mbedtls_cipher_id_t get_mbedtls_cipher( const struct key *key )
{
    switch (key->alg_id)
    {
    case ALG_ID_3DES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
        case CHAIN_MODE_ECB:
            return MBEDTLS_CIPHER_DES_EDE3_ECB;
        case CHAIN_MODE_CBC:
            return MBEDTLS_CIPHER_DES_EDE3_CBC;
        default:
            break;
        }
        FIXME( "3DES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return MBEDTLS_CIPHER_NONE;

    case ALG_ID_AES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
        case CHAIN_MODE_GCM:
            if (key->u.s.secret_len == 16) return MBEDTLS_CIPHER_AES_128_GCM;
            if (key->u.s.secret_len == 24) return MBEDTLS_CIPHER_AES_192_GCM;
            if (key->u.s.secret_len == 32) return MBEDTLS_CIPHER_AES_256_GCM;
            break;
        case CHAIN_MODE_ECB: /* can be emulated with CBC + empty IV */
        case CHAIN_MODE_CBC:
            if (key->u.s.secret_len == 16) return MBEDTLS_CIPHER_AES_128_CBC;
            if (key->u.s.secret_len == 24) return MBEDTLS_CIPHER_AES_192_CBC;
            if (key->u.s.secret_len == 32) return MBEDTLS_CIPHER_AES_256_CBC;
            break;
        case CHAIN_MODE_CFB:
            if (key->u.s.secret_len == 16) return MBEDTLS_CIPHER_AES_128_CFB8;
            if (key->u.s.secret_len == 24) return MBEDTLS_CIPHER_AES_192_CFB8;
            if (key->u.s.secret_len == 32) return MBEDTLS_CIPHER_AES_256_CFB8;
            break;
        default:
            break;
        }
        FIXME( "AES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return MBEDTLS_CIPHER_NONE;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return MBEDTLS_CIPHER_NONE;
    }
    return MBEDTLS_CIPHER_NONE;
}

NTSTATUS key_symmetric_vector_reset( void *args )
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->cipher_ctx.cipher_info != NULL && key->u.s.vector != NULL && key->u.s.vector_len)
    {
        if (mbedtls_cipher_set_iv( &key_data(key)->cipher_ctx, key->u.s.vector, key->u.s.vector_len )) return STATUS_INTERNAL_ERROR;
        return STATUS_SUCCESS;
    }
    TRACE( "invalidating cipher handle\n" );
    mbedtls_cipher_free( &key_data(key)->cipher_ctx );
    key_data(key)->cipher_ctx.cipher_info = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS init_cipher_handle( struct key *key, const mbedtls_operation_t operation )
{
    mbedtls_cipher_type_t cipher;
    const mbedtls_cipher_info_t *info;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->cipher_ctx.cipher_info != NULL)
    {
        if (mbedtls_cipher_get_operation( &key_data(key)->cipher_ctx ) == operation) return STATUS_SUCCESS;
        mbedtls_cipher_free( &key_data(key)->cipher_ctx );
        key_data(key)->cipher_ctx.cipher_info = NULL;
    }
    cipher = get_mbedtls_cipher( key );
    if (cipher == MBEDTLS_CIPHER_NONE) return STATUS_NOT_SUPPORTED;

    if ((info = mbedtls_cipher_info_from_type( cipher )) == NULL)
    {
        return STATUS_INTERNAL_ERROR;
    }
    mbedtls_cipher_init( &key_data(key)->cipher_ctx );
    if (mbedtls_cipher_setup( &key_data(key)->cipher_ctx, info ))
    {
        mbedtls_cipher_free( &key_data(key)->cipher_ctx );
        return STATUS_INTERNAL_ERROR;
    }
    mbedtls_cipher_set_padding_mode( &key_data(key)->cipher_ctx, MBEDTLS_PADDING_NONE );
    if (key->u.s.secret != NULL && key->u.s.secret_len > 0) mbedtls_cipher_setkey( &key_data(key)->cipher_ctx, key->u.s.secret, /*mbedtls_cipher_get_key_bitlen( &key_data(key)->cipher_ctx )*/key->u.s.secret_len*8, operation );
    if (key->u.s.vector != NULL && key->u.s.vector_len > 0) mbedtls_cipher_set_iv( &key_data(key)->cipher_ctx, key->u.s.vector, key->u.s.vector_len );

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_set_auth_data( void *args )
{
    struct key_symmetric_set_auth_data_params *auth_params = args;
    NTSTATUS status;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle( auth_params->key, auth_params->encrypt ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT ))) return status;

    ret = mbedtls_cipher_update_ad( &key_data(auth_params->key)->cipher_ctx, auth_params->auth_data, auth_params->len );
    if (ret) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_get_tag( void *args )
{
    struct key_symmetric_get_tag_params *tag_params = args;
    mbedtls_operation_t operation;
    int ret;
    NTSTATUS status;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    operation = mbedtls_cipher_get_operation( &key_data(tag_params->key)->cipher_ctx );
    if ((status = init_cipher_handle( tag_params->key, operation ))) return status;

    switch (operation) {
        case MBEDTLS_ENCRYPT:
            ret = mbedtls_cipher_write_tag( &key_data(tag_params->key)->cipher_ctx, tag_params->tag, tag_params->len );
            if (ret) status = STATUS_INTERNAL_ERROR;
            break;
        case MBEDTLS_DECRYPT:
            ret = mbedtls_cipher_check_tag( &key_data(tag_params->key)->cipher_ctx, tag_params->tag, tag_params->len );
            if (ret)
            {
                if (ret == MBEDTLS_ERR_CIPHER_AUTH_FAILED)
                    status = STATUS_AUTH_TAG_MISMATCH;
                else
                    status = STATUS_INTERNAL_ERROR;
            }
            break;
        default:
            FIXME( "AES operation %u not supported\n", operation );
            status = STATUS_INTERNAL_ERROR;
            break;
    }

    return status;
}

NTSTATUS key_symmetric_encrypt_internal( void *args )
{
    const struct key_symmetric_encrypt_params *params = args;
    NTSTATUS status;
    int ret;
    size_t output_len = 0;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle( params->key, MBEDTLS_ENCRYPT ))) return status;

    ret = mbedtls_cipher_update( &key_data(params->key)->cipher_ctx,
                                    params->input, params->input_len,
                                    params->output, &output_len);
    if (!output_len)
        ret = mbedtls_cipher_finish( &key_data(params->key)->cipher_ctx,
                                    params->output, &output_len);
    if (ret) return STATUS_INTERNAL_ERROR;

    if (params->output_len != output_len) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_decrypt_internal( void *args )
{
    const struct key_symmetric_decrypt_params *params = args;
    NTSTATUS status;
    int ret;
    size_t output_len = 0;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle( params->key, MBEDTLS_DECRYPT ))) return status;

    ret = mbedtls_cipher_update( &key_data(params->key)->cipher_ctx,
                                    params->input, params->input_len,
                                    params->output, &output_len);
    if (!output_len)
        ret = mbedtls_cipher_finish( &key_data(params->key)->cipher_ctx,
                                    params->output, &output_len);
    if (ret) return STATUS_INTERNAL_ERROR;

    if (params->output_len != output_len) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_destroy( void *args )
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->cipher_ctx.cipher_info != NULL) {
        mbedtls_cipher_free( &key_data(key)->cipher_ctx );
        key_data(key)->cipher_ctx.cipher_info = NULL;
    }
    return STATUS_SUCCESS;
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
