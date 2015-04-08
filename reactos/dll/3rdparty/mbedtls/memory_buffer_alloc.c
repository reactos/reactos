/*
 *  Buffer-based memory allocator
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

#if defined(POLARSSL_MEMORY_BUFFER_ALLOC_C)

#include "polarssl/memory_buffer_alloc.h"

#include <string.h>

#if defined(POLARSSL_MEMORY_DEBUG)
#include <stdio.h>
#endif
#if defined(POLARSSL_MEMORY_BACKTRACE)
#include <execinfo.h>
#endif

#if defined(POLARSSL_THREADING_C)
#include "polarssl/threading.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_fprintf fprintf
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#define MAGIC1       0xFF00AA55
#define MAGIC2       0xEE119966
#define MAX_BT 20

typedef struct _memory_header memory_header;
struct _memory_header
{
    size_t          magic1;
    size_t          size;
    size_t          alloc;
    memory_header   *prev;
    memory_header   *next;
    memory_header   *prev_free;
    memory_header   *next_free;
#if defined(POLARSSL_MEMORY_BACKTRACE)
    char            **trace;
    size_t          trace_count;
#endif
    size_t          magic2;
};

typedef struct
{
    unsigned char   *buf;
    size_t          len;
    memory_header   *first;
    memory_header   *first_free;
    size_t          current_alloc_size;
    int             verify;
#if defined(POLARSSL_MEMORY_DEBUG)
    size_t          malloc_count;
    size_t          free_count;
    size_t          total_used;
    size_t          maximum_used;
    size_t          header_count;
    size_t          maximum_header_count;
#endif
#if defined(POLARSSL_THREADING_C)
    threading_mutex_t   mutex;
#endif
}
buffer_alloc_ctx;

static buffer_alloc_ctx heap;

#if defined(POLARSSL_MEMORY_DEBUG)
static void debug_header( memory_header *hdr )
{
#if defined(POLARSSL_MEMORY_BACKTRACE)
    size_t i;
#endif

    polarssl_fprintf( stderr, "HDR:  PTR(%10zu), PREV(%10zu), NEXT(%10zu), "
                              "ALLOC(%zu), SIZE(%10zu)\n",
                      (size_t) hdr, (size_t) hdr->prev, (size_t) hdr->next,
                      hdr->alloc, hdr->size );
    polarssl_fprintf( stderr, "      FPREV(%10zu), FNEXT(%10zu)\n",
                      (size_t) hdr->prev_free, (size_t) hdr->next_free );

#if defined(POLARSSL_MEMORY_BACKTRACE)
    polarssl_fprintf( stderr, "TRACE: \n" );
    for( i = 0; i < hdr->trace_count; i++ )
        polarssl_fprintf( stderr, "%s\n", hdr->trace[i] );
    polarssl_fprintf( stderr, "\n" );
#endif
}

static void debug_chain()
{
    memory_header *cur = heap.first;

    polarssl_fprintf( stderr, "\nBlock list\n" );
    while( cur != NULL )
    {
        debug_header( cur );
        cur = cur->next;
    }

    polarssl_fprintf( stderr, "Free list\n" );
    cur = heap.first_free;

    while( cur != NULL )
    {
        debug_header( cur );
        cur = cur->next_free;
    }
}
#endif /* POLARSSL_MEMORY_DEBUG */

static int verify_header( memory_header *hdr )
{
    if( hdr->magic1 != MAGIC1 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: MAGIC1 mismatch\n" );
#endif
        return( 1 );
    }

    if( hdr->magic2 != MAGIC2 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: MAGIC2 mismatch\n" );
#endif
        return( 1 );
    }

    if( hdr->alloc > 1 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: alloc has illegal value\n" );
#endif
        return( 1 );
    }

    if( hdr->prev != NULL && hdr->prev == hdr->next )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: prev == next\n" );
#endif
        return( 1 );
    }

    if( hdr->prev_free != NULL && hdr->prev_free == hdr->next_free )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: prev_free == next_free\n" );
#endif
        return( 1 );
    }

    return( 0 );
}

static int verify_chain()
{
    memory_header *prv = heap.first, *cur = heap.first->next;

    if( verify_header( heap.first ) != 0 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: verification of first header "
                                  "failed\n" );
#endif
        return( 1 );
    }

    if( heap.first->prev != NULL )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: verification failed: "
                                  "first->prev != NULL\n" );
#endif
        return( 1 );
    }

    while( cur != NULL )
    {
        if( verify_header( cur ) != 0 )
        {
#if defined(POLARSSL_MEMORY_DEBUG)
            polarssl_fprintf( stderr, "FATAL: verification of header "
                                      "failed\n" );
#endif
            return( 1 );
        }

        if( cur->prev != prv )
        {
#if defined(POLARSSL_MEMORY_DEBUG)
            polarssl_fprintf( stderr, "FATAL: verification failed: "
                                      "cur->prev != prv\n" );
#endif
            return( 1 );
        }

        prv = cur;
        cur = cur->next;
    }

    return( 0 );
}

