/*
 * Performance Data Helper (pdh.dll)
 *
 * Copyright 2007 Andrey Turkin
 * Copyright 2007 Hans Leidekker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"

#include "pdh.h"
#include "pdhmsg.h"
#include "winperf.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(pdh);

static CRITICAL_SECTION pdh_handle_cs;
static CRITICAL_SECTION_DEBUG pdh_handle_cs_debug =
{
    0, 0, &pdh_handle_cs,
    { &pdh_handle_cs_debug.ProcessLocksList,
      &pdh_handle_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": pdh_handle_cs") }
};
static CRITICAL_SECTION pdh_handle_cs = { &pdh_handle_cs_debug, -1, 0, 0, 0, 0 };

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline void *heap_alloc_zero( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
}

static inline void heap_free( LPVOID mem )
{
    HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *pdh_strdup( const WCHAR *src )
{
    WCHAR *dst;

    if (!src) return NULL;
    if ((dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) ))) strcpyW( dst, src );
    return dst;
}

static inline WCHAR *pdh_strdup_aw( const char *src )
{
    int len;
    WCHAR *dst;

    if (!src) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if ((dst = heap_alloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    return dst;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    if (fdwReason == DLL_WINE_PREATTACH) return FALSE;    /* prefer native version */

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( hinstDLL );
    }

    return TRUE;
}

union value
{
    LONG     longvalue;
    double   doublevalue;
    LONGLONG largevalue;
};

struct counter
{
    DWORD           magic;                          /* signature */
    struct list     entry;                          /* list entry */
    WCHAR          *path;                           /* identifier */
    DWORD           type;                           /* counter type */
    DWORD           status;                         /* update status */
    LONG            scale;                          /* scale factor */
    LONG            defaultscale;                   /* default scale factor */
    DWORD_PTR       user;                           /* user data */
    DWORD_PTR       queryuser;                      /* query user data */
    LONGLONG        base;                           /* samples per second */
    FILETIME        stamp;                          /* time stamp */
    void (CALLBACK *collect)( struct counter * );   /* collect callback */
    union value     one;                            /* first value */
    union value     two;                            /* second value */
};

#define PDH_MAGIC_COUNTER   0x50444831 /* 'PDH1' */

static struct counter *create_counter( void )
{
    struct counter *counter;

    if ((counter = heap_alloc_zero( sizeof(struct counter) )))
    {
        counter->magic = PDH_MAGIC_COUNTER;
        return counter;
    }
    return NULL;
}

static void destroy_counter( struct counter *counter )
{
    counter->magic = 0;
    heap_free( counter->path );
    heap_free( counter );
}

#define PDH_MAGIC_QUERY     0x50444830 /* 'PDH0' */

struct query
{
    DWORD       magic;      /* signature */
    DWORD_PTR   user;       /* user data */
    HANDLE      thread;     /* collect thread */
    DWORD       interval;   /* collect interval */
    HANDLE      wait;       /* wait event */
    HANDLE      stop;       /* stop event */
    struct list counters;   /* counter list */
};

static struct query *create_query( void )
{
    struct query *query;

    if ((query = heap_alloc_zero( sizeof(struct query) )))
    {
        query->magic = PDH_MAGIC_QUERY;
        list_init( &query->counters );
        return query;
    }
    return NULL;
}

static void destroy_query( struct query *query )
{
    query->magic = 0;
    heap_free( query );
}

struct source
{
    DWORD           index;                          /* name index */
    const WCHAR    *path;                           /* identifier */
    void (CALLBACK *collect)( struct counter * );   /* collect callback */
    DWORD           type;                           /* counter type */
    LONG            scale;                          /* default scale factor */
    LONGLONG        base;                           /* samples per second */
};

