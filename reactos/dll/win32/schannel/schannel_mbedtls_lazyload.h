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

#define mbedtls_ctr_drbg_free           mbedtls_ctr_drbg_free
#define mbedtls_ctr_drbg_init           mbedtls_ctr_drbg_init
#define mbedtls_ctr_drbg_random         mbedtls_ctr_drbg_random
#define mbedtls_entropy_free            mbedtls_entropy_free
#define mbedtls_entropy_func            mbedtls_entropy_func
#define mbedtls_entropy_init            mbedtls_entropy_init
#define mbedtls_ssl_ciphersuite_from_id mbedtls_ssl_ciphersuite_from_id
#define mbedtls_ssl_free                mbedtls_ssl_free
#define mbedtls_ssl_get_ciphersuite     mbedtls_ssl_get_ciphersuite
#define mbedtls_ssl_get_ciphersuite_id  mbedtls_ssl_get_ciphersuite_id
#define mbedtls_ssl_get_max_frag_len    mbedtls_ssl_get_max_frag_len
#define mbedtls_ssl_get_version         mbedtls_ssl_get_version
#define mbedtls_ssl_handshake           mbedtls_ssl_handshake
#define mbedtls_ssl_init                mbedtls_ssl_init
#define mbedtls_ssl_read                mbedtls_ssl_read
#define mbedtls_ssl_conf_authmode       mbedtls_ssl_conf_authmode 
#define mbedtls_ssl_set_bio             mbedtls_ssl_set_bio
#define mbedtls_ssl_conf_endpoint       mbedtls_ssl_conf_endpoint
#define mbedtls_ssl_set_hostname        mbedtls_ssl_set_hostname
#define mbedtls_ssl_conf_max_version    mbedtls_ssl_conf_max_version
#define mbedtls_ssl_conf_min_version    mbedtls_ssl_conf_min_version
#define mbedtls_ssl_conf_rng            mbedtls_ssl_conf_rng
#define mbedtls_ssl_write               mbedtls_ssl_write
#define mbedtls_ssl_get_peer_cert       mbedtls_ssl_get_peer_cert
#define mbedtls_ssl_config_init         mbedtls_ssl_config_init
#define mbedtls_ssl_config_defaults     mbedtls_ssl_config_defaults
#define mbedtls_ssl_conf_dbg            mbedtls_ssl_conf_dbg
#define mbedtls_ssl_setup               mbedtls_ssl_setup
#define mbedtls_cipher_info_from_type   mbedtls_cipher_info_from_type      
#define mbedtls_md_info_from_type       mbedtls_md_info_from_type
#define mbedtls_pk_get_bitlen           mbedtls_pk_get_bitlen
#define mbedtls_ctr_drbg_seed           mbedtls_ctr_drbg_seed