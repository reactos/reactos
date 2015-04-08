/*
 *  SSL session cache implementation
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
 * These session callbacks use a simple chained list
 * to store and retrieve the session information.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_SSL_CACHE_C)

#include "polarssl/ssl_cache.h"

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#include <stdlib.h>

void ssl_cache_init( ssl_cache_context *cache )
{
    memset( cache, 0, sizeof( ssl_cache_context ) );

    cache->timeout = SSL_CACHE_DEFAULT_TIMEOUT;
    cache->max_entries = SSL_CACHE_DEFAULT_MAX_ENTRIES;

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_init( &cache->mutex );
#endif
}

int ssl_cache_get( void *data, ssl_session *session )
{
    int ret = 1;
#if defined(POLARSSL_HAVE_TIME)
    time_t t = time( NULL );
#endif
    ssl_cache_context *cache = (ssl_cache_context *) data;
    ssl_cache_entry *cur, *entry;

#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_lock( &cache->mutex ) != 0 )
        return( 1 );
#endif

    cur = cache->chain;
    entry = NULL;

    while( cur != NULL )
    {
        entry = cur;
        cur = cur->next;

#if defined(POLARSSL_HAVE_TIME)
        if( cache->timeout != 0 &&
            (int) ( t - entry->timestamp ) > cache->timeout )
            continue;
#endif

        if( session->ciphersuite != entry->session.ciphersuite ||
            session->compression != entry->session.compression ||
            session->length != entry->session.length )
            continue;

        if( memcmp( session->id, entry->session.id,
                    entry->session.length ) != 0 )
            continue;

        memcpy( session->master, entry->session.master, 48 );

        session->verify_result = entry->session.verify_result;

#if defined(POLARSSL_X509_CRT_PARSE_C)
        /*
         * Restore peer certificate (without rest of the original chain)
         */
        if( entry->peer_cert.p != NULL )
        {
            if( ( session->peer_cert = (x509_crt *) polarssl_malloc(
                                 sizeof(x509_crt) ) ) == NULL )
            {
                ret = 1;
                goto exit;
            }

            x509_crt_init( session->peer_cert );
            if( x509_crt_parse( session->peer_cert, entry->peer_cert.p,
                                entry->peer_cert.len ) != 0 )
            {
                polarssl_free( session->peer_cert );
                session->peer_cert = NULL;
                ret = 1;
                goto exit;
            }
        }
#endif /* POLARSSL_X509_CRT_PARSE_C */

        ret = 0;
        goto exit;
    }

exit:
#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &cache->mutex ) != 0 )
        ret = 1;
#endif

    return( ret );
}

int ssl_cache_set( void *data, const ssl_session *session )
{
    int ret = 1;
#if defined(POLARSSL_HAVE_TIME)
    time_t t = time( NULL ), oldest = 0;
    ssl_cache_entry *old = NULL;
#endif
    ssl_cache_context *cache = (ssl_cache_context *) data;
    ssl_cache_entry *cur, *prv;
    int count = 0;

#if defined(POLARSSL_THREADING_C)
    if( ( ret = polarssl_mutex_lock( &cache->mutex ) ) != 0 )
        return( ret );
#endif

    cur = cache->chain;
    prv = NULL;

    while( cur != NULL )
    {
        count++;

#if defined(POLARSSL_HAVE_TIME)
        if( cache->timeout != 0 &&
            (int) ( t - cur->timestamp ) > cache->timeout )
        {
            cur->timestamp = t;
            break; /* expired, reuse this slot, update timestamp */
        }
#endif

        if( memcmp( session->id, cur->session.id, cur->session.length ) == 0 )
            break; /* client reconnected, keep timestamp for session id */

#if defined(POLARSSL_HAVE_TIME)
        if( oldest == 0 || cur->timestamp < oldest )
        {
            oldest = cur->timestamp;
            old = cur;
        }
#endif

        prv = cur;
        cur = cur->next;
    }

    if( cur == NULL )
    {
#if defined(POLARSSL_HAVE_TIME)
        /*
         * Reuse oldest entry if max_entries reached
         */
        if( count >= cache->max_entries )
        {
            if( old == NULL )
            {
                ret = 1;
                goto exit;
            }

            cur = old;
        }
#else /* POLARSSL_HAVE_TIME */
        /*
         * Reuse first entry in chain if max_entries reached,
         * but move to last place
         */
        if( count >= cache->max_entries )
        {
            if( cache->chain == NULL )
            {
                ret = 1;
                goto exit;
            }

            cur = cache->chain;
            cache->chain = cur->next;
            cur->next = NULL;
            prv->next = cur;
        }
#endif /* POLARSSL_HAVE_TIME */
        else
        {
            /*
             * max_entries not reached, create new entry
             */
            cur = (ssl_cache_entry *) polarssl_malloc( sizeof(ssl_cache_entry) );
            if( cur == NULL )
            {
                ret = 1;
                goto exit;
            }

            memset( cur, 0, sizeof(ssl_cache_entry) );

            if( prv == NULL )
                cache->chain = cur;
            else
                prv->next = cur;
        }

#if defined(POLARSSL_HAVE_TIME)
        cur->timestamp = t;
#endif
    }

    memcpy( &cur->session, session, sizeof( ssl_session ) );

#if defined(POLARSSL_X509_CRT_PARSE_C)
    /*
     * If we're reusing an entry, free its certificate first
     */
    if( cur->peer_cert.p != NULL )
    {
        polarssl_free( cur->peer_cert.p );
        memset( &cur->peer_cert, 0, sizeof(x509_buf) );
    }

    /*
     * Store peer certificate
     */
    if( session->peer_cert != NULL )
    {
        cur->peer_cert.p = (unsigned char *) polarssl_malloc(
                            session->peer_cert->raw.len );
        if( cur->peer_cert.p == NULL )
        {
            ret = 1;
            goto exit;
        }

        memcpy( cur->peer_cert.p, session->peer_cert->raw.p,
                session->peer_cert->raw.len );
        cur->peer_cert.len = session->peer_cert->raw.len;

        cur->session.peer_cert = NULL;
    }
#endif /* POLARSSL_X509_CRT_PARSE_C */

    ret = 0;

exit:
#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &cache->mutex ) != 0 )
        ret = 1;
#endif

    return( ret );
}

#if defined(POLARSSL_HAVE_TIME)
void ssl_cache_set_timeout( ssl_cache_context *cache, int timeout )
{
    if( timeout < 0 ) timeout = 0;

    cache->timeout = timeout;
}
#endif /* POLARSSL_HAVE_TIME */

void ssl_cache_set_max_entries( ssl_cache_context *cache, int max )
{
    if( max < 0 ) max = 0;

    cache->max_entries = max;
}

void ssl_cache_free( ssl_cache_context *cache )
{
    ssl_cache_entry *cur, *prv;

    cur = cache->chain;

    while( cur != NULL )
    {
        prv = cur;
        cur = cur->next;

        ssl_session_free( &prv->session );

#if defined(POLARSSL_X509_CRT_PARSE_C)
        polarssl_free( prv->peer_cert.p );
#endif /* POLARSSL_X509_CRT_PARSE_C */

        polarssl_free( prv );
    }

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_free( &cache->mutex );
#endif
}

#endif /* POLARSSL_SSL_CACHE_C */
