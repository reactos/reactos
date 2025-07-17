#include "precomp.h"

#ifdef SONAME_LIBMBEDTLS

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

static void *libmbedtls_handle;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

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
MAKE_FUNCPTR(mbedtls_ctr_drbg_init);
MAKE_FUNCPTR(mbedtls_ctr_drbg_random);
MAKE_FUNCPTR(mbedtls_ctr_drbg_seed);
MAKE_FUNCPTR(mbedtls_ctr_drbg_free);
MAKE_FUNCPTR(mbedtls_entropy_init);
MAKE_FUNCPTR(mbedtls_entropy_func);
MAKE_FUNCPTR(mbedtls_entropy_free);
MAKE_FUNCPTR(mbedtls_pk_init);
MAKE_FUNCPTR(mbedtls_pk_get_type);
MAKE_FUNCPTR(mbedtls_pk_setup);
MAKE_FUNCPTR(mbedtls_pk_info_from_type);
MAKE_FUNCPTR(mbedtls_pk_write_key_der);
MAKE_FUNCPTR(mbedtls_pk_parse_key);
MAKE_FUNCPTR(mbedtls_pk_parse_public_key);
MAKE_FUNCPTR(mbedtls_pk_write_pubkey_der);
MAKE_FUNCPTR(mbedtls_pk_verify_ext);
MAKE_FUNCPTR(mbedtls_pk_sign);
MAKE_FUNCPTR(mbedtls_pk_encrypt);
MAKE_FUNCPTR(mbedtls_pk_decrypt);
MAKE_FUNCPTR(mbedtls_pk_free);
MAKE_FUNCPTR(mbedtls_ecdh_init);
MAKE_FUNCPTR(mbedtls_ecdh_compute_shared);
MAKE_FUNCPTR(mbedtls_ecdh_free);
MAKE_FUNCPTR(mbedtls_rsa_gen_key);
MAKE_FUNCPTR(mbedtls_rsa_export);
MAKE_FUNCPTR(mbedtls_rsa_import_raw);
MAKE_FUNCPTR(mbedtls_rsa_export_raw);
MAKE_FUNCPTR(mbedtls_rsa_export_crt);
MAKE_FUNCPTR(mbedtls_rsa_set_padding);
MAKE_FUNCPTR(mbedtls_rsa_complete);
MAKE_FUNCPTR(mbedtls_rsa_check_pubkey);
MAKE_FUNCPTR(mbedtls_rsa_check_privkey);
MAKE_FUNCPTR(mbedtls_rsa_check_pub_priv);
MAKE_FUNCPTR(mbedtls_ecp_gen_key);
MAKE_FUNCPTR(mbedtls_ecp_group_load);
MAKE_FUNCPTR(mbedtls_ecp_point_read_binary);
MAKE_FUNCPTR(mbedtls_ecp_point_write_binary);
MAKE_FUNCPTR(mbedtls_ecp_check_pubkey);
MAKE_FUNCPTR(mbedtls_ecp_copy);
MAKE_FUNCPTR(mbedtls_mpi_init);
MAKE_FUNCPTR(mbedtls_mpi_size);
MAKE_FUNCPTR(mbedtls_mpi_lset);
MAKE_FUNCPTR(mbedtls_mpi_copy);
MAKE_FUNCPTR(mbedtls_mpi_cmp_mpi);
MAKE_FUNCPTR(mbedtls_mpi_read_binary);
MAKE_FUNCPTR(mbedtls_mpi_write_binary);
MAKE_FUNCPTR(mbedtls_mpi_read_string);
MAKE_FUNCPTR(mbedtls_mpi_gen_prime);
MAKE_FUNCPTR(mbedtls_mpi_sub_int);
MAKE_FUNCPTR(mbedtls_mpi_div_int);
MAKE_FUNCPTR(mbedtls_mpi_is_prime_ext);
MAKE_FUNCPTR(mbedtls_mpi_free);
MAKE_FUNCPTR(mbedtls_asn1_get_len);
MAKE_FUNCPTR(mbedtls_asn1_get_mpi);
MAKE_FUNCPTR(mbedtls_asn1_get_tag);
MAKE_FUNCPTR(mbedtls_asn1_write_len);
MAKE_FUNCPTR(mbedtls_asn1_write_mpi);
MAKE_FUNCPTR(mbedtls_asn1_write_tag);
MAKE_FUNCPTR(mbedtls_asn1_write_raw_buffer);
MAKE_FUNCPTR(mbedtls_dhm_init);
MAKE_FUNCPTR(mbedtls_dhm_make_params);
MAKE_FUNCPTR(mbedtls_dhm_calc_secret);
MAKE_FUNCPTR(mbedtls_dhm_free);
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
#define mbedtls_ctr_drbg_init           pmbedtls_ctr_drbg_init
#define mbedtls_ctr_drbg_random         pmbedtls_ctr_drbg_random
#define mbedtls_ctr_drbg_seed           pmbedtls_ctr_drbg_seed
#define mbedtls_ctr_drbg_free           pmbedtls_ctr_drbg_free
#define mbedtls_entropy_init            pmbedtls_entropy_init
#define mbedtls_entropy_func            pmbedtls_entropy_func
#define mbedtls_entropy_free            pmbedtls_entropy_free
#define mbedtls_pk_init                 pmbedtls_pk_init
#define mbedtls_pk_get_type             pmbedtls_pk_get_type
#define mbedtls_pk_setup                pmbedtls_pk_setup
#define mbedtls_pk_info_from_type       pmbedtls_pk_info_from_type
#define mbedtls_pk_write_key_der        pmbedtls_pk_write_key_der
#define mbedtls_pk_parse_key            pmbedtls_pk_parse_key
#define mbedtls_pk_parse_public_key     pmbedtls_pk_parse_public_key
#define mbedtls_pk_write_pubkey_der     pmbedtls_pk_write_pubkey_der
#define mbedtls_pk_verify_ext           pmbedtls_pk_verify_ext
#define mbedtls_pk_sign                 pmbedtls_pk_sign
#define mbedtls_pk_encrypt              pmbedtls_pk_encrypt
#define mbedtls_pk_decrypt              pmbedtls_pk_decrypt
#define mbedtls_pk_free                 pmbedtls_pk_free
#define mbedtls_ecdh_init               pmbedtls_ecdh_init
#define mbedtls_ecdh_compute_shared     pmbedtls_ecdh_compute_shared
#define mbedtls_ecdh_free               pmbedtls_ecdh_free
#define mbedtls_rsa_gen_key             pmbedtls_rsa_gen_key
#define mbedtls_rsa_import_raw          pmbedtls_rsa_import_raw
#define mbedtls_rsa_export              pmbedtls_rsa_export
#define mbedtls_rsa_export_raw          pmbedtls_rsa_export_raw
#define mbedtls_rsa_export_crt          pmbedtls_rsa_export_crt
#define mbedtls_rsa_set_padding         pmbedtls_rsa_set_padding
#define mbedtls_rsa_complete            pmbedtls_rsa_complete
#define mbedtls_rsa_check_pubkey        pmbedtls_rsa_check_pubkey
#define mbedtls_rsa_check_privkey       pmbedtls_rsa_check_privkey
#define mbedtls_rsa_check_pub_priv      pmbedtls_rsa_check_pub_priv
#define mbedtls_ecp_gen_key             pmbedtls_ecp_gen_key
#define mbedtls_ecp_group_load          pmbedtls_ecp_group_load
#define mbedtls_ecp_point_read_binary   pmbedtls_ecp_point_read_binary
#define mbedtls_ecp_point_write_binary  pmbedtls_ecp_point_write_binary
#define mbedtls_ecp_check_pubkey        pmbedtls_ecp_check_pubkey
#define mbedtls_ecp_copy                pmbedtls_ecp_copy
#define mbedtls_mpi_init                pmbedtls_mpi_init
#define mbedtls_mpi_size                pmbedtls_mpi_size
#define mbedtls_mpi_lset                pmbedtls_mpi_lset
#define mbedtls_mpi_copy                pmbedtls_mpi_copy
#define mbedtls_mpi_cmp_mpi             pmbedtls_mpi_cmp_mpi
#define mbedtls_mpi_read_binary         pmbedtls_mpi_read_binary
#define mbedtls_mpi_write_binary        pmbedtls_mpi_write_binary
#define mbedtls_mpi_read_string         pmbedtls_mpi_read_string
#define mbedtls_mpi_gen_prime           pmbedtls_mpi_gen_prime
#define mbedtls_mpi_sub_int             pmbedtls_mpi_sub_int
#define mbedtls_mpi_div_int             pmbedtls_mpi_div_int
#define mbedtls_mpi_is_prime_ext        pmbedtls_mpi_is_prime_ext
#define mbedtls_mpi_free                pmbedtls_mpi_free
#define mbedtls_asn1_get_len            pmbedtls_asn1_get_len
#define mbedtls_asn1_get_mpi            pmbedtls_asn1_get_mpi
#define mbedtls_asn1_get_tag            pmbedtls_asn1_get_tag
#define mbedtls_asn1_write_len          pmbedtls_asn1_write_len
#define mbedtls_asn1_write_mpi          pmbedtls_asn1_write_mpi
#define mbedtls_asn1_write_tag          pmbedtls_asn1_write_tag
#define mbedtls_asn1_write_raw_buffer   pmbedtls_asn1_write_raw_buffer
#define mbedtls_dhm_init                pmbedtls_dhm_init
#define mbedtls_dhm_make_params         pmbedtls_dhm_make_params
#define mbedtls_dhm_calc_secret         pmbedtls_dhm_calc_secret
#define mbedtls_dhm_free                pmbedtls_dhm_free

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
    LOAD_FUNCPTR(mbedtls_ctr_drbg_init)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_random)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_seed)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_free)
    LOAD_FUNCPTR(mbedtls_entropy_init)
    LOAD_FUNCPTR(mbedtls_entropy_func)
    LOAD_FUNCPTR(mbedtls_entropy_free)
    LOAD_FUNCPTR(mbedtls_pk_init)
    LOAD_FUNCPTR(mbedtls_pk_get_type)
    LOAD_FUNCPTR(mbedtls_pk_setup)
    LOAD_FUNCPTR(mbedtls_pk_info_from_type)
    LOAD_FUNCPTR(mbedtls_pk_write_key_der)
    LOAD_FUNCPTR(mbedtls_pk_parse_key)
    LOAD_FUNCPTR(mbedtls_pk_parse_public_key)
    LOAD_FUNCPTR(mbedtls_pk_write_pubkey_der)
    LOAD_FUNCPTR(mbedtls_pk_verify_ext)
    LOAD_FUNCPTR(mbedtls_pk_sign)
    LOAD_FUNCPTR(mbedtls_pk_encrypt)
    LOAD_FUNCPTR(mbedtls_pk_decrypt)
    LOAD_FUNCPTR(mbedtls_pk_free)
    LOAD_FUNCPTR(mbedtls_ecdh_init)
    LOAD_FUNCPTR(mbedtls_ecdh_compute_shared)
    LOAD_FUNCPTR(mbedtls_ecdh_free)
    LOAD_FUNCPTR(mbedtls_rsa_gen_key)
    LOAD_FUNCPTR(mbedtls_rsa_import_raw)
    LOAD_FUNCPTR(mbedtls_rsa_export_raw)
    LOAD_FUNCPTR(mbedtls_rsa_export)
    LOAD_FUNCPTR(mbedtls_rsa_export_crt)
    LOAD_FUNCPTR(mbedtls_rsa_set_padding)
    LOAD_FUNCPTR(mbedtls_rsa_complete)
    LOAD_FUNCPTR(mbedtls_rsa_check_pubkey)
    LOAD_FUNCPTR(mbedtls_rsa_check_privkey)
    LOAD_FUNCPTR(mbedtls_rsa_check_pub_priv)
    LOAD_FUNCPTR(mbedtls_ecp_gen_key)
    LOAD_FUNCPTR(mbedtls_ecp_group_load)
    LOAD_FUNCPTR(mbedtls_ecp_point_read_binary)
    LOAD_FUNCPTR(mbedtls_ecp_point_write_binary)
    LOAD_FUNCPTR(mbedtls_ecp_check_pubkey)
    LOAD_FUNCPTR(mbedtls_ecp_copy)
    LOAD_FUNCPTR(mbedtls_mpi_init)
    LOAD_FUNCPTR(mbedtls_mpi_size)
    LOAD_FUNCPTR(mbedtls_mpi_lset)
    LOAD_FUNCPTR(mbedtls_mpi_copy)
    LOAD_FUNCPTR(mbedtls_mpi_cmp_mpi)
    LOAD_FUNCPTR(mbedtls_mpi_read_binary)
    LOAD_FUNCPTR(mbedtls_mpi_write_binary)
    LOAD_FUNCPTR(mbedtls_mpi_read_string)
    LOAD_FUNCPTR(mbedtls_mpi_gen_prime)
    LOAD_FUNCPTR(mbedtls_mpi_sub_int)
    LOAD_FUNCPTR(mbedtls_mpi_div_int)
    LOAD_FUNCPTR(mbedtls_mpi_is_prime_ext)
    LOAD_FUNCPTR(mbedtls_mpi_free)
    LOAD_FUNCPTR(mbedtls_asn1_get_len)
    LOAD_FUNCPTR(mbedtls_asn1_get_mpi)
    LOAD_FUNCPTR(mbedtls_asn1_get_tag)
    LOAD_FUNCPTR(mbedtls_asn1_write_len)
    LOAD_FUNCPTR(mbedtls_asn1_write_mpi)
    LOAD_FUNCPTR(mbedtls_asn1_write_tag)
    LOAD_FUNCPTR(mbedtls_asn1_write_raw_buffer)
    LOAD_FUNCPTR(mbedtls_dhm_init)
    LOAD_FUNCPTR(mbedtls_dhm_make_params)
    LOAD_FUNCPTR(mbedtls_dhm_calc_secret)
    LOAD_FUNCPTR(mbedtls_dhm_free)
