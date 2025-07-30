#include "precomp.h"

#ifdef HAVE_COMMONCRYPTO_COMMONDIGEST_H
NTSTATUS process_attach( void *args )
{
    return STATUS_SUCCESS;
}

NTSTATUS process_detach( void *args )
{
    return STATUS_SUCCESS;
}

NTSTATUS hash_init( struct hash *hash )
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

NTSTATUS hmac_init( struct hash *hash, UCHAR *key, ULONG key_size )
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


NTSTATUS hash_update( struct hash *hash, UCHAR *input, ULONG size )
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

NTSTATUS hmac_update( struct hash *hash, UCHAR *input, ULONG size )
{
    CCHmacUpdate( &hash->u.hmac_ctx, input, size );
    return STATUS_SUCCESS;
}

NTSTATUS hash_finish( struct hash *hash, UCHAR *output )
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

NTSTATUS hmac_finish( struct hash *hash, UCHAR *output )
{
    CCHmacFinal( &hash->u.hmac_ctx, output );

    return STATUS_SUCCESS;
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

#endif /* HAVE_COMMONCRYPTO_COMMONDIGEST_H */
