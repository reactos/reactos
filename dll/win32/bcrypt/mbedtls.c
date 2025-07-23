#include "precomp.h"

#ifdef SONAME_LIBMBEDTLS

#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/cipher.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/dhm.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/dsa.h>

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

#ifndef __REACTOS__
static void *libmbedtls_handle;
#endif
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

union key_data
{
    union
    {
        mbedtls_cipher_context_t cipher_ctx;
    } s;
    union
    {
        struct
        {
            mbedtls_pk_context privkey;
            mbedtls_pk_context pubkey;
        } pk;
        struct
        {
            mbedtls_dhm_context privkey;
            mbedtls_dhm_context pubkey;
        } dh;
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        struct
        {
            mbedtls_dsa_context privkey;
            mbedtls_dsa_context pubkey;
        } dsa;
#endif
    } a;
};
C_ASSERT(sizeof(union key_data) <= sizeof(((struct key *)0)->private));

static union key_data *key_data(struct key *key)
{
    return (union key_data *)key->private;
}

#ifndef __REACTOS__
#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(mbedtls_md_init)
MAKE_FUNCPTR(mbedtls_md_info_from_type)
MAKE_FUNCPTR(mbedtls_md_get_size)
MAKE_FUNCPTR(mbedtls_md_setup)
MAKE_FUNCPTR(mbedtls_md_update)
MAKE_FUNCPTR(mbedtls_md_starts)
MAKE_FUNCPTR(mbedtls_md_finish)
MAKE_FUNCPTR(mbedtls_md_hmac_update)
MAKE_FUNCPTR(mbedtls_md_hmac_starts)
MAKE_FUNCPTR(mbedtls_md_hmac_finish)
MAKE_FUNCPTR(mbedtls_md_free)
MAKE_FUNCPTR(mbedtls_cipher_init)
MAKE_FUNCPTR(mbedtls_cipher_setup)
MAKE_FUNCPTR(mbedtls_cipher_info_from_type)
MAKE_FUNCPTR(mbedtls_cipher_setkey)
MAKE_FUNCPTR(mbedtls_cipher_set_iv)
MAKE_FUNCPTR(mbedtls_cipher_set_padding_mode)
MAKE_FUNCPTR(mbedtls_cipher_update)
MAKE_FUNCPTR(mbedtls_cipher_update_ad)
MAKE_FUNCPTR(mbedtls_cipher_check_tag)
MAKE_FUNCPTR(mbedtls_cipher_write_tag)
MAKE_FUNCPTR(mbedtls_cipher_finish)
MAKE_FUNCPTR(mbedtls_cipher_free)
MAKE_FUNCPTR(mbedtls_ctr_drbg_init)
MAKE_FUNCPTR(mbedtls_ctr_drbg_random)
MAKE_FUNCPTR(mbedtls_ctr_drbg_seed)
MAKE_FUNCPTR(mbedtls_ctr_drbg_free)
MAKE_FUNCPTR(mbedtls_entropy_init)
MAKE_FUNCPTR(mbedtls_entropy_func)
MAKE_FUNCPTR(mbedtls_entropy_free)
MAKE_FUNCPTR(mbedtls_pk_init)
MAKE_FUNCPTR(mbedtls_pk_get_type)
MAKE_FUNCPTR(mbedtls_pk_setup)
MAKE_FUNCPTR(mbedtls_pk_info_from_type)
MAKE_FUNCPTR(mbedtls_pk_parse_key)
MAKE_FUNCPTR(mbedtls_pk_parse_public_key)
MAKE_FUNCPTR(mbedtls_pk_verify_ext)
MAKE_FUNCPTR(mbedtls_pk_sign)
MAKE_FUNCPTR(mbedtls_pk_encrypt)
MAKE_FUNCPTR(mbedtls_pk_decrypt)
MAKE_FUNCPTR(mbedtls_pk_free)
MAKE_FUNCPTR(mbedtls_ecdh_init)
MAKE_FUNCPTR(mbedtls_ecdh_compute_shared)
MAKE_FUNCPTR(mbedtls_ecdh_free)
MAKE_FUNCPTR(mbedtls_rsa_gen_key)
MAKE_FUNCPTR(mbedtls_rsa_export)
MAKE_FUNCPTR(mbedtls_rsa_import_raw)
MAKE_FUNCPTR(mbedtls_rsa_export_raw)
MAKE_FUNCPTR(mbedtls_rsa_export_crt)
MAKE_FUNCPTR(mbedtls_rsa_set_padding)
MAKE_FUNCPTR(mbedtls_rsa_complete)
MAKE_FUNCPTR(mbedtls_rsa_check_pubkey)
MAKE_FUNCPTR(mbedtls_rsa_check_privkey)
MAKE_FUNCPTR(mbedtls_rsa_check_pub_priv)
MAKE_FUNCPTR(mbedtls_ecp_gen_key)
MAKE_FUNCPTR(mbedtls_ecp_group_load)
MAKE_FUNCPTR(mbedtls_ecp_point_read_binary)
MAKE_FUNCPTR(mbedtls_ecp_point_write_binary)
MAKE_FUNCPTR(mbedtls_ecp_check_pubkey)
MAKE_FUNCPTR(mbedtls_ecp_copy)
MAKE_FUNCPTR(mbedtls_mpi_init)
MAKE_FUNCPTR(mbedtls_mpi_size)
MAKE_FUNCPTR(mbedtls_mpi_lset)
MAKE_FUNCPTR(mbedtls_mpi_copy)
MAKE_FUNCPTR(mbedtls_mpi_cmp_mpi)
MAKE_FUNCPTR(mbedtls_mpi_cmp_int)
MAKE_FUNCPTR(mbedtls_mpi_read_binary)
MAKE_FUNCPTR(mbedtls_mpi_write_binary)
MAKE_FUNCPTR(mbedtls_mpi_read_string)
MAKE_FUNCPTR(mbedtls_mpi_gen_prime)
MAKE_FUNCPTR(mbedtls_mpi_sub_int)
MAKE_FUNCPTR(mbedtls_mpi_div_int)
MAKE_FUNCPTR(mbedtls_mpi_is_prime_ext)
MAKE_FUNCPTR(mbedtls_mpi_free)
MAKE_FUNCPTR(mbedtls_asn1_get_len)
MAKE_FUNCPTR(mbedtls_asn1_get_mpi)
MAKE_FUNCPTR(mbedtls_asn1_get_tag)
MAKE_FUNCPTR(mbedtls_asn1_write_len)
MAKE_FUNCPTR(mbedtls_asn1_write_mpi)
MAKE_FUNCPTR(mbedtls_asn1_write_tag)
MAKE_FUNCPTR(mbedtls_asn1_write_raw_buffer)
MAKE_FUNCPTR(mbedtls_dhm_init)
MAKE_FUNCPTR(mbedtls_dhm_make_params)
MAKE_FUNCPTR(mbedtls_dhm_calc_secret)
MAKE_FUNCPTR(mbedtls_dhm_free)
MAKE_FUNCPTR(mbedtls_dsa_init)
MAKE_FUNCPTR(mbedtls_dsa_set_group)
MAKE_FUNCPTR(mbedtls_dsa_genkey)
MAKE_FUNCPTR(mbedtls_dsa_check_pqg)
MAKE_FUNCPTR(mbedtls_dsa_check_pubkey)
MAKE_FUNCPTR(mbedtls_dsa_check_privkey)
MAKE_FUNCPTR(mbedtls_dsa_pubkey_from_privkey)
MAKE_FUNCPTR(mbedtls_dsa_verify)
MAKE_FUNCPTR(mbedtls_dsa_sign)
MAKE_FUNCPTR(mbedtls_dsa_free)
#undef MAKE_FUNCPTR
#endif

