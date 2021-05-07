/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2018 Hans Leidekker for CodeWeavers
 * Copyright 2021 Micah Andersen & ReactOS Team
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


#include "config.h"
#include "wine/port.h"

#ifndef SONAME_LIBMBEDTLS
#define SONAME_LIBMBEDTLS
#endif

#ifdef SONAME_LIBMBEDTLS

#include <stdarg.h>
#include <assert.h>

#include <mbedtls/md.h>
#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/ecdsa.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntsecapi.h"
#include "wincrypt.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#ifdef __GNUC__         //runtime loaded version (wine-preferred) only works with GCC

void *libmbedtls_handle;

#define MAKE_FUNCPTR(f) static typeof(f) *p##f
MAKE_FUNCPTR(mbedtls_md_init);
//imports go here
#undef MAKE_FUNCPTR

#define mbedtls_md_init             pmbedtls_md_init
//imports go here

static BOOL mbedtls_initialize(void)
{
    if (!(libmbedtls_handle = wine_dlopen( SONAME_LIBMBEDTLS, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "failed to load libmbedtls, no support for crypto hashes\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libmbedtls_handle, #f, NULL, 0 ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(mbedtls_md_init)
    //imports go here
#undef LOAD_FUNCPTR

    return TRUE;

fail:
    wine_dlclose( libmbedtls_handle, NULL, 0 );
    libmbedtls_handle = NULL;
    return FALSE;
}

static void mbedtls_uninitialize(void)
{
    wine_dlclose( libmbedtls_handle, NULL, 0 );
    libmbedtls_handle = NULL;
}
#endif /* GNUC */


static NTSTATUS CDECL key_set_property( struct key *key, const WCHAR *prop, UCHAR *value, ULONG size, ULONG flags )
{
    FIXME("key_set_property stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_init( struct key *key )
{
    FIXME("key_symmetric_init stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_symmetric_vector_reset( struct key *key )
{
    FIXME("key_symmetric_vector_reset stub\n");
    return; // STATUS_NOT_IMPLEMENTED
}

static NTSTATUS CDECL key_symmetric_set_auth_data( struct key *key, UCHAR *auth_data, ULONG len )
{
    FIXME("key_symmetric_set_auth_data stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_encrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len )
{
    FIXME("key_symmetric_encrypt stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_decrypt( struct key *key, const UCHAR *input, ULONG input_len, UCHAR *output, ULONG output_len  )
{
    FIXME("key_symmetric_decrypt stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_symmetric_get_tag( struct key *key, UCHAR *tag, ULONG len )
{
    FIXME("key_symmetric_get_tag stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_symmetric_destroy( struct key *key )
{
    FIXME("key_symmetric_destroy stub\n");
    return; // STATUS_NOT_IMPLEMENTED
}

static NTSTATUS CDECL key_asymmetric_generate( struct key *key )
{
    FIXME("key_asymmetric_generate stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_ecc( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    FIXME("key_export_ecc stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_ecc( struct key *key, UCHAR *buf, ULONG len )
{
    FIXME("key_import_ecc stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_rsa( struct key *key, UCHAR *buf, ULONG len )
{
    FIXME("key_import_rsa stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_export_dsa_capi( struct key *key, UCHAR *buf, ULONG len, ULONG *ret_len )
{
    FIXME("key_export_dsa_capi stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_import_dsa_capi( struct key *key, UCHAR *buf, ULONG len )
{
    FIXME("key_import_dsa_capi stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_init( struct key *key )
{
    FIXME("key_asymmetric_init stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_verify( struct key *key, void *padding, UCHAR *hash, ULONG hash_len,
                                             UCHAR *signature, ULONG signature_len, DWORD flags )
{
    FIXME("key_asymmetric_verify stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_sign( struct key *key, void *padding, UCHAR *input, ULONG input_len, UCHAR *output,
                                           ULONG output_len, ULONG *ret_len, ULONG flags )
{
    FIXME("key_asymmetric_sign stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static void CDECL key_asymmetric_destroy( struct key *key )
{
    FIXME("key_asymmetric_destroy stub\n");
    return; // STATUS_NOT_IMPLEMENTED
}

static NTSTATUS CDECL key_asymmetric_duplicate( struct key *key_orig, struct key *key_copy )
{
    FIXME("key_asymmetric_duplicate stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS CDECL key_asymmetric_decrypt( struct key *key, UCHAR *input, ULONG input_len, UCHAR *output,
                                              ULONG output_len, ULONG *ret_len )
{
    FIXME("key_asymmetric_decrypt stub\n");
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
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        #ifdef __GNUC__
        if (!mbedtls_initialize()) return STATUS_DLL_NOT_FOUND;
        #endif
        *(const struct key_funcs **)ptr_out = &key_funcs;
        break;
    case DLL_PROCESS_DETACH:
        #ifdef __GNUC__
        if (libmbedtls_handle) mbedtls_uninitialize();
        #endif
        break;
    }
    return STATUS_SUCCESS;
}

#endif /* SONAME_LIBMBEDTLS */
