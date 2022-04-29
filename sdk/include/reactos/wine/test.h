/*
 * Definitions for Wine C unit tests.
 *
 * Copyright (C) 2002 Alexandre Julliard
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

#ifndef __WINE_WINE_TEST_H
#define __WINE_WINE_TEST_H

#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>

#ifdef __WINE_CONFIG_H
#error config.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_LIBRARY_H
#error wine/library.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_UNICODE_H
#error wine/unicode.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_DEBUG_H
#error wine/debug.h should not be used in Wine tests
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES  (~0u)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (~0u)
#endif

/* debug level */
extern int winetest_debug;

extern int report_success;

/* running in interactive mode? */
extern int winetest_interactive;

/* current platform */
extern const char *winetest_platform;

extern void winetest_set_location( const char* file, int line );
extern void winetest_subtest(const char* name);
extern void winetest_start_todo( int is_todo );
extern int winetest_loop_todo(void);
extern void winetest_end_todo(void);
extern void winetest_start_nocount(unsigned int flags);
extern int winetest_loop_nocount(void);
extern void winetest_end_nocount(void);
extern int winetest_get_mainargs( char*** pargv );
extern LONG winetest_get_failures(void);
extern LONG winetest_get_successes(void);
extern void winetest_add_failures( LONG new_failures );
extern void winetest_wait_child_process( HANDLE process );

extern const char *wine_dbgstr_wn( const WCHAR *str, intptr_t n );
extern const char *wine_dbgstr_an( const CHAR *str, intptr_t n );
extern const char *wine_dbgstr_guid( const GUID *guid );
extern const char *wine_dbgstr_point( const POINT *guid );
extern const char *wine_dbgstr_size( const SIZE *guid );
extern const char *wine_dbgstr_rect( const RECT *rect );
#ifdef WINETEST_USE_DBGSTR_LONGLONG
extern const char *wine_dbgstr_longlong( ULONGLONG ll );
#endif
static inline const char *debugstr_a( const char *s )  { return wine_dbgstr_an( s, -1 ); }
static inline const char *debugstr_an( const CHAR *s, intptr_t n ) { return wine_dbgstr_an( s, n ); }
static inline const char *wine_dbgstr_a( const char *s )  { return wine_dbgstr_an( s, -1 ); }
static inline const char *wine_dbgstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }

/* strcmpW is available for tests compiled under Wine, but not in standalone
 * builds under Windows, so we reimplement it under a different name. */
static inline int winetest_strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

#ifdef STANDALONE

