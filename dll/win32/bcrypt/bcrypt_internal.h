/*
 * Copyright 2016 Michael Müller
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

#ifndef __BCRYPT_INTERNAL_H
#define __BCRYPT_INTERNAL_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

typedef struct
{
    ULONG64 len;
    DWORD h[8];
    UCHAR buf[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx) DECLSPEC_HIDDEN;
void sha256_update(SHA256_CTX *ctx, const UCHAR *buffer, ULONG len) DECLSPEC_HIDDEN;
void sha256_finalize(SHA256_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

typedef struct
{
  ULONG64 len;
  ULONG64 h[8];
  UCHAR buf[128];
} SHA512_CTX;

void sha512_init(SHA512_CTX *ctx) DECLSPEC_HIDDEN;
void sha512_update(SHA512_CTX *ctx, const UCHAR *buffer, ULONG len) DECLSPEC_HIDDEN;
void sha512_finalize(SHA512_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

void sha384_init(SHA512_CTX *ctx) DECLSPEC_HIDDEN;
#define sha384_update sha512_update
void sha384_finalize(SHA512_CTX *ctx, UCHAR *buffer) DECLSPEC_HIDDEN;

typedef struct {
    unsigned char chksum[16], X[48], buf[16];
    unsigned long curlen;
} MD2_CTX;

void md2_init(MD2_CTX *ctx) DECLSPEC_HIDDEN;
void md2_update(MD2_CTX *ctx, const unsigned char *buf, ULONG len) DECLSPEC_HIDDEN;
void md2_finalize(MD2_CTX *ctx, unsigned char *hash) DECLSPEC_HIDDEN;

/* Definitions from advapi32 */
typedef struct tagMD4_CTX {
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

VOID WINAPI MD4Init(MD4_CTX *ctx);
VOID WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len);
VOID WINAPI MD4Final(MD4_CTX *ctx);

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

VOID WINAPI MD5Init(MD5_CTX *ctx);
VOID WINAPI MD5Update(MD5_CTX *ctx, const unsigned char *buf, unsigned int len);
VOID WINAPI MD5Final(MD5_CTX *ctx);

typedef struct
{
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX;

VOID WINAPI A_SHAInit(SHA_CTX *ctx);
VOID WINAPI A_SHAUpdate(SHA_CTX *ctx, const UCHAR *buffer, UINT size);
VOID WINAPI A_SHAFinal(SHA_CTX *ctx, PULONG result);

#endif /* __BCRYPT_INTERNAL_H */
