/*
 *  Public Key layer for parsing key files and structures
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

#if defined(POLARSSL_PK_PARSE_C)

#include "polarssl/pk.h"
#include "polarssl/asn1.h"
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
#if defined(POLARSSL_PEM_PARSE_C)
#include "polarssl/pem.h"
#endif
#if defined(POLARSSL_PKCS5_C)
#include "polarssl/pkcs5.h"
#endif
#if defined(POLARSSL_PKCS12_C)
#include "polarssl/pkcs12.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdlib.h>
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#if defined(POLARSSL_FS_IO)
/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * Load all data from a file into a given buffer.
 */
int pk_load_file( const char *path, unsigned char **buf, size_t *n )
{
    FILE *f;
    long size;

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( POLARSSL_ERR_PK_FILE_IO_ERROR );

    fseek( f, 0, SEEK_END );
    if( ( size = ftell( f ) ) == -1 )
    {
        fclose( f );
        return( POLARSSL_ERR_PK_FILE_IO_ERROR );
    }
    fseek( f, 0, SEEK_SET );

    *n = (size_t) size;

    if( *n + 1 == 0 ||
        ( *buf = (unsigned char *) polarssl_malloc( *n + 1 ) ) == NULL )
    {
        fclose( f );
        return( POLARSSL_ERR_PK_MALLOC_FAILED );
    }

    if( fread( *buf, 1, *n, f ) != *n )
    {
        fclose( f );
        polarssl_free( *buf );
        return( POLARSSL_ERR_PK_FILE_IO_ERROR );
    }

    fclose( f );

    (*buf)[*n] = '\0';

    return( 0 );
}

/*
 * Load and parse a private key
 */
int pk_parse_keyfile( pk_context *ctx,
                      const char *path, const char *pwd )
{
    int ret;
    size_t n;
    unsigned char *buf;

    if( ( ret = pk_load_file( path, &buf, &n ) ) != 0 )
        return( ret );

    if( pwd == NULL )
        ret = pk_parse_key( ctx, buf, n, NULL, 0 );
    else
        ret = pk_parse_key( ctx, buf, n,
                (const unsigned char *) pwd, strlen( pwd ) );

    polarssl_zeroize( buf, n + 1 );
    polarssl_free( buf );

    return( ret );
}

/*
 * Load and parse a public key
 */
int pk_parse_public_keyfile( pk_context *ctx, const char *path )
{
    int ret;
    size_t n;
    unsigned char *buf;

    if( ( ret = pk_load_file( path, &buf, &n ) ) != 0 )
        return( ret );

    ret = pk_parse_public_key( ctx, buf, n );

    polarssl_zeroize( buf, n + 1 );
    polarssl_free( buf );

    return( ret );
}
#endif /* POLARSSL_FS_IO */

#if defined(POLARSSL_ECP_C)
/* Minimally parse an ECParameters buffer to and asn1_buf
 *
 * ECParameters ::= CHOICE {
 *   namedCurve         OBJECT IDENTIFIER
 *   specifiedCurve     SpecifiedECDomain -- = SEQUENCE { ... }
 *   -- implicitCurve   NULL
 * }
 */