#define START_TEST(name) \
  static void func_##name(void); \
  const struct test winetest_testlist[] = { { #name, func_##name }, { 0, 0 } }; \
  static void func_##name(void)

#else /* STANDALONE */

#ifdef __cplusplus
#define START_TEST(name) extern "C" void func_##name(void)
#else
#define START_TEST(name) void func_##name(void)
#endif

#endif /* STANDALONE */

#if defined(__x86_64__) && defined(__GNUC__) && defined(__WINE_USE_MSVCRT)
#define __winetest_cdecl __cdecl
#define __winetest_va_list __builtin_ms_va_list
#else
#define __winetest_cdecl
#define __winetest_va_list va_list
#endif

extern int broken( int condition );
extern int winetest_vok( int condition, const char *msg, __winetest_va_list ap );
extern void winetest_vskip( const char *msg, __winetest_va_list ap );

#ifdef __GNUC__
# define WINETEST_PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
extern void __winetest_cdecl winetest_ok( int condition, const char *msg, ... ) __attribute__((format (printf,2,3) ));
extern void __winetest_cdecl winetest_skip( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void __winetest_cdecl winetest_win_skip( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void __winetest_cdecl winetest_trace( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void __winetest_cdecl winetest_print(const char* msg, ...) __attribute__((format(printf, 1, 2)));
extern void __winetest_cdecl winetest_push_context( const char *fmt, ... ) __attribute__((format(printf, 1, 2)));
extern void winetest_pop_context(void);

#else /* __GNUC__ */
# define WINETEST_PRINTF_ATTR(fmt,args)
extern void __winetest_cdecl winetest_ok( int condition, const char *msg, ... );
extern void __winetest_cdecl winetest_skip( const char *msg, ... );
extern void __winetest_cdecl winetest_win_skip( const char *msg, ... );
extern void __winetest_cdecl winetest_trace( const char *msg, ... );
extern void __winetest_cdecl winetest_print(const char* msg, ...);
extern void __winetest_cdecl winetest_push_context( const char *fmt, ... );
extern void winetest_pop_context(void);

#endif /* __GNUC__ */

#define subtest_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_subtest
#define ok_(file, line)       (winetest_set_location(file, line), 0) ? (void)0 : winetest_ok
#define skip_(file, line)     (winetest_set_location(file, line), 0) ? (void)0 : winetest_skip
#define win_skip_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : winetest_win_skip
#define trace_(file, line)    (winetest_set_location(file, line), 0) ? (void)0 : winetest_trace

#define subtest  subtest_(__FILE__, __LINE__)
#define ok       ok_(__FILE__, __LINE__)
#define skip     skip_(__FILE__, __LINE__)
#define win_skip win_skip_(__FILE__, __LINE__)
#define trace    trace_(__FILE__, __LINE__)

#define todo_if(is_todo) for (winetest_start_todo(is_todo); \
                              winetest_loop_todo(); \
                              winetest_end_todo())

#define todo_ros                todo_if(!strcmp(winetest_platform, "reactos"))
#define todo_ros_if(is_todo)    todo_if((is_todo) && !strcmp(winetest_platform, "reactos"))
#ifdef USE_WINE_TODOS
#define todo_wine               todo_ros
#define todo_wine_if            todo_ros_if
#else
#define todo_wine               todo_if(!strcmp(winetest_platform, "wine"))
#define todo_wine_if(is_todo)   todo_if((is_todo) && !strcmp(winetest_platform, "wine"))
#endif

#define ros_skip_flaky          for (winetest_start_nocount(3); \
                                     winetest_loop_nocount(); \
                                     winetest_end_nocount())

#define disable_success_count   for (winetest_start_nocount(1); \
                                     winetest_loop_nocount(); \
                                     winetest_end_nocount())

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifdef NONAMELESSUNION
# define U(x)  (x).u
# define U1(x) (x).u1
# define U2(x) (x).u2
# define U3(x) (x).u3
# define U4(x) (x).u4
# define U5(x) (x).u5
# define U6(x) (x).u6
# define U7(x) (x).u7
# define U8(x) (x).u8
#else
# define U(x)  (x)
# define U1(x) (x)
# define U2(x) (x)
# define U3(x) (x)
# define U4(x) (x)
# define U5(x) (x)
# define U6(x) (x)
# define U7(x) (x)
# define U8(x) (x)
#endif

#ifdef NONAMELESSSTRUCT
# define S(x)  (x).s
# define S1(x) (x).s1
# define S2(x) (x).s2
# define S3(x) (x).s3
# define S4(x) (x).s4
# define S5(x) (x).s5
#else
# define S(x)  (x)
# define S1(x) (x)
# define S2(x) (x)
# define S3(x) (x)
# define S4(x) (x)
# define S5(x) (x)
#endif


/************************************************************************/
/* Below is the implementation of the various functions, to be included
 * directly into the generated testlist.c file.
 * It is done that way so that the dlls can build the test routines with
 * different includes or flags if needed.
 */

#ifdef STANDALONE

#include <stdio.h>

#if defined(__x86_64__) && defined(__GNUC__) && defined(__WINE_USE_MSVCRT)
# define __winetest_va_start(list,arg) __builtin_ms_va_start(list,arg)
# define __winetest_va_end(list) __builtin_ms_va_end(list)
#else
# define __winetest_va_start(list,arg) va_start(list,arg)
# define __winetest_va_end(list) va_end(list)
#endif

/* Define WINETEST_MSVC_IDE_FORMATTING to alter the output format winetest will use for file/line numbers.
   This alternate format makes the file/line numbers clickable in visual studio, to directly jump to them. */
#if defined(WINETEST_MSVC_IDE_FORMATTING)
# define __winetest_file_line_prefix "%s(%d)"
#else
# define __winetest_file_line_prefix "%s:%d"
#endif

struct test
{
    const char *name;
    void (*func)(void);
};

extern const struct test winetest_testlist[];

/* debug level */
int winetest_debug = 1;

/* interactive mode? */
int winetest_interactive = 0;

/* current platform */
const char *winetest_platform = "windows";

/* report successful tests (BOOL) */
int report_success = 0;

/* passing arguments around */
static int winetest_argc;
static char** winetest_argv;

static const struct test *current_test; /* test currently being run */

static LONG successes;       /* number of successful tests */
static LONG failures;        /* number of failures */
static LONG skipped;         /* number of skipped test chunks */
static LONG todo_successes;  /* number of successful tests inside todo block */
static LONG todo_failures;   /* number of failures inside todo block */

/* The following data must be kept track of on a per-thread basis */
typedef struct
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    unsigned int todo_level;         /* current todo nesting level */
    unsigned int nocount_level;
    int todo_do_loop;
    char *str_pos;                   /* position in debug buffer */
    char strings[2000];              /* buffer for debug strings */
    char context[8][128];            /* data to print before messages */
    unsigned int context_count;      /* number of context prefixes */
} tls_data;
static DWORD tls_index;

static tls_data* get_tls_data(void)
{
    tls_data* data;
    DWORD last_error;

    last_error=GetLastError();
    data=(tls_data*)TlsGetValue(tls_index);
    if (!data)
    {
        data=(tls_data*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(tls_data));
        data->str_pos = data->strings;
        TlsSetValue(tls_index,data);
    }
    SetLastError(last_error);
    return data;
}

/* allocate some tmp space for a string */
static char *get_temp_buffer( size_t n )
{
    tls_data *data = get_tls_data();
    char *res = data->str_pos;

    if (res + n >= &data->strings[sizeof(data->strings)]) res = data->strings;
    data->str_pos = res + n;
    return res;
}

/* release extra space that we requested in gimme1() */
static void release_temp_buffer( char *ptr, size_t size )
{
    tls_data *data = get_tls_data();
    data->str_pos = ptr + size;
}

static void exit_process( int code )
{
    fflush( stdout );
    ExitProcess( code );
}


void winetest_set_location( const char* file, int line )
{
    tls_data* data=get_tls_data();
#if defined(WINETEST_MSVC_IDE_FORMATTING)
    data->current_file = file;
#else
    data->current_file=strrchr(file,'/');
    if (data->current_file==NULL)
        data->current_file=strrchr(file,'\\');
    if (data->current_file==NULL)
        data->current_file=file;
    else
        data->current_file++;
#endif
    data->current_line=line;
}

#ifdef __GNUC__
static void __winetest_cdecl winetest_printf( const char *msg, ... ) __attribute__((format(printf,1,2)));
#else
static void __winetest_cdecl winetest_printf(const char* msg, ...);
#endif
static void __winetest_cdecl winetest_printf( const char *msg, ... )
{
    tls_data *data = get_tls_data();
    __winetest_va_list valist;

    fprintf( stdout, __winetest_file_line_prefix ": ", data->current_file, data->current_line );
    __winetest_va_start( valist, msg );
    vfprintf( stdout, msg, valist );
    __winetest_va_end( valist );
}

static void __winetest_cdecl winetest_print_context( const char *msgtype )
{
    tls_data *data = get_tls_data();
    unsigned int i;

    winetest_printf( "%s", msgtype );
    for (i = 0; i < data->context_count; ++i)
        fprintf( stdout, "%s: ", data->context[i] );
}

void winetest_subtest(const char* name)
{
    tls_data* data = get_tls_data();
    fprintf(stdout, __winetest_file_line_prefix ": Subtest %s\n",
        data->current_file, data->current_line, name);
}

int broken( int condition )
{
    return ((strcmp(winetest_platform, "windows") == 0)
#ifndef USE_WINE_TODOS
    || (strcmp(winetest_platform, "reactos") == 0)
#endif
    ) && condition;
}

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 * Return:
 *   0 if condition does not have the expected value, 1 otherwise
 */
int winetest_vok( int condition, const char *msg, __winetest_va_list args )
{
    tls_data* data=get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            winetest_print_context( "Test succeeded inside todo block: " );
            vfprintf(stdout, msg, args);
            if ((data->nocount_level & 2) == 0)
            InterlockedIncrement(&todo_failures);
            return 0;
        }
        else
        {
            /* show todos even if traces are disabled*/
            /*if (winetest_debug > 0)*/
            {
                winetest_print_context( "Test marked todo: " );
                vfprintf(stdout, msg, args);
            }
            if ((data->nocount_level & 1) == 0)
            InterlockedIncrement(&todo_successes);
            return 1;
        }
    }
    else
    {
        if (!condition)
        {
            winetest_print_context( "Test failed: " );
            vfprintf(stdout, msg, args);
            if ((data->nocount_level & 2) == 0)
            InterlockedIncrement(&failures);
            return 0;
        }
        else
        {
            if (report_success && (data->nocount_level & 1) == 0)
            {
                winetest_printf("Test succeeded\n");
            }
            if ((data->nocount_level & 1) == 0)
            InterlockedIncrement(&successes);
            return 1;
        }
    }
}

