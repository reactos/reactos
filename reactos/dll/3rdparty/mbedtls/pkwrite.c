/*
 *  Public Key layer for writing key files and structures
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

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_PK_WRITE_C)

#include "polarssl/pk.h"
#include "polarssl/asn1write.h"
#include "polarssl/oid.h"

#if defined(POLARSSL_RSA_C)
#include "polarssl/rsa.h"
#endif
#if defined(POLARSSL_ECP_C)
#include "polarssl/ecp.h"
#endif
#if defined(POLARSSL_ECDSA_C)
#include "polarssl/ecdsa.h"
#endif
#if defined(POLARSSL_PEM_WRITE_C)
#include "polarssl/pem.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdlib.h>
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#if defined(POLARSSL_RSA_C)
/*
 *  RSAPublicKey ::= SEQUENCE {
 *      modulus           INTEGER,  -- n
 *      publicExponent    INTEGER   -- e
 *  }
 */
static int pk_write_rsa_pubkey( unsigned char **p, unsigned char *start,
                                  rsa_context *rsa )
{
    int ret;
    size_t len = 0;

    ASN1_CHK_ADD( len, asn1_write_mpi( p, start, &rsa->E ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( p, start, &rsa->N ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_CONSTRUCTED |
                                                 ASN1_SEQUENCE ) );

    return( (int) len );
}
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
/*
 * EC public key is an EC point
 */
static int pk_write_ec_pubkey( unsigned char **p, unsigned char *start,
                                 ecp_keypair *ec )
{
    int ret;
    size_t len = 0;
    unsigned char buf[POLARSSL_ECP_MAX_PT_LEN];

    if( ( ret = ecp_point_write_binary( &ec->grp, &ec->Q,
                                        POLARSSL_ECP_PF_UNCOMPRESSED,
                                        &len, buf, sizeof( buf ) ) ) != 0 )
    {
        return( ret );
    }

    if( *p - start < (int) len )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    *p -= len;
    memcpy( *p, buf, len );

    return( (int) len );
}

/*
 * ECParameters ::= CHOICE {
 *   namedCurve         OBJECT IDENTIFIER
 * }
 */
static int pk_write_ec_param( unsigned char **p, unsigned char *start,
                                ecp_keypair *ec )
{
    int ret;
    size_t len = 0;
    const char *oid;
    size_t oid_len;

    if( ( ret = oid_get_oid_by_ec_grp( ec->grp.id, &oid, &oid_len ) ) != 0 )
        return( ret );

    ASN1_CHK_ADD( len, asn1_write_oid( p, start, oid, oid_len ) );

    return( (int) len );
}
#endif /* POLARSSL_ECP_C */

int pk_write_pubkey( unsigned char **p, unsigned char *start,
                     const pk_context *key )
{
    int ret;
    size_t len = 0;

#if defined(POLARSSL_RSA_C)
    if( pk_get_type( key ) == POLARSSL_PK_RSA )
        ASN1_CHK_ADD( len, pk_write_rsa_pubkey( p, start, pk_rsa( *key ) ) );
    else
#endif
#if defined(POLARSSL_ECP_C)
    if( pk_get_type( key ) == POLARSSL_PK_ECKEY )
        ASN1_CHK_ADD( len, pk_write_ec_pubkey( p, start, pk_ec( *key ) ) );
    else
#endif
        return( POLARSSL_ERR_PK_FEATURE_UNAVAILABLE );

    return( (int) len );
}

int pk_write_pubkey_der( pk_context *key, unsigned char *buf, size_t size )
{
    int ret;
    unsigned char *c;
    size_t len = 0, par_len = 0, oid_len;
    const char *oid;

    c = buf + size;

    ASN1_CHK_ADD( len, pk_write_pubkey( &c, buf, key ) );

    if( c - buf < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    /*
     *  SubjectPublicKeyInfo  ::=  SEQUENCE  {
     *       algorithm            AlgorithmIdentifier,
     *       subjectPublicKey     BIT STRING }
     */
    *--c = 0;
    len += 1;

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_BIT_STRING ) );

    if( ( ret = oid_get_oid_by_pk_alg( pk_get_type( key ),
                                       &oid, &oid_len ) ) != 0 )
    {
        return( ret );
    }

#if defined(POLARSSL_ECP_C)
    if( pk_get_type( key ) == POLARSSL_PK_ECKEY )
    {
        ASN1_CHK_ADD( par_len, pk_write_ec_param( &c, buf, pk_ec( *key ) ) );
    }
#endif

    ASN1_CHK_ADD( len, asn1_write_algorithm_identifier( &c, buf, oid, oid_len,
                                                        par_len ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED |
                                                ASN1_SEQUENCE ) );

    return( (int) len );
}