static int pk_get_ecparams( unsigned char **p, const unsigned char *end,
                            asn1_buf *params )
{
    int ret;

    /* Tag may be either OID or SEQUENCE */
    params->tag = **p;
    if( params->tag != ASN1_OID
#if defined(POLARSSL_PK_PARSE_EC_EXTENDED)
            && params->tag != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE )
#endif
            )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );
    }

    if( ( ret = asn1_get_tag( p, end, &params->len, params->tag ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    params->p = *p;
    *p += params->len;

    if( *p != end )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

#if defined(POLARSSL_PK_PARSE_EC_EXTENDED)
/*
 * Parse a SpecifiedECDomain (SEC 1 C.2) and (mostly) fill the group with it.
 * WARNING: the resulting group should only be used with
 * pk_group_id_from_specified(), since its base point may not be set correctly
 * if it was encoded compressed.
 *
 *  SpecifiedECDomain ::= SEQUENCE {
 *      version SpecifiedECDomainVersion(ecdpVer1 | ecdpVer2 | ecdpVer3, ...),
 *      fieldID FieldID {{FieldTypes}},
 *      curve Curve,
 *      base ECPoint,
 *      order INTEGER,
 *      cofactor INTEGER OPTIONAL,
 *      hash HashAlgorithm OPTIONAL,
 *      ...
 *  }
 *
 * We only support prime-field as field type, and ignore hash and cofactor.
 */
static int pk_group_from_specified( const asn1_buf *params, ecp_group *grp )
{
    int ret;
    unsigned char *p = params->p;
    const unsigned char * const end = params->p + params->len;
    const unsigned char *end_field, *end_curve;
    size_t len;
    int ver;

    /* SpecifiedECDomainVersion ::= INTEGER { 1, 2, 3 } */
    if( ( ret = asn1_get_int( &p, end, &ver ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( ver < 1 || ver > 3 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );

    /*
     * FieldID { FIELD-ID:IOSet } ::= SEQUENCE { -- Finite field
     *       fieldType FIELD-ID.&id({IOSet}),
     *       parameters FIELD-ID.&Type({IOSet}{@fieldType})
     * }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( ret );

    end_field = p + len;

    /*
     * FIELD-ID ::= TYPE-IDENTIFIER
     * FieldTypes FIELD-ID ::= {
     *       { Prime-p IDENTIFIED BY prime-field } |
     *       { Characteristic-two IDENTIFIED BY characteristic-two-field }
     * }
     * prime-field OBJECT IDENTIFIER ::= { id-fieldType 1 }
     */
    if( ( ret = asn1_get_tag( &p, end_field, &len, ASN1_OID ) ) != 0 )
        return( ret );

    if( len != OID_SIZE( OID_ANSI_X9_62_PRIME_FIELD ) ||
        memcmp( p, OID_ANSI_X9_62_PRIME_FIELD, len ) != 0 )
    {
        return( POLARSSL_ERR_PK_FEATURE_UNAVAILABLE );
    }

    p += len;

    /* Prime-p ::= INTEGER -- Field of size p. */
    if( ( ret = asn1_get_mpi( &p, end_field, &grp->P ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    grp->pbits = mpi_msb( &grp->P );

    if( p != end_field )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    /*
     * Curve ::= SEQUENCE {
     *       a FieldElement,
     *       b FieldElement,
     *       seed BIT STRING OPTIONAL
     *       -- Shall be present if used in SpecifiedECDomain
     *       -- with version equal to ecdpVer2 or ecdpVer3
     * }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( ret );

    end_curve = p + len;

    /*
     * FieldElement ::= OCTET STRING
     * containing an integer in the case of a prime field
     */
    if( ( ret = asn1_get_tag( &p, end_curve, &len, ASN1_OCTET_STRING ) ) != 0 ||
        ( ret = mpi_read_binary( &grp->A, p, len ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    p += len;

    if( ( ret = asn1_get_tag( &p, end_curve, &len, ASN1_OCTET_STRING ) ) != 0 ||
        ( ret = mpi_read_binary( &grp->B, p, len ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    p += len;

    /* Ignore seed BIT STRING OPTIONAL */
    if( ( ret = asn1_get_tag( &p, end_curve, &len, ASN1_BIT_STRING ) ) == 0 )
        p += len;

    if( p != end_curve )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    /*
     * ECPoint ::= OCTET STRING
     */
    if( ( ret = asn1_get_tag( &p, end, &len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( ( ret = ecp_point_read_binary( grp, &grp->G,
                                      ( const unsigned char *) p, len ) ) != 0 )
    {
        /*
         * If we can't read the point because it's compressed, cheat by
         * reading only the X coordinate and the parity bit of Y.
         */
        if( ret != POLARSSL_ERR_ECP_FEATURE_UNAVAILABLE ||
            ( p[0] != 0x02 && p[0] != 0x03 ) ||
            len != mpi_size( &grp->P ) + 1 ||
            mpi_read_binary( &grp->G.X, p + 1, len - 1 ) != 0 ||
            mpi_lset( &grp->G.Y, p[0] - 2 ) != 0 ||
            mpi_lset( &grp->G.Z, 1 ) != 0 )
        {
            return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );
        }
    }

    p += len;

    /*
     * order INTEGER
     */
    if( ( ret = asn1_get_mpi( &p, end, &grp->N ) ) )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    grp->nbits = mpi_msb( &grp->N );

    /*
     * Allow optional elements by purposefully not enforcing p == end here.
     */

    return( 0 );
}

/*
 * Find the group id associated with an (almost filled) group as generated by
 * pk_group_from_specified(), or return an error if unknown.
 */
static int pk_group_id_from_group( const ecp_group *grp, ecp_group_id *grp_id )
{
    int ret = 0;
    ecp_group ref;
    const ecp_group_id *id;

    ecp_group_init( &ref );

    for( id = ecp_grp_id_list(); *id != POLARSSL_ECP_DP_NONE; id++ )
    {
        /* Load the group associated to that id */
        ecp_group_free( &ref );
        MPI_CHK( ecp_use_known_dp( &ref, *id ) );

        /* Compare to the group we were given, starting with easy tests */
        if( grp->pbits == ref.pbits && grp->nbits == ref.nbits &&
            mpi_cmp_mpi( &grp->P, &ref.P ) == 0 &&
            mpi_cmp_mpi( &grp->A, &ref.A ) == 0 &&
            mpi_cmp_mpi( &grp->B, &ref.B ) == 0 &&
            mpi_cmp_mpi( &grp->N, &ref.N ) == 0 &&
            mpi_cmp_mpi( &grp->G.X, &ref.G.X ) == 0 &&
            mpi_cmp_mpi( &grp->G.Z, &ref.G.Z ) == 0 &&
            /* For Y we may only know the parity bit, so compare only that */
            mpi_get_bit( &grp->G.Y, 0 ) == mpi_get_bit( &ref.G.Y, 0 ) )
        {
            break;
        }

    }

cleanup:
    ecp_group_free( &ref );

    *grp_id = *id;

    if( ret == 0 && *id == POLARSSL_ECP_DP_NONE )
        ret = POLARSSL_ERR_ECP_FEATURE_UNAVAILABLE;

    return( ret );
}

/*
 * Parse a SpecifiedECDomain (SEC 1 C.2) and find the associated group ID
 */
static int pk_group_id_from_specified( const asn1_buf *params,
                                       ecp_group_id *grp_id )
{
    int ret;
    ecp_group grp;

    ecp_group_init( &grp );

    if( ( ret = pk_group_from_specified( params, &grp ) ) != 0 )
        goto cleanup;

    ret = pk_group_id_from_group( &grp, grp_id );

cleanup:
    ecp_group_free( &grp );

    return( ret );
}
#endif /* POLARSSL_PK_PARSE_EC_EXTENDED */

/*
 * Use EC parameters to initialise an EC group
 *
 * ECParameters ::= CHOICE {
 *   namedCurve         OBJECT IDENTIFIER
 *   specifiedCurve     SpecifiedECDomain -- = SEQUENCE { ... }
 *   -- implicitCurve   NULL
 */
static int pk_use_ecparams( const asn1_buf *params, ecp_group *grp )
{
    int ret;
    ecp_group_id grp_id;

    if( params->tag == ASN1_OID )
    {
        if( oid_get_ec_grp( params, &grp_id ) != 0 )
            return( POLARSSL_ERR_PK_UNKNOWN_NAMED_CURVE );
    }
    else
    {
#if defined(POLARSSL_PK_PARSE_EC_EXTENDED)
        if( ( ret = pk_group_id_from_specified( params, &grp_id ) ) != 0 )
            return( ret );
#else
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );
#endif
    }

    /*
     * grp may already be initilialized; if so, make sure IDs match
     */
    if( grp->id != POLARSSL_ECP_DP_NONE && grp->id != grp_id )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );

    if( ( ret = ecp_use_known_dp( grp, grp_id ) ) != 0 )
        return( ret );

    return( 0 );
}

/*
 * EC public key is an EC point
 *
 * The caller is responsible for clearing the structure upon failure if
 * desired. Take care to pass along the possible ECP_FEATURE_UNAVAILABLE
 * return code of ecp_point_read_binary() and leave p in a usable state.
 */
static int pk_get_ecpubkey( unsigned char **p, const unsigned char *end,
                            ecp_keypair *key )
{
    int ret;

    if( ( ret = ecp_point_read_binary( &key->grp, &key->Q,
                    (const unsigned char *) *p, end - *p ) ) == 0 )
    {
        ret = ecp_check_pubkey( &key->grp, &key->Q );
    }

    /*
     * We know ecp_point_read_binary consumed all bytes or failed
     */
    *p = (unsigned char *) end;

    return( ret );
}
#endif /* POLARSSL_ECP_C */

#if defined(POLARSSL_RSA_C)
/*
 *  RSAPublicKey ::= SEQUENCE {
 *      modulus           INTEGER,  -- n
 *      publicExponent    INTEGER   -- e
 *  }
 */
static int pk_get_rsapubkey( unsigned char **p,
                             const unsigned char *end,
                             rsa_context *rsa )
{
    int ret;
    size_t len;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY + ret );

    if( *p + len != end )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    if( ( ret = asn1_get_mpi( p, end, &rsa->N ) ) != 0 ||
        ( ret = asn1_get_mpi( p, end, &rsa->E ) ) != 0 )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY + ret );

    if( *p != end )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    if( ( ret = rsa_check_pubkey( rsa ) ) != 0 )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY );

    rsa->len = mpi_size( &rsa->N );

    return( 0 );
}
#endif /* POLARSSL_RSA_C */

/* Get a PK algorithm identifier
 *
 *  AlgorithmIdentifier  ::=  SEQUENCE  {
 *       algorithm               OBJECT IDENTIFIER,
 *       parameters              ANY DEFINED BY algorithm OPTIONAL  }
 */
static int pk_get_pk_alg( unsigned char **p,
                          const unsigned char *end,
                          pk_type_t *pk_alg, asn1_buf *params )
{
    int ret;
    asn1_buf alg_oid;

    memset( params, 0, sizeof(asn1_buf) );

    if( ( ret = asn1_get_alg( p, end, &alg_oid, params ) ) != 0 )
        return( POLARSSL_ERR_PK_INVALID_ALG + ret );

    if( oid_get_pk_alg( &alg_oid, pk_alg ) != 0 )
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    /*
     * No parameters with RSA (only for EC)
     */
    if( *pk_alg == POLARSSL_PK_RSA &&
            ( ( params->tag != ASN1_NULL && params->tag != 0 ) ||
                params->len != 0 ) )
    {
        return( POLARSSL_ERR_PK_INVALID_ALG );
    }

    return( 0 );
}

/*
 *  SubjectPublicKeyInfo  ::=  SEQUENCE  {
 *       algorithm            AlgorithmIdentifier,
 *       subjectPublicKey     BIT STRING }
 */
int pk_parse_subpubkey( unsigned char **p, const unsigned char *end,
                        pk_context *pk )
{
    int ret;
    size_t len;
    asn1_buf alg_params;
    pk_type_t pk_alg = POLARSSL_PK_NONE;
    const pk_info_t *pk_info;

    if( ( ret = asn1_get_tag( p, end, &len,
                    ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    end = *p + len;

    if( ( ret = pk_get_pk_alg( p, end, &pk_alg, &alg_params ) ) != 0 )
        return( ret );

    if( ( ret = asn1_get_bitstring_null( p, end, &len ) ) != 0 )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY + ret );

    if( *p + len != end )
        return( POLARSSL_ERR_PK_INVALID_PUBKEY +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    if( ( pk_info = pk_info_from_type( pk_alg ) ) == NULL )
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    if( ( ret = pk_init_ctx( pk, pk_info ) ) != 0 )
        return( ret );

#if defined(POLARSSL_RSA_C)
    if( pk_alg == POLARSSL_PK_RSA )
    {
        ret = pk_get_rsapubkey( p, end, pk_rsa( *pk ) );
    } else
#endif /* POLARSSL_RSA_C */
#if defined(POLARSSL_ECP_C)
    if( pk_alg == POLARSSL_PK_ECKEY_DH || pk_alg == POLARSSL_PK_ECKEY )
    {
        ret = pk_use_ecparams( &alg_params, &pk_ec( *pk )->grp );
        if( ret == 0 )
            ret = pk_get_ecpubkey( p, end, pk_ec( *pk ) );
    } else
#endif /* POLARSSL_ECP_C */
        ret = POLARSSL_ERR_PK_UNKNOWN_PK_ALG;

    if( ret == 0 && *p != end )
        ret = POLARSSL_ERR_PK_INVALID_PUBKEY
              POLARSSL_ERR_ASN1_LENGTH_MISMATCH;

    if( ret != 0 )
        pk_free( pk );

    return( ret );
}

#if defined(POLARSSL_RSA_C)
/*
 * Parse a PKCS#1 encoded private RSA key
 */
static int pk_parse_key_pkcs1_der( rsa_context *rsa,
                                   const unsigned char *key,
                                   size_t keylen )
{
    int ret;
    size_t len;
    unsigned char *p, *end;

    p = (unsigned char *) key;
    end = p + keylen;

    /*
     * This function parses the RSAPrivateKey (PKCS#1)
     *
     *  RSAPrivateKey ::= SEQUENCE {
     *      version           Version,
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER,  -- e
     *      privateExponent   INTEGER,  -- d
     *      prime1            INTEGER,  -- p
     *      prime2            INTEGER,  -- q
     *      exponent1         INTEGER,  -- d mod (p-1)
     *      exponent2         INTEGER,  -- d mod (q-1)
     *      coefficient       INTEGER,  -- (inverse of q) mod p
     *      otherPrimeInfos   OtherPrimeInfos OPTIONAL
     *  }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    end = p + len;

    if( ( ret = asn1_get_int( &p, end, &rsa->ver ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    if( rsa->ver != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_VERSION );
    }

    if( ( ret = asn1_get_mpi( &p, end, &rsa->N  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->E  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->D  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->P  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->Q  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->DP ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->DQ ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &rsa->QP ) ) != 0 )
    {
        rsa_free( rsa );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    rsa->len = mpi_size( &rsa->N );

    if( p != end )
    {
        rsa_free( rsa );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    if( ( ret = rsa_check_privkey( rsa ) ) != 0 )
    {
        rsa_free( rsa );
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
/*
 * Parse a SEC1 encoded private EC key
 */
static int pk_parse_key_sec1_der( ecp_keypair *eck,
                                  const unsigned char *key,
                                  size_t keylen )
{
    int ret;
    int version, pubkey_done;
    size_t len;
    asn1_buf params;
    unsigned char *p = (unsigned char *) key;
    unsigned char *end = p + keylen;
    unsigned char *end2;

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
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    end = p + len;

    if( ( ret = asn1_get_int( &p, end, &version ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( version != 1 )
        return( POLARSSL_ERR_PK_KEY_INVALID_VERSION );

    if( ( ret = asn1_get_tag( &p, end, &len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( ( ret = mpi_read_binary( &eck->d, p, len ) ) != 0 )
    {
        ecp_keypair_free( eck );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    p += len;

    /*
     * Is 'parameters' present?
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0 ) ) == 0 )
    {
        if( ( ret = pk_get_ecparams( &p, p + len, &params) ) != 0 ||
            ( ret = pk_use_ecparams( &params, &eck->grp )  ) != 0 )
        {
            ecp_keypair_free( eck );
            return( ret );
        }
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
    {
        ecp_keypair_free( eck );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    /*
     * Is 'publickey' present? If not, or if we can't read it (eg because it
     * is compressed), create it from the private key.
     */
    pubkey_done = 0;
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 1 ) ) == 0 )
    {
        end2 = p + len;

        if( ( ret = asn1_get_bitstring_null( &p, end2, &len ) ) != 0 )
            return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

        if( p + len != end2 )
            return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

        if( ( ret = pk_get_ecpubkey( &p, end2, eck ) ) == 0 )
            pubkey_done = 1;
        else
        {
            /*
             * The only acceptable failure mode of pk_get_ecpubkey() above
             * is if the point format is not recognized.
             */
            if( ret != POLARSSL_ERR_ECP_FEATURE_UNAVAILABLE )
                return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );
        }
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
    {
        ecp_keypair_free( eck );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    if( ! pubkey_done &&
        ( ret = ecp_mul( &eck->grp, &eck->Q, &eck->d, &eck->grp.G,
                                                      NULL, NULL ) ) != 0 )
    {
        ecp_keypair_free( eck );
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    if( ( ret = ecp_check_privkey( &eck->grp, &eck->d ) ) != 0 )
    {
        ecp_keypair_free( eck );
        return( ret );
    }

    return( 0 );
}
#endif /* POLARSSL_ECP_C */

/*
 * Parse an unencrypted PKCS#8 encoded private key
 */
static int pk_parse_key_pkcs8_unencrypted_der(
                                    pk_context *pk,
                                    const unsigned char* key,
                                    size_t keylen )
{
    int ret, version;
    size_t len;
    asn1_buf params;
    unsigned char *p = (unsigned char *) key;
    unsigned char *end = p + keylen;
    pk_type_t pk_alg = POLARSSL_PK_NONE;
    const pk_info_t *pk_info;

    /*
     * This function parses the PrivatKeyInfo object (PKCS#8 v1.2 = RFC 5208)
     *
     *    PrivateKeyInfo ::= SEQUENCE {
     *      version                   Version,
     *      privateKeyAlgorithm       PrivateKeyAlgorithmIdentifier,
     *      privateKey                PrivateKey,
     *      attributes           [0]  IMPLICIT Attributes OPTIONAL }
     *
     *    Version ::= INTEGER
     *    PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
     *    PrivateKey ::= OCTET STRING
     *
     *  The PrivateKey OCTET STRING is a SEC1 ECPrivateKey
     */

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    end = p + len;

    if( ( ret = asn1_get_int( &p, end, &version ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( version != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_VERSION + ret );

    if( ( ret = pk_get_pk_alg( &p, end, &pk_alg, &params ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( ( ret = asn1_get_tag( &p, end, &len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( len < 1 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( ( pk_info = pk_info_from_type( pk_alg ) ) == NULL )
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    if( ( ret = pk_init_ctx( pk, pk_info ) ) != 0 )
        return( ret );

#if defined(POLARSSL_RSA_C)
    if( pk_alg == POLARSSL_PK_RSA )
    {
        if( ( ret = pk_parse_key_pkcs1_der( pk_rsa( *pk ), p, len ) ) != 0 )
        {
            pk_free( pk );
            return( ret );
        }
    } else
#endif /* POLARSSL_RSA_C */
#if defined(POLARSSL_ECP_C)
    if( pk_alg == POLARSSL_PK_ECKEY || pk_alg == POLARSSL_PK_ECKEY_DH )
    {
        if( ( ret = pk_use_ecparams( &params, &pk_ec( *pk )->grp ) ) != 0 ||
            ( ret = pk_parse_key_sec1_der( pk_ec( *pk ), p, len )  ) != 0 )
        {
            pk_free( pk );
            return( ret );
        }
    } else
#endif /* POLARSSL_ECP_C */
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    return( 0 );
}

/*
 * Parse an encrypted PKCS#8 encoded private key
 */
static int pk_parse_key_pkcs8_encrypted_der(
                                    pk_context *pk,
                                    const unsigned char *key, size_t keylen,
                                    const unsigned char *pwd, size_t pwdlen )
{
    int ret, decrypted = 0;
    size_t len;
    unsigned char buf[2048];
    unsigned char *p, *end;
    asn1_buf pbe_alg_oid, pbe_params;
#if defined(POLARSSL_PKCS12_C)
    cipher_type_t cipher_alg;
    md_type_t md_alg;
#endif

    memset( buf, 0, sizeof( buf ) );

    p = (unsigned char *) key;
    end = p + keylen;

    if( pwdlen == 0 )
        return( POLARSSL_ERR_PK_PASSWORD_REQUIRED );

    /*
     * This function parses the EncryptedPrivatKeyInfo object (PKCS#8)
     *
     *  EncryptedPrivateKeyInfo ::= SEQUENCE {
     *    encryptionAlgorithm  EncryptionAlgorithmIdentifier,
     *    encryptedData        EncryptedData
     *  }
     *
     *  EncryptionAlgorithmIdentifier ::= AlgorithmIdentifier
     *
     *  EncryptedData ::= OCTET STRING
     *
     *  The EncryptedData OCTET STRING is a PKCS#8 PrivateKeyInfo
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );
    }

    end = p + len;

    if( ( ret = asn1_get_alg( &p, end, &pbe_alg_oid, &pbe_params ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( ( ret = asn1_get_tag( &p, end, &len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT + ret );

    if( len > sizeof( buf ) )
        return( POLARSSL_ERR_PK_BAD_INPUT_DATA );

    /*
     * Decrypt EncryptedData with appropriate PDE
     */
#if defined(POLARSSL_PKCS12_C)
    if( oid_get_pkcs12_pbe_alg( &pbe_alg_oid, &md_alg, &cipher_alg ) == 0 )
    {
        if( ( ret = pkcs12_pbe( &pbe_params, PKCS12_PBE_DECRYPT,
                                cipher_alg, md_alg,
                                pwd, pwdlen, p, len, buf ) ) != 0 )
        {
            if( ret == POLARSSL_ERR_PKCS12_PASSWORD_MISMATCH )
                return( POLARSSL_ERR_PK_PASSWORD_MISMATCH );

            return( ret );
        }

        decrypted = 1;
    }
    else if( OID_CMP( OID_PKCS12_PBE_SHA1_RC4_128, &pbe_alg_oid ) )
    {
        if( ( ret = pkcs12_pbe_sha1_rc4_128( &pbe_params,
                                             PKCS12_PBE_DECRYPT,
                                             pwd, pwdlen,
                                             p, len, buf ) ) != 0 )
        {
            return( ret );
        }

        // Best guess for password mismatch when using RC4. If first tag is
        // not ASN1_CONSTRUCTED | ASN1_SEQUENCE
        //
        if( *buf != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE ) )
            return( POLARSSL_ERR_PK_PASSWORD_MISMATCH );

        decrypted = 1;
    }
    else
#endif /* POLARSSL_PKCS12_C */
#if defined(POLARSSL_PKCS5_C)
    if( OID_CMP( OID_PKCS5_PBES2, &pbe_alg_oid ) )
    {
        if( ( ret = pkcs5_pbes2( &pbe_params, PKCS5_DECRYPT, pwd, pwdlen,
                                  p, len, buf ) ) != 0 )
        {
            if( ret == POLARSSL_ERR_PKCS5_PASSWORD_MISMATCH )
                return( POLARSSL_ERR_PK_PASSWORD_MISMATCH );

            return( ret );
        }

        decrypted = 1;
    }
    else
#endif /* POLARSSL_PKCS5_C */
    {
        ((void) pwd);
    }

    if( decrypted == 0 )
        return( POLARSSL_ERR_PK_FEATURE_UNAVAILABLE );

    return( pk_parse_key_pkcs8_unencrypted_der( pk, buf, len ) );
}

/*
 * Parse a private key
 */
int pk_parse_key( pk_context *pk,
                  const unsigned char *key, size_t keylen,
                  const unsigned char *pwd, size_t pwdlen )
{
    int ret;
    const pk_info_t *pk_info;

#if defined(POLARSSL_PEM_PARSE_C)
    size_t len;
    pem_context pem;

    pem_init( &pem );

#if defined(POLARSSL_RSA_C)
    ret = pem_read_buffer( &pem,
                           "-----BEGIN RSA PRIVATE KEY-----",
                           "-----END RSA PRIVATE KEY-----",
                           key, pwd, pwdlen, &len );
    if( ret == 0 )
    {
        if( ( pk_info = pk_info_from_type( POLARSSL_PK_RSA ) ) == NULL )
            return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

        if( ( ret = pk_init_ctx( pk, pk_info                    ) ) != 0 ||
            ( ret = pk_parse_key_pkcs1_der( pk_rsa( *pk ),
                                            pem.buf, pem.buflen ) ) != 0 )
        {
            pk_free( pk );
        }

        pem_free( &pem );
        return( ret );
    }
    else if( ret == POLARSSL_ERR_PEM_PASSWORD_MISMATCH )
        return( POLARSSL_ERR_PK_PASSWORD_MISMATCH );
    else if( ret == POLARSSL_ERR_PEM_PASSWORD_REQUIRED )
        return( POLARSSL_ERR_PK_PASSWORD_REQUIRED );
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        return( ret );
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
    ret = pem_read_buffer( &pem,
                           "-----BEGIN EC PRIVATE KEY-----",
                           "-----END EC PRIVATE KEY-----",
                           key, pwd, pwdlen, &len );
    if( ret == 0 )
    {
        if( ( pk_info = pk_info_from_type( POLARSSL_PK_ECKEY ) ) == NULL )
            return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

        if( ( ret = pk_init_ctx( pk, pk_info                   ) ) != 0 ||
            ( ret = pk_parse_key_sec1_der( pk_ec( *pk ),
                                           pem.buf, pem.buflen ) ) != 0 )
        {
            pk_free( pk );
        }

        pem_free( &pem );
        return( ret );
    }
    else if( ret == POLARSSL_ERR_PEM_PASSWORD_MISMATCH )
        return( POLARSSL_ERR_PK_PASSWORD_MISMATCH );
    else if( ret == POLARSSL_ERR_PEM_PASSWORD_REQUIRED )
        return( POLARSSL_ERR_PK_PASSWORD_REQUIRED );
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        return( ret );
#endif /* POLARSSL_ECP_C */

    ret = pem_read_buffer( &pem,
                           "-----BEGIN PRIVATE KEY-----",
                           "-----END PRIVATE KEY-----",
                           key, NULL, 0, &len );
    if( ret == 0 )
    {
        if( ( ret = pk_parse_key_pkcs8_unencrypted_der( pk,
                                                pem.buf, pem.buflen ) ) != 0 )
        {
            pk_free( pk );
        }

        pem_free( &pem );
        return( ret );
    }
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        return( ret );

    ret = pem_read_buffer( &pem,
                           "-----BEGIN ENCRYPTED PRIVATE KEY-----",
                           "-----END ENCRYPTED PRIVATE KEY-----",
                           key, NULL, 0, &len );
    if( ret == 0 )
    {
        if( ( ret = pk_parse_key_pkcs8_encrypted_der( pk,
                                                      pem.buf, pem.buflen,
                                                      pwd, pwdlen ) ) != 0 )
        {
            pk_free( pk );
        }

        pem_free( &pem );
        return( ret );
    }
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        return( ret );
#else
    ((void) pwd);
    ((void) pwdlen);
#endif /* POLARSSL_PEM_PARSE_C */

    /*
    * At this point we only know it's not a PEM formatted key. Could be any
    * of the known DER encoded private key formats
    *
    * We try the different DER format parsers to see if one passes without
    * error
    */
    if( ( ret = pk_parse_key_pkcs8_encrypted_der( pk, key, keylen,
                                                  pwd, pwdlen ) ) == 0 )
    {
        return( 0 );
    }

    pk_free( pk );

    if( ret == POLARSSL_ERR_PK_PASSWORD_MISMATCH )
    {
        return( ret );
    }

    if( ( ret = pk_parse_key_pkcs8_unencrypted_der( pk, key, keylen ) ) == 0 )
        return( 0 );

    pk_free( pk );

#if defined(POLARSSL_RSA_C)
    if( ( pk_info = pk_info_from_type( POLARSSL_PK_RSA ) ) == NULL )
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    if( ( ret = pk_init_ctx( pk, pk_info                           ) ) != 0 ||
        ( ret = pk_parse_key_pkcs1_der( pk_rsa( *pk ), key, keylen ) ) == 0 )
    {
        return( 0 );
    }

    pk_free( pk );
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
    if( ( pk_info = pk_info_from_type( POLARSSL_PK_ECKEY ) ) == NULL )
        return( POLARSSL_ERR_PK_UNKNOWN_PK_ALG );

    if( ( ret = pk_init_ctx( pk, pk_info                         ) ) != 0 ||
        ( ret = pk_parse_key_sec1_der( pk_ec( *pk ), key, keylen ) ) == 0 )
    {
        return( 0 );
    }

    pk_free( pk );
#endif /* POLARSSL_ECP_C */

    return( POLARSSL_ERR_PK_KEY_INVALID_FORMAT );
}

/*
 * Parse a public key
 */
int pk_parse_public_key( pk_context *ctx,
                         const unsigned char *key, size_t keylen )
{
    int ret;
    unsigned char *p;
#if defined(POLARSSL_PEM_PARSE_C)
    size_t len;
    pem_context pem;

    pem_init( &pem );
    ret = pem_read_buffer( &pem,
            "-----BEGIN PUBLIC KEY-----",
            "-----END PUBLIC KEY-----",
            key, NULL, 0, &len );

    if( ret == 0 )
    {
        /*
         * Was PEM encoded
         */
        key = pem.buf;
        keylen = pem.buflen;
    }
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
    {
        pem_free( &pem );
        return( ret );
    }
#endif /* POLARSSL_PEM_PARSE_C */
    p = (unsigned char *) key;

    ret = pk_parse_subpubkey( &p, p + keylen, ctx );

#if defined(POLARSSL_PEM_PARSE_C)
    pem_free( &pem );
#endif

    return( ret );
}

#endif /* POLARSSL_PK_PARSE_C */
