/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Lightweight testing routines, based on an updated version of wine/test.h
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef __MINI_WINE_TEST_H
#define __MINI_WINE_TEST_H

#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wine/debug.h>

#ifdef __cplusplus
extern "C" {
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
#if 0
extern int winetest_get_mainargs( char*** pargv );
#endif
extern LONG winetest_get_failures(void);
extern LONG winetest_get_successes(void);
extern void winetest_add_failures( LONG new_failures );

extern int broken( int condition );
extern int winetest_vok( int condition, const char *msg, va_list ap );
extern void winetest_vskip( const char *msg, va_list ap );

extern void __cdecl winetest_ok( int condition, const char *msg, ... ) __WINE_PRINTF_ATTR(2,3);
extern void __cdecl winetest_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
extern void __cdecl winetest_win_skip( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
extern void __cdecl winetest_trace( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
extern void __cdecl winetest_push_context( const char *fmt, ... ) __WINE_PRINTF_ATTR(1,2);
extern void winetest_pop_context(void);

#define subtest_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_subtest
#define ok_(file, line)       (winetest_set_location(file, line), 0) ? (void)0 : winetest_ok
#define skip_(file, line)     (winetest_set_location(file, line), 0) ? (void)0 : winetest_skip
#define win_skip_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : winetest_win_skip
#define trace_(file, line)    (winetest_set_location(file, line), 0) ? (void)0 : winetest_trace

#define subtest  subtest_(__RELFILE__, __LINE__)
#define ok       ok_(__RELFILE__, __LINE__)
#define skip     skip_(__RELFILE__, __LINE__)
#define win_skip win_skip_(__RELFILE__, __LINE__)
#define trace    trace_(__RELFILE__, __LINE__)

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

#define skip_2k3_crash if (_winver < 0x600) skip("Test skipped, because it crashes on win 2003\n"); else
#define skip_2k3_fail if (_winver < 0x600) skip("Test skipped, because it fails on win 2003\n"); else

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


/************************************************************************/
/* Below is the implementation of the various functions, to be included
 * directly into the generated testlist.c file.
 * It is done that way so that the dlls can build the test routines with
 * different includes or flags if needed.
 */

#ifdef STANDALONE

#include <stdio.h>

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

#if 0
/* passing arguments around */
static int winetest_argc;
static char** winetest_argv;
#endif

static const struct test *current_test; /* test currently being run */

static LONG winetest_successes;       /* number of successful tests */
static LONG winetest_failures;        /* number of failures */
static LONG winetest_skipped;         /* number of skipped test chunks */
static LONG winetest_todo_successes;  /* number of successful tests inside todo block */
static LONG winetest_todo_failures;   /* number of failures inside todo block */

/* The following data must be kept track of on a per-thread basis */
struct winetest_thread_data
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    unsigned int todo_level;         /* current todo nesting level */
    unsigned int nocount_level;
    int todo_do_loop;
#if 0
    char *str_pos;                   /* position in debug buffer */
    char strings[2000];              /* buffer for debug strings */
#endif
    char context[8][128];            /* data to print before messages */
    unsigned int context_count;      /* number of context prefixes */
} tls_data;

static DWORD tls_index;

static struct winetest_thread_data *winetest_get_thread_data(void)
{
    struct winetest_thread_data *data;
    DWORD last_error;

    last_error = GetLastError();
    data = TlsGetValue( tls_index );
    if (!data)
    {
        data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) );
#if 0
        data->str_pos = data->strings;
#endif
        TlsSetValue( tls_index, data );
    }
    SetLastError( last_error );
    return data;
}

static int winetest_vprintf( const char *msg, va_list args )
{
    static struct __wine_debug_functions s_Debug = {NULL};
    if (!s_Debug.dbg_vprintf)
        __wine_dbg_set_functions(NULL, &s_Debug, sizeof(s_Debug));

    return s_Debug.dbg_vprintf( msg, args );
}

void winetest_set_location( const char* file, int line )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
#if 1 /*|| defined(WINETEST_MSVC_IDE_FORMATTING)*/
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

static int __cdecl winetest_printf( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static int __cdecl winetest_printf( const char *msg, ... )
{
    va_list valist;
    int ret;

    va_start( valist, msg );
    ret = winetest_vprintf( msg, valist );
    va_end( valist );

    return ret;
}

static void __cdecl winetest_print_location( const char *msg, ... ) __WINE_PRINTF_ATTR(1,2);
static void __cdecl winetest_print_location( const char *msg, ... )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    // char elapsed[64];
    va_list valist;

    // winetest_printf( "%s:%d:%s ", data->current_file, data->current_line, winetest_elapsed( elapsed ) );
    winetest_printf( __winetest_file_line_prefix ": ", data->current_file, data->current_line );
    va_start( valist, msg );
    winetest_vprintf( msg, valist );
    va_end( valist );
}

