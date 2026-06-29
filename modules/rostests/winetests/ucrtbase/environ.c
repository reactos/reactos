/*
 * Unit tests for C library environment routines
 *
 * Copyright 2004 Mike Hearn <mh@codeweavers.com>
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

#include "wine/test.h"
#include <errno.h>
#include <stdlib.h>
#include <process.h>
#include <winnls.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(invalid_parameter_handler);

static const char *a_very_long_env_string =
 "LIBRARY_PATH="
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/;"
 "/mingw/lib/gcc/mingw32/3.4.2/;"
 "/usr/lib/gcc/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../../mingw32/lib/mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../../mingw32/lib/;"
 "/mingw/mingw32/lib/mingw32/3.4.2/;"
 "/mingw/mingw32/lib/;"
 "/mingw/lib/mingw32/3.4.2/;"
 "/mingw/lib/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../mingw32/3.4.2/;"
 "C:/Program Files/GLBasic/Compiler/platform/Win32/Bin/../lib/gcc/mingw32/3.4.2/../../../;"
 "/mingw/lib/mingw32/3.4.2/;"
 "/mingw/lib/;"
 "/lib/mingw32/3.4.2/;"
 "/lib/;"
 "/usr/lib/mingw32/3.4.2/;"
 "/usr/lib/";

static char ***(__cdecl *p__p__environ)(void);
static WCHAR ***(__cdecl *p__p__wenviron)(void);
static char** (__cdecl *p_get_initial_narrow_environment)(void);
static wchar_t** (__cdecl *p_get_initial_wide_environment)(void);
static errno_t (__cdecl *p_putenv_s)(const char*, const char*);
static errno_t (__cdecl *p_wputenv_s)(const wchar_t*, const wchar_t*);
static errno_t (__cdecl *p_getenv_s)(size_t*, char*, size_t, const char*);

static char ***p_environ;
static WCHAR ***p_wenviron;

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
                                                   const wchar_t *function, const wchar_t *file,
                                                   unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %Ix\n", arg);
}

static BOOL init(void)
{
    HMODULE hmod = GetModuleHandleA( "ucrtbase.dll" );

    p__p__environ = (void *)GetProcAddress( hmod, "__p__environ" );
    p__p__wenviron = (void *)GetProcAddress( hmod, "__p__wenviron" );
    p_get_initial_narrow_environment = (void *)GetProcAddress( hmod, "_get_initial_narrow_environment" );
    p_get_initial_wide_environment = (void *)GetProcAddress( hmod, "_get_initial_wide_environment" );
    p_putenv_s = (void *)GetProcAddress( hmod, "_putenv_s" );
    p_wputenv_s = (void *)GetProcAddress( hmod, "_wputenv_s" );
    p_getenv_s = (void *)GetProcAddress( hmod, "getenv_s" );

    ok(p__p__environ != NULL, "Unexecped NULL pointer to environ\n" );
    ok(p__p__wenviron != NULL, "Unexecped NULL pointer to environ\n" );
    if (!p__p__environ || !p__p__wenviron)
    {
        skip( "NULL pointers for environment\n" );
        return FALSE;
    }
    p_environ = p__p__environ();
    p_wenviron = p__p__wenviron();
    return TRUE;
}

static unsigned env_get_entry_countA( char **env )
{
    unsigned count;

    if (!env) return 0;
    for (count = 0; env[count] != NULL; count++) {}
    return count;
}

static void test_initial_environ( void )
{
    ok( p__p__environ() != NULL, "Unexpected NULL _environ[]\n" );
    ok( *p__p__environ() != NULL, "Unexpected empty _environ[]\n" );
    ok( p_get_initial_narrow_environment() != NULL, "Unexpected empty narrow initial environment\n" );
    ok( p_get_initial_narrow_environment() == *p__p__environ(), "Expecting _environ[] to match initial narrow environment\n" );

    ok( p__p__wenviron() != NULL, "Unexpected NULL _wenviron[]\n" );
    ok( *p__p__wenviron() == NULL, "Unexpected non empty _wenviron[]\n" );
    ok( p_get_initial_wide_environment() != NULL, "Unexpected empty wide initial environment\n" );
    ok( p_get_initial_wide_environment() == *p__p__wenviron(), "Expecting _wenviron[] to match initial wide environment\n" );
}

static void test_environment_manipulation(void)
{
    char buf[256];
    errno_t ret;
    size_t len;
    unsigned count;
    char* first;
    char* second;

    ok( _putenv( "cat=" ) == 0, "_putenv failed on deletion of nonexistent environment variable\n" );
    ok( _putenv( "cat=dog" ) == 0, "failed setting cat=dog\n" );
    ok( strcmp( getenv( "cat" ), "dog" ) == 0, "getenv did not return 'dog'\n" );
    if (p_getenv_s)
    {
        ret = p_getenv_s( &len, buf, sizeof(buf), "cat" );
        ok( !ret, "getenv_s returned %d\n", ret );
        ok( len == 4, "getenv_s returned length is %Id\n", len);
        ok( !strcmp(buf, "dog"), "getenv_s did not return 'dog'\n" );
    }
    ok( _putenv("cat=") == 0, "failed deleting cat\n" );

    ok( _putenv("=") == -1, "should not accept '=' as input\n" );
    ok( _putenv("=dog") == -1, "should not accept '=dog' as input\n" );
    ok( _putenv(a_very_long_env_string) == 0, "_putenv failed for long environment string\n" );

    ok( getenv("nonexistent") == NULL, "getenv should fail with nonexistent var name\n" );

    if (p_putenv_s)
    {
        SET_EXPECT(invalid_parameter_handler);
        ret = p_putenv_s( NULL, "dog" );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret );
        SET_EXPECT(invalid_parameter_handler);
        ret = p_putenv_s( "cat", NULL );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret );
        SET_EXPECT(invalid_parameter_handler);
        ret = p_putenv_s( "a=b", NULL );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_putenv_s returned %d\n", ret );
        ret = p_putenv_s( "cat", "a=b" );
        ok( !ret, "_putenv_s returned %d\n", ret );
        ret = p_putenv_s( "cat", "" );
        ok( !ret, "_putenv_s returned %d\n", ret );
    }

    if (p_wputenv_s)
    {
        SET_EXPECT(invalid_parameter_handler);
        ret = p_wputenv_s( NULL, L"dog" );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret );
        SET_EXPECT(invalid_parameter_handler);
        ret = p_wputenv_s( L"cat", NULL );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret );
        SET_EXPECT(invalid_parameter_handler);
        ret = p_wputenv_s( L"a=b", NULL );
        CHECK_CALLED(invalid_parameter_handler);
        ok( ret == EINVAL, "_wputenv_s returned %d\n", ret );
        ret = p_wputenv_s( L"cat", L"a=b" );
        ok( !ret, "_wputenv_s returned %d\n", ret );
        ret = p_wputenv_s( L"cat", L"" );
        ok( !ret, "_wputenv_s returned %d\n", ret );
    }

    if (p_getenv_s)
    {
        buf[0] = 'x';
        len = 1;
        errno = 0xdeadbeef;
        ret = p_getenv_s( &len, buf, sizeof(buf), "nonexistent" );
        ok( !ret, "_getenv_s returned %d\n", ret );
        ok( !len, "getenv_s returned length is %Id\n", len );
        ok( !buf[0], "buf = %s\n", buf );
        ok( errno == 0xdeadbeef, "errno = %d\n", errno );

        buf[0] = 'x';
        len = 1;
        errno = 0xdeadbeef;
        ret = p_getenv_s( &len, buf, sizeof(buf), NULL );
        ok( !ret, "_getenv_s returned %d\n", ret );
        ok( !len, "getenv_s returned length is %Id\n", len );
        ok( !buf[0], "buf = %s\n", buf );
        ok( errno == 0xdeadbeef, "errno = %d\n", errno );
    }

    /* test stability of _environ[] pointers */
    ok( _putenv( "__winetest_cat=" ) == 0, "Couldn't reset env var\n" );
    ok( _putenv( "__winetest_dog=" ) == 0, "Couldn't reset env var\n" );
    count = env_get_entry_countA( *p_environ );
    ok( _putenv( "__winetest_cat=mew") == 0, "Couldn't set env var\n" );
    ok( !strcmp( (*p_environ)[count], "__winetest_cat=mew"), "Unexpected env var value\n" );
    first = (*p_environ)[count];
    ok( getenv("__winetest_cat") == strchr( (*p_environ)[count], '=') + 1, "Expected getenv() to return pointer inside _environ[] entry\n" );
    ok( _putenv( "__winetest_dog=bark" ) == 0, "Couldn't set env var\n" );
    ok( !strcmp( (*p_environ)[count + 1], "__winetest_dog=bark" ), "Unexpected env var value\n" );
    ok( getenv( "__winetest_dog" ) == strchr( (*p_environ)[count + 1], '=' ) + 1, "Expected getenv() to return pointer inside _environ[] entry\n" );
    ok( first == (*p_environ)[count], "Expected stability of _environ[count] pointer\n" );
    second = (*p_environ)[count + 1];
    ok( count + 2 == env_get_entry_countA( *p_environ ), "Unexpected count\n" );

    ok( _putenv( "__winetest_cat=purr" ) == 0, "Couldn't set env var\n" );
    ok( !strcmp( (*p_environ)[count], "__winetest_cat=purr" ), "Unexpected env var value\n" );
    ok( getenv( "__winetest_cat" ) == strchr( (*p_environ)[count], '=' ) + 1, "Expected getenv() to return pointer inside _environ[] entry\n" );
    ok( second == (*p_environ)[count + 1], "Expected stability of _environ[count] pointer\n" );
    ok( !strcmp( (*p_environ)[count + 1], "__winetest_dog=bark" ), "Couldn't get env var value\n" );
    ok( getenv( "__winetest_dog" ) == strchr( (*p_environ)[count + 1], '=' ) + 1, "Expected getenv() to return pointer inside _environ[] entry\n" );
    ok( count + 2 == env_get_entry_countA( *p_environ ), "Unexpected count\n" );
    ok( _putenv( "__winetest_cat=" ) == 0, "Couldn't reset env vat\n" );
    ok( second == (*p_environ)[count], "Expected _environ[count] to be second\n" );
    ok( !strcmp( (*p_environ)[count], "__winetest_dog=bark" ), "Unexpected env var value\n" );
    ok( count + 1 == env_get_entry_countA( *p_environ ), "Unexpected count\n" );
    ok( _putenv( "__winetest_dog=" ) == 0, "Couldn't reset env var\n" );
    ok( count == env_get_entry_countA( *p_environ ), "Unexpected count\n" );

    /* in putenv, only changed variable is updated (no other reload of kernel info is done) */
    ret = SetEnvironmentVariableA( "__winetest_cat", "meow" );
    ok( ret, "SetEnvironmentVariableA failed: %lu\n", GetLastError() );
    ok( _putenv( "__winetest_dog=bark" ) == 0, "Couldn't set env var\n" );
    ok( getenv( "__winetest_cat" ) == NULL, "msvcrt env cache shouldn't have been updated\n" );
    ok( _putenv( "__winetest_cat=" ) == 0, "Couldn't reset env var\n" );
    ok( _putenv( "__winetest_dog=" ) == 0, "Couldn't reset env var\n" );

    /* test setting unicode bits */
    count = env_get_entry_countA( *p_environ );
    ret = WideCharToMultiByte( CP_ACP, 0, L"\u263a", -1, buf, ARRAY_SIZE(buf), 0, 0 );
    ok( ret, "WideCharToMultiByte failed: %lu\n", GetLastError() );
    ok( _wputenv( L"__winetest_cat=\u263a" ) == 0, "Couldn't set env var\n" );
    ok( _wgetenv( L"__winetest_cat" ) && !wcscmp( _wgetenv( L"__winetest_cat" ), L"\u263a" ), "Couldn't retrieve env var\n" );
    ok( getenv( "__winetest_cat" ) && !strcmp( getenv( "__winetest_cat" ), buf ), "Couldn't retrieve env var\n" );
    ok( _wputenv( L"__winetest_cat=" ) == 0, "Couldn't reset env var\n" );

    ret = WideCharToMultiByte( CP_ACP, 0, L"__winetest_\u263a", -1, buf, ARRAY_SIZE(buf), 0, 0 );
    ok( ret, "WideCharToMultiByte failed: %lu\n", GetLastError() );
    ok( _wputenv( L"__winetest_\u263a=bark" ) == 0, "Couldn't set env var\n" );
    ok( _wgetenv( L"__winetest_\u263a" ) && !wcscmp( _wgetenv( L"__winetest_\u263a" ), L"bark"), "Couldn't retrieve env var\n" );
    ok( getenv( buf ) && !strcmp( getenv( buf ), "bark"), "Couldn't retrieve env var %s\n", wine_dbgstr_a(buf) );
    ok( _wputenv( L"__winetest_\u263a=" ) == 0, "Couldn't reset env var\n" );
    ok( count == env_get_entry_countA( *p_environ ), "Unexpected modification of _environ[]\n" );
}

