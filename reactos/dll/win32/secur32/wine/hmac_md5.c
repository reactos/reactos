/*
 * Copyright 2006 Kai Blin
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
 * This file implements RFC 2104 (HMAC) for the MD5 provider.
 * It is needed for NTLM2 signing and sealing.
 */

#include "hmac_md5.h"

void HMACMD5Init(HMAC_MD5_CTX *ctx, const unsigned char *key, unsigned int key_len)
{
    int i;
    unsigned char inner_padding[64];
    unsigned char temp_key[16];

    if(key_len > 64)
    {
        MD5_CTX temp_ctx;

        MD5Init(&temp_ctx);
        MD5Update(&temp_ctx, key, key_len);
        MD5Final(&temp_ctx);
        memcpy(temp_key, temp_ctx.digest, 16);

        key = temp_key;
        key_len = 16;
    }

    memset(inner_padding, 0, 64);
    memset(ctx->outer_padding, 0, 64);
    memcpy(inner_padding, key, key_len);
    memcpy(ctx->outer_padding, key, key_len);

    for(i = 0; i < 64; ++i)
    {
        inner_padding[i] ^= 0x36;
        ctx->outer_padding[i] ^= 0x5c;
    }

    MD5Init(&(ctx->ctx));
    MD5Update(&(ctx->ctx), inner_padding, 64);
}

void HMACMD5Update(HMAC_MD5_CTX *ctx, const unsigned char *data, unsigned int data_len)
{
    MD5Update(&(ctx->ctx), data, data_len);
}

void HMACMD5Final(HMAC_MD5_CTX *ctx, unsigned char *digest)
{
    MD5_CTX outer_ctx;
    unsigned char inner_digest[16];

    MD5Final(&(ctx->ctx));
    memcpy(inner_digest, ctx->ctx.digest, 16);

    MD5Init(&outer_ctx);
    MD5Update(&outer_ctx, ctx->outer_padding, 64);
    MD5Update(&outer_ctx, inner_digest, 16);
    MD5Final(&outer_ctx);

    memcpy(digest, outer_ctx.digest, 16);
}