void __winetest_cdecl winetest_ok( int condition, const char *msg, ... )
{
    __winetest_va_list valist;

    __winetest_va_start(valist, msg);
    winetest_vok(condition, msg, valist);
    __winetest_va_end(valist);
}

void __winetest_cdecl winetest_trace( const char *msg, ... )
{
    __winetest_va_list valist;

    if (winetest_debug > 0)
    {
        winetest_print_context( "" );
        __winetest_va_start(valist, msg);
        vfprintf(stdout, msg, valist);
        __winetest_va_end(valist);
    }
}

void __winetest_cdecl winetest_print(const char* msg, ...)
{
    __winetest_va_list valist;
    tls_data* data = get_tls_data();

    fprintf(stdout, __winetest_file_line_prefix ": ", data->current_file, data->current_line);
    __winetest_va_start(valist, msg);
    vfprintf(stdout, msg, valist);
    __winetest_va_end(valist);
}

void winetest_vskip( const char *msg, __winetest_va_list args )
{
    winetest_print_context( "Tests skipped: " );
    vfprintf(stdout, msg, args);
    skipped++;
}

void __winetest_cdecl winetest_skip( const char *msg, ... )
{
    __winetest_va_list valist;
    __winetest_va_start(valist, msg);
    winetest_vskip(msg, valist);
    __winetest_va_end(valist);
}