static void *buffer_alloc_malloc( size_t len )
{
    memory_header *new, *cur = heap.first_free;
    unsigned char *p;
#if defined(POLARSSL_MEMORY_BACKTRACE)
    void *trace_buffer[MAX_BT];
    size_t trace_cnt;
#endif

    if( heap.buf == NULL || heap.first == NULL )
        return( NULL );

    if( len % POLARSSL_MEMORY_ALIGN_MULTIPLE )
    {
        len -= len % POLARSSL_MEMORY_ALIGN_MULTIPLE;
        len += POLARSSL_MEMORY_ALIGN_MULTIPLE;
    }

    // Find block that fits
    //
    while( cur != NULL )
    {
        if( cur->size >= len )
            break;

        cur = cur->next_free;
    }

    if( cur == NULL )
        return( NULL );

    if( cur->alloc != 0 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: block in free_list but allocated "
                                  "data\n" );
#endif
        exit( 1 );
    }

#if defined(POLARSSL_MEMORY_DEBUG)
    heap.malloc_count++;
#endif

    // Found location, split block if > memory_header + 4 room left
    //
    if( cur->size - len < sizeof(memory_header) +
                          POLARSSL_MEMORY_ALIGN_MULTIPLE )
    {
        cur->alloc = 1;

        // Remove from free_list
        //
        if( cur->prev_free != NULL )
            cur->prev_free->next_free = cur->next_free;
        else
            heap.first_free = cur->next_free;

        if( cur->next_free != NULL )
            cur->next_free->prev_free = cur->prev_free;

        cur->prev_free = NULL;
        cur->next_free = NULL;

#if defined(POLARSSL_MEMORY_DEBUG)
        heap.total_used += cur->size;
        if( heap.total_used > heap.maximum_used )
            heap.maximum_used = heap.total_used;
#endif
#if defined(POLARSSL_MEMORY_BACKTRACE)
        trace_cnt = backtrace( trace_buffer, MAX_BT );
        cur->trace = backtrace_symbols( trace_buffer, trace_cnt );
        cur->trace_count = trace_cnt;
#endif

        if( ( heap.verify & MEMORY_VERIFY_ALLOC ) && verify_chain() != 0 )
            exit( 1 );

        return( ( (unsigned char *) cur ) + sizeof(memory_header) );
    }

    p = ( (unsigned char *) cur ) + sizeof(memory_header) + len;
    new = (memory_header *) p;

    new->size = cur->size - len - sizeof(memory_header);
    new->alloc = 0;
    new->prev = cur;
    new->next = cur->next;
#if defined(POLARSSL_MEMORY_BACKTRACE)
    new->trace = NULL;
    new->trace_count = 0;
#endif
    new->magic1 = MAGIC1;
    new->magic2 = MAGIC2;

    if( new->next != NULL )
        new->next->prev = new;

    // Replace cur with new in free_list
    //
    new->prev_free = cur->prev_free;
    new->next_free = cur->next_free;
    if( new->prev_free != NULL )
        new->prev_free->next_free = new;
    else
        heap.first_free = new;

    if( new->next_free != NULL )
        new->next_free->prev_free = new;

    cur->alloc = 1;
    cur->size = len;
    cur->next = new;
    cur->prev_free = NULL;
    cur->next_free = NULL;

#if defined(POLARSSL_MEMORY_DEBUG)
    heap.header_count++;
    if( heap.header_count > heap.maximum_header_count )
        heap.maximum_header_count = heap.header_count;
    heap.total_used += cur->size;
    if( heap.total_used > heap.maximum_used )
        heap.maximum_used = heap.total_used;
#endif
#if defined(POLARSSL_MEMORY_BACKTRACE)
    trace_cnt = backtrace( trace_buffer, MAX_BT );
    cur->trace = backtrace_symbols( trace_buffer, trace_cnt );
    cur->trace_count = trace_cnt;
#endif

    if( ( heap.verify & MEMORY_VERIFY_ALLOC ) && verify_chain() != 0 )
        exit( 1 );

    return( ( (unsigned char *) cur ) + sizeof(memory_header) );
}

static void buffer_alloc_free( void *ptr )
{
    memory_header *hdr, *old = NULL;
    unsigned char *p = (unsigned char *) ptr;

    if( ptr == NULL || heap.buf == NULL || heap.first == NULL )
        return;

    if( p < heap.buf || p > heap.buf + heap.len )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: polarssl_free() outside of managed "
                                  "space\n" );