int pk_write_key_der( pk_context *key, unsigned char *buf, size_t size )
{
    int ret;
    unsigned char *c = buf + size;
    size_t len = 0;

#if defined(POLARSSL_RSA_C)
    if( pk_get_type( key ) == POLARSSL_PK_RSA )
    {
        rsa_context *rsa = pk_rsa( *key );

        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->QP ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->DQ ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->DP ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->Q ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->P ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->D ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->E ) );
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->N ) );
        ASN1_CHK_ADD( len, asn1_write_int( &c, buf, 0 ) );

        ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );
    }
    else
#endif /* POLARSSL_RSA_C */
#if defined(POLARSSL_ECP_C)
    if( pk_get_type( key ) == POLARSSL_PK_ECKEY )
    {
        ecp_keypair *ec = pk_ec( *key );
        size_t pub_len = 0, par_len = 0;

        /*
         * RFC 5915, or SEC1 Appendix C.4
         *
         * ECPrivateKey ::= SEQUENCE {
         *      version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
         *      privateKey     OCTET STRING,
         *      parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
         *      publicKey  [1] BIT STRING OPTIONAL
         *    }
         */

        /* publicKey */
        ASN1_CHK_ADD( pub_len, pk_write_ec_pubkey( &c, buf, ec ) );

        if( c - buf < 1 )
            return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );
        *--c = 0;
        pub_len += 1;

        ASN1_CHK_ADD( pub_len, asn1_write_len( &c, buf, pub_len ) );
        ASN1_CHK_ADD( pub_len, asn1_write_tag( &c, buf, ASN1_BIT_STRING ) );

        ASN1_CHK_ADD( pub_len, asn1_write_len( &c, buf, pub_len ) );
        ASN1_CHK_ADD( pub_len, asn1_write_tag( &c, buf,
                            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 1 ) );
        len += pub_len;

        /* parameters */
        ASN1_CHK_ADD( par_len, pk_write_ec_param( &c, buf, ec ) );

        ASN1_CHK_ADD( par_len, asn1_write_len( &c, buf, par_len ) );
        ASN1_CHK_ADD( par_len, asn1_write_tag( &c, buf,
                            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0 ) );
        len += par_len;

        /* privateKey: write as MPI then fix tag */
        ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &ec->d ) );
        *c = ASN1_OCTET_STRING;

        /* version */
        ASN1_CHK_ADD( len, asn1_write_int( &c, buf, 1 ) );

        ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
        ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED |
                                                    ASN1_SEQUENCE ) );
    }
    else
#endif /* POLARSSL_ECP_C */
        return( POLARSSL_ERR_PK_FEATURE_UNAVAILABLE );

    return( (int) len );
}

#if defined(POLARSSL_PEM_WRITE_C)

#define PEM_BEGIN_PUBLIC_KEY    "-----BEGIN PUBLIC KEY-----\n"
#define PEM_END_PUBLIC_KEY      "-----END PUBLIC KEY-----\n"

#define PEM_BEGIN_PRIVATE_KEY_RSA   "-----BEGIN RSA PRIVATE KEY-----\n"
#define PEM_END_PRIVATE_KEY_RSA     "-----END RSA PRIVATE KEY-----\n"
#define PEM_BEGIN_PRIVATE_KEY_EC    "-----BEGIN EC PRIVATE KEY-----\n"
#define PEM_END_PRIVATE_KEY_EC      "-----END EC PRIVATE KEY-----\n"

/*
 * Max sizes of key per types. Shown as tag + len (+ content).
 */

#if defined(POLARSSL_RSA_C)
/*
 * RSA public keys:
 *  SubjectPublicKeyInfo  ::=  SEQUENCE  {          1 + 3
 *       algorithm            AlgorithmIdentifier,  1 + 1 (sequence)
 *                                                + 1 + 1 + 9 (rsa oid)
 *                                                + 1 + 1 (params null)
 *       subjectPublicKey     BIT STRING }          1 + 3 + (1 + below)
 *  RSAPublicKey ::= SEQUENCE {                     1 + 3
 *      modulus           INTEGER,  -- n            1 + 3 + MPI_MAX + 1
 *      publicExponent    INTEGER   -- e            1 + 3 + MPI_MAX + 1
 *  }
 */
#define RSA_PUB_DER_MAX_BYTES   38 + 2 * POLARSSL_MPI_MAX_SIZE

/*
 * RSA private keys:
 *  RSAPrivateKey ::= SEQUENCE {                    1 + 3
 *      version           Version,                  1 + 1 + 1
 *      modulus           INTEGER,                  1 + 3 + MPI_MAX + 1
 *      publicExponent    INTEGER,                  1 + 3 + MPI_MAX + 1
 *      privateExponent   INTEGER,                  1 + 3 + MPI_MAX + 1
 *      prime1            INTEGER,                  1 + 3 + MPI_MAX / 2 + 1
 *      prime2            INTEGER,                  1 + 3 + MPI_MAX / 2 + 1
 *      exponent1         INTEGER,                  1 + 3 + MPI_MAX / 2 + 1
 *      exponent2         INTEGER,                  1 + 3 + MPI_MAX / 2 + 1
 *      coefficient       INTEGER,                  1 + 3 + MPI_MAX / 2 + 1
 *      otherPrimeInfos   OtherPrimeInfos OPTIONAL  0 (not supported)
 *  }
 */
