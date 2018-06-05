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
   https://git.musl-libc.org/cgit/musl/tree/src/crypt/crypt_sha512.c */

#include "bcrypt_internal.h"

static ULONG64 ror(ULONG64 n, int k) { return (n >> k) | (n << (64-k)); }
#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) ((x & y) | (z & (x | y)))
#define S0(x)      (ror(x,28) ^ ror(x,34) ^ ror(x,39))
#define S1(x)      (ror(x,14) ^ ror(x,18) ^ ror(x,41))
#define R0(x)      (ror(x,1) ^ ror(x,8) ^ (x>>7))
#define R1(x)      (ror(x,19) ^ ror(x,61) ^ (x>>6))
#define ULL(a,b)   (((ULONG64)(a) << 32) | (b))

static const ULONG64 K[80] =
{
    ULL(0x428a2f98,0xd728ae22), ULL(0x71374491,0x23ef65cd), ULL(0xb5c0fbcf,0xec4d3b2f), ULL(0xe9b5dba5,0x8189dbbc),
    ULL(0x3956c25b,0xf348b538), ULL(0x59f111f1,0xb605d019), ULL(0x923f82a4,0xaf194f9b), ULL(0xab1c5ed5,0xda6d8118),
    ULL(0xd807aa98,0xa3030242), ULL(0x12835b01,0x45706fbe), ULL(0x243185be,0x4ee4b28c), ULL(0x550c7dc3,0xd5ffb4e2),
    ULL(0x72be5d74,0xf27b896f), ULL(0x80deb1fe,0x3b1696b1), ULL(0x9bdc06a7,0x25c71235), ULL(0xc19bf174,0xcf692694),
    ULL(0xe49b69c1,0x9ef14ad2), ULL(0xefbe4786,0x384f25e3), ULL(0x0fc19dc6,0x8b8cd5b5), ULL(0x240ca1cc,0x77ac9c65),
    ULL(0x2de92c6f,0x592b0275), ULL(0x4a7484aa,0x6ea6e483), ULL(0x5cb0a9dc,0xbd41fbd4), ULL(0x76f988da,0x831153b5),
    ULL(0x983e5152,0xee66dfab), ULL(0xa831c66d,0x2db43210), ULL(0xb00327c8,0x98fb213f), ULL(0xbf597fc7,0xbeef0ee4),
    ULL(0xc6e00bf3,0x3da88fc2), ULL(0xd5a79147,0x930aa725), ULL(0x06ca6351,0xe003826f), ULL(0x14292967,0x0a0e6e70),
    ULL(0x27b70a85,0x46d22ffc), ULL(0x2e1b2138,0x5c26c926), ULL(0x4d2c6dfc,0x5ac42aed), ULL(0x53380d13,0x9d95b3df),
    ULL(0x650a7354,0x8baf63de), ULL(0x766a0abb,0x3c77b2a8), ULL(0x81c2c92e,0x47edaee6), ULL(0x92722c85,0x1482353b),
    ULL(0xa2bfe8a1,0x4cf10364), ULL(0xa81a664b,0xbc423001), ULL(0xc24b8b70,0xd0f89791), ULL(0xc76c51a3,0x0654be30),
    ULL(0xd192e819,0xd6ef5218), ULL(0xd6990624,0x5565a910), ULL(0xf40e3585,0x5771202a), ULL(0x106aa070,0x32bbd1b8),
    ULL(0x19a4c116,0xb8d2d0c8), ULL(0x1e376c08,0x5141ab53), ULL(0x2748774c,0xdf8eeb99), ULL(0x34b0bcb5,0xe19b48a8),
    ULL(0x391c0cb3,0xc5c95a63), ULL(0x4ed8aa4a,0xe3418acb), ULL(0x5b9cca4f,0x7763e373), ULL(0x682e6ff3,0xd6b2b8a3),
    ULL(0x748f82ee,0x5defb2fc), ULL(0x78a5636f,0x43172f60), ULL(0x84c87814,0xa1f0ab72), ULL(0x8cc70208,0x1a6439ec),
    ULL(0x90befffa,0x23631e28), ULL(0xa4506ceb,0xde82bde9), ULL(0xbef9a3f7,0xb2c67915), ULL(0xc67178f2,0xe372532b),
    ULL(0xca273ece,0xea26619c), ULL(0xd186b8c7,0x21c0c207), ULL(0xeada7dd6,0xcde0eb1e), ULL(0xf57d4f7f,0xee6ed178),
    ULL(0x06f067aa,0x72176fba), ULL(0x0a637dc5,0xa2c898a6), ULL(0x113f9804,0xbef90dae), ULL(0x1b710b35,0x131c471b),
    ULL(0x28db77f5,0x23047d84), ULL(0x32caab7b,0x40c72493), ULL(0x3c9ebe0a,0x15c9bebc), ULL(0x431d67c4,0x9c100d4c),
    ULL(0x4cc5d4be,0xcb3e42b6), ULL(0x597f299c,0xfc657e2a), ULL(0x5fcb6fab,0x3ad6faec), ULL(0x6c44198c,0x4a475817)
};