#endif
        exit( 1 );
    }

    p -= sizeof(memory_header);
    hdr = (memory_header *) p;

    if( verify_header( hdr ) != 0 )
        exit( 1 );

    if( hdr->alloc != 1 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        polarssl_fprintf( stderr, "FATAL: polarssl_free() on unallocated "
                                  "data\n" );
#endif
        exit( 1 );
    }

    hdr->alloc = 0;

#if defined(POLARSSL_MEMORY_DEBUG)
    heap.free_count++;
    heap.total_used -= hdr->size;
#endif

    // Regroup with block before
    //
    if( hdr->prev != NULL && hdr->prev->alloc == 0 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        heap.header_count--;
#endif
        hdr->prev->size += sizeof(memory_header) + hdr->size;
        hdr->prev->next = hdr->next;
        old = hdr;
        hdr = hdr->prev;

        if( hdr->next != NULL )
            hdr->next->prev = hdr;

#if defined(POLARSSL_MEMORY_BACKTRACE)
        free( old->trace );
#endif
        memset( old, 0, sizeof(memory_header) );
    }

    // Regroup with block after
    //
    if( hdr->next != NULL && hdr->next->alloc == 0 )
    {
#if defined(POLARSSL_MEMORY_DEBUG)
        heap.header_count--;
#endif
        hdr->size += sizeof(memory_header) + hdr->next->size;
        old = hdr->next;
        hdr->next = hdr->next->next;

        if( hdr->prev_free != NULL || hdr->next_free != NULL )
        {
            if( hdr->prev_free != NULL )
                hdr->prev_free->next_free = hdr->next_free;
            else
                heap.first_free = hdr->next_free;

            if( hdr->next_free != NULL )
                hdr->next_free->prev_free = hdr->prev_free;
        }

        hdr->prev_free = old->prev_free;
        hdr->next_free = old->next_free;

        if( hdr->prev_free != NULL )
            hdr->prev_free->next_free = hdr;
        else
            heap.first_free = hdr;

        if( hdr->next_free != NULL )
            hdr->next_free->prev_free = hdr;

        if( hdr->next != NULL )
            hdr->next->prev = hdr;

#if defined(POLARSSL_MEMORY_BACKTRACE)
        free( old->trace );
#endif
        memset( old, 0, sizeof(memory_header) );
    }

    // Prepend to free_list if we have not merged
    // (Does not have to stay in same order as prev / next list)
    //
    if( old == NULL )
    {
        hdr->next_free = heap.first_free;
        if( heap.first_free != NULL )
            heap.first_free->prev_free = hdr;
        heap.first_free = hdr;
    }

#if defined(POLARSSL_MEMORY_BACKTRACE)
    hdr->trace = NULL;
    hdr->trace_count = 0;
#endif

    if( ( heap.verify & MEMORY_VERIFY_FREE ) && verify_chain() != 0 )
        exit( 1 );
}

void memory_buffer_set_verify( int verify )
{
    heap.verify = verify;
}

int memory_buffer_alloc_verify()
{
    return verify_chain();
}

#if defined(POLARSSL_MEMORY_DEBUG)
void memory_buffer_alloc_status()
{
    polarssl_fprintf( stderr,
                      "Current use: %zu blocks / %zu bytes, max: %zu blocks / "
                      "%zu bytes (total %zu bytes), malloc / free: %zu / %zu\n",
                      heap.header_count, heap.total_used,
                      heap.maximum_header_count, heap.maximum_used,
                      heap.maximum_header_count * sizeof( memory_header )
                      + heap.maximum_used,
                      heap.malloc_count, heap.free_count );

    if( heap.first->next == NULL )
        polarssl_fprintf( stderr, "All memory de-allocated in stack buffer\n" );
    else
    {
        polarssl_fprintf( stderr, "Memory currently allocated:\n" );
        debug_chain();
    }
}
#endif /* POLARSSL_MEMORY_DEBUG */

#if defined(POLARSSL_THREADING_C)
static void *buffer_alloc_malloc_mutexed( size_t len )
{
    void *buf;
    polarssl_mutex_lock( &heap.mutex );
    buf = buffer_alloc_malloc( len );
    polarssl_mutex_unlock( &heap.mutex );
    return( buf );
}

static void buffer_alloc_free_mutexed( void *ptr )
{
    polarssl_mutex_lock( &heap.mutex );
    buffer_alloc_free( ptr );
    polarssl_mutex_unlock( &heap.mutex );
}
#endif /* POLARSSL_THREADING_C */

