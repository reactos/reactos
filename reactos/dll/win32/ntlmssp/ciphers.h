/*
 * Copyright 2011 Samuel Serapión
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
#ifndef _CYPHERS_H_
#define _CYPHERS_H_

#include <string.h>
#include "windef.h"

typedef struct _rc4_key
{
    unsigned char perm[256];
    unsigned char index1;
    unsigned char index2;
}rc4_key;

void rc4_init(rc4_key *const state, const unsigned char *key, int keylen);
void rc4_crypt(rc4_key *const state, const unsigned char *inbuf, unsigned char *outbuf, int buflen);

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
}MD4_CTX;

/* advapi32 */
void WINAPI MD4Init(MD4_CTX *ctx);
void WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len);
void WINAPI MD4Final(MD4_CTX *ctx);

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
}MD5_CTX;

/* advapi32 */
void WINAPI MD5Init(MD5_CTX *ctx);
void WINAPI MD5Update(MD5_CTX *ctx, const unsigned char *buf, unsigned int len);
void WINAPI MD5Final(MD5_CTX *ctx);

typedef struct
{
    MD5_CTX ctx;
    unsigned char outer_padding[64];
} HMAC_MD5_CTX;

void HMACMD5Init(HMAC_MD5_CTX *ctx, const unsigned char *key, unsigned int key_len);
void HMACMD5Update(HMAC_MD5_CTX *ctx, const unsigned char *data, unsigned int data_len);
void HMACMD5Final(HMAC_MD5_CTX *ctx, unsigned char *digest);

/* high level api */
UINT32
CRC32 (const char *msg, int len);

void
RC4K (const unsigned char * k, unsigned long key_len, const unsigned char * d, int len, unsigned char * result);

void
DES(const unsigned char *k, const unsigned char *d, unsigned char * results);

void
DESL(unsigned char *k, const unsigned char *d, unsigned char * results);

void
MD4(const unsigned char * d, int len, unsigned char * result);

void
MD5(const unsigned char * d, int len, unsigned char * result);

void
HMAC_MD5(const unsigned char *key, int key_len, const unsigned char *data, int data_len, unsigned char *result);
#endif
