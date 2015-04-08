/*
 *  Portable interface to the CPU cycle counter
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

#if defined(POLARSSL_SELF_TEST) && defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdio.h>
#define polarssl_printf     printf
#endif

#if defined(POLARSSL_TIMING_C) && !defined(POLARSSL_TIMING_ALT)

#include "polarssl/timing.h"

#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)

#include <windows.h>
#include <winbase.h>

struct _hr_time
{
    LARGE_INTEGER start;
};

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

struct _hr_time
{
    struct timeval start;
};

#endif /* _WIN32 && !EFIX64 && !EFI32 */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    ( defined(_MSC_VER) && defined(_M_IX86) ) || defined(__WATCOMC__)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long tsc;
    __asm   rdtsc
    __asm   mov  [tsc], eax
    return( tsc );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          ( _MSC_VER && _M_IX86 ) || __WATCOMC__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    defined(__GNUC__) && defined(__i386__)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long lo, hi;
    asm volatile( "rdtsc" : "=a" (lo), "=d" (hi) );
    return( lo );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && __i386__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    defined(__GNUC__) && ( defined(__amd64__) || defined(__x86_64__) )

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long lo, hi;
    asm volatile( "rdtsc" : "=a" (lo), "=d" (hi) );
    return( lo | ( hi << 32 ) );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && ( __amd64__ || __x86_64__ ) */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    defined(__GNUC__) && ( defined(__powerpc__) || defined(__ppc__) )

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long tbl, tbu0, tbu1;

    do
    {
        asm volatile( "mftbu %0" : "=r" (tbu0) );
        asm volatile( "mftb  %0" : "=r" (tbl ) );
        asm volatile( "mftbu %0" : "=r" (tbu1) );
    }
    while( tbu0 != tbu1 );

    return( tbl );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && ( __powerpc__ || __ppc__ ) */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    defined(__GNUC__) && defined(__sparc64__)

#if defined(__OpenBSD__)
#warning OpenBSD does not allow access to tick register using software version instead
#else
#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long tick;
    asm volatile( "rdpr %%tick, %0;" : "=&r" (tick) );
    return( tick );
}
#endif /* __OpenBSD__ */
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && __sparc64__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&  \
    defined(__GNUC__) && defined(__sparc__) && !defined(__sparc64__)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long tick;
    asm volatile( ".byte 0x83, 0x41, 0x00, 0x00" );
    asm volatile( "mov   %%g1, %0" : "=r" (tick) );
    return( tick );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && __sparc__ && !__sparc64__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&      \
    defined(__GNUC__) && defined(__alpha__)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long cc;
    asm volatile( "rpcc %0" : "=r" (cc) );
    return( cc & 0xFFFFFFFF );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && __alpha__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(POLARSSL_HAVE_ASM) &&      \
    defined(__GNUC__) && defined(__ia64__)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    unsigned long itc;
    asm volatile( "mov %0 = ar.itc" : "=r" (itc) );
    return( itc );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && POLARSSL_HAVE_ASM &&
          __GNUC__ && __ia64__ */

#if !defined(POLARSSL_HAVE_HARDCLOCK) && defined(_MSC_VER) && \
    !defined(EFIX64) && !defined(EFI32)

#define POLARSSL_HAVE_HARDCLOCK

