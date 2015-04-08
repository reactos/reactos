/*
 *  FIPS-180-2 compliant SHA-384/512 implementation
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
 *  The SHA-512 Secure Hash Standard was published by NIST in 2002.
 *
 *  http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_SHA512_C)

#include "polarssl/sha512.h"

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

#if !defined(POLARSSL_SHA512_ALT)

/*
 * 64-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT64_BE
#define GET_UINT64_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint64_t) (b)[(i)    ] << 56 )       \
        | ( (uint64_t) (b)[(i) + 1] << 48 )       \
        | ( (uint64_t) (b)[(i) + 2] << 40 )       \
        | ( (uint64_t) (b)[(i) + 3] << 32 )       \
        | ( (uint64_t) (b)[(i) + 4] << 24 )       \
        | ( (uint64_t) (b)[(i) + 5] << 16 )       \
        | ( (uint64_t) (b)[(i) + 6] <<  8 )       \
        | ( (uint64_t) (b)[(i) + 7]       );      \
}
#endif /* GET_UINT64_BE */

#ifndef PUT_UINT64_BE
#define PUT_UINT64_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 56 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 48 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 40 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 32 );       \
    (b)[(i) + 4] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 5] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 6] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 7] = (unsigned char) ( (n)       );       \
}
#endif /* PUT_UINT64_BE */

/*
 * Round constants
 */
static const uint64_t K[80] =
{
    UL64(0x428A2F98D728AE22),  UL64(0x7137449123EF65CD),
    UL64(0xB5C0FBCFEC4D3B2F),  UL64(0xE9B5DBA58189DBBC),
    UL64(0x3956C25BF348B538),  UL64(0x59F111F1B605D019),
    UL64(0x923F82A4AF194F9B),  UL64(0xAB1C5ED5DA6D8118),
    UL64(0xD807AA98A3030242),  UL64(0x12835B0145706FBE),
    UL64(0x243185BE4EE4B28C),  UL64(0x550C7DC3D5FFB4E2),
    UL64(0x72BE5D74F27B896F),  UL64(0x80DEB1FE3B1696B1),
    UL64(0x9BDC06A725C71235),  UL64(0xC19BF174CF692694),
    UL64(0xE49B69C19EF14AD2),  UL64(0xEFBE4786384F25E3),
    UL64(0x0FC19DC68B8CD5B5),  UL64(0x240CA1CC77AC9C65),
    UL64(0x2DE92C6F592B0275),  UL64(0x4A7484AA6EA6E483),
    UL64(0x5CB0A9DCBD41FBD4),  UL64(0x76F988DA831153B5),
    UL64(0x983E5152EE66DFAB),  UL64(0xA831C66D2DB43210),
    UL64(0xB00327C898FB213F),  UL64(0xBF597FC7BEEF0EE4),
    UL64(0xC6E00BF33DA88FC2),  UL64(0xD5A79147930AA725),
    UL64(0x06CA6351E003826F),  UL64(0x142929670A0E6E70),
    UL64(0x27B70A8546D22FFC),  UL64(0x2E1B21385C26C926),
    UL64(0x4D2C6DFC5AC42AED),  UL64(0x53380D139D95B3DF),
    UL64(0x650A73548BAF63DE),  UL64(0x766A0ABB3C77B2A8),
    UL64(0x81C2C92E47EDAEE6),  UL64(0x92722C851482353B),
    UL64(0xA2BFE8A14CF10364),  UL64(0xA81A664BBC423001),
    UL64(0xC24B8B70D0F89791),  UL64(0xC76C51A30654BE30),
    UL64(0xD192E819D6EF5218),  UL64(0xD69906245565A910),
    UL64(0xF40E35855771202A),  UL64(0x106AA07032BBD1B8),
    UL64(0x19A4C116B8D2D0C8),  UL64(0x1E376C085141AB53),
    UL64(0x2748774CDF8EEB99),  UL64(0x34B0BCB5E19B48A8),
    UL64(0x391C0CB3C5C95A63),  UL64(0x4ED8AA4AE3418ACB),
    UL64(0x5B9CCA4F7763E373),  UL64(0x682E6FF3D6B2B8A3),
    UL64(0x748F82EE5DEFB2FC),  UL64(0x78A5636F43172F60),
    UL64(0x84C87814A1F0AB72),  UL64(0x8CC702081A6439EC),
    UL64(0x90BEFFFA23631E28),  UL64(0xA4506CEBDE82BDE9),
    UL64(0xBEF9A3F7B2C67915),  UL64(0xC67178F2E372532B),
    UL64(0xCA273ECEEA26619C),  UL64(0xD186B8C721C0C207),
    UL64(0xEADA7DD6CDE0EB1E),  UL64(0xF57D4F7FEE6ED178),
    UL64(0x06F067AA72176FBA),  UL64(0x0A637DC5A2C898A6),
    UL64(0x113F9804BEF90DAE),  UL64(0x1B710B35131C471B),
    UL64(0x28DB77F523047D84),  UL64(0x32CAAB7B40C72493),
    UL64(0x3C9EBE0A15C9BEBC),  UL64(0x431D67C49C100D4C),
    UL64(0x4CC5D4BECB3E42B6),  UL64(0x597F299CFC657E2A),
    UL64(0x5FCB6FAB3AD6FAEC),  UL64(0x6C44198C4A475817)
};

