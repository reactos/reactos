/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2018 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_GNUTLS_CIPHER_INIT

#include <stdarg.h>
#include <assert.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntsecapi.h"
#include "wincrypt.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#if GNUTLS_VERSION_MAJOR < 3
#define GNUTLS_CIPHER_AES_192_CBC 92
#define GNUTLS_CIPHER_AES_128_GCM 93
#define GNUTLS_CIPHER_AES_256_GCM 94
#define GNUTLS_PK_ECC 4

#define GNUTLS_CURVE_TO_BITS(curve) (unsigned int)(((unsigned int)1<<31)|((unsigned int)(curve)))

typedef enum
{
    GNUTLS_ECC_CURVE_INVALID,
    GNUTLS_ECC_CURVE_SECP224R1,
    GNUTLS_ECC_CURVE_SECP256R1,
    GNUTLS_ECC_CURVE_SECP384R1,
    GNUTLS_ECC_CURVE_SECP521R1,
} gnutls_ecc_curve_t;
#endif

union key_data
{
    gnutls_cipher_hd_t cipher;
    gnutls_privkey_t   privkey;
};
C_ASSERT( sizeof(union key_data) <= sizeof(((struct key *)0)->private) );

static union key_data *key_data( struct key *key )
{
    return (union key_data *)key->private;
}

/* Not present in gnutls version < 3.0 */
static int (*pgnutls_cipher_tag)(gnutls_cipher_hd_t, void *, size_t);
static int (*pgnutls_cipher_add_auth)(gnutls_cipher_hd_t, const void *, size_t);
static gnutls_sign_algorithm_t (*pgnutls_pk_to_sign)(gnutls_pk_algorithm_t, gnutls_digest_algorithm_t);
static int (*pgnutls_pubkey_import_ecc_raw)(gnutls_pubkey_t, gnutls_ecc_curve_t,
                                            const gnutls_datum_t *, const gnutls_datum_t *);
static int (*pgnutls_privkey_import_ecc_raw)(gnutls_privkey_t, gnutls_ecc_curve_t, const gnutls_datum_t *,
                                             const gnutls_datum_t *, const gnutls_datum_t *);
static int (*pgnutls_pubkey_verify_hash2)(gnutls_pubkey_t, gnutls_sign_algorithm_t, unsigned int,
                                          const gnutls_datum_t *, const gnutls_datum_t *);

/* Not present in gnutls version < 2.11.0 */
static int (*pgnutls_pubkey_import_rsa_raw)(gnutls_pubkey_t, const gnutls_datum_t *, const gnutls_datum_t *);

/* Not present in gnutls version < 2.12.0 */
static int (*pgnutls_pubkey_import_dsa_raw)(gnutls_pubkey_t, const gnutls_datum_t *, const gnutls_datum_t *,
                                            const gnutls_datum_t *, const gnutls_datum_t *);

/* Not present in gnutls version < 3.3.0 */
static int (*pgnutls_privkey_export_ecc_raw)(gnutls_privkey_t, gnutls_ecc_curve_t *,
                                             gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *);
static int (*pgnutls_privkey_export_rsa_raw)(gnutls_privkey_t, gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *,
                                             gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *,
                                             gnutls_datum_t *);
static int (*pgnutls_privkey_export_dsa_raw)(gnutls_privkey_t, gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *,
                                             gnutls_datum_t *, gnutls_datum_t *);
static int (*pgnutls_privkey_generate)(gnutls_privkey_t, gnutls_pk_algorithm_t, unsigned int, unsigned int);
static int (*pgnutls_privkey_import_rsa_raw)(gnutls_privkey_t, const gnutls_datum_t *, const gnutls_datum_t *,
                                             const gnutls_datum_t *, const gnutls_datum_t *, const gnutls_datum_t *,
                                             const gnutls_datum_t *, const gnutls_datum_t *, const gnutls_datum_t *);
static int (*pgnutls_privkey_decrypt_data)(gnutls_privkey_t, unsigned int flags, const gnutls_datum_t *, gnutls_datum_t *);

/* Not present in gnutls version < 3.6.0 */
static int (*pgnutls_decode_rs_value)(const gnutls_datum_t *, gnutls_datum_t *, gnutls_datum_t *);

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_cipher_decrypt2);
MAKE_FUNCPTR(gnutls_cipher_deinit);
MAKE_FUNCPTR(gnutls_cipher_encrypt2);
MAKE_FUNCPTR(gnutls_cipher_init);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_privkey_decrypt_data);
MAKE_FUNCPTR(gnutls_privkey_deinit);
MAKE_FUNCPTR(gnutls_privkey_import_dsa_raw);
MAKE_FUNCPTR(gnutls_privkey_init);
MAKE_FUNCPTR(gnutls_privkey_sign_hash);
MAKE_FUNCPTR(gnutls_pubkey_deinit);
MAKE_FUNCPTR(gnutls_pubkey_init);
#undef MAKE_FUNCPTR