unsigned long hardclock( void )
{
    LARGE_INTEGER offset;

    QueryPerformanceCounter( &offset );

    return( (unsigned long)( offset.QuadPart ) );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK && _MSC_VER && !EFIX64 && !EFI32 */

#if !defined(POLARSSL_HAVE_HARDCLOCK)

#define POLARSSL_HAVE_HARDCLOCK

static int hardclock_init = 0;
static struct timeval tv_init;

unsigned long hardclock( void )
{
    struct timeval tv_cur;

    if( hardclock_init == 0 )
    {
        gettimeofday( &tv_init, NULL );
        hardclock_init = 1;
    }

    gettimeofday( &tv_cur, NULL );
    return( ( tv_cur.tv_sec  - tv_init.tv_sec  ) * 1000000
          + ( tv_cur.tv_usec - tv_init.tv_usec ) );
}
#endif /* !POLARSSL_HAVE_HARDCLOCK */

volatile int alarmed = 0;

#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)

unsigned long get_timer( struct hr_time *val, int reset )
{
    unsigned long delta;
    LARGE_INTEGER offset, hfreq;
    struct _hr_time *t = (struct _hr_time *) val;

    QueryPerformanceCounter(  &offset );
    QueryPerformanceFrequency( &hfreq );

    delta = (unsigned long)( ( 1000 *
        ( offset.QuadPart - t->start.QuadPart ) ) /
           hfreq.QuadPart );

    if( reset )
        QueryPerformanceCounter( &t->start );

    return( delta );
}

DWORD WINAPI TimerProc( LPVOID uElapse )
{
    Sleep( (DWORD) uElapse );
    alarmed = 1;
    return( TRUE );
}

void set_alarm( int seconds )
{
    DWORD ThreadId;

    alarmed = 0;
    CloseHandle( CreateThread( NULL, 0, TimerProc,
        (LPVOID) ( seconds * 1000 ), 0, &ThreadId ) );
}

void m_sleep( int milliseconds )
{
    Sleep( milliseconds );
}

#else /* _WIN32 && !EFIX64 && !EFI32 */

unsigned long get_timer( struct hr_time *val, int reset )
{
    unsigned long delta;
    struct timeval offset;
    struct _hr_time *t = (struct _hr_time *) val;

    gettimeofday( &offset, NULL );

    if( reset )
    {
        t->start.tv_sec  = offset.tv_sec;
        t->start.tv_usec = offset.tv_usec;
        return( 0 );
    }

    delta = ( offset.tv_sec  - t->start.tv_sec  ) * 1000
          + ( offset.tv_usec - t->start.tv_usec ) / 1000;

    return( delta );
}

#if defined(INTEGRITY)
void m_sleep( int milliseconds )
{
    usleep( milliseconds * 1000 );
}

#else /* INTEGRITY */

static void sighandler( int signum )
{
    alarmed = 1;
    signal( signum, sighandler );
}

void set_alarm( int seconds )
{
    alarmed = 0;
    signal( SIGALRM, sighandler );
    alarm( seconds );
}

void m_sleep( int milliseconds )
{
    struct timeval tv;

    tv.tv_sec  = milliseconds / 1000;
    tv.tv_usec = ( milliseconds % 1000 ) * 1000;

    select( 0, NULL, NULL, NULL, &tv );
}
#endif /* INTEGRITY */

#endif /* _WIN32 && !EFIX64 && !EFI32 */

#if defined(POLARSSL_SELF_TEST)

/* To test net_usleep against our functions */
#if defined(POLARSSL_NET_C) && defined(POLARSSL_HAVE_TIME)
#include "polarssl/net.h"
#endif

/*
 * Busy-waits for the given number of milliseconds.
 * Used for testing hardclock.
 */
static void busy_msleep( unsigned long msec )
{
    struct hr_time hires;
    unsigned long i = 0; /* for busy-waiting */
    volatile unsigned long j; /* to prevent optimisation */

    (void) get_timer( &hires, 1 );

    while( get_timer( &hires, 0 ) < msec )
        i++;

    j = i;
    (void) j;
}

/*
 * Checkup routine
 *
 * Warning: this is work in progress, some tests may not be reliable enough
 * yet! False positives may happen.
 */
int timing_self_test( int verbose )
{
    unsigned long cycles, ratio;
    unsigned long millisecs, secs;
    int hardfail;
    struct hr_time hires;

    if( verbose != 0 )
        polarssl_printf( "  TIMING tests note: will take some time!\n" );

    if( verbose != 0 )
        polarssl_printf( "  TIMING test #1 (m_sleep   / get_timer): " );

    for( secs = 1; secs <= 3; secs++ )
    {
        (void) get_timer( &hires, 1 );

        m_sleep( (int)( 500 * secs ) );

        millisecs = get_timer( &hires, 0 );

        if( millisecs < 450 * secs || millisecs > 550 * secs )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            return( 1 );
        }
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

    if( verbose != 0 )
        polarssl_printf( "  TIMING test #2 (set_alarm / get_timer): " );

    for( secs = 1; secs <= 3; secs++ )
    {
        (void) get_timer( &hires, 1 );

        set_alarm( (int) secs );
        while( !alarmed )
            ;

        millisecs = get_timer( &hires, 0 );

        if( millisecs < 900 * secs || millisecs > 1100 * secs )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            return( 1 );
        }
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

    if( verbose != 0 )
        polarssl_printf( "  TIMING test #3 (hardclock / get_timer): " );

    /*
     * Allow one failure for possible counter wrapping.
     * On a 4Ghz 32-bit machine the cycle counter wraps about once per second;
     * since the whole test is about 10ms, it shouldn't happen twice in a row.
     */
    hardfail = 0;

hard_test:
    if( hardfail > 1 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    /* Get a reference ratio cycles/ms */
    millisecs = 1;
    cycles = hardclock();
    busy_msleep( millisecs );
    cycles = hardclock() - cycles;
    ratio = cycles / millisecs;

    /* Check that the ratio is mostly constant */
    for( millisecs = 2; millisecs <= 4; millisecs++ )
    {
        cycles = hardclock();
        busy_msleep( millisecs );
        cycles = hardclock() - cycles;

        /* Allow variation up to 20% */
        if( cycles / millisecs < ratio - ratio / 5 ||
            cycles / millisecs > ratio + ratio / 5 )
        {
            hardfail++;
            goto hard_test;
        }
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n" );

#if defined(POLARSSL_NET_C) && defined(POLARSSL_HAVE_TIME)
    if( verbose != 0 )
        polarssl_printf( "  TIMING test #4 (net_usleep/ get_timer): " );

    for( secs = 1; secs <= 3; secs++ )
    {
        (void) get_timer( &hires, 1 );

        net_usleep( 500000 * secs );

        millisecs = get_timer( &hires, 0 );

        if( millisecs < 450 * secs || millisecs > 550 * secs )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            return( 1 );
        }
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n" );
#endif /* POLARSSL_NET_C */

    if( verbose != 0 )
        polarssl_printf( "\n" );

    return( 0 );
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_TIMING_C && !POLARSSL_TIMING_ALT */
