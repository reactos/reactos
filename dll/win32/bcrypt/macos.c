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

#include <stdarg.h>
#ifdef HAVE_COMMONCRYPTO_COMMONCRYPTOR_H
#include <AvailabilityMacros.h>
#include <CommonCrypto/CommonCryptor.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#if defined(HAVE_COMMONCRYPTO_COMMONCRYPTOR_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1080 && !defined(HAVE_GNUTLS_CIPHER_INIT)
WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

struct key_data
{
    CCCryptorRef   ref_encrypt;
    CCCryptorRef   ref_decrypt;
};
C_ASSERT( sizeof(struct key_data) <= sizeof(((struct key *)0)->private) );

static struct key_data *key_data( struct key *key )
{
    return (struct key_data *)key->private;
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
    switch (key->alg_id)
    {
    case ALG_ID_AES:
        switch (key->u.s.mode)
        {
        case MODE_ID_ECB:
        case MODE_ID_CBC:
            break;
        default:
            FIXME( "mode %u not supported\n", key->u.s.mode );
            return STATUS_NOT_SUPPORTED;
        }
        return STATUS_SUCCESS;

    default:
        FIXME( "algorithm %u not supported\n", key->alg_id );
        return STATUS_NOT_SUPPORTED;
    }
}

static CCMode get_cryptor_mode( struct key *key )
{
    switch (key->u.s.mode)
    {
    case MODE_ID_ECB: return kCCModeECB;
    case MODE_ID_CBC: return kCCModeCBC;
    default:
        FIXME( "unsupported mode %u\n", key->u.s.mode );
        return 0;
    }
}

static void CDECL key_symmetric_vector_reset( struct key *key )
{
    if (!key_data(key)->ref_encrypt) return;

    TRACE( "invalidating cryptor handles\n" );
    CCCryptorRelease( key_data(key)->ref_encrypt );
    key_data(key)->ref_encrypt = NULL;

    CCCryptorRelease( key_data(key)->ref_decrypt );
    key_data(key)->ref_decrypt = NULL;
}

static NTSTATUS init_cryptor_handles( struct key *key )
{
    CCCryptorStatus status;
    CCMode mode;

    if (key_data(key)->ref_encrypt) return STATUS_SUCCESS;
    if (!(mode = get_cryptor_mode( key ))) return STATUS_NOT_SUPPORTED;

    if ((status = CCCryptorCreateWithMode( kCCEncrypt, mode, kCCAlgorithmAES128, ccNoPadding, key->u.s.vector,
                                           key->u.s.secret, key->u.s.secret_len, NULL, 0, 0, 0,
                                           &key_data(key)->ref_encrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    if ((status = CCCryptorCreateWithMode( kCCDecrypt, mode, kCCAlgorithmAES128, ccNoPadding, key->u.s.vector,
                                           key->u.s.secret, key->u.s.secret_len, NULL, 0, 0, 0,
                                           &key_data(key)->ref_decrypt )) != kCCSuccess)
    {
        WARN( "CCCryptorCreateWithMode failed %d\n", status );
        CCCryptorRelease( key_data(key)->ref_encrypt );
        key_data(key)->ref_encrypt = NULL;
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    CCCryptorStatus status;
    NTSTATUS ret;

    if ((ret = init_cryptor_handles( key ))) return ret;

    if ((status = CCCryptorUpdate( key_data(key)->ref_encrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    CCCryptorStatus status;
    NTSTATUS ret;

    if ((ret = init_cryptor_handles( key ))) return ret;

    if ((status = CCCryptorUpdate( key_data(key)->ref_decrypt, input, input_len, output, output_len, NULL  )) != kCCSuccess)
    {
        WARN( "CCCryptorUpdate failed %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS CDECL key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_symmetric_destroy( struct key *key )
{
    if (key_data(key)->ref_encrypt) CCCryptorRelease( key_data(key)->ref_encrypt );
    if (key_data(key)->ref_decrypt) CCCryptorRelease( key_data(key)->ref_decrypt );
}

static NTSTATUS CDECL key_asymmetric_init( struct key *key )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_sign( struct key *key, void *padding, UCHAR *input, ULONG input_len, UCHAR *output,
                                           ULONG output_len, ULONG *ret_len, ULONG flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                             UCHAR *signature, ULONG signature_len, DWORD flags )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_dsa_capi( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_ecc( struct key *key, UCHAR *output, ULONG len, ULONG *ret_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_dsa_capi( struct key *key, UCHAR *buf, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_ecc( struct key *key, UCHAR *input, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_rsa( struct key *key, UCHAR *input, ULONG len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_generate( struct key *key )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_asymmetric_destroy( struct key *key )
{
}

static NTSTATUS CDECL key_asymmetric_duplicate( struct key *key_orig, struct key *key_copy )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_decrypt( struct key *key, UCHAR *input, ULONG input_len,
        UCHAR *output, ULONG *output_len )
{
    FIXME( "not implemented on Mac\n" );
    return STATUS_NOT_IMPLEMENTED;
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
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    *(const struct key_funcs **)ptr_out = &key_funcs;
    return STATUS_SUCCESS;
}

#endif