void __winetest_cdecl winetest_win_skip( const char *msg, ... )
{
    __winetest_va_list valist;
    __winetest_va_start(valist, msg);
    if ((strcmp(winetest_platform, "windows") == 0)
#ifndef USE_WINE_TODOS
    || (strcmp(winetest_platform, "reactos") == 0)
#endif
    )
        winetest_vskip(msg, valist);
    else
        winetest_vok(0, msg, valist);
    __winetest_va_end(valist);
}

void winetest_start_todo( int is_todo )
{
    tls_data* data=get_tls_data();
    data->todo_level = (data->todo_level << 1) | (is_todo != 0);
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    tls_data* data=get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo(void)
{
    tls_data* data=get_tls_data();
    data->todo_level >>= 1;
}

void winetest_start_nocount(unsigned int flags)
{
    tls_data* data = get_tls_data();

    /* The lowest 2 bits of nocount_level specify whether counting of successes
       and/or failures is disabled. For each nested level the bits are shifted
       left, the new lowest 2 bits are copied from the previous state and ored
       with the new mask. This allows nested handling of both states up tp a
       level of 16. */
    flags |= data->nocount_level & 3;
    data->nocount_level = (data->nocount_level << 2) | flags;
    data->todo_do_loop = 1;
}

int winetest_loop_nocount(void)
{
    tls_data* data = get_tls_data();
    int do_loop = data->todo_do_loop;
    data->todo_do_loop = 0;
    return do_loop;
}

void winetest_end_nocount(void)
{
    tls_data* data = get_tls_data();
    data->nocount_level >>= 2;
}

void __winetest_cdecl winetest_push_context(const char* fmt, ...)
{
    tls_data* data = get_tls_data();
    __winetest_va_list valist;

    if (data->context_count < ARRAY_SIZE(data->context))
    {
        __winetest_va_start(valist, fmt);
        vsnprintf(data->context[data->context_count], sizeof(data->context[data->context_count]), fmt, valist);
        __winetest_va_end(valist);
        data->context[data->context_count][sizeof(data->context[data->context_count]) - 1] = 0;
    }
    ++data->context_count;
}

void winetest_pop_context(void)
{
    tls_data* data = get_tls_data();

    if (data->context_count)
        --data->context_count;
}

int winetest_get_mainargs( char*** pargv )
{
    *pargv = winetest_argv;
    return winetest_argc;
}

LONG winetest_get_failures(void)
{
    return failures;
}

LONG winetest_get_successes(void)
{
    return successes;
}

void winetest_add_failures( LONG new_failures )
{
    while (new_failures-- > 0)
        InterlockedIncrement( &failures );
}

void winetest_wait_child_process( HANDLE process )
{
    DWORD exit_code = 1;

    if (WaitForSingleObject( process, 30000 ))
        fprintf( stdout, "%s: child process wait failed\n", current_test->name );
    else
        GetExitCodeProcess( process, &exit_code );

    if (exit_code)
    {
        if (exit_code > 255)
        {
            fprintf( stdout, "%s: exception 0x%08x in child process\n", current_test->name, (unsigned)exit_code );
            InterlockedIncrement( &failures );
        }
        else
        {
            fprintf( stdout, "%s: %u failures in child process\n",
                     current_test->name, (unsigned)exit_code );
            while (exit_code-- > 0)
                InterlockedIncrement(&failures);
        }
    }
}

const char *wine_dbgstr_an( const CHAR *str, intptr_t n )
{
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1)
    {
        const CHAR *end = str;
        while (*end) end++;
        n = end - str;
    }
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5 );
    dst = res = get_temp_buffer( size );
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        CHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = (char)c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    release_temp_buffer( res, dst - res );
    return res;
}

