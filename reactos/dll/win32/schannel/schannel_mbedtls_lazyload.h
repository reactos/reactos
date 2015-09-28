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

MAKE_FUNCPTR(ctr_drbg_free)
MAKE_FUNCPTR(ctr_drbg_init)
MAKE_FUNCPTR(ctr_drbg_random)
MAKE_FUNCPTR(entropy_free)
MAKE_FUNCPTR(entropy_func)
MAKE_FUNCPTR(entropy_init)
MAKE_FUNCPTR(ssl_ciphersuite_from_id)
MAKE_FUNCPTR(ssl_free)
MAKE_FUNCPTR(ssl_get_ciphersuite)
MAKE_FUNCPTR(ssl_get_ciphersuite_id)
MAKE_FUNCPTR(ssl_get_version)
MAKE_FUNCPTR(ssl_handshake)
MAKE_FUNCPTR(ssl_init)
MAKE_FUNCPTR(ssl_read)
MAKE_FUNCPTR(ssl_set_authmode)
MAKE_FUNCPTR(ssl_set_bio)
MAKE_FUNCPTR(ssl_set_endpoint)
MAKE_FUNCPTR(ssl_set_hostname)
MAKE_FUNCPTR(ssl_set_max_version)
MAKE_FUNCPTR(ssl_set_min_version)
MAKE_FUNCPTR(ssl_set_rng)
MAKE_FUNCPTR(ssl_write)

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

	LOAD_FUNCPTR(ctr_drbg_free)
	LOAD_FUNCPTR(ctr_drbg_init)
	LOAD_FUNCPTR(ctr_drbg_random)
	LOAD_FUNCPTR(entropy_free)
	LOAD_FUNCPTR(entropy_func)
	LOAD_FUNCPTR(entropy_init)
	LOAD_FUNCPTR(ssl_ciphersuite_from_id)
	LOAD_FUNCPTR(ssl_free)
	LOAD_FUNCPTR(ssl_get_ciphersuite)
	LOAD_FUNCPTR(ssl_get_ciphersuite_id)
	LOAD_FUNCPTR(ssl_get_version)
	LOAD_FUNCPTR(ssl_handshake)
	LOAD_FUNCPTR(ssl_init)
	LOAD_FUNCPTR(ssl_read)
	LOAD_FUNCPTR(ssl_set_authmode)
	LOAD_FUNCPTR(ssl_set_bio)
	LOAD_FUNCPTR(ssl_set_endpoint)
	LOAD_FUNCPTR(ssl_set_hostname)
	LOAD_FUNCPTR(ssl_set_max_version)
	LOAD_FUNCPTR(ssl_set_min_version)
	LOAD_FUNCPTR(ssl_set_rng)
	LOAD_FUNCPTR(ssl_write)

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

#define schan_imp_init          schan_imp_init_unused
#define schan_imp_deinit        schan_imp_deinit_unused

/* seamlessly redirect the function pointers with some preprocessor magic */

#define ctr_drbg_free           pctr_drbg_free
#define ctr_drbg_init           pctr_drbg_init
#define ctr_drbg_random         pctr_drbg_random
#define entropy_free            pentropy_free
#define entropy_func            pentropy_func
#define entropy_init            pentropy_init
#define ssl_ciphersuite_from_id pssl_ciphersuite_from_id
#define ssl_free                pssl_free
#define ssl_get_ciphersuite     pssl_get_ciphersuite
#define ssl_get_ciphersuite_id  pssl_get_ciphersuite_id
#define ssl_get_version         pssl_get_version
#define ssl_handshake           pssl_handshake
#define ssl_init                pssl_init
#define ssl_read                pssl_read
#define ssl_set_authmode        pssl_set_authmode
#define ssl_set_bio             pssl_set_bio
#define ssl_set_endpoint        pssl_set_endpoint
#define ssl_set_hostname        pssl_set_hostname
#define ssl_set_max_version     pssl_set_max_version
#define ssl_set_min_version     pssl_set_min_version
#define ssl_set_rng             pssl_set_rng
#define ssl_write               pssl_write