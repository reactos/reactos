/*
 *  SSLv3/TLSv1 shared functions
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
 *  The SSL 3.0 specification was drafted by Netscape in 1996,
 *  and became an IETF standard in 1999.
 *
 *  http://wp.netscape.com/eng/ssl3/
 *  http://www.ietf.org/rfc/rfc2246.txt
 *  http://www.ietf.org/rfc/rfc4346.txt
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_SSL_TLS_C)

#include "polarssl/debug.h"
#include "polarssl/ssl.h"

#if defined(POLARSSL_X509_CRT_PARSE_C) && \
    defined(POLARSSL_X509_CHECK_EXTENDED_KEY_USAGE)
#include "polarssl/oid.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#include <stdlib.h>

#if defined(_MSC_VER) && !defined strcasecmp && !defined(EFIX64) && \
    !defined(EFI32)
#define strcasecmp _stricmp
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if defined(POLARSSL_SSL_MAX_FRAGMENT_LENGTH)
/*
 * Convert max_fragment_length codes to length.
 * RFC 6066 says:
 *    enum{
 *        2^9(1), 2^10(2), 2^11(3), 2^12(4), (255)
 *    } MaxFragmentLength;
 * and we add 0 -> extension unused
 */
static unsigned int mfl_code_to_length[SSL_MAX_FRAG_LEN_INVALID] =
{
    SSL_MAX_CONTENT_LEN,    /* SSL_MAX_FRAG_LEN_NONE */
    512,                    /* SSL_MAX_FRAG_LEN_512  */
    1024,                   /* SSL_MAX_FRAG_LEN_1024 */
    2048,                   /* SSL_MAX_FRAG_LEN_2048 */
    4096,                   /* SSL_MAX_FRAG_LEN_4096 */
};
#endif /* POLARSSL_SSL_MAX_FRAGMENT_LENGTH */

static int ssl_session_copy( ssl_session *dst, const ssl_session *src )
{
    ssl_session_free( dst );
    memcpy( dst, src, sizeof( ssl_session ) );

#if defined(POLARSSL_X509_CRT_PARSE_C)
    if( src->peer_cert != NULL )
    {
        int ret;

        dst->peer_cert = (x509_crt *) polarssl_malloc( sizeof(x509_crt) );
        if( dst->peer_cert == NULL )
            return( POLARSSL_ERR_SSL_MALLOC_FAILED );

        x509_crt_init( dst->peer_cert );

        if( ( ret = x509_crt_parse_der( dst->peer_cert, src->peer_cert->raw.p,
                                        src->peer_cert->raw.len ) ) != 0 )
        {
            polarssl_free( dst->peer_cert );
            dst->peer_cert = NULL;
            return( ret );
        }
    }
#endif /* POLARSSL_X509_CRT_PARSE_C */

#if defined(POLARSSL_SSL_SESSION_TICKETS)
    if( src->ticket != NULL )
    {
        dst->ticket = (unsigned char *) polarssl_malloc( src->ticket_len );
        if( dst->ticket == NULL )
            return( POLARSSL_ERR_SSL_MALLOC_FAILED );

        memcpy( dst->ticket, src->ticket, src->ticket_len );
    }
#endif /* POLARSSL_SSL_SESSION_TICKETS */

    return( 0 );
}

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
int (*ssl_hw_record_init)( ssl_context *ssl,
                     const unsigned char *key_enc, const unsigned char *key_dec,
                     size_t keylen,
                     const unsigned char *iv_enc,  const unsigned char *iv_dec,
                     size_t ivlen,
                     const unsigned char *mac_enc, const unsigned char *mac_dec,
                     size_t maclen ) = NULL;
int (*ssl_hw_record_activate)( ssl_context *ssl, int direction) = NULL;
int (*ssl_hw_record_reset)( ssl_context *ssl ) = NULL;
int (*ssl_hw_record_write)( ssl_context *ssl ) = NULL;
int (*ssl_hw_record_read)( ssl_context *ssl ) = NULL;
int (*ssl_hw_record_finish)( ssl_context *ssl ) = NULL;
#endif /* POLARSSL_SSL_HW_RECORD_ACCEL */

/*
 * Key material generation
 */
#if defined(POLARSSL_SSL_PROTO_SSL3)
static int ssl3_prf( const unsigned char *secret, size_t slen,
                     const char *label,
                     const unsigned char *random, size_t rlen,
                     unsigned char *dstbuf, size_t dlen )
{
    size_t i;
    md5_context md5;
    sha1_context sha1;
    unsigned char padding[16];
    unsigned char sha1sum[20];
    ((void)label);

    md5_init(  &md5  );
    sha1_init( &sha1 );

    /*
     *  SSLv3:
     *    block =
     *      MD5( secret + SHA1( 'A'    + secret + random ) ) +
     *      MD5( secret + SHA1( 'BB'   + secret + random ) ) +
     *      MD5( secret + SHA1( 'CCC'  + secret + random ) ) +
     *      ...
     */
    for( i = 0; i < dlen / 16; i++ )
    {
        memset( padding, (unsigned char) ('A' + i), 1 + i );

        sha1_starts( &sha1 );
        sha1_update( &sha1, padding, 1 + i );
        sha1_update( &sha1, secret, slen );
        sha1_update( &sha1, random, rlen );
        sha1_finish( &sha1, sha1sum );

        md5_starts( &md5 );
        md5_update( &md5, secret, slen );
        md5_update( &md5, sha1sum, 20 );
        md5_finish( &md5, dstbuf + i * 16 );
    }

    md5_free(  &md5  );
    sha1_free( &sha1 );

    polarssl_zeroize( padding, sizeof( padding ) );
    polarssl_zeroize( sha1sum, sizeof( sha1sum ) );

    return( 0 );
}
#endif /* POLARSSL_SSL_PROTO_SSL3 */

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1)
static int tls1_prf( const unsigned char *secret, size_t slen,
                     const char *label,
                     const unsigned char *random, size_t rlen,
                     unsigned char *dstbuf, size_t dlen )
{
    size_t nb, hs;
    size_t i, j, k;
    const unsigned char *S1, *S2;
    unsigned char tmp[128];
    unsigned char h_i[20];

    if( sizeof( tmp ) < 20 + strlen( label ) + rlen )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    hs = ( slen + 1 ) / 2;
    S1 = secret;
    S2 = secret + slen - hs;

    nb = strlen( label );
    memcpy( tmp + 20, label, nb );
    memcpy( tmp + 20 + nb, random, rlen );
    nb += rlen;

    /*
     * First compute P_md5(secret,label+random)[0..dlen]
     */
    md5_hmac( S1, hs, tmp + 20, nb, 4 + tmp );

    for( i = 0; i < dlen; i += 16 )
    {
        md5_hmac( S1, hs, 4 + tmp, 16 + nb, h_i );
        md5_hmac( S1, hs, 4 + tmp, 16,  4 + tmp );

        k = ( i + 16 > dlen ) ? dlen % 16 : 16;

        for( j = 0; j < k; j++ )
            dstbuf[i + j]  = h_i[j];
    }

    /*
     * XOR out with P_sha1(secret,label+random)[0..dlen]
     */
    sha1_hmac( S2, hs, tmp + 20, nb, tmp );

    for( i = 0; i < dlen; i += 20 )
    {
        sha1_hmac( S2, hs, tmp, 20 + nb, h_i );
        sha1_hmac( S2, hs, tmp, 20,      tmp );

        k = ( i + 20 > dlen ) ? dlen % 20 : 20;

        for( j = 0; j < k; j++ )
            dstbuf[i + j] = (unsigned char)( dstbuf[i + j] ^ h_i[j] );
    }

    polarssl_zeroize( tmp, sizeof( tmp ) );
    polarssl_zeroize( h_i, sizeof( h_i ) );

    return( 0 );
}
#endif /* POLARSSL_SSL_PROTO_TLS1) || POLARSSL_SSL_PROTO_TLS1_1 */

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
static int tls_prf_sha256( const unsigned char *secret, size_t slen,
                           const char *label,
                           const unsigned char *random, size_t rlen,
                           unsigned char *dstbuf, size_t dlen )
{
    size_t nb;
    size_t i, j, k;
    unsigned char tmp[128];
    unsigned char h_i[32];

    if( sizeof( tmp ) < 32 + strlen( label ) + rlen )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    nb = strlen( label );
    memcpy( tmp + 32, label, nb );
    memcpy( tmp + 32 + nb, random, rlen );
    nb += rlen;

    /*
     * Compute P_<hash>(secret, label + random)[0..dlen]
     */
    sha256_hmac( secret, slen, tmp + 32, nb, tmp, 0 );

    for( i = 0; i < dlen; i += 32 )
    {
        sha256_hmac( secret, slen, tmp, 32 + nb, h_i, 0 );
        sha256_hmac( secret, slen, tmp, 32,      tmp, 0 );

        k = ( i + 32 > dlen ) ? dlen % 32 : 32;

        for( j = 0; j < k; j++ )
            dstbuf[i + j]  = h_i[j];
    }

    polarssl_zeroize( tmp, sizeof( tmp ) );
    polarssl_zeroize( h_i, sizeof( h_i ) );

    return( 0 );
}
#endif /* POLARSSL_SHA256_C */

#if defined(POLARSSL_SHA512_C)
static int tls_prf_sha384( const unsigned char *secret, size_t slen,
                           const char *label,
                           const unsigned char *random, size_t rlen,
                           unsigned char *dstbuf, size_t dlen )
{
    size_t nb;
    size_t i, j, k;
    unsigned char tmp[128];
    unsigned char h_i[48];

    if( sizeof( tmp ) < 48 + strlen( label ) + rlen )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    nb = strlen( label );
    memcpy( tmp + 48, label, nb );
    memcpy( tmp + 48 + nb, random, rlen );
    nb += rlen;

    /*
     * Compute P_<hash>(secret, label + random)[0..dlen]
     */
    sha512_hmac( secret, slen, tmp + 48, nb, tmp, 1 );

    for( i = 0; i < dlen; i += 48 )
    {
        sha512_hmac( secret, slen, tmp, 48 + nb, h_i, 1 );
        sha512_hmac( secret, slen, tmp, 48,      tmp, 1 );

        k = ( i + 48 > dlen ) ? dlen % 48 : 48;

        for( j = 0; j < k; j++ )
            dstbuf[i + j]  = h_i[j];
    }

    polarssl_zeroize( tmp, sizeof( tmp ) );
    polarssl_zeroize( h_i, sizeof( h_i ) );

    return( 0 );
}
#endif /* POLARSSL_SHA512_C */
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

static void ssl_update_checksum_start( ssl_context *, const unsigned char *, size_t );

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_1)
static void ssl_update_checksum_md5sha1( ssl_context *, const unsigned char *, size_t );
#endif

#if defined(POLARSSL_SSL_PROTO_SSL3)
static void ssl_calc_verify_ssl( ssl_context *, unsigned char * );
static void ssl_calc_finished_ssl( ssl_context *, unsigned char *, int );
#endif

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1)
static void ssl_calc_verify_tls( ssl_context *, unsigned char * );
static void ssl_calc_finished_tls( ssl_context *, unsigned char *, int );
#endif

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
static void ssl_update_checksum_sha256( ssl_context *, const unsigned char *, size_t );
static void ssl_calc_verify_tls_sha256( ssl_context *,unsigned char * );
static void ssl_calc_finished_tls_sha256( ssl_context *,unsigned char *, int );
#endif

#if defined(POLARSSL_SHA512_C)
static void ssl_update_checksum_sha384( ssl_context *, const unsigned char *, size_t );
static void ssl_calc_verify_tls_sha384( ssl_context *, unsigned char * );
static void ssl_calc_finished_tls_sha384( ssl_context *, unsigned char *, int );
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

int ssl_derive_keys( ssl_context *ssl )
{
    int ret = 0;
    unsigned char tmp[64];
    unsigned char keyblk[256];
    unsigned char *key1;
    unsigned char *key2;
    unsigned char *mac_enc;
    unsigned char *mac_dec;
    size_t iv_copy_len;
    const cipher_info_t *cipher_info;
    const md_info_t *md_info;

    ssl_session *session = ssl->session_negotiate;
    ssl_transform *transform = ssl->transform_negotiate;
    ssl_handshake_params *handshake = ssl->handshake;

    SSL_DEBUG_MSG( 2, ( "=> derive keys" ) );

    cipher_info = cipher_info_from_type( transform->ciphersuite_info->cipher );
    if( cipher_info == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "cipher info for %d not found",
                            transform->ciphersuite_info->cipher ) );
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    md_info = md_info_from_type( transform->ciphersuite_info->mac );
    if( md_info == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "md info for %d not found",
                            transform->ciphersuite_info->mac ) );
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    /*
     * Set appropriate PRF function and other SSL / TLS / TLS1.2 functions
     */
#if defined(POLARSSL_SSL_PROTO_SSL3)
    if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
    {
        handshake->tls_prf = ssl3_prf;
        handshake->calc_verify = ssl_calc_verify_ssl;
        handshake->calc_finished = ssl_calc_finished_ssl;
    }
    else
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1)
    if( ssl->minor_ver < SSL_MINOR_VERSION_3 )
    {
        handshake->tls_prf = tls1_prf;
        handshake->calc_verify = ssl_calc_verify_tls;
        handshake->calc_finished = ssl_calc_finished_tls;
    }
    else
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA512_C)
    if( ssl->minor_ver == SSL_MINOR_VERSION_3 &&
        transform->ciphersuite_info->mac == POLARSSL_MD_SHA384 )
    {
        handshake->tls_prf = tls_prf_sha384;
        handshake->calc_verify = ssl_calc_verify_tls_sha384;
        handshake->calc_finished = ssl_calc_finished_tls_sha384;
    }
    else
#endif
#if defined(POLARSSL_SHA256_C)
    if( ssl->minor_ver == SSL_MINOR_VERSION_3 )
    {
        handshake->tls_prf = tls_prf_sha256;
        handshake->calc_verify = ssl_calc_verify_tls_sha256;
        handshake->calc_finished = ssl_calc_finished_tls_sha256;
    }
    else
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    /*
     * SSLv3:
     *   master =
     *     MD5( premaster + SHA1( 'A'   + premaster + randbytes ) ) +
     *     MD5( premaster + SHA1( 'BB'  + premaster + randbytes ) ) +
     *     MD5( premaster + SHA1( 'CCC' + premaster + randbytes ) )
     *
     * TLSv1+:
     *   master = PRF( premaster, "master secret", randbytes )[0..47]
     */
    if( handshake->resume == 0 )
    {
        SSL_DEBUG_BUF( 3, "premaster secret", handshake->premaster,
                       handshake->pmslen );

#if defined(POLARSSL_SSL_EXTENDED_MASTER_SECRET)
        if( ssl->handshake->extended_ms == SSL_EXTENDED_MS_ENABLED )
        {
            unsigned char session_hash[48];
            size_t hash_len;

            SSL_DEBUG_MSG( 3, ( "using extended master secret" ) );

            ssl->handshake->calc_verify( ssl, session_hash );

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
            if( ssl->minor_ver == SSL_MINOR_VERSION_3 )
            {
#if defined(POLARSSL_SHA512_C)
                if( ssl->transform_negotiate->ciphersuite_info->mac ==
                    POLARSSL_MD_SHA384 )
                {
                    hash_len = 48;
                }
                else
#endif
                    hash_len = 32;
            }
            else
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */
                hash_len = 36;

            SSL_DEBUG_BUF( 3, "session hash", session_hash, hash_len );

            handshake->tls_prf( handshake->premaster, handshake->pmslen,
                                "extended master secret",
                                session_hash, hash_len, session->master, 48 );

        }
        else
#endif
        handshake->tls_prf( handshake->premaster, handshake->pmslen,
                            "master secret",
                            handshake->randbytes, 64, session->master, 48 );


        polarssl_zeroize( handshake->premaster, sizeof(handshake->premaster) );
    }
    else
        SSL_DEBUG_MSG( 3, ( "no premaster (session resumed)" ) );

    /*
     * Swap the client and server random values.
     */
    memcpy( tmp, handshake->randbytes, 64 );
    memcpy( handshake->randbytes, tmp + 32, 32 );
    memcpy( handshake->randbytes + 32, tmp, 32 );
    polarssl_zeroize( tmp, sizeof( tmp ) );

    /*
     *  SSLv3:
     *    key block =
     *      MD5( master + SHA1( 'A'    + master + randbytes ) ) +
     *      MD5( master + SHA1( 'BB'   + master + randbytes ) ) +
     *      MD5( master + SHA1( 'CCC'  + master + randbytes ) ) +
     *      MD5( master + SHA1( 'DDDD' + master + randbytes ) ) +
     *      ...
     *
     *  TLSv1:
     *    key block = PRF( master, "key expansion", randbytes )
     */
    handshake->tls_prf( session->master, 48, "key expansion",
                        handshake->randbytes, 64, keyblk, 256 );

    SSL_DEBUG_MSG( 3, ( "ciphersuite = %s",
                   ssl_get_ciphersuite_name( session->ciphersuite ) ) );
    SSL_DEBUG_BUF( 3, "master secret", session->master, 48 );
    SSL_DEBUG_BUF( 4, "random bytes", handshake->randbytes, 64 );
    SSL_DEBUG_BUF( 4, "key block", keyblk, 256 );

    polarssl_zeroize( handshake->randbytes, sizeof( handshake->randbytes ) );

    /*
     * Determine the appropriate key, IV and MAC length.
     */

    transform->keylen = cipher_info->key_length / 8;

    if( cipher_info->mode == POLARSSL_MODE_GCM ||
        cipher_info->mode == POLARSSL_MODE_CCM )
    {
        transform->maclen = 0;

        transform->ivlen = 12;
        transform->fixed_ivlen = 4;

        /* Minimum length is expicit IV + tag */
        transform->minlen = transform->ivlen - transform->fixed_ivlen
                            + ( transform->ciphersuite_info->flags &
                                POLARSSL_CIPHERSUITE_SHORT_TAG ? 8 : 16 );
    }
    else
    {
        int ret;

        /* Initialize HMAC contexts */
        if( ( ret = md_init_ctx( &transform->md_ctx_enc, md_info ) ) != 0 ||
            ( ret = md_init_ctx( &transform->md_ctx_dec, md_info ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "md_init_ctx", ret );
            return( ret );
        }

        /* Get MAC length */
        transform->maclen = md_get_size( md_info );

#if defined(POLARSSL_SSL_TRUNCATED_HMAC)
        /*
         * If HMAC is to be truncated, we shall keep the leftmost bytes,
         * (rfc 6066 page 13 or rfc 2104 section 4),
         * so we only need to adjust the length here.
         */
        if( session->trunc_hmac == SSL_TRUNC_HMAC_ENABLED )
            transform->maclen = SSL_TRUNCATED_HMAC_LEN;
#endif /* POLARSSL_SSL_TRUNCATED_HMAC */

        /* IV length */
        transform->ivlen = cipher_info->iv_size;

        /* Minimum length */
        if( cipher_info->mode == POLARSSL_MODE_STREAM )
            transform->minlen = transform->maclen;
        else
        {
            /*
             * GenericBlockCipher:
             * 1. if EtM is in use: one block plus MAC
             *    otherwise: * first multiple of blocklen greater than maclen
             * 2. IV except for SSL3 and TLS 1.0
             */
#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
            if( session->encrypt_then_mac == SSL_ETM_ENABLED )
            {
                transform->minlen = transform->maclen
                                  + cipher_info->block_size;
            }
            else
#endif
            {
                transform->minlen = transform->maclen
                                  + cipher_info->block_size
                                  - transform->maclen % cipher_info->block_size;
            }

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1)
            if( ssl->minor_ver == SSL_MINOR_VERSION_0 ||
                ssl->minor_ver == SSL_MINOR_VERSION_1 )
                ; /* No need to adjust minlen */
            else
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1_1) || defined(POLARSSL_SSL_PROTO_TLS1_2)
            if( ssl->minor_ver == SSL_MINOR_VERSION_2 ||
                ssl->minor_ver == SSL_MINOR_VERSION_3 )
            {
                transform->minlen += transform->ivlen;
            }
            else
#endif
            {
                SSL_DEBUG_MSG( 1, ( "should never happen" ) );
                return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
            }
        }
    }

    SSL_DEBUG_MSG( 3, ( "keylen: %d, minlen: %d, ivlen: %d, maclen: %d",
                   transform->keylen, transform->minlen, transform->ivlen,
                   transform->maclen ) );

    /*
     * Finally setup the cipher contexts, IVs and MAC secrets.
     */
#if defined(POLARSSL_SSL_CLI_C)
    if( ssl->endpoint == SSL_IS_CLIENT )
    {
        key1 = keyblk + transform->maclen * 2;
        key2 = keyblk + transform->maclen * 2 + transform->keylen;

        mac_enc = keyblk;
        mac_dec = keyblk + transform->maclen;

        /*
         * This is not used in TLS v1.1.
         */
        iv_copy_len = ( transform->fixed_ivlen ) ?
                            transform->fixed_ivlen : transform->ivlen;
        memcpy( transform->iv_enc, key2 + transform->keylen,  iv_copy_len );
        memcpy( transform->iv_dec, key2 + transform->keylen + iv_copy_len,
                iv_copy_len );
    }
    else
