/*
 * Copyright (C) 2001 Nikos Mavroyanopoulos
 * Copyright (C) 2004 Hans Leidekker
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This code implements the MD4 message-digest algorithm.
 * It is based on code in the public domain written by Colin
 * Plumb in 1993. The algorithm is due to Ron Rivest.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD4_CTX structure, pass it to MD4Init, call MD4Update as
 * needed on buffers full of bytes, and then call MD4Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#include <advapi32.h>
#include "crypt.h"


typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;


/* The three core functions */

#define rotl32(x,n)  (((x) << ((unsigned int)(n))) | ((x) >> (32 - (unsigned int)(n))))

#define F( x, y, z ) (((x) & (y)) | ((~x) & (z)))
#define G( x, y, z ) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H( x, y, z ) ((x) ^ (y) ^ (z))

#define FF( a, b, c, d, x, s ) { \
    (a) += F( (b), (c), (d) ) + (x); \
    (a) = rotl32( (a), (s) ); \
  }
#define GG( a, b, c, d, x, s ) { \
    (a) += G( (b), (c), (d) ) + (x) + (unsigned int)0x5a827999; \
    (a) = rotl32( (a), (s) ); \
  }
#define HH( a, b, c, d, x, s ) { \
    (a) += H( (b), (c), (d) ) + (x) + (unsigned int)0x6ed9eba1; \
    (a) = rotl32( (a), (s) ); \
  }

/*
 * The core of the MD4 algorithm
 */
static VOID MD4Transform(unsigned int buf[4], const unsigned int in[16])
{
    register unsigned int a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    FF( a, b, c, d, in[0], 3 );
    FF( d, a, b, c, in[1], 7 );
    FF( c, d, a, b, in[2], 11 );
    FF( b, c, d, a, in[3], 19 );
    FF( a, b, c, d, in[4], 3 );
    FF( d, a, b, c, in[5], 7 );
    FF( c, d, a, b, in[6], 11 );
    FF( b, c, d, a, in[7], 19 );
    FF( a, b, c, d, in[8], 3 );
    FF( d, a, b, c, in[9], 7 );
    FF( c, d, a, b, in[10], 11 );
    FF( b, c, d, a, in[11], 19 );
    FF( a, b, c, d, in[12], 3 );
    FF( d, a, b, c, in[13], 7 );
    FF( c, d, a, b, in[14], 11 );
    FF( b, c, d, a, in[15], 19 );

    GG( a, b, c, d, in[0], 3 );
    GG( d, a, b, c, in[4], 5 );
    GG( c, d, a, b, in[8], 9 );
    GG( b, c, d, a, in[12], 13 );
    GG( a, b, c, d, in[1], 3 );
    GG( d, a, b, c, in[5], 5 );
    GG( c, d, a, b, in[9], 9 );
    GG( b, c, d, a, in[13], 13 );
    GG( a, b, c, d, in[2], 3 );
    GG( d, a, b, c, in[6], 5 );
    GG( c, d, a, b, in[10], 9 );
    GG( b, c, d, a, in[14], 13 );
    GG( a, b, c, d, in[3], 3 );
    GG( d, a, b, c, in[7], 5 );
    GG( c, d, a, b, in[11], 9 );
    GG( b, c, d, a, in[15], 13 );

    HH( a, b, c, d, in[0], 3 );
    HH( d, a, b, c, in[8], 9 );
    HH( c, d, a, b, in[4], 11 );
    HH( b, c, d, a, in[12], 15 );
    HH( a, b, c, d, in[2], 3 );
    HH( d, a, b, c, in[10], 9 );
    HH( c, d, a, b, in[6], 11 );
    HH( b, c, d, a, in[14], 15 );
    HH( a, b, c, d, in[1], 3 );
    HH( d, a, b, c, in[9], 9 );
    HH( c, d, a, b, in[5], 11 );
    HH( b, c, d, a, in[13], 15 );
    HH( a, b, c, d, in[3], 3 );
    HH( d, a, b, c, in[11], 9 );
    HH( c, d, a, b, in[7], 11 );
    HH( b, c, d, a, in[15], 15 );

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/*
 * Start MD4 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
VOID WINAPI MD4Init(MD4_CTX *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->i[0] = ctx->i[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
VOID WINAPI MD4Update(MD4_CTX *ctx, const unsigned char *buf, unsigned int len)
{
    register unsigned int t;

    /* Update bitcount */
    t = ctx->i[0];

    if ((ctx->i[0] = t + ((unsigned int)len << 3)) < t)
        ctx->i[1]++;        /* Carry from low to high */

    ctx->i[1] += len >> 29;
    t = (t >> 3) & 0x3f;

    /* Handle any leading odd-sized chunks */
    if (t)
    {
        unsigned char *p = (unsigned char *)ctx->in + t;
        t = 64 - t;

        if (len < t)
        {
            memcpy(p, buf, len);
            return;
        }

        memcpy(p, buf, t);
        byteReverse(ctx->in, 16);

        MD4Transform(ctx->buf, (unsigned int *)ctx->in);

        buf += t;
        len -= t;
    }

    /* Process data in 64-byte chunks */
    while (len >= 64)
    {
        memcpy(ctx->in, buf, 64);
        byteReverse(ctx->in, 16);

        MD4Transform(ctx->buf, (unsigned int *)ctx->in);

        buf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */
    memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
VOID WINAPI MD4Final(MD4_CTX *ctx)
{
    unsigned int count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->i[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8)
    {
        /* Two lots of padding:  Pad the first block to 64 bytes */
        memset( p, 0, count );
        byteReverse(ctx->in, 16);
        MD4Transform(ctx->buf, (unsigned int *)ctx->in);

        /* Now fill the next block with 56 bytes */
        memset(ctx->in, 0, 56);
    }
    else
    {
        /* Pad block to 56 bytes */
        memset(p, 0, count - 8);
    }

    byteReverse(ctx->in, 14);

    /* Append length in bits and transform */
    ((unsigned int *)ctx->in)[14] = ctx->i[0];
    ((unsigned int *)ctx->in)[15] = ctx->i[1];

    MD4Transform( ctx->buf, (unsigned int *)ctx->in );
    byteReverse( (unsigned char *)ctx->buf, 4 );
    memcpy(ctx->digest, ctx->buf, 16);
}