void sha512_init( sha512_context *ctx )
{
    memset( ctx, 0, sizeof( sha512_context ) );
}

void sha512_free( sha512_context *ctx )
{
    if( ctx == NULL )
        return;

    polarssl_zeroize( ctx, sizeof( sha512_context ) );
}

/*
 * SHA-512 context setup
 */
void sha512_starts( sha512_context *ctx, int is384 )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    if( is384 == 0 )
    {
        /* SHA-512 */
        ctx->state[0] = UL64(0x6A09E667F3BCC908);
        ctx->state[1] = UL64(0xBB67AE8584CAA73B);
        ctx->state[2] = UL64(0x3C6EF372FE94F82B);
        ctx->state[3] = UL64(0xA54FF53A5F1D36F1);
        ctx->state[4] = UL64(0x510E527FADE682D1);
        ctx->state[5] = UL64(0x9B05688C2B3E6C1F);
        ctx->state[6] = UL64(0x1F83D9ABFB41BD6B);
        ctx->state[7] = UL64(0x5BE0CD19137E2179);
    }
    else
    {
        /* SHA-384 */
        ctx->state[0] = UL64(0xCBBB9D5DC1059ED8);
        ctx->state[1] = UL64(0x629A292A367CD507);
        ctx->state[2] = UL64(0x9159015A3070DD17);
        ctx->state[3] = UL64(0x152FECD8F70E5939);
        ctx->state[4] = UL64(0x67332667FFC00B31);
        ctx->state[5] = UL64(0x8EB44A8768581511);
        ctx->state[6] = UL64(0xDB0C2E0D64F98FA7);
        ctx->state[7] = UL64(0x47B5481DBEFA4FA4);
    }

    ctx->is384 = is384;
}