#endif /* POLARSSL_SSL_CLI_C */
#if defined(POLARSSL_SSL_SRV_C)
    if( ssl->endpoint == SSL_IS_SERVER )
    {
        key1 = keyblk + transform->maclen * 2 + transform->keylen;
        key2 = keyblk + transform->maclen * 2;

        mac_enc = keyblk + transform->maclen;
        mac_dec = keyblk;

        /*
         * This is not used in TLS v1.1.
         */
        iv_copy_len = ( transform->fixed_ivlen ) ?
                            transform->fixed_ivlen : transform->ivlen;
        memcpy( transform->iv_dec, key1 + transform->keylen,  iv_copy_len );
        memcpy( transform->iv_enc, key1 + transform->keylen + iv_copy_len,
                iv_copy_len );
    }
    else
#endif /* POLARSSL_SSL_SRV_C */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

#if defined(POLARSSL_SSL_PROTO_SSL3)
    if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
    {
        if( transform->maclen > sizeof transform->mac_enc )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        memcpy( transform->mac_enc, mac_enc, transform->maclen );
        memcpy( transform->mac_dec, mac_dec, transform->maclen );
    }
    else
#endif /* POLARSSL_SSL_PROTO_SSL3 */
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_2)
    if( ssl->minor_ver >= SSL_MINOR_VERSION_1 )
    {
        md_hmac_starts( &transform->md_ctx_enc, mac_enc, transform->maclen );
        md_hmac_starts( &transform->md_ctx_dec, mac_dec, transform->maclen );
    }
    else