static void test_child_env(char** argv)
{
    STARTUPINFOA si = {sizeof(si)};
    WCHAR *cur_env, *env, *p, *q;
    PROCESS_INFORMATION pi;
    char tmp[1024];
    BOOL ret;
    int len;

    cur_env = GetEnvironmentStringsW();
    ok( cur_env != NULL, "GetEnvironemntStrings failed\n" );

    p = cur_env;
    while (*p) p += wcslen( p ) + 1;
    len = p - cur_env;
    env = malloc( (len + 1024) * sizeof(*env) );
    memcpy(env, cur_env, len * sizeof(*env) );
    q = env + len;
    FreeEnvironmentStringsW( cur_env );

    wcscpy( q, L"__winetest_dog=bark" );
    q += wcslen( L"__winetest_dog=bark" ) + 1;
    wcscpy( q, L"__winetest_\u263a=\u03b2" );
    q += wcslen( L"__winetest_\u263a=\u03b2" ) + 1;
    *q = 0;

    snprintf( tmp, sizeof(tmp), "%s %s create", argv[0], argv[1] );
    ret = CreateProcessA( NULL, tmp, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, env, NULL, &si, &pi );
    ok( ret, "Couldn't create child process %s\n", tmp );
    winetest_wait_child_process( pi.hProcess );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    free( env );
}

START_TEST(environ)
{
    char **argv;
    int argc;

    if (!init()) return;

    ok( _set_invalid_parameter_handler( test_invalid_parameter_handler ) == NULL,
       "Invalid parameter handler was already set\n" );

    argc = winetest_get_mainargs( &argv );
    if (argc == 3 && !strcmp( argv[2], "create" ))
    {
        ok( getenv( "__winetest_dog" ) && !strcmp( getenv( "__winetest_dog" ), "bark" ),
                "Couldn't find env var\n" );
        ok( _wgetenv( L"__winetest_\u263a" ) && !wcscmp( _wgetenv( L"__winetest_\u263a" ), L"\u03b2" ),
                "Couldn't find unicode env var\n" );
        return;
    }

    test_initial_environ();
    test_environment_manipulation();
    test_child_env(argv);
}
