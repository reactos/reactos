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

/* Based on public domain implementation from
   https://git.musl-libc.org/cgit/musl/tree/src/crypt/crypt_sha256.c */

#include "bcrypt_internal.h"

static DWORD ror(DWORD n, int k) { return (n >> k) | (n << (32-k)); }
#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) ((x & y) | (z & (x | y)))
#define S0(x)      (ror(x,2) ^ ror(x,13) ^ ror(x,22))
#define S1(x)      (ror(x,6) ^ ror(x,11) ^ ror(x,25))
#define R0(x)      (ror(x,7) ^ ror(x,18) ^ (x>>3))
#define R1(x)      (ror(x,17) ^ ror(x,19) ^ (x>>10))

static const DWORD K[64] =
{
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void processblock(SHA256_CTX *ctx, const UCHAR *buffer)
{
    DWORD W[64], t1, t2, a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++)
    {
        W[i]  = (DWORD)buffer[4*i]<<24;
        W[i] |= (DWORD)buffer[4*i+1]<<16;
        W[i] |= (DWORD)buffer[4*i+2]<<8;
        W[i] |= buffer[4*i+3];
    }

    for (; i < 64; i++)
        W[i] = R1(W[i-2]) + W[i-7] + R0(W[i-15]) + W[i-16];

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];

    for (i = 0; i < 64; i++)
    {
        t1 = h + S1(e) + Ch(e,f,g) + K[i] + W[i];
        t2 = S0(a) + Maj(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
    ctx->h[5] += f;
    ctx->h[6] += g;
    ctx->h[7] += h;
}

static void pad(SHA256_CTX *ctx)
{
    ULONG64 r = ctx->len % 64;

    ctx->buf[r++] = 0x80;

    if (r > 56)
    {
        memset(ctx->buf + r, 0, 64 - r);
        r = 0;
        processblock(ctx, ctx->buf);
    }

    memset(ctx->buf + r, 0, 56 - r);
    ctx->len *= 8;
    ctx->buf[56] = ctx->len >> 56;
    ctx->buf[57] = ctx->len >> 48;
    ctx->buf[58] = ctx->len >> 40;
    ctx->buf[59] = ctx->len >> 32;
    ctx->buf[60] = ctx->len >> 24;
    ctx->buf[61] = ctx->len >> 16;
    ctx->buf[62] = ctx->len >> 8;
    ctx->buf[63] = ctx->len;

    processblock(ctx, ctx->buf);
}

void sha256_init(SHA256_CTX *ctx)
{
    ctx->len = 0;
    ctx->h[0] = 0x6a09e667;
    ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372;
    ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f;
    ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab;
    ctx->h[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const UCHAR *buffer, ULONG len)
{
    const UCHAR *p = buffer;
    ULONG64 r = ctx->len % 64;

    ctx->len += len;
    if (r)
    {
        if (len < 64 - r)
        {
            memcpy(ctx->buf + r, p, len);
            return;
        }
        memcpy(ctx->buf + r, p, 64 - r);
        len -= 64 - r;
        p += 64 - r;
        processblock(ctx, ctx->buf);
    }
    for (; len >= 64; len -= 64, p += 64)
        processblock(ctx, p);
    memcpy(ctx->buf, p, len);
}

void sha256_finalize(SHA256_CTX *ctx, UCHAR *buffer)
{
    int i;

    pad(ctx);
    for (i = 0; i < 8; i++)
    {
        buffer[4*i]   = ctx->h[i] >> 24;
        buffer[4*i+1] = ctx->h[i] >> 16;
        buffer[4*i+2] = ctx->h[i] >> 8;
        buffer[4*i+3] = ctx->h[i];
    }
}