#undef LOAD_FUNCPTR
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

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
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
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
        mbedtls_dhm_context dh_privkey;
        mbedtls_dhm_context dh_pubkey;
    } a;
};
C_ASSERT( sizeof(union key_data) <= sizeof(((struct key *)0)->private) );

static union key_data *key_data( struct key *key )
{
    return (union key_data *)key->private;
}

mbedtls_cipher_id_t key_get_cipher( const struct key *key )
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
    cipher = key_get_cipher( key );
    if (cipher == MBEDTLS_CIPHER_NONE) return STATUS_NOT_SUPPORTED;

    if ((info = mbedtls_cipher_info_from_type( cipher )) == NULL) return STATUS_INTERNAL_ERROR;
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
    if (key_data(key)->cipher_ctx.cipher_info != NULL)
    {
        mbedtls_cipher_free( &key_data(key)->cipher_ctx );
        key_data(key)->cipher_ctx.cipher_info = NULL;
    }
    return STATUS_SUCCESS;
}

static mbedtls_pk_type_t key_get_type(enum alg_id alg_id)
{
    mbedtls_pk_type_t pk_type;

    switch (alg_id)
    {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            pk_type = MBEDTLS_PK_RSA;
            break;
#if 0 /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
            pk_type = MBEDTLS_PK_DSA;
            break;
#endif
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
            pk_type = MBEDTLS_PK_ECKEY_DH;
            break;
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            pk_type = MBEDTLS_PK_ECDSA;
            break;
        default:
            FIXME("algorithm %u not supported\n", alg_id);
            pk_type = MBEDTLS_PK_NONE;
            break;
    }
    return pk_type;
}

