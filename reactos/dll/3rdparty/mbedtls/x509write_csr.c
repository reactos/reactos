/*
 *  X.509 Certificate Signing Request writing
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
 * References:
 * - CSRs: PKCS#10 v1.7 aka RFC 2986
 * - attributes: PKCS#9 v2.0 aka RFC 2985
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_X509_CSR_WRITE_C)

#include "polarssl/x509_csr.h"
#include "polarssl/oid.h"
#include "polarssl/asn1write.h"

#if defined(POLARSSL_PEM_WRITE_C)
#include "polarssl/pem.h"
#endif

#include <string.h>
#include <stdlib.h>

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

void x509write_csr_init( x509write_csr *ctx )
{
    memset( ctx, 0, sizeof(x509write_csr) );
}

void x509write_csr_free( x509write_csr *ctx )
{
    asn1_free_named_data_list( &ctx->subject );
    asn1_free_named_data_list( &ctx->extensions );

    polarssl_zeroize( ctx, sizeof(x509write_csr) );
}

void x509write_csr_set_md_alg( x509write_csr *ctx, md_type_t md_alg )
{
    ctx->md_alg = md_alg;
}

void x509write_csr_set_key( x509write_csr *ctx, pk_context *key )
{
    ctx->key = key;
}

int x509write_csr_set_subject_name( x509write_csr *ctx,
                                    const char *subject_name )
{
    return x509_string_to_names( &ctx->subject, subject_name );
}

int x509write_csr_set_extension( x509write_csr *ctx,
                                 const char *oid, size_t oid_len,
                                 const unsigned char *val, size_t val_len )
{
    return x509_set_extension( &ctx->extensions, oid, oid_len,
                               0, val, val_len );
}

int x509write_csr_set_key_usage( x509write_csr *ctx, unsigned char key_usage )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = asn1_write_bitstring( &c, buf, &key_usage, 7 ) ) != 4 )
        return( ret );

    ret = x509write_csr_set_extension( ctx, OID_KEY_USAGE,
                                       OID_SIZE( OID_KEY_USAGE ),
                                       buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}

int x509write_csr_set_ns_cert_type( x509write_csr *ctx,
                                    unsigned char ns_cert_type )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = asn1_write_bitstring( &c, buf, &ns_cert_type, 8 ) ) != 4 )
        return( ret );

    ret = x509write_csr_set_extension( ctx, OID_NS_CERT_TYPE,
                                       OID_SIZE( OID_NS_CERT_TYPE ),
                                       buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}

int x509write_csr_der( x509write_csr *ctx, unsigned char *buf, size_t size,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng )
{
    int ret;
    const char *sig_oid;
    size_t sig_oid_len = 0;
    unsigned char *c, *c2;
    unsigned char hash[64];
    unsigned char sig[POLARSSL_MPI_MAX_SIZE];
    unsigned char tmp_buf[2048];
    size_t pub_len = 0, sig_and_oid_len = 0, sig_len;
    size_t len = 0;
    pk_type_t pk_alg;

    /*
     * Prepare data to be signed in tmp_buf
     */
    c = tmp_buf + sizeof( tmp_buf );

    ASN1_CHK_ADD( len, x509_write_extensions( &c, tmp_buf, ctx->extensions ) );

    if( len )
    {
        ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                        ASN1_SEQUENCE ) );

        ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                        ASN1_SET ) );

        ASN1_CHK_ADD( len, asn1_write_oid( &c, tmp_buf, OID_PKCS9_CSR_EXT_REQ,
                                          OID_SIZE( OID_PKCS9_CSR_EXT_REQ ) ) );

        ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                        ASN1_SEQUENCE ) );
    }

    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                    ASN1_CONTEXT_SPECIFIC ) );

    ASN1_CHK_ADD( pub_len, pk_write_pubkey_der( ctx->key,
                                                tmp_buf, c - tmp_buf ) );
    c -= pub_len;
    len += pub_len;

    /*
     *  Subject  ::=  Name
     */
    ASN1_CHK_ADD( len, x509_write_names( &c, tmp_buf, ctx->subject ) );

    /*
     *  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     */
    ASN1_CHK_ADD( len, asn1_write_int( &c, tmp_buf, 0 ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );

    /*
     * Prepare signature
     */
    md( md_info_from_type( ctx->md_alg ), c, len, hash );

    pk_alg = pk_get_type( ctx->key );
    if( pk_alg == POLARSSL_PK_ECKEY )
        pk_alg = POLARSSL_PK_ECDSA;

    if( ( ret = pk_sign( ctx->key, ctx->md_alg, hash, 0, sig, &sig_len,
                         f_rng, p_rng ) ) != 0 ||
        ( ret = oid_get_oid_by_sig_alg( pk_alg, ctx->md_alg,
                                        &sig_oid, &sig_oid_len ) ) != 0 )
    {
        return( ret );
    }

    /*
     * Write data to output buffer
     */
    c2 = buf + size;
    ASN1_CHK_ADD( sig_and_oid_len, x509_write_sig( &c2, buf,
                                        sig_oid, sig_oid_len, sig, sig_len ) );

    c2 -= len;
    memcpy( c2, c, len );

    len += sig_and_oid_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c2, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c2, buf, ASN1_CONSTRUCTED |
                                                 ASN1_SEQUENCE ) );

    return( (int) len );
}

#define PEM_BEGIN_CSR           "-----BEGIN CERTIFICATE REQUEST-----\n"
#define PEM_END_CSR             "-----END CERTIFICATE REQUEST-----\n"

#if defined(POLARSSL_PEM_WRITE_C)
int x509write_csr_pem( x509write_csr *ctx, unsigned char *buf, size_t size,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng )
{
    int ret;
    unsigned char output_buf[4096];
    size_t olen = 0;

    if( ( ret = x509write_csr_der( ctx, output_buf, sizeof(output_buf),
                                   f_rng, p_rng ) ) < 0 )
    {
        return( ret );
    }

    if( ( ret = pem_write_buffer( PEM_BEGIN_CSR, PEM_END_CSR,
                                  output_buf + sizeof(output_buf) - ret,
                                  ret, buf, size, &olen ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_PEM_WRITE_C */

#endif /* POLARSSL_X509_CSR_WRITE_C */