static int compat_gnutls_cipher_tag(gnutls_cipher_hd_t handle, void *tag, size_t tag_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_cipher_add_auth(gnutls_cipher_hd_t handle, const void *ptext, size_t ptext_size)
{
    return GNUTLS_E_UNKNOWN_CIPHER_TYPE;
}

static int compat_gnutls_pubkey_import_ecc_raw(gnutls_pubkey_t key, gnutls_ecc_curve_t curve,
                                               const gnutls_datum_t *x, const gnutls_datum_t *y)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_export_rsa_raw(gnutls_privkey_t key, gnutls_datum_t *m, gnutls_datum_t *e,
                                                gnutls_datum_t *d, gnutls_datum_t *p, gnutls_datum_t *q,
                                                gnutls_datum_t *u, gnutls_datum_t *e1, gnutls_datum_t *e2)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_export_ecc_raw(gnutls_privkey_t key, gnutls_ecc_curve_t *curve,
                                                gnutls_datum_t *x, gnutls_datum_t *y, gnutls_datum_t *k)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_import_ecc_raw(gnutls_privkey_t key, gnutls_ecc_curve_t curve,
                                                const gnutls_datum_t *x, const gnutls_datum_t *y,
                                                const gnutls_datum_t *k)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_export_dsa_raw(gnutls_privkey_t key, gnutls_datum_t *p, gnutls_datum_t *q,
                                                gnutls_datum_t *g, gnutls_datum_t *y, gnutls_datum_t *x)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static gnutls_sign_algorithm_t compat_gnutls_pk_to_sign(gnutls_pk_algorithm_t pk, gnutls_digest_algorithm_t hash)
{
    return GNUTLS_SIGN_UNKNOWN;
}

static int compat_gnutls_pubkey_verify_hash2(gnutls_pubkey_t key, gnutls_sign_algorithm_t algo,
                                             unsigned int flags, const gnutls_datum_t *hash,
                                             const gnutls_datum_t *signature)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_pubkey_import_rsa_raw(gnutls_pubkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_pubkey_import_dsa_raw(gnutls_pubkey_t key, const gnutls_datum_t *p, const gnutls_datum_t *q,
                                               const gnutls_datum_t *g, const gnutls_datum_t *y)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_generate(gnutls_privkey_t key, gnutls_pk_algorithm_t algo, unsigned int bits,
                                          unsigned int flags)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_decode_rs_value(const gnutls_datum_t * sig_value, gnutls_datum_t * r, gnutls_datum_t * s)
{
    return GNUTLS_E_INTERNAL_ERROR;
}

static int compat_gnutls_privkey_import_rsa_raw(gnutls_privkey_t key, const gnutls_datum_t *m, const gnutls_datum_t *e,
                                                const gnutls_datum_t *d, const gnutls_datum_t *p, const gnutls_datum_t *q,
                                                const gnutls_datum_t *u, const gnutls_datum_t *e1, const gnutls_datum_t *e2)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static int compat_gnutls_privkey_decrypt_data(gnutls_privkey_t key, unsigned int flags, const gnutls_datum_t *cipher_text,
                                              gnutls_datum_t *plain_text)
{
    return GNUTLS_E_UNKNOWN_PK_ALGORITHM;
}

static void gnutls_log( int level, const char *msg )
{
    TRACE( "<%d> %s", level, msg );
}

static BOOL gnutls_initialize(void)
{
    const char *env_str;
    int ret;

    if ((env_str = getenv("GNUTLS_SYSTEM_PRIORITY_FILE")))
    {
        WARN("GNUTLS_SYSTEM_PRIORITY_FILE is %s.\n", debugstr_a(env_str));
    }
    else
    {
        WARN("Setting GNUTLS_SYSTEM_PRIORITY_FILE to \"/dev/null\".\n");
        setenv("GNUTLS_SYSTEM_PRIORITY_FILE", "/dev/null", 0);
    }

    if (!(libgnutls_handle = dlopen( SONAME_LIBGNUTLS, RTLD_NOW )))
    {
        ERR_(winediag)( "failed to load libgnutls, no support for encryption\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgnutls_handle, #f ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_cipher_decrypt2)
    LOAD_FUNCPTR(gnutls_cipher_deinit)
    LOAD_FUNCPTR(gnutls_cipher_encrypt2)
    LOAD_FUNCPTR(gnutls_cipher_init)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_privkey_deinit);
    LOAD_FUNCPTR(gnutls_privkey_import_dsa_raw);
    LOAD_FUNCPTR(gnutls_privkey_init);
    LOAD_FUNCPTR(gnutls_privkey_sign_hash);
    LOAD_FUNCPTR(gnutls_pubkey_deinit);
    LOAD_FUNCPTR(gnutls_pubkey_init);
#undef LOAD_FUNCPTR

    if (!(pgnutls_cipher_tag = dlsym( libgnutls_handle, "gnutls_cipher_tag" )))
    {
        WARN("gnutls_cipher_tag not found\n");
        pgnutls_cipher_tag = compat_gnutls_cipher_tag;
    }
    if (!(pgnutls_cipher_add_auth = dlsym( libgnutls_handle, "gnutls_cipher_add_auth" )))
    {
        WARN("gnutls_cipher_add_auth not found\n");
        pgnutls_cipher_add_auth = compat_gnutls_cipher_add_auth;
    }

    if ((ret = pgnutls_global_init()) != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror( ret );
        goto fail;
    }
    if (!(pgnutls_pubkey_import_ecc_raw = dlsym( libgnutls_handle, "gnutls_pubkey_import_ecc_raw" )))
    {
        WARN("gnutls_pubkey_import_ecc_raw not found\n");
        pgnutls_pubkey_import_ecc_raw = compat_gnutls_pubkey_import_ecc_raw;
    }
    if (!(pgnutls_privkey_export_rsa_raw = dlsym( libgnutls_handle, "gnutls_privkey_export_rsa_raw" )))
    {
        WARN("gnutls_privkey_export_rsa_raw not found\n");
        pgnutls_privkey_export_rsa_raw = compat_gnutls_privkey_export_rsa_raw;
    }
    if (!(pgnutls_privkey_export_ecc_raw = dlsym( libgnutls_handle, "gnutls_privkey_export_ecc_raw" )))
    {
        WARN("gnutls_privkey_export_ecc_raw not found\n");
        pgnutls_privkey_export_ecc_raw = compat_gnutls_privkey_export_ecc_raw;
    }
    if (!(pgnutls_privkey_import_ecc_raw = dlsym( libgnutls_handle, "gnutls_privkey_import_ecc_raw" )))
    {
        WARN("gnutls_privkey_import_ecc_raw not found\n");
        pgnutls_privkey_import_ecc_raw = compat_gnutls_privkey_import_ecc_raw;
    }
    if (!(pgnutls_privkey_export_dsa_raw = dlsym( libgnutls_handle, "gnutls_privkey_export_dsa_raw" )))
    {
        WARN("gnutls_privkey_export_dsa_raw not found\n");
        pgnutls_privkey_export_dsa_raw = compat_gnutls_privkey_export_dsa_raw;
    }
    if (!(pgnutls_pk_to_sign = dlsym( libgnutls_handle, "gnutls_pk_to_sign" )))
    {
        WARN("gnutls_pk_to_sign not found\n");
        pgnutls_pk_to_sign = compat_gnutls_pk_to_sign;
    }
    if (!(pgnutls_pubkey_verify_hash2 = dlsym( libgnutls_handle, "gnutls_pubkey_verify_hash2" )))
    {
        WARN("gnutls_pubkey_verify_hash2 not found\n");
        pgnutls_pubkey_verify_hash2 = compat_gnutls_pubkey_verify_hash2;
    }
    if (!(pgnutls_pubkey_import_rsa_raw = dlsym( libgnutls_handle, "gnutls_pubkey_import_rsa_raw" )))
    {
        WARN("gnutls_pubkey_import_rsa_raw not found\n");
        pgnutls_pubkey_import_rsa_raw = compat_gnutls_pubkey_import_rsa_raw;
    }
    if (!(pgnutls_pubkey_import_dsa_raw = dlsym( libgnutls_handle, "gnutls_pubkey_import_dsa_raw" )))
    {
        WARN("gnutls_pubkey_import_dsa_raw not found\n");
        pgnutls_pubkey_import_dsa_raw = compat_gnutls_pubkey_import_dsa_raw;
    }
    if (!(pgnutls_privkey_generate = dlsym( libgnutls_handle, "gnutls_privkey_generate" )))
    {
        WARN("gnutls_privkey_generate not found\n");
        pgnutls_privkey_generate = compat_gnutls_privkey_generate;
    }
    if (!(pgnutls_decode_rs_value = dlsym( libgnutls_handle, "gnutls_decode_rs_value" )))
    {
        WARN("gnutls_decode_rs_value not found\n");
        pgnutls_decode_rs_value = compat_gnutls_decode_rs_value;
    }
    if (!(pgnutls_privkey_import_rsa_raw = dlsym( libgnutls_handle, "gnutls_privkey_import_rsa_raw" )))
    {
        WARN("gnutls_privkey_import_rsa_raw not found\n");
        pgnutls_privkey_import_rsa_raw = compat_gnutls_privkey_import_rsa_raw;
    }
    if (!(pgnutls_privkey_decrypt_data = dlsym( libgnutls_handle, "gnutls_privkey_decrypt_data" )))
    {
        WARN("gnutls_privkey_decrypt_data not found\n");
        pgnutls_privkey_decrypt_data = compat_gnutls_privkey_decrypt_data;
    }

    if (TRACE_ON( bcrypt ))
    {
        pgnutls_global_set_log_level( 4 );
        pgnutls_global_set_log_function( gnutls_log );
    }

    return TRUE;

fail:
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
    return FALSE;
}

static void gnutls_uninitialize(void)
{
    pgnutls_global_deinit();
    dlclose( libgnutls_handle );
    libgnutls_handle = NULL;
}

struct buffer
{
    BYTE  *buffer;
    DWORD  length;
    DWORD  pos;
    BOOL   error;
};

static void buffer_init( struct buffer *buffer )
{
    buffer->buffer = NULL;
    buffer->length = 0;
    buffer->pos    = 0;
    buffer->error  = FALSE;
}

static void buffer_free( struct buffer *buffer )
{
    free( buffer->buffer );
}

static void buffer_append( struct buffer *buffer, BYTE *data, DWORD len )
{
    if (!len) return;

    if (buffer->pos + len > buffer->length)
    {
        DWORD new_length = max( max( buffer->pos + len, buffer->length * 2 ), 64 );
        BYTE *new_buffer;

        if (!(new_buffer = realloc( buffer->buffer, new_length )))
        {
            ERR( "out of memory\n" );
            buffer->error = TRUE;
            return;
        }

        buffer->buffer = new_buffer;
        buffer->length = new_length;
    }

    memcpy( &buffer->buffer[buffer->pos], data, len );
    buffer->pos += len;
}

static void buffer_append_byte( struct buffer *buffer, BYTE value )
{
    buffer_append( buffer, &value, sizeof(value) );
}

static void buffer_append_asn1_length( struct buffer *buffer, DWORD length )
{
    DWORD num_bytes;

    if (length < 128)
    {
        buffer_append_byte( buffer, length );
        return;
    }

    if (length <= 0xff) num_bytes = 1;
    else if (length <= 0xffff) num_bytes = 2;
    else if (length <= 0xffffff) num_bytes = 3;
    else num_bytes = 4;

    buffer_append_byte( buffer, 0x80 | num_bytes );
    while (num_bytes--) buffer_append_byte( buffer, length >> (num_bytes * 8) );
}

static void buffer_append_asn1_integer( struct buffer *buffer, BYTE *data, DWORD len )
{
    DWORD leading_zero = (*data & 0x80) != 0;

    buffer_append_byte( buffer, 0x02 );  /* tag */
    buffer_append_asn1_length( buffer, len + leading_zero );
    if (leading_zero) buffer_append_byte( buffer, 0 );
    buffer_append( buffer, data, len );
}

static void buffer_append_asn1_sequence( struct buffer *buffer, struct buffer *content )
{
    if (content->error)
    {
        buffer->error = TRUE;
        return;
    }

    buffer_append_byte( buffer, 0x30 );  /* tag */
    buffer_append_asn1_length( buffer, content->pos );
    buffer_append( buffer, content->buffer, content->pos );
}

static void buffer_append_asn1_r_s( struct buffer *buffer, BYTE *r, DWORD r_len, BYTE *s, DWORD s_len )
{
    struct buffer value;

    buffer_init( &value );
    buffer_append_asn1_integer( &value, r, r_len );
    buffer_append_asn1_integer( &value, s, s_len );
    buffer_append_asn1_sequence( buffer, &value );
    buffer_free( &value );
}

static NTSTATUS CDECL key_set_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    if (!strcmpW( prop, BCRYPT_CHAINING_MODE ))
    {
        if (!strcmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_ECB ))
        {
            key->u.s.mode = MODE_ID_ECB;
            return STATUS_SUCCESS;
        }
        else if (!strcmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_CBC ))
        {
            key->u.s.mode = MODE_ID_CBC;
            return STATUS_SUCCESS;
        }
        else if (!strcmpW( (WCHAR *)value, BCRYPT_CHAIN_MODE_GCM ))
        {
            key->u.s.mode = MODE_ID_GCM;
            return STATUS_SUCCESS;
        }
        else
        {
            FIXME( "unsupported mode %s\n", debugstr_w((WCHAR *)value) );
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    FIXME( "unsupported key property %s\n", debugstr_w(prop) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_init( struct key *key )
{
    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (key->alg_id)
    {
    case ALG_ID_3DES:
    case ALG_ID_AES:
        return STATUS_SUCCESS;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return STATUS_NOT_SUPPORTED;
    }
}

static gnutls_cipher_algorithm_t get_gnutls_cipher( const struct key *key )
{
    switch (key->alg_id)
    {
    case ALG_ID_3DES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
        case MODE_ID_CBC:
            return GNUTLS_CIPHER_3DES_CBC;
        default:
            break;
        }
        FIXME( "3DES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return GNUTLS_CIPHER_UNKNOWN;

    case ALG_ID_AES:
        WARN( "handle block size\n" );
        switch (key->u.s.mode)
        {
        case MODE_ID_GCM:
            if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_GCM;
            if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_GCM;
            break;
        case MODE_ID_ECB: /* can be emulated with CBC + empty IV */
        case MODE_ID_CBC:
            if (key->u.s.secret_len == 16) return GNUTLS_CIPHER_AES_128_CBC;
            if (key->u.s.secret_len == 24) return GNUTLS_CIPHER_AES_192_CBC;
            if (key->u.s.secret_len == 32) return GNUTLS_CIPHER_AES_256_CBC;
            break;
        default:
            break;
        }
        FIXME( "AES mode %u with key length %u not supported\n", key->u.s.mode, key->u.s.secret_len );
        return GNUTLS_CIPHER_UNKNOWN;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return GNUTLS_CIPHER_UNKNOWN;
    }
}

static void CDECL key_symmetric_vector_reset( struct key *key )
{
    if (!key_data(key)->cipher) return;
    TRACE( "invalidating cipher handle\n" );
    pgnutls_cipher_deinit( key_data(key)->cipher );
    key_data(key)->cipher = NULL;
}

static NTSTATUS init_cipher_handle( struct key *key )
{
    gnutls_cipher_algorithm_t cipher;
    gnutls_datum_t secret, vector;
    int ret;

    if (key_data(key)->cipher) return STATUS_SUCCESS;
    if ((cipher = get_gnutls_cipher( key )) == GNUTLS_CIPHER_UNKNOWN) return STATUS_NOT_SUPPORTED;

    secret.data = key->u.s.secret;
    secret.size = key->u.s.secret_len;

    vector.data = key->u.s.vector;
    vector.size = key->u.s.vector_len;

    if ((ret = pgnutls_cipher_init( &key_data(key)->cipher, cipher, &secret, key->u.s.vector ? &vector : NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    NTSTATUS status;
    int ret;

    if (!auth_data) return STATUS_SUCCESS;
    if ((status = init_cipher_handle( key ))) return status;

    if ((ret = pgnutls_cipher_add_auth( key_data(key)->cipher, auth_data, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    NTSTATUS status;
    int ret;

    if ((status = init_cipher_handle( key ))) return status;

    if ((ret = pgnutls_cipher_encrypt2( key_data(key)->cipher, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    NTSTATUS status;
    int ret;

    if ((status = init_cipher_handle( key ))) return status;

    if ((ret = pgnutls_cipher_decrypt2( key_data(key)->cipher, input, input_len, output, output_len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    NTSTATUS status;
    int ret;

    if ((status = init_cipher_handle( key ))) return status;

    if ((ret = pgnutls_cipher_tag( key_data(key)->cipher, tag, len )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static void CDECL key_symmetric_destroy( struct key *key )
{
    if (key_data(key)->cipher) pgnutls_cipher_deinit( key_data(key)->cipher );
}

static void export_gnutls_datum( UCHAR *buffer, ULONG length, gnutls_datum_t *d, ULONG *actual_length )
{
    ULONG size = d->size;
    UCHAR *src = d->data;
    ULONG offset;

    assert( size <= length + 1 );
    if (size == length + 1)
    {
        assert(!src[0]);
        ++src;
        --size;
    }
    if (actual_length)
    {
        offset = 0;
        *actual_length = size;
    }
    else
    {
        offset = length - size;
        memset( buffer, 0, offset );
    }
    memcpy( buffer + offset, src, size );
}

static NTSTATUS export_gnutls_pubkey_rsa( gnutls_privkey_t gnutls_key, ULONG bitlen, UCHAR **pubkey, ULONG *pubkey_len )
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    gnutls_datum_t m, e;
    UCHAR *dst;
    int ret;

    if ((ret = pgnutls_privkey_export_rsa_raw( gnutls_key, &m, &e, NULL, NULL, NULL, NULL, NULL, NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (!(rsa_blob = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*rsa_blob) + e.size + m.size )))
    {
        pgnutls_perror( ret );
        free( e.data ); free( m.data );
        return STATUS_NO_MEMORY;
    }

    dst = (UCHAR *)(rsa_blob + 1);
    export_gnutls_datum( dst, bitlen / 8, &e, &rsa_blob->cbPublicExp );

    dst += rsa_blob->cbPublicExp;
    export_gnutls_datum( dst, bitlen / 8, &m, &rsa_blob->cbModulus );

    rsa_blob->Magic       = BCRYPT_RSAPUBLIC_MAGIC;
    rsa_blob->BitLength   = bitlen;
    rsa_blob->cbPrime1    = 0;
    rsa_blob->cbPrime2    = 0;

    *pubkey = (UCHAR *)rsa_blob;
    *pubkey_len = sizeof(*rsa_blob) + rsa_blob->cbPublicExp + rsa_blob->cbModulus;

    free( e.data ); free( m.data );
    return STATUS_SUCCESS;
}

static NTSTATUS export_gnutls_pubkey_ecc( gnutls_privkey_t gnutls_key, enum alg_id alg_id, UCHAR **pubkey,
                                          ULONG *pubkey_len )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y;
    DWORD magic, size;
    UCHAR *dst;
    int ret;

    switch (alg_id)
    {
    case ALG_ID_ECDH_P256:
        magic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
        size = 32;
        break;
    case ALG_ID_ECDSA_P256:
        magic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
        size = 32;
        break;
    default:
        FIXME( "algorithm %u not supported\n", alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_privkey_export_ecc_raw( gnutls_key, &curve, &x, &y, NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (curve != GNUTLS_ECC_CURVE_SECP256R1)
    {
        FIXME( "curve %u not supported\n", curve );
        free( x.data ); free( y.data );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(ecc_blob = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ecc_blob) + size * 2 )))
    {
        pgnutls_perror( ret );
        free( x.data ); free( y.data );
        return STATUS_NO_MEMORY;
    }

    ecc_blob->dwMagic = magic;
    ecc_blob->cbKey   = size;

    dst = (UCHAR *)(ecc_blob + 1);
    export_gnutls_datum( dst, size, &x, NULL );

    dst += size;
    export_gnutls_datum( dst, size, &y, NULL );

    *pubkey = (UCHAR *)ecc_blob;
    *pubkey_len = sizeof(*ecc_blob) + ecc_blob->cbKey * 2;

    free( x.data ); free( y.data );
    return STATUS_SUCCESS;
}

static NTSTATUS export_gnutls_pubkey_dsa( gnutls_privkey_t gnutls_key, ULONG bitlen, UCHAR **pubkey, ULONG *pubkey_len )
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob;
    gnutls_datum_t p, q, g, y;
    UCHAR *dst;
    int ret;

    if ((ret = pgnutls_privkey_export_dsa_raw( gnutls_key, &p, &q, &g, &y, NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (bitlen > 1024)
    {
        FIXME( "bitlen > 1024 not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(dsa_blob = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*dsa_blob) + bitlen / 8 * 3 )))
    {
        pgnutls_perror( ret );
        free( p.data ); free( q.data ); free( g.data ); free( y.data );
        return STATUS_NO_MEMORY;
    }

    dst = (UCHAR *)(dsa_blob + 1);
    export_gnutls_datum( dst, bitlen / 8, &p, NULL );

    dst += bitlen / 8;
    export_gnutls_datum( dst, bitlen / 8, &g, NULL );

    dst += bitlen / 8;
    export_gnutls_datum( dst, bitlen / 8, &y, NULL );

    dst = dsa_blob->q;
    export_gnutls_datum( dst, sizeof(dsa_blob->q), &q, NULL );

    dsa_blob->dwMagic = BCRYPT_DSA_PUBLIC_MAGIC;
    dsa_blob->cbKey   = bitlen / 8;
    memset( dsa_blob->Count, 0, sizeof(dsa_blob->Count) ); /* FIXME */
    memset( dsa_blob->Seed, 0, sizeof(dsa_blob->Seed) ); /* FIXME */

    *pubkey = (UCHAR *)dsa_blob;
    *pubkey_len = sizeof(*dsa_blob) + dsa_blob->cbKey * 3;

    free( p.data ); free( q.data ); free( g.data ); free( y.data );
    return STATUS_SUCCESS;
}

static void reverse_bytes( UCHAR *buf, ULONG len )
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
static NTSTATUS export_gnutls_pubkey_dsa_capi( gnutls_privkey_t gnutls_key, const DSSSEED *seed, ULONG bitlen,
                                               UCHAR **pubkey, ULONG *pubkey_len )
{
    BLOBHEADER *hdr;
    DSSPUBKEY *dsskey;
    gnutls_datum_t p, q, g, y;
    UCHAR *dst;
    int ret, size = sizeof(*hdr) + sizeof(*dsskey) + sizeof(*seed);

    if (bitlen > 1024)
    {
        FIXME( "bitlen > 1024 not supported\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_privkey_export_dsa_raw( gnutls_key, &p, &q, &g, &y, NULL )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (!(hdr = RtlAllocateHeap( GetProcessHeap(), 0, size + bitlen / 8 * 3 + Q_SIZE )))
    {
        pgnutls_perror( ret );
        free( p.data ); free( q.data ); free( g.data ); free( y.data );
        return STATUS_NO_MEMORY;
    }

    hdr->bType    = PUBLICKEYBLOB;
    hdr->bVersion = 2;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_DSS_SIGN;

    dsskey = (DSSPUBKEY *)(hdr + 1);
    dsskey->magic  = MAGIC_DSS1;
    dsskey->bitlen = bitlen;

    dst = (UCHAR *)(dsskey + 1);
    export_gnutls_datum( dst, bitlen / 8, &p, NULL );
    reverse_bytes( dst, bitlen / 8 );
    dst += bitlen / 8;

    export_gnutls_datum( dst, Q_SIZE, &q, NULL );
    reverse_bytes( dst, Q_SIZE );
    dst += Q_SIZE;

    export_gnutls_datum( dst, bitlen / 8, &g, NULL );
    reverse_bytes( dst, bitlen / 8 );
    dst += bitlen / 8;

    export_gnutls_datum( dst, bitlen / 8, &y, NULL );
    reverse_bytes( dst, bitlen / 8 );
    dst += bitlen / 8;

    memcpy( dst, seed, sizeof(*seed) );

    *pubkey = (UCHAR *)hdr;
    *pubkey_len = size + bitlen / 8 * 3 + Q_SIZE;

    free( p.data ); free( q.data ); free( g.data ); free( y.data );
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_asymmetric_generate( struct key *key )
{
    gnutls_pk_algorithm_t pk_alg;
    gnutls_privkey_t handle;
    unsigned int bitlen;
    NTSTATUS status;
    int ret;

    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (key->alg_id)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        pk_alg = GNUTLS_PK_RSA;
        bitlen = key->u.a.bitlen;
        break;

    case ALG_ID_DSA:
        pk_alg = GNUTLS_PK_DSA;
        bitlen = key->u.a.bitlen;
        break;

    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDSA_P256:
        pk_alg = GNUTLS_PK_ECC; /* compatible with ECDSA and ECDH */
        bitlen = GNUTLS_CURVE_TO_BITS( GNUTLS_ECC_CURVE_SECP256R1 );
        break;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return STATUS_NOT_SUPPORTED;
    }

    if ((ret = pgnutls_privkey_init( &handle )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if ((ret = pgnutls_privkey_generate( handle, pk_alg, bitlen, 0 )))
    {
        pgnutls_perror( ret );
        pgnutls_privkey_deinit( handle );
        return STATUS_INTERNAL_ERROR;
    }

    switch (pk_alg)
    {
    case GNUTLS_PK_RSA:
        status = export_gnutls_pubkey_rsa( handle, key->u.a.bitlen, &key->u.a.pubkey, &key->u.a.pubkey_len );
        break;

    case GNUTLS_PK_ECC:
        status = export_gnutls_pubkey_ecc( handle, key->alg_id, &key->u.a.pubkey, &key->u.a.pubkey_len );
        break;

    case GNUTLS_PK_DSA:
        status = export_gnutls_pubkey_dsa( handle, key->u.a.bitlen, &key->u.a.pubkey, &key->u.a.pubkey_len );
        break;

    default:
        ERR( "unhandled algorithm %u\n", pk_alg );
        return STATUS_INTERNAL_ERROR;
    }

    if (status)
    {
        pgnutls_privkey_deinit( handle );
        return status;
    }

    key_data(key)->privkey = handle;
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_export_ecc( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y, d;
    DWORD magic, size;
    UCHAR *dst;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDH_P256:
        magic = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
        size = 32;
        break;
    case ALG_ID_ECDSA_P256:
        magic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
        size = 32;
        break;

    default:
        FIXME( "algorithm %u does not yet support exporting ecc blob\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_privkey_export_ecc_raw( key_data(key)->privkey, &curve, &x, &y, &d )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (curve != GNUTLS_ECC_CURVE_SECP256R1)
    {
        FIXME( "curve %u not supported\n", curve );
        free( x.data ); free( y.data ); free( d.data );
        return STATUS_NOT_IMPLEMENTED;
    }

    *ret_len = sizeof(*ecc_blob) + size * 3;
    if (len >= *ret_len && buf)
    {
        ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
        ecc_blob->dwMagic = magic;
        ecc_blob->cbKey   = size;

        dst = (UCHAR *)(ecc_blob + 1);
        export_gnutls_datum( dst, size, &x, NULL );
        dst += size;

        export_gnutls_datum( dst, size, &y, NULL );
        dst += size;

        export_gnutls_datum( dst, size, &d, NULL );
    }

    free( x.data ); free( y.data ); free( d.data );
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_import_ecc( struct key *key, UCHAR *buf, ULONG len )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_privkey_t handle;
    gnutls_datum_t x, y, k;
    NTSTATUS status;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDSA_P256:
        curve = GNUTLS_ECC_CURVE_SECP256R1;
        break;

    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_privkey_init( &handle )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)buf;
    x.data = (unsigned char *)(ecc_blob + 1);
    x.size = ecc_blob->cbKey;
    y.data = x.data + ecc_blob->cbKey;
    y.size = ecc_blob->cbKey;
    k.data = y.data + ecc_blob->cbKey;
    k.size = ecc_blob->cbKey;

    if ((ret = pgnutls_privkey_import_ecc_raw( handle, curve, &x, &y, &k )))
    {
        pgnutls_perror( ret );
        pgnutls_privkey_deinit( handle );
        return STATUS_INTERNAL_ERROR;
    }

    if ((status = export_gnutls_pubkey_ecc( handle, key->alg_id, &key->u.a.pubkey, &key->u.a.pubkey_len )))
    {
        pgnutls_privkey_deinit( handle );
        return status;
    }

    key_data(key)->privkey = handle;
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_import_rsa( struct key *key, UCHAR *buf, ULONG len )
{
    BCRYPT_RSAKEY_BLOB *rsa_blob = (BCRYPT_RSAKEY_BLOB *)buf;
    gnutls_datum_t m, e, p, q;
    gnutls_privkey_t handle;
    int ret;

    if ((ret = pgnutls_privkey_init( &handle )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    e.data = (unsigned char *)(rsa_blob + 1);
    e.size = rsa_blob->cbPublicExp;
    m.data = e.data + e.size;
    m.size = rsa_blob->cbModulus;
    p.data = m.data + m.size;
    p.size = rsa_blob->cbPrime1;
    q.data = p.data + p.size;
    q.size = rsa_blob->cbPrime2;

    if ((ret = pgnutls_privkey_import_rsa_raw( handle, &m, &e, NULL, &p, &q, NULL, NULL, NULL )))
    {
        pgnutls_perror( ret );
        pgnutls_privkey_deinit( handle );
        return STATUS_INTERNAL_ERROR;
    }

    key_data(key)->privkey = handle;
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_export_dsa_capi( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    BLOBHEADER *hdr;
    DSSPUBKEY *pubkey;
    gnutls_datum_t p, q, g, y, x;
    UCHAR *dst;
    int ret, size;

    if ((ret = pgnutls_privkey_export_dsa_raw( key_data(key)->privkey, &p, &q, &g, &y, &x )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    if (q.size > 21 || x.size > 21)
    {
        ERR( "can't export key in this format\n" );
        free( p.data ); free( q.data ); free( g.data ); free( y.data ); free( x.data );
        return STATUS_NOT_SUPPORTED;
    }

    size = key->u.a.bitlen / 8;
    *ret_len = sizeof(*hdr) + sizeof(*pubkey) + size * 2 + 40 + sizeof(key->u.a.dss_seed);
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
        export_gnutls_datum( dst, size, &p, NULL );
        reverse_bytes( dst, size );
        dst += size;

        export_gnutls_datum( dst, 20, &q, NULL );
        reverse_bytes( dst, 20 );
        dst += 20;

        export_gnutls_datum( dst, size, &g, NULL );
        reverse_bytes( dst, size );
        dst += size;

        export_gnutls_datum( dst, 20, &x, NULL );
        reverse_bytes( dst, 20 );
        dst += 20;

        memcpy( dst, &key->u.a.dss_seed, sizeof(key->u.a.dss_seed) );
    }

    free( p.data ); free( q.data ); free( g.data ); free( y.data ); free( x.data );
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_import_dsa_capi( struct key *key, UCHAR *buf, ULONG len )
{
    BLOBHEADER *hdr = (BLOBHEADER *)buf;
    DSSPUBKEY *pubkey;
    gnutls_privkey_t handle;
    gnutls_datum_t p, q, g, y, x;
    unsigned char dummy[128];
    unsigned char *data, p_data[128], q_data[20], g_data[128], x_data[20];
    int i, ret, size;
    NTSTATUS status;

    if ((ret = pgnutls_privkey_init( &handle )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    hdr = (BLOBHEADER *)buf;
    pubkey = (DSSPUBKEY *)(hdr + 1);
    if ((size = pubkey->bitlen / 8) > sizeof(p_data))
    {
        FIXME( "size %u not supported\n", size );
        pgnutls_privkey_deinit( handle );
        return STATUS_NOT_SUPPORTED;
    }
    data = (unsigned char *)(pubkey + 1);

    p.data = p_data;
    p.size = size;
    for (i = 0; i < p.size; i++) p.data[i] = data[p.size - i - 1];
    data += p.size;

    q.data = q_data;
    q.size = sizeof(q_data);
    for (i = 0; i < q.size; i++) q.data[i] = data[q.size - i - 1];
    data += q.size;

    g.data = g_data;
    g.size = size;
    for (i = 0; i < g.size; i++) g.data[i] = data[g.size - i - 1];
    data += g.size;

    x.data = x_data;
    x.size = sizeof(x_data);
    for (i = 0; i < x.size; i++) x.data[i] = data[x.size - i - 1];
    data += x.size;

    WARN( "using dummy public key\n" );
    memset( dummy, 1, sizeof(dummy) );
    y.data = dummy;
    y.size = min( p.size, sizeof(dummy) );

    if ((ret = pgnutls_privkey_import_dsa_raw( handle, &p, &q, &g, &y, &x )))
    {
        pgnutls_perror( ret );
        pgnutls_privkey_deinit( handle );
        return STATUS_INTERNAL_ERROR;
    }

    if ((status = export_gnutls_pubkey_dsa_capi( handle, &key->u.a.dss_seed, key->u.a.bitlen, &key->u.a.pubkey,
                                                 &key->u.a.pubkey_len )))
    {
        pgnutls_privkey_deinit( handle );
        return status;
    }

    memcpy( &key->u.a.dss_seed, data, sizeof(key->u.a.dss_seed) );

    key->u.a.flags |= KEY_FLAG_LEGACY_DSA_V2;
    key_data(key)->privkey = handle;

    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_asymmetric_init( struct key *key )
{
    if (!libgnutls_handle) return STATUS_INTERNAL_ERROR;

    switch (key->alg_id)
    {
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
    case ALG_ID_DSA:
        return STATUS_SUCCESS;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS import_gnutls_pubkey_ecc( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_ECCKEY_BLOB *ecc_blob;
    gnutls_ecc_curve_t curve;
    gnutls_datum_t x, y;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256: curve = GNUTLS_ECC_CURVE_SECP256R1; break;
    case ALG_ID_ECDSA_P384: curve = GNUTLS_ECC_CURVE_SECP384R1; break;

    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    ecc_blob = (BCRYPT_ECCKEY_BLOB *)key->u.a.pubkey;
    x.data = key->u.a.pubkey + sizeof(*ecc_blob);
    x.size = ecc_blob->cbKey;
    y.data = key->u.a.pubkey + sizeof(*ecc_blob) + ecc_blob->cbKey;
    y.size = ecc_blob->cbKey;

    if ((ret = pgnutls_pubkey_import_ecc_raw( *gnutls_key, curve, &x, &y )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_rsa( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_RSAKEY_BLOB *rsa_blob;
    gnutls_datum_t m, e;
    int ret;

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    rsa_blob = (BCRYPT_RSAKEY_BLOB *)key->u.a.pubkey;
    e.data = key->u.a.pubkey + sizeof(*rsa_blob);
    e.size = rsa_blob->cbPublicExp;
    m.data = key->u.a.pubkey + sizeof(*rsa_blob) + rsa_blob->cbPublicExp;
    m.size = rsa_blob->cbModulus;

    if ((ret = pgnutls_pubkey_import_rsa_raw( *gnutls_key, &m, &e )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_dsa( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BCRYPT_DSA_KEY_BLOB *dsa_blob;
    gnutls_datum_t p, q, g, y;
    int ret;

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    dsa_blob = (BCRYPT_DSA_KEY_BLOB *)key->u.a.pubkey;
    p.data = key->u.a.pubkey + sizeof(*dsa_blob);
    p.size = dsa_blob->cbKey;
    q.data = dsa_blob->q;
    q.size = sizeof(dsa_blob->q);
    g.data = key->u.a.pubkey + sizeof(*dsa_blob) + dsa_blob->cbKey;
    g.size = dsa_blob->cbKey;
    y.data = key->u.a.pubkey + sizeof(*dsa_blob) + dsa_blob->cbKey * 2;
    y.size = dsa_blob->cbKey;

    if ((ret = pgnutls_pubkey_import_dsa_raw( *gnutls_key, &p, &q, &g, &y )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey_dsa_capi( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    BLOBHEADER *hdr;
    DSSPUBKEY *pubkey;
    gnutls_datum_t p, q, g, y;
    unsigned char *data, p_data[128], q_data[20], g_data[128], y_data[128];
    int i, ret, size;

    if ((ret = pgnutls_pubkey_init( gnutls_key )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    hdr = (BLOBHEADER *)key->u.a.pubkey;
    pubkey = (DSSPUBKEY *)(hdr + 1);
    size = pubkey->bitlen / 8;
    data = (unsigned char *)(pubkey + 1);

    p.data = p_data;
    p.size = size;
    for (i = 0; i < p.size; i++) p.data[i] = data[p.size - i - 1];
    data += p.size;

    q.data = q_data;
    q.size = sizeof(q_data);
    for (i = 0; i < q.size; i++) q.data[i] = data[q.size - i - 1];
    data += q.size;

    g.data = g_data;
    g.size = size;
    for (i = 0; i < g.size; i++) g.data[i] = data[g.size - i - 1];
    data += g.size;

    y.data = y_data;
    y.size = sizeof(y_data);
    for (i = 0; i < y.size; i++) y.data[i] = data[y.size - i - 1];

    if ((ret = pgnutls_pubkey_import_dsa_raw( *gnutls_key, &p, &q, &g, &y )))
    {
        pgnutls_perror( ret );
        pgnutls_pubkey_deinit( *gnutls_key );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS import_gnutls_pubkey( struct key *key, gnutls_pubkey_t *gnutls_key )
{
    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
        return import_gnutls_pubkey_ecc( key, gnutls_key );

    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        return import_gnutls_pubkey_rsa( key, gnutls_key );

    case ALG_ID_DSA:
        if (key->u.a.flags & KEY_FLAG_LEGACY_DSA_V2)
            return import_gnutls_pubkey_dsa_capi( key, gnutls_key );
        else
            return import_gnutls_pubkey_dsa( key, gnutls_key );

    default:
        FIXME("algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS prepare_gnutls_signature_dsa( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    struct buffer buffer;
    DWORD r_len = signature_len / 2;
    DWORD s_len = r_len;
    BYTE *r = signature;
    BYTE *s = signature + r_len;

    buffer_init( &buffer );
    buffer_append_asn1_r_s( &buffer, r, r_len, s, s_len );
    if (buffer.error)
    {
        buffer_free( &buffer );
        return STATUS_NO_MEMORY;
    }

    gnutls_signature->data = buffer.buffer;
    gnutls_signature->size = buffer.pos;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature_rsa( struct key *key, UCHAR *signature, ULONG signature_len,
                                              gnutls_datum_t *gnutls_signature )
{
    gnutls_signature->data = signature;
    gnutls_signature->size = signature_len;
    return STATUS_SUCCESS;
}

static NTSTATUS prepare_gnutls_signature( struct key *key, UCHAR *signature, ULONG signature_len,
                                          gnutls_datum_t *gnutls_signature )
{
    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_DSA:
        return prepare_gnutls_signature_dsa( key, signature, signature_len, gnutls_signature );

    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
        return prepare_gnutls_signature_rsa( key, signature, signature_len, gnutls_signature );

    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }
}

static gnutls_digest_algorithm_t get_digest_from_id( const WCHAR *alg_id )
{
    if (!strcmpW( alg_id, BCRYPT_SHA1_ALGORITHM ))   return GNUTLS_DIG_SHA1;
    if (!strcmpW( alg_id, BCRYPT_SHA256_ALGORITHM )) return GNUTLS_DIG_SHA256;
    if (!strcmpW( alg_id, BCRYPT_SHA384_ALGORITHM )) return GNUTLS_DIG_SHA384;
    if (!strcmpW( alg_id, BCRYPT_SHA512_ALGORITHM )) return GNUTLS_DIG_SHA512;
    if (!strcmpW( alg_id, BCRYPT_MD2_ALGORITHM ))    return GNUTLS_DIG_MD2;
    if (!strcmpW( alg_id, BCRYPT_MD5_ALGORITHM ))    return GNUTLS_DIG_MD5;
    return -1;
}

static NTSTATUS CDECL key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                             UCHAR *signature, ULONG signature_len, DWORD flags )
{
    gnutls_digest_algorithm_t hash_alg;
    gnutls_sign_algorithm_t sign_alg;
    gnutls_datum_t gnutls_hash, gnutls_signature;
    gnutls_pk_algorithm_t pk_alg;
    gnutls_pubkey_t gnutls_key;
    NTSTATUS status;
    int ret;

    switch (key->alg_id)
    {
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    {
        if (flags) FIXME( "flags %08x not supported\n", flags );

        /* only the hash size must match, not the actual hash function */
        switch (hash_len)
        {
        case 20: hash_alg = GNUTLS_DIG_SHA1; break;
        case 32: hash_alg = GNUTLS_DIG_SHA256; break;
        case 48: hash_alg = GNUTLS_DIG_SHA384; break;

        default:
            FIXME( "hash size %u not yet supported\n", hash_len );
            return STATUS_INVALID_SIGNATURE;
        }
        pk_alg = GNUTLS_PK_ECC;
        break;
    }
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
    {
        BCRYPT_PKCS1_PADDING_INFO *info = (BCRYPT_PKCS1_PADDING_INFO *)padding;

        if (!(flags & BCRYPT_PAD_PKCS1) || !info) return STATUS_INVALID_PARAMETER;
        if (!info->pszAlgId) return STATUS_INVALID_SIGNATURE;

        if ((hash_alg = get_digest_from_id(info->pszAlgId)) == -1)
        {
            FIXME( "hash algorithm %s not supported\n", debugstr_w(info->pszAlgId) );
            return STATUS_NOT_SUPPORTED;
        }
        pk_alg = GNUTLS_PK_RSA;
        break;
    }
    case ALG_ID_DSA:
    {
        if (flags) FIXME( "flags %08x not supported\n", flags );
        if (hash_len != 20)
        {
            FIXME( "hash size %u not supported\n", hash_len );
            return STATUS_INVALID_PARAMETER;
        }
        hash_alg = GNUTLS_DIG_SHA1;
        pk_alg = GNUTLS_PK_DSA;
        break;
    }
    default:
        FIXME( "algorithm %u not yet supported\n", key->alg_id );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((sign_alg = pgnutls_pk_to_sign( pk_alg, hash_alg )) == GNUTLS_SIGN_UNKNOWN)
    {
        FIXME("GnuTLS does not support algorithm %u with hash len %u\n", key->alg_id, hash_len );
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = import_gnutls_pubkey( key, &gnutls_key ))) return status;
    if ((status = prepare_gnutls_signature( key, signature, signature_len, &gnutls_signature )))
    {
        pgnutls_pubkey_deinit( gnutls_key );
        return status;
    }

    gnutls_hash.data = hash;
    gnutls_hash.size = hash_len;
    ret = pgnutls_pubkey_verify_hash2( gnutls_key, sign_alg, 0, &gnutls_hash, &gnutls_signature );

    if (gnutls_signature.data != signature) free( gnutls_signature.data );
    pgnutls_pubkey_deinit( gnutls_key );
    return (ret < 0) ? STATUS_INVALID_SIGNATURE : STATUS_SUCCESS;
}

static unsigned int get_signature_length( enum alg_id id )
{
    switch (id)
    {
    case ALG_ID_ECDSA_P256: return 64;
    case ALG_ID_ECDSA_P384: return 96;
    case ALG_ID_DSA:        return 40;
    default:
        FIXME( "unhandled algorithm %u\n", id );
        return 0;
    }
}

static NTSTATUS format_gnutls_signature( enum alg_id type, gnutls_datum_t signature,
                                         UCHAR *output, ULONG output_len, ULONG *ret_len )
{
    switch (type)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
    {
        *ret_len = signature.size;
        if (output_len < signature.size) return STATUS_BUFFER_TOO_SMALL;
        if (output) memcpy( output, signature.data, signature.size );
        return STATUS_SUCCESS;
    }
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    case ALG_ID_DSA:
    {
        int err;
        unsigned int sig_len = get_signature_length( type );
        gnutls_datum_t r, s; /* format as r||s */

        if ((err = pgnutls_decode_rs_value( &signature, &r, &s )))
        {
            pgnutls_perror( err );
            return STATUS_INTERNAL_ERROR;
        }

        *ret_len = sig_len;
        if (output_len < sig_len) return STATUS_BUFFER_TOO_SMALL;

        if (r.size > sig_len / 2 + 1 || s.size > sig_len / 2 + 1)
        {
            ERR( "we didn't get a correct signature\n" );
            return STATUS_INTERNAL_ERROR;
        }

        if (output)
        {
            export_gnutls_datum( output, sig_len / 2, &r, NULL );
            export_gnutls_datum( output + sig_len / 2, sig_len / 2, &s, NULL );
        }

        free( r.data ); free( s.data );
        return STATUS_SUCCESS;
    }
    default:
        return STATUS_INTERNAL_ERROR;
    }
}

static NTSTATUS CDECL key_asymmetric_sign( struct key *key, void *padding, UCHAR *input, ULONG input_len, UCHAR *output,
                                           ULONG output_len, ULONG *ret_len, ULONG flags )
{
    BCRYPT_PKCS1_PADDING_INFO *pad = padding;
    gnutls_datum_t hash, signature;
    gnutls_digest_algorithm_t hash_alg;
    NTSTATUS status;
    int ret;

    if (key->alg_id == ALG_ID_ECDSA_P256 || key->alg_id == ALG_ID_ECDSA_P384)
    {
        /* With ECDSA, we find the digest algorithm from the hash length, and verify it */
        switch (input_len)
        {
        case 20: hash_alg = GNUTLS_DIG_SHA1; break;
        case 32: hash_alg = GNUTLS_DIG_SHA256; break;
        case 48: hash_alg = GNUTLS_DIG_SHA384; break;
        case 64: hash_alg = GNUTLS_DIG_SHA512; break;

        default:
            FIXME( "hash size %u not yet supported\n", input_len );
            return STATUS_INVALID_PARAMETER;
        }

        if (flags == BCRYPT_PAD_PKCS1 && pad && pad->pszAlgId && get_digest_from_id( pad->pszAlgId ) != hash_alg)
        {
            WARN( "incorrect hashing algorithm %s, expected %u\n", debugstr_w(pad->pszAlgId), hash_alg );
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (key->alg_id == ALG_ID_DSA)
    {
        if (flags) FIXME( "flags %08x not supported\n", flags );
        if (input_len != 20)
        {
            FIXME( "hash size %u not supported\n", input_len );
            return STATUS_INVALID_PARAMETER;
        }
        hash_alg = GNUTLS_DIG_SHA1;
    }
    else if (flags == BCRYPT_PAD_PKCS1)
    {
        if (!pad || !pad->pszAlgId)
        {
            WARN( "padding info not found\n" );
            return STATUS_INVALID_PARAMETER;
        }

        if ((hash_alg = get_digest_from_id( pad->pszAlgId )) == -1)
        {
            FIXME( "hash algorithm %s not recognized\n", debugstr_w(pad->pszAlgId) );
            return STATUS_NOT_SUPPORTED;
        }
    }
    else if (!flags)
    {
         WARN( "invalid flags %08x\n", flags );
         return STATUS_INVALID_PARAMETER;
    }
    else
    {
        FIXME( "flags %08x not implemented\n", flags );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!input)
    {
        *ret_len = key->u.a.bitlen / 8;
        return STATUS_SUCCESS;
    }
    if (!key_data(key)->privkey) return STATUS_INVALID_PARAMETER;

    hash.data = input;
    hash.size = input_len;

    signature.data = NULL;
    signature.size = 0;

    if ((ret = pgnutls_privkey_sign_hash( key_data(key)->privkey, hash_alg, 0, &hash, &signature )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    status = format_gnutls_signature( key->alg_id, signature, output, output_len, ret_len );

    free( signature.data );
    return status;
}

static void CDECL key_asymmetric_destroy( struct key *key )
{
    if (key_data(key)->privkey) pgnutls_privkey_deinit( key_data(key)->privkey );
}

static NTSTATUS CDECL key_asymmetric_duplicate( struct key *key_orig, struct key *key_copy )
{
    int ret;

    if (!key_data(key_orig)->privkey) return STATUS_SUCCESS;

    if ((ret = pgnutls_privkey_init( &key_data(key_copy)->privkey )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    switch (key_orig->alg_id)
    {
    case ALG_ID_RSA:
    case ALG_ID_RSA_SIGN:
    {
        gnutls_datum_t m, e, d, p, q, u, e1, e2;
        if ((ret = pgnutls_privkey_export_rsa_raw( key_data(key_orig)->privkey, &m, &e, &d, &p, &q, &u, &e1, &e2 )))
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        ret = pgnutls_privkey_import_rsa_raw( key_data(key_copy)->privkey, &m, &e, &d, &p, &q, &u, &e1, &e2 );
        free( m.data ); free( e.data ); free( d.data ); free( p.data ); free( q.data ); free( u.data );
        free( e1.data ); free( e2.data );
        if (ret)
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        break;
    }
    case ALG_ID_DSA:
    {
        gnutls_datum_t p, q, g, y, x;
        if ((ret = pgnutls_privkey_export_dsa_raw( key_data(key_orig)->privkey, &p, &q, &g, &y, &x )))
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        ret = pgnutls_privkey_import_dsa_raw( key_data(key_copy)->privkey, &p, &q, &g, &y, &x );
        free( p.data ); free( q.data ); free( g.data ); free( y.data ); free( x.data );
        if (ret)
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        break;
    }
    case ALG_ID_ECDH_P256:
    case ALG_ID_ECDSA_P256:
    case ALG_ID_ECDSA_P384:
    {
        gnutls_ecc_curve_t curve;
        gnutls_datum_t x, y, k;
        if ((ret = pgnutls_privkey_export_ecc_raw( key_data(key_orig)->privkey, &curve, &x, &y, &k )))
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        ret = pgnutls_privkey_import_ecc_raw( key_data(key_copy)->privkey, curve, &x, &y, &k );
        free( x.data ); free( y.data ); free( k.data );
        if (ret)
        {
            pgnutls_perror( ret );
            return STATUS_INTERNAL_ERROR;
        }
        break;
    }
    default:
        ERR( "unhandled algorithm %u\n", key_orig->alg_id );
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_asymmetric_decrypt( struct key *key, UCHAR *input, ULONG input_len, UCHAR *output,
                                              ULONG output_len, ULONG *ret_len )
{
    gnutls_datum_t e, d = { 0 };
    NTSTATUS status = STATUS_SUCCESS;
    int ret;

    e.data = input;
    e.size = input_len;
    if ((ret = pgnutls_privkey_decrypt_data( key_data(key)->privkey, 0, &e, &d )))
    {
        pgnutls_perror( ret );
        return STATUS_INTERNAL_ERROR;
    }

    *ret_len = d.size;
    if (output_len >= d.size) memcpy( output, d.data, *ret_len );
    else status = STATUS_BUFFER_TOO_SMALL;

    free( d.data );
    return status;
}

static const struct key_funcs key_funcs =
{
    key_set_property,
    key_symmetric_init,
    key_symmetric_vector_reset,
    key_symmetric_set_auth_data,
    key_symmetric_encrypt,
    key_symmetric_decrypt,
    key_symmetric_get_tag,
    key_symmetric_destroy,
    key_asymmetric_init,
    key_asymmetric_generate,
    key_asymmetric_decrypt,
    key_asymmetric_duplicate,
    key_asymmetric_sign,
    key_asymmetric_verify,
    key_asymmetric_destroy,
    key_export_dsa_capi,
    key_export_ecc,
    key_import_dsa_capi,
    key_import_ecc,
    key_import_rsa
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!gnutls_initialize()) return STATUS_DLL_NOT_FOUND;
        *(const struct key_funcs **)ptr_out = &key_funcs;
        break;
    case DLL_PROCESS_DETACH:
        if (libgnutls_handle) gnutls_uninitialize();
        break;
    }
    return STATUS_SUCCESS;
}

#endif /* HAVE_GNUTLS_CIPHER_INIT */
