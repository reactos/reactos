/*
 * ASN.1 buffer writing functionality
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

#if defined(POLARSSL_ASN1_WRITE_C)

#include "polarssl/asn1write.h"

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdlib.h>
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

int asn1_write_len( unsigned char **p, unsigned char *start, size_t len )
{
    if( len < 0x80 )
    {
        if( *p - start < 1 )
            return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

        *--(*p) = (unsigned char) len;
        return( 1 );
    }

    if( len <= 0xFF )
    {
        if( *p - start < 2 )
            return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

        *--(*p) = (unsigned char) len;
        *--(*p) = 0x81;
        return( 2 );
    }

    if( *p - start < 3 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    // We assume we never have lengths larger than 65535 bytes
    //
    *--(*p) = len % 256;
    *--(*p) = ( len / 256 ) % 256;
    *--(*p) = 0x82;

    return( 3 );
}

int asn1_write_tag( unsigned char **p, unsigned char *start, unsigned char tag )
{
    if( *p - start < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    *--(*p) = tag;

    return( 1 );
}

int asn1_write_raw_buffer( unsigned char **p, unsigned char *start,
                           const unsigned char *buf, size_t size )
{
    size_t len = 0;

    if( *p - start < (int) size )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    len = size;
    (*p) -= len;
    memcpy( *p, buf, len );

    return( (int) len );
}

#if defined(POLARSSL_BIGNUM_C)
int asn1_write_mpi( unsigned char **p, unsigned char *start, mpi *X )
{
    int ret;
    size_t len = 0;

    // Write the MPI
    //
    len = mpi_size( X );

    if( *p - start < (int) len )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    (*p) -= len;
    MPI_CHK( mpi_write_binary( X, *p, len ) );

    // DER format assumes 2s complement for numbers, so the leftmost bit
    // should be 0 for positive numbers and 1 for negative numbers.
    //
    if( X->s ==1 && **p & 0x80 )
    {
        if( *p - start < 1 )
            return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

        *--(*p) = 0x00;
        len += 1;
    }

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_INTEGER ) );

    ret = (int) len;

cleanup:
    return( ret );
}
#endif /* POLARSSL_BIGNUM_C */

int asn1_write_null( unsigned char **p, unsigned char *start )
{
    int ret;
    size_t len = 0;

    // Write NULL
    //
    ASN1_CHK_ADD( len, asn1_write_len( p, start, 0) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_NULL ) );

    return( (int) len );
}

int asn1_write_oid( unsigned char **p, unsigned char *start,
                    const char *oid, size_t oid_len )
{
    int ret;
    size_t len = 0;

    ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start,
                                  (const unsigned char *) oid, oid_len ) );
    ASN1_CHK_ADD( len , asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len , asn1_write_tag( p, start, ASN1_OID ) );

    return( (int) len );
}

int asn1_write_algorithm_identifier( unsigned char **p, unsigned char *start,
                                     const char *oid, size_t oid_len,
                                     size_t par_len )
{
    int ret;
    size_t len = 0;

    if( par_len == 0 )
        ASN1_CHK_ADD( len, asn1_write_null( p, start ) );
    else
        len += par_len;

    ASN1_CHK_ADD( len, asn1_write_oid( p, start, oid, oid_len ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start,
                                       ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    return( (int) len );
}

int asn1_write_bool( unsigned char **p, unsigned char *start, int boolean )
{
    int ret;
    size_t len = 0;

    if( *p - start < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    *--(*p) = (boolean) ? 1 : 0;
    len++;

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_BOOLEAN ) );

    return( (int) len );
}

int asn1_write_int( unsigned char **p, unsigned char *start, int val )
{
    int ret;
    size_t len = 0;

    // TODO negative values and values larger than 128
    // DER format assumes 2s complement for numbers, so the leftmost bit
    // should be 0 for positive numbers and 1 for negative numbers.
    //
    if( *p - start < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    len += 1;
    *--(*p) = val;

    if( val > 0 && **p & 0x80 )
    {
        if( *p - start < 1 )
            return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

        *--(*p) = 0x00;
        len += 1;
    }

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_INTEGER ) );

    return( (int) len );
}

int asn1_write_printable_string( unsigned char **p, unsigned char *start,
                                 const char *text, size_t text_len )
{
    int ret;
    size_t len = 0;

    ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start,
                  (const unsigned char *) text, text_len ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_PRINTABLE_STRING ) );

    return( (int) len );
}

int asn1_write_ia5_string( unsigned char **p, unsigned char *start,
                           const char *text, size_t text_len )
{
    int ret;
    size_t len = 0;

    ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start,
                  (const unsigned char *) text, text_len ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_IA5_STRING ) );

    return( (int) len );
}

int asn1_write_bitstring( unsigned char **p, unsigned char *start,
                          const unsigned char *buf, size_t bits )
{
    int ret;
    size_t len = 0, size;

    size = ( bits / 8 ) + ( ( bits % 8 ) ? 1 : 0 );

    // Calculate byte length
    //
    if( *p - start < (int) size + 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    len = size + 1;
    (*p) -= size;
    memcpy( *p, buf, size );

    // Write unused bits
    //
    *--(*p) = (unsigned char) (size * 8 - bits);

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_BIT_STRING ) );

    return( (int) len );
}

int asn1_write_octet_string( unsigned char **p, unsigned char *start,
                             const unsigned char *buf, size_t size )
{
    int ret;
    size_t len = 0;

    ASN1_CHK_ADD( len, asn1_write_raw_buffer( p, start, buf, size ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_OCTET_STRING ) );

    return( (int) len );
}

asn1_named_data *asn1_store_named_data( asn1_named_data **head,
                                        const char *oid, size_t oid_len,
                                        const unsigned char *val,
                                        size_t val_len )
{
    asn1_named_data *cur;

    if( ( cur = asn1_find_named_data( *head, oid, oid_len ) ) == NULL )
    {
        // Add new entry if not present yet based on OID
        //
        if( ( cur = polarssl_malloc( sizeof(asn1_named_data) ) ) == NULL )
            return( NULL );

        memset( cur, 0, sizeof(asn1_named_data) );

        cur->oid.len = oid_len;
        cur->oid.p = polarssl_malloc( oid_len );
        if( cur->oid.p == NULL )
        {
            polarssl_free( cur );
            return( NULL );
        }

        memcpy( cur->oid.p, oid, oid_len );

        cur->val.len = val_len;
        cur->val.p = polarssl_malloc( val_len );
        if( cur->val.p == NULL )
        {
            polarssl_free( cur->oid.p );
            polarssl_free( cur );
            return( NULL );
        }

        cur->next = *head;
        *head = cur;
    }
    else if( cur->val.len < val_len )
    {
        // Enlarge existing value buffer if needed
        //
        polarssl_free( cur->val.p );
        cur->val.p = NULL;

        cur->val.len = val_len;
        cur->val.p = polarssl_malloc( val_len );
        if( cur->val.p == NULL )
        {
            polarssl_free( cur->oid.p );
            polarssl_free( cur );
            return( NULL );
        }
    }

    if( val != NULL )
        memcpy( cur->val.p, val, val_len );

    return( cur );
}
#endif /* POLARSSL_ASN1_WRITE_C */
