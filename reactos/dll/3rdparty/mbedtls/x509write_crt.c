/*
 *  X.509 certificate writing
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
 * - certificates: RFC 5280, updated by RFC 6818
 * - CSRs: PKCS#10 v1.7 aka RFC 2986
 * - attributes: PKCS#9 v2.0 aka RFC 2985
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_X509_CRT_WRITE_C)

#include "polarssl/x509_crt.h"
#include "polarssl/oid.h"
#include "polarssl/asn1write.h"
#include "polarssl/sha1.h"

#if defined(POLARSSL_PEM_WRITE_C)
#include "polarssl/pem.h"
#endif /* POLARSSL_PEM_WRITE_C */

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

void x509write_crt_init( x509write_cert *ctx )
{
    memset( ctx, 0, sizeof(x509write_cert) );

    mpi_init( &ctx->serial );
    ctx->version = X509_CRT_VERSION_3;
}

void x509write_crt_free( x509write_cert *ctx )
{
    mpi_free( &ctx->serial );

    asn1_free_named_data_list( &ctx->subject );
    asn1_free_named_data_list( &ctx->issuer );
    asn1_free_named_data_list( &ctx->extensions );

    polarssl_zeroize( ctx, sizeof(x509write_cert) );
}

void x509write_crt_set_version( x509write_cert *ctx, int version )
{
    ctx->version = version;
}

void x509write_crt_set_md_alg( x509write_cert *ctx, md_type_t md_alg )
{
    ctx->md_alg = md_alg;
}

void x509write_crt_set_subject_key( x509write_cert *ctx, pk_context *key )
{
    ctx->subject_key = key;
}

void x509write_crt_set_issuer_key( x509write_cert *ctx, pk_context *key )
{
    ctx->issuer_key = key;
}

int x509write_crt_set_subject_name( x509write_cert *ctx,
                                    const char *subject_name )
{
    return x509_string_to_names( &ctx->subject, subject_name );
}

int x509write_crt_set_issuer_name( x509write_cert *ctx,
                                   const char *issuer_name )
{
    return x509_string_to_names( &ctx->issuer, issuer_name );
}

int x509write_crt_set_serial( x509write_cert *ctx, const mpi *serial )
{
    int ret;

    if( ( ret = mpi_copy( &ctx->serial, serial ) ) != 0 )
        return( ret );

    return( 0 );
}

int x509write_crt_set_validity( x509write_cert *ctx, const char *not_before,
                                const char *not_after )
{
    if( strlen( not_before ) != X509_RFC5280_UTC_TIME_LEN - 1 ||
        strlen( not_after )  != X509_RFC5280_UTC_TIME_LEN - 1 )
    {
        return( POLARSSL_ERR_X509_BAD_INPUT_DATA );
    }
    strncpy( ctx->not_before, not_before, X509_RFC5280_UTC_TIME_LEN );
    strncpy( ctx->not_after , not_after , X509_RFC5280_UTC_TIME_LEN );
    ctx->not_before[X509_RFC5280_UTC_TIME_LEN - 1] = 'Z';
    ctx->not_after[X509_RFC5280_UTC_TIME_LEN - 1] = 'Z';

    return( 0 );
}

int x509write_crt_set_extension( x509write_cert *ctx,
                                 const char *oid, size_t oid_len,
                                 int critical,
                                 const unsigned char *val, size_t val_len )
{
    return x509_set_extension( &ctx->extensions, oid, oid_len,
                               critical, val, val_len );
}

int x509write_crt_set_basic_constraints( x509write_cert *ctx,
                                         int is_ca, int max_pathlen )
{
    int ret;
    unsigned char buf[9];
    unsigned char *c = buf + sizeof(buf);
    size_t len = 0;

    memset( buf, 0, sizeof(buf) );

    if( is_ca && max_pathlen > 127 )
        return( POLARSSL_ERR_X509_BAD_INPUT_DATA );

    if( is_ca )
    {
        if( max_pathlen >= 0 )
        {
            ASN1_CHK_ADD( len, asn1_write_int( &c, buf, max_pathlen ) );
        }
        ASN1_CHK_ADD( len, asn1_write_bool( &c, buf, 1 ) );
    }

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED |
                                                ASN1_SEQUENCE ) );

    return x509write_crt_set_extension( ctx, OID_BASIC_CONSTRAINTS,
                                        OID_SIZE( OID_BASIC_CONSTRAINTS ),
                                        0, buf + sizeof(buf) - len, len );
}

