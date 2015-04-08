/*
 *  X.509 Certidicate Revocation List (CRL) parsing
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

#if defined(POLARSSL_X509_CRL_PARSE_C)

#include "polarssl/x509_crl.h"
#include "polarssl/oid.h"
#if defined(POLARSSL_PEM_PARSE_C)
#include "polarssl/pem.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
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

#if defined(POLARSSL_FS_IO) || defined(EFIX64) || defined(EFI32)
#include <stdio.h>
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 *  Version  ::=  INTEGER  {  v1(0), v2(1)  }
 */
static int x509_crl_get_version( unsigned char **p,
                             const unsigned char *end,
                             int *ver )
{
    int ret;

    if( ( ret = asn1_get_int( p, end, ver ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        {
            *ver = 0;
            return( 0 );
        }

        return( POLARSSL_ERR_X509_INVALID_VERSION + ret );
    }

    return( 0 );
}

/*
 * X.509 CRL v2 extensions (no extensions parsed yet.)
 */
static int x509_get_crl_ext( unsigned char **p,
                             const unsigned char *end,
                             x509_buf *ext )
{
    int ret;
    size_t len = 0;

    /* Get explicit tag */
    if( ( ret = x509_get_ext( p, end, ext, 0) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( 0 );

        return( ret );
    }

    while( *p < end )
    {
        if( ( ret = asn1_get_tag( p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_EXTENSIONS + ret );

        *p += len;
    }

    if( *p != end )
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 * X.509 CRL v2 entry extensions (no extensions parsed yet.)
 */
static int x509_get_crl_entry_ext( unsigned char **p,
                             const unsigned char *end,
                             x509_buf *ext )
{
    int ret;
    size_t len = 0;

    /* OPTIONAL */
    if( end <= *p )
        return( 0 );

    ext->tag = **p;
    ext->p = *p;

    /*
     * Get CRL-entry extension sequence header
     * crlEntryExtensions      Extensions OPTIONAL  -- if present, MUST be v2
     */
    if( ( ret = asn1_get_tag( p, end, &ext->len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
        {
            ext->p = NULL;
            return( 0 );
        }
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS + ret );
    }

    end = *p + ext->len;

    if( end != *p + ext->len )
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    while( *p < end )
    {
        if( ( ret = asn1_get_tag( p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
            return( POLARSSL_ERR_X509_INVALID_EXTENSIONS + ret );

        *p += len;
    }

    if( *p != end )
        return( POLARSSL_ERR_X509_INVALID_EXTENSIONS +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

/*
 * X.509 CRL Entries
 */
static int x509_get_entries( unsigned char **p,
                             const unsigned char *end,
                             x509_crl_entry *entry )
{
    int ret;
    size_t entry_len;
    x509_crl_entry *cur_entry = entry;

    if( *p == end )
        return( 0 );

    if( ( ret = asn1_get_tag( p, end, &entry_len,
            ASN1_SEQUENCE | ASN1_CONSTRUCTED ) ) != 0 )
    {
        if( ret == POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( 0 );

        return( ret );
    }

    end = *p + entry_len;

    while( *p < end )
    {
        size_t len2;
        const unsigned char *end2;

        if( ( ret = asn1_get_tag( p, end, &len2,
                ASN1_SEQUENCE | ASN1_CONSTRUCTED ) ) != 0 )
        {
            return( ret );
        }

        cur_entry->raw.tag = **p;
        cur_entry->raw.p = *p;
        cur_entry->raw.len = len2;
        end2 = *p + len2;

        if( ( ret = x509_get_serial( p, end2, &cur_entry->serial ) ) != 0 )
            return( ret );

        if( ( ret = x509_get_time( p, end2,
                                   &cur_entry->revocation_date ) ) != 0 )
            return( ret );

        if( ( ret = x509_get_crl_entry_ext( p, end2,
                                            &cur_entry->entry_ext ) ) != 0 )
            return( ret );

        if( *p < end )
        {
            cur_entry->next = polarssl_malloc( sizeof( x509_crl_entry ) );

            if( cur_entry->next == NULL )
                return( POLARSSL_ERR_X509_MALLOC_FAILED );

            memset( cur_entry->next, 0, sizeof( x509_crl_entry ) );
            cur_entry = cur_entry->next;
        }
    }

    return( 0 );
}

/*
 * Parse one  CRLs in DER format and append it to the chained list
 */
int x509_crl_parse_der( x509_crl *chain,
                        const unsigned char *buf, size_t buflen )
{
    int ret;
    size_t len;
    unsigned char *p, *end;
    x509_buf sig_params1, sig_params2;
    x509_crl *crl = chain;

    /*
     * Check for valid input
     */
    if( crl == NULL || buf == NULL )
        return( POLARSSL_ERR_X509_BAD_INPUT_DATA );

    memset( &sig_params1, 0, sizeof( x509_buf ) );
    memset( &sig_params2, 0, sizeof( x509_buf ) );

    /*
     * Add new CRL on the end of the chain if needed.
     */
    while( crl->version != 0 && crl->next != NULL )
        crl = crl->next;

    if( crl->version != 0 && crl->next == NULL )
    {
        crl->next = (x509_crl *) polarssl_malloc( sizeof( x509_crl ) );

        if( crl->next == NULL )
        {
            x509_crl_free( crl );
            return( POLARSSL_ERR_X509_MALLOC_FAILED );
        }

        x509_crl_init( crl->next );
        crl = crl->next;
    }

    /*
     * Copy raw DER-encoded CRL
     */
    if( ( p = polarssl_malloc( buflen ) ) == NULL )
        return( POLARSSL_ERR_X509_MALLOC_FAILED );

    memcpy( p, buf, buflen );

    crl->raw.p = p;
    crl->raw.len = buflen;

    end = p + buflen;

    /*
     * CertificateList  ::=  SEQUENCE  {
     *      tbsCertList          TBSCertList,
     *      signatureAlgorithm   AlgorithmIdentifier,
     *      signatureValue       BIT STRING  }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT );
    }

    if( len != (size_t) ( end - p ) )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    /*
     * TBSCertList  ::=  SEQUENCE  {
     */
    crl->tbs.p = p;

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT + ret );
    }

    end = p + len;
    crl->tbs.len = end - crl->tbs.p;

    /*
     * Version  ::=  INTEGER  OPTIONAL {  v1(0), v2(1)  }
     *               -- if present, MUST be v2
     *
     * signature            AlgorithmIdentifier
     */
    if( ( ret = x509_crl_get_version( &p, end, &crl->version ) ) != 0 ||
        ( ret = x509_get_alg( &p, end, &crl->sig_oid1, &sig_params1 ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    crl->version++;

    if( crl->version > 2 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_UNKNOWN_VERSION );
    }

    if( ( ret = x509_get_sig_alg( &crl->sig_oid1, &sig_params1,
                                  &crl->sig_md, &crl->sig_pk,
                                  &crl->sig_opts ) ) != 0 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_UNKNOWN_SIG_ALG );
    }

    /*
     * issuer               Name
     */
    crl->issuer_raw.p = p;

    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT + ret );
    }

    if( ( ret = x509_get_name( &p, p + len, &crl->issuer ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    crl->issuer_raw.len = p - crl->issuer_raw.p;

    /*
     * thisUpdate          Time
     * nextUpdate          Time OPTIONAL
     */
    if( ( ret = x509_get_time( &p, end, &crl->this_update ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    if( ( ret = x509_get_time( &p, end, &crl->next_update ) ) != 0 )
    {
        if( ret != ( POLARSSL_ERR_X509_INVALID_DATE +
                        POLARSSL_ERR_ASN1_UNEXPECTED_TAG ) &&
            ret != ( POLARSSL_ERR_X509_INVALID_DATE +
                        POLARSSL_ERR_ASN1_OUT_OF_DATA ) )
        {
            x509_crl_free( crl );
            return( ret );
        }
    }

    /*
     * revokedCertificates    SEQUENCE OF SEQUENCE   {
     *      userCertificate        CertificateSerialNumber,
     *      revocationDate         Time,
     *      crlEntryExtensions     Extensions OPTIONAL
     *                                   -- if present, MUST be v2
     *                        } OPTIONAL
     */
    if( ( ret = x509_get_entries( &p, end, &crl->entry ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    /*
     * crlExtensions          EXPLICIT Extensions OPTIONAL
     *                              -- if present, MUST be v2
     */
    if( crl->version == 2 )
    {
        ret = x509_get_crl_ext( &p, end, &crl->crl_ext );

        if( ret != 0 )
        {
            x509_crl_free( crl );
            return( ret );
        }
    }

    if( p != end )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    end = crl->raw.p + crl->raw.len;

    /*
     *  signatureAlgorithm   AlgorithmIdentifier,
     *  signatureValue       BIT STRING
     */
    if( ( ret = x509_get_alg( &p, end, &crl->sig_oid2, &sig_params2 ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    if( crl->sig_oid1.len != crl->sig_oid2.len ||
        memcmp( crl->sig_oid1.p, crl->sig_oid2.p, crl->sig_oid1.len ) != 0 ||
        sig_params1.len != sig_params2.len ||
        memcmp( sig_params1.p, sig_params2.p, sig_params1.len ) != 0 )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_SIG_MISMATCH );
    }

    if( ( ret = x509_get_sig( &p, end, &crl->sig ) ) != 0 )
    {
        x509_crl_free( crl );
        return( ret );
    }

    if( p != end )
    {
        x509_crl_free( crl );
        return( POLARSSL_ERR_X509_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    return( 0 );
}

/*
 * Parse one or more CRLs and add them to the chained list
 */
int x509_crl_parse( x509_crl *chain, const unsigned char *buf, size_t buflen )
{
#if defined(POLARSSL_PEM_PARSE_C)
    int ret;
    size_t use_len;
    pem_context pem;
    int is_pem = 0;

    if( chain == NULL || buf == NULL )
        return( POLARSSL_ERR_X509_BAD_INPUT_DATA );

    do
    {
        pem_init( &pem );
        ret = pem_read_buffer( &pem,
                               "-----BEGIN X509 CRL-----",
                               "-----END X509 CRL-----",
                               buf, NULL, 0, &use_len );

        if( ret == 0 )
        {
            /*
             * Was PEM encoded
             */
            is_pem = 1;

            buflen -= use_len;
            buf += use_len;

            if( ( ret = x509_crl_parse_der( chain,
                                            pem.buf, pem.buflen ) ) != 0 )
            {
                return( ret );
            }

            pem_free( &pem );
        }
        else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        {
            pem_free( &pem );
            return( ret );
        }
    }
    while( is_pem && buflen > 0 );

    if( is_pem )
        return( 0 );
    else
#endif /* POLARSSL_PEM_PARSE_C */
        return( x509_crl_parse_der( chain, buf, buflen ) );
}

#if defined(POLARSSL_FS_IO)
/*
 * Load one or more CRLs and add them to the chained list
 */
int x509_crl_parse_file( x509_crl *chain, const char *path )
{
    int ret;
    size_t n;
    unsigned char *buf;

    if( ( ret = pk_load_file( path, &buf, &n ) ) != 0 )
        return( ret );

    ret = x509_crl_parse( chain, buf, n );

    polarssl_zeroize( buf, n + 1 );
    polarssl_free( buf );

    return( ret );
}
#endif /* POLARSSL_FS_IO */

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
 * Return an informational string about the certificate.
 */
#define BEFORE_COLON    14
#define BC              "14"
/*
 * Return an informational string about the CRL.
 */
int x509_crl_info( char *buf, size_t size, const char *prefix,
                   const x509_crl *crl )
{
    int ret;
    size_t n;
    char *p;
    const x509_crl_entry *entry;

    p = buf;
    n = size;

    ret = snprintf( p, n, "%sCRL version   : %d",
                               prefix, crl->version );
    SAFE_SNPRINTF();

    ret = snprintf( p, n, "\n%sissuer name   : ", prefix );
    SAFE_SNPRINTF();
    ret = x509_dn_gets( p, n, &crl->issuer );
    SAFE_SNPRINTF();

    ret = snprintf( p, n, "\n%sthis update   : " \
                   "%04d-%02d-%02d %02d:%02d:%02d", prefix,
                   crl->this_update.year, crl->this_update.mon,
                   crl->this_update.day,  crl->this_update.hour,
                   crl->this_update.min,  crl->this_update.sec );
    SAFE_SNPRINTF();

    ret = snprintf( p, n, "\n%snext update   : " \
                   "%04d-%02d-%02d %02d:%02d:%02d", prefix,
                   crl->next_update.year, crl->next_update.mon,
                   crl->next_update.day,  crl->next_update.hour,
                   crl->next_update.min,  crl->next_update.sec );
    SAFE_SNPRINTF();

    entry = &crl->entry;

    ret = snprintf( p, n, "\n%sRevoked certificates:",
                               prefix );
    SAFE_SNPRINTF();

    while( entry != NULL && entry->raw.len != 0 )
    {
        ret = snprintf( p, n, "\n%sserial number: ",
                               prefix );
        SAFE_SNPRINTF();

        ret = x509_serial_gets( p, n, &entry->serial );
        SAFE_SNPRINTF();

        ret = snprintf( p, n, " revocation date: " \
                   "%04d-%02d-%02d %02d:%02d:%02d",
                   entry->revocation_date.year, entry->revocation_date.mon,
                   entry->revocation_date.day,  entry->revocation_date.hour,
                   entry->revocation_date.min,  entry->revocation_date.sec );
        SAFE_SNPRINTF();

        entry = entry->next;
    }

    ret = snprintf( p, n, "\n%ssigned using  : ", prefix );
    SAFE_SNPRINTF();

    ret = x509_sig_alg_gets( p, n, &crl->sig_oid1, crl->sig_pk, crl->sig_md,
                             crl->sig_opts );
    SAFE_SNPRINTF();

    ret = snprintf( p, n, "\n" );
    SAFE_SNPRINTF();

    return( (int) ( size - n ) );
}

/*
 * Initialize a CRL chain
 */
void x509_crl_init( x509_crl *crl )
{
    memset( crl, 0, sizeof(x509_crl) );
}

/*
 * Unallocate all CRL data
 */
void x509_crl_free( x509_crl *crl )
{
    x509_crl *crl_cur = crl;
    x509_crl *crl_prv;
    x509_name *name_cur;
    x509_name *name_prv;
    x509_crl_entry *entry_cur;
    x509_crl_entry *entry_prv;

    if( crl == NULL )
        return;

    do
    {
#if defined(POLARSSL_X509_RSASSA_PSS_SUPPORT)
        polarssl_free( crl_cur->sig_opts );
#endif

        name_cur = crl_cur->issuer.next;
        while( name_cur != NULL )
        {
            name_prv = name_cur;
            name_cur = name_cur->next;
            polarssl_zeroize( name_prv, sizeof( x509_name ) );
            polarssl_free( name_prv );
        }

        entry_cur = crl_cur->entry.next;
        while( entry_cur != NULL )
        {
            entry_prv = entry_cur;
            entry_cur = entry_cur->next;
            polarssl_zeroize( entry_prv, sizeof( x509_crl_entry ) );
            polarssl_free( entry_prv );
        }

        if( crl_cur->raw.p != NULL )
        {
            polarssl_zeroize( crl_cur->raw.p, crl_cur->raw.len );
            polarssl_free( crl_cur->raw.p );
        }

        crl_cur = crl_cur->next;
    }
    while( crl_cur != NULL );

    crl_cur = crl;
    do
    {
        crl_prv = crl_cur;
        crl_cur = crl_cur->next;

        polarssl_zeroize( crl_prv, sizeof( x509_crl ) );
        if( crl_prv != crl )
            polarssl_free( crl_prv );
    }
    while( crl_cur != NULL );
}

#endif /* POLARSSL_X509_CRL_PARSE_C */
