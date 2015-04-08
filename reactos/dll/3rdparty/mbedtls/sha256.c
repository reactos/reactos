/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *  The SHA-256 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_SHA256_C)

#include "polarssl/sha256.h"

#if defined(POLARSSL_FS_IO) || defined(POLARSSL_SELF_TEST)
#include <stdio.h>
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_printf printf
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if !defined(POLARSSL_SHA256_ALT)

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

void sha256_init( sha256_context *ctx )
{
    memset( ctx, 0, sizeof( sha256_context ) );
}

void sha256_free( sha256_context *ctx )
{
    if( ctx == NULL )
        return;

    polarssl_zeroize( ctx, sizeof( sha256_context ) );
}

/*
 * SHA-256 context setup
 */
void sha256_starts( sha256_context *ctx, int is224 )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    if( is224 == 0 )
    {
        /* SHA-256 */
        ctx->state[0] = 0x6A09E667;
        ctx->state[1] = 0xBB67AE85;
        ctx->state[2] = 0x3C6EF372;
        ctx->state[3] = 0xA54FF53A;
        ctx->state[4] = 0x510E527F;
        ctx->state[5] = 0x9B05688C;
        ctx->state[6] = 0x1F83D9AB;
        ctx->state[7] = 0x5BE0CD19;
    }
    else
    {
        /* SHA-224 */
        ctx->state[0] = 0xC1059ED8;
        ctx->state[1] = 0x367CD507;
        ctx->state[2] = 0x3070DD17;
        ctx->state[3] = 0xF70E5939;
        ctx->state[4] = 0xFFC00B31;
        ctx->state[5] = 0x68581511;
        ctx->state[6] = 0x64F98FA7;
        ctx->state[7] = 0xBEFA4FA4;
    }

    ctx->is224 = is224;
}