#if defined(POLARSSL_SHA1_C)
int x509write_crt_set_subject_key_identifier( x509write_cert *ctx )
{
    int ret;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE * 2 + 20]; /* tag, length + 2xMPI */
    unsigned char *c = buf + sizeof(buf);
    size_t len = 0;

    memset( buf, 0, sizeof(buf) );
    ASN1_CHK_ADD( len, pk_write_pubkey( &c, buf, ctx->subject_key ) );

    sha1( buf + sizeof(buf) - len, len, buf + sizeof(buf) - 20 );
    c = buf + sizeof(buf) - 20;
    len = 20;

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_OCTET_STRING ) );

    return x509write_crt_set_extension( ctx, OID_SUBJECT_KEY_IDENTIFIER,
                                        OID_SIZE( OID_SUBJECT_KEY_IDENTIFIER ),
                                        0, buf + sizeof(buf) - len, len );
}

int x509write_crt_set_authority_key_identifier( x509write_cert *ctx )
{
    int ret;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE * 2 + 20]; /* tag, length + 2xMPI */
    unsigned char *c = buf + sizeof(buf);
    size_t len = 0;

    memset( buf, 0, sizeof(buf) );
    ASN1_CHK_ADD( len, pk_write_pubkey( &c, buf, ctx->issuer_key ) );

    sha1( buf + sizeof(buf) - len, len, buf + sizeof(buf) - 20 );
    c = buf + sizeof(buf) - 20;
    len = 20;

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONTEXT_SPECIFIC | 0 ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED |
                                                ASN1_SEQUENCE ) );

    return x509write_crt_set_extension( ctx, OID_AUTHORITY_KEY_IDENTIFIER,
                                   OID_SIZE( OID_AUTHORITY_KEY_IDENTIFIER ),
                                   0, buf + sizeof(buf) - len, len );
}
#endif /* POLARSSL_SHA1_C */

int x509write_crt_set_key_usage( x509write_cert *ctx, unsigned char key_usage )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = asn1_write_bitstring( &c, buf, &key_usage, 7 ) ) != 4 )
        return( ret );

    ret = x509write_crt_set_extension( ctx, OID_KEY_USAGE,
                                       OID_SIZE( OID_KEY_USAGE ),
                                       1, buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}

int x509write_crt_set_ns_cert_type( x509write_cert *ctx,
                                    unsigned char ns_cert_type )
{
    unsigned char buf[4];
    unsigned char *c;
    int ret;

    c = buf + 4;

    if( ( ret = asn1_write_bitstring( &c, buf, &ns_cert_type, 8 ) ) != 4 )
        return( ret );

    ret = x509write_crt_set_extension( ctx, OID_NS_CERT_TYPE,
                                       OID_SIZE( OID_NS_CERT_TYPE ),
                                       0, buf, 4 );
    if( ret != 0 )
        return( ret );

    return( 0 );
}

static int x509_write_time( unsigned char **p, unsigned char *start,
                            const char *time, size_t size )
{
    int ret;
    size_t len = 0;

    /*
     * write ASN1_UTC_TIME if year < 2050 (2 bytes shorter)
     */
    if( time[0] == '2' && time[1] == '0' && time [2] < '5' )
    {
        ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start,
                                             (const unsigned char *) time + 2,
                                             size - 2 ) );
        ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_UTC_TIME ) );
    }
    else
    {
        ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start,
                                                  (const unsigned char *) time,
                                                  size ) );
        ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_GENERALIZED_TIME ) );
    }

    return( (int) len );
}