const char *wine_dbgstr_wn( const WCHAR *str, intptr_t n )
{
    char *res;
    static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    char buffer[300], *dst = buffer;

    if (!str) return "(null)";
    if (!((ULONG_PTR)str >> 16))
    {
        res = get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (IsBadStringPtrW(str,n)) return "(invalid)";
    if (n == -1) for (n = 0; str[n]; n++) ;
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= buffer + sizeof(buffer) - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c < ' ' || c >= 127)
            {
                *dst++ = '\\';
                *dst++ = hex[(c >> 12) & 0x0f];
                *dst++ = hex[(c >> 8) & 0x0f];
                *dst++ = hex[(c >> 4) & 0x0f];
                *dst++ = hex[c & 0x0f];
            }
            else *dst++ = (char)c;
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst = 0;

    res = get_temp_buffer(strlen(buffer + 1));
    strcpy(res, buffer);
    return res;
}

const char *wine_dbgstr_guid( const GUID *guid )
{
    char *res;

    if (!guid) return "(null)";
    res = get_temp_buffer( 39 ); /* CHARS_IN_GUID */
    sprintf( res, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
             guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
             guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    return res;
}

const char *wine_dbgstr_point( const POINT *point )
{
    char *res;

    if (!point) return "(null)";
    res = get_temp_buffer( 60 );
#ifdef __ROS_LONG64__
    sprintf( res, "(%d,%d)", point->x, point->y );
#else
    sprintf( res, "(%ld,%ld)", point->x, point->y );
#endif
    release_temp_buffer( res, strlen(res) + 1 );
    return res;
}

const char *wine_dbgstr_size( const SIZE *size )
{
    char *res;

    if (!size) return "(null)";
    res = get_temp_buffer( 60 );
#ifdef __ROS_LONG64__
    sprintf( res, "(%d,%d)", size->cx, size->cy );
#else
    sprintf( res, "(%ld,%ld)", size->cx, size->cy );
#endif
    release_temp_buffer( res, strlen(res) + 1 );
    return res;
}

const char *wine_dbgstr_rect( const RECT *rect )
{
    char *res;

    if (!rect) return "(null)";
    res = get_temp_buffer( 60 );
#ifdef __ROS_LONG64__
    sprintf( res, "(%d,%d)-(%d,%d)", rect->left, rect->top, rect->right, rect->bottom );
#else
    sprintf( res, "(%ld,%ld)-(%ld,%ld)", rect->left, rect->top, rect->right, rect->bottom );
#endif
    release_temp_buffer( res, strlen(res) + 1 );
    return res;
}

#ifdef WINETEST_USE_DBGSTR_LONGLONG
const char *wine_dbgstr_longlong( ULONGLONG ll )
{
    char *res;

    res = get_temp_buffer( 20 );
    if (/*sizeof(ll) > sizeof(unsigned long) &&*/ ll >> 32) /* ULONGLONG is always > long in ReactOS */
        sprintf( res, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll );
    else
        sprintf( res, "%lx", (unsigned long)ll );
    release_temp_buffer( res, strlen(res) + 1 );
    return res;
}
#endif

/* Find a test by name */
static const struct test *find_test( const char *name )
{
    const struct test *test;
    const char *p;
    size_t len;

    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    len = strlen(name);
    if (len > 2 && !strcmp( name + len - 2, ".c" )) len -= 2;

    for (test = winetest_testlist; test->name; test++)
    {
        if (!strncmp( test->name, name, len ) && !test->name[len]) break;
    }
    return test->name ? test : NULL;
}


/* Display list of valid tests */
static void list_tests(void)
{
    const struct test *test;

    fprintf( stdout, "Valid test names:\n" );
    for (test = winetest_testlist; test->name; test++) fprintf( stdout, "    %s\n", test->name );
}

/* Disable false-positive claiming "test" would be NULL-dereferenced */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:28182)
#endif