int memory_buffer_alloc_init( unsigned char *buf, size_t len )
{
    memset( &heap, 0, sizeof(buffer_alloc_ctx) );
    memset( buf, 0, len );

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_init( &heap.mutex );
    platform_set_malloc_free( buffer_alloc_malloc_mutexed,
                              buffer_alloc_free_mutexed );
#else
    platform_set_malloc_free( buffer_alloc_malloc, buffer_alloc_free );
#endif

    if( (size_t) buf % POLARSSL_MEMORY_ALIGN_MULTIPLE )
    {
        /* Adjust len first since buf is used in the computation */
        len -= POLARSSL_MEMORY_ALIGN_MULTIPLE
             - (size_t) buf % POLARSSL_MEMORY_ALIGN_MULTIPLE;
        buf += POLARSSL_MEMORY_ALIGN_MULTIPLE
             - (size_t) buf % POLARSSL_MEMORY_ALIGN_MULTIPLE;
    }

    heap.buf = buf;
    heap.len = len;

    heap.first = (memory_header *) buf;
    heap.first->size = len - sizeof(memory_header);
    heap.first->magic1 = MAGIC1;
    heap.first->magic2 = MAGIC2;
    heap.first_free = heap.first;
    return( 0 );
}

void memory_buffer_alloc_free()
{
#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_free( &heap.mutex );
#endif
    polarssl_zeroize( &heap, sizeof(buffer_alloc_ctx) );
}

#if defined(POLARSSL_SELF_TEST)
static int check_pointer( void *p )
{
    if( p == NULL )
        return( -1 );

    if( (size_t) p % POLARSSL_MEMORY_ALIGN_MULTIPLE != 0 )
        return( -1 );

    return( 0 );
}

static int check_all_free( )
{
    if( heap.current_alloc_size != 0 ||
        heap.first != heap.first_free ||
        (void *) heap.first != (void *) heap.buf )
    {
        return( -1 );
    }

    return( 0 );
}

#define TEST_ASSERT( condition )            \
    if( ! (condition) )                     \
    {                                       \
        if( verbose != 0 )                  \
            polarssl_printf( "failed\n" );  \
                                            \
        ret = 1;                            \
        goto cleanup;                       \
    }

int memory_buffer_alloc_self_test( int verbose )
{
    unsigned char buf[1024];
    unsigned char *p, *q, *r, *end;
    int ret = 0;

    if( verbose != 0 )
        polarssl_printf( "  MBA test #1 (basic alloc-free cycle): " );

    memory_buffer_alloc_init( buf, sizeof( buf ) );

    p = polarssl_malloc( 1 );
    q = polarssl_malloc( 128 );
    r = polarssl_malloc( 16 );

    TEST_ASSERT( check_pointer( p ) == 0 &&
                 check_pointer( q ) == 0 &&
                 check_pointer( r ) == 0 );

    polarssl_free( r );
    polarssl_free( q );
    polarssl_free( p );

    TEST_ASSERT( check_all_free( ) == 0 );

    /* Memorize end to compare with the next test */
    end = heap.buf + heap.len;

    memory_buffer_alloc_free( );

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

    if( verbose != 0 )
        polarssl_printf( "  MBA test #2 (buf not aligned): " );

    memory_buffer_alloc_init( buf + 1, sizeof( buf ) - 1 );

    TEST_ASSERT( heap.buf + heap.len == end );

    p = polarssl_malloc( 1 );
    q = polarssl_malloc( 128 );
    r = polarssl_malloc( 16 );

    TEST_ASSERT( check_pointer( p ) == 0 &&
                 check_pointer( q ) == 0 &&
                 check_pointer( r ) == 0 );

    polarssl_free( r );
    polarssl_free( q );
    polarssl_free( p );

    TEST_ASSERT( check_all_free( ) == 0 );

    memory_buffer_alloc_free( );

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

    if( verbose != 0 )
        polarssl_printf( "  MBA test #3 (full): " );

    memory_buffer_alloc_init( buf, sizeof( buf ) );

    p = polarssl_malloc( sizeof( buf ) - sizeof( memory_header ) );

    TEST_ASSERT( check_pointer( p ) == 0 );
    TEST_ASSERT( polarssl_malloc( 1 ) == NULL );

    polarssl_free( p );

    p = polarssl_malloc( sizeof( buf ) - 2 * sizeof( memory_header ) - 16 );
    q = polarssl_malloc( 16 );

    TEST_ASSERT( check_pointer( p ) == 0 && check_pointer( q ) == 0 );
    TEST_ASSERT( polarssl_malloc( 1 ) == NULL );

    polarssl_free( q );

    TEST_ASSERT( polarssl_malloc( 17 ) == NULL );

    polarssl_free( p );

    TEST_ASSERT( check_all_free( ) == 0 );

    memory_buffer_alloc_free( );

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

cleanup:
    memory_buffer_alloc_free( );

    return( ret );
}
#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_MEMORY_BUFFER_ALLOC_C */