#define MPI_MAX_SIZE_2          POLARSSL_MPI_MAX_SIZE / 2 + \
                                POLARSSL_MPI_MAX_SIZE % 2
#define RSA_PRV_DER_MAX_BYTES   47 + 3 * POLARSSL_MPI_MAX_SIZE \
                                   + 5 * MPI_MAX_SIZE_2

#else /* POLARSSL_RSA_C */

#define RSA_PUB_DER_MAX_BYTES   0
#define RSA_PRV_DER_MAX_BYTES   0

#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
/*
 * EC public keys:
 *  SubjectPublicKeyInfo  ::=  SEQUENCE  {      1 + 2
 *    algorithm         AlgorithmIdentifier,    1 + 1 (sequence)
 *                                            + 1 + 1 + 7 (ec oid)
 *                                            + 1 + 1 + 9 (namedCurve oid)
 *    subjectPublicKey  BIT STRING              1 + 2 + 1               [1]
 *                                            + 1 (point format)        [1]
 *                                            + 2 * ECP_MAX (coords)    [1]
 *  }
 */
#define ECP_PUB_DER_MAX_BYTES   30 + 2 * POLARSSL_ECP_MAX_BYTES

/*
 * EC private keys:
 * ECPrivateKey ::= SEQUENCE {                  1 + 2
 *      version        INTEGER ,                1 + 1 + 1
 *      privateKey     OCTET STRING,            1 + 1 + ECP_MAX
 *      parameters [0] ECParameters OPTIONAL,   1 + 1 + (1 + 1 + 9)
 *      publicKey  [1] BIT STRING OPTIONAL      1 + 2 + [1] above
 *    }
 */
#define ECP_PRV_DER_MAX_BYTES   29 + 3 * POLARSSL_ECP_MAX_BYTES

#else /* POLARSSL_ECP_C */

#define ECP_PUB_DER_MAX_BYTES   0
#define ECP_PRV_DER_MAX_BYTES   0

#endif /* POLARSSL_ECP_C */

#define PUB_DER_MAX_BYTES   RSA_PUB_DER_MAX_BYTES > ECP_PUB_DER_MAX_BYTES ? \
                            RSA_PUB_DER_MAX_BYTES : ECP_PUB_DER_MAX_BYTES
#define PRV_DER_MAX_BYTES   RSA_PRV_DER_MAX_BYTES > ECP_PRV_DER_MAX_BYTES ? \
                            RSA_PRV_DER_MAX_BYTES : ECP_PRV_DER_MAX_BYTES

int pk_write_pubkey_pem( pk_context *key, unsigned char *buf, size_t size )
{
    int ret;
    unsigned char output_buf[PUB_DER_MAX_BYTES];
    size_t olen = 0;

    if( ( ret = pk_write_pubkey_der( key, output_buf,
                                     sizeof(output_buf) ) ) < 0 )
    {
        return( ret );
    }

    if( ( ret = pem_write_buffer( PEM_BEGIN_PUBLIC_KEY, PEM_END_PUBLIC_KEY,
                                  output_buf + sizeof(output_buf) - ret,
                                  ret, buf, size, &olen ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int pk_write_key_pem( pk_context *key, unsigned char *buf, size_t size )
{
    int ret;
    unsigned char output_buf[PRV_DER_MAX_BYTES];
    const char *begin, *end;
    size_t olen = 0;

    if( ( ret = pk_write_key_der( key, output_buf, sizeof(output_buf) ) ) < 0 )
        return( ret );

#if defined(POLARSSL_RSA_C)
    if( pk_get_type( key ) == POLARSSL_PK_RSA )
    {
        begin = PEM_BEGIN_PRIVATE_KEY_RSA;
        end = PEM_END_PRIVATE_KEY_RSA;
    }
    else
#endif
#if defined(POLARSSL_ECP_C)
    if( pk_get_type( key ) == POLARSSL_PK_ECKEY )
    {
        begin = PEM_BEGIN_PRIVATE_KEY_EC;
        end = PEM_END_PRIVATE_KEY_EC;
    }
    else
#endif
        return( POLARSSL_ERR_PK_FEATURE_UNAVAILABLE );

    if( ( ret = pem_write_buffer( begin, end,
                                  output_buf + sizeof(output_buf) - ret,
                                  ret, buf, size, &olen ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_PEM_WRITE_C */

#endif /* POLARSSL_PK_WRITE_C */