/* Run a named test, and return exit status */
static int run_test( const char *name )
{
    const struct test *test;
    int status;

    if (!(test = find_test( name )))
    {
        fprintf( stdout, "Fatal: test '%s' does not exist.\n", name );
        exit_process(1);
    }
    successes = failures = todo_successes = todo_failures = 0;
    tls_index=TlsAlloc();
    current_test = test;
    test->func();

    /* show test results even if traces are disabled */
    /*if (winetest_debug)*/
    {
        fprintf( stdout, "\n%s: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
                 test->name, (int)(successes + failures + todo_successes + todo_failures),
                 (int)todo_successes, (int)(failures + todo_failures),
                 (failures + todo_failures != 1) ? "failures" : "failure",
                 (int)skipped );
    }
    status = (failures + todo_failures < 255) ? failures + todo_failures : 255;
    return status;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/* Display usage and exit */
static void usage( const char *argv0 )
{
    fprintf( stdout, "Usage: %s test_name\n\n", argv0 );
    list_tests();
    exit_process(1);
}


/* main function */
int main( int argc, char **argv )
{
    char p[128];

    setvbuf (stdout, NULL, _IONBF, 0);

    winetest_argc = argc;
    winetest_argv = argv;

    if (GetEnvironmentVariableA( "WINETEST_PLATFORM", p, sizeof(p) )) winetest_platform = _strdup(p);
    if (GetEnvironmentVariableA( "WINETEST_DEBUG", p, sizeof(p) )) winetest_debug = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_INTERACTIVE", p, sizeof(p) )) winetest_interactive = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_REPORT_SUCCESS", p, sizeof(p) )) report_success = atoi(p);

    if (!winetest_interactive) SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    if (!argv[1])
    {
        if (winetest_testlist[0].name && !winetest_testlist[1].name)  /* only one test */
            return run_test( winetest_testlist[0].name );
        usage( argv[0] );
    }
    if (!strcmp( argv[1], "--list" ))
    {
        list_tests();
        return 0;
    }
    return run_test(argv[1]);
}

#endif  /* STANDALONE */

// hack for ntdll winetest (this is defined in excpt.h)
#undef exception_info

// Some helpful definitions

#define ok_hex_(file, line, expression, result) \
    do { \
        int _value = (expression); \
        int _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_hex(expression, result)      ok_hex_(__FILE__, __LINE__, expression, result)

#define ok_dec_(file, line, expression, result) \
    do { \
        int _value = (expression); \
        int _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%d), got: %d\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_dec(expression, result)      ok_dec_(__FILE__, __LINE__, expression, result)

#define ok_ptr_(file, line, expression, result) \
    do { \
        const void *_value = (expression); \
        const void *_result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%p), got: %p\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_ptr(expression, result)      ok_ptr_(__FILE__, __LINE__, expression, result)

#define ok_size_t_(file, line, expression, result) \
    do { \
        size_t _value = (expression); \
        size_t _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%Ix), got: %Ix\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_size_t(expression, result)   ok_size_t_(__FILE__, __LINE__, expression, result)

#define ok_char(expression, result) ok_hex(expression, result)

#define ok_err_(file, line, error) \
    ok_(file, line)(GetLastError() == (error), "Wrong last error. Expected " #error ", got 0x%lx\n", GetLastError())
#define ok_err(error)      ok_err_(__FILE__, __LINE__, error)

#define ok_str_(file, line, x, y) \
    ok_(file, line)(strcmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)
#define ok_str(x, y)      ok_str_(__FILE__, __LINE__, x, y)

#define ok_wstr_(file, line, x, y) \
    ok_(file, line)(wcscmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)
#define ok_wstr(x, y)     ok_wstr_(__FILE__, __LINE__, x, y)

#define ok_long(expression, result) ok_hex(expression, result)
#define ok_int(expression, result) ok_dec(expression, result)
#define ok_int_(file, line, expression, result) ok_dec_(file, line, expression, result)
#define ok_ntstatus(status, expected) ok_hex(status, expected)
#define ok_hdl ok_ptr

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __WINE_WINE_TEST_H */
