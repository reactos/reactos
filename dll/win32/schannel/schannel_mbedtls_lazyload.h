/* Copyright 2015 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
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
 * --
 *
 * This file transparently wraps lazy-loading hooks in the schannel
 * implementation on top of the PolarSSL/mbedTLS open source library.
 */

static void *libmbedtls_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;

MAKE_FUNCPTR(mbedtls_ctr_drbg_free)
MAKE_FUNCPTR(mbedtls_ctr_drbg_init)
MAKE_FUNCPTR(mbedtls_ctr_drbg_random)
MAKE_FUNCPTR(mbedtls_entropy_free)
MAKE_FUNCPTR(mbedtls_entropy_func)
MAKE_FUNCPTR(mbedtls_entropy_init)
MAKE_FUNCPTR(mbedtls_ssl_ciphersuite_from_id)
MAKE_FUNCPTR(mbedtls_ssl_free)
MAKE_FUNCPTR(mbedtls_ssl_get_ciphersuite)
MAKE_FUNCPTR(mbedtls_ssl_get_ciphersuite_id)
MAKE_FUNCPTR(mbedtls_ssl_get_max_frag_len)
MAKE_FUNCPTR(mbedtls_ssl_get_version)
MAKE_FUNCPTR(mbedtls_ssl_handshake)
MAKE_FUNCPTR(mbedtls_ssl_init)
MAKE_FUNCPTR(mbedtls_ssl_read)
MAKE_FUNCPTR(mbedtls_ssl_conf_authmode)
MAKE_FUNCPTR(mbedtls_ssl_set_bio)
MAKE_FUNCPTR(mbedtls_ssl_conf_endpoint)
MAKE_FUNCPTR(mbedtls_ssl_set_hostname)
MAKE_FUNCPTR(mbedtls_ssl_conf_max_version)
MAKE_FUNCPTR(mbedtls_ssl_conf_min_version)
MAKE_FUNCPTR(mbedtls_ssl_conf_rng)
MAKE_FUNCPTR(mbedtls_ssl_write)
MAKE_FUNCPTR(mbedtls_ssl_get_peer_cert)
MAKE_FUNCPTR(mbedtls_ssl_config_init)
MAKE_FUNCPTR(mbedtls_ssl_config_free)
MAKE_FUNCPTR(mbedtls_ssl_config_defaults)
MAKE_FUNCPTR(mbedtls_ssl_conf_dbg)
MAKE_FUNCPTR(mbedtls_ssl_setup)
MAKE_FUNCPTR(mbedtls_cipher_info_from_type)
MAKE_FUNCPTR(mbedtls_md_info_from_type)
MAKE_FUNCPTR(mbedtls_pk_get_bitlen)
MAKE_FUNCPTR(mbedtls_ctr_drbg_seed)

#undef MAKE_FUNCPTR

/* replace the initialization functions by our own, specially tailored, ones */

