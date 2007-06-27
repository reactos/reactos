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
#include <wingdi.h>
#include <winreg.h>

#ifdef __WINE_WINE_LIBRARY_H
#error wine/library.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_UNICODE_H
#error wine/unicode.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_DEBUG_H
#error wine/debug.h should not be used in Wine tests
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES  ((DWORD)~0UL)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)~0UL)
#endif

/* debug level */
extern int winetest_debug;

/* running in interactive mode? */
extern int winetest_interactive;

/* current platform */
extern const char *winetest_platform;

extern void winetest_set_location( const char* file, int line );
extern void winetest_start_todo( const char* platform );
extern int winetest_loop_todo(void);
extern void winetest_end_todo( const char* platform );
extern int winetest_get_mainargs( char*** pargv );

#ifdef STANDALONE
#define START_TEST(name) \
  static void func_##name(void); \
  const struct test winetest_testlist[] = { { #name, func_##name }, { 0, 0 } }; \
  static void func_##name(void)
#else
#define START_TEST(name) void func_##name(void)
#endif

#ifdef __GNUC__

extern int winetest_ok( int condition, const char *msg, ... ) __attribute__((format (printf,2,3) ));
extern void winetest_skip( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void winetest_trace( const char *msg, ... ) __attribute__((format (printf,1,2)));

#else /* __GNUC__ */

extern int winetest_ok( int condition, const char *msg, ... );
extern void winetest_skip( const char *msg, ... );
extern void winetest_trace( const char *msg, ... );

#endif /* __GNUC__ */

#define ok_(file, line)     (winetest_set_location(file, line), 0) ? 0 : winetest_ok
#define skip_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_skip
#define trace_(file, line)  (winetest_set_location(file, line), 0) ? (void)0 : winetest_trace

#define ok     ok_(__FILE__, __LINE__)
#define skip   skip_(__FILE__, __LINE__)
#define trace  trace_(__FILE__, __LINE__)

#define todo(platform) for (winetest_start_todo(platform); \
                            winetest_loop_todo(); \
                            winetest_end_todo(platform))
#define todo_wine      todo("wine")


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
static int report_success = 0;

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
    int todo_level;                  /* current todo nesting level */
    int todo_do_loop;
} tls_data;
static DWORD tls_index;

static tls_data* get_tls_data(void)
{
    tls_data* data;
    DWORD last_error;

    last_error=GetLastError();
    data=TlsGetValue(tls_index);
    if (!data)
    {
        data=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(tls_data));
        TlsSetValue(tls_index,data);
    }
    SetLastError(last_error);
    return data;
}

static void exit_process( int code )
{
    fflush( stdout );
    ExitProcess( code );
}


void winetest_set_location( const char* file, int line )
{
    tls_data* data=get_tls_data();
    data->current_file=strrchr(file,'/');
    if (data->current_file==NULL)
        data->current_file=strrchr(file,'\\');
    if (data->current_file==NULL)
        data->current_file=file;
    else
        data->current_file++;
    data->current_line=line;
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
int winetest_ok( int condition, const char *msg, ... )
{
    va_list valist;
    tls_data* data=get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            fprintf( stdout, "%s:%d: Test succeeded inside todo block",
                     data->current_file, data->current_line );
            if (msg[0])
            {
                va_start(valist, msg);
                fprintf(stdout,": ");
                vfprintf(stdout, msg, valist);
                va_end(valist);
            }
            InterlockedIncrement(&todo_failures);
            return 0;
        }
        else InterlockedIncrement(&todo_successes);
    }
    else
    {
        if (!condition)
        {
            fprintf( stdout, "%s:%d: Test failed",
                     data->current_file, data->current_line );
            if (msg[0])
            {
                va_start(valist, msg);
                fprintf( stdout,": ");
                vfprintf(stdout, msg, valist);
                va_end(valist);
            }
            InterlockedIncrement(&failures);
            return 0;
        }
        else
        {
            if (report_success)
                fprintf( stdout, "%s:%d: Test succeeded\n",
                         data->current_file, data->current_line);
            InterlockedIncrement(&successes);
        }
    }
    return 1;
}

void winetest_trace( const char *msg, ... )
{
    va_list valist;
    tls_data* data=get_tls_data();

    if (winetest_debug > 0)
    {
        fprintf( stdout, "%s:%d:", data->current_file, data->current_line );
        va_start(valist, msg);
        vfprintf(stdout, msg, valist);
        va_end(valist);
    }
}

void winetest_skip( const char *msg, ... )
{
    va_list valist;
    tls_data* data=get_tls_data();

    fprintf( stdout, "%s:%d: Tests skipped: ", data->current_file, data->current_line );
    va_start(valist, msg);
    vfprintf(stdout, msg, valist);
    va_end(valist);
    skipped++;
}

void winetest_start_todo( const char* platform )
{
    tls_data* data=get_tls_data();
    if (strcmp(winetest_platform,platform)==0)
        data->todo_level++;
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    tls_data* data=get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo( const char* platform )
{
    if (strcmp(winetest_platform,platform)==0)
    {
        tls_data* data=get_tls_data();
        data->todo_level--;
    }
}

int winetest_get_mainargs( char*** pargv )
{
    *pargv = winetest_argv;
    return winetest_argc;
}

/* Find a test by name */
static const struct test *find_test( const char *name )
{
    const struct test *test;
    const char *p;
    int len;

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

    if (winetest_debug)
    {
        fprintf( stdout, "%s: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
                 test->name, successes + failures + todo_successes + todo_failures,
                 todo_successes, failures + todo_failures,
                 (failures + todo_failures != 1) ? "failures" : "failure",
                 skipped );
    }
    status = (failures + todo_failures < 255) ? failures + todo_failures : 255;
    return status;
}


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
    char *p;

    setvbuf (stdout, NULL, _IONBF, 0);

    winetest_argc = argc;
    winetest_argv = argv;

    if ((p = getenv( "WINETEST_PLATFORM" ))) winetest_platform = strdup(p);
    if ((p = getenv( "WINETEST_DEBUG" ))) winetest_debug = atoi(p);
    if ((p = getenv( "WINETEST_INTERACTIVE" ))) winetest_interactive = atoi(p);
    if ((p = getenv( "WINETEST_REPORT_SUCCESS"))) report_success = atoi(p);
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

#endif  /* __WINE_WINE_TEST_H */