void sha256_process( sha256_context *ctx, const unsigned char data[64] )
{
    uint32_t temp1, temp2, W[64];
    uint32_t A, B, C, D, E, F, G, H;

    GET_UINT32_BE( W[ 0], data,  0 );
    GET_UINT32_BE( W[ 1], data,  4 );
    GET_UINT32_BE( W[ 2], data,  8 );
    GET_UINT32_BE( W[ 3], data, 12 );
    GET_UINT32_BE( W[ 4], data, 16 );
    GET_UINT32_BE( W[ 5], data, 20 );
    GET_UINT32_BE( W[ 6], data, 24 );
    GET_UINT32_BE( W[ 7], data, 28 );
    GET_UINT32_BE( W[ 8], data, 32 );
    GET_UINT32_BE( W[ 9], data, 36 );
    GET_UINT32_BE( W[10], data, 40 );
    GET_UINT32_BE( W[11], data, 44 );
    GET_UINT32_BE( W[12], data, 48 );
    GET_UINT32_BE( W[13], data, 52 );
    GET_UINT32_BE( W[14], data, 56 );
    GET_UINT32_BE( W[15], data, 60 );

#define  SHR(x,n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define R(t)                                    \
(                                               \
    W[t] = S1(W[t -  2]) + W[t -  7] +          \
           S0(W[t - 15]) + W[t - 16]            \
)

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];

    P( A, B, C, D, E, F, G, H, W[ 0], 0x428A2F98 );
    P( H, A, B, C, D, E, F, G, W[ 1], 0x71374491 );
    P( G, H, A, B, C, D, E, F, W[ 2], 0xB5C0FBCF );
    P( F, G, H, A, B, C, D, E, W[ 3], 0xE9B5DBA5 );
    P( E, F, G, H, A, B, C, D, W[ 4], 0x3956C25B );
    P( D, E, F, G, H, A, B, C, W[ 5], 0x59F111F1 );
    P( C, D, E, F, G, H, A, B, W[ 6], 0x923F82A4 );
    P( B, C, D, E, F, G, H, A, W[ 7], 0xAB1C5ED5 );
    P( A, B, C, D, E, F, G, H, W[ 8], 0xD807AA98 );
    P( H, A, B, C, D, E, F, G, W[ 9], 0x12835B01 );
    P( G, H, A, B, C, D, E, F, W[10], 0x243185BE );
    P( F, G, H, A, B, C, D, E, W[11], 0x550C7DC3 );
    P( E, F, G, H, A, B, C, D, W[12], 0x72BE5D74 );
    P( D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE );
    P( C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7 );
    P( B, C, D, E, F, G, H, A, W[15], 0xC19BF174 );
    P( A, B, C, D, E, F, G, H, R(16), 0xE49B69C1 );
    P( H, A, B, C, D, E, F, G, R(17), 0xEFBE4786 );
    P( G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6 );
    P( F, G, H, A, B, C, D, E, R(19), 0x240CA1CC );
    P( E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F );
    P( D, E, F, G, H, A, B, C, R(21), 0x4A7484AA );
    P( C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC );
    P( B, C, D, E, F, G, H, A, R(23), 0x76F988DA );
    P( A, B, C, D, E, F, G, H, R(24), 0x983E5152 );
    P( H, A, B, C, D, E, F, G, R(25), 0xA831C66D );
    P( G, H, A, B, C, D, E, F, R(26), 0xB00327C8 );
    P( F, G, H, A, B, C, D, E, R(27), 0xBF597FC7 );
    P( E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3 );
    P( D, E, F, G, H, A, B, C, R(29), 0xD5A79147 );
    P( C, D, E, F, G, H, A, B, R(30), 0x06CA6351 );
    P( B, C, D, E, F, G, H, A, R(31), 0x14292967 );
    P( A, B, C, D, E, F, G, H, R(32), 0x27B70A85 );
    P( H, A, B, C, D, E, F, G, R(33), 0x2E1B2138 );
    P( G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC );
    P( F, G, H, A, B, C, D, E, R(35), 0x53380D13 );
    P( E, F, G, H, A, B, C, D, R(36), 0x650A7354 );
    P( D, E, F, G, H, A, B, C, R(37), 0x766A0ABB );
    P( C, D, E, F, G, H, A, B, R(38), 0x81C2C92E );
    P( B, C, D, E, F, G, H, A, R(39), 0x92722C85 );
    P( A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1 );
    P( H, A, B, C, D, E, F, G, R(41), 0xA81A664B );
    P( G, H, A, B, C, D, E, F, R(42), 0xC24B8B70 );
    P( F, G, H, A, B, C, D, E, R(43), 0xC76C51A3 );
    P( E, F, G, H, A, B, C, D, R(44), 0xD192E819 );
    P( D, E, F, G, H, A, B, C, R(45), 0xD6990624 );
    P( C, D, E, F, G, H, A, B, R(46), 0xF40E3585 );
    P( B, C, D, E, F, G, H, A, R(47), 0x106AA070 );
    P( A, B, C, D, E, F, G, H, R(48), 0x19A4C116 );
    P( H, A, B, C, D, E, F, G, R(49), 0x1E376C08 );
    P( G, H, A, B, C, D, E, F, R(50), 0x2748774C );
    P( F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5 );
    P( E, F, G, H, A, B, C, D, R(52), 0x391C0CB3 );
    P( D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A );
    P( C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F );
    P( B, C, D, E, F, G, H, A, R(55), 0x682E6FF3 );
    P( A, B, C, D, E, F, G, H, R(56), 0x748F82EE );
    P( H, A, B, C, D, E, F, G, R(57), 0x78A5636F );
    P( G, H, A, B, C, D, E, F, R(58), 0x84C87814 );
    P( F, G, H, A, B, C, D, E, R(59), 0x8CC70208 );
    P( E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA );
    P( D, E, F, G, H, A, B, C, R(61), 0xA4506CEB );
    P( C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7 );
    P( B, C, D, E, F, G, H, A, R(63), 0xC67178F2 );

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
    ctx->state[5] += F;
    ctx->state[6] += G;
    ctx->state[7] += H;
}

/*
 * SHA-256 process buffer
 */
void sha256_update( sha256_context *ctx, const unsigned char *input,
                    size_t ilen )
{
    size_t fill;
    uint32_t left;

    if( ilen == 0 )
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );
        sha256_process( ctx, ctx->buffer );
        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        sha256_process( ctx, input );
        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );
}

static const unsigned char sha256_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * SHA-256 final digest
 */
void sha256_finish( sha256_context *ctx, unsigned char output[32] )
{
    uint32_t last, padn;
    uint32_t high, low;
    unsigned char msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE( high, msglen, 0 );
    PUT_UINT32_BE( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sha256_update( ctx, sha256_padding, padn );
    sha256_update( ctx, msglen, 8 );

    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );
    PUT_UINT32_BE( ctx->state[5], output, 20 );
    PUT_UINT32_BE( ctx->state[6], output, 24 );

    if( ctx->is224 == 0 )
        PUT_UINT32_BE( ctx->state[7], output, 28 );
}

#endif /* !POLARSSL_SHA256_ALT */

/*
 * output = SHA-256( input buffer )
 */
