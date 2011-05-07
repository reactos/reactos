/*
 * Copyright 2011
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

#ifndef _HMAC_MD5_H_
#define _HMAC_MD5_H_

#include <string.h>
#include "windef.h"

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

typedef struct
{
    MD5_CTX ctx;
    unsigned char outer_padding[64];
} HMAC_MD5_CTX;

VOID WINAPI MD4Init( MD4_CTX *ctx );
VOID WINAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD4Final( MD4_CTX *ctx );
void WINAPI MD5Init( MD5_CTX *ctx );
void WINAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len );
void WINAPI MD5Final( MD5_CTX *ctx );

void HMACMD5Init(HMAC_MD5_CTX *ctx, const unsigned char *key, unsigned int key_len);
void HMACMD5Update(HMAC_MD5_CTX *ctx, const unsigned char *data, unsigned int data_len);
void HMACMD5Final(HMAC_MD5_CTX *ctx, unsigned char *digest);
#endif /*_HMAC_MD5_H_*/