static void processblock(SHA512_CTX *ctx, const UCHAR *buffer)
{
    ULONG64 W[80], t1, t2, a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++)
    {
        W[i]  = (ULONG64)buffer[8*i]<<56;
        W[i] |= (ULONG64)buffer[8*i+1]<<48;
        W[i] |= (ULONG64)buffer[8*i+2]<<40;
        W[i] |= (ULONG64)buffer[8*i+3]<<32;
        W[i] |= (ULONG64)buffer[8*i+4]<<24;
        W[i] |= (ULONG64)buffer[8*i+5]<<16;
        W[i] |= (ULONG64)buffer[8*i+6]<<8;
        W[i] |= buffer[8*i+7];
    }

    for (; i < 80; i++)
        W[i] = R1(W[i-2]) + W[i-7] + R0(W[i-15]) + W[i-16];

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];
    f = ctx->h[5];
    g = ctx->h[6];
    h = ctx->h[7];

    for (i = 0; i < 80; i++)
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

static void pad(SHA512_CTX *ctx)
{
    ULONG64 r = ctx->len % 128;

    ctx->buf[r++] = 0x80;
    if (r > 112)
    {
        memset(ctx->buf + r, 0, 128 - r);
        r = 0;
        processblock(ctx, ctx->buf);
    }

    memset(ctx->buf + r, 0, 120 - r);
    ctx->len *= 8;
    ctx->buf[120] = ctx->len >> 56;
    ctx->buf[121] = ctx->len >> 48;
    ctx->buf[122] = ctx->len >> 40;
    ctx->buf[123] = ctx->len >> 32;
    ctx->buf[124] = ctx->len >> 24;
    ctx->buf[125] = ctx->len >> 16;
    ctx->buf[126] = ctx->len >> 8;
    ctx->buf[127] = ctx->len;

    processblock(ctx, ctx->buf);
}

void sha512_init(SHA512_CTX *ctx)
{
    ctx->len = 0;
    ctx->h[0] = ULL(0x6a09e667,0xf3bcc908);
    ctx->h[1] = ULL(0xbb67ae85,0x84caa73b);
    ctx->h[2] = ULL(0x3c6ef372,0xfe94f82b);
    ctx->h[3] = ULL(0xa54ff53a,0x5f1d36f1);
    ctx->h[4] = ULL(0x510e527f,0xade682d1);
    ctx->h[5] = ULL(0x9b05688c,0x2b3e6c1f);
    ctx->h[6] = ULL(0x1f83d9ab,0xfb41bd6b);
    ctx->h[7] = ULL(0x5be0cd19,0x137e2179);
}

void sha512_update(SHA512_CTX *ctx, const UCHAR *buffer, ULONG len)
{
    const UCHAR *p = buffer;
    unsigned r = ctx->len % 128;

    ctx->len += len;
    if (r)
    {
        if (len < 128 - r)
        {
            memcpy(ctx->buf + r, p, len);
            return;
        }
        memcpy(ctx->buf + r, p, 128 - r);
        len -= 128 - r;
        p += 128 - r;
        processblock(ctx, ctx->buf);
    }

    for (; len >= 128; len -= 128, p += 128)
        processblock(ctx, p);

    memcpy(ctx->buf, p, len);
}

void sha512_finalize(SHA512_CTX *ctx, UCHAR *buffer)
{
    int i;

    pad(ctx);
    for (i = 0; i < 8; i++)
    {
        buffer[8*i] = ctx->h[i] >> 56;
        buffer[8*i+1] = ctx->h[i] >> 48;
        buffer[8*i+2] = ctx->h[i] >> 40;
        buffer[8*i+3] = ctx->h[i] >> 32;
        buffer[8*i+4] = ctx->h[i] >> 24;
        buffer[8*i+5] = ctx->h[i] >> 16;
        buffer[8*i+6] = ctx->h[i] >> 8;
        buffer[8*i+7] = ctx->h[i];
    }
}

void sha384_init(SHA512_CTX *ctx)
{
    ctx->len = 0;
    ctx->h[0] = ULL(0xcbbb9d5d,0xc1059ed8);
    ctx->h[1] = ULL(0x629a292a,0x367cd507);
    ctx->h[2] = ULL(0x9159015a,0x3070dd17);
    ctx->h[3] = ULL(0x152fecd8,0xf70e5939);
    ctx->h[4] = ULL(0x67332667,0xffc00b31);
    ctx->h[5] = ULL(0x8eb44a87,0x68581511);
    ctx->h[6] = ULL(0xdb0c2e0d,0x64f98fa7);
    ctx->h[7] = ULL(0x47b5481d,0xbefa4fa4);
}

void sha384_finalize(SHA512_CTX *ctx, UCHAR *buffer)
{
    UCHAR buffer512[64];

    sha512_finalize(ctx, buffer512);
    memcpy(buffer, buffer512, 48);
}