void sha512_process( sha512_context *ctx, const unsigned char data[128] )
{
    int i;
    uint64_t temp1, temp2, W[80];
    uint64_t A, B, C, D, E, F, G, H;

#define  SHR(x,n) (x >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (64 - n)))

#define S0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^  SHR(x, 7))
#define S1(x) (ROTR(x,19) ^ ROTR(x,61) ^  SHR(x, 6))

#define S2(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define S3(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define P(a,b,c,d,e,f,g,h,x,K)                  \
{                                               \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
}

    for( i = 0; i < 16; i++ )
    {
        GET_UINT64_BE( W[i], data, i << 3 );
    }

    for( ; i < 80; i++ )
    {
        W[i] = S1(W[i -  2]) + W[i -  7] +
               S0(W[i - 15]) + W[i - 16];
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];
    i = 0;

    do
    {
        P( A, B, C, D, E, F, G, H, W[i], K[i] ); i++;
        P( H, A, B, C, D, E, F, G, W[i], K[i] ); i++;
        P( G, H, A, B, C, D, E, F, W[i], K[i] ); i++;
        P( F, G, H, A, B, C, D, E, W[i], K[i] ); i++;
        P( E, F, G, H, A, B, C, D, W[i], K[i] ); i++;
        P( D, E, F, G, H, A, B, C, W[i], K[i] ); i++;
        P( C, D, E, F, G, H, A, B, W[i], K[i] ); i++;
        P( B, C, D, E, F, G, H, A, W[i], K[i] ); i++;
    }
    while( i < 80 );

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
 * SHA-512 process buffer
 */
void sha512_update( sha512_context *ctx, const unsigned char *input,
                    size_t ilen )
{
    size_t fill;
    unsigned int left;

    if( ilen == 0 )
        return;

    left = (unsigned int) (ctx->total[0] & 0x7F);
    fill = 128 - left;

    ctx->total[0] += (uint64_t) ilen;

    if( ctx->total[0] < (uint64_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );
        sha512_process( ctx, ctx->buffer );
        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 128 )
    {
        sha512_process( ctx, input );
        input += 128;
        ilen  -= 128;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );
}

static const unsigned char sha512_padding[128] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * SHA-512 final digest
 */
void sha512_finish( sha512_context *ctx, unsigned char output[64] )
{
    size_t last, padn;
    uint64_t high, low;
    unsigned char msglen[16];

    high = ( ctx->total[0] >> 61 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT64_BE( high, msglen, 0 );
    PUT_UINT64_BE( low,  msglen, 8 );

    last = (size_t)( ctx->total[0] & 0x7F );
    padn = ( last < 112 ) ? ( 112 - last ) : ( 240 - last );

    sha512_update( ctx, sha512_padding, padn );
    sha512_update( ctx, msglen, 16 );

    PUT_UINT64_BE( ctx->state[0], output,  0 );
    PUT_UINT64_BE( ctx->state[1], output,  8 );
    PUT_UINT64_BE( ctx->state[2], output, 16 );
    PUT_UINT64_BE( ctx->state[3], output, 24 );
    PUT_UINT64_BE( ctx->state[4], output, 32 );
    PUT_UINT64_BE( ctx->state[5], output, 40 );

    if( ctx->is384 == 0 )
    {
        PUT_UINT64_BE( ctx->state[6], output, 48 );
        PUT_UINT64_BE( ctx->state[7], output, 56 );
    }
}

#endif /* !POLARSSL_SHA512_ALT */

/*
 * output = SHA-512( input buffer )
 */
void sha512( const unsigned char *input, size_t ilen,
             unsigned char output[64], int is384 )
{
    sha512_context ctx;

    sha512_init( &ctx );
    sha512_starts( &ctx, is384 );
    sha512_update( &ctx, input, ilen );
    sha512_finish( &ctx, output );
    sha512_free( &ctx );
}

#if defined(POLARSSL_FS_IO)
/*
 * output = SHA-512( file contents )
 */
int sha512_file( const char *path, unsigned char output[64], int is384 )
{
    FILE *f;
    size_t n;
    sha512_context ctx;
    unsigned char buf[1024];

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( POLARSSL_ERR_SHA512_FILE_IO_ERROR );

    sha512_init( &ctx );
    sha512_starts( &ctx, is384 );

    while( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        sha512_update( &ctx, buf, n );

    sha512_finish( &ctx, output );
    sha512_free( &ctx );

    if( ferror( f ) != 0 )
    {
        fclose( f );
        return( POLARSSL_ERR_SHA512_FILE_IO_ERROR );
    }

    fclose( f );
    return( 0 );
}
#endif /* POLARSSL_FS_IO */

/*
 * SHA-512 HMAC context setup
 */
void sha512_hmac_starts( sha512_context *ctx, const unsigned char *key,
                         size_t keylen, int is384 )
{
    size_t i;
    unsigned char sum[64];

    if( keylen > 128 )
    {
        sha512( key, keylen, sum, is384 );
        keylen = ( is384 ) ? 48 : 64;
        key = sum;
    }

    memset( ctx->ipad, 0x36, 128 );
    memset( ctx->opad, 0x5C, 128 );

    for( i = 0; i < keylen; i++ )
    {
        ctx->ipad[i] = (unsigned char)( ctx->ipad[i] ^ key[i] );
        ctx->opad[i] = (unsigned char)( ctx->opad[i] ^ key[i] );
    }

    sha512_starts( ctx, is384 );
    sha512_update( ctx, ctx->ipad, 128 );

    polarssl_zeroize( sum, sizeof( sum ) );
}

/*
 * SHA-512 HMAC process buffer
 */
void sha512_hmac_update( sha512_context  *ctx,
                         const unsigned char *input, size_t ilen )
{
    sha512_update( ctx, input, ilen );
}

/*
 * SHA-512 HMAC final digest
 */
void sha512_hmac_finish( sha512_context *ctx, unsigned char output[64] )
{
    int is384, hlen;
    unsigned char tmpbuf[64];

    is384 = ctx->is384;
    hlen = ( is384 == 0 ) ? 64 : 48;

    sha512_finish( ctx, tmpbuf );
    sha512_starts( ctx, is384 );
    sha512_update( ctx, ctx->opad, 128 );
    sha512_update( ctx, tmpbuf, hlen );
    sha512_finish( ctx, output );

    polarssl_zeroize( tmpbuf, sizeof( tmpbuf ) );
}

/*
 * SHA-512 HMAC context reset
 */
void sha512_hmac_reset( sha512_context *ctx )
{
    sha512_starts( ctx, ctx->is384 );
    sha512_update( ctx, ctx->ipad, 128 );
}

/*
 * output = HMAC-SHA-512( hmac key, input buffer )
 */
void sha512_hmac( const unsigned char *key, size_t keylen,
                const unsigned char *input, size_t ilen,
                unsigned char output[64], int is384 )
{
    sha512_context ctx;

    sha512_init( &ctx );
    sha512_hmac_starts( &ctx, key, keylen, is384 );
    sha512_hmac_update( &ctx, input, ilen );
    sha512_hmac_finish( &ctx, output );
    sha512_free( &ctx );
}

#if defined(POLARSSL_SELF_TEST)

/*
 * FIPS-180-2 test vectors
 */
static unsigned char sha512_test_buf[3][113] =
{
    { "abc" },
    { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
      "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu" },
    { "" }
};

static const int sha512_test_buflen[3] =
{
    3, 112, 1000
};

static const unsigned char sha512_test_sum[6][64] =
{
    /*
     * SHA-384 test vectors
     */
    { 0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B,
      0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
      0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63,
      0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
      0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23,
      0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7 },
    { 0x09, 0x33, 0x0C, 0x33, 0xF7, 0x11, 0x47, 0xE8,
      0x3D, 0x19, 0x2F, 0xC7, 0x82, 0xCD, 0x1B, 0x47,
      0x53, 0x11, 0x1B, 0x17, 0x3B, 0x3B, 0x05, 0xD2,
      0x2F, 0xA0, 0x80, 0x86, 0xE3, 0xB0, 0xF7, 0x12,
      0xFC, 0xC7, 0xC7, 0x1A, 0x55, 0x7E, 0x2D, 0xB9,
      0x66, 0xC3, 0xE9, 0xFA, 0x91, 0x74, 0x60, 0x39 },
    { 0x9D, 0x0E, 0x18, 0x09, 0x71, 0x64, 0x74, 0xCB,
      0x08, 0x6E, 0x83, 0x4E, 0x31, 0x0A, 0x4A, 0x1C,
      0xED, 0x14, 0x9E, 0x9C, 0x00, 0xF2, 0x48, 0x52,
      0x79, 0x72, 0xCE, 0xC5, 0x70, 0x4C, 0x2A, 0x5B,
      0x07, 0xB8, 0xB3, 0xDC, 0x38, 0xEC, 0xC4, 0xEB,
      0xAE, 0x97, 0xDD, 0xD8, 0x7F, 0x3D, 0x89, 0x85 },

    /*
     * SHA-512 test vectors
     */
    { 0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
      0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
      0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
      0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
      0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
      0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
      0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
      0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F },
    { 0x8E, 0x95, 0x9B, 0x75, 0xDA, 0xE3, 0x13, 0xDA,
      0x8C, 0xF4, 0xF7, 0x28, 0x14, 0xFC, 0x14, 0x3F,
      0x8F, 0x77, 0x79, 0xC6, 0xEB, 0x9F, 0x7F, 0xA1,
      0x72, 0x99, 0xAE, 0xAD, 0xB6, 0x88, 0x90, 0x18,
      0x50, 0x1D, 0x28, 0x9E, 0x49, 0x00, 0xF7, 0xE4,
      0x33, 0x1B, 0x99, 0xDE, 0xC4, 0xB5, 0x43, 0x3A,
      0xC7, 0xD3, 0x29, 0xEE, 0xB6, 0xDD, 0x26, 0x54,
      0x5E, 0x96, 0xE5, 0x5B, 0x87, 0x4B, 0xE9, 0x09 },
    { 0xE7, 0x18, 0x48, 0x3D, 0x0C, 0xE7, 0x69, 0x64,
      0x4E, 0x2E, 0x42, 0xC7, 0xBC, 0x15, 0xB4, 0x63,
      0x8E, 0x1F, 0x98, 0xB1, 0x3B, 0x20, 0x44, 0x28,
      0x56, 0x32, 0xA8, 0x03, 0xAF, 0xA9, 0x73, 0xEB,
      0xDE, 0x0F, 0xF2, 0x44, 0x87, 0x7E, 0xA6, 0x0A,
      0x4C, 0xB0, 0x43, 0x2C, 0xE5, 0x77, 0xC3, 0x1B,
      0xEB, 0x00, 0x9C, 0x5C, 0x2C, 0x49, 0xAA, 0x2E,
      0x4E, 0xAD, 0xB2, 0x17, 0xAD, 0x8C, 0xC0, 0x9B }
};

/*
 * RFC 4231 test vectors
 */
static unsigned char sha512_hmac_test_key[7][26] =
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

static const int sha512_hmac_test_keylen[7] =
{
    20, 4, 20, 25, 20, 131, 131
};

static unsigned char sha512_hmac_test_buf[7][153] =
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

static const int sha512_hmac_test_buflen[7] =
{
    8, 28, 50, 50, 20, 54, 152
};

static const unsigned char sha512_hmac_test_sum[14][64] =
{
    /*
     * HMAC-SHA-384 test vectors
     */
    { 0xAF, 0xD0, 0x39, 0x44, 0xD8, 0x48, 0x95, 0x62,
      0x6B, 0x08, 0x25, 0xF4, 0xAB, 0x46, 0x90, 0x7F,
      0x15, 0xF9, 0xDA, 0xDB, 0xE4, 0x10, 0x1E, 0xC6,
      0x82, 0xAA, 0x03, 0x4C, 0x7C, 0xEB, 0xC5, 0x9C,
      0xFA, 0xEA, 0x9E, 0xA9, 0x07, 0x6E, 0xDE, 0x7F,
      0x4A, 0xF1, 0x52, 0xE8, 0xB2, 0xFA, 0x9C, 0xB6 },
    { 0xAF, 0x45, 0xD2, 0xE3, 0x76, 0x48, 0x40, 0x31,
      0x61, 0x7F, 0x78, 0xD2, 0xB5, 0x8A, 0x6B, 0x1B,
      0x9C, 0x7E, 0xF4, 0x64, 0xF5, 0xA0, 0x1B, 0x47,
      0xE4, 0x2E, 0xC3, 0x73, 0x63, 0x22, 0x44, 0x5E,
      0x8E, 0x22, 0x40, 0xCA, 0x5E, 0x69, 0xE2, 0xC7,
      0x8B, 0x32, 0x39, 0xEC, 0xFA, 0xB2, 0x16, 0x49 },
    { 0x88, 0x06, 0x26, 0x08, 0xD3, 0xE6, 0xAD, 0x8A,
      0x0A, 0xA2, 0xAC, 0xE0, 0x14, 0xC8, 0xA8, 0x6F,
      0x0A, 0xA6, 0x35, 0xD9, 0x47, 0xAC, 0x9F, 0xEB,
      0xE8, 0x3E, 0xF4, 0xE5, 0x59, 0x66, 0x14, 0x4B,
      0x2A, 0x5A, 0xB3, 0x9D, 0xC1, 0x38, 0x14, 0xB9,
      0x4E, 0x3A, 0xB6, 0xE1, 0x01, 0xA3, 0x4F, 0x27 },
    { 0x3E, 0x8A, 0x69, 0xB7, 0x78, 0x3C, 0x25, 0x85,
      0x19, 0x33, 0xAB, 0x62, 0x90, 0xAF, 0x6C, 0xA7,
      0x7A, 0x99, 0x81, 0x48, 0x08, 0x50, 0x00, 0x9C,
      0xC5, 0x57, 0x7C, 0x6E, 0x1F, 0x57, 0x3B, 0x4E,
      0x68, 0x01, 0xDD, 0x23, 0xC4, 0xA7, 0xD6, 0x79,
      0xCC, 0xF8, 0xA3, 0x86, 0xC6, 0x74, 0xCF, 0xFB },
    { 0x3A, 0xBF, 0x34, 0xC3, 0x50, 0x3B, 0x2A, 0x23,
      0xA4, 0x6E, 0xFC, 0x61, 0x9B, 0xAE, 0xF8, 0x97 },
    { 0x4E, 0xCE, 0x08, 0x44, 0x85, 0x81, 0x3E, 0x90,
      0x88, 0xD2, 0xC6, 0x3A, 0x04, 0x1B, 0xC5, 0xB4,
      0x4F, 0x9E, 0xF1, 0x01, 0x2A, 0x2B, 0x58, 0x8F,
      0x3C, 0xD1, 0x1F, 0x05, 0x03, 0x3A, 0xC4, 0xC6,
      0x0C, 0x2E, 0xF6, 0xAB, 0x40, 0x30, 0xFE, 0x82,
      0x96, 0x24, 0x8D, 0xF1, 0x63, 0xF4, 0x49, 0x52 },
    { 0x66, 0x17, 0x17, 0x8E, 0x94, 0x1F, 0x02, 0x0D,
      0x35, 0x1E, 0x2F, 0x25, 0x4E, 0x8F, 0xD3, 0x2C,
      0x60, 0x24, 0x20, 0xFE, 0xB0, 0xB8, 0xFB, 0x9A,
      0xDC, 0xCE, 0xBB, 0x82, 0x46, 0x1E, 0x99, 0xC5,
      0xA6, 0x78, 0xCC, 0x31, 0xE7, 0x99, 0x17, 0x6D,
      0x38, 0x60, 0xE6, 0x11, 0x0C, 0x46, 0x52, 0x3E },

    /*
     * HMAC-SHA-512 test vectors
     */
    { 0x87, 0xAA, 0x7C, 0xDE, 0xA5, 0xEF, 0x61, 0x9D,
      0x4F, 0xF0, 0xB4, 0x24, 0x1A, 0x1D, 0x6C, 0xB0,
      0x23, 0x79, 0xF4, 0xE2, 0xCE, 0x4E, 0xC2, 0x78,
      0x7A, 0xD0, 0xB3, 0x05, 0x45, 0xE1, 0x7C, 0xDE,
      0xDA, 0xA8, 0x33, 0xB7, 0xD6, 0xB8, 0xA7, 0x02,
      0x03, 0x8B, 0x27, 0x4E, 0xAE, 0xA3, 0xF4, 0xE4,
      0xBE, 0x9D, 0x91, 0x4E, 0xEB, 0x61, 0xF1, 0x70,
      0x2E, 0x69, 0x6C, 0x20, 0x3A, 0x12, 0x68, 0x54 },
    { 0x16, 0x4B, 0x7A, 0x7B, 0xFC, 0xF8, 0x19, 0xE2,
      0xE3, 0x95, 0xFB, 0xE7, 0x3B, 0x56, 0xE0, 0xA3,
      0x87, 0xBD, 0x64, 0x22, 0x2E, 0x83, 0x1F, 0xD6,
      0x10, 0x27, 0x0C, 0xD7, 0xEA, 0x25, 0x05, 0x54,
      0x97, 0x58, 0xBF, 0x75, 0xC0, 0x5A, 0x99, 0x4A,
      0x6D, 0x03, 0x4F, 0x65, 0xF8, 0xF0, 0xE6, 0xFD,
      0xCA, 0xEA, 0xB1, 0xA3, 0x4D, 0x4A, 0x6B, 0x4B,
      0x63, 0x6E, 0x07, 0x0A, 0x38, 0xBC, 0xE7, 0x37 },
    { 0xFA, 0x73, 0xB0, 0x08, 0x9D, 0x56, 0xA2, 0x84,
      0xEF, 0xB0, 0xF0, 0x75, 0x6C, 0x89, 0x0B, 0xE9,
      0xB1, 0xB5, 0xDB, 0xDD, 0x8E, 0xE8, 0x1A, 0x36,
      0x55, 0xF8, 0x3E, 0x33, 0xB2, 0x27, 0x9D, 0x39,
      0xBF, 0x3E, 0x84, 0x82, 0x79, 0xA7, 0x22, 0xC8,
      0x06, 0xB4, 0x85, 0xA4, 0x7E, 0x67, 0xC8, 0x07,
      0xB9, 0x46, 0xA3, 0x37, 0xBE, 0xE8, 0x94, 0x26,
      0x74, 0x27, 0x88, 0x59, 0xE1, 0x32, 0x92, 0xFB },
    { 0xB0, 0xBA, 0x46, 0x56, 0x37, 0x45, 0x8C, 0x69,
      0x90, 0xE5, 0xA8, 0xC5, 0xF6, 0x1D, 0x4A, 0xF7,
      0xE5, 0x76, 0xD9, 0x7F, 0xF9, 0x4B, 0x87, 0x2D,
      0xE7, 0x6F, 0x80, 0x50, 0x36, 0x1E, 0xE3, 0xDB,
      0xA9, 0x1C, 0xA5, 0xC1, 0x1A, 0xA2, 0x5E, 0xB4,
      0xD6, 0x79, 0x27, 0x5C, 0xC5, 0x78, 0x80, 0x63,
      0xA5, 0xF1, 0x97, 0x41, 0x12, 0x0C, 0x4F, 0x2D,
      0xE2, 0xAD, 0xEB, 0xEB, 0x10, 0xA2, 0x98, 0xDD },
    { 0x41, 0x5F, 0xAD, 0x62, 0x71, 0x58, 0x0A, 0x53,
      0x1D, 0x41, 0x79, 0xBC, 0x89, 0x1D, 0x87, 0xA6 },
    { 0x80, 0xB2, 0x42, 0x63, 0xC7, 0xC1, 0xA3, 0xEB,
      0xB7, 0x14, 0x93, 0xC1, 0xDD, 0x7B, 0xE8, 0xB4,
      0x9B, 0x46, 0xD1, 0xF4, 0x1B, 0x4A, 0xEE, 0xC1,
      0x12, 0x1B, 0x01, 0x37, 0x83, 0xF8, 0xF3, 0x52,
      0x6B, 0x56, 0xD0, 0x37, 0xE0, 0x5F, 0x25, 0x98,
      0xBD, 0x0F, 0xD2, 0x21, 0x5D, 0x6A, 0x1E, 0x52,
      0x95, 0xE6, 0x4F, 0x73, 0xF6, 0x3F, 0x0A, 0xEC,
      0x8B, 0x91, 0x5A, 0x98, 0x5D, 0x78, 0x65, 0x98 },
    { 0xE3, 0x7B, 0x6A, 0x77, 0x5D, 0xC8, 0x7D, 0xBA,
      0xA4, 0xDF, 0xA9, 0xF9, 0x6E, 0x5E, 0x3F, 0xFD,
      0xDE, 0xBD, 0x71, 0xF8, 0x86, 0x72, 0x89, 0x86,
      0x5D, 0xF5, 0xA3, 0x2D, 0x20, 0xCD, 0xC9, 0x44,
      0xB6, 0x02, 0x2C, 0xAC, 0x3C, 0x49, 0x82, 0xB1,
      0x0D, 0x5E, 0xEB, 0x55, 0xC3, 0xE4, 0xDE, 0x15,
      0x13, 0x46, 0x76, 0xFB, 0x6D, 0xE0, 0x44, 0x60,
      0x65, 0xC9, 0x74, 0x40, 0xFA, 0x8C, 0x6A, 0x58 }
};

/*
 * Checkup routine
 */
int sha512_self_test( int verbose )
{
    int i, j, k, buflen, ret = 0;
    unsigned char buf[1024];
    unsigned char sha512sum[64];
    sha512_context ctx;

    sha512_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        if( verbose != 0 )
            polarssl_printf( "  SHA-%d test #%d: ", 512 - k * 128, j + 1 );

        sha512_starts( &ctx, k );

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );

            for( j = 0; j < 1000; j++ )
                sha512_update( &ctx, buf, buflen );
        }
        else
            sha512_update( &ctx, sha512_test_buf[j],
                                 sha512_test_buflen[j] );

        sha512_finish( &ctx, sha512sum );

        if( memcmp( sha512sum, sha512_test_sum[i], 64 - k * 16 ) != 0 )
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
            polarssl_printf( "  HMAC-SHA-%d test #%d: ", 512 - k * 128, j + 1 );

        if( j == 5 || j == 6 )
        {
            memset( buf, '\xAA', buflen = 131 );
            sha512_hmac_starts( &ctx, buf, buflen, k );
        }
        else
            sha512_hmac_starts( &ctx, sha512_hmac_test_key[j],
                                      sha512_hmac_test_keylen[j], k );

        sha512_hmac_update( &ctx, sha512_hmac_test_buf[j],
                                  sha512_hmac_test_buflen[j] );

        sha512_hmac_finish( &ctx, sha512sum );

        buflen = ( j == 4 ) ? 16 : 64 - k * 16;

        if( memcmp( sha512sum, sha512_hmac_test_sum[i], buflen ) != 0 )
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
    sha512_free( &ctx );

    return( ret );
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_SHA512_C */