#endif
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_init != NULL )
    {
        int ret = 0;

        SSL_DEBUG_MSG( 2, ( "going for ssl_hw_record_init()" ) );

        if( ( ret = ssl_hw_record_init( ssl, key1, key2, transform->keylen,
                                        transform->iv_enc, transform->iv_dec,
                                        iv_copy_len,
                                        mac_enc, mac_dec,
                                        transform->maclen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_init", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }
    }
#endif /* POLARSSL_SSL_HW_RECORD_ACCEL */

    if( ( ret = cipher_init_ctx( &transform->cipher_ctx_enc,
                                 cipher_info ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "cipher_init_ctx", ret );
        return( ret );
    }

    if( ( ret = cipher_init_ctx( &transform->cipher_ctx_dec,
                                 cipher_info ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "cipher_init_ctx", ret );
        return( ret );
    }

    if( ( ret = cipher_setkey( &transform->cipher_ctx_enc, key1,
                               cipher_info->key_length,
                               POLARSSL_ENCRYPT ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "cipher_setkey", ret );
        return( ret );
    }

    if( ( ret = cipher_setkey( &transform->cipher_ctx_dec, key2,
                               cipher_info->key_length,
                               POLARSSL_DECRYPT ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "cipher_setkey", ret );
        return( ret );
    }

#if defined(POLARSSL_CIPHER_MODE_CBC)
    if( cipher_info->mode == POLARSSL_MODE_CBC )
    {
        if( ( ret = cipher_set_padding_mode( &transform->cipher_ctx_enc,
                                             POLARSSL_PADDING_NONE ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_set_padding_mode", ret );
            return( ret );
        }

        if( ( ret = cipher_set_padding_mode( &transform->cipher_ctx_dec,
                                             POLARSSL_PADDING_NONE ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_set_padding_mode", ret );
            return( ret );
        }
    }
#endif /* POLARSSL_CIPHER_MODE_CBC */

    polarssl_zeroize( keyblk, sizeof( keyblk ) );

#if defined(POLARSSL_ZLIB_SUPPORT)
    // Initialize compression
    //
    if( session->compression == SSL_COMPRESS_DEFLATE )
    {
        if( ssl->compress_buf == NULL )
        {
            SSL_DEBUG_MSG( 3, ( "Allocating compression buffer" ) );
            ssl->compress_buf = polarssl_malloc( SSL_BUFFER_LEN );
            if( ssl->compress_buf == NULL )
            {
                SSL_DEBUG_MSG( 1, ( "malloc(%d bytes) failed",
                                    SSL_BUFFER_LEN ) );
                return( POLARSSL_ERR_SSL_MALLOC_FAILED );
            }
        }

        SSL_DEBUG_MSG( 3, ( "Initializing zlib states" ) );

        memset( &transform->ctx_deflate, 0, sizeof( transform->ctx_deflate ) );
        memset( &transform->ctx_inflate, 0, sizeof( transform->ctx_inflate ) );

        if( deflateInit( &transform->ctx_deflate,
                         Z_DEFAULT_COMPRESSION )   != Z_OK ||
            inflateInit( &transform->ctx_inflate ) != Z_OK )
        {
            SSL_DEBUG_MSG( 1, ( "Failed to initialize compression" ) );
            return( POLARSSL_ERR_SSL_COMPRESSION_FAILED );
        }
    }
#endif /* POLARSSL_ZLIB_SUPPORT */

    SSL_DEBUG_MSG( 2, ( "<= derive keys" ) );

    return( 0 );
}

#if defined(POLARSSL_SSL_PROTO_SSL3)
void ssl_calc_verify_ssl( ssl_context *ssl, unsigned char hash[36] )
{
    md5_context md5;
    sha1_context sha1;
    unsigned char pad_1[48];
    unsigned char pad_2[48];

    SSL_DEBUG_MSG( 2, ( "=> calc verify ssl" ) );

    memcpy( &md5 , &ssl->handshake->fin_md5 , sizeof(md5_context)  );
    memcpy( &sha1, &ssl->handshake->fin_sha1, sizeof(sha1_context) );

    memset( pad_1, 0x36, 48 );
    memset( pad_2, 0x5C, 48 );

    md5_update( &md5, ssl->session_negotiate->master, 48 );
    md5_update( &md5, pad_1, 48 );
    md5_finish( &md5, hash );

    md5_starts( &md5 );
    md5_update( &md5, ssl->session_negotiate->master, 48 );
    md5_update( &md5, pad_2, 48 );
    md5_update( &md5, hash,  16 );
    md5_finish( &md5, hash );

    sha1_update( &sha1, ssl->session_negotiate->master, 48 );
    sha1_update( &sha1, pad_1, 40 );
    sha1_finish( &sha1, hash + 16 );

    sha1_starts( &sha1 );
    sha1_update( &sha1, ssl->session_negotiate->master, 48 );
    sha1_update( &sha1, pad_2, 40 );
    sha1_update( &sha1, hash + 16, 20 );
    sha1_finish( &sha1, hash + 16 );

    SSL_DEBUG_BUF( 3, "calculated verify result", hash, 36 );
    SSL_DEBUG_MSG( 2, ( "<= calc verify" ) );

    md5_free(  &md5  );
    sha1_free( &sha1 );

    return;
}
#endif /* POLARSSL_SSL_PROTO_SSL3 */

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1)
void ssl_calc_verify_tls( ssl_context *ssl, unsigned char hash[36] )
{
    md5_context md5;
    sha1_context sha1;

    SSL_DEBUG_MSG( 2, ( "=> calc verify tls" ) );

    memcpy( &md5 , &ssl->handshake->fin_md5 , sizeof(md5_context)  );
    memcpy( &sha1, &ssl->handshake->fin_sha1, sizeof(sha1_context) );

     md5_finish( &md5,  hash );
    sha1_finish( &sha1, hash + 16 );

    SSL_DEBUG_BUF( 3, "calculated verify result", hash, 36 );
    SSL_DEBUG_MSG( 2, ( "<= calc verify" ) );

    md5_free(  &md5  );
    sha1_free( &sha1 );

    return;
}
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 */

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
void ssl_calc_verify_tls_sha256( ssl_context *ssl, unsigned char hash[32] )
{
    sha256_context sha256;

    SSL_DEBUG_MSG( 2, ( "=> calc verify sha256" ) );

    memcpy( &sha256, &ssl->handshake->fin_sha256, sizeof(sha256_context) );
    sha256_finish( &sha256, hash );

    SSL_DEBUG_BUF( 3, "calculated verify result", hash, 32 );
    SSL_DEBUG_MSG( 2, ( "<= calc verify" ) );

    sha256_free( &sha256 );

    return;
}
#endif /* POLARSSL_SHA256_C */

#if defined(POLARSSL_SHA512_C)
void ssl_calc_verify_tls_sha384( ssl_context *ssl, unsigned char hash[48] )
{
    sha512_context sha512;

    SSL_DEBUG_MSG( 2, ( "=> calc verify sha384" ) );

    memcpy( &sha512, &ssl->handshake->fin_sha512, sizeof(sha512_context) );
    sha512_finish( &sha512, hash );

    SSL_DEBUG_BUF( 3, "calculated verify result", hash, 48 );
    SSL_DEBUG_MSG( 2, ( "<= calc verify" ) );

    sha512_free( &sha512 );

    return;
}
#endif /* POLARSSL_SHA512_C */
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

#if defined(POLARSSL_KEY_EXCHANGE__SOME__PSK_ENABLED)
int ssl_psk_derive_premaster( ssl_context *ssl, key_exchange_type_t key_ex )
{
    unsigned char *p = ssl->handshake->premaster;
    unsigned char *end = p + sizeof( ssl->handshake->premaster );

    /*
     * PMS = struct {
     *     opaque other_secret<0..2^16-1>;
     *     opaque psk<0..2^16-1>;
     * };
     * with "other_secret" depending on the particular key exchange
     */
#if defined(POLARSSL_KEY_EXCHANGE_PSK_ENABLED)
    if( key_ex == POLARSSL_KEY_EXCHANGE_PSK )
    {
        if( end - p < 2 + (int) ssl->psk_len )
            return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

        *(p++) = (unsigned char)( ssl->psk_len >> 8 );
        *(p++) = (unsigned char)( ssl->psk_len      );
        p += ssl->psk_len;
    }
    else
#endif /* POLARSSL_KEY_EXCHANGE_PSK_ENABLED */
#if defined(POLARSSL_KEY_EXCHANGE_RSA_PSK_ENABLED)
    if( key_ex == POLARSSL_KEY_EXCHANGE_RSA_PSK )
    {
        /*
         * other_secret already set by the ClientKeyExchange message,
         * and is 48 bytes long
         */
        *p++ = 0;
        *p++ = 48;
        p += 48;
    }
    else
#endif /* POLARSSL_KEY_EXCHANGE_RSA_PKS_ENABLED */
#if defined(POLARSSL_KEY_EXCHANGE_DHE_PSK_ENABLED)
    if( key_ex == POLARSSL_KEY_EXCHANGE_DHE_PSK )
    {
        int ret;
        size_t len = end - ( p + 2 );

        /* Write length only when we know the actual value */
        if( ( ret = dhm_calc_secret( &ssl->handshake->dhm_ctx,
                                      p + 2, &len,
                                      ssl->f_rng, ssl->p_rng ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "dhm_calc_secret", ret );
            return( ret );
        }
        *(p++) = (unsigned char)( len >> 8 );
        *(p++) = (unsigned char)( len );
        p += len;

        SSL_DEBUG_MPI( 3, "DHM: K ", &ssl->handshake->dhm_ctx.K  );
    }
    else
#endif /* POLARSSL_KEY_EXCHANGE_DHE_PSK_ENABLED */
#if defined(POLARSSL_KEY_EXCHANGE_ECDHE_PSK_ENABLED)
    if( key_ex == POLARSSL_KEY_EXCHANGE_ECDHE_PSK )
    {
        int ret;
        size_t zlen;

        if( ( ret = ecdh_calc_secret( &ssl->handshake->ecdh_ctx, &zlen,
                                       p + 2, end - ( p + 2 ),
                                       ssl->f_rng, ssl->p_rng ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ecdh_calc_secret", ret );
            return( ret );
        }

        *(p++) = (unsigned char)( zlen >> 8 );
        *(p++) = (unsigned char)( zlen      );
        p += zlen;

        SSL_DEBUG_MPI( 3, "ECDH: z", &ssl->handshake->ecdh_ctx.z );
    }
    else
#endif /* POLARSSL_KEY_EXCHANGE_ECDHE_PSK_ENABLED */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    /* opaque psk<0..2^16-1>; */
    if( end - p < 2 + (int) ssl->psk_len )
            return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    *(p++) = (unsigned char)( ssl->psk_len >> 8 );
    *(p++) = (unsigned char)( ssl->psk_len      );
    memcpy( p, ssl->psk, ssl->psk_len );
    p += ssl->psk_len;

    ssl->handshake->pmslen = p - ssl->handshake->premaster;

    return( 0 );
}
#endif /* POLARSSL_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(POLARSSL_SSL_PROTO_SSL3)
/*
 * SSLv3.0 MAC functions
 */
static void ssl_mac( md_context_t *md_ctx, unsigned char *secret,
                     unsigned char *buf, size_t len,
                     unsigned char *ctr, int type )
{
    unsigned char header[11];
    unsigned char padding[48];
    int padlen;
    int md_size = md_get_size( md_ctx->md_info );
    int md_type = md_get_type( md_ctx->md_info );

    /* Only MD5 and SHA-1 supported */
    if( md_type == POLARSSL_MD_MD5 )
        padlen = 48;
    else
        padlen = 40;

    memcpy( header, ctr, 8 );
    header[ 8] = (unsigned char)  type;
    header[ 9] = (unsigned char)( len >> 8 );
    header[10] = (unsigned char)( len      );

    memset( padding, 0x36, padlen );
    md_starts( md_ctx );
    md_update( md_ctx, secret,  md_size );
    md_update( md_ctx, padding, padlen  );
    md_update( md_ctx, header,  11      );
    md_update( md_ctx, buf,     len     );
    md_finish( md_ctx, buf +    len     );

    memset( padding, 0x5C, padlen );
    md_starts( md_ctx );
    md_update( md_ctx, secret,    md_size );
    md_update( md_ctx, padding,   padlen  );
    md_update( md_ctx, buf + len, md_size );
    md_finish( md_ctx, buf + len          );
}
#endif /* POLARSSL_SSL_PROTO_SSL3 */

#if defined(POLARSSL_ARC4_C) || defined(POLARSSL_CIPHER_NULL_CIPHER) ||     \
    ( defined(POLARSSL_CIPHER_MODE_CBC) &&                                  \
      ( defined(POLARSSL_AES_C) || defined(POLARSSL_CAMELLIA_C) ) )
#define POLARSSL_SOME_MODES_USE_MAC
#endif

/*
 * Encryption/decryption functions
 */
static int ssl_encrypt_buf( ssl_context *ssl )
{
    size_t i;
    cipher_mode_t mode;
    int auth_done = 0;

    SSL_DEBUG_MSG( 2, ( "=> encrypt buf" ) );

    if( ssl->session_out == NULL || ssl->transform_out == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    mode = cipher_get_cipher_mode( &ssl->transform_out->cipher_ctx_enc );

    SSL_DEBUG_BUF( 4, "before encrypt: output payload",
                      ssl->out_msg, ssl->out_msglen );

    /*
     * Add MAC before if needed
     */
#if defined(POLARSSL_SOME_MODES_USE_MAC)
    if( mode == POLARSSL_MODE_STREAM ||
        ( mode == POLARSSL_MODE_CBC
#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
          && ssl->session_out->encrypt_then_mac == SSL_ETM_DISABLED
#endif
        ) )
    {
#if defined(POLARSSL_SSL_PROTO_SSL3)
        if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
        {
            ssl_mac( &ssl->transform_out->md_ctx_enc,
                      ssl->transform_out->mac_enc,
                      ssl->out_msg, ssl->out_msglen,
                      ssl->out_ctr, ssl->out_msgtype );
        }
        else
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
        defined(POLARSSL_SSL_PROTO_TLS1_2)
        if( ssl->minor_ver >= SSL_MINOR_VERSION_1 )
        {
            md_hmac_update( &ssl->transform_out->md_ctx_enc, ssl->out_ctr, 13 );
            md_hmac_update( &ssl->transform_out->md_ctx_enc,
                             ssl->out_msg, ssl->out_msglen );
            md_hmac_finish( &ssl->transform_out->md_ctx_enc,
                             ssl->out_msg + ssl->out_msglen );
            md_hmac_reset( &ssl->transform_out->md_ctx_enc );
        }
        else
#endif
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        SSL_DEBUG_BUF( 4, "computed mac",
                       ssl->out_msg + ssl->out_msglen,
                       ssl->transform_out->maclen );

        ssl->out_msglen += ssl->transform_out->maclen;
        auth_done++;
    }
#endif /* AEAD not the only option */

    /*
     * Encrypt
     */
#if defined(POLARSSL_ARC4_C) || defined(POLARSSL_CIPHER_NULL_CIPHER)
    if( mode == POLARSSL_MODE_STREAM )
    {
        int ret;
        size_t olen = 0;

        SSL_DEBUG_MSG( 3, ( "before encrypt: msglen = %d, "
                            "including %d bytes of padding",
                       ssl->out_msglen, 0 ) );

        if( ( ret = cipher_crypt( &ssl->transform_out->cipher_ctx_enc,
                                   ssl->transform_out->iv_enc,
                                   ssl->transform_out->ivlen,
                                   ssl->out_msg, ssl->out_msglen,
                                   ssl->out_msg, &olen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_crypt", ret );
            return( ret );
        }

        if( ssl->out_msglen != olen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }
    }
    else
#endif /* POLARSSL_ARC4_C || POLARSSL_CIPHER_NULL_CIPHER */
#if defined(POLARSSL_GCM_C) || defined(POLARSSL_CCM_C)
    if( mode == POLARSSL_MODE_GCM ||
        mode == POLARSSL_MODE_CCM )
    {
        int ret;
        size_t enc_msglen, olen;
        unsigned char *enc_msg;
        unsigned char add_data[13];
        unsigned char taglen = ssl->transform_out->ciphersuite_info->flags &
                               POLARSSL_CIPHERSUITE_SHORT_TAG ? 8 : 16;

        memcpy( add_data, ssl->out_ctr, 8 );
        add_data[8]  = ssl->out_msgtype;
        add_data[9]  = ssl->major_ver;
        add_data[10] = ssl->minor_ver;
        add_data[11] = ( ssl->out_msglen >> 8 ) & 0xFF;
        add_data[12] = ssl->out_msglen & 0xFF;

        SSL_DEBUG_BUF( 4, "additional data used for AEAD",
                       add_data, 13 );

        /*
         * Generate IV
         */
#if defined(POLARSSL_SSL_AEAD_RANDOM_IV)
        ret = ssl->f_rng( ssl->p_rng,
                ssl->transform_out->iv_enc + ssl->transform_out->fixed_ivlen,
                ssl->transform_out->ivlen - ssl->transform_out->fixed_ivlen );
        if( ret != 0 )
            return( ret );

        memcpy( ssl->out_iv,
                ssl->transform_out->iv_enc + ssl->transform_out->fixed_ivlen,
                ssl->transform_out->ivlen - ssl->transform_out->fixed_ivlen );
#else
        if( ssl->transform_out->ivlen - ssl->transform_out->fixed_ivlen != 8 )
        {
            /* Reminder if we ever add an AEAD mode with a different size */
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        memcpy( ssl->transform_out->iv_enc + ssl->transform_out->fixed_ivlen,
                             ssl->out_ctr, 8 );
        memcpy( ssl->out_iv, ssl->out_ctr, 8 );
#endif

        SSL_DEBUG_BUF( 4, "IV used", ssl->out_iv,
                ssl->transform_out->ivlen - ssl->transform_out->fixed_ivlen );

        /*
         * Fix pointer positions and message length with added IV
         */
        enc_msg = ssl->out_msg;
        enc_msglen = ssl->out_msglen;
        ssl->out_msglen += ssl->transform_out->ivlen -
                           ssl->transform_out->fixed_ivlen;

        SSL_DEBUG_MSG( 3, ( "before encrypt: msglen = %d, "
                            "including %d bytes of padding",
                       ssl->out_msglen, 0 ) );

        /*
         * Encrypt and authenticate
         */
        if( ( ret = cipher_auth_encrypt( &ssl->transform_out->cipher_ctx_enc,
                                         ssl->transform_out->iv_enc,
                                         ssl->transform_out->ivlen,
                                         add_data, 13,
                                         enc_msg, enc_msglen,
                                         enc_msg, &olen,
                                         enc_msg + enc_msglen, taglen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_auth_encrypt", ret );
            return( ret );
        }

        if( olen != enc_msglen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        ssl->out_msglen += taglen;
        auth_done++;

        SSL_DEBUG_BUF( 4, "after encrypt: tag", enc_msg + enc_msglen, taglen );
    }
    else
#endif /* POLARSSL_GCM_C || POLARSSL_CCM_C */
#if defined(POLARSSL_CIPHER_MODE_CBC) &&                                    \
    ( defined(POLARSSL_AES_C) || defined(POLARSSL_CAMELLIA_C) )
    if( mode == POLARSSL_MODE_CBC )
    {
        int ret;
        unsigned char *enc_msg;
        size_t enc_msglen, padlen, olen = 0;

        padlen = ssl->transform_out->ivlen - ( ssl->out_msglen + 1 ) %
                 ssl->transform_out->ivlen;
        if( padlen == ssl->transform_out->ivlen )
            padlen = 0;

        for( i = 0; i <= padlen; i++ )
            ssl->out_msg[ssl->out_msglen + i] = (unsigned char) padlen;

        ssl->out_msglen += padlen + 1;

        enc_msglen = ssl->out_msglen;
        enc_msg = ssl->out_msg;

#if defined(POLARSSL_SSL_PROTO_TLS1_1) || defined(POLARSSL_SSL_PROTO_TLS1_2)
        /*
         * Prepend per-record IV for block cipher in TLS v1.1 and up as per
         * Method 1 (6.2.3.2. in RFC4346 and RFC5246)
         */
        if( ssl->minor_ver >= SSL_MINOR_VERSION_2 )
        {
            /*
             * Generate IV
             */
            int ret = ssl->f_rng( ssl->p_rng, ssl->transform_out->iv_enc,
                                  ssl->transform_out->ivlen );
            if( ret != 0 )
                return( ret );

            memcpy( ssl->out_iv, ssl->transform_out->iv_enc,
                    ssl->transform_out->ivlen );

            /*
             * Fix pointer positions and message length with added IV
             */
            enc_msg = ssl->out_msg;
            enc_msglen = ssl->out_msglen;
            ssl->out_msglen += ssl->transform_out->ivlen;
        }
#endif /* POLARSSL_SSL_PROTO_TLS1_1 || POLARSSL_SSL_PROTO_TLS1_2 */

        SSL_DEBUG_MSG( 3, ( "before encrypt: msglen = %d, "
                            "including %d bytes of IV and %d bytes of padding",
                            ssl->out_msglen, ssl->transform_out->ivlen,
                            padlen + 1 ) );

        if( ( ret = cipher_crypt( &ssl->transform_out->cipher_ctx_enc,
                                   ssl->transform_out->iv_enc,
                                   ssl->transform_out->ivlen,
                                   enc_msg, enc_msglen,
                                   enc_msg, &olen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_crypt", ret );
            return( ret );
        }

        if( enc_msglen != olen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1)
        if( ssl->minor_ver < SSL_MINOR_VERSION_2 )
        {
            /*
             * Save IV in SSL3 and TLS1
             */
            memcpy( ssl->transform_out->iv_enc,
                    ssl->transform_out->cipher_ctx_enc.iv,
                    ssl->transform_out->ivlen );
        }
#endif

#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
        if( auth_done == 0 )
        {
            /*
             * MAC(MAC_write_key, seq_num +
             *     TLSCipherText.type +
             *     TLSCipherText.version +
             *     length_of( (IV +) ENC(...) ) +
             *     IV + // except for TLS 1.0
             *     ENC(content + padding + padding_length));
             */
            unsigned char pseudo_hdr[13];

            SSL_DEBUG_MSG( 3, ( "using encrypt then mac" ) );

            memcpy( pseudo_hdr +  0, ssl->out_ctr, 8 );
            memcpy( pseudo_hdr +  8, ssl->out_hdr, 3 );
            pseudo_hdr[11] = (unsigned char)( ( ssl->out_msglen >> 8 ) & 0xFF );
            pseudo_hdr[12] = (unsigned char)( ( ssl->out_msglen      ) & 0xFF );

            SSL_DEBUG_BUF( 4, "MAC'd meta-data", pseudo_hdr, 13 );

            md_hmac_update( &ssl->transform_out->md_ctx_enc, pseudo_hdr, 13 );
            md_hmac_update( &ssl->transform_out->md_ctx_enc,
                             ssl->out_iv, ssl->out_msglen );
            md_hmac_finish( &ssl->transform_out->md_ctx_enc,
                             ssl->out_iv + ssl->out_msglen );
            md_hmac_reset( &ssl->transform_out->md_ctx_enc );

            ssl->out_msglen += ssl->transform_out->maclen;
            auth_done++;
        }
#endif /* POLARSSL_SSL_ENCRYPT_THEN_MAC */
    }
    else
#endif /* POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_CAMELLIA_C ) */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    /* Make extra sure authentication was performed, exactly once */
    if( auth_done != 1 )
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    for( i = 8; i > 0; i-- )
        if( ++ssl->out_ctr[i - 1] != 0 )
            break;

    /* The loops goes to its end iff the counter is wrapping */
    if( i == 0 )
    {
        SSL_DEBUG_MSG( 1, ( "outgoing message counter would wrap" ) );
        return( POLARSSL_ERR_SSL_COUNTER_WRAPPING );
    }

    SSL_DEBUG_MSG( 2, ( "<= encrypt buf" ) );

    return( 0 );
}

#define POLARSSL_SSL_MAX_MAC_SIZE   48

static int ssl_decrypt_buf( ssl_context *ssl )
{
    size_t i;
    cipher_mode_t mode;
    int auth_done = 0;
#if defined(POLARSSL_SOME_MODES_USE_MAC)
    size_t padlen = 0, correct = 1;
#endif

    SSL_DEBUG_MSG( 2, ( "=> decrypt buf" ) );

    if( ssl->session_in == NULL || ssl->transform_in == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    mode = cipher_get_cipher_mode( &ssl->transform_in->cipher_ctx_dec );

    if( ssl->in_msglen < ssl->transform_in->minlen )
    {
        SSL_DEBUG_MSG( 1, ( "in_msglen (%d) < minlen (%d)",
                       ssl->in_msglen, ssl->transform_in->minlen ) );
        return( POLARSSL_ERR_SSL_INVALID_MAC );
    }

#if defined(POLARSSL_ARC4_C) || defined(POLARSSL_CIPHER_NULL_CIPHER)
    if( mode == POLARSSL_MODE_STREAM )
    {
        int ret;
        size_t olen = 0;

        padlen = 0;

        if( ( ret = cipher_crypt( &ssl->transform_in->cipher_ctx_dec,
                                   ssl->transform_in->iv_dec,
                                   ssl->transform_in->ivlen,
                                   ssl->in_msg, ssl->in_msglen,
                                   ssl->in_msg, &olen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_crypt", ret );
            return( ret );
        }

        if( ssl->in_msglen != olen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }
    }
    else
#endif /* POLARSSL_ARC4_C || POLARSSL_CIPHER_NULL_CIPHER */
#if defined(POLARSSL_GCM_C) || defined(POLARSSL_CCM_C)
    if( mode == POLARSSL_MODE_GCM ||
        mode == POLARSSL_MODE_CCM )
    {
        int ret;
        size_t dec_msglen, olen;
        unsigned char *dec_msg;
        unsigned char *dec_msg_result;
        unsigned char add_data[13];
        unsigned char taglen = ssl->transform_in->ciphersuite_info->flags &
                               POLARSSL_CIPHERSUITE_SHORT_TAG ? 8 : 16;
        unsigned char explicit_iv_len =  ssl->transform_in->ivlen -
                                         ssl->transform_in->fixed_ivlen;

        if( ssl->in_msglen < explicit_iv_len + taglen )
        {
            SSL_DEBUG_MSG( 1, ( "msglen (%d) < explicit_iv_len (%d) "
                                "+ taglen (%d)", ssl->in_msglen,
                                explicit_iv_len, taglen ) );
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }
        dec_msglen = ssl->in_msglen - explicit_iv_len - taglen;

        dec_msg = ssl->in_msg;
        dec_msg_result = ssl->in_msg;
        ssl->in_msglen = dec_msglen;

        memcpy( add_data, ssl->in_ctr, 8 );
        add_data[8]  = ssl->in_msgtype;
        add_data[9]  = ssl->major_ver;
        add_data[10] = ssl->minor_ver;
        add_data[11] = ( ssl->in_msglen >> 8 ) & 0xFF;
        add_data[12] = ssl->in_msglen & 0xFF;

        SSL_DEBUG_BUF( 4, "additional data used for AEAD",
                       add_data, 13 );

        memcpy( ssl->transform_in->iv_dec + ssl->transform_in->fixed_ivlen,
                ssl->in_iv,
                ssl->transform_in->ivlen - ssl->transform_in->fixed_ivlen );

        SSL_DEBUG_BUF( 4, "IV used", ssl->transform_in->iv_dec,
                                     ssl->transform_in->ivlen );
        SSL_DEBUG_BUF( 4, "TAG used", dec_msg + dec_msglen, taglen );

        /*
         * Decrypt and authenticate
         */
        if( ( ret = cipher_auth_decrypt( &ssl->transform_in->cipher_ctx_dec,
                                         ssl->transform_in->iv_dec,
                                         ssl->transform_in->ivlen,
                                         add_data, 13,
                                         dec_msg, dec_msglen,
                                         dec_msg_result, &olen,
                                         dec_msg + dec_msglen, taglen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_auth_decrypt", ret );

            if( ret == POLARSSL_ERR_CIPHER_AUTH_FAILED )
                return( POLARSSL_ERR_SSL_INVALID_MAC );

            return( ret );
        }
        auth_done++;

        if( olen != dec_msglen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }
    }
    else
#endif /* POLARSSL_GCM_C || POLARSSL_CCM_C */
#if defined(POLARSSL_CIPHER_MODE_CBC) &&                                    \
    ( defined(POLARSSL_AES_C) || defined(POLARSSL_CAMELLIA_C) )
    if( mode == POLARSSL_MODE_CBC )
    {
        /*
         * Decrypt and check the padding
         */
        int ret;
        unsigned char *dec_msg;
        unsigned char *dec_msg_result;
        size_t dec_msglen;
        size_t minlen = 0;
        size_t olen = 0;

        /*
         * Check immediate ciphertext sanity
         */
#if defined(POLARSSL_SSL_PROTO_TLS1_1) || defined(POLARSSL_SSL_PROTO_TLS1_2)
        if( ssl->minor_ver >= SSL_MINOR_VERSION_2 )
            minlen += ssl->transform_in->ivlen;
#endif

        if( ssl->in_msglen < minlen + ssl->transform_in->ivlen ||
            ssl->in_msglen < minlen + ssl->transform_in->maclen + 1 )
        {
            SSL_DEBUG_MSG( 1, ( "msglen (%d) < max( ivlen(%d), maclen (%d) "
                                "+ 1 ) ( + expl IV )", ssl->in_msglen,
                                ssl->transform_in->ivlen,
                                ssl->transform_in->maclen ) );
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }

        dec_msglen = ssl->in_msglen;
        dec_msg = ssl->in_msg;
        dec_msg_result = ssl->in_msg;

        /*
         * Authenticate before decrypt if enabled
         */
#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
        if( ssl->session_in->encrypt_then_mac == SSL_ETM_ENABLED )
        {
            unsigned char computed_mac[POLARSSL_SSL_MAX_MAC_SIZE];
            unsigned char pseudo_hdr[13];

            SSL_DEBUG_MSG( 3, ( "using encrypt then mac" ) );

            dec_msglen -= ssl->transform_in->maclen;
            ssl->in_msglen -= ssl->transform_in->maclen;

            memcpy( pseudo_hdr +  0, ssl->in_ctr, 8 );
            memcpy( pseudo_hdr +  8, ssl->in_hdr, 3 );
            pseudo_hdr[11] = (unsigned char)( ( ssl->in_msglen >> 8 ) & 0xFF );
            pseudo_hdr[12] = (unsigned char)( ( ssl->in_msglen      ) & 0xFF );

            SSL_DEBUG_BUF( 4, "MAC'd meta-data", pseudo_hdr, 13 );

            md_hmac_update( &ssl->transform_in->md_ctx_dec, pseudo_hdr, 13 );
            md_hmac_update( &ssl->transform_in->md_ctx_dec,
                             ssl->in_iv, ssl->in_msglen );
            md_hmac_finish( &ssl->transform_in->md_ctx_dec, computed_mac );
            md_hmac_reset( &ssl->transform_in->md_ctx_dec );

            SSL_DEBUG_BUF( 4, "message  mac", ssl->in_iv + ssl->in_msglen,
                                              ssl->transform_in->maclen );
            SSL_DEBUG_BUF( 4, "computed mac", computed_mac,
                                              ssl->transform_in->maclen );

            if( safer_memcmp( ssl->in_iv + ssl->in_msglen, computed_mac,
                              ssl->transform_in->maclen ) != 0 )
            {
                SSL_DEBUG_MSG( 1, ( "message mac does not match" ) );

                return( POLARSSL_ERR_SSL_INVALID_MAC );
            }
            auth_done++;
        }
#endif /* POLARSSL_SSL_ENCRYPT_THEN_MAC */

        /*
         * Check length sanity
         */
        if( ssl->in_msglen % ssl->transform_in->ivlen != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "msglen (%d) %% ivlen (%d) != 0",
                           ssl->in_msglen, ssl->transform_in->ivlen ) );
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }

#if defined(POLARSSL_SSL_PROTO_TLS1_1) || defined(POLARSSL_SSL_PROTO_TLS1_2)
        /*
         * Initialize for prepended IV for block cipher in TLS v1.1 and up
         */
        if( ssl->minor_ver >= SSL_MINOR_VERSION_2 )
        {
            dec_msglen -= ssl->transform_in->ivlen;
            ssl->in_msglen -= ssl->transform_in->ivlen;

            for( i = 0; i < ssl->transform_in->ivlen; i++ )
                ssl->transform_in->iv_dec[i] = ssl->in_iv[i];
        }
#endif /* POLARSSL_SSL_PROTO_TLS1_1 || POLARSSL_SSL_PROTO_TLS1_2 */

        if( ( ret = cipher_crypt( &ssl->transform_in->cipher_ctx_dec,
                                   ssl->transform_in->iv_dec,
                                   ssl->transform_in->ivlen,
                                   dec_msg, dec_msglen,
                                   dec_msg_result, &olen ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "cipher_crypt", ret );
            return( ret );
        }

        if( dec_msglen != olen )
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1)
        if( ssl->minor_ver < SSL_MINOR_VERSION_2 )
        {
            /*
             * Save IV in SSL3 and TLS1
             */
            memcpy( ssl->transform_in->iv_dec,
                    ssl->transform_in->cipher_ctx_dec.iv,
                    ssl->transform_in->ivlen );
        }
#endif

        padlen = 1 + ssl->in_msg[ssl->in_msglen - 1];

        if( ssl->in_msglen < ssl->transform_in->maclen + padlen &&
            auth_done == 0 )
        {
#if defined(POLARSSL_SSL_DEBUG_ALL)
            SSL_DEBUG_MSG( 1, ( "msglen (%d) < maclen (%d) + padlen (%d)",
                        ssl->in_msglen, ssl->transform_in->maclen, padlen ) );
#endif
            padlen = 0;
            correct = 0;
        }

#if defined(POLARSSL_SSL_PROTO_SSL3)
        if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
        {
            if( padlen > ssl->transform_in->ivlen )
            {
#if defined(POLARSSL_SSL_DEBUG_ALL)
                SSL_DEBUG_MSG( 1, ( "bad padding length: is %d, "
                                    "should be no more than %d",
                               padlen, ssl->transform_in->ivlen ) );
#endif
                correct = 0;
            }
        }
        else
#endif /* POLARSSL_SSL_PROTO_SSL3 */
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_2)
        if( ssl->minor_ver > SSL_MINOR_VERSION_0 )
        {
            /*
             * TLSv1+: always check the padding up to the first failure
             * and fake check up to 256 bytes of padding
             */
            size_t pad_count = 0, real_count = 1;
            size_t padding_idx = ssl->in_msglen - padlen - 1;

            /*
             * Padding is guaranteed to be incorrect if:
             *   1. padlen >= ssl->in_msglen
             *
             *   2. padding_idx >= SSL_MAX_CONTENT_LEN +
             *                     ssl->transform_in->maclen
             *
             * In both cases we reset padding_idx to a safe value (0) to
             * prevent out-of-buffer reads.
             */
            correct &= ( ssl->in_msglen >= padlen + 1 );
            correct &= ( padding_idx < SSL_MAX_CONTENT_LEN +
                                       ssl->transform_in->maclen );

            padding_idx *= correct;

            for( i = 1; i <= 256; i++ )
            {
                real_count &= ( i <= padlen );
                pad_count += real_count *
                             ( ssl->in_msg[padding_idx + i] == padlen - 1 );
            }

            correct &= ( pad_count == padlen ); /* Only 1 on correct padding */

#if defined(POLARSSL_SSL_DEBUG_ALL)
            if( padlen > 0 && correct == 0 )
                SSL_DEBUG_MSG( 1, ( "bad padding byte detected" ) );
#endif
            padlen &= correct * 0x1FF;
        }
        else
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 || \
          POLARSSL_SSL_PROTO_TLS1_2 */
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        ssl->in_msglen -= padlen;
    }
    else
#endif /* POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_CAMELLIA_C ) */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    SSL_DEBUG_BUF( 4, "raw buffer after decryption",
                   ssl->in_msg, ssl->in_msglen );

    /*
     * Authenticate if not done yet.
     * Compute the MAC regardless of the padding result (RFC4346, CBCTIME).
     */
#if defined(POLARSSL_SOME_MODES_USE_MAC)
    if( auth_done == 0 )
    {
        unsigned char tmp[POLARSSL_SSL_MAX_MAC_SIZE];

        ssl->in_msglen -= ssl->transform_in->maclen;

        ssl->in_hdr[3] = (unsigned char)( ssl->in_msglen >> 8 );
        ssl->in_hdr[4] = (unsigned char)( ssl->in_msglen      );

        memcpy( tmp, ssl->in_msg + ssl->in_msglen, ssl->transform_in->maclen );

#if defined(POLARSSL_SSL_PROTO_SSL3)
        if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
        {
            ssl_mac( &ssl->transform_in->md_ctx_dec,
                      ssl->transform_in->mac_dec,
                      ssl->in_msg, ssl->in_msglen,
                      ssl->in_ctr, ssl->in_msgtype );
        }
        else
#endif /* POLARSSL_SSL_PROTO_SSL3 */
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
        defined(POLARSSL_SSL_PROTO_TLS1_2)
        if( ssl->minor_ver > SSL_MINOR_VERSION_0 )
        {
            /*
             * Process MAC and always update for padlen afterwards to make
             * total time independent of padlen
             *
             * extra_run compensates MAC check for padlen
             *
             * Known timing attacks:
             *  - Lucky Thirteen (http://www.isg.rhul.ac.uk/tls/TLStiming.pdf)
             *
             * We use ( ( Lx + 8 ) / 64 ) to handle 'negative Lx' values
             * correctly. (We round down instead of up, so -56 is the correct
             * value for our calculations instead of -55)
             */
            size_t j, extra_run = 0;
            extra_run = ( 13 + ssl->in_msglen + padlen + 8 ) / 64 -
                        ( 13 + ssl->in_msglen          + 8 ) / 64;

            extra_run &= correct * 0xFF;

            md_hmac_update( &ssl->transform_in->md_ctx_dec, ssl->in_ctr, 13 );
            md_hmac_update( &ssl->transform_in->md_ctx_dec, ssl->in_msg,
                             ssl->in_msglen );
            md_hmac_finish( &ssl->transform_in->md_ctx_dec,
                             ssl->in_msg + ssl->in_msglen );
            for( j = 0; j < extra_run; j++ )
                md_process( &ssl->transform_in->md_ctx_dec, ssl->in_msg );

            md_hmac_reset( &ssl->transform_in->md_ctx_dec );
        }
        else
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 || \
              POLARSSL_SSL_PROTO_TLS1_2 */
        {
            SSL_DEBUG_MSG( 1, ( "should never happen" ) );
            return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
        }

        SSL_DEBUG_BUF( 4, "message  mac", tmp, ssl->transform_in->maclen );
        SSL_DEBUG_BUF( 4, "computed mac", ssl->in_msg + ssl->in_msglen,
                       ssl->transform_in->maclen );

        if( safer_memcmp( tmp, ssl->in_msg + ssl->in_msglen,
                         ssl->transform_in->maclen ) != 0 )
        {
#if defined(POLARSSL_SSL_DEBUG_ALL)
            SSL_DEBUG_MSG( 1, ( "message mac does not match" ) );
#endif
            correct = 0;
        }
        auth_done++;

        /*
         * Finally check the correct flag
         */
        if( correct == 0 )
            return( POLARSSL_ERR_SSL_INVALID_MAC );
    }
#endif /* POLARSSL_SOME_MODES_USE_MAC */

    /* Make extra sure authentication was performed, exactly once */
    if( auth_done != 1 )
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
    }

    if( ssl->in_msglen == 0 )
    {
        ssl->nb_zero++;

        /*
         * Three or more empty messages may be a DoS attack
         * (excessive CPU consumption).
         */
        if( ssl->nb_zero > 3 )
        {
            SSL_DEBUG_MSG( 1, ( "received four consecutive empty "
                                "messages, possible DoS attack" ) );
            return( POLARSSL_ERR_SSL_INVALID_MAC );
        }
    }
    else
        ssl->nb_zero = 0;

    for( i = 8; i > 0; i-- )
        if( ++ssl->in_ctr[i - 1] != 0 )
            break;

    /* The loops goes to its end iff the counter is wrapping */
    if( i == 0 )
    {
        SSL_DEBUG_MSG( 1, ( "incoming message counter would wrap" ) );
        return( POLARSSL_ERR_SSL_COUNTER_WRAPPING );
    }

    SSL_DEBUG_MSG( 2, ( "<= decrypt buf" ) );

    return( 0 );
}

#undef MAC_NONE
#undef MAC_PLAINTEXT
#undef MAC_CIPHERTEXT

#if defined(POLARSSL_ZLIB_SUPPORT)
/*
 * Compression/decompression functions
 */
static int ssl_compress_buf( ssl_context *ssl )
{
    int ret;
    unsigned char *msg_post = ssl->out_msg;
    size_t len_pre = ssl->out_msglen;
    unsigned char *msg_pre = ssl->compress_buf;

    SSL_DEBUG_MSG( 2, ( "=> compress buf" ) );

    if( len_pre == 0 )
        return( 0 );

    memcpy( msg_pre, ssl->out_msg, len_pre );

    SSL_DEBUG_MSG( 3, ( "before compression: msglen = %d, ",
                   ssl->out_msglen ) );

    SSL_DEBUG_BUF( 4, "before compression: output payload",
                   ssl->out_msg, ssl->out_msglen );

    ssl->transform_out->ctx_deflate.next_in = msg_pre;
    ssl->transform_out->ctx_deflate.avail_in = len_pre;
    ssl->transform_out->ctx_deflate.next_out = msg_post;
    ssl->transform_out->ctx_deflate.avail_out = SSL_BUFFER_LEN;

    ret = deflate( &ssl->transform_out->ctx_deflate, Z_SYNC_FLUSH );
    if( ret != Z_OK )
    {
        SSL_DEBUG_MSG( 1, ( "failed to perform compression (%d)", ret ) );
        return( POLARSSL_ERR_SSL_COMPRESSION_FAILED );
    }

    ssl->out_msglen = SSL_BUFFER_LEN -
                      ssl->transform_out->ctx_deflate.avail_out;

    SSL_DEBUG_MSG( 3, ( "after compression: msglen = %d, ",
                   ssl->out_msglen ) );

    SSL_DEBUG_BUF( 4, "after compression: output payload",
                   ssl->out_msg, ssl->out_msglen );

    SSL_DEBUG_MSG( 2, ( "<= compress buf" ) );

    return( 0 );
}

static int ssl_decompress_buf( ssl_context *ssl )
{
    int ret;
    unsigned char *msg_post = ssl->in_msg;
    size_t len_pre = ssl->in_msglen;
    unsigned char *msg_pre = ssl->compress_buf;

    SSL_DEBUG_MSG( 2, ( "=> decompress buf" ) );

    if( len_pre == 0 )
        return( 0 );

    memcpy( msg_pre, ssl->in_msg, len_pre );

    SSL_DEBUG_MSG( 3, ( "before decompression: msglen = %d, ",
                   ssl->in_msglen ) );

    SSL_DEBUG_BUF( 4, "before decompression: input payload",
                   ssl->in_msg, ssl->in_msglen );

    ssl->transform_in->ctx_inflate.next_in = msg_pre;
    ssl->transform_in->ctx_inflate.avail_in = len_pre;
    ssl->transform_in->ctx_inflate.next_out = msg_post;
    ssl->transform_in->ctx_inflate.avail_out = SSL_MAX_CONTENT_LEN;

    ret = inflate( &ssl->transform_in->ctx_inflate, Z_SYNC_FLUSH );
    if( ret != Z_OK )
    {
        SSL_DEBUG_MSG( 1, ( "failed to perform decompression (%d)", ret ) );
        return( POLARSSL_ERR_SSL_COMPRESSION_FAILED );
    }

    ssl->in_msglen = SSL_MAX_CONTENT_LEN -
                     ssl->transform_in->ctx_inflate.avail_out;

    SSL_DEBUG_MSG( 3, ( "after decompression: msglen = %d, ",
                   ssl->in_msglen ) );

    SSL_DEBUG_BUF( 4, "after decompression: input payload",
                   ssl->in_msg, ssl->in_msglen );

    SSL_DEBUG_MSG( 2, ( "<= decompress buf" ) );

    return( 0 );
}
#endif /* POLARSSL_ZLIB_SUPPORT */

/*
 * Fill the input message buffer
 */
int ssl_fetch_input( ssl_context *ssl, size_t nb_want )
{
    int ret;
    size_t len;

    SSL_DEBUG_MSG( 2, ( "=> fetch input" ) );

    if( nb_want > SSL_BUFFER_LEN - 8 )
    {
        SSL_DEBUG_MSG( 1, ( "requesting more data than fits" ) );
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    while( ssl->in_left < nb_want )
    {
        len = nb_want - ssl->in_left;
        ret = ssl->f_recv( ssl->p_recv, ssl->in_hdr + ssl->in_left, len );

        SSL_DEBUG_MSG( 2, ( "in_left: %d, nb_want: %d",
                       ssl->in_left, nb_want ) );
        SSL_DEBUG_RET( 2, "ssl->f_recv", ret );

        if( ret == 0 )
            return( POLARSSL_ERR_SSL_CONN_EOF );

        if( ret < 0 )
            return( ret );

        ssl->in_left += ret;
    }

    SSL_DEBUG_MSG( 2, ( "<= fetch input" ) );

    return( 0 );
}

/*
 * Flush any data not yet written
 */
int ssl_flush_output( ssl_context *ssl )
{
    int ret;
    unsigned char *buf;

    SSL_DEBUG_MSG( 2, ( "=> flush output" ) );

    while( ssl->out_left > 0 )
    {
        SSL_DEBUG_MSG( 2, ( "message length: %d, out_left: %d",
                       5 + ssl->out_msglen, ssl->out_left ) );

        buf = ssl->out_hdr + 5 + ssl->out_msglen - ssl->out_left;
        ret = ssl->f_send( ssl->p_send, buf, ssl->out_left );

        SSL_DEBUG_RET( 2, "ssl->f_send", ret );

        if( ret <= 0 )
            return( ret );

        ssl->out_left -= ret;
    }

    SSL_DEBUG_MSG( 2, ( "<= flush output" ) );

    return( 0 );
}

/*
 * Record layer functions
 */
int ssl_write_record( ssl_context *ssl )
{
    int ret, done = 0;
    size_t len = ssl->out_msglen;

    SSL_DEBUG_MSG( 2, ( "=> write record" ) );

    if( ssl->out_msgtype == SSL_MSG_HANDSHAKE )
    {
        ssl->out_msg[1] = (unsigned char)( ( len - 4 ) >> 16 );
        ssl->out_msg[2] = (unsigned char)( ( len - 4 ) >>  8 );
        ssl->out_msg[3] = (unsigned char)( ( len - 4 )       );

        if( ssl->out_msg[0] != SSL_HS_HELLO_REQUEST )
            ssl->handshake->update_checksum( ssl, ssl->out_msg, len );
    }

#if defined(POLARSSL_ZLIB_SUPPORT)
    if( ssl->transform_out != NULL &&
        ssl->session_out->compression == SSL_COMPRESS_DEFLATE )
    {
        if( ( ret = ssl_compress_buf( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_compress_buf", ret );
            return( ret );
        }

        len = ssl->out_msglen;
    }
#endif /*POLARSSL_ZLIB_SUPPORT */

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_write != NULL )
    {
        SSL_DEBUG_MSG( 2, ( "going for ssl_hw_record_write()" ) );

        ret = ssl_hw_record_write( ssl );
        if( ret != 0 && ret != POLARSSL_ERR_SSL_HW_ACCEL_FALLTHROUGH )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_write", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }

        if( ret == 0 )
            done = 1;
    }
#endif /* POLARSSL_SSL_HW_RECORD_ACCEL */
    if( !done )
    {
        ssl->out_hdr[0] = (unsigned char) ssl->out_msgtype;
        ssl->out_hdr[1] = (unsigned char) ssl->major_ver;
        ssl->out_hdr[2] = (unsigned char) ssl->minor_ver;
        ssl->out_hdr[3] = (unsigned char)( len >> 8 );
        ssl->out_hdr[4] = (unsigned char)( len      );

        if( ssl->transform_out != NULL )
        {
            if( ( ret = ssl_encrypt_buf( ssl ) ) != 0 )
            {
                SSL_DEBUG_RET( 1, "ssl_encrypt_buf", ret );
                return( ret );
            }

            len = ssl->out_msglen;
            ssl->out_hdr[3] = (unsigned char)( len >> 8 );
            ssl->out_hdr[4] = (unsigned char)( len      );
        }

        ssl->out_left = 5 + ssl->out_msglen;

        SSL_DEBUG_MSG( 3, ( "output record: msgtype = %d, "
                            "version = [%d:%d], msglen = %d",
                       ssl->out_hdr[0], ssl->out_hdr[1], ssl->out_hdr[2],
                     ( ssl->out_hdr[3] << 8 ) | ssl->out_hdr[4] ) );

        SSL_DEBUG_BUF( 4, "output record sent to network",
                       ssl->out_hdr, 5 + ssl->out_msglen );
    }

    if( ( ret = ssl_flush_output( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_flush_output", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= write record" ) );

    return( 0 );
}

int ssl_read_record( ssl_context *ssl )
{
    int ret, done = 0;

    SSL_DEBUG_MSG( 2, ( "=> read record" ) );

    if( ssl->in_hslen != 0 &&
        ssl->in_hslen < ssl->in_msglen )
    {
        /*
         * Get next Handshake message in the current record
         */
        ssl->in_msglen -= ssl->in_hslen;

        memmove( ssl->in_msg, ssl->in_msg + ssl->in_hslen,
                 ssl->in_msglen );

        ssl->in_hslen  = 4;
        ssl->in_hslen += ( ssl->in_msg[2] << 8 ) | ssl->in_msg[3];

        SSL_DEBUG_MSG( 3, ( "handshake message: msglen ="
                            " %d, type = %d, hslen = %d",
                       ssl->in_msglen, ssl->in_msg[0], ssl->in_hslen ) );

        if( ssl->in_msglen < 4 || ssl->in_msg[1] != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "bad handshake length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->in_msglen < ssl->in_hslen )
        {
            SSL_DEBUG_MSG( 1, ( "bad handshake length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->state != SSL_HANDSHAKE_OVER )
            ssl->handshake->update_checksum( ssl, ssl->in_msg, ssl->in_hslen );

        return( 0 );
    }

    ssl->in_hslen = 0;

    /*
     * Read the record header and validate it
     */
    if( ( ret = ssl_fetch_input( ssl, 5 ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_fetch_input", ret );
        return( ret );
    }

    ssl->in_msgtype =  ssl->in_hdr[0];
    ssl->in_msglen = ( ssl->in_hdr[3] << 8 ) | ssl->in_hdr[4];

    SSL_DEBUG_MSG( 3, ( "input record: msgtype = %d, "
                        "version = [%d:%d], msglen = %d",
                     ssl->in_hdr[0], ssl->in_hdr[1], ssl->in_hdr[2],
                   ( ssl->in_hdr[3] << 8 ) | ssl->in_hdr[4] ) );

    if( ssl->in_hdr[1] != ssl->major_ver )
    {
        SSL_DEBUG_MSG( 1, ( "major version mismatch" ) );
        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    if( ssl->in_hdr[2] > ssl->max_minor_ver )
    {
        SSL_DEBUG_MSG( 1, ( "minor version mismatch" ) );
        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    /* Sanity check (outer boundaries) */
    if( ssl->in_msglen < 1 || ssl->in_msglen > SSL_BUFFER_LEN - 13 )
    {
        SSL_DEBUG_MSG( 1, ( "bad message length" ) );
        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    /*
     * Make sure the message length is acceptable for the current transform
     * and protocol version.
     */
    if( ssl->transform_in == NULL )
    {
        if( ssl->in_msglen > SSL_MAX_CONTENT_LEN )
        {
            SSL_DEBUG_MSG( 1, ( "bad message length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
    }
    else
    {
        if( ssl->in_msglen < ssl->transform_in->minlen )
        {
            SSL_DEBUG_MSG( 1, ( "bad message length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

#if defined(POLARSSL_SSL_PROTO_SSL3)
        if( ssl->minor_ver == SSL_MINOR_VERSION_0 &&
            ssl->in_msglen > ssl->transform_in->minlen + SSL_MAX_CONTENT_LEN )
        {
            SSL_DEBUG_MSG( 1, ( "bad message length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
#endif

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_2)
        /*
         * TLS encrypted messages can have up to 256 bytes of padding
         */
        if( ssl->minor_ver >= SSL_MINOR_VERSION_1 &&
            ssl->in_msglen > ssl->transform_in->minlen +
                             SSL_MAX_CONTENT_LEN + 256 )
        {
            SSL_DEBUG_MSG( 1, ( "bad message length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
#endif
    }

    /*
     * Read and optionally decrypt the message contents
     */
    if( ( ret = ssl_fetch_input( ssl, 5 + ssl->in_msglen ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_fetch_input", ret );
        return( ret );
    }

    SSL_DEBUG_BUF( 4, "input record from network",
                   ssl->in_hdr, 5 + ssl->in_msglen );

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_read != NULL )
    {
        SSL_DEBUG_MSG( 2, ( "going for ssl_hw_record_read()" ) );

        ret = ssl_hw_record_read( ssl );
        if( ret != 0 && ret != POLARSSL_ERR_SSL_HW_ACCEL_FALLTHROUGH )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_read", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }

        if( ret == 0 )
            done = 1;
    }
#endif /* POLARSSL_SSL_HW_RECORD_ACCEL */
    if( !done && ssl->transform_in != NULL )
    {
        if( ( ret = ssl_decrypt_buf( ssl ) ) != 0 )
        {
#if defined(POLARSSL_SSL_ALERT_MESSAGES)
            if( ret == POLARSSL_ERR_SSL_INVALID_MAC )
            {
                ssl_send_alert_message( ssl,
                                        SSL_ALERT_LEVEL_FATAL,
                                        SSL_ALERT_MSG_BAD_RECORD_MAC );
            }
#endif
            SSL_DEBUG_RET( 1, "ssl_decrypt_buf", ret );
            return( ret );
        }

        SSL_DEBUG_BUF( 4, "input payload after decrypt",
                       ssl->in_msg, ssl->in_msglen );

        if( ssl->in_msglen > SSL_MAX_CONTENT_LEN )
        {
            SSL_DEBUG_MSG( 1, ( "bad message length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }
    }

#if defined(POLARSSL_ZLIB_SUPPORT)
    if( ssl->transform_in != NULL &&
        ssl->session_in->compression == SSL_COMPRESS_DEFLATE )
    {
        if( ( ret = ssl_decompress_buf( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_decompress_buf", ret );
            return( ret );
        }

        ssl->in_hdr[3] = (unsigned char)( ssl->in_msglen >> 8 );
        ssl->in_hdr[4] = (unsigned char)( ssl->in_msglen      );
    }
#endif /* POLARSSL_ZLIB_SUPPORT */

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE &&
        ssl->in_msgtype != SSL_MSG_ALERT &&
        ssl->in_msgtype != SSL_MSG_CHANGE_CIPHER_SPEC &&
        ssl->in_msgtype != SSL_MSG_APPLICATION_DATA )
    {
        SSL_DEBUG_MSG( 1, ( "unknown record type" ) );

        if( ( ret = ssl_send_alert_message( ssl,
                        SSL_ALERT_LEVEL_FATAL,
                        SSL_ALERT_MSG_UNEXPECTED_MESSAGE ) ) != 0 )
        {
            return( ret );
        }

        return( POLARSSL_ERR_SSL_INVALID_RECORD );
    }

    if( ssl->in_msgtype == SSL_MSG_HANDSHAKE )
    {
        ssl->in_hslen  = 4;
        ssl->in_hslen += ( ssl->in_msg[2] << 8 ) | ssl->in_msg[3];

        SSL_DEBUG_MSG( 3, ( "handshake message: msglen ="
                            " %d, type = %d, hslen = %d",
                       ssl->in_msglen, ssl->in_msg[0], ssl->in_hslen ) );

        /*
         * Additional checks to validate the handshake header
         */
        if( ssl->in_msglen < 4 || ssl->in_msg[1] != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "bad handshake length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->in_msglen < ssl->in_hslen )
        {
            SSL_DEBUG_MSG( 1, ( "bad handshake length" ) );
            return( POLARSSL_ERR_SSL_INVALID_RECORD );
        }

        if( ssl->state != SSL_HANDSHAKE_OVER )
            ssl->handshake->update_checksum( ssl, ssl->in_msg, ssl->in_hslen );
    }

    if( ssl->in_msgtype == SSL_MSG_ALERT )
    {
        SSL_DEBUG_MSG( 2, ( "got an alert message, type: [%d:%d]",
                       ssl->in_msg[0], ssl->in_msg[1] ) );

        /*
         * Ignore non-fatal alerts, except close_notify
         */
        if( ssl->in_msg[0] == SSL_ALERT_LEVEL_FATAL )
        {
            SSL_DEBUG_MSG( 1, ( "is a fatal alert message (msg %d)",
                           ssl->in_msg[1] ) );
            return( POLARSSL_ERR_SSL_FATAL_ALERT_MESSAGE );
        }

        if( ssl->in_msg[0] == SSL_ALERT_LEVEL_WARNING &&
            ssl->in_msg[1] == SSL_ALERT_MSG_CLOSE_NOTIFY )
        {
            SSL_DEBUG_MSG( 2, ( "is a close notify message" ) );
            return( POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY );
        }
    }

    ssl->in_left = 0;

    SSL_DEBUG_MSG( 2, ( "<= read record" ) );

    return( 0 );
}

int ssl_send_fatal_handshake_failure( ssl_context *ssl )
{
    int ret;

    if( ( ret = ssl_send_alert_message( ssl,
                    SSL_ALERT_LEVEL_FATAL,
                    SSL_ALERT_MSG_HANDSHAKE_FAILURE ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int ssl_send_alert_message( ssl_context *ssl,
                            unsigned char level,
                            unsigned char message )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> send alert message" ) );

    ssl->out_msgtype = SSL_MSG_ALERT;
    ssl->out_msglen = 2;
    ssl->out_msg[0] = level;
    ssl->out_msg[1] = message;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_write_record", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= send alert message" ) );

    return( 0 );
}

/*
 * Handshake functions
 */
#if !defined(POLARSSL_KEY_EXCHANGE_RSA_ENABLED)         && \
    !defined(POLARSSL_KEY_EXCHANGE_RSA_PSK_ENABLED)     && \
    !defined(POLARSSL_KEY_EXCHANGE_DHE_RSA_ENABLED)     && \
    !defined(POLARSSL_KEY_EXCHANGE_ECDHE_RSA_ENABLED)   && \
    !defined(POLARSSL_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED) && \
    !defined(POLARSSL_KEY_EXCHANGE_ECDH_RSA_ENABLED)    && \
    !defined(POLARSSL_KEY_EXCHANGE_ECDH_ECDSA_ENABLED)
int ssl_write_certificate( ssl_context *ssl )
{
    const ssl_ciphersuite_t *ciphersuite_info = ssl->transform_negotiate->ciphersuite_info;

    SSL_DEBUG_MSG( 2, ( "=> write certificate" ) );

    if( ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_DHE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_ECDHE_PSK )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip write certificate" ) );
        ssl->state++;
        return( 0 );
    }

    SSL_DEBUG_MSG( 1, ( "should never happen" ) );
    return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
}

int ssl_parse_certificate( ssl_context *ssl )
{
    const ssl_ciphersuite_t *ciphersuite_info = ssl->transform_negotiate->ciphersuite_info;

    SSL_DEBUG_MSG( 2, ( "=> parse certificate" ) );

    if( ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_DHE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_ECDHE_PSK )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip parse certificate" ) );
        ssl->state++;
        return( 0 );
    }

    SSL_DEBUG_MSG( 1, ( "should never happen" ) );
    return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
}
#else
int ssl_write_certificate( ssl_context *ssl )
{
    int ret = POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE;
    size_t i, n;
    const x509_crt *crt;
    const ssl_ciphersuite_t *ciphersuite_info = ssl->transform_negotiate->ciphersuite_info;

    SSL_DEBUG_MSG( 2, ( "=> write certificate" ) );

    if( ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_DHE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_ECDHE_PSK )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip write certificate" ) );
        ssl->state++;
        return( 0 );
    }

#if defined(POLARSSL_SSL_CLI_C)
    if( ssl->endpoint == SSL_IS_CLIENT )
    {
        if( ssl->client_auth == 0 )
        {
            SSL_DEBUG_MSG( 2, ( "<= skip write certificate" ) );
            ssl->state++;
            return( 0 );
        }

#if defined(POLARSSL_SSL_PROTO_SSL3)
        /*
         * If using SSLv3 and got no cert, send an Alert message
         * (otherwise an empty Certificate message will be sent).
         */
        if( ssl_own_cert( ssl )  == NULL &&
            ssl->minor_ver == SSL_MINOR_VERSION_0 )
        {
            ssl->out_msglen  = 2;
            ssl->out_msgtype = SSL_MSG_ALERT;
            ssl->out_msg[0]  = SSL_ALERT_LEVEL_WARNING;
            ssl->out_msg[1]  = SSL_ALERT_MSG_NO_CERT;

            SSL_DEBUG_MSG( 2, ( "got no certificate to send" ) );
            goto write_msg;
        }
#endif /* POLARSSL_SSL_PROTO_SSL3 */
    }
#endif /* POLARSSL_SSL_CLI_C */
#if defined(POLARSSL_SSL_SRV_C)
    if( ssl->endpoint == SSL_IS_SERVER )
    {
        if( ssl_own_cert( ssl ) == NULL )
        {
            SSL_DEBUG_MSG( 1, ( "got no certificate to send" ) );
            return( POLARSSL_ERR_SSL_CERTIFICATE_REQUIRED );
        }
    }
#endif

    SSL_DEBUG_CRT( 3, "own certificate", ssl_own_cert( ssl ) );

    /*
     *     0  .  0    handshake type
     *     1  .  3    handshake length
     *     4  .  6    length of all certs
     *     7  .  9    length of cert. 1
     *    10  . n-1   peer certificate
     *     n  . n+2   length of cert. 2
     *    n+3 . ...   upper level cert, etc.
     */
    i = 7;
    crt = ssl_own_cert( ssl );

    while( crt != NULL )
    {
        n = crt->raw.len;
        if( n > SSL_MAX_CONTENT_LEN - 3 - i )
        {
            SSL_DEBUG_MSG( 1, ( "certificate too large, %d > %d",
                           i + 3 + n, SSL_MAX_CONTENT_LEN ) );
            return( POLARSSL_ERR_SSL_CERTIFICATE_TOO_LARGE );
        }

        ssl->out_msg[i    ] = (unsigned char)( n >> 16 );
        ssl->out_msg[i + 1] = (unsigned char)( n >>  8 );
        ssl->out_msg[i + 2] = (unsigned char)( n       );

        i += 3; memcpy( ssl->out_msg + i, crt->raw.p, n );
        i += n; crt = crt->next;
    }

    ssl->out_msg[4]  = (unsigned char)( ( i - 7 ) >> 16 );
    ssl->out_msg[5]  = (unsigned char)( ( i - 7 ) >>  8 );
    ssl->out_msg[6]  = (unsigned char)( ( i - 7 )       );

    ssl->out_msglen  = i;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_CERTIFICATE;

#if defined(POLARSSL_SSL_PROTO_SSL3)
write_msg:
#endif

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_write_record", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= write certificate" ) );

    return( ret );
}

int ssl_parse_certificate( ssl_context *ssl )
{
    int ret = POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE;
    size_t i, n;
    const ssl_ciphersuite_t *ciphersuite_info = ssl->transform_negotiate->ciphersuite_info;

    SSL_DEBUG_MSG( 2, ( "=> parse certificate" ) );

    if( ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_DHE_PSK ||
        ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_ECDHE_PSK )
    {
        SSL_DEBUG_MSG( 2, ( "<= skip parse certificate" ) );
        ssl->state++;
        return( 0 );
    }

#if defined(POLARSSL_SSL_SRV_C)
    if( ssl->endpoint == SSL_IS_SERVER &&
        ( ssl->authmode == SSL_VERIFY_NONE ||
          ciphersuite_info->key_exchange == POLARSSL_KEY_EXCHANGE_RSA_PSK ) )
    {
        ssl->session_negotiate->verify_result = BADCERT_SKIP_VERIFY;
        SSL_DEBUG_MSG( 2, ( "<= skip parse certificate" ) );
        ssl->state++;
        return( 0 );
    }
#endif

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    ssl->state++;

#if defined(POLARSSL_SSL_SRV_C)
#if defined(POLARSSL_SSL_PROTO_SSL3)
    /*
     * Check if the client sent an empty certificate
     */
    if( ssl->endpoint  == SSL_IS_SERVER &&
        ssl->minor_ver == SSL_MINOR_VERSION_0 )
    {
        if( ssl->in_msglen  == 2                        &&
            ssl->in_msgtype == SSL_MSG_ALERT            &&
            ssl->in_msg[0]  == SSL_ALERT_LEVEL_WARNING  &&
            ssl->in_msg[1]  == SSL_ALERT_MSG_NO_CERT )
        {
            SSL_DEBUG_MSG( 1, ( "SSLv3 client has no certificate" ) );

            ssl->session_negotiate->verify_result = BADCERT_MISSING;
            if( ssl->authmode == SSL_VERIFY_OPTIONAL )
                return( 0 );
            else
                return( POLARSSL_ERR_SSL_NO_CLIENT_CERTIFICATE );
        }
    }
#endif /* POLARSSL_SSL_PROTO_SSL3 */

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_2)
    if( ssl->endpoint  == SSL_IS_SERVER &&
        ssl->minor_ver != SSL_MINOR_VERSION_0 )
    {
        if( ssl->in_hslen   == 7                    &&
            ssl->in_msgtype == SSL_MSG_HANDSHAKE    &&
            ssl->in_msg[0]  == SSL_HS_CERTIFICATE   &&
            memcmp( ssl->in_msg + 4, "\0\0\0", 3 ) == 0 )
        {
            SSL_DEBUG_MSG( 1, ( "TLSv1 client has no certificate" ) );

            ssl->session_negotiate->verify_result = BADCERT_MISSING;
            if( ssl->authmode == SSL_VERIFY_REQUIRED )
                return( POLARSSL_ERR_SSL_NO_CLIENT_CERTIFICATE );
            else
                return( 0 );
        }
    }
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 || \
          POLARSSL_SSL_PROTO_TLS1_2 */
#endif /* POLARSSL_SSL_SRV_C */

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate message" ) );
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_msg[0] != SSL_HS_CERTIFICATE || ssl->in_hslen < 10 )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
    }

    /*
     * Same message structure as in ssl_write_certificate()
     */
    n = ( ssl->in_msg[5] << 8 ) | ssl->in_msg[6];

    if( ssl->in_msg[4] != 0 || ssl->in_hslen != 7 + n )
    {
        SSL_DEBUG_MSG( 1, ( "bad certificate message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
    }

    /* In case we tried to reuse a session but it failed */
    if( ssl->session_negotiate->peer_cert != NULL )
    {
        x509_crt_free( ssl->session_negotiate->peer_cert );
        polarssl_free( ssl->session_negotiate->peer_cert );
    }

    if( ( ssl->session_negotiate->peer_cert = (x509_crt *) polarssl_malloc(
                    sizeof( x509_crt ) ) ) == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "malloc(%d bytes) failed",
                       sizeof( x509_crt ) ) );
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );
    }

    x509_crt_init( ssl->session_negotiate->peer_cert );

    i = 7;

    while( i < ssl->in_hslen )
    {
        if( ssl->in_msg[i] != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "bad certificate message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }

        n = ( (unsigned int) ssl->in_msg[i + 1] << 8 )
            | (unsigned int) ssl->in_msg[i + 2];
        i += 3;

        if( n < 128 || i + n > ssl->in_hslen )
        {
            SSL_DEBUG_MSG( 1, ( "bad certificate message" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }

        ret = x509_crt_parse_der( ssl->session_negotiate->peer_cert,
                                  ssl->in_msg + i, n );
        if( ret != 0 )
        {
            SSL_DEBUG_RET( 1, " x509_crt_parse_der", ret );
            return( ret );
        }

        i += n;
    }

    SSL_DEBUG_CRT( 3, "peer certificate", ssl->session_negotiate->peer_cert );

    /*
     * On client, make sure the server cert doesn't change during renego to
     * avoid "triple handshake" attack: https://secure-resumption.com/
     */
#if defined(POLARSSL_SSL_RENEGOTIATION) && defined(POLARSSL_SSL_CLI_C)
    if( ssl->endpoint == SSL_IS_CLIENT &&
        ssl->renegotiation == SSL_RENEGOTIATION )
    {
        if( ssl->session->peer_cert == NULL )
        {
            SSL_DEBUG_MSG( 1, ( "new server cert during renegotiation" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }

        if( ssl->session->peer_cert->raw.len !=
            ssl->session_negotiate->peer_cert->raw.len ||
            memcmp( ssl->session->peer_cert->raw.p,
                    ssl->session_negotiate->peer_cert->raw.p,
                    ssl->session->peer_cert->raw.len ) != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "server cert changed during renegotiation" ) );
            return( POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE );
        }
    }
#endif /* POLARSSL_SSL_RENEGOTIATION && POLARSSL_SSL_CLI_C */

    if( ssl->authmode != SSL_VERIFY_NONE )
    {
        if( ssl->ca_chain == NULL )
        {
            SSL_DEBUG_MSG( 1, ( "got no CA chain" ) );
            return( POLARSSL_ERR_SSL_CA_CHAIN_REQUIRED );
        }

        /*
         * Main check: verify certificate
         */
        ret = x509_crt_verify( ssl->session_negotiate->peer_cert,
                               ssl->ca_chain, ssl->ca_crl, ssl->peer_cn,
                              &ssl->session_negotiate->verify_result,
                               ssl->f_vrfy, ssl->p_vrfy );

        if( ret != 0 )
        {
            SSL_DEBUG_RET( 1, "x509_verify_cert", ret );
        }

        /*
         * Secondary checks: always done, but change 'ret' only if it was 0
         */

#if defined(POLARSSL_SSL_SET_CURVES)
        {
            pk_context *pk = &ssl->session_negotiate->peer_cert->pk;

            /* If certificate uses an EC key, make sure the curve is OK */
            if( pk_can_do( pk, POLARSSL_PK_ECKEY ) &&
                ! ssl_curve_is_acceptable( ssl, pk_ec( *pk )->grp.id ) )
            {
                SSL_DEBUG_MSG( 1, ( "bad certificate (EC key curve)" ) );
                if( ret == 0 )
                    ret = POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE;
            }
        }
#endif /* POLARSSL_SSL_SET_CURVES */

        if( ssl_check_cert_usage( ssl->session_negotiate->peer_cert,
                                  ciphersuite_info,
                                  ! ssl->endpoint ) != 0 )
        {
            SSL_DEBUG_MSG( 1, ( "bad certificate (usage extensions)" ) );
            if( ret == 0 )
                ret = POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE;
        }

        if( ssl->authmode != SSL_VERIFY_REQUIRED )
            ret = 0;
    }

    SSL_DEBUG_MSG( 2, ( "<= parse certificate" ) );

    return( ret );
}
#endif /* !POLARSSL_KEY_EXCHANGE_RSA_ENABLED
          !POLARSSL_KEY_EXCHANGE_RSA_PSK_ENABLED
          !POLARSSL_KEY_EXCHANGE_DHE_RSA_ENABLED
          !POLARSSL_KEY_EXCHANGE_ECDHE_RSA_ENABLED
          !POLARSSL_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
          !POLARSSL_KEY_EXCHANGE_ECDH_RSA_ENABLED
          !POLARSSL_KEY_EXCHANGE_ECDH_ECDSA_ENABLED */

int ssl_write_change_cipher_spec( ssl_context *ssl )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> write change cipher spec" ) );

    ssl->out_msgtype = SSL_MSG_CHANGE_CIPHER_SPEC;
    ssl->out_msglen  = 1;
    ssl->out_msg[0]  = 1;

    ssl->state++;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_write_record", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= write change cipher spec" ) );

    return( 0 );
}

int ssl_parse_change_cipher_spec( ssl_context *ssl )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> parse change cipher spec" ) );

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_CHANGE_CIPHER_SPEC )
    {
        SSL_DEBUG_MSG( 1, ( "bad change cipher spec message" ) );
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    if( ssl->in_msglen != 1 || ssl->in_msg[0] != 1 )
    {
        SSL_DEBUG_MSG( 1, ( "bad change cipher spec message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC );
    }

    ssl->state++;

    SSL_DEBUG_MSG( 2, ( "<= parse change cipher spec" ) );

    return( 0 );
}

void ssl_optimize_checksum( ssl_context *ssl,
                            const ssl_ciphersuite_t *ciphersuite_info )
{
    ((void) ciphersuite_info);

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_1)
    if( ssl->minor_ver < SSL_MINOR_VERSION_3 )
        ssl->handshake->update_checksum = ssl_update_checksum_md5sha1;
    else
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA512_C)
    if( ciphersuite_info->mac == POLARSSL_MD_SHA384 )
        ssl->handshake->update_checksum = ssl_update_checksum_sha384;
    else
#endif
#if defined(POLARSSL_SHA256_C)
    if( ciphersuite_info->mac != POLARSSL_MD_SHA384 )
        ssl->handshake->update_checksum = ssl_update_checksum_sha256;
    else
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */
    {
        SSL_DEBUG_MSG( 1, ( "should never happen" ) );
        return;
    }
}

static void ssl_update_checksum_start( ssl_context *ssl,
                                       const unsigned char *buf, size_t len )
{
#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_1)
     md5_update( &ssl->handshake->fin_md5 , buf, len );
    sha1_update( &ssl->handshake->fin_sha1, buf, len );
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
    sha256_update( &ssl->handshake->fin_sha256, buf, len );
#endif
#if defined(POLARSSL_SHA512_C)
    sha512_update( &ssl->handshake->fin_sha512, buf, len );
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */
}

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_1)
static void ssl_update_checksum_md5sha1( ssl_context *ssl,
                                         const unsigned char *buf, size_t len )
{
     md5_update( &ssl->handshake->fin_md5 , buf, len );
    sha1_update( &ssl->handshake->fin_sha1, buf, len );
}
#endif

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
static void ssl_update_checksum_sha256( ssl_context *ssl,
                                        const unsigned char *buf, size_t len )
{
    sha256_update( &ssl->handshake->fin_sha256, buf, len );
}
#endif

#if defined(POLARSSL_SHA512_C)
static void ssl_update_checksum_sha384( ssl_context *ssl,
                                        const unsigned char *buf, size_t len )
{
    sha512_update( &ssl->handshake->fin_sha512, buf, len );
}
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

#if defined(POLARSSL_SSL_PROTO_SSL3)
static void ssl_calc_finished_ssl(
                ssl_context *ssl, unsigned char *buf, int from )
{
    const char *sender;
    md5_context  md5;
    sha1_context sha1;

    unsigned char padbuf[48];
    unsigned char md5sum[16];
    unsigned char sha1sum[20];

    ssl_session *session = ssl->session_negotiate;
    if( !session )
        session = ssl->session;

    SSL_DEBUG_MSG( 2, ( "=> calc  finished ssl" ) );

    memcpy( &md5 , &ssl->handshake->fin_md5 , sizeof(md5_context)  );
    memcpy( &sha1, &ssl->handshake->fin_sha1, sizeof(sha1_context) );

    /*
     * SSLv3:
     *   hash =
     *      MD5( master + pad2 +
     *          MD5( handshake + sender + master + pad1 ) )
     *   + SHA1( master + pad2 +
     *         SHA1( handshake + sender + master + pad1 ) )
     */

#if !defined(POLARSSL_MD5_ALT)
    SSL_DEBUG_BUF( 4, "finished  md5 state", (unsigned char *)
                    md5.state, sizeof(  md5.state ) );
#endif

#if !defined(POLARSSL_SHA1_ALT)
    SSL_DEBUG_BUF( 4, "finished sha1 state", (unsigned char *)
                   sha1.state, sizeof( sha1.state ) );
#endif

    sender = ( from == SSL_IS_CLIENT ) ? "CLNT"
                                       : "SRVR";

    memset( padbuf, 0x36, 48 );

    md5_update( &md5, (const unsigned char *) sender, 4 );
    md5_update( &md5, session->master, 48 );
    md5_update( &md5, padbuf, 48 );
    md5_finish( &md5, md5sum );

    sha1_update( &sha1, (const unsigned char *) sender, 4 );
    sha1_update( &sha1, session->master, 48 );
    sha1_update( &sha1, padbuf, 40 );
    sha1_finish( &sha1, sha1sum );

    memset( padbuf, 0x5C, 48 );

    md5_starts( &md5 );
    md5_update( &md5, session->master, 48 );
    md5_update( &md5, padbuf, 48 );
    md5_update( &md5, md5sum, 16 );
    md5_finish( &md5, buf );

    sha1_starts( &sha1 );
    sha1_update( &sha1, session->master, 48 );
    sha1_update( &sha1, padbuf , 40 );
    sha1_update( &sha1, sha1sum, 20 );
    sha1_finish( &sha1, buf + 16 );

    SSL_DEBUG_BUF( 3, "calc finished result", buf, 36 );

    md5_free(  &md5  );
    sha1_free( &sha1 );

    polarssl_zeroize(  padbuf, sizeof(  padbuf ) );
    polarssl_zeroize(  md5sum, sizeof(  md5sum ) );
    polarssl_zeroize( sha1sum, sizeof( sha1sum ) );

    SSL_DEBUG_MSG( 2, ( "<= calc  finished" ) );
}
#endif /* POLARSSL_SSL_PROTO_SSL3 */

#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1)
static void ssl_calc_finished_tls(
                ssl_context *ssl, unsigned char *buf, int from )
{
    int len = 12;
    const char *sender;
    md5_context  md5;
    sha1_context sha1;
    unsigned char padbuf[36];

    ssl_session *session = ssl->session_negotiate;
    if( !session )
        session = ssl->session;

    SSL_DEBUG_MSG( 2, ( "=> calc  finished tls" ) );

    memcpy( &md5 , &ssl->handshake->fin_md5 , sizeof(md5_context)  );
    memcpy( &sha1, &ssl->handshake->fin_sha1, sizeof(sha1_context) );

    /*
     * TLSv1:
     *   hash = PRF( master, finished_label,
     *               MD5( handshake ) + SHA1( handshake ) )[0..11]
     */

#if !defined(POLARSSL_MD5_ALT)
    SSL_DEBUG_BUF( 4, "finished  md5 state", (unsigned char *)
                    md5.state, sizeof(  md5.state ) );
#endif

#if !defined(POLARSSL_SHA1_ALT)
    SSL_DEBUG_BUF( 4, "finished sha1 state", (unsigned char *)
                   sha1.state, sizeof( sha1.state ) );
#endif

    sender = ( from == SSL_IS_CLIENT )
             ? "client finished"
             : "server finished";

    md5_finish(  &md5, padbuf );
    sha1_finish( &sha1, padbuf + 16 );

    ssl->handshake->tls_prf( session->master, 48, sender,
                             padbuf, 36, buf, len );

    SSL_DEBUG_BUF( 3, "calc finished result", buf, len );

    md5_free(  &md5  );
    sha1_free( &sha1 );

    polarssl_zeroize(  padbuf, sizeof(  padbuf ) );

    SSL_DEBUG_MSG( 2, ( "<= calc  finished" ) );
}
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 */

#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
static void ssl_calc_finished_tls_sha256(
                ssl_context *ssl, unsigned char *buf, int from )
{
    int len = 12;
    const char *sender;
    sha256_context sha256;
    unsigned char padbuf[32];

    ssl_session *session = ssl->session_negotiate;
    if( !session )
        session = ssl->session;

    SSL_DEBUG_MSG( 2, ( "=> calc  finished tls sha256" ) );

    memcpy( &sha256, &ssl->handshake->fin_sha256, sizeof(sha256_context) );

    /*
     * TLSv1.2:
     *   hash = PRF( master, finished_label,
     *               Hash( handshake ) )[0.11]
     */

#if !defined(POLARSSL_SHA256_ALT)
    SSL_DEBUG_BUF( 4, "finished sha2 state", (unsigned char *)
                   sha256.state, sizeof( sha256.state ) );
#endif

    sender = ( from == SSL_IS_CLIENT )
             ? "client finished"
             : "server finished";

    sha256_finish( &sha256, padbuf );

    ssl->handshake->tls_prf( session->master, 48, sender,
                             padbuf, 32, buf, len );

    SSL_DEBUG_BUF( 3, "calc finished result", buf, len );

    sha256_free( &sha256 );

    polarssl_zeroize(  padbuf, sizeof(  padbuf ) );

    SSL_DEBUG_MSG( 2, ( "<= calc  finished" ) );
}
#endif /* POLARSSL_SHA256_C */

#if defined(POLARSSL_SHA512_C)
static void ssl_calc_finished_tls_sha384(
                ssl_context *ssl, unsigned char *buf, int from )
{
    int len = 12;
    const char *sender;
    sha512_context sha512;
    unsigned char padbuf[48];

    ssl_session *session = ssl->session_negotiate;
    if( !session )
        session = ssl->session;

    SSL_DEBUG_MSG( 2, ( "=> calc  finished tls sha384" ) );

    memcpy( &sha512, &ssl->handshake->fin_sha512, sizeof(sha512_context) );

    /*
     * TLSv1.2:
     *   hash = PRF( master, finished_label,
     *               Hash( handshake ) )[0.11]
     */

#if !defined(POLARSSL_SHA512_ALT)
    SSL_DEBUG_BUF( 4, "finished sha512 state", (unsigned char *)
                   sha512.state, sizeof( sha512.state ) );
#endif

    sender = ( from == SSL_IS_CLIENT )
             ? "client finished"
             : "server finished";

    sha512_finish( &sha512, padbuf );

    ssl->handshake->tls_prf( session->master, 48, sender,
                             padbuf, 48, buf, len );

    SSL_DEBUG_BUF( 3, "calc finished result", buf, len );

    sha512_free( &sha512 );

    polarssl_zeroize(  padbuf, sizeof( padbuf ) );

    SSL_DEBUG_MSG( 2, ( "<= calc  finished" ) );
}
#endif /* POLARSSL_SHA512_C */
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

void ssl_handshake_wrapup( ssl_context *ssl )
{
    int resume = ssl->handshake->resume;

    SSL_DEBUG_MSG( 3, ( "=> handshake wrapup" ) );

    /*
     * Free our handshake params
     */
    ssl_handshake_free( ssl->handshake );
    polarssl_free( ssl->handshake );
    ssl->handshake = NULL;

#if defined(POLARSSL_SSL_RENEGOTIATION)
    if( ssl->renegotiation == SSL_RENEGOTIATION )
    {
        ssl->renegotiation =  SSL_RENEGOTIATION_DONE;
        ssl->renego_records_seen = 0;
    }
#endif

    /*
     * Switch in our now active transform context
     */
    if( ssl->transform )
    {
        ssl_transform_free( ssl->transform );
        polarssl_free( ssl->transform );
    }
    ssl->transform = ssl->transform_negotiate;
    ssl->transform_negotiate = NULL;

    if( ssl->session )
    {
#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
        /* RFC 7366 3.1: keep the EtM state */
        ssl->session_negotiate->encrypt_then_mac =
                  ssl->session->encrypt_then_mac;
#endif

        ssl_session_free( ssl->session );
        polarssl_free( ssl->session );
    }
    ssl->session = ssl->session_negotiate;
    ssl->session_negotiate = NULL;

    /*
     * Add cache entry
     */
    if( ssl->f_set_cache != NULL &&
        ssl->session->length != 0 &&
        resume == 0 )
    {
        if( ssl->f_set_cache( ssl->p_set_cache, ssl->session ) != 0 )
            SSL_DEBUG_MSG( 1, ( "cache did not store session" ) );
    }

    ssl->state++;

    SSL_DEBUG_MSG( 3, ( "<= handshake wrapup" ) );
}

int ssl_write_finished( ssl_context *ssl )
{
    int ret, hash_len;

    SSL_DEBUG_MSG( 2, ( "=> write finished" ) );

    /*
     * Set the out_msg pointer to the correct location based on IV length
     */
    if( ssl->minor_ver >= SSL_MINOR_VERSION_2 )
    {
        ssl->out_msg = ssl->out_iv + ssl->transform_negotiate->ivlen -
                       ssl->transform_negotiate->fixed_ivlen;
    }
    else
        ssl->out_msg = ssl->out_iv;

    ssl->handshake->calc_finished( ssl, ssl->out_msg + 4, ssl->endpoint );

    // TODO TLS/1.2 Hash length is determined by cipher suite (Page 63)
    hash_len = ( ssl->minor_ver == SSL_MINOR_VERSION_0 ) ? 36 : 12;

#if defined(POLARSSL_SSL_RENEGOTIATION)
    ssl->verify_data_len = hash_len;
    memcpy( ssl->own_verify_data, ssl->out_msg + 4, hash_len );
#endif

    ssl->out_msglen  = 4 + hash_len;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_FINISHED;

    /*
     * In case of session resuming, invert the client and server
     * ChangeCipherSpec messages order.
     */
    if( ssl->handshake->resume != 0 )
    {
#if defined(POLARSSL_SSL_CLI_C)
        if( ssl->endpoint == SSL_IS_CLIENT )
            ssl->state = SSL_HANDSHAKE_WRAPUP;
#endif
#if defined(POLARSSL_SSL_SRV_C)
        if( ssl->endpoint == SSL_IS_SERVER )
            ssl->state = SSL_CLIENT_CHANGE_CIPHER_SPEC;
#endif
    }
    else
        ssl->state++;

    /*
     * Switch to our negotiated transform and session parameters for outbound
     * data.
     */
    SSL_DEBUG_MSG( 3, ( "switching to new transform spec for outbound data" ) );
    ssl->transform_out = ssl->transform_negotiate;
    ssl->session_out = ssl->session_negotiate;
    memset( ssl->out_ctr, 0, 8 );

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_activate != NULL )
    {
        if( ( ret = ssl_hw_record_activate( ssl, SSL_CHANNEL_OUTBOUND ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_activate", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }
    }
#endif

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_write_record", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= write finished" ) );

    return( 0 );
}

int ssl_parse_finished( ssl_context *ssl )
{
    int ret;
    unsigned int hash_len;
    unsigned char buf[36];

    SSL_DEBUG_MSG( 2, ( "=> parse finished" ) );

    ssl->handshake->calc_finished( ssl, buf, ssl->endpoint ^ 1 );

    /*
     * Switch to our negotiated transform and session parameters for inbound
     * data.
     */
    SSL_DEBUG_MSG( 3, ( "switching to new transform spec for inbound data" ) );
    ssl->transform_in = ssl->transform_negotiate;
    ssl->session_in = ssl->session_negotiate;
    memset( ssl->in_ctr, 0, 8 );

    /*
     * Set the in_msg pointer to the correct location based on IV length
     */
    if( ssl->minor_ver >= SSL_MINOR_VERSION_2 )
    {
        ssl->in_msg = ssl->in_iv + ssl->transform_negotiate->ivlen -
                      ssl->transform_negotiate->fixed_ivlen;
    }
    else
        ssl->in_msg = ssl->in_iv;

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_activate != NULL )
    {
        if( ( ret = ssl_hw_record_activate( ssl, SSL_CHANNEL_INBOUND ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_activate", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }
    }
#endif

    if( ( ret = ssl_read_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_read_record", ret );
        return( ret );
    }

    if( ssl->in_msgtype != SSL_MSG_HANDSHAKE )
    {
        SSL_DEBUG_MSG( 1, ( "bad finished message" ) );
        return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
    }

    // TODO TLS/1.2 Hash length is determined by cipher suite (Page 63)
    hash_len = ( ssl->minor_ver == SSL_MINOR_VERSION_0 ) ? 36 : 12;

    if( ssl->in_msg[0] != SSL_HS_FINISHED ||
        ssl->in_hslen  != 4 + hash_len )
    {
        SSL_DEBUG_MSG( 1, ( "bad finished message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_FINISHED );
    }

    if( safer_memcmp( ssl->in_msg + 4, buf, hash_len ) != 0 )
    {
        SSL_DEBUG_MSG( 1, ( "bad finished message" ) );
        return( POLARSSL_ERR_SSL_BAD_HS_FINISHED );
    }

#if defined(POLARSSL_SSL_RENEGOTIATION)
    ssl->verify_data_len = hash_len;
    memcpy( ssl->peer_verify_data, buf, hash_len );
#endif

    if( ssl->handshake->resume != 0 )
    {
#if defined(POLARSSL_SSL_CLI_C)
        if( ssl->endpoint == SSL_IS_CLIENT )
            ssl->state = SSL_CLIENT_CHANGE_CIPHER_SPEC;
#endif
#if defined(POLARSSL_SSL_SRV_C)
        if( ssl->endpoint == SSL_IS_SERVER )
            ssl->state = SSL_HANDSHAKE_WRAPUP;
#endif
    }
    else
        ssl->state++;

    SSL_DEBUG_MSG( 2, ( "<= parse finished" ) );

    return( 0 );
}

static void ssl_handshake_params_init( ssl_handshake_params *handshake )
{
    memset( handshake, 0, sizeof( ssl_handshake_params ) );

#if defined(POLARSSL_SSL_PROTO_SSL3) || defined(POLARSSL_SSL_PROTO_TLS1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_1)
     md5_init(   &handshake->fin_md5  );
    sha1_init(   &handshake->fin_sha1 );
     md5_starts( &handshake->fin_md5  );
    sha1_starts( &handshake->fin_sha1 );
#endif
#if defined(POLARSSL_SSL_PROTO_TLS1_2)
#if defined(POLARSSL_SHA256_C)
    sha256_init(   &handshake->fin_sha256    );
    sha256_starts( &handshake->fin_sha256, 0 );
#endif
#if defined(POLARSSL_SHA512_C)
    sha512_init(   &handshake->fin_sha512    );
    sha512_starts( &handshake->fin_sha512, 1 );
#endif
#endif /* POLARSSL_SSL_PROTO_TLS1_2 */

    handshake->update_checksum = ssl_update_checksum_start;
    handshake->sig_alg = SSL_HASH_SHA1;

#if defined(POLARSSL_DHM_C)
    dhm_init( &handshake->dhm_ctx );
#endif
#if defined(POLARSSL_ECDH_C)
    ecdh_init( &handshake->ecdh_ctx );
#endif
}

static void ssl_transform_init( ssl_transform *transform )
{
    memset( transform, 0, sizeof(ssl_transform) );

    cipher_init( &transform->cipher_ctx_enc );
    cipher_init( &transform->cipher_ctx_dec );

    md_init( &transform->md_ctx_enc );
    md_init( &transform->md_ctx_dec );
}

void ssl_session_init( ssl_session *session )
{
    memset( session, 0, sizeof(ssl_session) );
}

static int ssl_handshake_init( ssl_context *ssl )
{
    /* Clear old handshake information if present */
    if( ssl->transform_negotiate )
        ssl_transform_free( ssl->transform_negotiate );
    if( ssl->session_negotiate )
        ssl_session_free( ssl->session_negotiate );
    if( ssl->handshake )
        ssl_handshake_free( ssl->handshake );

    /*
     * Either the pointers are now NULL or cleared properly and can be freed.
     * Now allocate missing structures.
     */
    if( ssl->transform_negotiate == NULL )
    {
        ssl->transform_negotiate = (ssl_transform *) polarssl_malloc(
                             sizeof(ssl_transform) );
    }

    if( ssl->session_negotiate == NULL )
    {
        ssl->session_negotiate = (ssl_session *) polarssl_malloc(
                           sizeof(ssl_session) );
    }

    if( ssl->handshake == NULL )
    {
        ssl->handshake = (ssl_handshake_params *)
            polarssl_malloc( sizeof(ssl_handshake_params) );
    }

    /* All pointers should exist and can be directly freed without issue */
    if( ssl->handshake == NULL ||
        ssl->transform_negotiate == NULL ||
        ssl->session_negotiate == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "malloc() of ssl sub-contexts failed" ) );

        polarssl_free( ssl->handshake );
        polarssl_free( ssl->transform_negotiate );
        polarssl_free( ssl->session_negotiate );

        ssl->handshake = NULL;
        ssl->transform_negotiate = NULL;
        ssl->session_negotiate = NULL;

        return( POLARSSL_ERR_SSL_MALLOC_FAILED );
    }

    /* Initialize structures */
    ssl_session_init( ssl->session_negotiate );
    ssl_transform_init( ssl->transform_negotiate );
    ssl_handshake_params_init( ssl->handshake );

#if defined(POLARSSL_X509_CRT_PARSE_C)
    ssl->handshake->key_cert = ssl->key_cert;
#endif

    return( 0 );
}

/*
 * Initialize an SSL context
 */
int ssl_init( ssl_context *ssl )
{
    int ret;
    int len = SSL_BUFFER_LEN;

    memset( ssl, 0, sizeof( ssl_context ) );

    /*
     * Sane defaults
     */
    ssl->min_major_ver = SSL_MIN_MAJOR_VERSION;
    ssl->min_minor_ver = SSL_MIN_MINOR_VERSION;
    ssl->max_major_ver = SSL_MAX_MAJOR_VERSION;
    ssl->max_minor_ver = SSL_MAX_MINOR_VERSION;

    ssl_set_ciphersuites( ssl, ssl_list_ciphersuites() );

#if defined(POLARSSL_SSL_RENEGOTIATION)
    ssl->renego_max_records = SSL_RENEGO_MAX_RECORDS_DEFAULT;
    memset( ssl->renego_period, 0xFF, 7 );
    ssl->renego_period[7] = 0x00;
#endif

#if defined(POLARSSL_DHM_C)
    if( ( ret = mpi_read_string( &ssl->dhm_P, 16,
                                 POLARSSL_DHM_RFC5114_MODP_1024_P) ) != 0 ||
        ( ret = mpi_read_string( &ssl->dhm_G, 16,
                                 POLARSSL_DHM_RFC5114_MODP_1024_G) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "mpi_read_string", ret );
        return( ret );
    }
#endif

    /*
     * Prepare base structures
     */
    ssl->in_ctr = (unsigned char *) polarssl_malloc( len );
    ssl->in_hdr = ssl->in_ctr +  8;
    ssl->in_iv  = ssl->in_ctr + 13;
    ssl->in_msg = ssl->in_ctr + 13;

    if( ssl->in_ctr == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "malloc(%d bytes) failed", len ) );
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );
    }

    ssl->out_ctr = (unsigned char *) polarssl_malloc( len );
    ssl->out_hdr = ssl->out_ctr +  8;
    ssl->out_iv  = ssl->out_ctr + 13;
    ssl->out_msg = ssl->out_ctr + 13;

    if( ssl->out_ctr == NULL )
    {
        SSL_DEBUG_MSG( 1, ( "malloc(%d bytes) failed", len ) );
        polarssl_free( ssl->in_ctr );
        ssl->in_ctr = NULL;
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );
    }

    memset( ssl-> in_ctr, 0, SSL_BUFFER_LEN );
    memset( ssl->out_ctr, 0, SSL_BUFFER_LEN );

#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
    ssl->encrypt_then_mac = SSL_ETM_ENABLED;
#endif

#if defined(POLARSSL_SSL_EXTENDED_MASTER_SECRET)
    ssl->extended_ms = SSL_EXTENDED_MS_ENABLED;
#endif

#if defined(POLARSSL_SSL_SESSION_TICKETS)
    ssl->ticket_lifetime = SSL_DEFAULT_TICKET_LIFETIME;
#endif

#if defined(POLARSSL_SSL_SET_CURVES)
    ssl->curve_list = ecp_grp_id_list( );
#endif

    if( ( ret = ssl_handshake_init( ssl ) ) != 0 )
        return( ret );

    return( 0 );
}

/*
 * Reset an initialized and used SSL context for re-use while retaining
 * all application-set variables, function pointers and data.
 */
int ssl_session_reset( ssl_context *ssl )
{
    int ret;

    ssl->state = SSL_HELLO_REQUEST;

#if defined(POLARSSL_SSL_RENEGOTIATION)
    ssl->renegotiation = SSL_INITIAL_HANDSHAKE;
    ssl->renego_records_seen = 0;

    ssl->verify_data_len = 0;
    memset( ssl->own_verify_data, 0, SSL_VERIFY_DATA_MAX_LEN );
    memset( ssl->peer_verify_data, 0, SSL_VERIFY_DATA_MAX_LEN );
#endif
    ssl->secure_renegotiation = SSL_LEGACY_RENEGOTIATION;

    ssl->in_offt = NULL;

    ssl->in_msg = ssl->in_ctr + 13;
    ssl->in_msgtype = 0;
    ssl->in_msglen = 0;
    ssl->in_left = 0;

    ssl->in_hslen = 0;
    ssl->nb_zero = 0;
    ssl->record_read = 0;

    ssl->out_msg = ssl->out_ctr + 13;
    ssl->out_msgtype = 0;
    ssl->out_msglen = 0;
    ssl->out_left = 0;
#if defined(POLARSSL_SSL_CBC_RECORD_SPLITTING)
    if( ssl->split_done != SSL_CBC_RECORD_SPLITTING_DISABLED )
        ssl->split_done = 0;
#endif

    ssl->transform_in = NULL;
    ssl->transform_out = NULL;

    memset( ssl->out_ctr, 0, SSL_BUFFER_LEN );
    memset( ssl->in_ctr, 0, SSL_BUFFER_LEN );

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_reset != NULL )
    {
        SSL_DEBUG_MSG( 2, ( "going for ssl_hw_record_reset()" ) );
        if( ( ret = ssl_hw_record_reset( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_hw_record_reset", ret );
            return( POLARSSL_ERR_SSL_HW_ACCEL_FAILED );
        }
    }
#endif

    if( ssl->transform )
    {
        ssl_transform_free( ssl->transform );
        polarssl_free( ssl->transform );
        ssl->transform = NULL;
    }

    if( ssl->session )
    {
        ssl_session_free( ssl->session );
        polarssl_free( ssl->session );
        ssl->session = NULL;
    }

#if defined(POLARSSL_SSL_ALPN)
    ssl->alpn_chosen = NULL;
#endif

    if( ( ret = ssl_handshake_init( ssl ) ) != 0 )
        return( ret );

    return( 0 );
}

#if defined(POLARSSL_SSL_SESSION_TICKETS)
static void ssl_ticket_keys_free( ssl_ticket_keys *tkeys )
{
    aes_free( &tkeys->enc );
    aes_free( &tkeys->dec );

    polarssl_zeroize( tkeys, sizeof(ssl_ticket_keys) );
}

/*
 * Allocate and initialize ticket keys
 */
static int ssl_ticket_keys_init( ssl_context *ssl )
{
    int ret;
    ssl_ticket_keys *tkeys;
    unsigned char buf[16];

    if( ssl->ticket_keys != NULL )
        return( 0 );

    tkeys = (ssl_ticket_keys *) polarssl_malloc( sizeof(ssl_ticket_keys) );
    if( tkeys == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    aes_init( &tkeys->enc );
    aes_init( &tkeys->dec );

    if( ( ret = ssl->f_rng( ssl->p_rng, tkeys->key_name, 16 ) ) != 0 )
    {
        ssl_ticket_keys_free( tkeys );
        polarssl_free( tkeys );
        return( ret );
    }

    if( ( ret = ssl->f_rng( ssl->p_rng, buf, 16 ) ) != 0 ||
        ( ret = aes_setkey_enc( &tkeys->enc, buf, 128 ) ) != 0 ||
        ( ret = aes_setkey_dec( &tkeys->dec, buf, 128 ) ) != 0 )
    {
        ssl_ticket_keys_free( tkeys );
        polarssl_free( tkeys );
        return( ret );
    }

    if( ( ret = ssl->f_rng( ssl->p_rng, tkeys->mac_key, 16 ) ) != 0 )
    {
        ssl_ticket_keys_free( tkeys );
        polarssl_free( tkeys );
        return( ret );
    }

    ssl->ticket_keys = tkeys;

    return( 0 );
}
#endif /* POLARSSL_SSL_SESSION_TICKETS */

/*
 * SSL set accessors
 */
void ssl_set_endpoint( ssl_context *ssl, int endpoint )
{
    ssl->endpoint   = endpoint;

#if defined(POLARSSL_SSL_SESSION_TICKETS) && \
    defined(POLARSSL_SSL_CLI_C)
    if( endpoint == SSL_IS_CLIENT )
        ssl->session_tickets = SSL_SESSION_TICKETS_ENABLED;
#endif

#if defined(POLARSSL_SSL_TRUNCATED_HMAC)
    if( endpoint == SSL_IS_SERVER )
        ssl->trunc_hmac = SSL_TRUNC_HMAC_ENABLED;
#endif
}

void ssl_set_authmode( ssl_context *ssl, int authmode )
{
    ssl->authmode   = authmode;
}

#if defined(POLARSSL_X509_CRT_PARSE_C)
void ssl_set_verify( ssl_context *ssl,
                     int (*f_vrfy)(void *, x509_crt *, int, int *),
                     void *p_vrfy )
{
    ssl->f_vrfy      = f_vrfy;
    ssl->p_vrfy      = p_vrfy;
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

void ssl_set_rng( ssl_context *ssl,
                  int (*f_rng)(void *, unsigned char *, size_t),
                  void *p_rng )
{
    ssl->f_rng      = f_rng;
    ssl->p_rng      = p_rng;
}

void ssl_set_dbg( ssl_context *ssl,
                  void (*f_dbg)(void *, int, const char *),
                  void  *p_dbg )
{
    ssl->f_dbg      = f_dbg;
    ssl->p_dbg      = p_dbg;
}

void ssl_set_bio( ssl_context *ssl,
            int (*f_recv)(void *, unsigned char *, size_t), void *p_recv,
            int (*f_send)(void *, const unsigned char *, size_t), void *p_send )
{
    ssl->f_recv     = f_recv;
    ssl->f_send     = f_send;
    ssl->p_recv     = p_recv;
    ssl->p_send     = p_send;
}

#if defined(POLARSSL_SSL_SRV_C)
void ssl_set_session_cache( ssl_context *ssl,
        int (*f_get_cache)(void *, ssl_session *), void *p_get_cache,
        int (*f_set_cache)(void *, const ssl_session *), void *p_set_cache )
{
    ssl->f_get_cache = f_get_cache;
    ssl->p_get_cache = p_get_cache;
    ssl->f_set_cache = f_set_cache;
    ssl->p_set_cache = p_set_cache;
}
#endif /* POLARSSL_SSL_SRV_C */

#if defined(POLARSSL_SSL_CLI_C)
int ssl_set_session( ssl_context *ssl, const ssl_session *session )
{
    int ret;

    if( ssl == NULL ||
        session == NULL ||
        ssl->session_negotiate == NULL ||
        ssl->endpoint != SSL_IS_CLIENT )
    {
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    if( ( ret = ssl_session_copy( ssl->session_negotiate, session ) ) != 0 )
        return( ret );

    ssl->handshake->resume = 1;

    return( 0 );
}
#endif /* POLARSSL_SSL_CLI_C */

void ssl_set_ciphersuites( ssl_context *ssl, const int *ciphersuites )
{
    ssl->ciphersuite_list[SSL_MINOR_VERSION_0] = ciphersuites;
    ssl->ciphersuite_list[SSL_MINOR_VERSION_1] = ciphersuites;
    ssl->ciphersuite_list[SSL_MINOR_VERSION_2] = ciphersuites;
    ssl->ciphersuite_list[SSL_MINOR_VERSION_3] = ciphersuites;
}

void ssl_set_ciphersuites_for_version( ssl_context *ssl,
                                       const int *ciphersuites,
                                       int major, int minor )
{
    if( major != SSL_MAJOR_VERSION_3 )
        return;

    if( minor < SSL_MINOR_VERSION_0 || minor > SSL_MINOR_VERSION_3 )
        return;

    ssl->ciphersuite_list[minor] = ciphersuites;
}

#if defined(POLARSSL_X509_CRT_PARSE_C)
/* Add a new (empty) key_cert entry an return a pointer to it */
static ssl_key_cert *ssl_add_key_cert( ssl_context *ssl )
{
    ssl_key_cert *key_cert, *last;

    key_cert = (ssl_key_cert *) polarssl_malloc( sizeof(ssl_key_cert) );
    if( key_cert == NULL )
        return( NULL );

    memset( key_cert, 0, sizeof( ssl_key_cert ) );

    /* Append the new key_cert to the (possibly empty) current list */
    if( ssl->key_cert == NULL )
    {
        ssl->key_cert = key_cert;
        if( ssl->handshake != NULL )
            ssl->handshake->key_cert = key_cert;
    }
    else
    {
        last = ssl->key_cert;
        while( last->next != NULL )
            last = last->next;
        last->next = key_cert;
    }

    return( key_cert );
}

void ssl_set_ca_chain( ssl_context *ssl, x509_crt *ca_chain,
                       x509_crl *ca_crl, const char *peer_cn )
{
    ssl->ca_chain   = ca_chain;
    ssl->ca_crl     = ca_crl;
    ssl->peer_cn    = peer_cn;
}

int ssl_set_own_cert( ssl_context *ssl, x509_crt *own_cert,
                       pk_context *pk_key )
{
    ssl_key_cert *key_cert = ssl_add_key_cert( ssl );

    if( key_cert == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    key_cert->cert = own_cert;
    key_cert->key  = pk_key;

    return( pk_check_pair( &key_cert->cert->pk, key_cert->key ) );
}

#if defined(POLARSSL_RSA_C)
int ssl_set_own_cert_rsa( ssl_context *ssl, x509_crt *own_cert,
                           rsa_context *rsa_key )
{
    int ret;
    ssl_key_cert *key_cert = ssl_add_key_cert( ssl );

    if( key_cert == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    key_cert->key = (pk_context *) polarssl_malloc( sizeof(pk_context) );
    if( key_cert->key == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    pk_init( key_cert->key );

    ret = pk_init_ctx( key_cert->key, pk_info_from_type( POLARSSL_PK_RSA ) );
    if( ret != 0 )
        return( ret );

    if( ( ret = rsa_copy( pk_rsa( *key_cert->key ), rsa_key ) ) != 0 )
        return( ret );

    key_cert->cert = own_cert;
    key_cert->key_own_alloc = 1;

    return( pk_check_pair( &key_cert->cert->pk, key_cert->key ) );
}
#endif /* POLARSSL_RSA_C */

int ssl_set_own_cert_alt( ssl_context *ssl, x509_crt *own_cert,
                          void *rsa_key,
                          rsa_decrypt_func rsa_decrypt,
                          rsa_sign_func rsa_sign,
                          rsa_key_len_func rsa_key_len )
{
    int ret;
    ssl_key_cert *key_cert = ssl_add_key_cert( ssl );

    if( key_cert == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    key_cert->key = (pk_context *) polarssl_malloc( sizeof(pk_context) );
    if( key_cert->key == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    pk_init( key_cert->key );

    if( ( ret = pk_init_ctx_rsa_alt( key_cert->key, rsa_key,
                                 rsa_decrypt, rsa_sign, rsa_key_len ) ) != 0 )
        return( ret );

    key_cert->cert = own_cert;
    key_cert->key_own_alloc = 1;

    return( pk_check_pair( &key_cert->cert->pk, key_cert->key ) );
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

#if defined(POLARSSL_KEY_EXCHANGE__SOME__PSK_ENABLED)
int ssl_set_psk( ssl_context *ssl, const unsigned char *psk, size_t psk_len,
                 const unsigned char *psk_identity, size_t psk_identity_len )
{
    if( psk == NULL || psk_identity == NULL )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    if( psk_len > POLARSSL_PSK_MAX_LEN )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    if( ssl->psk != NULL )
    {
        polarssl_free( ssl->psk );
        polarssl_free( ssl->psk_identity );
    }

    ssl->psk_len = psk_len;
    ssl->psk_identity_len = psk_identity_len;

    ssl->psk = (unsigned char *) polarssl_malloc( ssl->psk_len );
    ssl->psk_identity = (unsigned char *)
                                polarssl_malloc( ssl->psk_identity_len );

    if( ssl->psk == NULL || ssl->psk_identity == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    memcpy( ssl->psk, psk, ssl->psk_len );
    memcpy( ssl->psk_identity, psk_identity, ssl->psk_identity_len );

    return( 0 );
}

void ssl_set_psk_cb( ssl_context *ssl,
                     int (*f_psk)(void *, ssl_context *, const unsigned char *,
                     size_t),
                     void *p_psk )
{
    ssl->f_psk = f_psk;
    ssl->p_psk = p_psk;
}
#endif /* POLARSSL_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(POLARSSL_DHM_C)
int ssl_set_dh_param( ssl_context *ssl, const char *dhm_P, const char *dhm_G )
{
    int ret;

    if( ( ret = mpi_read_string( &ssl->dhm_P, 16, dhm_P ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "mpi_read_string", ret );
        return( ret );
    }

    if( ( ret = mpi_read_string( &ssl->dhm_G, 16, dhm_G ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "mpi_read_string", ret );
        return( ret );
    }

    return( 0 );
}

int ssl_set_dh_param_ctx( ssl_context *ssl, dhm_context *dhm_ctx )
{
    int ret;

    if( ( ret = mpi_copy( &ssl->dhm_P, &dhm_ctx->P ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "mpi_copy", ret );
        return( ret );
    }

    if( ( ret = mpi_copy( &ssl->dhm_G, &dhm_ctx->G ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "mpi_copy", ret );
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_DHM_C */

#if defined(POLARSSL_SSL_SET_CURVES)
/*
 * Set the allowed elliptic curves
 */
void ssl_set_curves( ssl_context *ssl, const ecp_group_id *curve_list )
{
  ssl->curve_list = curve_list;
}
#endif

#if defined(POLARSSL_SSL_SERVER_NAME_INDICATION)
int ssl_set_hostname( ssl_context *ssl, const char *hostname )
{
    if( hostname == NULL )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    ssl->hostname_len = strlen( hostname );

    if( ssl->hostname_len + 1 == 0 )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    ssl->hostname = (unsigned char *) polarssl_malloc( ssl->hostname_len + 1 );

    if( ssl->hostname == NULL )
        return( POLARSSL_ERR_SSL_MALLOC_FAILED );

    memcpy( ssl->hostname, (const unsigned char *) hostname,
            ssl->hostname_len );

    ssl->hostname[ssl->hostname_len] = '\0';

    return( 0 );
}

void ssl_set_sni( ssl_context *ssl,
                  int (*f_sni)(void *, ssl_context *,
                                const unsigned char *, size_t),
                  void *p_sni )
{
    ssl->f_sni = f_sni;
    ssl->p_sni = p_sni;
}
#endif /* POLARSSL_SSL_SERVER_NAME_INDICATION */

#if defined(POLARSSL_SSL_ALPN)
int ssl_set_alpn_protocols( ssl_context *ssl, const char **protos )
{
    size_t cur_len, tot_len;
    const char **p;

    /*
     * "Empty strings MUST NOT be included and byte strings MUST NOT be
     * truncated". Check lengths now rather than later.
     */
    tot_len = 0;
    for( p = protos; *p != NULL; p++ )
    {
        cur_len = strlen( *p );
        tot_len += cur_len;

        if( cur_len == 0 || cur_len > 255 || tot_len > 65535 )
            return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    ssl->alpn_list = protos;

    return( 0 );
}

const char *ssl_get_alpn_protocol( const ssl_context *ssl )
{
    return( ssl->alpn_chosen );
}
#endif /* POLARSSL_SSL_ALPN */

void ssl_set_max_version( ssl_context *ssl, int major, int minor )
{
    if( major >= SSL_MIN_MAJOR_VERSION && major <= SSL_MAX_MAJOR_VERSION &&
        minor >= SSL_MIN_MINOR_VERSION && minor <= SSL_MAX_MINOR_VERSION )
    {
        ssl->max_major_ver = major;
        ssl->max_minor_ver = minor;
    }
}

void ssl_set_min_version( ssl_context *ssl, int major, int minor )
{
    if( major >= SSL_MIN_MAJOR_VERSION && major <= SSL_MAX_MAJOR_VERSION &&
        minor >= SSL_MIN_MINOR_VERSION && minor <= SSL_MAX_MINOR_VERSION )
    {
        ssl->min_major_ver = major;
        ssl->min_minor_ver = minor;
    }
}

#if defined(POLARSSL_SSL_FALLBACK_SCSV) && defined(POLARSSL_SSL_CLI_C)
void ssl_set_fallback( ssl_context *ssl, char fallback )
{
    ssl->fallback = fallback;
}
#endif

#if defined(POLARSSL_SSL_ENCRYPT_THEN_MAC)
void ssl_set_encrypt_then_mac( ssl_context *ssl, char etm )
{
    ssl->encrypt_then_mac = etm;
}
#endif

#if defined(POLARSSL_SSL_EXTENDED_MASTER_SECRET)
void ssl_set_extended_master_secret( ssl_context *ssl, char ems )
{
    ssl->extended_ms = ems;
}
#endif

void ssl_set_arc4_support( ssl_context *ssl, char arc4 )
{
    ssl->arc4_disabled = arc4;
}

#if defined(POLARSSL_SSL_MAX_FRAGMENT_LENGTH)
int ssl_set_max_frag_len( ssl_context *ssl, unsigned char mfl_code )
{
    if( mfl_code >= SSL_MAX_FRAG_LEN_INVALID ||
        mfl_code_to_length[mfl_code] > SSL_MAX_CONTENT_LEN )
    {
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    ssl->mfl_code = mfl_code;

    return( 0 );
}
#endif /* POLARSSL_SSL_MAX_FRAGMENT_LENGTH */

#if defined(POLARSSL_SSL_TRUNCATED_HMAC)
int ssl_set_truncated_hmac( ssl_context *ssl, int truncate )
{
    ssl->trunc_hmac = truncate;

    return( 0 );
}
#endif /* POLARSSL_SSL_TRUNCATED_HMAC */

#if defined(POLARSSL_SSL_CBC_RECORD_SPLITTING)
void ssl_set_cbc_record_splitting( ssl_context *ssl, char split )
{
    ssl->split_done = split;
}
#endif

void ssl_legacy_renegotiation( ssl_context *ssl, int allow_legacy )
{
    ssl->allow_legacy_renegotiation = allow_legacy;
}

#if defined(POLARSSL_SSL_RENEGOTIATION)
void ssl_set_renegotiation( ssl_context *ssl, int renegotiation )
{
    ssl->disable_renegotiation = renegotiation;
}

void ssl_set_renegotiation_enforced( ssl_context *ssl, int max_records )
{
    ssl->renego_max_records = max_records;
}

void ssl_set_renegotiation_period( ssl_context *ssl,
                                   const unsigned char period[8] )
{
    memcpy( ssl->renego_period, period, 8 );
}
#endif /* POLARSSL_SSL_RENEGOTIATION */

#if defined(POLARSSL_SSL_SESSION_TICKETS)
int ssl_set_session_tickets( ssl_context *ssl, int use_tickets )
{
    ssl->session_tickets = use_tickets;

#if defined(POLARSSL_SSL_CLI_C)
    if( ssl->endpoint == SSL_IS_CLIENT )
        return( 0 );
#endif

    if( use_tickets == SSL_SESSION_TICKETS_DISABLED )
        return( 0 );

    if( ssl->f_rng == NULL )
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

    return( ssl_ticket_keys_init( ssl ) );
}

void ssl_set_session_ticket_lifetime( ssl_context *ssl, int lifetime )
{
    ssl->ticket_lifetime = lifetime;
}
#endif /* POLARSSL_SSL_SESSION_TICKETS */

/*
 * SSL get accessors
 */
size_t ssl_get_bytes_avail( const ssl_context *ssl )
{
    return( ssl->in_offt == NULL ? 0 : ssl->in_msglen );
}

int ssl_get_verify_result( const ssl_context *ssl )
{
    if( ssl->session != NULL )
        return( ssl->session->verify_result );

    if( ssl->session_negotiate != NULL )
        return( ssl->session_negotiate->verify_result );

    return( -1 );
}

const char *ssl_get_ciphersuite( const ssl_context *ssl )
{
    if( ssl == NULL || ssl->session == NULL )
        return( NULL );

    return ssl_get_ciphersuite_name( ssl->session->ciphersuite );
}

const char *ssl_get_version( const ssl_context *ssl )
{
    switch( ssl->minor_ver )
    {
        case SSL_MINOR_VERSION_0:
            return( "SSLv3.0" );

        case SSL_MINOR_VERSION_1:
            return( "TLSv1.0" );

        case SSL_MINOR_VERSION_2:
            return( "TLSv1.1" );

        case SSL_MINOR_VERSION_3:
            return( "TLSv1.2" );

        default:
            break;
    }
    return( "unknown" );
}

#if defined(POLARSSL_X509_CRT_PARSE_C)
const x509_crt *ssl_get_peer_cert( const ssl_context *ssl )
{
    if( ssl == NULL || ssl->session == NULL )
        return( NULL );

    return( ssl->session->peer_cert );
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

#if defined(POLARSSL_SSL_CLI_C)
int ssl_get_session( const ssl_context *ssl, ssl_session *dst )
{
    if( ssl == NULL ||
        dst == NULL ||
        ssl->session == NULL ||
        ssl->endpoint != SSL_IS_CLIENT )
    {
        return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );
    }

    return( ssl_session_copy( dst, ssl->session ) );
}
#endif /* POLARSSL_SSL_CLI_C */

/*
 * Perform a single step of the SSL handshake
 */
int ssl_handshake_step( ssl_context *ssl )
{
    int ret = POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE;

#if defined(POLARSSL_SSL_CLI_C)
    if( ssl->endpoint == SSL_IS_CLIENT )
        ret = ssl_handshake_client_step( ssl );
#endif
#if defined(POLARSSL_SSL_SRV_C)
    if( ssl->endpoint == SSL_IS_SERVER )
        ret = ssl_handshake_server_step( ssl );
#endif

    return( ret );
}

/*
 * Perform the SSL handshake
 */
int ssl_handshake( ssl_context *ssl )
{
    int ret = 0;

    SSL_DEBUG_MSG( 2, ( "=> handshake" ) );

    while( ssl->state != SSL_HANDSHAKE_OVER )
    {
        ret = ssl_handshake_step( ssl );

        if( ret != 0 )
            break;
    }

    SSL_DEBUG_MSG( 2, ( "<= handshake" ) );

    return( ret );
}

#if defined(POLARSSL_SSL_RENEGOTIATION)
#if defined(POLARSSL_SSL_SRV_C)
/*
 * Write HelloRequest to request renegotiation on server
 */
static int ssl_write_hello_request( ssl_context *ssl )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> write hello request" ) );

    ssl->out_msglen  = 4;
    ssl->out_msgtype = SSL_MSG_HANDSHAKE;
    ssl->out_msg[0]  = SSL_HS_HELLO_REQUEST;

    if( ( ret = ssl_write_record( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_write_record", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= write hello request" ) );

    return( 0 );
}
#endif /* POLARSSL_SSL_SRV_C */

/*
 * Actually renegotiate current connection, triggered by either:
 * - any side: calling ssl_renegotiate(),
 * - client: receiving a HelloRequest during ssl_read(),
 * - server: receiving any handshake message on server during ssl_read() after
 *   the initial handshake is completed.
 * If the handshake doesn't complete due to waiting for I/O, it will continue
 * during the next calls to ssl_renegotiate() or ssl_read() respectively.
 */
static int ssl_start_renegotiation( ssl_context *ssl )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> renegotiate" ) );

    if( ( ret = ssl_handshake_init( ssl ) ) != 0 )
        return( ret );

    ssl->state = SSL_HELLO_REQUEST;
    ssl->renegotiation = SSL_RENEGOTIATION;

    if( ( ret = ssl_handshake( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_handshake", ret );
        return( ret );
    }

    SSL_DEBUG_MSG( 2, ( "<= renegotiate" ) );

    return( 0 );
}

/*
 * Renegotiate current connection on client,
 * or request renegotiation on server
 */
int ssl_renegotiate( ssl_context *ssl )
{
    int ret = POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE;

#if defined(POLARSSL_SSL_SRV_C)
    /* On server, just send the request */
    if( ssl->endpoint == SSL_IS_SERVER )
    {
        if( ssl->state != SSL_HANDSHAKE_OVER )
            return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

        ssl->renegotiation = SSL_RENEGOTIATION_PENDING;

        /* Did we already try/start sending HelloRequest? */
        if( ssl->out_left != 0 )
            return( ssl_flush_output( ssl ) );

        return( ssl_write_hello_request( ssl ) );
    }
#endif /* POLARSSL_SSL_SRV_C */

#if defined(POLARSSL_SSL_CLI_C)
    /*
     * On client, either start the renegotiation process or,
     * if already in progress, continue the handshake
     */
    if( ssl->renegotiation != SSL_RENEGOTIATION )
    {
        if( ssl->state != SSL_HANDSHAKE_OVER )
            return( POLARSSL_ERR_SSL_BAD_INPUT_DATA );

        if( ( ret = ssl_start_renegotiation( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_start_renegotiation", ret );
            return( ret );
        }
    }
    else
    {
        if( ( ret = ssl_handshake( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_handshake", ret );
            return( ret );
        }
    }
#endif /* POLARSSL_SSL_CLI_C */

    return( ret );
}

/*
 * Check record counters and renegotiate if they're above the limit.
 */
static int ssl_check_ctr_renegotiate( ssl_context *ssl )
{
    if( ssl->state != SSL_HANDSHAKE_OVER ||
        ssl->renegotiation == SSL_RENEGOTIATION_PENDING ||
        ssl->disable_renegotiation == SSL_RENEGOTIATION_DISABLED )
    {
        return( 0 );
    }

    // TODO: adapt for DTLS
    if( memcmp( ssl->in_ctr,  ssl->renego_period, 8 ) <= 0 &&
        memcmp( ssl->out_ctr, ssl->renego_period, 8 ) <= 0 )
    {
        return( 0 );
    }

    SSL_DEBUG_MSG( 0, ( "record counter limit reached: renegotiate" ) );
    return( ssl_renegotiate( ssl ) );
}
#endif /* POLARSSL_SSL_RENEGOTIATION */

/*
 * Receive application data decrypted from the SSL layer
 */
int ssl_read( ssl_context *ssl, unsigned char *buf, size_t len )
{
    int ret, record_read = 0;
    size_t n;

    SSL_DEBUG_MSG( 2, ( "=> read" ) );

#if defined(POLARSSL_SSL_RENEGOTIATION)
    if( ( ret = ssl_check_ctr_renegotiate( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_check_ctr_renegotiate", ret );
        return( ret );
    }
#endif

    if( ssl->state != SSL_HANDSHAKE_OVER )
    {
        ret = ssl_handshake( ssl );
        if( ret == POLARSSL_ERR_SSL_WAITING_SERVER_HELLO_RENEGO )
        {
            record_read = 1;
        }
        else if( ret != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_handshake", ret );
            return( ret );
        }
    }

    if( ssl->in_offt == NULL )
    {
        if( ! record_read )
        {
            if( ( ret = ssl_read_record( ssl ) ) != 0 )
            {
                if( ret == POLARSSL_ERR_SSL_CONN_EOF )
                    return( 0 );

                SSL_DEBUG_RET( 1, "ssl_read_record", ret );
                return( ret );
            }
        }

        if( ssl->in_msglen  == 0 &&
            ssl->in_msgtype == SSL_MSG_APPLICATION_DATA )
        {
            /*
             * OpenSSL sends empty messages to randomize the IV
             */
            if( ( ret = ssl_read_record( ssl ) ) != 0 )
            {
                if( ret == POLARSSL_ERR_SSL_CONN_EOF )
                    return( 0 );

                SSL_DEBUG_RET( 1, "ssl_read_record", ret );
                return( ret );
            }
        }

#if defined(POLARSSL_SSL_RENEGOTIATION)
        if( ssl->in_msgtype == SSL_MSG_HANDSHAKE )
        {
            SSL_DEBUG_MSG( 1, ( "received handshake message" ) );

#if defined(POLARSSL_SSL_CLI_C)
            if( ssl->endpoint == SSL_IS_CLIENT &&
                ( ssl->in_msg[0] != SSL_HS_HELLO_REQUEST ||
                  ssl->in_hslen != 4 ) )
            {
                SSL_DEBUG_MSG( 1, ( "handshake received (not HelloRequest)" ) );
                return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
            }
#endif

            if( ssl->disable_renegotiation == SSL_RENEGOTIATION_DISABLED ||
                ( ssl->secure_renegotiation == SSL_LEGACY_RENEGOTIATION &&
                  ssl->allow_legacy_renegotiation ==
                                                SSL_LEGACY_NO_RENEGOTIATION ) )
            {
                SSL_DEBUG_MSG( 3, ( "ignoring renegotiation, sending alert" ) );

#if defined(POLARSSL_SSL_PROTO_SSL3)
                if( ssl->minor_ver == SSL_MINOR_VERSION_0 )
                {
                    /*
                     * SSLv3 does not have a "no_renegotiation" alert
                     */
                    if( ( ret = ssl_send_fatal_handshake_failure( ssl ) ) != 0 )
                        return( ret );
                }
                else
#endif /* POLARSSL_SSL_PROTO_SSL3 */
#if defined(POLARSSL_SSL_PROTO_TLS1) || defined(POLARSSL_SSL_PROTO_TLS1_1) || \
    defined(POLARSSL_SSL_PROTO_TLS1_2)
                if( ssl->minor_ver >= SSL_MINOR_VERSION_1 )
                {
                    if( ( ret = ssl_send_alert_message( ssl,
                                    SSL_ALERT_LEVEL_WARNING,
                                    SSL_ALERT_MSG_NO_RENEGOTIATION ) ) != 0 )
                    {
                        return( ret );
                    }
                }
                else
#endif /* POLARSSL_SSL_PROTO_TLS1 || POLARSSL_SSL_PROTO_TLS1_1 ||
          POLARSSL_SSL_PROTO_TLS1_2 */
                {
                    SSL_DEBUG_MSG( 1, ( "should never happen" ) );
                    return( POLARSSL_ERR_SSL_INTERNAL_ERROR );
                }
            }
            else
            {
                ret = ssl_start_renegotiation( ssl );
                if( ret == POLARSSL_ERR_SSL_WAITING_SERVER_HELLO_RENEGO )
                {
                    record_read = 1;
                }
                else if( ret != 0 )
                {
                    SSL_DEBUG_RET( 1, "ssl_start_renegotiation", ret );
                    return( ret );
                }
            }

            /* If a non-handshake record was read during renego, fallthrough,
             * else tell the user they should call ssl_read() again */
            if( ! record_read )
                return( POLARSSL_ERR_NET_WANT_READ );
        }
        else if( ssl->renegotiation == SSL_RENEGOTIATION_PENDING )
        {
            ssl->renego_records_seen++;

            if( ssl->renego_max_records >= 0 &&
                ssl->renego_records_seen > ssl->renego_max_records )
            {
                SSL_DEBUG_MSG( 1, ( "renegotiation requested, "
                                    "but not honored by client" ) );
                return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
            }
        }
#endif /* POLARSSL_SSL_RENEGOTIATION */

        /* Fatal and closure alerts handled by ssl_read_record() */
        if( ssl->in_msgtype == SSL_MSG_ALERT )
        {
            SSL_DEBUG_MSG( 2, ( "ignoring non-fatal non-closure alert" ) );
            return( POLARSSL_ERR_NET_WANT_READ );
        }

        if( ssl->in_msgtype != SSL_MSG_APPLICATION_DATA )
        {
            SSL_DEBUG_MSG( 1, ( "bad application data message" ) );
            return( POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE );
        }

        ssl->in_offt = ssl->in_msg;
    }

    n = ( len < ssl->in_msglen )
        ? len : ssl->in_msglen;

    memcpy( buf, ssl->in_offt, n );
    ssl->in_msglen -= n;

    if( ssl->in_msglen == 0 )
        /* all bytes consumed  */
        ssl->in_offt = NULL;
    else
        /* more data available */
        ssl->in_offt += n;

    SSL_DEBUG_MSG( 2, ( "<= read" ) );

    return( (int) n );
}

/*
 * Send application data to be encrypted by the SSL layer
 */
#if defined(POLARSSL_SSL_CBC_RECORD_SPLITTING)
static int ssl_write_real( ssl_context *ssl, const unsigned char *buf, size_t len )
#else
int ssl_write( ssl_context *ssl, const unsigned char *buf, size_t len )
#endif
{
    int ret;
    size_t n;
    unsigned int max_len = SSL_MAX_CONTENT_LEN;

    SSL_DEBUG_MSG( 2, ( "=> write" ) );

#if defined(POLARSSL_SSL_RENEGOTIATION)
    if( ( ret = ssl_check_ctr_renegotiate( ssl ) ) != 0 )
    {
        SSL_DEBUG_RET( 1, "ssl_check_ctr_renegotiate", ret );
        return( ret );
    }
#endif

    if( ssl->state != SSL_HANDSHAKE_OVER )
    {
        if( ( ret = ssl_handshake( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_handshake", ret );
            return( ret );
        }
    }

#if defined(POLARSSL_SSL_MAX_FRAGMENT_LENGTH)
    /*
     * Assume mfl_code is correct since it was checked when set
     */
    max_len = mfl_code_to_length[ssl->mfl_code];

    /*
     * Check if a smaller max length was negotiated
     */
    if( ssl->session_out != NULL &&
        mfl_code_to_length[ssl->session_out->mfl_code] < max_len )
    {
        max_len = mfl_code_to_length[ssl->session_out->mfl_code];
    }
#endif /* POLARSSL_SSL_MAX_FRAGMENT_LENGTH */

    n = ( len < max_len) ? len : max_len;

    if( ssl->out_left != 0 )
    {
        if( ( ret = ssl_flush_output( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_flush_output", ret );
            return( ret );
        }
    }
    else
    {
        ssl->out_msglen  = n;
        ssl->out_msgtype = SSL_MSG_APPLICATION_DATA;
        memcpy( ssl->out_msg, buf, n );

        if( ( ret = ssl_write_record( ssl ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_write_record", ret );
            return( ret );
        }
    }

    SSL_DEBUG_MSG( 2, ( "<= write" ) );

    return( (int) n );
}

/*
 * Write application data, doing 1/n-1 splitting if necessary.
 *
 * With non-blocking I/O, ssl_write_real() may return WANT_WRITE,
 * then the caller will call us again with the same arguments, so
 * remember wether we already did the split or not.
 */
#if defined(POLARSSL_SSL_CBC_RECORD_SPLITTING)
int ssl_write( ssl_context *ssl, const unsigned char *buf, size_t len )
{
    int ret;

    if( ssl->split_done == SSL_CBC_RECORD_SPLITTING_DISABLED ||
        len <= 1 ||
        ssl->minor_ver > SSL_MINOR_VERSION_1 ||
        cipher_get_cipher_mode( &ssl->transform_out->cipher_ctx_enc )
                                != POLARSSL_MODE_CBC )
    {
        return( ssl_write_real( ssl, buf, len ) );
    }

    if( ssl->split_done == 0 )
    {
        if( ( ret = ssl_write_real( ssl, buf, 1 ) ) <= 0 )
            return( ret );
        ssl->split_done = 1;
    }

    if( ( ret = ssl_write_real( ssl, buf + 1, len - 1 ) ) <= 0 )
        return( ret );
    ssl->split_done = 0;

    return( ret + 1 );
}
#endif /* POLARSSL_SSL_CBC_RECORD_SPLITTING */

/*
 * Notify the peer that the connection is being closed
 */
int ssl_close_notify( ssl_context *ssl )
{
    int ret;

    SSL_DEBUG_MSG( 2, ( "=> write close notify" ) );

    if( ssl->out_left != 0 )
        return( ssl_flush_output( ssl ) );

    if( ssl->state == SSL_HANDSHAKE_OVER )
    {
        if( ( ret = ssl_send_alert_message( ssl,
                        SSL_ALERT_LEVEL_WARNING,
                        SSL_ALERT_MSG_CLOSE_NOTIFY ) ) != 0 )
        {
            SSL_DEBUG_RET( 1, "ssl_send_alert_message", ret );
            return( ret );
        }
    }

    SSL_DEBUG_MSG( 2, ( "<= write close notify" ) );

    return( 0 );
}

void ssl_transform_free( ssl_transform *transform )
{
    if( transform == NULL )
        return;

#if defined(POLARSSL_ZLIB_SUPPORT)
    deflateEnd( &transform->ctx_deflate );
    inflateEnd( &transform->ctx_inflate );
#endif

    cipher_free( &transform->cipher_ctx_enc );
    cipher_free( &transform->cipher_ctx_dec );

    md_free( &transform->md_ctx_enc );
    md_free( &transform->md_ctx_dec );

    polarssl_zeroize( transform, sizeof( ssl_transform ) );
}

#if defined(POLARSSL_X509_CRT_PARSE_C)
static void ssl_key_cert_free( ssl_key_cert *key_cert )
{
    ssl_key_cert *cur = key_cert, *next;

    while( cur != NULL )
    {
        next = cur->next;

        if( cur->key_own_alloc )
        {
            pk_free( cur->key );
            polarssl_free( cur->key );
        }
        polarssl_free( cur );

        cur = next;
    }
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

void ssl_handshake_free( ssl_handshake_params *handshake )
{
    if( handshake == NULL )
        return;

#if defined(POLARSSL_DHM_C)
    dhm_free( &handshake->dhm_ctx );
#endif
#if defined(POLARSSL_ECDH_C)
    ecdh_free( &handshake->ecdh_ctx );
#endif

#if defined(POLARSSL_ECDH_C) || defined(POLARSSL_ECDSA_C)
    /* explicit void pointer cast for buggy MS compiler */
    polarssl_free( (void *) handshake->curves );
#endif

#if defined(POLARSSL_X509_CRT_PARSE_C) && \
    defined(POLARSSL_SSL_SERVER_NAME_INDICATION)
    /*
     * Free only the linked list wrapper, not the keys themselves
     * since the belong to the SNI callback
     */
    if( handshake->sni_key_cert != NULL )
    {
        ssl_key_cert *cur = handshake->sni_key_cert, *next;

        while( cur != NULL )
        {
            next = cur->next;
            polarssl_free( cur );
            cur = next;
        }
    }
#endif /* POLARSSL_X509_CRT_PARSE_C && POLARSSL_SSL_SERVER_NAME_INDICATION */

    polarssl_zeroize( handshake, sizeof( ssl_handshake_params ) );
}

void ssl_session_free( ssl_session *session )
{
    if( session == NULL )
        return;

#if defined(POLARSSL_X509_CRT_PARSE_C)
    if( session->peer_cert != NULL )
    {
        x509_crt_free( session->peer_cert );
        polarssl_free( session->peer_cert );
    }
#endif

#if defined(POLARSSL_SSL_SESSION_TICKETS)
    polarssl_free( session->ticket );
#endif

    polarssl_zeroize( session, sizeof( ssl_session ) );
}

/*
 * Free an SSL context
 */
void ssl_free( ssl_context *ssl )
{
    if( ssl == NULL )
        return;

    SSL_DEBUG_MSG( 2, ( "=> free" ) );

    if( ssl->out_ctr != NULL )
    {
        polarssl_zeroize( ssl->out_ctr, SSL_BUFFER_LEN );
        polarssl_free( ssl->out_ctr );
    }

    if( ssl->in_ctr != NULL )
    {
        polarssl_zeroize( ssl->in_ctr, SSL_BUFFER_LEN );
        polarssl_free( ssl->in_ctr );
    }

#if defined(POLARSSL_ZLIB_SUPPORT)
    if( ssl->compress_buf != NULL )
    {
        polarssl_zeroize( ssl->compress_buf, SSL_BUFFER_LEN );
        polarssl_free( ssl->compress_buf );
    }
#endif

#if defined(POLARSSL_DHM_C)
    mpi_free( &ssl->dhm_P );
    mpi_free( &ssl->dhm_G );
#endif

    if( ssl->transform )
    {
        ssl_transform_free( ssl->transform );
        polarssl_free( ssl->transform );
    }

    if( ssl->handshake )
    {
        ssl_handshake_free( ssl->handshake );
        ssl_transform_free( ssl->transform_negotiate );
        ssl_session_free( ssl->session_negotiate );

        polarssl_free( ssl->handshake );
        polarssl_free( ssl->transform_negotiate );
        polarssl_free( ssl->session_negotiate );
    }

    if( ssl->session )
    {
        ssl_session_free( ssl->session );
        polarssl_free( ssl->session );
    }

#if defined(POLARSSL_SSL_SESSION_TICKETS)
    if( ssl->ticket_keys )
    {
        ssl_ticket_keys_free( ssl->ticket_keys );
        polarssl_free( ssl->ticket_keys );
    }
#endif

#if defined(POLARSSL_SSL_SERVER_NAME_INDICATION)
    if( ssl->hostname != NULL )
    {
        polarssl_zeroize( ssl->hostname, ssl->hostname_len );
        polarssl_free( ssl->hostname );
        ssl->hostname_len = 0;
    }
#endif

#if defined(POLARSSL_KEY_EXCHANGE__SOME__PSK_ENABLED)
    if( ssl->psk != NULL )
    {
        polarssl_zeroize( ssl->psk, ssl->psk_len );
        polarssl_zeroize( ssl->psk_identity, ssl->psk_identity_len );
        polarssl_free( ssl->psk );
        polarssl_free( ssl->psk_identity );
        ssl->psk_len = 0;
        ssl->psk_identity_len = 0;
    }
#endif

#if defined(POLARSSL_X509_CRT_PARSE_C)
    ssl_key_cert_free( ssl->key_cert );
#endif

#if defined(POLARSSL_SSL_HW_RECORD_ACCEL)
    if( ssl_hw_record_finish != NULL )
    {
        SSL_DEBUG_MSG( 2, ( "going for ssl_hw_record_finish()" ) );
        ssl_hw_record_finish( ssl );
    }
#endif

    SSL_DEBUG_MSG( 2, ( "<= free" ) );

    /* Actually clear after last debug message */
    polarssl_zeroize( ssl, sizeof( ssl_context ) );
}

#if defined(POLARSSL_PK_C)
/*
 * Convert between POLARSSL_PK_XXX and SSL_SIG_XXX
 */
unsigned char ssl_sig_from_pk( pk_context *pk )
{
#if defined(POLARSSL_RSA_C)
    if( pk_can_do( pk, POLARSSL_PK_RSA ) )
        return( SSL_SIG_RSA );
#endif
#if defined(POLARSSL_ECDSA_C)
    if( pk_can_do( pk, POLARSSL_PK_ECDSA ) )
        return( SSL_SIG_ECDSA );
#endif
    return( SSL_SIG_ANON );
}

pk_type_t ssl_pk_alg_from_sig( unsigned char sig )
{
    switch( sig )
    {
#if defined(POLARSSL_RSA_C)
        case SSL_SIG_RSA:
            return( POLARSSL_PK_RSA );
#endif
#if defined(POLARSSL_ECDSA_C)
        case SSL_SIG_ECDSA:
            return( POLARSSL_PK_ECDSA );
#endif
        default:
            return( POLARSSL_PK_NONE );
    }
}
#endif /* POLARSSL_PK_C */

/*
 * Convert between SSL_HASH_XXX and POLARSSL_MD_XXX
 */
md_type_t ssl_md_alg_from_hash( unsigned char hash )
{
    switch( hash )
    {
#if defined(POLARSSL_MD5_C)
        case SSL_HASH_MD5:
            return( POLARSSL_MD_MD5 );
#endif
#if defined(POLARSSL_SHA1_C)
        case SSL_HASH_SHA1:
            return( POLARSSL_MD_SHA1 );
#endif
#if defined(POLARSSL_SHA256_C)
        case SSL_HASH_SHA224:
            return( POLARSSL_MD_SHA224 );
        case SSL_HASH_SHA256:
            return( POLARSSL_MD_SHA256 );
#endif
#if defined(POLARSSL_SHA512_C)
        case SSL_HASH_SHA384:
            return( POLARSSL_MD_SHA384 );
        case SSL_HASH_SHA512:
            return( POLARSSL_MD_SHA512 );
#endif
        default:
            return( POLARSSL_MD_NONE );
    }
}

#if defined(POLARSSL_SSL_SET_CURVES)
/*
 * Check is a curve proposed by the peer is in our list.
 * Return 1 if we're willing to use it, 0 otherwise.
 */
int ssl_curve_is_acceptable( const ssl_context *ssl, ecp_group_id grp_id )
{
    const ecp_group_id *gid;

    for( gid = ssl->curve_list; *gid != POLARSSL_ECP_DP_NONE; gid++ )
        if( *gid == grp_id )
            return( 1 );

    return( 0 );
}
#endif /* POLARSSL_SSL_SET_CURVES */

#if defined(POLARSSL_X509_CRT_PARSE_C)
int ssl_check_cert_usage( const x509_crt *cert,
                          const ssl_ciphersuite_t *ciphersuite,
                          int cert_endpoint )
{
#if defined(POLARSSL_X509_CHECK_KEY_USAGE)
    int usage = 0;
#endif
#if defined(POLARSSL_X509_CHECK_EXTENDED_KEY_USAGE)
    const char *ext_oid;
    size_t ext_len;
#endif

#if !defined(POLARSSL_X509_CHECK_KEY_USAGE) &&          \
    !defined(POLARSSL_X509_CHECK_EXTENDED_KEY_USAGE)
    ((void) cert);
    ((void) cert_endpoint);
#endif

#if defined(POLARSSL_X509_CHECK_KEY_USAGE)
    if( cert_endpoint == SSL_IS_SERVER )
    {
        /* Server part of the key exchange */
        switch( ciphersuite->key_exchange )
        {
            case POLARSSL_KEY_EXCHANGE_RSA:
            case POLARSSL_KEY_EXCHANGE_RSA_PSK:
                usage = KU_KEY_ENCIPHERMENT;
                break;

            case POLARSSL_KEY_EXCHANGE_DHE_RSA:
            case POLARSSL_KEY_EXCHANGE_ECDHE_RSA:
            case POLARSSL_KEY_EXCHANGE_ECDHE_ECDSA:
                usage = KU_DIGITAL_SIGNATURE;
                break;

            case POLARSSL_KEY_EXCHANGE_ECDH_RSA:
            case POLARSSL_KEY_EXCHANGE_ECDH_ECDSA:
                usage = KU_KEY_AGREEMENT;
                break;

            /* Don't use default: we want warnings when adding new values */
            case POLARSSL_KEY_EXCHANGE_NONE:
            case POLARSSL_KEY_EXCHANGE_PSK:
            case POLARSSL_KEY_EXCHANGE_DHE_PSK:
            case POLARSSL_KEY_EXCHANGE_ECDHE_PSK:
                usage = 0;
        }
    }
    else
    {
        /* Client auth: we only implement rsa_sign and ecdsa_sign for now */
        usage = KU_DIGITAL_SIGNATURE;
    }

    if( x509_crt_check_key_usage( cert, usage ) != 0 )
        return( -1 );
#else
    ((void) ciphersuite);
#endif /* POLARSSL_X509_CHECK_KEY_USAGE */

#if defined(POLARSSL_X509_CHECK_EXTENDED_KEY_USAGE)
    if( cert_endpoint == SSL_IS_SERVER )
    {
        ext_oid = OID_SERVER_AUTH;
        ext_len = OID_SIZE( OID_SERVER_AUTH );
    }
    else
    {
        ext_oid = OID_CLIENT_AUTH;
        ext_len = OID_SIZE( OID_CLIENT_AUTH );
    }

    if( x509_crt_check_extended_key_usage( cert, ext_oid, ext_len ) != 0 )
        return( -1 );
#endif /* POLARSSL_X509_CHECK_EXTENDED_KEY_USAGE */

    return( 0 );
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

#endif /* POLARSSL_SSL_TLS_C */