BOOL schan_imp_init(void)
{
    libmbedtls_handle = wine_dlopen(SONAME_LIBMBEDTLS, RTLD_NOW, NULL, 0);

    if (!libmbedtls_handle)
    {
        WARN("Failed to load the mbedTLS dynamic library (" SONAME_LIBMBEDTLS ").\n");
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym(libmbedtls_handle, #f, NULL, 0))) \
    { \
        ERR("Failed to retrieve function %s from the mbedTLS dynamic library (" SONAME_LIBMBEDTLS ")\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(mbedtls_ctr_drbg_free)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_init)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_random)
    LOAD_FUNCPTR(mbedtls_entropy_free)
    LOAD_FUNCPTR(mbedtls_entropy_func)
    LOAD_FUNCPTR(mbedtls_entropy_init)
    LOAD_FUNCPTR(mbedtls_ssl_ciphersuite_from_id)
    LOAD_FUNCPTR(mbedtls_ssl_free)
    LOAD_FUNCPTR(mbedtls_ssl_get_ciphersuite)
    LOAD_FUNCPTR(mbedtls_ssl_get_ciphersuite_id)
    LOAD_FUNCPTR(mbedtls_ssl_get_max_frag_len)
    LOAD_FUNCPTR(mbedtls_ssl_get_version)
    LOAD_FUNCPTR(mbedtls_ssl_handshake)
    LOAD_FUNCPTR(mbedtls_ssl_init)
    LOAD_FUNCPTR(mbedtls_ssl_read)
    LOAD_FUNCPTR(mbedtls_ssl_conf_authmode)
    LOAD_FUNCPTR(mbedtls_ssl_set_bio)
    LOAD_FUNCPTR(mbedtls_ssl_conf_endpoint)
    LOAD_FUNCPTR(mbedtls_ssl_set_hostname)
    LOAD_FUNCPTR(mbedtls_ssl_conf_max_version)
    LOAD_FUNCPTR(mbedtls_ssl_conf_min_version)
    LOAD_FUNCPTR(mbedtls_ssl_conf_rng)
    LOAD_FUNCPTR(mbedtls_ssl_write)
    LOAD_FUNCPTR(mbedtls_ssl_get_peer_cert)
    LOAD_FUNCPTR(mbedtls_ssl_config_init)
    LOAD_FUNCPTR(mbedtls_ssl_config_free)
    LOAD_FUNCPTR(mbedtls_ssl_config_defaults)
    LOAD_FUNCPTR(mbedtls_ssl_conf_dbg)
    LOAD_FUNCPTR(mbedtls_ssl_setup)
    LOAD_FUNCPTR(mbedtls_cipher_info_from_type)
    LOAD_FUNCPTR(mbedtls_md_info_from_type)
    LOAD_FUNCPTR(mbedtls_pk_get_bitlen)
    LOAD_FUNCPTR(mbedtls_ctr_drbg_seed)

#undef LOAD_FUNCPTR

    return TRUE;

fail:

    wine_dlclose(libmbedtls_handle, NULL, 0);
    libmbedtls_handle = NULL;

    return FALSE;
}

void schan_imp_deinit(void)
{
    wine_dlclose(libmbedtls_handle, NULL, 0);
    libmbedtls_handle = NULL;
}

/* now that we have overridden the initialization functions
   cancel out the original stubs used when dynamically linking */

#define schan_imp_init                  schan_imp_init_unused
#define schan_imp_deinit                schan_imp_deinit_unused

/* seamlessly redirect the function pointers with some preprocessor magic */

#define mbedtls_ctr_drbg_free           pmbedtls_ctr_drbg_free
#define mbedtls_ctr_drbg_init           pmbedtls_ctr_drbg_init
#define mbedtls_ctr_drbg_random         pmbedtls_ctr_drbg_random
#define mbedtls_entropy_free            pmbedtls_entropy_free
#define mbedtls_entropy_func            pmbedtls_entropy_func
#define mbedtls_entropy_init            pmbedtls_entropy_init
#define mbedtls_ssl_ciphersuite_from_id pmbedtls_ssl_ciphersuite_from_id
#define mbedtls_ssl_free                pmbedtls_ssl_free
#define mbedtls_ssl_get_ciphersuite     pmbedtls_ssl_get_ciphersuite
#define mbedtls_ssl_get_ciphersuite_id  pmbedtls_ssl_get_ciphersuite_id
#define mbedtls_ssl_get_max_frag_len    pmbedtls_ssl_get_max_frag_len
#define mbedtls_ssl_get_version         pmbedtls_ssl_get_version
#define mbedtls_ssl_handshake           pmbedtls_ssl_handshake
#define mbedtls_ssl_init                pmbedtls_ssl_init
#define mbedtls_ssl_read                pmbedtls_ssl_read
#define mbedtls_ssl_conf_authmode       pmbedtls_ssl_conf_authmode
#define mbedtls_ssl_set_bio             pmbedtls_ssl_set_bio
#define mbedtls_ssl_conf_endpoint       pmbedtls_ssl_conf_endpoint
#define mbedtls_ssl_set_hostname        pmbedtls_ssl_set_hostname
#define mbedtls_ssl_conf_max_version    pmbedtls_ssl_conf_max_version
#define mbedtls_ssl_conf_min_version    pmbedtls_ssl_conf_min_version
#define mbedtls_ssl_conf_rng            pmbedtls_ssl_conf_rng
#define mbedtls_ssl_write               pmbedtls_ssl_write
#define mbedtls_ssl_get_peer_cert       pmbedtls_ssl_get_peer_cert
#define mbedtls_ssl_config_init         pmbedtls_ssl_config_init
#define mbedtls_ssl_config_free         pmbedtls_ssl_config_free
#define mbedtls_ssl_config_defaults     pmbedtls_ssl_config_defaults
#define mbedtls_ssl_conf_dbg            pmbedtls_ssl_conf_dbg
#define mbedtls_ssl_setup               pmbedtls_ssl_setup
#define mbedtls_cipher_info_from_type   pmbedtls_cipher_info_from_type
#define mbedtls_md_info_from_type       pmbedtls_md_info_from_type
#define mbedtls_pk_get_bitlen           pmbedtls_pk_get_bitlen
#define mbedtls_ctr_drbg_seed           pmbedtls_ctr_drbg_seed