int x509write_crt_der( x509write_cert *ctx, unsigned char *buf, size_t size,
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
    size_t sub_len = 0, pub_len = 0, sig_and_oid_len = 0, sig_len;
    size_t len = 0;
    pk_type_t pk_alg;

    /*
     * Prepare data to be signed in tmp_buf
     */
    c = tmp_buf + sizeof( tmp_buf );

    /* Signature algorithm needed in TBS, and later for actual signature */
    pk_alg = pk_get_type( ctx->issuer_key );
    if( pk_alg == POLARSSL_PK_ECKEY )
        pk_alg = POLARSSL_PK_ECDSA;

    if( ( ret = oid_get_oid_by_sig_alg( pk_alg, ctx->md_alg,
                                        &sig_oid, &sig_oid_len ) ) != 0 )
    {
        return( ret );
    }

    /*
     *  Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
     */
    ASN1_CHK_ADD( len, x509_write_extensions( &c, tmp_buf, ctx->extensions ) );
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONTEXT_SPECIFIC |
                                                    ASN1_CONSTRUCTED | 3 ) );

    /*
     *  SubjectPublicKeyInfo
     */
    ASN1_CHK_ADD( pub_len, pk_write_pubkey_der( ctx->subject_key,
                                                tmp_buf, c - tmp_buf ) );
    c -= pub_len;
    len += pub_len;

    /*
     *  Subject  ::=  Name
     */
    ASN1_CHK_ADD( len, x509_write_names( &c, tmp_buf, ctx->subject ) );

    /*
     *  Validity ::= SEQUENCE {
     *       notBefore      Time,
     *       notAfter       Time }
     */
    sub_len = 0;

    ASN1_CHK_ADD( sub_len, x509_write_time( &c, tmp_buf, ctx->not_after,
                                            X509_RFC5280_UTC_TIME_LEN ) );

    ASN1_CHK_ADD( sub_len, x509_write_time( &c, tmp_buf, ctx->not_before,
                                            X509_RFC5280_UTC_TIME_LEN ) );

    len += sub_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, sub_len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );

    /*
     *  Issuer  ::=  Name
     */
    ASN1_CHK_ADD( len, x509_write_names( &c, tmp_buf, ctx->issuer ) );

    /*
     *  Signature   ::=  AlgorithmIdentifier
     */
    ASN1_CHK_ADD( len, asn1_write_algorithm_identifier( &c, tmp_buf,
                       sig_oid, strlen( sig_oid ), 0 ) );

    /*
     *  Serial   ::=  INTEGER
     */
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, tmp_buf, &ctx->serial ) );

    /*
     *  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     */
    sub_len = 0;
    ASN1_CHK_ADD( sub_len, asn1_write_int( &c, tmp_buf, ctx->version ) );
    len += sub_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, sub_len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONTEXT_SPECIFIC |
                                                    ASN1_CONSTRUCTED | 0 ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );

    /*
     * Make signature
     */
    md( md_info_from_type( ctx->md_alg ), c, len, hash );

    if( ( ret = pk_sign( ctx->issuer_key, ctx->md_alg, hash, 0, sig, &sig_len,
                         f_rng, p_rng ) ) != 0 )
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

#define PEM_BEGIN_CRT           "-----BEGIN CERTIFICATE-----\n"
#define PEM_END_CRT             "-----END CERTIFICATE-----\n"

#if defined(POLARSSL_PEM_WRITE_C)
int x509write_crt_pem( x509write_cert *crt, unsigned char *buf, size_t size,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng )
{
    int ret;
    unsigned char output_buf[4096];
    size_t olen = 0;

    if( ( ret = x509write_crt_der( crt, output_buf, sizeof(output_buf),
                                   f_rng, p_rng ) ) < 0 )
    {
        return( ret );
    }

    if( ( ret = pem_write_buffer( PEM_BEGIN_CRT, PEM_END_CRT,
                                  output_buf + sizeof(output_buf) - ret,
                                  ret, buf, size, &olen ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_PEM_WRITE_C */

#endif /* POLARSSL_X509_CRT_WRITE_C */