static void __cdecl winetest_print_context( const char *msgtype )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    unsigned int i;

    winetest_print_location( "%s", msgtype );
    for (i = 0; i < data->context_count; ++i)
        winetest_printf( "%s: ", data->context[i] );
}

void winetest_subtest(const char* name)
{
    winetest_print_location( "Subtest %s\n", name );
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
int winetest_vok( int condition, const char *msg, va_list args )
{
    struct winetest_thread_data *data = winetest_get_thread_data();

    if (data->todo_level)
    {
        if (condition)
        {
            winetest_print_context( "Test succeeded inside todo block: " );
            winetest_vprintf( msg, args );
            if ((data->nocount_level & 2) == 0)
            InterlockedIncrement( &winetest_todo_failures );
            return 0;
        }
        else
        {
            /* show todos even if traces are disabled*/
            /*if (winetest_debug > 0)*/
            {
                winetest_print_context( "Test marked todo: " );
                winetest_vprintf( msg, args );
            }
            if ((data->nocount_level & 1) == 0)
            InterlockedIncrement( &winetest_todo_successes );
            return 1;
        }
    }
    else
    {
        if (!condition)
        {
            winetest_print_context( "Test failed: " );
            winetest_vprintf( msg, args );
            if ((data->nocount_level & 2) == 0)
            InterlockedIncrement( &winetest_failures );
            return 0;
        }
        else
        {
            if (report_success && (data->nocount_level & 1) == 0)
            {
                winetest_print_location("Test succeeded\n");
            }
            if ((data->nocount_level & 1) == 0)
            InterlockedIncrement( &winetest_successes );
            return 1;
        }
    }
}

void __cdecl winetest_ok( int condition, const char *msg, ... )
{
    va_list valist;

    va_start(valist, msg);
    winetest_vok(condition, msg, valist);
    va_end(valist);
}

void __cdecl winetest_trace( const char *msg, ... )
{
    va_list valist;

    if (winetest_debug > 0)
    {
        winetest_print_context( "" );
        va_start(valist, msg);
        winetest_vprintf( msg, valist );
        va_end(valist);
    }
}

void winetest_vskip( const char *msg, va_list args )
{
    winetest_print_context( "Tests skipped: " );
    winetest_vprintf( msg, args );
    InterlockedIncrement( &winetest_skipped );
}

void __cdecl winetest_skip( const char *msg, ... )
{
    va_list valist;
    va_start(valist, msg);
    winetest_vskip(msg, valist);
    va_end(valist);
}

void __cdecl winetest_win_skip( const char *msg, ... )
{
    va_list valist;
    va_start(valist, msg);
    if ((strcmp(winetest_platform, "windows") == 0)
#if !defined(USE_WINE_TODOS) || defined(USE_WIN_SKIP)
    || (strcmp(winetest_platform, "reactos") == 0)
#endif
    )
        winetest_vskip(msg, valist);
    else
        winetest_vok(0, msg, valist);
    va_end(valist);
}

void winetest_start_todo( int is_todo )
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    data->todo_level = (data->todo_level << 1) | (is_todo != 0);
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo(void)
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    data->todo_level >>= 1;
}

void winetest_start_nocount(unsigned int flags)
{
    struct winetest_thread_data *data = winetest_get_thread_data();

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
    struct winetest_thread_data *data = winetest_get_thread_data();
    int do_loop = data->todo_do_loop;
    data->todo_do_loop = 0;
    return do_loop;
}

void winetest_end_nocount(void)
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    data->nocount_level >>= 2;
}

void __cdecl winetest_push_context(const char* fmt, ...)
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    va_list valist;

    if (data->context_count < ARRAY_SIZE(data->context))
    {
        va_start(valist, fmt);
        vsnprintf(data->context[data->context_count], sizeof(data->context[data->context_count]), fmt, valist);
        va_end(valist);
        data->context[data->context_count][sizeof(data->context[data->context_count]) - 1] = 0;
    }
    ++data->context_count;
}

void winetest_pop_context(void)
{
    struct winetest_thread_data *data = winetest_get_thread_data();

    if (data->context_count)
        --data->context_count;
}