NTSTATUS process_attach(void *args)
{
#ifndef __REACTOS__
    if (!(libmbedtls_handle = wine_dlopen(SONAME_LIBMBEDTLS, RTLD_NOW, NULL, 0)))
    {
        ERR("failed to load libmbedtls, no support for crypto hashes\n");
        return STATUS_DLL_NOT_FOUND;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym(libmbedtls_handle, #f, NULL, 0))) \
    { \
        ERR("failed to load %s\n", #f); \
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
    LOAD_FUNCPTR(mbedtls_pk_parse_key)
    LOAD_FUNCPTR(mbedtls_pk_parse_public_key)
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
    LOAD_FUNCPTR(mbedtls_mpi_cmp_int)
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
    LOAD_FUNCPTR(mbedtls_dsa_init)
    LOAD_FUNCPTR(mbedtls_dsa_set_group)
    LOAD_FUNCPTR(mbedtls_dsa_genkey)
    LOAD_FUNCPTR(mbedtls_dsa_check_pqg)
    LOAD_FUNCPTR(mbedtls_dsa_check_pubkey)
    LOAD_FUNCPTR(mbedtls_dsa_check_privkey)
    LOAD_FUNCPTR(mbedtls_dsa_pubkey_from_privkey)
    LOAD_FUNCPTR(mbedtls_dsa_verify)
    LOAD_FUNCPTR(mbedtls_dsa_sign)
    LOAD_FUNCPTR(mbedtls_dsa_free)
#undef LOAD_FUNCPTR
#endif
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

    return STATUS_SUCCESS;
#ifndef __REACTOS__
fail:
    wine_dlclose(libmbedtls_handle, NULL, 0);
    libmbedtls_handle = NULL;
    return STATUS_DLL_NOT_FOUND;
#endif
}

#ifndef __REACTOS__
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
#define mbedtls_pk_parse_key            pmbedtls_pk_parse_key
#define mbedtls_pk_parse_public_key     pmbedtls_pk_parse_public_key
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
#define mbedtls_mpi_cmp_int             pmbedtls_mpi_cmp_int
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
#define mbedtls_dsa_init                pmbedtls_dsa_init
#define mbedtls_dsa_set_group           pmbedtls_dsa_set_group
#define mbedtls_dsa_genkey              pmbedtls_dsa_genkey
#define mbedtls_dsa_check_pqg           pmbedtls_dsa_check_pqg
#define mbedtls_dsa_check_pubkey        pmbedtls_dsa_check_pubkey
#define mbedtls_dsa_check_privkey       pmbedtls_dsa_check_privkey
#define mbedtls_dsa_pubkey_from_privkey pmbedtls_dsa_pubkey_from_privkey
#define mbedtls_dsa_verify              pmbedtls_dsa_verify
#define mbedtls_dsa_sign                pmbedtls_dsa_sign
#define mbedtls_dsa_free                pmbedtls_dsa_free
#endif

NTSTATUS process_detach(void *args)
{
#ifndef __REACTOS__
    if (libmbedtls_handle)
    {
#endif
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
#ifndef __REACTOS__
        wine_dlclose(libmbedtls_handle, NULL, 0);
        libmbedtls_handle = NULL;
    }
#endif
    return STATUS_SUCCESS;
}

NTSTATUS hash_init(struct hash *hash)
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

NTSTATUS hash_update(struct hash *hash, UCHAR *input, ULONG size)
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

NTSTATUS hash_finish(struct hash *hash, UCHAR *output)
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

NTSTATUS hash_get_size(struct hash *hash, ULONG *output)
{
#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (!hash->hash_ctx.md_info) return STATUS_INTERNAL_ERROR;
    if (!output) return STATUS_INTERNAL_ERROR;
    *output = mbedtls_md_get_size(hash->hash_ctx.md_info);

    return STATUS_SUCCESS;
}

mbedtls_cipher_type_t key_get_cipher(const struct key *key)
{
    switch (key->alg_id)
    {
        case ALG_ID_RC4:
            WARN("handle block size\n");
            return MBEDTLS_CIPHER_ARC4_128;

        case ALG_ID_3DES:
            WARN("handle block size\n");
            switch (key->u.s.mode)
            {
                case CHAIN_MODE_ECB:
                    return MBEDTLS_CIPHER_DES_EDE3_ECB;
                case CHAIN_MODE_CBC:
                    return MBEDTLS_CIPHER_DES_EDE3_CBC;
                default:
                    break;
            }
            FIXME("3DES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len);
            return MBEDTLS_CIPHER_NONE;

        case ALG_ID_AES:
            WARN("handle block size\n");
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
            FIXME("AES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len);
            return MBEDTLS_CIPHER_NONE;

        default:
            FIXME("algorithm %u not supported\n", key->alg_id);
            return MBEDTLS_CIPHER_NONE;
    }
    return MBEDTLS_CIPHER_NONE;
}

NTSTATUS key_symmetric_vector_reset(void *args)
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->s.cipher_ctx.cipher_info != NULL && key->u.s.vector != NULL && key->u.s.vector_len)
    {
        if (mbedtls_cipher_set_iv(&key_data(key)->s.cipher_ctx, key->u.s.vector, key->u.s.vector_len)) return STATUS_INTERNAL_ERROR;
        return STATUS_SUCCESS;
    }
    TRACE("invalidating cipher handle\n");
    mbedtls_cipher_free(&key_data(key)->s.cipher_ctx);
    key_data(key)->s.cipher_ctx.cipher_info = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS init_cipher_handle(struct key *key, const mbedtls_operation_t operation)
{
    mbedtls_cipher_type_t cipher;
    const mbedtls_cipher_info_t *info;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->s.cipher_ctx.cipher_info != NULL)
    {
        if (mbedtls_cipher_get_operation(&key_data(key)->s.cipher_ctx) == operation) return STATUS_SUCCESS;
        mbedtls_cipher_free(&key_data(key)->s.cipher_ctx);
        key_data(key)->s.cipher_ctx.cipher_info = NULL;
    }
    cipher = key_get_cipher(key);
    if (cipher == MBEDTLS_CIPHER_NONE) return STATUS_NOT_SUPPORTED;

    if ((info = mbedtls_cipher_info_from_type(cipher)) == NULL) return STATUS_INTERNAL_ERROR;
    mbedtls_cipher_init(&key_data(key)->s.cipher_ctx);
    if (mbedtls_cipher_setup(&key_data(key)->s.cipher_ctx, info))
    {
        mbedtls_cipher_free(&key_data(key)->s.cipher_ctx);
        return STATUS_INTERNAL_ERROR;
    }
    mbedtls_cipher_set_padding_mode(&key_data(key)->s.cipher_ctx, MBEDTLS_PADDING_NONE);
    if (key->u.s.secret != NULL && key->u.s.secret_len > 0) mbedtls_cipher_setkey(&key_data(key)->s.cipher_ctx, key->u.s.secret, key->u.s.secret_len * 8, operation);
    if (key->u.s.vector != NULL && key->u.s.vector_len > 0) mbedtls_cipher_set_iv(&key_data(key)->s.cipher_ctx, key->u.s.vector, key->u.s.vector_len);

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_set_auth_data(void *args)
{
    struct key_symmetric_set_auth_data_params *auth_params = args;
    NTSTATUS status;
    int ret;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle(auth_params->key, auth_params->encrypt ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT))) return status;

    ret = mbedtls_cipher_update_ad(&key_data(auth_params->key)->s.cipher_ctx, auth_params->auth_data, auth_params->len);
    if (ret) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_get_tag(void *args)
{
    struct key_symmetric_get_tag_params *tag_params = args;
    mbedtls_operation_t operation;
    int ret;
    NTSTATUS status;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    operation = mbedtls_cipher_get_operation(&key_data(tag_params->key)->s.cipher_ctx);
    if ((status = init_cipher_handle(tag_params->key, operation))) return status;

    switch (operation) {
        case MBEDTLS_ENCRYPT:
            ret = mbedtls_cipher_write_tag(&key_data(tag_params->key)->s.cipher_ctx, tag_params->tag, tag_params->len);
            if (ret) return STATUS_INTERNAL_ERROR;
            break;
        case MBEDTLS_DECRYPT:
            ret = mbedtls_cipher_check_tag(&key_data(tag_params->key)->s.cipher_ctx, tag_params->tag, tag_params->len);
            if (ret)
            {
                if (ret == MBEDTLS_ERR_CIPHER_AUTH_FAILED)
                    return STATUS_AUTH_TAG_MISMATCH;
                else
                    return STATUS_INTERNAL_ERROR;
            }
            break;
        default:
            FIXME("AES operation %u not supported\n", operation);
            return STATUS_INTERNAL_ERROR;
            break;
    }

    return status;
}

NTSTATUS key_symmetric_encrypt_internal(void *args)
{
    const struct key_symmetric_encrypt_params *params = args;
    NTSTATUS status;
    int ret;
    size_t output_len = 0;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle(params->key, MBEDTLS_ENCRYPT))) return status;

    ret = mbedtls_cipher_update(&key_data(params->key)->s.cipher_ctx,
                                    params->input, params->input_len,
                                    params->output, &output_len);
    if (!output_len)
        ret = mbedtls_cipher_finish(&key_data(params->key)->s.cipher_ctx,
                                    params->output, &output_len);
    if (ret) return STATUS_INTERNAL_ERROR;

    if (params->output_len != output_len) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_decrypt_internal(void *args)
{
    const struct key_symmetric_decrypt_params *params = args;
    NTSTATUS status;
    int ret;
    size_t output_len = 0;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if ((status = init_cipher_handle(params->key, MBEDTLS_DECRYPT))) return status;

    ret = mbedtls_cipher_update(&key_data(params->key)->s.cipher_ctx,
                                params->input, params->input_len,
                                params->output, &output_len);
    if (!output_len)
        ret = mbedtls_cipher_finish(&key_data(params->key)->s.cipher_ctx,
                                    params->output, &output_len);
    if (ret) return STATUS_INTERNAL_ERROR;

    if (params->output_len != output_len) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

NTSTATUS key_symmetric_destroy(void *args)
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif
    if (key_data(key)->s.cipher_ctx.cipher_info != NULL)
    {
        mbedtls_cipher_free(&key_data(key)->s.cipher_ctx);
        key_data(key)->s.cipher_ctx.cipher_info = NULL;
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

    keypair = mbedtls_pk_ec(key_data(key)->a.pk.pubkey);
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

    keypair = mbedtls_pk_ec(key_data(key)->a.pk.privkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    curve = keypair->grp.id;

    if (curve != MBEDTLS_ECP_DP_SECP256R1 && curve != MBEDTLS_ECP_DP_SECP384R1)
    {
        FIXME("curve %u not supported\n", curve);
        return STATUS_NOT_IMPLEMENTED;
    }


    *ret_len = sizeof(*ecc_blob) + size + size + size;
    if (len >= *ret_len && buf)
    {
        ecc_blob->dwMagic = magic;
        ecc_blob->cbKey = size;

        dst = buf + sizeof(*ecc_blob);
        mbedtls_mpi_write_binary(&keypair->Q.X, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&keypair->Q.Y, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&keypair->d, dst, size);
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

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pk.pubkey);
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

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pk.privkey);
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

static NTSTATUS key_export_params(struct key *key, mbedtls_dhm_context *dh_key, UCHAR *buf, ULONG len, ULONG *ret_len)
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
        mbedtls_mpi_write_binary(&dh_key->GX, dst, size);
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
        mbedtls_mpi_write_binary(&dh_key->GX, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dh_key->X, dst, size);
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
static NTSTATUS key_export_dsa_public(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob = (BCRYPT_DSA_KEY_BLOB *)buf;
    size_t q_size;
    ULONG size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (key->u.a.bitlen > 1024)
    {
        FIXME("bitlen > 1024 not supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    q_size = mbedtls_mpi_size(&dsa_key->Q);

    if (q_size > sizeof(dsa_blob->q)) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*dsa_blob) + size + size + size;
    if (len >= *ret_len && buf)
    {
        dst = buf + sizeof(*dsa_blob);;
        mbedtls_mpi_write_binary(&dsa_key->P, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dsa_key->G, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dsa_key->Y, dst, size);

        dst = dsa_blob->q;
        mbedtls_mpi_write_binary(&dsa_key->Q, dst, sizeof(dsa_blob->q));

        dsa_blob->dwMagic = BCRYPT_DSA_PUBLIC_MAGIC;
        dsa_blob->cbKey   = size;
        memset(dsa_blob->Count, 0, sizeof(dsa_blob->Count)); /* FIXME */
        memset(dsa_blob->Seed, 0, sizeof(dsa_blob->Seed)); /* FIXME */
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static void reverse_bytes(UCHAR *buf, ULONG len)
{
    unsigned int i;
    UCHAR tmp;

    for (i = 0; i < len / 2; ++i)
    {
        tmp = buf[i];
        buf[i] = buf[len - i - 1];
        buf[len - i - 1] = tmp;
    }
}

#define Q_SIZE 20
static NTSTATUS key_export_dsa_capi_public(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BLOBHEADER *hdr = (BLOBHEADER *)buf;
    DSSPUBKEY *dsskey;
    size_t q_size;
    ULONG size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (key->u.a.bitlen > 1024)
    {
        FIXME("bitlen > 1024 not supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    q_size = mbedtls_mpi_size(&dsa_key->Q);
    if (q_size > Q_SIZE) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*hdr) + sizeof(*dsskey) + sizeof(key->u.a.dss_seed) +
               size + Q_SIZE + size + size;
    if (len >= *ret_len && buf)
    {
        hdr->bType    = PUBLICKEYBLOB;
        hdr->bVersion = 2;
        hdr->reserved = 0;
        hdr->aiKeyAlg = CALG_DSS_SIGN;

        dsskey = (DSSPUBKEY *)(hdr + 1);
        dsskey->magic  = MAGIC_DSS1;
        dsskey->bitlen = key->u.a.bitlen;

        dst = (UCHAR *)(dsskey + 1);
        mbedtls_mpi_write_binary(&dsa_key->P, dst, size);
        reverse_bytes(dst, size);
        dst += size;

        mbedtls_mpi_write_binary(&dsa_key->Q, dst, Q_SIZE);
        reverse_bytes(dst, Q_SIZE);
        dst += Q_SIZE;

        mbedtls_mpi_write_binary(&dsa_key->G, dst, size);
        reverse_bytes(dst, size);
        dst += size;

        mbedtls_mpi_write_binary(&dsa_key->Y, dst, size);
        reverse_bytes(dst, size);
        dst += size;

        memcpy(dst, &key->u.a.dss_seed, sizeof(key->u.a.dss_seed));
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_dsa(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob = (BCRYPT_DSA_KEY_BLOB *)buf;
    size_t q_size;
    ULONG size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (key->u.a.bitlen > 1024)
    {
        FIXME("bitlen > 1024 not supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    q_size = mbedtls_mpi_size(&dsa_key->Q);
    if (q_size > sizeof(dsa_blob->q)) return STATUS_INVALID_PARAMETER;

    *ret_len = sizeof(*dsa_blob) + size + size + size + size;
    if (len >= *ret_len && buf)
    {
        dst = buf + sizeof(*dsa_blob);;
        mbedtls_mpi_write_binary(&dsa_key->P, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dsa_key->G, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dsa_key->Y, dst, size);
        dst += size;
        mbedtls_mpi_write_binary(&dsa_key->X, dst, size);

        dst = dsa_blob->q;
        mbedtls_mpi_write_binary(&dsa_key->Q, dst, sizeof(dsa_blob->q));

        dsa_blob->dwMagic = BCRYPT_DSA_PUBLIC_MAGIC;
        dsa_blob->cbKey   = size;
        memset(dsa_blob->Count, 0, sizeof(dsa_blob->Count)); /* FIXME */
        memset(dsa_blob->Seed, 0, sizeof(dsa_blob->Seed)); /* FIXME */
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}

static NTSTATUS key_export_dsa_capi(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len, ULONG *ret_len)
{
    BLOBHEADER *hdr;
    DSSPUBKEY *pubkey;
    size_t q_size, x_size;
    ULONG size = key->u.a.bitlen / 8;
    UCHAR *dst;

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    q_size = mbedtls_mpi_size(&dsa_key->Q);
    x_size = mbedtls_mpi_size(&dsa_key->X);

    if (q_size > 21 || x_size > 21)
    {
        ERR("can't export key in this format\n");
        return STATUS_NOT_SUPPORTED;
    }

    *ret_len = sizeof(*hdr) + sizeof(*pubkey) + sizeof(key->u.a.dss_seed) +
               size + Q_SIZE + size + Q_SIZE;
    if (len >= *ret_len && buf)
    {
        hdr = (BLOBHEADER *)buf;
        hdr->bType    = PRIVATEKEYBLOB;
        hdr->bVersion = 2;
        hdr->reserved = 0;
        hdr->aiKeyAlg = CALG_DSS_SIGN;

        pubkey = (DSSPUBKEY *)(hdr + 1);
        pubkey->magic  = MAGIC_DSS2;
        pubkey->bitlen = key->u.a.bitlen;

        dst = (UCHAR *)(pubkey + 1);
        mbedtls_mpi_write_binary(&dsa_key->P, dst, size);
        reverse_bytes(dst, size);
        dst += size;

        mbedtls_mpi_write_binary(&dsa_key->Q, dst, Q_SIZE);
        reverse_bytes(dst, Q_SIZE);
        dst += Q_SIZE;

        mbedtls_mpi_write_binary(&dsa_key->G, dst, size);
        reverse_bytes(dst, size);
        dst += size;

        mbedtls_mpi_write_binary(&dsa_key->X, dst, Q_SIZE);
        reverse_bytes(dst, Q_SIZE);
        dst += Q_SIZE;

        memcpy(dst, &key->u.a.dss_seed, sizeof(key->u.a.dss_seed));
    }
    if (len < *ret_len && buf) return STATUS_BUFFER_TOO_SMALL;

    return STATUS_SUCCESS;
}
#endif

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

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
    case ALG_ID_DSA:
        if (flags & KEY_EXPORT_FLAG_PUBLIC)
        {
            if (key->u.a.flags & KEY_FLAG_LEGACY_DSA_V2)
                return key_export_dsa_capi_public(key, &key_data(key)->a.dsa.pubkey, params->buf, params->len, params->ret_len);
            return key_export_dsa_public(key, &key_data(key)->a.dsa.pubkey, params->buf, params->len, params->ret_len);
        }
        /* if private key is not set return invalid parameter */
        if (mbedtls_mpi_cmp_int(&key_data(key)->a.dsa.privkey.X, 0) == 0) return STATUS_INVALID_PARAMETER;
        if (key->u.a.flags & KEY_FLAG_LEGACY_DSA_V2)
            return key_export_dsa_capi(key, &key_data(key)->a.dsa.privkey, params->buf, params->len, params->ret_len);
        return key_export_dsa(key, &key_data(key)->a.dsa.privkey, params->buf, params->len, params->ret_len);
#endif

        case ALG_ID_DH:
            if (flags & KEY_EXPORT_FLAG_DH_PARAMETERS)
                return key_export_params(key, &key_data(key)->a.dh.pubkey, params->buf, params->len, params->ret_len);
            if (flags & KEY_EXPORT_FLAG_PUBLIC)
                return key_export_dh_public(key, &key_data(key)->a.dh.pubkey, params->buf, params->len, params->ret_len);
            return key_export_dh(key, &key_data(key)->a.dh.privkey, params->buf, params->len, params->ret_len);
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

    keypair = mbedtls_pk_ec(key_data(key)->a.pk.pubkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_ecp_group_load((struct mbedtls_ecp_group *)&keypair->grp, curve);
    if (ret) return STATUS_INTERNAL_ERROR;

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    plen = mbedtls_mpi_size(&keypair->grp.P);
    if (plen != ecc_blob->cbKey) return STATUS_INTERNAL_ERROR;

    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.X, buf + sizeof(*ecc_blob), ecc_blob->cbKey);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.Y, buf + sizeof(*ecc_blob) + ecc_blob->cbKey, ecc_blob->cbKey);
    mbedtls_mpi_lset((struct mbedtls_mpi *)&keypair->Q.Z, 1);
    ret = mbedtls_ecp_check_pubkey(&keypair->grp, &keypair->Q);

    if (ret) return STATUS_INTERNAL_ERROR;

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

    keypair = mbedtls_pk_ec(key_data(key)->a.pk.privkey);
    if (!keypair) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_ecp_group_load((struct mbedtls_ecp_group *)&keypair->grp, curve);

    if (ret) return STATUS_INTERNAL_ERROR;

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    plen = mbedtls_mpi_size(&keypair->grp.P);
    if (plen != ecc_blob->cbKey) return STATUS_INTERNAL_ERROR;

    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.X, buf + sizeof(*ecc_blob), ecc_blob->cbKey);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->Q.Y, buf + sizeof(*ecc_blob) + ecc_blob->cbKey, ecc_blob->cbKey);
    mbedtls_mpi_lset((struct mbedtls_mpi *)&keypair->Q.Z, 1);
    mbedtls_mpi_read_binary((struct mbedtls_mpi *)&keypair->d, buf + sizeof(*ecc_blob) + 2 * ecc_blob->cbKey, ecc_blob->cbKey);
    ret = mbedtls_ecp_check_pubkey(&keypair->grp, &keypair->Q);

    if (ret) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_rsa_public(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    mbedtls_rsa_context *rsa_context;
    int ret;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pk.pubkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    ret = mbedtls_rsa_import_raw(rsa_context, buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp, rsa_blob->cbModulus, NULL, 0, NULL, 0, NULL, 0,
        buf + sizeof(*rsa_blob), rsa_blob->cbPublicExp);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_complete(rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pubkey(rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_rsa(struct key *key, UCHAR *buf, ULONG len)
{
    BCRYPT_RSAKEY_BLOB *rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    mbedtls_rsa_context *rsa_context;
    int ret;

    rsa_context = mbedtls_pk_rsa(key_data(key)->a.pk.privkey);
    if (!rsa_context) return STATUS_INVALID_PARAMETER;

    ret = mbedtls_rsa_import_raw(rsa_context, buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp, rsa_blob->cbModulus,
        buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus, rsa_blob->cbPrime1,
        buf + sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus + rsa_blob->cbPrime1, rsa_blob->cbPrime2,
        NULL, 0, buf + sizeof(*rsa_blob), rsa_blob->cbPublicExp);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_complete(rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_privkey(rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pubkey(rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;
    ret = mbedtls_rsa_check_pub_priv(rsa_context, rsa_context);
    if (ret) return STATUS_INTERNAL_ERROR;
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
    mbedtls_mpi_read_binary(&dh_key->GX, p, key_len);

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
    mbedtls_mpi_read_binary(&dh_key->GX, p, key_len);
    p += key_len;
    mbedtls_mpi_read_binary(&dh_key->X, p, key_len);

    key->u.a.bitlen = dh_blob->cbKey * 8;
    dh_key->len = key_len;

    return STATUS_SUCCESS;
}

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
static NTSTATUS key_import_dsa_public(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len)
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob;
    mbedtls_mpi P, Q, G, Y;
    UCHAR *p_data, *q_data, *g_data, *y_data;
    int ret;
    size_t p_size, q_size, g_size, y_size;

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    dsa_blob = (BCRYPT_DSA_KEY_BLOB *)buf;
    p_data = buf + sizeof(*dsa_blob);
    p_size = dsa_blob->cbKey;
    q_data = dsa_blob->q;
    q_size = sizeof(dsa_blob->q);
    g_data = buf + sizeof(*dsa_blob) + dsa_blob->cbKey;
    g_size = dsa_blob->cbKey;
    y_data = buf + sizeof(*dsa_blob) + dsa_blob->cbKey * 2;
    y_size = dsa_blob->cbKey;

    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&G);
    mbedtls_mpi_init(&Y);

    mbedtls_mpi_read_binary(&P, p_data, p_size);
    mbedtls_mpi_read_binary(&Q, q_data, q_size);
    mbedtls_mpi_read_binary(&G, g_data, g_size);
    mbedtls_mpi_read_binary(&Y, y_data, y_size);

    ret = mbedtls_dsa_check_pqg(&P, &Q, &G);
    ret = mbedtls_dsa_set_group(dsa_key, &P, &Q, &G);
    if (ret)
    {
        mbedtls_mpi_free(&P);
        mbedtls_mpi_free(&Q);
        mbedtls_mpi_free(&G);
        mbedtls_mpi_free(&Y);
        return STATUS_INTERNAL_ERROR;
    }

    mbedtls_mpi_copy(&dsa_key->Y, &Y);

    ret = mbedtls_dsa_check_pubkey(dsa_key);

    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&G);
    mbedtls_mpi_free(&Y);

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dsa(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len)
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob;
    mbedtls_mpi P, Q, G, Y, X;
    UCHAR *p_data, *q_data, *g_data, *y_data, *x_data;
    int ret, p_size, q_size, g_size, y_size, x_size;

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    dsa_blob = (BCRYPT_DSA_KEY_BLOB *)buf;
    p_data = buf + sizeof(*dsa_blob);
    p_size = dsa_blob->cbKey;
    q_data = dsa_blob->q;
    q_size = sizeof(dsa_blob->q);
    g_data = buf + sizeof(*dsa_blob) + dsa_blob->cbKey;
    g_size = dsa_blob->cbKey;
    y_data = buf + sizeof(*dsa_blob) + dsa_blob->cbKey * 2;
    y_size = dsa_blob->cbKey;
    x_data = buf + sizeof(*dsa_blob) + dsa_blob->cbKey * 3;
    x_size = dsa_blob->cbKey;

    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&G);
    mbedtls_mpi_init(&Y);
    mbedtls_mpi_init(&X);

    mbedtls_mpi_read_binary(&P, p_data, p_size);
    mbedtls_mpi_read_binary(&Q, q_data, q_size);
    mbedtls_mpi_read_binary(&G, g_data, g_size);
    mbedtls_mpi_read_binary(&Y, y_data, y_size);
    mbedtls_mpi_read_binary(&X, x_data, x_size);

    ret = mbedtls_dsa_check_pqg(&P, &Q, &G);
    ret = mbedtls_dsa_set_group(dsa_key, &P, &Q, &G);
    if (ret)
    {
        mbedtls_mpi_free(&P);
        mbedtls_mpi_free(&Q);
        mbedtls_mpi_free(&G);
        mbedtls_mpi_free(&Y);
        return STATUS_INTERNAL_ERROR;
    }

    mbedtls_mpi_copy(&dsa_key->Y, &Y);
    mbedtls_mpi_copy(&dsa_key->X, &X);

    ret = mbedtls_dsa_check_privkey(dsa_key);

    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&G);
    mbedtls_mpi_free(&Y);
    mbedtls_mpi_free(&X);

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dsa_capi_public(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len)
{
    BLOBHEADER *hdr = (BLOBHEADER *)buf;
    DSSPUBKEY *pubkey;
    mbedtls_mpi P, Q, G, Y;
    unsigned char *data, p_data[128], q_data[20], g_data[128], y_data[128];
    int i, ret, size, p_size, q_size, g_size, y_size;

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    pubkey = (DSSPUBKEY *)(buf+sizeof(*hdr));
    size = pubkey->bitlen / 8;
    if (size > sizeof(p_data))
    {
        FIXME("size %u not supported\n", size);
        return STATUS_NOT_SUPPORTED;
    }
    data = buf + sizeof(*hdr) + sizeof(*pubkey);

    p_size = size;
    for (i = 0; i < p_size; i++) p_data[i] = data[p_size - i - 1];
    data += p_size;

    q_size = sizeof(q_data);
    for (i = 0; i < q_size; i++) q_data[i] = data[q_size - i - 1];
    data += q_size;

    g_size = size;
    for (i = 0; i < g_size; i++) g_data[i] = data[g_size - i - 1];
    data += g_size;

    y_size = sizeof(y_data);
    for (i = 0; i < y_size; i++) y_data[i] = data[y_size - i - 1];
    data += y_size;

    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&G);
    mbedtls_mpi_init(&Y);

    mbedtls_mpi_read_binary(&P, p_data, p_size);
    mbedtls_mpi_read_binary(&Q, q_data, q_size);
    mbedtls_mpi_read_binary(&G, g_data, g_size);
    mbedtls_mpi_read_binary(&Y, y_data, y_size);


    ret = mbedtls_dsa_check_pqg(&P, &Q, &G);
    ret = mbedtls_dsa_set_group(dsa_key, &P, &Q, &G);
    if (ret)
    {
        mbedtls_mpi_free(&P);
        mbedtls_mpi_free(&Q);
        mbedtls_mpi_free(&G);
        mbedtls_mpi_free(&Y);
        return STATUS_INTERNAL_ERROR;
    }

    mbedtls_mpi_copy(&dsa_key->Y, &Y);

    ret = mbedtls_dsa_check_pubkey(dsa_key);

    memcpy(&key->u.a.dss_seed, data, sizeof(key->u.a.dss_seed));

    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&G);
    mbedtls_mpi_free(&Y);

    return STATUS_SUCCESS;
}

static NTSTATUS key_import_dsa_capi(struct key *key, mbedtls_dsa_context *dsa_key, UCHAR *buf, ULONG len)
{
    BLOBHEADER *hdr = (BLOBHEADER *)buf;
    DSSPUBKEY *pubkey;
    mbedtls_mpi P, Q, G, X;
    unsigned char *data, p_data[128], q_data[20], g_data[128], x_data[20];
    int i, ret, size, p_size, q_size, g_size, x_size;

    if (!dsa_key) return STATUS_INVALID_PARAMETER;

    pubkey = (DSSPUBKEY *)(buf+sizeof(*hdr));
    if ((size = pubkey->bitlen / 8) > sizeof(p_data))
    {
        FIXME("size %u not supported\n", size);
        return STATUS_NOT_SUPPORTED;
    }
    data = buf + sizeof(*hdr) + sizeof(*pubkey);

    p_size = size;
    for (i = 0; i < p_size; i++) p_data[i] = data[p_size - i - 1];
    data += p_size;

    q_size = sizeof(q_data);
    for (i = 0; i < q_size; i++) q_data[i] = data[q_size - i - 1];
    data += q_size;

    g_size = size;
    for (i = 0; i < g_size; i++) g_data[i] = data[g_size - i - 1];
    data += g_size;

    x_size = sizeof(x_data);
    for (i = 0; i < x_size; i++) x_data[i] = data[x_size - i - 1];
    data += x_size;

    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&G);
    mbedtls_mpi_init(&X);

    mbedtls_mpi_read_binary(&P, p_data, p_size);
    mbedtls_mpi_read_binary(&Q, q_data, q_size);
    mbedtls_mpi_read_binary(&G, g_data, g_size);
    mbedtls_mpi_read_binary(&X, x_data, x_size);

    ret = mbedtls_dsa_check_pqg(&P, &Q, &G);
    ret = mbedtls_dsa_set_group(dsa_key, &P, &Q, &G);
    if (ret)
    {
        mbedtls_mpi_free(&P);
        mbedtls_mpi_free(&Q);
        mbedtls_mpi_free(&G);
        mbedtls_mpi_free(&X);
        return STATUS_INTERNAL_ERROR;
    }

    mbedtls_mpi_copy(&dsa_key->X, &X);

    ret = mbedtls_dsa_pubkey_from_privkey(dsa_key);
    ret = mbedtls_dsa_check_privkey(dsa_key);

    memcpy(&key->u.a.dss_seed, data, sizeof(key->u.a.dss_seed));

    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&G);
    mbedtls_mpi_free(&X);

    return STATUS_SUCCESS;
}
#endif

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

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
static void init_dsa_key(mbedtls_dsa_context *key, int len, BOOL reinit)
{
    if (reinit) mbedtls_dsa_free(key);
    mbedtls_dsa_init(key);
    key->len = len;
}
#endif

NTSTATUS key_asymmetric_import(void *args)
{
    const struct key_asymmetric_import_params *params = args;
    struct key *key = params->key;
    NTSTATUS status = STATUS_SUCCESS;
    int ret;
    mbedtls_pk_type_t pk_type;
    UCHAR *buf;
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
                status = init_pk_key(&key_data(key)->a.pk.privkey, pk_type, TRUE);
                status = key_import_ecc(key, params->buf, params->len);
                break;

            case ALG_ID_RSA:
            case ALG_ID_RSA_SIGN:
                status = init_pk_key(&key_data(key)->a.pk.privkey, pk_type, TRUE);
                status = key_import_rsa(key, params->buf, params->len);
                break;

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
            case ALG_ID_DSA:
                init_dsa_key(&key_data(key)->a.dsa.privkey, (key->u.a.bitlen + 7) / 8, TRUE);
                if (key->u.a.flags & KEY_FLAG_LEGACY_DSA_V2)
                    ret = key_import_dsa_capi(key, &key_data(key)->a.dsa.privkey, params->buf, params->len);
                else
                    ret = key_import_dsa(key, &key_data(key)->a.dsa.privkey, params->buf, params->len);
                if (ret) status = STATUS_INTERNAL_ERROR;
                break;
#endif
            case ALG_ID_DH:
                init_dh_key(&key_data(key)->a.dh.privkey, TRUE);
                if (params->flags & KEY_IMPORT_FLAG_DH_PARAMETERS)
                    status = key_import_dh_params(key, &key_data(key)->a.dh.privkey, params->buf, params->len);
                else
                    status = key_import_dh(key, &key_data(key)->a.dh.privkey, params->buf, params->len);
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
            status = init_pk_key(&key_data(key)->a.pk.pubkey, pk_type, TRUE);
            status = key_import_ecc_public(key, params->buf, params->len);
            break;

        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            status = init_pk_key(&key_data(key)->a.pk.pubkey, pk_type, TRUE);
            status = key_import_rsa_public(key, params->buf, params->len);
            if (status) break;
            if (bPrivate)
            {
                ret = mbedtls_rsa_check_pub_priv(mbedtls_pk_rsa(key_data(key)->a.pk.pubkey), mbedtls_pk_rsa(key_data(key)->a.pk.privkey));
                if (ret) status = STATUS_INTERNAL_ERROR;
            }
            break;

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
            init_dsa_key(&key_data(key)->a.dsa.pubkey, (key->u.a.bitlen + 7) / 8, TRUE);
            if (key->u.a.flags & KEY_FLAG_LEGACY_DSA_V2)
            {
                if (bPrivate)
                {
                    ret = key_import_dsa_capi(key, &key_data(key)->a.dsa.pubkey, params->buf, params->len);
                    mbedtls_mpi_lset(&key_data(key)->a.dsa.pubkey.X, 0);
                }
                else
                    ret = key_import_dsa_capi_public(key, &key_data(key)->a.dsa.pubkey, params->buf, params->len);
            }
            else
                ret = key_import_dsa_public(key, &key_data(key)->a.dsa.pubkey, params->buf, params->len);
            if (ret) status = STATUS_INTERNAL_ERROR;
            break;
#endif
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh.pubkey, TRUE);
            if (params->flags & KEY_IMPORT_FLAG_DH_PARAMETERS)
            {
                status = key_import_dh_params(key, &key_data(key)->a.dh.pubkey, params->buf, params->len);
                if (status) break;
                buf = malloc(1024);
                if (!buf) return STATUS_NO_MEMORY;
                ret = mbedtls_dhm_make_params(&key_data(key)->a.dh.pubkey, (key->u.a.bitlen + 7) / 8, buf, &olen, mbedtls_ctr_drbg_random, &ctr_drbg);
                free(buf);
                if (ret) status = STATUS_INTERNAL_ERROR;
            }
            else
                status = key_import_dh_public(key, &key_data(key)->a.dh.pubkey, params->buf, params->len);
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    return status;
}

#define MBEDTLS_DH_GEN_PRIME
#ifdef MBEDTLS_DH_GEN_PRIME
static int dh_gen_prime(mbedtls_dhm_context *dh, int nbits, int (*f_rng)(void *, unsigned char *, size_t), mbedtls_ctr_drbg_context *ctr_drbg)
{
    int ret;
    mbedtls_mpi Q, P1;
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&P1);
    ret = mbedtls_mpi_read_string(&dh->G, 10, "4");
    ret = mbedtls_mpi_gen_prime(&dh->P, nbits, MBEDTLS_MPI_GEN_PRIME_FLAG_DH, mbedtls_ctr_drbg_random, ctr_drbg);
    // Verifying that Q = (P-1)/2 is prime
    ret = mbedtls_mpi_sub_int(&P1, &dh->P, 1);
    ret = mbedtls_mpi_div_int(&Q, NULL, &P1, 2);
    ret = mbedtls_mpi_is_prime_ext(&Q, 50, f_rng, ctr_drbg);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&P1);
    return ret;
}
#endif /* MBEDTLS_DH_GEN_PRIME */

NTSTATUS key_asymmetric_generate(void *args)
{
    struct key *key = args;
    mbedtls_pk_type_t pk_type;
    mbedtls_ecp_group_id grp_id;
    int ret;
    NTSTATUS status;
    UCHAR *buf;
    ULONG len;
    size_t olen;
    BCRYPT_DSA_KEY_BLOB *dsa_blob;

#ifndef __REACTOS__
    if (!libmbedtls_handle)
        return STATUS_INTERNAL_ERROR;
#endif

    switch (key->alg_id)
    {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            pk_type = key_get_type(key->alg_id);
            status = init_pk_key(&key_data(key)->a.pk.privkey, pk_type, FALSE);
            ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key_data(key)->a.pk.privkey), mbedtls_ctr_drbg_random, &ctr_drbg, key->u.a.bitlen, 65537);
            ret = mbedtls_rsa_complete(mbedtls_pk_rsa(key_data(key)->a.pk.privkey));
            ret = mbedtls_rsa_check_privkey(mbedtls_pk_rsa(key_data(key)->a.pk.privkey));
            ret = mbedtls_rsa_check_pubkey(mbedtls_pk_rsa(key_data(key)->a.pk.privkey));
            if (ret) mbedtls_pk_free(&key_data(key)->a.pk.privkey);
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
            pk_type = key_get_type(key->alg_id);
            status = init_pk_key(&key_data(key)->a.pk.privkey, pk_type, FALSE);
            ret = mbedtls_ecp_gen_key(grp_id, mbedtls_pk_ec(key_data(key)->a.pk.privkey), mbedtls_ctr_drbg_random, &ctr_drbg);
            if (ret) mbedtls_pk_free(&key_data(key)->a.pk.privkey);
            break;
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh.privkey, FALSE);
#ifdef MBEDTLS_DH_GEN_PRIME
            ret = dh_gen_prime(&key_data(key)->a.dh.privkey, key->u.a.bitlen, mbedtls_ctr_drbg_random, &ctr_drbg);
#endif
            buf = malloc(1024);
            if (!buf) return STATUS_NO_MEMORY;
            ret = mbedtls_dhm_make_params(&key_data(key)->a.dh.privkey, (key->u.a.bitlen + 7) / 8, buf, &olen, mbedtls_ctr_drbg_random, &ctr_drbg);
            free(buf);
            if (ret) mbedtls_dhm_free(&key_data(key)->a.dh.privkey);
            break;

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
            ret = 0;
            init_dsa_key(&key_data(key)->a.dsa.privkey, (key->u.a.bitlen + 7) / 8, FALSE);
            ret = mbedtls_dsa_genkey(&key_data(key)->a.dsa.privkey, sizeof(dsa_blob->q), (key->u.a.bitlen + 7) / 8, mbedtls_ctr_drbg_random, &ctr_drbg);
            ret = mbedtls_dsa_check_pqg(&key_data(key)->a.dsa.privkey.P, &key_data(key)->a.dsa.privkey.Q, &key_data(key)->a.dsa.privkey.G);
            ret = mbedtls_dsa_check_pubkey(&key_data(key)->a.dsa.privkey);
            ret = mbedtls_dsa_check_privkey(&key_data(key)->a.dsa.privkey);
            if (ret) mbedtls_dsa_free(&key_data(key)->a.dsa.privkey);
            break;
#endif
        default:
            FIXME("unsupported alg_id %d\n", key->alg_id);
            ret = MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
            break;
    }

    if (ret) return STATUS_INTERNAL_ERROR;

    buf = malloc(1024);
    if (!buf) return STATUS_NO_MEMORY;

    switch (key->alg_id)
    {
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
            status = init_pk_key(&key_data(key)->a.pk.pubkey, pk_type, FALSE);
            status = key_export_ecc(key, buf, 1024, &len);
            status = key_import_ecc_public(key, buf, len);
            if (status) mbedtls_pk_free(&key_data(key)->a.pk.pubkey);
            break;

        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
            status = init_pk_key(&key_data(key)->a.pk.pubkey, pk_type, FALSE);
            status = key_export_rsa(key, 0, buf, 1024, &len);
            status = key_import_rsa_public(key, buf, len);
            if (status) mbedtls_pk_free(&key_data(key)->a.pk.pubkey);
            break;

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
            init_dsa_key(&key_data(key)->a.dsa.pubkey, (key->u.a.bitlen + 7) / 8, FALSE);
            status = key_export_dsa(key, &key_data(key)->a.dsa.privkey, buf, 1024, &len);
            status = key_import_dsa_public(key, &key_data(key)->a.dsa.pubkey, buf, len);
            if (status) mbedtls_dsa_free(&key_data(key)->a.dsa.pubkey);
            break;
#endif
        case ALG_ID_DH:
            init_dh_key(&key_data(key)->a.dh.pubkey, FALSE);
            status = key_export_dh(key, &key_data(key)->a.dh.privkey, buf, 1024, &len);
            status = key_import_dh_public(key, &key_data(key)->a.dh.pubkey, buf, len);
            if (status) mbedtls_dhm_free(&key_data(key)->a.dh.pubkey);
            break;

        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }
    free(buf);

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
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
        {
            if (params->flags) FIXME("flags %x not supported\n", params->flags);
            if (params->hash_len != 20)
            {
                FIXME("hash size %u not supported\n", params->hash_len);
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

    if (key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
         || key->alg_id == ALG_ID_DSA
#endif
        )
    {
        len = params->signature_len / 2;
        out_signature = malloc(params->signature_len + 6);
        if (!out_signature) return STATUS_NO_MEMORY;
        p = out_signature + params->signature_len + 6;
        start = out_signature;
        ret = mbedtls_asn1_write_raw_buffer(&p, start, params->signature + len, len);
        ret = mbedtls_asn1_write_len(&p, start, len);
        ret = mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_INTEGER);
        ret = mbedtls_asn1_write_raw_buffer(&p, start, params->signature, len);
        ret = mbedtls_asn1_write_len(&p, start, len);
        ret = mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_INTEGER);
        ret = mbedtls_asn1_write_len(&p, start, params->signature_len + 4);
        ret = mbedtls_asn1_write_tag(&p, start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        out_signature_len = params->signature_len + 6;
    }
    else
    {
        out_signature = params->signature;
        out_signature_len = params->signature_len;
    }

#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
    if (key->alg_id == ALG_ID_DSA)
        ret = mbedtls_dsa_verify(&key_data(params->key)->a.dsa.pubkey, params->hash, params->hash_len, out_signature, out_signature_len);
    else
#endif
        ret = mbedtls_pk_verify_ext(pk_type, (params->flags & BCRYPT_PAD_PSS) == 0 ? NULL : &pss_options, &key_data(params->key)->a.pk.pubkey,
            hash_alg, params->hash, params->hash_len, out_signature, out_signature_len);
    if (out_signature != params->signature) free(out_signature);
    if (ret) return STATUS_INVALID_SIGNATURE;

    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_sign(void *args)
{
    const struct key_asymmetric_sign_params *params = args;
    struct key *key = params->key;
    size_t sig_len, len, expected_len;
    mbedtls_md_type_t hash_alg = MBEDTLS_MD_NONE;
    unsigned int flags = params->flags;
    UCHAR *p, *end, *output;
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
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
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
#endif
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

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(key)->a.pk.privkey), MBEDTLS_RSA_PKCS_V21, hash_alg);
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

    output = malloc(1024);
    if (!output) return STATUS_NO_MEMORY;
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
    if (key->alg_id == ALG_ID_DSA)
        ret = mbedtls_dsa_sign(&key_data(params->key)->a.dsa.privkey, params->input, params->input_len, output, &sig_len,
                            mbedtls_ctr_drbg_random, &ctr_drbg);
    else
#endif
        ret = mbedtls_pk_sign(&key_data(params->key)->a.pk.privkey, hash_alg, params->input, params->input_len, output, &sig_len,
                            mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret)
    {
        free(output);
        return STATUS_INTERNAL_ERROR;
    }
    if ((key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
         || key->alg_id == ALG_ID_DSA
#endif
        ))
    {
        p = output;
        end = output + sig_len;
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
        expected_len = (len / 2) * 2;
        if (!ret && expected_len < len) p += len - expected_len;
        if (!ret) sig_len = expected_len;
        memcpy(params->output, p, expected_len);
        p += expected_len;
        ret = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_INTEGER);
        expected_len = (len / 2) * 2;
        if (!ret && expected_len < len) p += len - expected_len;
        if (!ret) memcpy(params->output + sig_len, p, expected_len);
        if (!ret) sig_len += expected_len;
    }
    else
        if (sig_len <= params->output_len) memcpy(params->output, output, sig_len);
    free(output);

    *params->ret_len = sig_len;
    if (sig_len > params->output_len) return STATUS_BUFFER_TOO_SMALL;
    if (ret) return STATUS_INTERNAL_ERROR;
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_destroy(void *args)
{
    struct key *key = args;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    switch (key->alg_id)
    {
        case ALG_ID_ECDSA_P256:
        case ALG_ID_ECDSA_P384:
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
        {
            mbedtls_pk_free(&key_data(key)->a.pk.privkey);
            mbedtls_pk_free(&key_data(key)->a.pk.pubkey);
            break;
        }
        case ALG_ID_DH:
        {
            mbedtls_dhm_free(&key_data(key)->a.dh.privkey);
            mbedtls_dhm_free(&key_data(key)->a.dh.pubkey);
            break;
        }
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
        {
            mbedtls_dsa_free(&key_data(key)->a.dsa.privkey);
            mbedtls_dsa_free(&key_data(key)->a.dsa.pubkey);
            break;
        }
#endif
        default:
            FIXME("algorithm %u not yet supported\n", key->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }
    return STATUS_SUCCESS;
}

NTSTATUS key_asymmetric_duplicate(void *args)
{
    const struct key_asymmetric_duplicate_params *params = args;
    mbedtls_pk_type_t pk_type;
    unsigned char *buf;
    ULONG len;
    NTSTATUS status;

#ifndef __REACTOS__
    if (!libmbedtls_handle) return STATUS_INTERNAL_ERROR;
#endif

    buf = malloc(1024);
    if (!buf) return STATUS_NO_MEMORY;

    switch (params->key_orig->alg_id)
    {
        case ALG_ID_RSA:
        case ALG_ID_RSA_SIGN:
        {
            pk_type = key_get_type(params->key_orig->alg_id);
            init_pk_key(&key_data(params->key_copy)->a.pk.privkey, pk_type, TRUE);
            init_pk_key(&key_data(params->key_copy)->a.pk.pubkey, pk_type, TRUE);
            status = key_export_rsa(params->key_orig, KEY_EXPORT_FLAG_RSA_FULL, buf, 1024, &len);
            status = key_import_rsa(params->key_copy, buf, len);
            status = key_export_rsa_public(params->key_orig, buf, 1024, &len);
            if (status)
            {
                mbedtls_pk_free(&key_data(params->key_copy)->a.pk.privkey);
                mbedtls_pk_free(&key_data(params->key_copy)->a.pk.pubkey);
                break;
            }
            status = key_import_rsa_public(params->key_copy, buf, len);
            break;
        }
        case ALG_ID_ECDH_P256:
        case ALG_ID_ECDH_P384:
        {
            pk_type = key_get_type(params->key_orig->alg_id);
            init_pk_key(&key_data(params->key_copy)->a.pk.privkey, pk_type, TRUE);
            init_pk_key(&key_data(params->key_copy)->a.pk.pubkey, pk_type, TRUE);
            status = key_export_ecc(params->key_orig, buf, 1024, &len);
            status = key_import_ecc(params->key_copy, buf, len);
            status = key_export_ecc_public(params->key_orig, buf, 1024, &len);
            if (status)
            {
                mbedtls_pk_free(&key_data(params->key_copy)->a.pk.privkey);
                mbedtls_pk_free(&key_data(params->key_copy)->a.pk.pubkey);
                break;
            }
            status = key_import_ecc_public(params->key_copy, buf, len);
            break;
        }
#ifdef MBEDTLS_DSA_C /* DSA is not supported by mbedtls */
        case ALG_ID_DSA:
        {
            init_dsa_key(&key_data(params->key_copy)->a.dsa.privkey, (params->key_orig->u.a.bitlen + 7) / 8, TRUE);
            init_dsa_key(&key_data(params->key_copy)->a.dsa.pubkey, (params->key_orig->u.a.bitlen + 7) / 8, TRUE);
            status = key_export_dsa(params->key_orig, &key_data(params->key_orig)->a.dsa.privkey, buf, 1024, &len);
            status = key_import_dsa(params->key_copy, &key_data(params->key_copy)->a.dsa.privkey, buf, len);
            status = key_export_dsa_public(params->key_orig, &key_data(params->key_orig)->a.dsa.pubkey, buf, 1024, &len);
            if (status)
            {
                mbedtls_dsa_free(&key_data(params->key_copy)->a.dsa.privkey);
                mbedtls_dsa_free(&key_data(params->key_copy)->a.dsa.pubkey);
                break;
            }
            status = key_import_dsa_public(params->key_copy, &key_data(params->key_copy)->a.dsa.pubkey, buf, len);
            break;
        }
#endif
        case ALG_ID_DH:
        {
            init_dh_key(&key_data(params->key_copy)->a.dh.privkey, TRUE);
            init_dh_key(&key_data(params->key_copy)->a.dh.pubkey, TRUE);
            status = key_export_dh(params->key_orig, &key_data(params->key_orig)->a.dh.privkey, buf, 1024, &len);
            status = key_import_dh(params->key_copy, &key_data(params->key_copy)->a.dh.privkey, buf, len);
            status = key_export_dh_public(params->key_orig, &key_data(params->key_orig)->a.dh.pubkey, buf, 1024, &len);
            if (status)
            {
                mbedtls_dhm_free(&key_data(params->key_copy)->a.dh.privkey);
                mbedtls_dhm_free(&key_data(params->key_copy)->a.dh.pubkey);
                break;
            }
            status = key_import_dh_public(params->key_copy, &key_data(params->key_copy)->a.dh.pubkey, buf, len);
            break;
        }
        default:
            FIXME("unsupported algorithm %u\n", params->key_orig->alg_id);
            return STATUS_NOT_IMPLEMENTED;
    }

    free(buf);
    if (!status) return STATUS_SUCCESS;

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

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(params->key)->a.pk.privkey), MBEDTLS_RSA_PKCS_V21, dig);
    }
    ret = mbedtls_pk_decrypt(&key_data(params->key)->a.pk.privkey, params->input, params->input_len, params->output, &olen, params->output_len, mbedtls_ctr_drbg_random, &ctr_drbg);

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

        mbedtls_rsa_set_padding(mbedtls_pk_rsa(key_data(params->key)->a.pk.pubkey), MBEDTLS_RSA_PKCS_V21, dig);
    }

    ret = mbedtls_pk_encrypt(&key_data(params->key)->a.pk.pubkey, params->input, params->input_len, params->output, &olen, params->output_len,
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
            mbedtls_ecp_keypair *ecp_keypair_pr;
            mbedtls_ecp_keypair *ecp_keypair_pu;

            mbedtls_ecdh_init(&ecdh);
            ecp_keypair_pr = mbedtls_pk_ec(key_data(params->privkey)->a.pk.privkey);
            ecp_keypair_pu = mbedtls_pk_ec(key_data(params->pubkey)->a.pk.pubkey);
            if (!ecp_keypair_pr || !ecp_keypair_pu) return STATUS_INTERNAL_ERROR;
            mbedtls_ecp_group_load(&ecdh.grp, ecp_keypair_pr->grp.id);
            mbedtls_mpi_copy(&ecdh.d, &ecp_keypair_pr->d);
            mbedtls_ecp_copy(&ecdh.Qp, &ecp_keypair_pu->Q);

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
            mbedtls_dhm_context *dhm;
            mbedtls_dhm_context *dhm_pub;
            size_t secret_len;

            dhm = &key_data(params->privkey)->a.dh.privkey;
            dhm_pub = &key_data(params->pubkey)->a.dh.pubkey;

            mbedtls_mpi_copy(&dhm->GY, &dhm_pub->GX);

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
