/*
 * dlls/rsaenh/implglue.h
 * Glueing the RSAENH specific code to the crypto library
 *
 * Copyright (c) 2004 Michael Jung
 *
 * based on code by Mike McCormack and David Hammerton
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
 */

#ifndef __WINE_IMPLGLUE_H
#define __WINE_IMPLGLUE_H

#include "symcrypt.h"
#include "symcrypt_low_level.h"

#define RSAENH_MAX_HASH_SIZE        104

struct rsa_key
{
    SYMCRYPT_RSAKEY *key;
    UINT32           flags;
};

typedef union tagKEY_CONTEXT
{
    SYMCRYPT_DES_EXPANDED_KEY  des;
    SYMCRYPT_3DES_EXPANDED_KEY des3;
    SYMCRYPT_RC2_EXPANDED_KEY  rc2;
    SYMCRYPT_RC4_STATE         rc4;
    SYMCRYPT_AES_EXPANDED_KEY  aes;
    struct rsa_key             rsa;
} KEY_CONTEXT;

struct hash
{
    const SYMCRYPT_HASH *desc;
    SYMCRYPT_HASH_STATE  state;
};

BOOL init_hash_impl(ALG_ID algid, struct hash *hash);

static inline void update_hash_impl(struct hash *hash, const BYTE *data, DWORD len)
{
    SymCryptHashAppend( hash->desc, &hash->state, data, len );
}

static inline void finalize_hash_impl(struct hash *hash, BYTE *hash_value, DWORD hash_size)
{
    SymCryptHashResult( hash->desc, &hash->state, hash_value, hash_size );
}

BOOL new_key_impl(ALG_ID algid, KEY_CONTEXT *ctx, DWORD keylen);
BOOL free_key_impl(ALG_ID algid, KEY_CONTEXT *ctx);
BOOL setup_key_impl(ALG_ID algid, KEY_CONTEXT *ctx, DWORD keylen, DWORD effective_keylen, DWORD saltlen,
                    const BYTE *keyvalue);
BOOL duplicate_key_impl(ALG_ID algid, const KEY_CONTEXT *src, KEY_CONTEXT *dst);

BOOL encrypt_block_impl(ALG_ID algid, KEY_CONTEXT *ctx, const BYTE *in, BYTE *out);
BOOL decrypt_block_impl(ALG_ID algid, KEY_CONTEXT *ctx, const BYTE *in, BYTE *out);
BOOL encrypt_stream_impl(ALG_ID algid, KEY_CONTEXT *ctx, BYTE *inout, DWORD len);

BOOL sign_hash_impl(KEY_CONTEXT *ctx, const BYTE *in, BYTE *out);
BOOL verify_signature_impl(KEY_CONTEXT *ctx, const BYTE *in, BYTE *out);

BOOL export_public_key_impl(const KEY_CONTEXT *ctx, BYTE *dst, DWORD *pubexp);
BOOL import_public_key_impl(ALG_ID algid, const BYTE *src, DWORD keylen, DWORD pubexp, KEY_CONTEXT *ctx);
BOOL export_private_key_impl(const KEY_CONTEXT *ctx, BYTE *dst, DWORD *pubexp);
BOOL import_private_key_impl(ALG_ID algid, const BYTE *src, DWORD keylen, DWORD pubexp, KEY_CONTEXT *ctx);

#endif /* __WINE_IMPLGLUE_H */
