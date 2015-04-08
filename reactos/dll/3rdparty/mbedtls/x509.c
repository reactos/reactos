/*
 *  X.509 common functions for parsing and verification
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
 *  The ITU-T X.509 standard defines a certificate format for PKI.
 *
 *  http://www.ietf.org/rfc/rfc5280.txt (Certificates and CRLs)
 *  http://www.ietf.org/rfc/rfc3279.txt (Alg IDs for CRLs)
 *  http://www.ietf.org/rfc/rfc2986.txt (CSRs, aka PKCS#10)
 *
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.680-0207.pdf
 *  http://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_X509_USE_C)

#include "polarssl/x509.h"
#include "polarssl/asn1.h"
#include "polarssl/oid.h"
#if defined(POLARSSL_PEM_PARSE_C)
#include "polarssl/pem.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_printf     printf
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#include <string.h>
#include <stdlib.h>
#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)
#include <windows.h>
#else
#include <time.h>
#endif

#include <stdio.h>

#if defined(POLARSSL_FS_IO)
#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif
#endif

/*
 *  CertificateSerialNumber  ::=  INTEGER
 */
int x509_get_serial( unsigned char **p, const unsigned char *end,
                     x509_buf *serial )
{
    int ret;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_INVALID_SERIAL +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != ( ASN1_CONTEXT_SPECIFIC | ASN1_PRIMITIVE | 2 ) &&
        **p !=   ASN1_INTEGER )
        return( POLARSSL_ERR_X509_INVALID_SERIAL +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    serial->tag = *(*p)++;

    if( ( ret = asn1_get_len( p, end, &serial->len ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_SERIAL + ret );

    serial->p = *p;
    *p += serial->len;

    return( 0 );
}

/* Get an algorithm identifier without parameters (eg for signatures)
 *
 *  AlgorithmIdentifier  ::=  SEQUENCE  {
 *       algorithm               OBJECT IDENTIFIER,
 *       parameters              ANY DEFINED BY algorithm OPTIONAL  }
 */
int x509_get_alg_null( unsigned char **p, const unsigned char *end,
                       x509_buf *alg )
{
    int ret;

    if( ( ret = asn1_get_alg_null( p, end, alg ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    return( 0 );
}

/*
 * Parse an algorithm identifier with (optional) paramaters
 */
int x509_get_alg( unsigned char **p, const unsigned char *end,
                  x509_buf *alg, x509_buf *params )
{
    int ret;

    if( ( ret = asn1_get_alg( p, end, alg, params ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    return( 0 );
}

#if defined(POLARSSL_X509_RSASSA_PSS_SUPPORT)
/*
 * HashAlgorithm ::= AlgorithmIdentifier
 *
 * AlgorithmIdentifier  ::=  SEQUENCE  {
 *      algorithm               OBJECT IDENTIFIER,
 *      parameters              ANY DEFINED BY algorithm OPTIONAL  }
 *
 * For HashAlgorithm, parameters MUST be NULL or absent.
 */
static int x509_get_hash_alg( const x509_buf *alg, md_type_t *md_alg )
{
    int ret;
    unsigned char *p;
    const unsigned char *end;
    x509_buf md_oid;
    size_t len;

    /* Make sure we got a SEQUENCE and setup bounds */
    if( alg->tag != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE ) )
        return( POLARSSL_ERR_X509_INVALID_ALG +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    p = (unsigned char *) alg->p;
    end = p + alg->len;

    if( p >= end )
        return( POLARSSL_ERR_X509_INVALID_ALG +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    /* Parse md_oid */
    md_oid.tag = *p;

    if( ( ret = asn1_get_tag( &p, end, &md_oid.len, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    md_oid.p = p;
    p += md_oid.len;

    /* Get md_alg from md_oid */
    if( ( ret = oid_get_md_alg( &md_oid, md_alg ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    /* Make sure params is absent of NULL */
    if( p == end )
        return( 0 );

    if( ( ret = asn1_get_tag( &p, end, &len, ASN1_NULL ) ) != 0 || len != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    if( p != end )
        return( POLARSSL_ERR_X509_INVALID_ALG +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 *    RSASSA-PSS-params  ::=  SEQUENCE  {
 *       hashAlgorithm     [0] HashAlgorithm DEFAULT sha1Identifier,
 *       maskGenAlgorithm  [1] MaskGenAlgorithm DEFAULT mgf1SHA1Identifier,
 *       saltLength        [2] INTEGER DEFAULT 20,
 *       trailerField      [3] INTEGER DEFAULT 1  }
 *    -- Note that the tags in this Sequence are explicit.
 *
 * RFC 4055 (which defines use of RSASSA-PSS in PKIX) states that the value
 * of trailerField MUST be 1, and PKCS#1 v2.2 doesn't even define any other
 * option. Enfore this at parsing time.
 */
int x509_get_rsassa_pss_params( const x509_buf *params,
                                md_type_t *md_alg, md_type_t *mgf_md,
                                int *salt_len )
{
    int ret;
    unsigned char *p;
    const unsigned char *end, *end2;
    size_t len;
    x509_buf alg_id, alg_params;

    /* First set everything to defaults */
    *md_alg = POLARSSL_MD_SHA1;
    *mgf_md = POLARSSL_MD_SHA1;
    *salt_len = 20;

    /* Make sure params is a SEQUENCE and setup bounds */
    if( params->tag != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE ) )
        return( POLARSSL_ERR_X509_INVALID_ALG +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    p = (unsigned char *) params->p;
    end = p + params->len;

    if( p == end )
        return( 0 );

    /*
     * HashAlgorithm
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0 ) ) == 0 )
    {
        end2 = p + len;

        /* HashAlgorithm ::= AlgorithmIdentifier (without parameters) */
        if( ( ret = x509_get_alg_null( &p, end2, &alg_id ) ) != 0 )
            return( ret );

        if( ( ret = oid_get_md_alg( &alg_id, md_alg ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_ALG + ret );

        if( p != end2 )
            return( POLARSSL_ERR_X509_INVALID_ALG +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    if( p == end )
        return( 0 );

    /*
     * MaskGenAlgorithm
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 1 ) ) == 0 )
    {
        end2 = p + len;

        /* MaskGenAlgorithm ::= AlgorithmIdentifier (params = HashAlgorithm) */
        if( ( ret = x509_get_alg( &p, end2, &alg_id, &alg_params ) ) != 0 )
            return( ret );

        /* Only MFG1 is recognised for now */
        if( ! OID_CMP( OID_MGF1, &alg_id ) )
            return( POLARSSL_ERR_X509_FEATURE_UNAVAILABLE +
                    POLARSSL_ERR_OID_NOT_FOUND );

        /* Parse HashAlgorithm */
        if( ( ret = x509_get_hash_alg( &alg_params, mgf_md ) ) != 0 )
            return( ret );

        if( p != end2 )
            return( POLARSSL_ERR_X509_INVALID_ALG +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    if( p == end )
        return( 0 );

    /*
     * salt_len
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 2 ) ) == 0 )
    {
        end2 = p + len;

        if( ( ret = asn1_get_int( &p, end2, salt_len ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_ALG + ret );

        if( p != end2 )
            return( POLARSSL_ERR_X509_INVALID_ALG +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    if( p == end )
        return( 0 );

    /*
     * trailer_field (if present, must be 1)
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
                    ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 3 ) ) == 0 )
    {
        int trailer_field;

        end2 = p + len;

        if( ( ret = asn1_get_int( &p, end2, &trailer_field ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_ALG + ret );

        if( p != end2 )
            return( POLARSSL_ERR_X509_INVALID_ALG +
                    POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

        if( trailer_field != 1 )
            return( POLARSSL_ERR_X509_INVALID_ALG );
    }
    else if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        return( POLARSSL_ERR_X509_INVALID_ALG + ret );

    if( p != end )
        return( POLARSSL_ERR_X509_INVALID_ALG +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}
#endif /* POLARSSL_X509_RSASSA_PSS_SUPPORT */

/*
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 */
static int x509_get_attr_type_value( unsigned char **p,
                                     const unsigned char *end,
                                     x509_name *cur )
{
    int ret;
    size_t len;
    x509_buf *oid;
    x509_buf *val;

    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_NAME + ret );

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_INVALID_NAME +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    oid = &cur->oid;
    oid->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &oid->len, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_NAME + ret );

    oid->p = *p;
    *p += oid->len;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_INVALID_NAME +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    if( **p != ASN1_BMP_STRING && **p != ASN1_UTF8_STRING      &&
        **p != ASN1_T61_STRING && **p != ASN1_PRINTABLE_STRING &&
        **p != ASN1_IA5_STRING && **p != ASN1_UNIVERSAL_STRING )
        return( POLARSSL_ERR_X509_INVALID_NAME +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    val = &cur->val;
    val->tag = *(*p)++;

    if( ( ret = asn1_get_len( p, end, &val->len ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_NAME + ret );

    val->p = *p;
    *p += val->len;

    cur->next = NULL;

    return( 0 );
}

/*
 *  Name ::= CHOICE { -- only one possibility for now --
 *       rdnSequence  RDNSequence }
 *
 *  RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
 *
 *  RelativeDistinguishedName ::=
 *    SET OF AttributeTypeAndValue
 *
 *  AttributeTypeAndValue ::= SEQUENCE {
 *    type     AttributeType,
 *    value    AttributeValue }
 *
 *  AttributeType ::= OBJECT IDENTIFIER
 *
 *  AttributeValue ::= ANY DEFINED BY AttributeType
 *
 * The data structure is optimized for the common case where each RDN has only
 * one element, which is represented as a list of AttributeTypeAndValue.
 * For the general case we still use a flat list, but we mark elements of the
 * same set so that they are "merged" together in the functions that consume
 * this list, eg x509_dn_gets().
 */
int x509_get_name( unsigned char **p, const unsigned char *end,
                   x509_name *cur )
{
    int ret;
    size_t set_len;
    const unsigned char *end_set;

    /* don't use recursion, we'd risk stack overflow if not optimized */
    while( 1 )
    {
        /*
         * parse SET
         */
        if( ( ret = asn1_get_tag( p, end, &set_len,
                ASN1_CONSTRUCTED | ASN1_SET ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_NAME + ret );

        end_set  = *p + set_len;

        while( 1 )
        {
            if( ( ret = x509_get_attr_type_value( p, end_set, cur ) ) != 0 )
                return( ret );

            if( *p == end_set )
                break;

            /* Mark this item as being only one in a set */
            cur->next_merged = 1;

            cur->next = (x509_name *) polarssl_malloc( sizeof( x509_name ) );

            if( cur->next == NULL )
                return( POLARSSL_ERR_X509_MALLOC_FAILED );

            memset( cur->next, 0, sizeof( x509_name ) );

            cur = cur->next;
        }

        /*
         * continue until end of SEQUENCE is reached
         */
        if( *p == end )
            return( 0 );

        cur->next = (x509_name *) polarssl_malloc( sizeof( x509_name ) );

        if( cur->next == NULL )
            return( POLARSSL_ERR_X509_MALLOC_FAILED );

        memset( cur->next, 0, sizeof( x509_name ) );

        cur = cur->next;
    }
}

/*
 *  Time ::= CHOICE {
 *       utcTime        UTCTime,
 *       generalTime    GeneralizedTime }
 */
int x509_get_time( unsigned char **p, const unsigned char *end,
                   x509_time *time )
{
    int ret;
    size_t len;
    char date[64];
    unsigned char tag;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_INVALID_DATE +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    tag = **p;

    if( tag == ASN1_UTC_TIME )
    {
        (*p)++;
        ret = asn1_get_len( p, end, &len );

        if( ret != 0 )
            return( POLARSSL_ERR_X509_INVALID_DATE + ret );

        memset( date,  0, sizeof( date ) );
        memcpy( date, *p, ( len < sizeof( date ) - 1 ) ?
                len : sizeof( date ) - 1 );

        if( sscanf( date, "%2d%2d%2d%2d%2d%2dZ",
                    &time->year, &time->mon, &time->day,
                    &time->hour, &time->min, &time->sec ) < 5 )
            return( POLARSSL_ERR_X509_INVALID_DATE );

        time->year +=  100 * ( time->year < 50 );
        time->year += 1900;

        *p += len;

        return( 0 );
    }
    else if( tag == ASN1_GENERALIZED_TIME )
    {
        (*p)++;
        ret = asn1_get_len( p, end, &len );

        if( ret != 0 )
            return( POLARSSL_ERR_X509_INVALID_DATE + ret );

        memset( date,  0, sizeof( date ) );
        memcpy( date, *p, ( len < sizeof( date ) - 1 ) ?
                len : sizeof( date ) - 1 );

        if( sscanf( date, "%4d%2d%2d%2d%2d%2dZ",
                    &time->year, &time->mon, &time->day,
                    &time->hour, &time->min, &time->sec ) < 5 )
            return( POLARSSL_ERR_X509_INVALID_DATE );

        *p += len;

        return( 0 );
    }
    else
        return( POLARSSL_ERR_X509_INVALID_DATE +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );
}

int x509_get_sig( unsigned char **p, const unsigned char *end, x509_buf *sig )
{
    int ret;
    size_t len;

    if( ( end - *p ) < 1 )
        return( POLARSSL_ERR_X509_INVALID_SIGNATURE +
                POLARSSL_ERR_ASN1_OUT_OF_DATA );

    sig->tag = **p;

    if( ( ret = asn1_get_bitstring_null( p, end, &len ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_SIGNATURE + ret );

    sig->len = len;
    sig->p = *p;

    *p += len;

    return( 0 );
}

/*
 * Get signature algorithm from alg OID and optional parameters
 */
int x509_get_sig_alg( const x509_buf *sig_oid, const x509_buf *sig_params,
                      md_type_t *md_alg, pk_type_t *pk_alg,
                      void **sig_opts )
{
    int ret;

    if( *sig_opts != NULL )
        return( POLARSSL_ERR_X509_BAD_INPUT_DATA );

    if( ( ret = oid_get_sig_alg( sig_oid, md_alg, pk_alg ) ) != 0 )
        return( POLARSSL_ERR_X509_UNKNOWN_SIG_ALG + ret );

#if defined(POLARSSL_X509_RSASSA_PSS_SUPPORT)
    if( *pk_alg == POLARSSL_PK_RSASSA_PSS )
    {
        pk_rsassa_pss_options *pss_opts;

        pss_opts = polarssl_malloc( sizeof( pk_rsassa_pss_options ) );
        if( pss_opts == NULL )
            return( POLARSSL_ERR_X509_MALLOC_FAILED );

        ret = x509_get_rsassa_pss_params( sig_params,
                                          md_alg,
                                          &pss_opts->mgf1_hash_id,
                                          &pss_opts->expected_salt_len );
        if( ret != 0 )
        {
            polarssl_free( pss_opts );
            return( ret );
        }

        *sig_opts = (void *) pss_opts;
    }
    else
#endif /* POLARSSL_X509_RSASSA_PSS_SUPPORT */
    {
        /* Make sure parameters are absent or NULL */
        if( ( sig_params->tag != ASN1_NULL && sig_params->tag != 0 ) ||
              sig_params->len != 0 )
        return( POLARSSL_ERR_X509_INVALID_ALG );
    }

    return( 0 );
}

/*
 * X.509 Extensions (No parsing of extensions, pointer should
 * be either manually updated or extensions should be parsed!
 */
int x509_get_ext( unsigned char **p, const unsigned char *end,
                  x509_buf *ext, int tag )
{
    int ret;
    size_t len;

    if( *p == end )
        return( 0 );

    ext->tag = **p;

    if( ( ret = asn1_get_tag( p, end, &ext->len,
            ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | tag ) ) != 0 )
        return( ret );

    ext->p = *p;
    end = *p + ext->len;

    /*
     * Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension
     *
     * Extension  ::=  SEQUENCE  {
     *      extnID      OBJECT IDENTIFIER,
     *      critical    BOOLEAN DEFAULT FALSE,
     *      extnValue   OCTET STRING  }
     */
    if( ( ret = asn1_get_tag( p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS + ret );

    if( end != *p + len )
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

#if defined(_MSC_VER) && !defined snprintf && !defined(EFIX64) && \
    !defined(EFI32)
#include <stdarg.h>

#if !defined vsnprintf
#define vsnprintf _vsnprintf
#endif // vsnprintf

/*
 * Windows _snprintf and _vsnprintf are not compatible to linux versions.
 * Result value is not size of buffer needed, but -1 if no fit is possible.
 *
 * This fuction tries to 'fix' this by at least suggesting enlarging the
 * size by 20.
 */
static int compat_snprintf( char *str, size_t size, const char *format, ... )
{
    va_list ap;
    int res = -1;

    va_start( ap, format );

    res = vsnprintf( str, size, format, ap );

    va_end( ap );

    // No quick fix possible
    if( res < 0 )
        return( (int) size + 20 );

    return( res );
}

#define snprintf compat_snprintf
#endif /* _MSC_VER && !snprintf && !EFIX64 && !EFI32 */

#define POLARSSL_ERR_DEBUG_BUF_TOO_SMALL    -2

#define SAFE_SNPRINTF()                             \
{                                                   \
    if( ret == -1 )                                 \
        return( -1 );                               \
                                                    \
    if( (unsigned int) ret > n ) {                  \
        p[n - 1] = '\0';                            \
        return( POLARSSL_ERR_DEBUG_BUF_TOO_SMALL ); \
    }                                               \
                                                    \
    n -= (unsigned int) ret;                        \
    p += (unsigned int) ret;                        \
}

/*
 * Store the name in printable form into buf; no more
 * than size characters will be written
 */
int x509_dn_gets( char *buf, size_t size, const x509_name *dn )
{
    int ret;
    size_t i, n;
    unsigned char c, merge = 0;
    const x509_name *name;
    const char *short_name = NULL;
    char s[X509_MAX_DN_NAME_SIZE], *p;

    memset( s, 0, sizeof( s ) );

    name = dn;
    p = buf;
    n = size;

    while( name != NULL )
    {
        if( !name->oid.p )
        {
            name = name->next;
            continue;
        }

        if( name != dn )
        {
            ret = snprintf( p, n, merge ? " + " : ", " );
            SAFE_SNPRINTF();
        }

        ret = oid_get_attr_short_name( &name->oid, &short_name );

        if( ret == 0 )
            ret = snprintf( p, n, "%s=", short_name );
        else
            ret = snprintf( p, n, "\?\?=" );
        SAFE_SNPRINTF();

        for( i = 0; i < name->val.len; i++ )
        {
            if( i >= sizeof( s ) - 1 )
                break;

            c = name->val.p[i];
            if( c < 32 || c == 127 || ( c > 128 && c < 160 ) )
                 s[i] = '?';
            else s[i] = c;
        }
        s[i] = '\0';
        ret = snprintf( p, n, "%s", s );
        SAFE_SNPRINTF();

        merge = name->next_merged;
        name = name->next;
    }

    return( (int) ( size - n ) );
}

/*
 * Store the serial in printable form into buf; no more
 * than size characters will be written
 */
int x509_serial_gets( char *buf, size_t size, const x509_buf *serial )
{
    int ret;
    size_t i, n, nr;
    char *p;

    p = buf;
    n = size;

    nr = ( serial->len <= 32 )
        ? serial->len  : 28;

    for( i = 0; i < nr; i++ )
    {
        if( i == 0 && nr > 1 && serial->p[i] == 0x0 )
            continue;

        ret = snprintf( p, n, "%02X%s",
                serial->p[i], ( i < nr - 1 ) ? ":" : "" );
        SAFE_SNPRINTF();
    }

    if( nr != serial->len )
    {
        ret = snprintf( p, n, "...." );
        SAFE_SNPRINTF();
    }

    return( (int) ( size - n ) );
}

/*
 * Helper for writing signature algorithms
 */
int x509_sig_alg_gets( char *buf, size_t size, const x509_buf *sig_oid,
                       pk_type_t pk_alg, md_type_t md_alg,
                       const void *sig_opts )
{
    int ret;
    char *p = buf;
    size_t n = size;
    const char *desc = NULL;

    ret = oid_get_sig_alg_desc( sig_oid, &desc );
    if( ret != 0 )
        ret = snprintf( p, n, "???"  );
    else
        ret = snprintf( p, n, "%s", desc );
    SAFE_SNPRINTF();

#if defined(POLARSSL_X509_RSASSA_PSS_SUPPORT)
    if( pk_alg == POLARSSL_PK_RSASSA_PSS )
    {
        const pk_rsassa_pss_options *pss_opts;
        const md_info_t *md_info, *mgf_md_info;

        pss_opts = (const pk_rsassa_pss_options *) sig_opts;

        md_info = md_info_from_type( md_alg );
        mgf_md_info = md_info_from_type( pss_opts->mgf1_hash_id );

        ret = snprintf( p, n, " (%s, MGF1-%s, 0x%02X)",
                              md_info ? md_info->name : "???",
                              mgf_md_info ? mgf_md_info->name : "???",
                              pss_opts->expected_salt_len );
        SAFE_SNPRINTF();
    }
#else
    ((void) pk_alg);
    ((void) md_alg);
    ((void) sig_opts);
#endif /* POLARSSL_X509_RSASSA_PSS_SUPPORT */

    return( (int)( size - n ) );
}

/*
 * Helper for writing "RSA key size", "EC key size", etc
 */
int x509_key_size_helper( char *buf, size_t size, const char *name )
{
    char *p = buf;
    size_t n = size;
    int ret;

    if( strlen( name ) + sizeof( " key size" ) > size )
        return( POLARSSL_ERR_DEBUG_BUF_TOO_SMALL );

    ret = snprintf( p, n, "%s key size", name );
    SAFE_SNPRINTF();

    return( 0 );
}

/*
 * Return an informational string describing the given OID
 */
const char *x509_oid_get_description( x509_buf *oid )
{
    const char *desc = NULL;
    int ret;

    ret = oid_get_extended_key_usage( oid, &desc );

    if( ret != 0 )
        return( NULL );

    return( desc );
}

/* Return the x.y.z.... style numeric string for the given OID */
int x509_oid_get_numeric_string( char *buf, size_t size, x509_buf *oid )
{
    return oid_get_numeric_string( buf, size, oid );
}

/*
 * Return 0 if the x509_time is still valid, or 1 otherwise.
 */
#if defined(POLARSSL_HAVE_TIME)

static void x509_get_current_time( x509_time *now )
{
#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)
    SYSTEMTIME st;

    GetSystemTime( &st );

    now->year = st.wYear;
    now->mon = st.wMonth;
    now->day = st.wDay;
    now->hour = st.wHour;
    now->min = st.wMinute;
    now->sec = st.wSecond;
#else
    struct tm lt;
    time_t tt;

    tt = time( NULL );
    gmtime_r( &tt, &lt );

    now->year = lt.tm_year + 1900;
    now->mon = lt.tm_mon + 1;
    now->day = lt.tm_mday;
    now->hour = lt.tm_hour;
    now->min = lt.tm_min;
    now->sec = lt.tm_sec;
#endif /* _WIN32 && !EFIX64 && !EFI32 */
}

/*
 * Return 0 if before <= after, 1 otherwise
 */
static int x509_check_time( const x509_time *before, const x509_time *after )
{
    if( before->year  > after->year )
        return( 1 );

    if( before->year == after->year &&
        before->mon   > after->mon )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day   > after->day )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour  > after->hour )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour == after->hour &&
        before->min   > after->min  )
        return( 1 );

    if( before->year == after->year &&
        before->mon  == after->mon  &&
        before->day  == after->day  &&
        before->hour == after->hour &&
        before->min  == after->min  &&
        before->sec   > after->sec  )
        return( 1 );

    return( 0 );
}

int x509_time_expired( const x509_time *to )
{
    x509_time now;

    x509_get_current_time( &now );

    return( x509_check_time( &now, to ) );
}

int x509_time_future( const x509_time *from )
{
    x509_time now;

    x509_get_current_time( &now );

    return( x509_check_time( from, &now ) );
}

#else  /* POLARSSL_HAVE_TIME */

int x509_time_expired( const x509_time *to )
{
    ((void) to);
    return( 0 );
}

int x509_time_future( const x509_time *from )
{
    ((void) from);
    return( 0 );
}
#endif /* POLARSSL_HAVE_TIME */

#if defined(POLARSSL_SELF_TEST)

#include "polarssl/x509_crt.h"
#include "polarssl/certs.h"

/*
 * Checkup routine
 */
int x509_self_test( int verbose )
{
#if defined(POLARSSL_CERTS_C) && defined(POLARSSL_SHA1_C)
    int ret;
    int flags;
    x509_crt cacert;
    x509_crt clicert;

    if( verbose != 0 )
        polarssl_printf( "  X.509 certificate load: " );

    x509_crt_init( &clicert );

    ret = x509_crt_parse( &clicert, (const unsigned char *) test_cli_crt,
                          strlen( test_cli_crt ) );
    if( ret != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( ret );
    }

    x509_crt_init( &cacert );

    ret = x509_crt_parse( &cacert, (const unsigned char *) test_ca_crt,
                          strlen( test_ca_crt ) );
    if( ret != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( ret );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n  X.509 signature verify: ");

    ret = x509_crt_verify( &clicert, &cacert, NULL, NULL, &flags, NULL, NULL );
    if( ret != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        polarssl_printf( "ret = %d, &flags = %04x\n", ret, flags );

        return( ret );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n\n");

    x509_crt_free( &cacert  );
    x509_crt_free( &clicert );

    return( 0 );
#else
    ((void) verbose);
    return( POLARSSL_ERR_X509_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CERTS_C && POLARSSL_SHA1_C */
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_X509_USE_C */