#if 0
int winetest_get_mainargs( char*** pargv )
{
    *pargv = winetest_argv;
    return winetest_argc;
}
#endif

LONG winetest_get_failures(void)
{
    return winetest_failures;
}

LONG winetest_get_successes(void)
{
    return winetest_successes;
}

void winetest_add_failures( LONG new_failures )
{
    while (new_failures-- > 0)
        InterlockedIncrement( &winetest_failures );
}

#if 0
static char *_strdup(const char *str)
{
    char* ptr = malloc((strlen(str)+1)*sizeof(char));
    if (ptr)
        strcpy(ptr, str);
    return ptr;
}
#endif

/* Initialize testing support */
static void init_test( const struct test *test )
{
    char p[128];

#if 0
    winetest_argc = __argc;
    winetest_argv = __argv;
#endif

    // if (GetEnvironmentVariableA( "WINETEST_PLATFORM", p, sizeof(p) )) winetest_platform = _strdup(p);
    if (GetEnvironmentVariableA( "WINETEST_DEBUG", p, sizeof(p) )) winetest_debug = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_INTERACTIVE", p, sizeof(p) )) winetest_interactive = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_REPORT_SUCCESS", p, sizeof(p) )) report_success = atoi(p);

    if (!winetest_interactive) SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    current_test = test;
    winetest_successes = winetest_failures = winetest_skipped = winetest_todo_successes = winetest_todo_failures = 0;
    tls_index = TlsAlloc();
}

/* Executed once test is finished: shows results and returns exit status */
static int fini_test(void)
{
    int status;
    const struct test *test = current_test;

    /* show test results even if traces are disabled */
    /*if (winetest_debug)*/
    {
        //winetest_printf
        winetest_print_location( "\n%s: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
                 test->name, (int)(winetest_successes + winetest_failures + winetest_todo_successes + winetest_todo_failures),
                 (int)winetest_todo_successes, (int)(winetest_failures + winetest_todo_failures),
                 (winetest_failures + winetest_todo_failures != 1) ? "failures" : "failure",
                 (int)winetest_skipped );
    }
    status = (winetest_failures + winetest_todo_failures < 255) ? winetest_failures + winetest_todo_failures : 255;
    return status;
}

#endif  /* STANDALONE */

// Some helpful definitions

#define ok_hex_(file, line, expression, result) \
    do { \
        int _value = (expression); \
        int _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (0x%x), got: 0x%x\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_hex(expression, result)      ok_hex_(__RELFILE__, __LINE__, expression, result)

#define ok_dec_(file, line, expression, result) \
    do { \
        int _value = (expression); \
        int _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%d), got: %d\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_dec(expression, result)      ok_dec_(__RELFILE__, __LINE__, expression, result)

#define ok_ptr_(file, line, expression, result) \
    do { \
        const void *_value = (expression); \
        const void *_result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%p), got: %p\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_ptr(expression, result)      ok_ptr_(__RELFILE__, __LINE__, expression, result)

#define ok_size_t_(file, line, expression, result) \
    do { \
        size_t _value = (expression); \
        size_t _result = (result); \
        ok_(file, line)(_value == _result, "Wrong value for '%s', expected: " #result " (%Ix), got: %Ix\n", \
           #expression, _result, _value); \
    } while (0)
#define ok_size_t(expression, result)   ok_size_t_(__RELFILE__, __LINE__, expression, result)

#define ok_char(expression, result) ok_hex(expression, result)

#define ok_err_(file, line, error) \
    ok_(file, line)(GetLastError() == (error), "Wrong last error. Expected " #error ", got 0x%lx\n", GetLastError())
#define ok_err(error)      ok_err_(__RELFILE__, __LINE__, error)

#define ok_str_(file, line, x, y) \
    ok_(file, line)(strcmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)
#define ok_str(x, y)      ok_str_(__RELFILE__, __LINE__, x, y)

#define ok_wstr_(file, line, x, y) \
    ok_(file, line)(wcscmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)
#define ok_wstr(x, y)     ok_wstr_(__RELFILE__, __LINE__, x, y)

#define ok_long(expression, result) ok_hex(expression, result)
#define ok_int(expression, result) ok_dec(expression, result)
#define ok_int_(file, line, expression, result) ok_dec_(file, line, expression, result)
#define ok_ntstatus(status, expected) ok_hex(status, expected)
#define ok_hdl ok_ptr

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __MINI_WINE_TEST_H */