static const WCHAR path_processor_time[] =
    {'\\','P','r','o','c','e','s','s','o','r','(','_','T','o','t','a','l',')',
     '\\','%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0};
static const WCHAR path_uptime[] =
    {'\\','S','y','s','t','e','m', '\\', 'S','y','s','t','e','m',' ','U','p',' ','T','i','m','e',0};

static void CALLBACK collect_processor_time( struct counter *counter )
{
    counter->two.largevalue = 500000; /* FIXME */
    counter->status = PDH_CSTATUS_VALID_DATA;
}

static void CALLBACK collect_uptime( struct counter *counter )
{
    counter->two.largevalue = GetTickCount64();
    counter->status = PDH_CSTATUS_VALID_DATA;
}

#define TYPE_PROCESSOR_TIME \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_DELTA_COUNTER | \
     PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

#define TYPE_UPTIME \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED | PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS)

/* counter source registry */
static const struct source counter_sources[] =
{
    { 6,    path_processor_time,    collect_processor_time,     TYPE_PROCESSOR_TIME,    -5,     10000000 },
    { 674,  path_uptime,            collect_uptime,             TYPE_UPTIME,            -3,     1000 }
};

static BOOL pdh_match_path( LPCWSTR fullpath, LPCWSTR path )
{
    const WCHAR *p;

    if (strchrW( path, '\\')) p = fullpath;
    else p = strrchrW( fullpath, '\\' ) + 1;
    if (strcmpW( p, path )) return FALSE;
    return TRUE;
}

/***********************************************************************
 *              PdhAddCounterA   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddCounterA( PDH_HQUERY query, LPCSTR path,
                                  DWORD_PTR userdata, PDH_HCOUNTER *counter )
{
    PDH_STATUS ret;
    WCHAR *pathW;

    TRACE("%p %s %lx %p\n", query, debugstr_a(path), userdata, counter);

    if (!path) return PDH_INVALID_ARGUMENT;

    if (!(pathW = pdh_strdup_aw( path )))
        return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhAddCounterW( query, pathW, userdata, counter );

    heap_free( pathW );
    return ret;
}

/***********************************************************************
 *              PdhAddCounterW   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddCounterW( PDH_HQUERY hquery, LPCWSTR path,
                                  DWORD_PTR userdata, PDH_HCOUNTER *hcounter )
{
    struct query *query = hquery;
    struct counter *counter;
    unsigned int i;

    TRACE("%p %s %lx %p\n", hquery, debugstr_w(path), userdata, hcounter);

    if (!path  || !hcounter) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!query || query->magic != PDH_MAGIC_QUERY)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    *hcounter = NULL;
    for (i = 0; i < sizeof(counter_sources) / sizeof(counter_sources[0]); i++)
    {
        if (pdh_match_path( counter_sources[i].path, path ))
        {
            if ((counter = create_counter()))
            {
                counter->path         = pdh_strdup( counter_sources[i].path );
                counter->collect      = counter_sources[i].collect;
                counter->type         = counter_sources[i].type;
                counter->defaultscale = counter_sources[i].scale;
                counter->base         = counter_sources[i].base;
                counter->queryuser    = query->user;
                counter->user         = userdata;

                list_add_tail( &query->counters, &counter->entry );
                *hcounter = counter;

                LeaveCriticalSection( &pdh_handle_cs );
                return ERROR_SUCCESS;
            }
            LeaveCriticalSection( &pdh_handle_cs );
            return PDH_MEMORY_ALLOCATION_FAILURE;
        }
    }
    LeaveCriticalSection( &pdh_handle_cs );
    return PDH_CSTATUS_NO_COUNTER;
}

/***********************************************************************
 *              PdhAddEnglishCounterA   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddEnglishCounterA( PDH_HQUERY query, LPCSTR path,
                                         DWORD_PTR userdata, PDH_HCOUNTER *counter )
{
    TRACE("%p %s %lx %p\n", query, debugstr_a(path), userdata, counter);

    if (!query) return PDH_INVALID_ARGUMENT;
    return PdhAddCounterA( query, path, userdata, counter );
}

/***********************************************************************
 *              PdhAddEnglishCounterW   (PDH.@)
 */
PDH_STATUS WINAPI PdhAddEnglishCounterW( PDH_HQUERY query, LPCWSTR path,
                                         DWORD_PTR userdata, PDH_HCOUNTER *counter )
{
    TRACE("%p %s %lx %p\n", query, debugstr_w(path), userdata, counter);

    if (!query) return PDH_INVALID_ARGUMENT;
    return PdhAddCounterW( query, path, userdata, counter );
}

/* caller must hold counter lock */
static PDH_STATUS format_value( struct counter *counter, DWORD format, union value *raw1,
                                union value *raw2, PDH_FMT_COUNTERVALUE *value )
{
    LONG factor;

    factor = counter->scale ? counter->scale : counter->defaultscale;
    if (format & PDH_FMT_LONG)
    {
        if (format & PDH_FMT_1000) value->u.longValue = raw2->longvalue * 1000;
        else value->u.longValue = raw2->longvalue * pow( 10, factor );
    }
    else if (format & PDH_FMT_LARGE)
    {
        if (format & PDH_FMT_1000) value->u.largeValue = raw2->largevalue * 1000;
        else value->u.largeValue = raw2->largevalue * pow( 10, factor );
    }
    else if (format & PDH_FMT_DOUBLE)
    {
        if (format & PDH_FMT_1000) value->u.doubleValue = raw2->doublevalue * 1000;
        else value->u.doubleValue = raw2->doublevalue * pow( 10, factor );
    }
    else
    {
        WARN("unknown format %x\n", format);
        return PDH_INVALID_ARGUMENT;
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhCalculateCounterFromRawValue   (PDH.@)
 */
PDH_STATUS WINAPI PdhCalculateCounterFromRawValue( PDH_HCOUNTER handle, DWORD format,
                                                   PPDH_RAW_COUNTER raw1, PPDH_RAW_COUNTER raw2,
                                                   PPDH_FMT_COUNTERVALUE value )
{
    PDH_STATUS ret;
    struct counter *counter = handle;

    TRACE("%p 0x%08x %p %p %p\n", handle, format, raw1, raw2, value);

    if (!value) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    ret = format_value( counter, format, (union value *)&raw1->SecondValue,
                                         (union value *)&raw2->SecondValue, value );

    LeaveCriticalSection( &pdh_handle_cs );
    return ret;
}


/***********************************************************************
 *              PdhCloseQuery   (PDH.@)
 */
PDH_STATUS WINAPI PdhCloseQuery( PDH_HQUERY handle )
{
    struct query *query = handle;
    struct list *item, *next;

    TRACE("%p\n", handle);

    EnterCriticalSection( &pdh_handle_cs );
    if (!query || query->magic != PDH_MAGIC_QUERY)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    if (query->thread)
    {
        HANDLE thread = query->thread;
        SetEvent( query->stop );
        LeaveCriticalSection( &pdh_handle_cs );

        WaitForSingleObject( thread, INFINITE );

        EnterCriticalSection( &pdh_handle_cs );
        if (query->magic != PDH_MAGIC_QUERY)
        {
            LeaveCriticalSection( &pdh_handle_cs );
            return ERROR_SUCCESS;
        }
        CloseHandle( query->stop );
        CloseHandle( query->thread );
        query->thread = NULL;
    }

    LIST_FOR_EACH_SAFE( item, next, &query->counters )
    {
        struct counter *counter = LIST_ENTRY( item, struct counter, entry );

        list_remove( &counter->entry );
        destroy_counter( counter );
    }

    destroy_query( query );

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/* caller must hold query lock */
static void collect_query_data( struct query *query )
{
    struct list *item;

    LIST_FOR_EACH( item, &query->counters )
    {
        SYSTEMTIME time;
        struct counter *counter = LIST_ENTRY( item, struct counter, entry );

        counter->collect( counter );

        GetLocalTime( &time );
        SystemTimeToFileTime( &time, &counter->stamp );
    }
}

/***********************************************************************
 *              PdhCollectQueryData   (PDH.@)
 */
PDH_STATUS WINAPI PdhCollectQueryData( PDH_HQUERY handle )
{
    struct query *query = handle;

    TRACE("%p\n", handle);

    EnterCriticalSection( &pdh_handle_cs );
    if (!query || query->magic != PDH_MAGIC_QUERY)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    if (list_empty( &query->counters ))
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_NO_DATA;
    }

    collect_query_data( query );

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

static DWORD CALLBACK collect_query_thread( void *arg )
{
    struct query *query = arg;
    DWORD interval = query->interval;
    HANDLE stop = query->stop;

    for (;;)
    {
        if (WaitForSingleObject( stop, interval ) != WAIT_TIMEOUT) ExitThread( 0 );

        EnterCriticalSection( &pdh_handle_cs );
        if (query->magic != PDH_MAGIC_QUERY)
        {
            LeaveCriticalSection( &pdh_handle_cs );
            ExitThread( PDH_INVALID_HANDLE );
        }

        collect_query_data( query );

        if (!SetEvent( query->wait ))
        {
            LeaveCriticalSection( &pdh_handle_cs );
            ExitThread( 0 );
        }
        LeaveCriticalSection( &pdh_handle_cs );
    }
}

/***********************************************************************
 *              PdhCollectQueryDataEx   (PDH.@)
 */
PDH_STATUS WINAPI PdhCollectQueryDataEx( PDH_HQUERY handle, DWORD interval, HANDLE event )
{
    PDH_STATUS ret;
    struct query *query = handle;

    TRACE("%p %d %p\n", handle, interval, event);

    EnterCriticalSection( &pdh_handle_cs );
    if (!query || query->magic != PDH_MAGIC_QUERY)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (list_empty( &query->counters ))
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_NO_DATA;
    }
    if (query->thread)
    {
        HANDLE thread = query->thread;
        SetEvent( query->stop );
        LeaveCriticalSection( &pdh_handle_cs );

        WaitForSingleObject( thread, INFINITE );

        EnterCriticalSection( &pdh_handle_cs );
        if (query->magic != PDH_MAGIC_QUERY)
        {
            LeaveCriticalSection( &pdh_handle_cs );
            return PDH_INVALID_HANDLE;
        }
        CloseHandle( query->thread );
        query->thread = NULL;
    }
    else if (!(query->stop = CreateEventW( NULL, FALSE, FALSE, NULL )))
    {
        ret = GetLastError();
        LeaveCriticalSection( &pdh_handle_cs );
        return ret;
    }
    query->wait = event;
    query->interval = interval * 1000;
    if (!(query->thread = CreateThread( NULL, 0, collect_query_thread, query, 0, NULL )))
    {
        ret = GetLastError();
        CloseHandle( query->stop );

        LeaveCriticalSection( &pdh_handle_cs );
        return ret;
    }

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhCollectQueryDataWithTime   (PDH.@)
 */
PDH_STATUS WINAPI PdhCollectQueryDataWithTime( PDH_HQUERY handle, LONGLONG *timestamp )
{
    struct query *query = handle;
    struct counter *counter;
    struct list *item;

    TRACE("%p %p\n", handle, timestamp);

    if (!timestamp) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!query || query->magic != PDH_MAGIC_QUERY)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (list_empty( &query->counters ))
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_NO_DATA;
    }

    collect_query_data( query );

    item = list_head( &query->counters );
    counter = LIST_ENTRY( item, struct counter, entry );

    *timestamp = ((LONGLONG)counter->stamp.dwHighDateTime << 32) | counter->stamp.dwLowDateTime;

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterInfoA   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterInfoA( PDH_HCOUNTER handle, BOOLEAN text, LPDWORD size, PPDH_COUNTER_INFO_A info )
{
    struct counter *counter = handle;

    TRACE("%p %d %p %p\n", handle, text, size, info);

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (!size)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_ARGUMENT;
    }
    if (*size < sizeof(PDH_COUNTER_INFO_A))
    {
        *size = sizeof(PDH_COUNTER_INFO_A);
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_MORE_DATA;
    }

    memset( info, 0, sizeof(PDH_COUNTER_INFO_A) );

    info->dwType          = counter->type;
    info->CStatus         = counter->status;
    info->lScale          = counter->scale;
    info->lDefaultScale   = counter->defaultscale;
    info->dwUserData      = counter->user;
    info->dwQueryUserData = counter->queryuser;

    *size = sizeof(PDH_COUNTER_INFO_A);

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterInfoW   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterInfoW( PDH_HCOUNTER handle, BOOLEAN text, LPDWORD size, PPDH_COUNTER_INFO_W info )
{
    struct counter *counter = handle;

    TRACE("%p %d %p %p\n", handle, text, size, info);

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (!size)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_ARGUMENT;
    }
    if (*size < sizeof(PDH_COUNTER_INFO_W))
    {
        *size = sizeof(PDH_COUNTER_INFO_W);
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_MORE_DATA;
    }

    memset( info, 0, sizeof(PDH_COUNTER_INFO_W) );

    info->dwType          = counter->type;
    info->CStatus         = counter->status;
    info->lScale          = counter->scale;
    info->lDefaultScale   = counter->defaultscale;
    info->dwUserData      = counter->user;
    info->dwQueryUserData = counter->queryuser;

    *size = sizeof(PDH_COUNTER_INFO_W);

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetCounterTimeBase   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetCounterTimeBase( PDH_HCOUNTER handle, LONGLONG *base )
{
    struct counter *counter = handle;

    TRACE("%p %p\n", handle, base);

    if (!base) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    *base = counter->base;

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhGetFormattedCounterValue   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetFormattedCounterValue( PDH_HCOUNTER handle, DWORD format,
                                               LPDWORD type, PPDH_FMT_COUNTERVALUE value )
{
    PDH_STATUS ret;
    struct counter *counter = handle;

    TRACE("%p %x %p %p\n", handle, format, type, value);

    if (!value) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (counter->status)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_DATA;
    }
    if (!(ret = format_value( counter, format, &counter->one, &counter->two, value )))
    {
        value->CStatus = ERROR_SUCCESS;
        if (type) *type = counter->type;
    }

    LeaveCriticalSection( &pdh_handle_cs );
    return ret;
}

/***********************************************************************
 *              PdhGetRawCounterValue   (PDH.@)
 */
PDH_STATUS WINAPI PdhGetRawCounterValue( PDH_HCOUNTER handle, LPDWORD type,
                                         PPDH_RAW_COUNTER value )
{
    struct counter *counter = handle;

    TRACE("%p %p %p\n", handle, type, value);

    if (!value) return PDH_INVALID_ARGUMENT;

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    value->CStatus                  = counter->status;
    value->TimeStamp.dwLowDateTime  = counter->stamp.dwLowDateTime;
    value->TimeStamp.dwHighDateTime = counter->stamp.dwHighDateTime;
    value->FirstValue               = counter->one.largevalue;
    value->SecondValue              = counter->two.largevalue;
    value->MultiCount               = 1; /* FIXME */

    if (type) *type = counter->type;

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhLookupPerfIndexByNameA   (PDH.@)
 */
PDH_STATUS WINAPI PdhLookupPerfIndexByNameA( LPCSTR machine, LPCSTR name, LPDWORD index )
{
    PDH_STATUS ret;
    WCHAR *machineW = NULL;
    WCHAR *nameW;

    TRACE("%s %s %p\n", debugstr_a(machine), debugstr_a(name), index);

    if (!name) return PDH_INVALID_ARGUMENT;

    if (machine && !(machineW = pdh_strdup_aw( machine ))) return PDH_MEMORY_ALLOCATION_FAILURE;

    if (!(nameW = pdh_strdup_aw( name )))
        return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhLookupPerfIndexByNameW( machineW, nameW, index );

    heap_free( nameW );
    heap_free( machineW );
    return ret;
}

/***********************************************************************
 *              PdhLookupPerfIndexByNameW   (PDH.@)
 */
PDH_STATUS WINAPI PdhLookupPerfIndexByNameW( LPCWSTR machine, LPCWSTR name, LPDWORD index )
{
    unsigned int i;

    TRACE("%s %s %p\n", debugstr_w(machine), debugstr_w(name), index);

    if (!name || !index) return PDH_INVALID_ARGUMENT;

    if (machine)
    {
        FIXME("remote machine not supported\n");
        return PDH_CSTATUS_NO_MACHINE;
    }
    for (i = 0; i < sizeof(counter_sources) / sizeof(counter_sources[0]); i++)
    {
        if (pdh_match_path( counter_sources[i].path, name ))
        {
            *index = counter_sources[i].index;
            return ERROR_SUCCESS;
        }
    }
    return PDH_STRING_NOT_FOUND;
}

/***********************************************************************
 *              PdhLookupPerfNameByIndexA   (PDH.@)
 */
PDH_STATUS WINAPI PdhLookupPerfNameByIndexA( LPCSTR machine, DWORD index, LPSTR buffer, LPDWORD size )
{
    PDH_STATUS ret;
    WCHAR *machineW = NULL;
    WCHAR bufferW[PDH_MAX_COUNTER_NAME];
    DWORD sizeW = sizeof(bufferW) / sizeof(WCHAR);

    TRACE("%s %d %p %p\n", debugstr_a(machine), index, buffer, size);

    if (!buffer || !size) return PDH_INVALID_ARGUMENT;

    if (machine && !(machineW = pdh_strdup_aw( machine ))) return PDH_MEMORY_ALLOCATION_FAILURE;

    if (!(ret = PdhLookupPerfNameByIndexW( machineW, index, bufferW, &sizeW )))
    {
        int required = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );

        if (size && *size < required) ret = PDH_MORE_DATA;
        else WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, required, NULL, NULL );
        if (size) *size = required;
    }
    heap_free( machineW );
    return ret;
}

/***********************************************************************
 *              PdhLookupPerfNameByIndexW   (PDH.@)
 */
PDH_STATUS WINAPI PdhLookupPerfNameByIndexW( LPCWSTR machine, DWORD index, LPWSTR buffer, LPDWORD size )
{
    PDH_STATUS ret;
    unsigned int i;

    TRACE("%s %d %p %p\n", debugstr_w(machine), index, buffer, size);

    if (machine)
    {
        FIXME("remote machine not supported\n");
        return PDH_CSTATUS_NO_MACHINE;
    }

    if (!buffer || !size) return PDH_INVALID_ARGUMENT;
    if (!index) return ERROR_SUCCESS;

    for (i = 0; i < sizeof(counter_sources) / sizeof(counter_sources[0]); i++)
    {
        if (counter_sources[i].index == index)
        {
            WCHAR *p = strrchrW( counter_sources[i].path, '\\' ) + 1;
            unsigned int required = strlenW( p ) + 1;

            if (*size < required) ret = PDH_MORE_DATA;
            else
            {
                strcpyW( buffer, p );
                ret = ERROR_SUCCESS;
            }
            *size = required;
            return ret;
        }
    }
    return PDH_INVALID_ARGUMENT;
}

/***********************************************************************
 *              PdhOpenQueryA   (PDH.@)
 */
PDH_STATUS WINAPI PdhOpenQueryA( LPCSTR source, DWORD_PTR userdata, PDH_HQUERY *query )
{
    PDH_STATUS ret;
    WCHAR *sourceW = NULL;

    TRACE("%s %lx %p\n", debugstr_a(source), userdata, query);

    if (source && !(sourceW = pdh_strdup_aw( source ))) return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhOpenQueryW( sourceW, userdata, query );
    heap_free( sourceW );

    return ret;
}

/***********************************************************************
 *              PdhOpenQueryW   (PDH.@)
 */
PDH_STATUS WINAPI PdhOpenQueryW( LPCWSTR source, DWORD_PTR userdata, PDH_HQUERY *handle )
{
    struct query *query;

    TRACE("%s %lx %p\n", debugstr_w(source), userdata, handle);

    if (!handle) return PDH_INVALID_ARGUMENT;

    if (source)
    {
        FIXME("log file data source not supported\n");
        return PDH_INVALID_ARGUMENT;
    }
    if ((query = create_query()))
    {
        query->user = userdata;
        *handle = query;

        return ERROR_SUCCESS;
    }
    return PDH_MEMORY_ALLOCATION_FAILURE;
}

/***********************************************************************
 *              PdhRemoveCounter   (PDH.@)
 */
PDH_STATUS WINAPI PdhRemoveCounter( PDH_HCOUNTER handle )
{
    struct counter *counter = handle;

    TRACE("%p\n", handle);

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }

    list_remove( &counter->entry );
    destroy_counter( counter );

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhSetCounterScaleFactor   (PDH.@)
 */
PDH_STATUS WINAPI PdhSetCounterScaleFactor( PDH_HCOUNTER handle, LONG factor )
{
    struct counter *counter = handle;

    TRACE("%p\n", handle);

    EnterCriticalSection( &pdh_handle_cs );
    if (!counter || counter->magic != PDH_MAGIC_COUNTER)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_HANDLE;
    }
    if (factor < PDH_MIN_SCALE || factor > PDH_MAX_SCALE)
    {
        LeaveCriticalSection( &pdh_handle_cs );
        return PDH_INVALID_ARGUMENT;
    }

    counter->scale = factor;

    LeaveCriticalSection( &pdh_handle_cs );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *              PdhValidatePathA   (PDH.@)
 */
PDH_STATUS WINAPI PdhValidatePathA( LPCSTR path )
{
    PDH_STATUS ret;
    WCHAR *pathW;

    TRACE("%s\n", debugstr_a(path));

    if (!path) return PDH_INVALID_ARGUMENT;
    if (!(pathW = pdh_strdup_aw( path ))) return PDH_MEMORY_ALLOCATION_FAILURE;

    ret = PdhValidatePathW( pathW );

    heap_free( pathW );
    return ret;
}

static PDH_STATUS validate_path( LPCWSTR path )
{
    if (!path || !*path) return PDH_INVALID_ARGUMENT;
    if (*path++ != '\\' || !strchrW( path, '\\' )) return PDH_CSTATUS_BAD_COUNTERNAME;
    return ERROR_SUCCESS;
 }

/***********************************************************************
 *              PdhValidatePathW   (PDH.@)
 */
PDH_STATUS WINAPI PdhValidatePathW( LPCWSTR path )
{
    PDH_STATUS ret;
    unsigned int i;

    TRACE("%s\n", debugstr_w(path));

    if ((ret = validate_path( path ))) return ret;

    for (i = 0; i < sizeof(counter_sources) / sizeof(counter_sources[0]); i++)
        if (pdh_match_path( counter_sources[i].path, path )) return ERROR_SUCCESS;

    return PDH_CSTATUS_NO_COUNTER;
}

/***********************************************************************
 *              PdhValidatePathExA   (PDH.@)
 */
PDH_STATUS WINAPI PdhValidatePathExA( PDH_HLOG source, LPCSTR path )
{
    TRACE("%p %s\n", source, debugstr_a(path));

    if (source)
    {
        FIXME("log file data source not supported\n");
        return ERROR_SUCCESS;
    }
    return PdhValidatePathA( path );
}

/***********************************************************************
 *              PdhValidatePathExW   (PDH.@)
 */
PDH_STATUS WINAPI PdhValidatePathExW( PDH_HLOG source, LPCWSTR path )
{
    TRACE("%p %s\n", source, debugstr_w(path));

    if (source)
    {
        FIXME("log file data source not supported\n");
        return ERROR_SUCCESS;
    }
    return PdhValidatePathW( path );
}

/***********************************************************************
 *              PdhEnumObjectItemsA   (PDH.@)
 */
PDH_STATUS WINAPI PdhEnumObjectItemsA(LPCSTR szDataSource, LPCSTR szMachineName, LPCSTR szObjectName,
                                      LPSTR mszCounterList, LPDWORD pcchCounterListLength, LPSTR mszInstanceList,
                                      LPDWORD pcchInstanceListLength, DWORD dwDetailLevel, DWORD dwFlags)
{
    FIXME("%s, %s, %s, %p, %p, %p, %p, %d, 0x%x: stub\n", debugstr_a(szDataSource), debugstr_a(szMachineName),
         debugstr_a(szObjectName), mszCounterList, pcchCounterListLength, mszInstanceList,
         pcchInstanceListLength, dwDetailLevel, dwFlags);

    return PDH_NOT_IMPLEMENTED;
}

/***********************************************************************
 *              PdhEnumObjectItemsW   (PDH.@)
 */
PDH_STATUS WINAPI PdhEnumObjectItemsW(LPCWSTR szDataSource, LPCWSTR szMachineName, LPCWSTR szObjectName,
                                      LPWSTR mszCounterList, LPDWORD pcchCounterListLength, LPWSTR mszInstanceList,
                                      LPDWORD pcchInstanceListLength, DWORD dwDetailLevel, DWORD dwFlags)
{
    FIXME("%s, %s, %s, %p, %p, %p, %p, %d, 0x%x: stub\n", debugstr_w(szDataSource), debugstr_w(szMachineName),
         debugstr_w(szObjectName), mszCounterList, pcchCounterListLength, mszInstanceList,
         pcchInstanceListLength, dwDetailLevel, dwFlags);

    return PDH_NOT_IMPLEMENTED;
}