void sha256( const unsigned char *input, size_t ilen,
             unsigned char output[32], int is224 )
{
    sha256_context ctx;

    sha256_init( &ctx );
    sha256_starts( &ctx, is224 );
    sha256_update( &ctx, input, ilen );
    sha256_finish( &ctx, output );
    sha256_free( &ctx );
}

#if defined(POLARSSL_FS_IO)
/*
 * output = SHA-256( file contents )
 */
int sha256_file( const char *path, unsigned char output[32], int is224 )
{
    FILE *f;
    size_t n;
    sha256_context ctx;
    unsigned char buf[1024];

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( POLARSSL_ERR_SHA256_FILE_IO_ERROR );

    sha256_init( &ctx );
    sha256_starts( &ctx, is224 );

    while( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        sha256_update( &ctx, buf, n );

    sha256_finish( &ctx, output );
    sha256_free( &ctx );

    if( ferror( f ) != 0 )
    {
        fclose( f );
        return( POLARSSL_ERR_SHA256_FILE_IO_ERROR );
    }

    fclose( f );
    return( 0 );
}
#endif /* POLARSSL_FS_IO */

/*
 * SHA-256 HMAC context setup
 */
void sha256_hmac_starts( sha256_context *ctx, const unsigned char *key,
                         size_t keylen, int is224 )
{
    size_t i;
    unsigned char sum[32];

    if( keylen > 64 )
    {
        sha256( key, keylen, sum, is224 );
        keylen = ( is224 ) ? 28 : 32;
        key = sum;
    }

    memset( ctx->ipad, 0x36, 64 );
    memset( ctx->opad, 0x5C, 64 );

    for( i = 0; i < keylen; i++ )
    {
        ctx->ipad[i] = (unsigned char)( ctx->ipad[i] ^ key[i] );
        ctx->opad[i] = (unsigned char)( ctx->opad[i] ^ key[i] );
    }

    sha256_starts( ctx, is224 );
    sha256_update( ctx, ctx->ipad, 64 );

    polarssl_zeroize( sum, sizeof( sum ) );
}

/*
 * SHA-256 HMAC process buffer
 */
void sha256_hmac_update( sha256_context *ctx, const unsigned char *input,
                         size_t ilen )
{
    sha256_update( ctx, input, ilen );
}

/*
 * SHA-256 HMAC final digest
 */
void sha256_hmac_finish( sha256_context *ctx, unsigned char output[32] )
{
    int is224, hlen;
    unsigned char tmpbuf[32];

    is224 = ctx->is224;
    hlen = ( is224 == 0 ) ? 32 : 28;

    sha256_finish( ctx, tmpbuf );
    sha256_starts( ctx, is224 );
    sha256_update( ctx, ctx->opad, 64 );
    sha256_update( ctx, tmpbuf, hlen );
    sha256_finish( ctx, output );

    polarssl_zeroize( tmpbuf, sizeof( tmpbuf ) );
}

/*
 * SHA-256 HMAC context reset
 */
void sha256_hmac_reset( sha256_context *ctx )
{
    sha256_starts( ctx, ctx->is224 );
    sha256_update( ctx, ctx->ipad, 64 );
}

/*
 * output = HMAC-SHA-256( hmac key, input buffer )
 */
void sha256_hmac( const unsigned char *key, size_t keylen,
                  const unsigned char *input, size_t ilen,
                  unsigned char output[32], int is224 )
{
    sha256_context ctx;

    sha256_init( &ctx );
    sha256_hmac_starts( &ctx, key, keylen, is224 );
    sha256_hmac_update( &ctx, input, ilen );
    sha256_hmac_finish( &ctx, output );
    sha256_free( &ctx );
}

#if defined(POLARSSL_SELF_TEST)
/*
 * FIPS-180-2 test vectors
 */
static unsigned char sha256_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const int sha256_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha256_test_sum[6][32] =
{
    /*
     * SHA-224 test vectors
     */
    { 0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
      0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
      0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
      0xE3, 0x6C, 0x9D, 0xA7 },
    { 0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC,
      0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89, 0x01, 0x50,
      0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19,
      0x52, 0x52, 0x25, 0x25 },
    { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0C, 0x91, 0xD8,
      0xBB, 0xB4, 0xC1, 0xEA, 0x97, 0x61, 0x8A, 0x4B,
      0xF0, 0x3F, 0x42, 0x58, 0x19, 0x48, 0xB2, 0xEE,
      0x4E, 0xE7, 0xAD, 0x67 },

    /*
     * SHA-256 test vectors
     */
    { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
      0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
      0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
      0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
    { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
      0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
      0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
      0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
    { 0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
      0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
      0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
      0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0 }
};

/*
 * RFC 4231 test vectors
 */
static unsigned char sha256_hmac_test_key[7][26] =
{
    { "\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B"
      "\x0B\x0B\x0B\x0B" },
    { "Jefe" },
    { "\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA"
      "\xAA\xAA\xAA\xAA" },
    { "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10"
      "\x11\x12\x13\x14\x15\x16\x17\x18\x19" },
    { "\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C\x0C"
      "\x0C\x0C\x0C\x0C" },
    { "" }, /* 0xAA 131 times */
    { "" }
};

static const int sha256_hmac_test_keylen[7] =
{
    20, 4, 20, 25, 20, 131, 131
};

static unsigned char sha256_hmac_test_buf[7][153] =
{
    { "Hi There" },
    { "what do ya want for nothing?" },
    { "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
      "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
      "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
      "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD"
      "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD" },
    { "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
      "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
      "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
      "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
      "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD" },
    { "Test With Truncation" },
    { "Test Using Larger Than Block-Size Key - Hash Key First" },
    { "This is a test using a larger than block-size key "
      "and a larger than block-size data. The key needs to "
      "be hashed before being used by the HMAC algorithm." }
};

static const int sha256_hmac_test_buflen[7] =
{
    8, 28, 50, 50, 20, 54, 152
};

static const unsigned char sha256_hmac_test_sum[14][32] =
{
    /*
     * HMAC-SHA-224 test vectors
     */
    { 0x89, 0x6F, 0xB1, 0x12, 0x8A, 0xBB, 0xDF, 0x19,
      0x68, 0x32, 0x10, 0x7C, 0xD4, 0x9D, 0xF3, 0x3F,
      0x47, 0xB4, 0xB1, 0x16, 0x99, 0x12, 0xBA, 0x4F,
      0x53, 0x68, 0x4B, 0x22 },
    { 0xA3, 0x0E, 0x01, 0x09, 0x8B, 0xC6, 0xDB, 0xBF,
      0x45, 0x69, 0x0F, 0x3A, 0x7E, 0x9E, 0x6D, 0x0F,
      0x8B, 0xBE, 0xA2, 0xA3, 0x9E, 0x61, 0x48, 0x00,
      0x8F, 0xD0, 0x5E, 0x44 },
    { 0x7F, 0xB3, 0xCB, 0x35, 0x88, 0xC6, 0xC1, 0xF6,
      0xFF, 0xA9, 0x69, 0x4D, 0x7D, 0x6A, 0xD2, 0x64,
      0x93, 0x65, 0xB0, 0xC1, 0xF6, 0x5D, 0x69, 0xD1,
      0xEC, 0x83, 0x33, 0xEA },
    { 0x6C, 0x11, 0x50, 0x68, 0x74, 0x01, 0x3C, 0xAC,
      0x6A, 0x2A, 0xBC, 0x1B, 0xB3, 0x82, 0x62, 0x7C,
      0xEC, 0x6A, 0x90, 0xD8, 0x6E, 0xFC, 0x01, 0x2D,
      0xE7, 0xAF, 0xEC, 0x5A },
    { 0x0E, 0x2A, 0xEA, 0x68, 0xA9, 0x0C, 0x8D, 0x37,
      0xC9, 0x88, 0xBC, 0xDB, 0x9F, 0xCA, 0x6F, 0xA8 },
    { 0x95, 0xE9, 0xA0, 0xDB, 0x96, 0x20, 0x95, 0xAD,
      0xAE, 0xBE, 0x9B, 0x2D, 0x6F, 0x0D, 0xBC, 0xE2,
      0xD4, 0x99, 0xF1, 0x12, 0xF2, 0xD2, 0xB7, 0x27,
      0x3F, 0xA6, 0x87, 0x0E },
    { 0x3A, 0x85, 0x41, 0x66, 0xAC, 0x5D, 0x9F, 0x02,
      0x3F, 0x54, 0xD5, 0x17, 0xD0, 0xB3, 0x9D, 0xBD,
      0x94, 0x67, 0x70, 0xDB, 0x9C, 0x2B, 0x95, 0xC9,
      0xF6, 0xF5, 0x65, 0xD1 },

    /*
     * HMAC-SHA-256 test vectors
     */
    { 0xB0, 0x34, 0x4C, 0x61, 0xD8, 0xDB, 0x38, 0x53,
      0x5C, 0xA8, 0xAF, 0xCE, 0xAF, 0x0B, 0xF1, 0x2B,
      0x88, 0x1D, 0xC2, 0x00, 0xC9, 0x83, 0x3D, 0xA7,
      0x26, 0xE9, 0x37, 0x6C, 0x2E, 0x32, 0xCF, 0xF7 },
    { 0x5B, 0xDC, 0xC1, 0x46, 0xBF, 0x60, 0x75, 0x4E,
      0x6A, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xC7,
      0x5A, 0x00, 0x3F, 0x08, 0x9D, 0x27, 0x39, 0x83,
      0x9D, 0xEC, 0x58, 0xB9, 0x64, 0xEC, 0x38, 0x43 },
    { 0x77, 0x3E, 0xA9, 0x1E, 0x36, 0x80, 0x0E, 0x46,
      0x85, 0x4D, 0xB8, 0xEB, 0xD0, 0x91, 0x81, 0xA7,
      0x29, 0x59, 0x09, 0x8B, 0x3E, 0xF8, 0xC1, 0x22,
      0xD9, 0x63, 0x55, 0x14, 0xCE, 0xD5, 0x65, 0xFE },
    { 0x82, 0x55, 0x8A, 0x38, 0x9A, 0x44, 0x3C, 0x0E,
      0xA4, 0xCC, 0x81, 0x98, 0x99, 0xF2, 0x08, 0x3A,
      0x85, 0xF0, 0xFA, 0xA3, 0xE5, 0x78, 0xF8, 0x07,
      0x7A, 0x2E, 0x3F, 0xF4, 0x67, 0x29, 0x66, 0x5B },
    { 0xA3, 0xB6, 0x16, 0x74, 0x73, 0x10, 0x0E, 0xE0,
      0x6E, 0x0C, 0x79, 0x6C, 0x29, 0x55, 0x55, 0x2B },
    { 0x60, 0xE4, 0x31, 0x59, 0x1E, 0xE0, 0xB6, 0x7F,
      0x0D, 0x8A, 0x26, 0xAA, 0xCB, 0xF5, 0xB7, 0x7F,
      0x8E, 0x0B, 0xC6, 0x21, 0x37, 0x28, 0xC5, 0x14,
      0x05, 0x46, 0x04, 0x0F, 0x0E, 0xE3, 0x7F, 0x54 },
    { 0x9B, 0x09, 0xFF, 0xA7, 0x1B, 0x94, 0x2F, 0xCB,
      0x27, 0x63, 0x5F, 0xBC, 0xD5, 0xB0, 0xE9, 0x44,
      0xBF, 0xDC, 0x63, 0x64, 0x4F, 0x07, 0x13, 0x93,
      0x8A, 0x7F, 0x51, 0x53, 0x5C, 0x3A, 0x35, 0xE2 }
};

/*
 * Checkup routine
 */
int sha256_self_test( int verbose )
{
    int i, j, k, buflen, ret = 0;
    unsigned char buf[1024];
    unsigned char sha256sum[32];
    sha256_context ctx;

    sha256_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        if( verbose != 0 )
            polarssl_printf( "  SHA-%d test #%d: ", 256 - k * 32, j + 1 );

        sha256_starts( &ctx, k );

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
                sha256_update( &ctx, buf, buflen );
        }
        else
            sha256_update( &ctx, sha256_test_buf[j],
                                 sha256_test_buflen[j] );

        sha256_finish( &ctx, sha256sum );

        if( memcmp( sha256sum, sha256_test_sum[i], 32 - k * 4 ) != 0 )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            polarssl_printf( "passed\n" );
    }

    if( verbose != 0 )
        polarssl_printf( "\n" );

    for( i = 0; i < 14; i++ )
    {
        j = i % 7;
        k = i < 7;

        if( verbose != 0 )
            polarssl_printf( "  HMAC-SHA-%d test #%d: ", 256 - k * 32, j + 1 );

        if( j == 5 || j == 6 )
        {
            memset( buf, '\xAA', buflen = 131 );
            sha256_hmac_starts( &ctx, buf, buflen, k );
        }
        else
            sha256_hmac_starts( &ctx, sha256_hmac_test_key[j],
                                      sha256_hmac_test_keylen[j], k );

        sha256_hmac_update( &ctx, sha256_hmac_test_buf[j],
                                  sha256_hmac_test_buflen[j] );

        sha256_hmac_finish( &ctx, sha256sum );

        buflen = ( j == 4 ) ? 16 : 32 - k * 4;

        if( memcmp( sha256sum, sha256_hmac_test_sum[i], buflen ) != 0 )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            polarssl_printf( "passed\n" );
    }

    if( verbose != 0 )
        polarssl_printf( "\n" );

exit:
    sha256_free( &ctx );

    return( ret );
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_SHA256_C */