static NTSTATUS key_export_ecc_public(struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_ECCKEY_BLOB *ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    mbedtls_ecp_group_id curve;
    mbedtls_ecp_keypair *keypair;
    DWORD magic, size;
    UCHAR *dst;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
            magic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
            size = 32;
            break;

        case ALG_ID_ECDH_P384:
            magic = BCRYPT_ECDH_PUBLIC_P384_MAGIC;
            size = 48;
            break;

        case ALG_ID_ECDSA_P256:
            magic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
            size = 32;
            break;

        case ALG_ID_ECDSA_P384:
            magic = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
            size = 48;
            break;

        default:
            FIXME("algorithm %u not supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    keypair = mbedtls_pk_ec(key_data(key)->a.pubkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    curve = keypair->grp.id;

    if (curve != MBEDTLS_ECP_DP_SECP256R1 && curve != MBEDTLS_ECP_DP_SECP384R1)
    {
        FIXME("curve %u not supported\n", curve);
        return STATUS_NOT_IMPLEMENTED;
    }

    *ret_len = sizeof(*ecc_blob) + size + size;
    if (len >= *ret_len && buf)
    {
        ecc_blob->dwMagic = magic;
        ecc_blob->cbKey = size;

        dst = buf + sizeof(*ecc_blob);
        mbedtls_mpi_write_binary(&keypair->Q.X, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&keypair->Q.Y, dst, size);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_ecc(struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_ECCKEY_BLOB *ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    mbedtls_ecp_group_id curve;
    mbedtls_ecp_keypair *keypair;
    size_t x_size, y_size, d_size;
    DWORD magic, size;
    UCHAR *dst;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
            magic = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
            size = 32;
            break;

        case ALG_ID_ECDH_P384:
            magic = BCRYPT_ECDH_PRIVATE_P384_MAGIC;
            size = 48;
            break;

        case ALG_ID_ECDSA_P256:
            magic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
            size = 32;
            break;

        case ALG_ID_ECDSA_P384:
            magic = BCRYPT_ECDSA_PRIVATE_P384_MAGIC;
            size = 48;
            break;

        default:
            FIXME("algorithm %u does not yet support exporting ecc blob\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    keypair = mbedtls_pk_ec(key_data(key)->a.privkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    curve = keypair->grp.id;

    if (curve != MBEDTLS_ECP_DP_SECP256R1 && curve != MBEDTLS_ECP_DP_SECP384R1)
    {
        FIXME("curve %u not supported\n", curve);
        return STATUS_NOT_IMPLEMENTED;
    }

    x_size = mbedtls_mpi_size(&keypair->Q.X);
    y_size = mbedtls_mpi_size(&keypair->Q.Y);
    d_size = mbedtls_mpi_size(&keypair->d);

    *ret_len = sizeof(*ecc_blob) + size + size + size;
    if (len >= *ret_len && buf)
    {
        ecc_blob->dwMagic = magic;
        ecc_blob->cbKey = size;

        dst = buf + sizeof(*ecc_blob);
        mbedtls_mpi_write_binary(&keypair->Q.X, dst, x_size);
        dst += x_size;
        mbedtls_mpi_write_binary(&keypair->Q.Y, dst, y_size);
        dst += y_size;
        mbedtls_mpi_write_binary(&keypair->d, dst, d_size);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_rsa_public(struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    mbedtls_rsa_context *rsa_context;
    mbedtls_mpi m, e;
    UCHAR *dst;
    int ret;
    size_t m_size, e_size;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pubkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    mbedtls_mpi_init(&m);
    mbedtls_mpi_init(&e);

    ret = mbedtls_rsa_export(rsa_context, &m, NULL, NULL, NULL, &e);
    if (ret)
    {
        mbedtls_mpi_free(&e);
        mbedtls_mpi_free(&m);
        return STATUS_INTERNAL_ERROR;
    }

    m_size = mbedtls_mpi_size(&m);
    e_size = mbedtls_mpi_size(&e);

    *ret_len = sizeof(*rsa_blob) + e_size + m_size;
    if (len >= *ret_len && buf)
    {
        dst = buf + sizeof(*rsa_blob);
        mbedtls_mpi_write_binary(&e, dst, e_size);
        rsa_blob->cbPublicExp = e_size;

        dst += rsa_blob->cbPublicExp;
        mbedtls_mpi_write_binary(&m, dst, m_size);
        rsa_blob->cbModulus = m_size;

        rsa_blob->Magic = BCRYPT_RSAPUBLIC_MAGIC;
        rsa_blob->BitLength = key->u.a.bitlen;
        rsa_blob->cbPrime1 = 0;
        rsa_blob->cbPrime2 = 0;
    }

    mbedtls_mpi_free(&e);
    mbedtls_mpi_free(&m);
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_rsa(struct key *key, ULONG flags, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    mbedtls_rsa_context *rsa_context;
    mbedtls_mpi m, e, d, p, q, u, e1, e2;
    BOOL full = (flags & KEY_EXPORT_FLAG_RSA_FULL);
    UCHAR *dst;
    int ret;
    size_t m_size, e_size, d_size, p_size, q_size, u_size, e1_size, e2_size;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.privkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    mbedtls_mpi_init(&m);
    mbedtls_mpi_init(&e);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&p);
    mbedtls_mpi_init(&q);
    if (full)
    {
        mbedtls_mpi_init(&u);
        mbedtls_mpi_init(&e1);
        mbedtls_mpi_init(&e2);
    }
    ret = mbedtls_rsa_export(rsa_context, &m, &p, &q, &d, &e);
    if (ret)
    {
        mbedtls_mpi_free(&m);
        mbedtls_mpi_free(&e);
        mbedtls_mpi_free(&d);
        mbedtls_mpi_free(&p);
        mbedtls_mpi_free(&q);
        if (full)
        {
            mbedtls_mpi_free(&u);
            mbedtls_mpi_free(&e1);
            mbedtls_mpi_free(&e2);
        }
        return STATUS_INTERNAL_ERROR;
    }

    if (full)
    {
        ret = mbedtls_rsa_export_crt(rsa_context, &e1, &e2, &u);
        if (ret)
        {
            mbedtls_mpi_free(&m);
            mbedtls_mpi_free(&e);
            mbedtls_mpi_free(&d);
            mbedtls_mpi_free(&p);
            mbedtls_mpi_free(&q);
            mbedtls_mpi_free(&u);
            mbedtls_mpi_free(&e1);
            mbedtls_mpi_free(&e2);
            return STATUS_INTERNAL_ERROR;
        }
    }

    m_size = mbedtls_mpi_size(&m);
    e_size = mbedtls_mpi_size(&e);
    d_size = mbedtls_mpi_size(&d);
    p_size = mbedtls_mpi_size(&p);
    q_size = mbedtls_mpi_size(&q);

    if (full)
    {
        u_size = mbedtls_mpi_size(&u);
        e1_size = mbedtls_mpi_size(&e1);
        e2_size = mbedtls_mpi_size(&e2);
    }
    *ret_len = sizeof(*rsa_blob) + e_size + m_size + p_size + q_size;

    if (full) *ret_len += e1_size + e2_size + u_size + d_size;

    if (len >= *ret_len && buf)
    {
        rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
        rsa_blob->Magic = full ? BCRYPT_RSAFULLPRIVATE_MAGIC : BCRYPT_RSAPRIVATE_MAGIC;
        rsa_blob->BitLength = key->u.a.bitlen;

        dst = buf + sizeof(*rsa_blob);
        mbedtls_mpi_write_binary(&e, dst, e_size);
        rsa_blob->cbPublicExp = e_size;

        dst += rsa_blob->cbPublicExp;
        mbedtls_mpi_write_binary(&m, dst, m_size);
        rsa_blob->cbModulus = m_size;

        dst += rsa_blob->cbModulus;
        mbedtls_mpi_write_binary(&p, dst, p_size);
        rsa_blob->cbPrime1 = p_size;

        dst += rsa_blob->cbPrime1;
        mbedtls_mpi_write_binary(&q, dst, q_size);
        rsa_blob->cbPrime2 = q_size;

        if (full)
        {
            dst += rsa_blob->cbPrime2;
            mbedtls_mpi_write_binary(&e1, dst, e1_size);

            dst += e1_size;
            mbedtls_mpi_write_binary(&e2, dst, e2_size);

            dst += e2_size;
            mbedtls_mpi_write_binary(&u, dst, u_size);

            dst += u_size;
            mbedtls_mpi_write_binary(&d, dst, d_size);
        }
    }

    mbedtls_mpi_free(&m);
    mbedtls_mpi_free(&e);
    mbedtls_mpi_free(&d);
    mbedtls_mpi_free(&p);
    mbedtls_mpi_free(&q);
    if (full)
    {
        mbedtls_mpi_free(&u);
        mbedtls_mpi_free(&e1);
        mbedtls_mpi_free(&e2);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_dh_params(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_DH_PARAMETER_HEADER *dh_params = (BCRYPT_DH_PARAMETER_HEADER *)buf;
    size_t size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*dh_params) + size + size;

    if (len >= *ret_len && buf)
    {
        dh_params->dwMagic = BCRYPT_DH_PARAMETERS_MAGIC;
        dh_params->cbLength = *ret_len;
        dh_params->cbKeyLength = key->u.a.bitlen / 8;

        dst = buf + sizeof(*dh_params);
        mbedtls_mpi_write_binary(&dh_key->P, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->G, dst, size);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_dh_public(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_DH_KEY_BLOB *dh_blob = (BCRYPT_DH_KEY_BLOB *)buf;
    size_t size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*dh_blob) + size + size + size;

    if (len >= *ret_len && buf)
    {
        dh_blob->dwMagic = BCRYPT_DH_PUBLIC_MAGIC;
        dh_blob->cbKey = key->u.a.bitlen / 8;

        dst = buf + sizeof(*dh_blob);
        mbedtls_mpi_write_binary(&dh_key->P, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->G, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->GY, dst, size);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_dh(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_DH_KEY_BLOB *dh_blob = (BCRYPT_DH_KEY_BLOB *)buf;
    size_t size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*dh_blob) + size + size + size + size;

    if (len >= *ret_len && buf)
    {
        dh_blob->dwMagic = BCRYPT_DH_PRIVATE_MAGIC;
        dh_blob->cbKey = key->u.a.bitlen / 8;

        dst = buf + sizeof(*dh_blob);
        mbedtls_mpi_write_binary(&dh_key->P, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->G, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->GY, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->X, dst, size);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_export(void *args)
{
    const struct key_asymmetric_export_params *params = args;
    struct key *key = params->key;
    unsigned flags = params->flags;

    if (!(key->u.a.flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            if (flags & KEY_EXPORT_FLAG_PUBLIC)
                return key_export_ecc_public(key, params->buf, params->len, params->ret_len);
            return key_export_ecc(key, params->buf, params->len, params->ret_len);

        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            if (flags & KEY_EXPORT_FLAG_PUBLIC)
                return key_export_rsa_public(key, params->buf, params->len, params->ret_len);
            return key_export_rsa(key, flags, params->buf, params->len, params->ret_len);

#if 0 /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
            return STATUS_NOT_IMPLEMENTED;
#endif

        case ALG_ID_DH:
            if (flags & KEY_EXPORT_FLAG_DH_PARAMETERS)
                return key_export_dh_params(key, &key_data(key)->a.dh_pubkey, params->buf, params->len, params->ret_len);
            if (flags & KEY_EXPORT_FLAG_PUBLIC)
                return key_export_dh_public(key, &key_data(key)->a.dh_pubkey, params->buf, params->len, params->ret_len);
            return key_export_dh(key, &key_data(key)->a.dh_privkey, params->buf, params->len, params->ret_len);
        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS key_import_ecc_public(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    mbedtls_ecp_group_id curve;
    const mbedtls_ecp_keypair *keypair;
    int ret;
    size_t plen;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDSA_P256:
            curve = MBEDTLS_ECP_DP_SECP256R1;
            break;

        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P384:
            curve = MBEDTLS_ECP_DP_SECP384R1;
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    keypair = mbedtls_pk_ec(key_data(key)->a.pubkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_ecp_group_load((struct mbedtls_ecp_group *)&keypair->grp, curve);
    if (ret) STATUS_INTERNAL_ERROR;

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    plen = mbedtls_mpi_size(&keypair->grp.P);
    if (plen != ecc_blob->cbKey) STATUS_INTERNAL_ERROR;

    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.X, buf + sizeof(*ecc_blob), ecc_blob->cbKey);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.Y, buf + sizeof(*ecc_blob) + ecc_blob->cbKey, ecc_blob->cbKey);
    mbedtls_mpi_lset((struct mbedtls_mpi *)&keypair->Q.Z, 1);
    ret = mbedtls_ecp_check_pubkey(&keypair->grp, &keypair->Q);

    if (ret) STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_ecc(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    mbedtls_ecp_group_id curve;
    const mbedtls_ecp_keypair *keypair;
    int ret;
    size_t plen;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDSA_P256:
            curve = MBEDTLS_ECP_DP_SECP256R1;
            break;

        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P384:
            curve = MBEDTLS_ECP_DP_SECP384R1;
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    keypair = mbedtls_pk_ec(key_data(key)->a.privkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_ecp_group_load((struct mbedtls_ecp_group *)&keypair->grp, curve);

    if (ret) STATUS_INTERNAL_ERROR;

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    plen = mbedtls_mpi_size(&keypair->grp.P);
    if (plen != ecc_blob->cbKey) STATUS_INTERNAL_ERROR;

    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.X, buf + sizeof(*ecc_blob), ecc_blob->cbKey);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.Y, buf + sizeof(*ecc_blob) + ecc_blob->cbKey, ecc_blob->cbKey);
    mbedtls_mpi_lset((struct mbedtls_mpi *)&keypair->Q.Z, 1);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->d, buf + sizeof(*ecc_blob) + 2 * ecc_blob->cbKey, ecc_blob->cbKey);
    ret = mbedtls_ecp_check_pubkey(&keypair->grp, &keypair->Q);

    if (ret) STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_rsa_public(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    mbedtls_rsa_context *rsa_context;
    int ret;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pubkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    ret = mbedtls_rsa_import_raw(rsa_context, buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp, rsa_blob->cbModulus, NULL, 0, NULL, 0, NULL, 0,
        buf + sizeof(*rsa_blob), rsa_blob->cbPublicExp);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_complete(rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pubkey(rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_rsa(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    mbedtls_rsa_context *rsa_context;
    int ret;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.privkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_rsa_import_raw(rsa_context, buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp, rsa_blob->cbModulus,
        buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus, rsa_blob->cbPrime1,
        buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus + rsa_blob->cbPrime1, rsa_blob->cbPrime2,
        NULL, 0, buf + sizeof(*rsa_blob), rsa_blob->cbPublicExp);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_complete(rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_privkey(rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pubkey(rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pub_priv(rsa_context, rsa_context);
    if (ret) STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dh_params(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len)
{
    BCRYPT_DH_PARAMETER_HEADER *dh_params = (BCRYPT_DH_PARAMETER_HEADER *)buf;
    UCHAR *p;
    size_t key_len = dh_params->cbKeyLength;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    if (dh_params->dwMagic != BCRYPT_DH_PARAMETERS_MAGIC && dh_params->dwMagic != BCRYPT_DH_PUBLIC_MAGIC && dh_params->dwMagic != BCRYPT_DH_PRIVATE_MAGIC)
        return STATUS_INVALID_PARAMETER;
    if (len < 2 * key_len) return STATUS_INVALID_PARAMETER;

    p = (UCHAR *)buf + sizeof(*dh_params);

    mbedtls_mpi_read_binary(&dh_key->P, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->G, p, key_len);

    key->u.a.bitlen = dh_params->cbKeyLength * 8;
    dh_key->len = key_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dh_public(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len)
{
    BCRYPT_DH_KEY_BLOB *dh_blob = (BCRYPT_DH_KEY_BLOB *)buf;
    UCHAR *p;
    size_t key_len = dh_blob->cbKey;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    if (dh_blob->dwMagic != BCRYPT_DH_PUBLIC_MAGIC && dh_blob->dwMagic != BCRYPT_DH_PRIVATE_MAGIC)
        return STATUS_INVALID_PARAMETER;
    if (len < 3 * key_len) return STATUS_INVALID_PARAMETER;

    p = (UCHAR *)buf + sizeof(*dh_blob);
    mbedtls_mpi_read_binary(&dh_key->P, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->G, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->GY, p, key_len);

    key->u.a.bitlen = dh_blob->cbKey * 8;
    dh_key->len = key_len;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dh(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len)
{
    BCRYPT_DH_KEY_BLOB *dh_blob = (BCRYPT_DH_KEY_BLOB *)buf;
    UCHAR *p;
    size_t key_len = dh_blob->cbKey;

    if (!dh_key) return STATUS_INVALID_PARAMETER;

    if (dh_blob->dwMagic != BCRYPT_DH_PRIVATE_MAGIC) return STATUS_INVALID_PARAMETER;
    if (len < 4 * key_len) return STATUS_INVALID_PARAMETER;

    p = (UCHAR *)buf + sizeof(*dh_blob);
    mbedtls_mpi_read_binary(&dh_key->P, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->G, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->GY, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->X, p, key_len);

    key->u.a.bitlen = dh_blob->cbKey * 8;
    dh_key->len = key_len;

    return STATUS_SUCCESS;
}

static NTSTATUS init_pk_key(mbedtls_pk_context *key, mbedtls_pk_type_t pk_type, BOOL reinit)
{
    const mbedtls_pk_info_t *info;
    int ret;

    if (pk_type == MBEDTLS_PK_NONE) return STATUS_NOT_SUPPORTED;
    info = mbedtls_pk_info_from_type(pk_type);
    if (info == NULL) return STATUS_NOT_SUPPORTED;
    if (reinit) mbedtls_pk_free(key);
    mbedtls_pk_init(key);
    ret = mbedtls_pk_setup(key, info);
    if (ret) return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;
}

static void init_dh_key(mbedtls_dhm_context *key, BOOL reinit)
{
    if (reinit) mbedtls_dhm_free(key);
    mbedtls_dhm_init(key);
}

NTSTATUS key_asymmetric_import(void *args)
{
    const struct key_asymmetric_import_params *params = args;
    struct key *key = params->key;
    NTSTATUS status = STATUS_SUCCESS;
    int ret;
    mbedtls_pk_type_t pk_type;
    UCHAR buf[1024];
    size_t olen;
    BOOL bPrivate = (params->flags & KEY_IMPORT_FLAG_PUBLIC) == 0;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    pk_type = key_get_type(key->alg_id);
    if (bPrivate)
    {
        switch (key->alg_id)
        {
            case ALG_ID_ECDH_P256:
            case ALG_ID_ECDH_P384:
            case ALG_ID_ECDSA_P256:
            case ALG_ID_ECDSA_P384:
                status = init_pk_key(&key_data(key)->a.privkey, pk_type, TRUE);
                status = key_import_ecc(key, params->buf, params->len);
                break;

            case ALG_ID_RSA:
            case ALG_ID_RSA_SIGN:
                status = init_pk_key(&key_data(key)->a.privkey, pk_type, TRUE);
                status = key_import_rsa(key, params->buf, params->len);
                break;

#if 0 /* mbedtls doesn't support DSA */
            case ALG_ID_DSA:
                break;
#endif
            case ALG_ID_DH:
                init_dh_key(&key_data(key)->a.dh_privkey, TRUE);
                if (params->flags & KEY_IMPORT_FLAG_DH_PARAMETERS)
                    status = key_import_dh_params(key, &key_data(key)->a.dh_privkey, params->buf, params->len);
                else
                    status = key_import_dh(key, &key_data(key)->a.dh_privkey, params->buf, params->len);
                break;

            default:
                FIXME("algorithm %u not yet supported\n", key->alg_id);
                return STATUS_NOT_IMPLEMENTED;
        }

        if (status) return status;
    }
    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            status = init_pk_key(&key_data(key)->a.pubkey, pk_type, TRUE);
            status = key_import_ecc_public(key, params->buf, params->len);
            break;

        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            status = init_pk_key(&key_data(key)->a.pubkey, pk_type, TRUE);
            status = key_import_rsa_public(key, params->buf, params->len);
            if (bPrivate)
            {
                ret = mbedtls_rsa_check_pub_priv(mbedtls_pk_rsa(key_data(key)->a.pubkey), mbedtls_pk_rsa(key_data(key)->a.privkey));
                if (ret) STATUS_INTERNAL_ERROR;
            }
            break;

#if 0 /* mbedtls doesn't support DSA */
        case ALG_ID_DSA:
            break;
#endif
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh_pubkey, TRUE);
            if (params->flags & KEY_IMPORT_FLAG_DH_PARAMETERS)
            {
                status = key_import_dh_params(key, &key_data(key)->a.dh_pubkey, params->buf, params->len);
                mbedtls_dhm_make_params(&key_data(key)->a.dh_pubkey, (key->u.a.bitlen + 7) / 8, buf, &olen, mbedtls_ctr_drbg_random, &ctr_drbg);
            }
            else
                status = key_import_dh_public(key, &key_data(key)->a.dh_pubkey, params->buf, params->len);
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    return status;
}

#ifdef DH_GEN_PRIME
static int dh_gen_prime(mbedtls_dhm_context *dh, int nbits, int (*f_rng)(void *, unsigned char *, size_t), mbedtls_ctr_drbg_context *ctr_drbg)
{
    int ret;
    mbedtls_mpi Q;
    mbedtls_mpi_init(&Q);
    ret = mbedtls_mpi_read_string(&dh->G, 10, "4");
    ret = mbedtls_mpi_gen_prime(&dh->P, nbits, MBEDTLS_MPI_GEN_PRIME_FLAG_DH, mbedtls_ctr_drbg_random, ctr_drbg);
    // Verifying that Q = (P-1)/2 is prime
    ret = mbedtls_mpi_sub_int(&Q, &dh->P, 1);
    ret = mbedtls_mpi_div_int(&Q, NULL, &Q, 2);
    ret = mbedtls_mpi_is_prime_ext(&Q, 50, f_rng, ctr_drbg);
    mbedtls_mpi_free(&Q);
    return ret;
}
#endif

NTSTATUS key_asymmetric_generate(void *args)
{
    struct key *key = args;
    mbedtls_pk_type_t pk_type;
    mbedtls_ecp_group_id grp_id;
    int ret;
    NTSTATUS status;
    UCHAR buf[1024];
    ULONG len;
    size_t olen;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    pk_type = key_get_type(key->alg_id);

    switch (key->alg_id)
    {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            status = init_pk_key(&key_data(key)->a.privkey, pk_type, FALSE);
            mbedtls_rsa_gen_key(mbedtls_pk_rsa(key_data(key)->a.privkey), mbedtls_ctr_drbg_random, &ctr_drbg, key->u.a.bitlen, 65537);
            mbedtls_rsa_complete(mbedtls_pk_rsa(key_data(key)->a.privkey));
            mbedtls_rsa_check_privkey(mbedtls_pk_rsa(key_data(key)->a.privkey));
            mbedtls_rsa_check_pubkey(mbedtls_pk_rsa(key_data(key)->a.privkey));
            break;
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P384:
            switch (key->alg_id)
            {
                case ALG_ID_ECDH_P256:
                case ALG_ID_ECDSA_P256:
                    grp_id = MBEDTLS_ECP_DP_SECP256R1;
                    break;
                case ALG_ID_ECDH_P384:
                case ALG_ID_ECDSA_P384:
                    grp_id = MBEDTLS_ECP_DP_SECP384R1;
                    break;
                default:
                    grp_id = MBEDTLS_ECP_DP_NONE;
            }
            status = init_pk_key(&key_data(key)->a.privkey, pk_type, FALSE);
            ret = mbedtls_ecp_gen_key(grp_id, mbedtls_pk_ec(key_data(key)->a.privkey), mbedtls_ctr_drbg_random, &ctr_drbg);
            break;
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh_privkey, FALSE);
#ifdef DH_GEN_PRIME
            dh_gen_prime(&key_data(key)->a.dh_privkey, key->u.a.bitlen, mbedtls_ctr_drbg_random, &ctr_drbg);
#endif
            mbedtls_dhm_make_params(&key_data(key)->a.dh_privkey, (key->u.a.bitlen + 7) / 8, buf, &olen, mbedtls_ctr_drbg_random, &ctr_drbg);
            break;

#if 0 /* mbedtls doesn't support DSA */
        case ALG_ID_DSA:
            break;
#endif
        default:
            FIXME("unsupported alg_id %d\n", key->alg_id);
            ret = MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
            break;
    }

    if (ret != 0)
    {
        mbedtls_pk_free(&key_data(key)->a.privkey);
        return STATUS_INTERNAL_ERROR;
    }

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            status = init_pk_key(&key_data(key)->a.pubkey, pk_type, FALSE);
            status = key_export_ecc(key, buf, sizeof(buf), &len);
            status = key_import_ecc_public(key, buf, len);
            break;

        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            status = init_pk_key(&key_data(key)->a.pubkey, pk_type, FALSE);
            status = key_export_rsa(key, 0, buf, sizeof(buf), &len);
            status = key_import_rsa_public(key, buf, len);
            break;

#if 0 /* mbedtls doesn't support DSA */
        case ALG_ID_DSA:
            break;
#endif
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh_pubkey, FALSE);
            status = key_export_dh(key, &key_data(key)->a.dh_privkey, buf, sizeof(buf), &len);
            status = key_import_dh_public(key, &key_data(key)->a.dh_pubkey, buf, len);
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    if (status) return status;

    return STATUS_SUCCESS;
}

static mbedtls_md_type_t get_digest_from_id(const WCHAR *alg_id)
{
    if (!wcscmp(alg_id, BCRYPT_SHA1_ALGORITHM)) return MBEDTLS_MD_SHA1;
    if (!wcscmp(alg_id, BCRYPT_SHA256_ALGORITHM)) return MBEDTLS_MD_SHA256;
    if (!wcscmp(alg_id, BCRYPT_SHA384_ALGORITHM)) return MBEDTLS_MD_SHA384;
    if (!wcscmp(alg_id, BCRYPT_SHA512_ALGORITHM)) return MBEDTLS_MD_SHA512;
    if (!wcscmp(alg_id, BCRYPT_MD2_ALGORITHM)) return MBEDTLS_MD_MD2;
    if (!wcscmp(alg_id, BCRYPT_MD5_ALGORITHM)) return MBEDTLS_MD_MD5;
    return MBEDTLS_MD_NONE;
}

NTSTATUS key_asymmetric_verify(void *args)
{
    const struct key_asymmetric_verify_params *params = args;
    struct key *key = params->key;
    mbedtls_md_type_t hash_alg = MBEDTLS_MD_NONE;
    mbedtls_pk_type_t pk_type = key_get_type(key->alg_id);
    mbedtls_pk_rsassa_pss_options pss_options;
    UCHAR *out_signature;
    ULONG out_signature_len;
    UCHAR *p, *start;
    size_t len;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
        {
            if (params->flags) FIXME("flags %x not supported\n", params->flags);

            /* only the hash size must match, not the actual hash function */
            switch (params->hash_len)
            {
                case 20:
                    hash_alg = MBEDTLS_MD_SHA1;
                    break;
                case 32:
                    hash_alg = MBEDTLS_MD_SHA256;
                    break;
                case 48:
                    hash_alg = MBEDTLS_MD_SHA384;
                    break;

                default:
                    FIXME("hash size %u not yet supported\n", params->hash_len);
                    return STATUS_INVALID_SIGNATURE;
            }
            break;
        }
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
        {
            if (params->flags & BCRYPT_PAD_PKCS1)
            {
                BCRYPT_PKCS1_PADDING_INFO *info = params->padding;

                if (!info) return STATUS_INVALID_PARAMETER;
                if (!info->pszAlgId)
                {
                    hash_alg = MBEDTLS_MD_NONE;
                }
                else
                {
                    if ((hash_alg = get_digest_from_id(info->pszAlgId)) == MBEDTLS_MD_NONE)
                    {
                        FIXME("hash algorithm %s not supported\n", debugstr_w(info->pszAlgId));
                        return STATUS_NOT_SUPPORTED;
                    }
                }
            }
            else if (params->flags & BCRYPT_PAD_PSS)
            {
                BCRYPT_PSS_PADDING_INFO *info = params->padding;

                if (!info) return STATUS_INVALID_PARAMETER;
                if (!info->pszAlgId) return STATUS_INVALID_SIGNATURE;
                if ((hash_alg = get_digest_from_id(info->pszAlgId)) == MBEDTLS_MD_NONE)
                {
                    FIXME("hash algorithm %s not supported\n", debugstr_w(info->pszAlgId));
                    return STATUS_NOT_SUPPORTED;
                }
                pss_options.mgf1_hash_id = hash_alg;
                pss_options.expected_salt_len = info->cbSalt;
                pk_type = MBEDTLS_PK_RSASSA_PSS;
            }
            else
                return STATUS_INVALID_PARAMETER;
            break;
        }
#if 0 /* mbedtls doesn't support DSA */
        case ALG_ID_DSA:
        {
            if (params->flags) FIXME( "flags %x not supported\n", params->flags );
            if (params->hash_len != 20)
            {
                FIXME( "hash size %u not supported\n", params->hash_len );
                return STATUS_INVALID_PARAMETER;
            }
            hash_alg = MBEDTLS_MD_SHA1;
            break;
        }
#endif
        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    if (key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384)
    {
        len = params->signature_len / 2;
        out_signature = malloc(params->signature_len + 6);
        p = out_signature + params->signature_len + 6;
        start = out_signature;
        mbedtls_asn1_write_raw_buffer(&p, start, params->signature + len, len);
        mbedtls_asn1_write_len(&p, start, len);
        mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_INTEGER);
        mbedtls_asn1_write_raw_buffer(&p, start, params->signature, len);
        mbedtls_asn1_write_len(&p, start, len);
        mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_INTEGER);
        mbedtls_asn1_write_len(&p, start, params->signature_len + 4);
        mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        out_signature_len = params->signature_len + 6;
    }
    else
    {
        out_signature = params->signature;
        out_signature_len = params->signature_len;
    }

    ret = mbedtls_pk_verify_ext(pk_type, (params->flags & BCRYPT_PAD_PSS) == 0 ? NULL : &pss_options, &key_data(params->key)->a.pubkey,
        hash_alg, params->hash, params->hash_len, out_signature, out_signature_len);
    if (out_signature != params->signature) free(out_signature);
    if (ret) return STATUS_INVALID_SIGNATURE;

    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_sign(void *args)
{
    const struct key_asymmetric_sign_params *params = args;
    struct key *key = params->key;
    size_t sig_len = 0;
    mbedtls_md_type_t hash_alg = MBEDTLS_MD_NONE;
    unsigned int flags = params->flags;
    UCHAR *p, *end;
    size_t len, expected_len;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    if (key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384)
    {
        /* With ECDSA, we find the digest algorithm from the hash length, and verify it */
        switch (params->input_len)
        {
            case 20:
                hash_alg = MBEDTLS_MD_SHA1;
                break;
            case 32:
                hash_alg = MBEDTLS_MD_SHA256;
                break;
            case 48:
                hash_alg = MBEDTLS_MD_SHA384;
                break;
            case 64:
                hash_alg = MBEDTLS_MD_SHA512;
                break;

            default:
                FIXME("hash size %u not yet supported\n", params->input_len);
                return STATUS_INVALID_PARAMETER;
        }

        if (flags == BCRYPT_PAD_PKCS1)
        {
            BCRYPT_PKCS1_PADDING_INFO *pad = params->padding;
            if (pad && pad->pszAlgId && get_digest_from_id(pad->pszAlgId) != hash_alg)
            {
                WARN("incorrect hashing algorithm %s, expected %u\n", debugstr_w(pad->pszAlgId), hash_alg);
                return STATUS_INVALID_PARAMETER;
            }
        }
    }
    else if (key->alg_id == ALG_ID_DSA)
    {
        if (flags) FIXME("flags %#x not supported\n", flags);
        if (params->input_len != 20)
        {
            FIXME("hash size %u not supported\n", params->input_len);
            return STATUS_INVALID_PARAMETER;
        }
        hash_alg = MBEDTLS_MD_SHA1;
    }
    else if (flags == BCRYPT_PAD_PKCS1)
    {
        BCRYPT_PKCS1_PADDING_INFO *pad = params->padding;

        if (!pad)
        {
            WARN("padding info not found\n");
            return STATUS_INVALID_PARAMETER;
        }
        if (!pad->pszAlgId) hash_alg = MBEDTLS_MD_NONE;
        else if ((hash_alg = get_digest_from_id(pad->pszAlgId)) == MBEDTLS_MD_NONE)
        {
            FIXME("hash algorithm %s not recognized\n", debugstr_w(pad->pszAlgId));
            return STATUS_NOT_SUPPORTED;
        }
    }
    else if (flags == BCRYPT_PAD_PSS)
    {
        BCRYPT_PSS_PADDING_INFO *pad = params->padding;

        if (!pad || !pad->pszAlgId)
        {
            WARN("padding info not found\n");
            return STATUS_INVALID_PARAMETER;
        }
        if (key->alg_id != ALG_ID_RSA && key->alg_id != ALG_ID_RSA_SIGN)
        {
            FIXME("BCRYPT_PAD_PSS not supported for key algorithm %u\n", key->alg_id);
            return STATUS_NOT_SUPPORTED;
        }
        if ((hash_alg = get_digest_from_id(pad->pszAlgId)) == MBEDTLS_MD_NONE)
        {
            FIXME("hash algorithm %s not recognized\n", debugstr_w(pad->pszAlgId));
            return STATUS_NOT_SUPPORTED;
        }

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(key)->a.privkey), MBEDTLS_RSA_PKCS_V21, hash_alg);
    }
    else if (!flags)
    {
        WARN("invalid flags %#x\n", flags);
        return STATUS_INVALID_PARAMETER;
    }
    else
    {
        FIXME("flags %#x not implemented\n", flags);
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!params->output)
    {
        *params->ret_len = key->u.a.bitlen / 8;
        return STATUS_SUCCESS;
    }
    if (!key_data(key)->a.privkey.pk_info) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_pk_sign(&key_data(params->key)->a.privkey, hash_alg, params->input, params->input_len, params->output, &sig_len,
                          mbedtls_ctr_drbg_random, &ctr_drbg);
    if (key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384)
    {
        p = params->output;
        end = params->output + sig_len;
        // expected_len = mbedtls_md_get_size( mbedtls_md_info_from_type( hash_alg ) ) / 2;
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
        expected_len = (len / 2) * 2;
        if (!ret && expected_len < len) p += len - expected_len;
        if (!ret) sig_len = expected_len;
        memcpy(params->output, p, len);
        p += expected_len;
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
        expected_len = (len / 2) * 2;
        if (!ret && expected_len < len) p += len - expected_len;
        if (!ret) memcpy(params->output + sig_len, p, len);
        if (!ret) sig_len += expected_len;
    }

    *params->ret_len = sig_len;
    if (sig_len > params->output_len) return STATUS_BUFFER_TOO_SMALL;
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_destroy(void *args)
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    mbedtls_pk_free(&key_data(key)->a.privkey);
    mbedtls_pk_free(&key_data(key)->a.pubkey);
    mbedtls_dhm_free(&key_data(key)->a.dh_privkey);
    mbedtls_dhm_free(&key_data(key)->a.dh_pubkey);
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_duplicate(void *args)
{
    const struct key_asymmetric_duplicate_params *params = args;
    mbedtls_pk_type_t pk_type;
    unsigned char buf[2048];
    ULONG len;
    NTSTATUS status;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    pk_type = key_get_type(params->key_orig->alg_id);
    init_pk_key(&key_data(params->key_copy)->a.privkey, pk_type, TRUE);
    init_pk_key(&key_data(params->key_copy)->a.pubkey, pk_type, TRUE);
    init_dh_key(&key_data(params->key_copy)->a.dh_privkey, TRUE);
    init_dh_key(&key_data(params->key_copy)->a.dh_pubkey, TRUE);

    switch (params->key_orig->alg_id)
    {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
        {
            status = key_export_rsa(params->key_orig, KEY_EXPORT_FLAG_RSA_FULL, buf, sizeof(buf), &len);
            status = key_import_rsa(params->key_copy, buf, len);
            status = key_export_rsa_public(params->key_orig, buf, sizeof(buf), &len);
            if (status) break;
            status = key_import_rsa_public(params->key_copy, buf, len);
            break;
        }
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        {
            status = key_export_ecc(params->key_orig, buf, sizeof(buf), &len);
            status = key_import_ecc(params->key_copy, buf, len);
            status = key_export_ecc_public(params->key_orig, buf, sizeof(buf), &len);
            if (status) break;
            status = key_import_ecc_public(params->key_copy, buf, len);
            break;
        }
        case ALG_ID_DH:
        {
            status = key_export_dh(params->key_orig, &key_data(params->key_orig)->a.dh_privkey, buf, sizeof(buf), &len);
            status = key_import_dh(params->key_copy, &key_data(params->key_copy)->a.dh_privkey, buf, len);
            status = key_export_dh_public(params->key_orig, &key_data(params->key_orig)->a.dh_pubkey, buf, sizeof(buf), &len);
            if (status) break;
            status = key_import_dh_public(params->key_copy, &key_data(params->key_copy)->a.dh_pubkey, buf, len);
            break;
        }
        default:
            FIXME("unsupported algorithm %u\n", params->key_orig->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    if (!status) return STATUS_SUCCESS;

    mbedtls_pk_free(&key_data(params->key_copy)->a.privkey);
    mbedtls_pk_free(&key_data(params->key_copy)->a.pubkey);
    return STATUS_INTERNAL_ERROR;
}

NTSTATUS key_asymmetric_decrypt(void *args)
{
    const struct key_asymmetric_decrypt_params *params = args;
    size_t olen;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    if (!(params->key->u.a.flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    if (params->key->alg_id == ALG_ID_RSA && params->flags & BCRYPT_PAD_OAEP)
    {
        BCRYPT_OAEP_PADDING_INFO *pad = params->padding;
        mbedtls_md_type_t dig;

        if (!pad || !pad->pszAlgId)
        {
            WARN("padding info not found\n");
            return STATUS_INVALID_PARAMETER;
        }
        if ((dig = get_digest_from_id(pad->pszAlgId)) == MBEDTLS_MD_NONE)
        {
            FIXME("hash algorithm %s not recognized\n", debugstr_w(pad->pszAlgId));
            return STATUS_NOT_SUPPORTED;
        }

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(params->key)->a.privkey), MBEDTLS_RSA_PKCS_V21, dig);
    }
    ret = mbedtls_pk_decrypt(&key_data(params->key)->a.privkey, params->input, params->input_len, params->output, &olen, params->output_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret) return STATUS_INTERNAL_ERROR;

    *params->ret_len = olen;
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_encrypt(void *args)
{
    const struct key_asymmetric_encrypt_params *params = args;
    size_t olen;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    if (!(params->key->u.a.flags & KEY_FLAG_FINALIZED)) return STATUS_INVALID_HANDLE;

    if (params->key->alg_id == ALG_ID_RSA && params->flags & BCRYPT_PAD_OAEP)
    {
        BCRYPT_OAEP_PADDING_INFO *pad = params->padding;
        mbedtls_md_type_t dig;

        if (!pad || !pad->pszAlgId || !pad->pbLabel)
        {
            WARN("padding info not found\n");
            return STATUS_INVALID_PARAMETER;
        }
        if ((dig = get_digest_from_id(pad->pszAlgId)) == MBEDTLS_MD_NONE)
        {
            FIXME("hash algorithm %s not recognized\n", debugstr_w(pad->pszAlgId));
            return STATUS_NOT_SUPPORTED;
        }

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(params->key)->a.pubkey), MBEDTLS_RSA_PKCS_V21, dig);
    }

    ret = mbedtls_pk_encrypt(&key_data(params->key)->a.pubkey, params->input, params->input_len, params->output, &olen, params->output_len,
        mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret) return STATUS_INTERNAL_ERROR;

    *params->ret_len = olen;
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_derive_key(void *args)
{
    const struct key_asymmetric_derive_key_params *params = args;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    switch (params->privkey->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        {
            mbedtls_ecdh_context ecdh;

            mbedtls_ecdh_init(&ecdh);
            mbedtls_ecp_group_load(&ecdh.grp, mbedtls_pk_ec(key_data(params->privkey)->a.privkey)->grp.id);
            mbedtls_mpi_copy(&ecdh.d, &mbedtls_pk_ec(key_data(params->privkey)->a.privkey)->d);
            mbedtls_ecp_copy(&ecdh.Qp, &mbedtls_pk_ec(key_data(params->pubkey)->a.pubkey)->Q);

            *params->ret_len = mbedtls_mpi_size(&ecdh.grp.P);
            if (params->output)
            {
                if (params->output_len < *params->ret_len)
                {
                    mbedtls_ecdh_free(&ecdh);
                    return STATUS_BUFFER_TOO_SMALL;
                }
                ret = mbedtls_ecdh_compute_shared(&ecdh.grp, &ecdh.z, &ecdh.Qp, &ecdh.d, mbedtls_ctr_drbg_random, &ctr_drbg);
                if (ret)
                {
                    FIXME("mbedtls_ecdh_compute_shared failed: %d\n", ret);
                    mbedtls_ecdh_free(&ecdh);
                    return STATUS_UNSUCCESSFUL;
                }
                mbedtls_mpi_write_binary(&ecdh.z, params->output, *params->ret_len);
            }
            mbedtls_ecdh_free(&ecdh);
            break;
        }
        case ALG_ID_DH:
        {
            mbedtls_dhm_context *dhm = &key_data(params->privkey)->a.dh_privkey;
            mbedtls_dhm_context *dhm_pub = &key_data(params->pubkey)->a.dh_pubkey;
            size_t secret_len;

            mbedtls_mpi_copy(&dhm->GY, &dhm_pub->GY);

            *params->ret_len = mbedtls_mpi_size(&dhm->P);
            if (params->output)
            {
                if (params->output_len < *params->ret_len) return STATUS_BUFFER_TOO_SMALL;
                ret = mbedtls_dhm_calc_secret(dhm, params->output, params->output_len, &secret_len, mbedtls_ctr_drbg_random, &ctr_drbg);
                if (ret)
                {
                    FIXME("mbedtls_dhm_calc_secret failed with %d\n", ret);
                    return STATUS_UNSUCCESSFUL;
                }
                *params->ret_len = secret_len;
            }
            break;
        }
        default:
            FIXME("unsupported algorithm %u\n", params->privkey->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}

#endif /* SONAME_LIBMBEDTLS */
