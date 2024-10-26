/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <io.h>
#include <sys/stat.h>
#include <share.h>
#include <fcntl.h>
#include <time.h>
#include <direct.h>
#include <locale.h>
#include <process.h>
#include <fenv.h>
#include <malloc.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

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

static inline double __port_min_pos_double(void)
{
    static const UINT64 __min_pos_double = 0x10000000000000;
    return *(const double *)&__min_pos_double;
}

static inline double __port_max_double(void)
{
    static const UINT64 __max_double = 0x7FEFFFFFFFFFFFFF;
    return *(const double *)&__max_double;
}

DEFINE_EXPECT(global_invalid_parameter_handler);
DEFINE_EXPECT(thread_invalid_parameter_handler);

typedef struct {
    const char *short_wday[7];
    const char *wday[7];
    const char *short_mon[12];
    const char *mon[12];
    const char *am;
    const char *pm;
    const char *short_date;
    const char *date;
    const char *time;
    int unk;
    int refcount;
    const wchar_t *short_wdayW[7];
    const wchar_t *wdayW[7];
    const wchar_t *short_monW[12];
    const wchar_t *monW[12];
    const wchar_t *amW;
    const wchar_t *pmW;
    const wchar_t *short_dateW;
    const wchar_t *dateW;
    const wchar_t *timeW;
    const wchar_t *locnameW;
} __lc_time_data;

typedef void (__cdecl *_se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);

static LONGLONG crt_init_end;

_ACRTIMP int __cdecl _o__initialize_onexit_table(_onexit_table_t *table);
_ACRTIMP int __cdecl _o__register_onexit_function(_onexit_table_t *table, _onexit_t func);
_ACRTIMP int __cdecl _o__execute_onexit_table(_onexit_table_t *table);
_ACRTIMP void *__cdecl _o_malloc(size_t);
_se_translator_function __cdecl _set_se_translator(_se_translator_function func);
void** __cdecl __current_exception(void);
int* __cdecl __processing_throw(void);

#define _MAX__TIME64_T     (((__time64_t)0x00000007 << 32) | 0x93406FFF)

static void test__initialize_onexit_table(void)
{
    _onexit_table_t table, table2;
    int ret;

    ret = _initialize_onexit_table(NULL);
    ok(ret == -1, "got %d\n", ret);

    memset(&table, 0, sizeof(table));
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);

    memset(&table2, 0, sizeof(table2));
    ret = _initialize_onexit_table(&table2);
    ok(ret == 0, "got %d\n", ret);
    ok(table2._first == table._first, "got %p, %p\n", table2._first, table._first);
    ok(table2._last == table._last, "got %p, %p\n", table2._last, table._last);
    ok(table2._end == table._end, "got %p, %p\n", table2._end, table._end);

    memset(&table2, 0, sizeof(table2));
    ret = _o__initialize_onexit_table(&table2);
    ok(ret == 0, "got %d\n", ret);
    ok(table2._first == table._first, "got %p, %p\n", table2._first, table._first);
    ok(table2._last == table._last, "got %p, %p\n", table2._last, table._last);
    ok(table2._end == table._end, "got %p, %p\n", table2._end, table._end);

    /* uninitialized table */
    table._first = table._last = table._end = (void*)0x123;
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
    ok(table._first != (void*)0x123, "got %p\n", table._first);

    table._first = (void*)0x123;
    table._last = (void*)0x456;
    table._end = (void*)0x123;
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
    ok(table._first != (void*)0x123, "got %p\n", table._first);

    table._first = (void*)0x123;
    table._last = (void*)0x456;
    table._end = (void*)0x789;
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == (void*)0x123, "got %p\n", table._first);
    ok(table._last == (void*)0x456, "got %p\n", table._last);
    ok(table._end == (void*)0x789, "got %p\n", table._end);

    table._first = NULL;
    table._last = (void*)0x456;
    table._end = NULL;
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
}

static int g_onexit_called;
static int CDECL onexit_func(void)
{
    g_onexit_called++;
    return 0;
}

static int CDECL onexit_func2(void)
{
    ok(g_onexit_called == 0, "got %d\n", g_onexit_called);
    g_onexit_called++;
    return 0;
}

static void test__register_onexit_function(void)
{
    _onexit_table_t table;
    _PVFV *f;
    int ret;

    memset(&table, 0, sizeof(table));
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(NULL, NULL);
    ok(ret == -1, "got %d\n", ret);

    ret = _register_onexit_function(NULL, onexit_func);
    ok(ret == -1, "got %d\n", ret);

    f = table._last;
    ret = _register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    f = table._last;
    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    f = table._last;
    ret = _o__register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    f = table._last;
    ret = _o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    ret = _execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
}

static void test__execute_onexit_table(void)
{
    _onexit_table_t table;
    int ret;

    ret = _execute_onexit_table(NULL);
    ok(ret == -1, "got %d\n", ret);

    memset(&table, 0, sizeof(table));
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    /* execute empty table */
    ret = _execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    /* same function registered multiple times */
    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = _o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ok(table._first != table._end, "got %p, %p\n", table._first, table._end);
    g_onexit_called = 0;
    ret = _execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 3, "got %d\n", g_onexit_called);
    ok(table._first == table._end, "got %p, %p\n", table._first, table._end);

    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = _o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ok(table._first != table._end, "got %p, %p\n", table._first, table._end);
    g_onexit_called = 0;
    ret = _o__execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 3, "got %d\n", g_onexit_called);
    ok(table._first == table._end, "got %p, %p\n", table._first, table._end);

    /* execute again, table is already empty */
    g_onexit_called = 0;
    ret = _execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 0, "got %d\n", g_onexit_called);

    /* check call order */
    memset(&table, 0, sizeof(table));
    ret = _initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = _register_onexit_function(&table, onexit_func2);
    ok(ret == 0, "got %d\n", ret);

    g_onexit_called = 0;
    ret = _execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 2, "got %d\n", g_onexit_called);
}

static void test___fpe_flt_rounds(void)
{
    unsigned int cfp = _controlfp(0, 0);
    int ret;

    if(!cfp) {
        skip("_controlfp not supported\n");
        return;
    }

    ok((_controlfp(_RC_NEAR, _RC_CHOP) & _RC_CHOP) == _RC_NEAR, "_controlfp(_RC_NEAR, _RC_CHOP) failed\n");
    ret = __fpe_flt_rounds();
    ok(ret == 1, "__fpe_flt_rounds returned %d\n", ret);

    ok((_controlfp(_RC_UP, _RC_CHOP) & _RC_CHOP) == _RC_UP, "_controlfp(_RC_UP, _RC_CHOP) failed\n");
    ret = __fpe_flt_rounds();
    ok(ret == 2 || broken(ret == 3) /* w1064v1507 */, "__fpe_flt_rounds returned %d\n", ret);

    ok((_controlfp(_RC_DOWN, _RC_CHOP) & _RC_CHOP) == _RC_DOWN, "_controlfp(_RC_DOWN, _RC_CHOP) failed\n");
    ret = __fpe_flt_rounds();
    ok(ret == 3 || broken(ret == 2) /* w1064v1507 */, "__fpe_flt_rounds returned %d\n", ret);

    ok((_controlfp(_RC_CHOP, _RC_CHOP) & _RC_CHOP) == _RC_CHOP, "_controlfp(_RC_CHOP, _RC_CHOP) failed\n");
    ret = __fpe_flt_rounds();
    ok(ret == 0, "__fpe_flt_rounds returned %d\n", ret);

    _controlfp(cfp, _MCW_EM | _MCW_RC | _MCW_PC);
}

static void test__control87_2(void)
{
#ifdef __i386__
    unsigned int x86_cw_init, sse2_cw_init, x86_cw, sse2_cw, r;

    r = __control87_2(0, 0, &x86_cw_init, &sse2_cw_init);
    ok(r == 1, "__control87_2 returned %d\n", r);

    r = __control87_2(0, _EM_INVALID, &x86_cw, NULL);
    ok(r == 1, "__control87_2 returned %d\n", r);
    ok(x86_cw == (x86_cw_init & ~_EM_INVALID), "x86_cw = %x, x86_cw_init = %x\n", x86_cw, x86_cw_init);

    r = __control87_2(0, 0, &x86_cw, &sse2_cw);
    ok(r == 1, "__control87_2 returned %d\n", r);
    ok(x86_cw == (x86_cw_init & ~_EM_INVALID), "x86_cw = %x, x86_cw_init = %x\n", x86_cw, x86_cw_init);
    ok(sse2_cw == sse2_cw_init, "sse2_cw = %x, sse2_cw_init = %x\n", sse2_cw, sse2_cw_init);

    r = _control87(0, 0);
    ok(r == (x86_cw | sse2_cw | _EM_AMBIGUOUS), "r = %x, expected %x\n",
            r, x86_cw | sse2_cw | _EM_AMBIGUOUS);

    _control87(x86_cw_init, ~0);
#endif
}

static void __cdecl global_invalid_parameter_handler(
        const wchar_t *expression, const wchar_t *function,
        const wchar_t *file, unsigned line, uintptr_t arg)
{
    CHECK_EXPECT2(global_invalid_parameter_handler);
}

static void __cdecl thread_invalid_parameter_handler(
        const wchar_t *expression, const wchar_t *function,
        const wchar_t *file, unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(thread_invalid_parameter_handler);
}

static void test_invalid_parameter_handler(void)
{
    _invalid_parameter_handler ret;

    ret = _get_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = _get_thread_local_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = _set_thread_local_invalid_parameter_handler(thread_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");

    ret = _get_thread_local_invalid_parameter_handler();
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    ret = _get_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = _set_invalid_parameter_handler(global_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");

    ret = _get_invalid_parameter_handler();
    ok(ret == global_invalid_parameter_handler, "ret = %p\n", ret);

    ret = _get_thread_local_invalid_parameter_handler();
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    SET_EXPECT(thread_invalid_parameter_handler);
    _ltoa_s(0, NULL, 0, 0);
    CHECK_CALLED(thread_invalid_parameter_handler);

    ret = _set_thread_local_invalid_parameter_handler(NULL);
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    SET_EXPECT(global_invalid_parameter_handler);
    _ltoa_s(0, NULL, 0, 0);
    CHECK_CALLED(global_invalid_parameter_handler);

    ret = _set_invalid_parameter_handler(NULL);
    ok(ret == global_invalid_parameter_handler, "ret = %p\n", ret);

    ret = _set_invalid_parameter_handler(global_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");
}

static void test__get_narrow_winmain_command_line(char *path)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup;
    char cmd[MAX_PATH+32];
    char *ret, *cmdline, *name;
    int len;

    ret = _get_narrow_winmain_command_line();
    cmdline = GetCommandLineA();
    len = strlen(cmdline);
    ok(ret>cmdline && ret<cmdline+len, "ret = %p, cmdline = %p (len = %d)\n", ret, cmdline, len);

    if(!path) {
        ok(!lstrcmpA(ret, "\"misc\" cmd"), "ret = %s\n", ret);
        return;
    }

    for(len = strlen(path); len>0; len--)
        if(path[len-1]=='\\' || path[len-1]=='/') break;
    if(len) name = path+len;
    else name = path;

    sprintf(cmd, "\"\"%c\"\"\"%s\" \t \"misc\" cmd", name[0], name+1);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(path, cmd, NULL, NULL, TRUE,
            CREATE_DEFAULT_ERROR_MODE|NORMAL_PRIORITY_CLASS,
            NULL, NULL, &startup, &proc);
    wait_child_process(proc.hProcess);
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
}

static void test__sopen_dispatch(void)
{
    int ret, fd;
    char *tempf;

    tempf = _tempnam(".", "wne");

    fd = 0;
    ret = _sopen_dispatch(tempf, _O_CREAT, _SH_DENYWR, 0xff, &fd, 0);
    ok(!ret, "got %d\n", ret);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = _sopen_dispatch(tempf, _O_CREAT, _SH_DENYWR, 0xff, &fd, 1);
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
    if (fd > 0)
    {
        _close(fd);
        unlink(tempf);
    }

    free(tempf);
}

static void test__sopen_s(void)
{
    int ret, fd;
    char *tempf;

    tempf = _tempnam(".", "wne");

    fd = 0;
    ret = _sopen_s(&fd, tempf, _O_CREAT, _SH_DENYWR, 0);
    ok(!ret, "got %d\n", ret);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    /* _open() does not validate pmode */
    fd = _open(tempf, _O_CREAT, 0xff);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    /* _sopen_s() invokes invalid parameter handler on invalid pmode */
    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = _sopen_s(&fd, tempf, _O_CREAT, _SH_DENYWR, 0xff);
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);

    free(tempf);
}

static void test_lldiv(void)
{
    lldiv_t r;

    r = lldiv(((LONGLONG)0x111 << 32) + 0x222, (LONGLONG)1 << 32);
    ok(r.quot == 0x111, "quot = %s\n", wine_dbgstr_longlong(r.quot));
    ok(r.rem == 0x222, "rem = %s\n", wine_dbgstr_longlong(r.rem));

    r = lldiv(((LONGLONG)0x69CF0012 << 32) + 0x0033E78A, 0x30);
    ok(r.quot == ((LONGLONG)0x02345000 << 32) + 0x600114D2, "quot = %s\n", wine_dbgstr_longlong(r.quot));
    ok(r.rem == 0x2A, "rem = %s\n", wine_dbgstr_longlong(r.rem));

    r = lldiv(((LONGLONG)0x243A5678 << 32) + 0x9ABCDEF0, (LONGLONG)0x12 << 48);
    ok(r.quot == 0x0203, "quot = %s\n", wine_dbgstr_longlong(r.quot));
    ok(r.rem == ((LONGLONG)0x00045678 << 32) + 0x9ABCDEF0, "rem = %s\n", wine_dbgstr_longlong(r.rem));
}

static void test_isblank(void)
{
    int c, r;

    for(c = 0; c <= 0xff; c++) {
        if(c == '\t') {
            ok(!_isctype(c, _BLANK), "tab shouldn't be blank\n");
            ok(isblank(c), "%d should be blank\n", c);
            r = _isblank_l(c, NULL);
            ok(!r || broken(r == _BLANK), "tab shouldn't be blank (got %x)\n", r);
        } else if(c == ' ') {
            ok(_isctype(c, _BLANK), "space should be blank\n");
            ok(isblank(c), "%d should be blank\n", c);
            r = _isblank_l(c, NULL);
            ok(r == _BLANK, "space should be blank (got %x)\n", r);
        } else {
            ok(!_isctype(c, _BLANK), "%d shouldn't be blank\n", c);
            ok(!isblank(c), "%d shouldn't be blank\n", c);
            ok(!_isblank_l(c, NULL), "%d shouldn't be blank\n", c);
        }
    }

    for(c = 0; c <= 0xffff; c++) {
        if(c == '\t' || c == ' ' || c == 0x3000 || c == 0xfeff) {
            if(c == '\t')
                ok(!_iswctype_l(c, _BLANK, NULL), "tab shouldn't be blank\n");
            else
                ok(_iswctype_l(c, _BLANK, NULL), "%d should be blank\n", c);
            ok(iswblank(c), "%d should be blank\n", c);
            ok(_iswblank_l(c, NULL), "%d should be blank\n", c);
        } else {
            ok(!_iswctype_l(c, _BLANK, NULL), "%d shouldn't be blank\n", c);
            ok(!iswblank(c), "%d shouldn't be blank\n", c);
            ok(!_iswblank_l(c, NULL), "%d shouldn't be blank\n", c);
        }
    }
}

static struct _exception exception;

static int CDECL matherr_callback(struct _exception *e)
{
    exception = *e;

    if (!strcmp(e->name, "acos") && e->arg1 == 2)
        e->retval = -1;
    return 0;
}

static void test_math_errors(void)
{
    const struct {
        char func[16];
        double x;
        int error;
        int exception;
    } testsd[] = {
        {"_logb", -INFINITY, -1, -1},
        {"_logb", -1, -1, -1},
        {"_logb", 0, ERANGE, _SING},
        {"_logb", INFINITY, -1, -1},
        {"acos", -INFINITY, EDOM, _DOMAIN},
        {"acos", -2, EDOM, _DOMAIN},
        {"acos", -1, -1, -1},
        {"acos", 1, -1, -1},
        {"acos", 2, EDOM, _DOMAIN},
        {"acos", INFINITY, EDOM, _DOMAIN},
        {"acosh", -INFINITY, EDOM, -1},
        {"acosh", 0, EDOM, -1},
        {"acosh", 1, -1, -1},
        {"acosh", INFINITY, -1, -1},
        {"asin", -INFINITY, EDOM, _DOMAIN},
        {"asin", -2, EDOM, _DOMAIN},
        {"asin", -1, -1, -1},
        {"asin", 1, -1, -1},
        {"asin", 2, EDOM, _DOMAIN},
        {"asin", INFINITY, EDOM, _DOMAIN},
        {"asinh", -INFINITY, -1, -1},
        {"asinh", INFINITY, -1, -1},
        {"atan", -INFINITY, -1, -1},
        {"atan", 0, -1, -1},
        {"atan", INFINITY, -1, -1},
        {"atanh", -INFINITY, EDOM, -1},
        {"atanh", -2, EDOM, -1},
        {"atanh", -1, ERANGE, -1},
        {"atanh", 1, ERANGE, -1},
        {"atanh", 2, EDOM, -1},
        {"atanh", INFINITY, EDOM, -1},
        {"cos", -INFINITY, EDOM, _DOMAIN},
        {"cos", INFINITY, EDOM, _DOMAIN},
        {"cosh", -INFINITY, -1, -1},
        {"cosh", 0, -1, -1},
        {"cosh", INFINITY, -1, -1},
        {"exp", -INFINITY, -1, -1},
        {"exp", -1e100, -1, _UNDERFLOW},
        {"exp", 1e100, ERANGE, _OVERFLOW},
        {"exp", INFINITY, -1, -1},
        {"exp2", -INFINITY, -1, -1},
        {"exp2", -1e100, -1, -1},
        {"exp2", 1e100, ERANGE, -1},
        {"exp2", INFINITY, -1, -1},
        {"expm1", -INFINITY, -1, -1},
        {"expm1", INFINITY, -1, -1},
        {"log", -INFINITY, EDOM, _DOMAIN},
        {"log", -1, EDOM, _DOMAIN},
        {"log", 0, ERANGE, _SING},
        {"log", INFINITY, -1, -1},
        {"log10", -INFINITY, EDOM, _DOMAIN},
        {"log10", -1, EDOM, _DOMAIN},
        {"log10", 0, ERANGE, _SING},
        {"log10", INFINITY, -1, -1},
        {"log1p", -INFINITY, EDOM, -1},
        {"log1p", -2, EDOM, -1},
        {"log1p", -1, ERANGE, -1},
        {"log1p", INFINITY, -1, -1},
        {"log2", INFINITY, -1, -1},
        {"sin", -INFINITY, EDOM, _DOMAIN},
        {"sin", INFINITY, EDOM, _DOMAIN},
        {"sinh", -INFINITY, -1, -1},
        {"sinh", 0, -1, -1},
        {"sinh", INFINITY, -1, -1},
        {"sqrt", -INFINITY, EDOM, _DOMAIN},
        {"sqrt", -1, EDOM, _DOMAIN},
        {"sqrt", 0, -1, -1},
        {"sqrt", INFINITY, -1, -1},
        {"tan", -INFINITY, EDOM, _DOMAIN},
        {"tan", -M_PI_2, -1, -1},
        {"tan", M_PI_2, -1, -1},
        {"tan", INFINITY, EDOM, _DOMAIN},
        {"tanh", -INFINITY, -1, -1},
        {"tanh", 0, -1, -1},
        {"tanh", INFINITY, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        double b;
        int error;
        int exception;
    } tests2d[] = {
        {"atan2", -INFINITY, 0, -1, -1},
        {"atan2", 0, 0, -1, -1},
        {"atan2", INFINITY, 0, -1, -1},
        {"atan2", 0, -INFINITY, -1, -1},
        {"atan2", 0, INFINITY, -1, -1},
        {"pow", -INFINITY, -2, -1, -1},
        {"pow", -INFINITY, -1, -1, -1},
        {"pow", -INFINITY, 0, -1, -1},
        {"pow", -INFINITY, 1, -1, -1},
        {"pow", -INFINITY, 2, -1, -1},
        {"pow", -1e100, -10, -1, _UNDERFLOW},
        {"pow", -1e100, 10, ERANGE, _OVERFLOW},
        {"pow", -1, 1.5, EDOM, _DOMAIN},
        {"pow", 0, -2, ERANGE, _SING},
        {"pow", 0, -1, ERANGE, _SING},
        {"pow", 0.5, -INFINITY, -1, -1},
        {"pow", 0.5, INFINITY, -1, -1},
        {"pow", 2, -INFINITY, -1, -1},
        {"pow", 2, -1e100, -1, _UNDERFLOW},
        {"pow", 2, 1e100, ERANGE, _OVERFLOW},
        {"pow", 2, INFINITY, -1, -1},
        {"pow", 1e100, -10, -1, _UNDERFLOW},
        {"pow", 1e100, 10, ERANGE, _OVERFLOW},
        {"pow", INFINITY, -2, -1, -1},
        {"pow", INFINITY, -1, -1, -1},
        {"pow", INFINITY, 0, -1, -1},
        {"pow", INFINITY, 1, -1, -1},
        {"pow", INFINITY, 2, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        double b;
        double c;
        int error;
        int exception;
    } tests3d[] = {
        /* 0 * inf --> EDOM */
        {"fma", INFINITY, 0, 0, EDOM, -1},
        {"fma", 0, INFINITY, 0, EDOM, -1},
        /* inf - inf -> EDOM */
        {"fma", INFINITY, 1, -INFINITY, EDOM, -1},
        {"fma", -INFINITY, 1, INFINITY, EDOM, -1},
        {"fma", 1, INFINITY, -INFINITY, EDOM, -1},
        {"fma", 1, -INFINITY, INFINITY, EDOM, -1},
        /* NaN */
        {"fma", NAN, 0, 0, -1, -1},
        {"fma", 0, NAN, 0, -1, -1},
        {"fma", 0, 0, NAN, -1, -1},
        /* over/underflow */
        {"fma", __port_max_double(), __port_max_double(), __port_max_double(), -1, -1},
        {"fma", __port_min_pos_double(), __port_min_pos_double(), 1, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        long b;
        int error;
        int exception;
    } testsdl[] = {
        {"_scalb", -INFINITY, 1, -1, -1},
        {"_scalb", -1e100, 1, -1, -1},
        {"_scalb", 0, 1, -1, -1},
        {"_scalb", 1e100, 1, -1, -1},
        {"_scalb", INFINITY, 1, -1, -1},
        {"_scalb", 1, 1e9, ERANGE, _OVERFLOW},
        {"ldexp", -INFINITY, 1, -1, -1},
        {"ldexp", -1e100, 1, -1, -1},
        {"ldexp", 0, 1, -1, -1},
        {"ldexp", 1e100, 1, -1, -1},
        {"ldexp", INFINITY, 1, -1, -1},
        {"ldexp", 1, -1e9, -1, _UNDERFLOW},
        {"ldexp", 1, 1e9, ERANGE, _OVERFLOW},
    };
    double (CDECL *p_funcd)(double);
    double (CDECL *p_func2d)(double, double);
    double (CDECL *p_func3d)(double, double, double);
    double (CDECL *p_funcdl)(double, long);
    HMODULE module;
    double d;
    int i;

    __setusermatherr(matherr_callback);
    module = GetModuleHandleW(L"ucrtbase.dll");

    /* necessary so that exp(1e100)==INFINITY on glibc, we can remove this if we change our implementation */
    fesetround(FE_TONEAREST);

    for(i = 0; i < ARRAY_SIZE(testsd); i++) {
        p_funcd = (void*)GetProcAddress(module, testsd[i].func);
        errno = -1;
        exception.type = -1;
        p_funcd(testsd[i].x);
        ok(errno == testsd[i].error,
           "%s(%f) got errno %d\n", testsd[i].func, testsd[i].x, errno);
        ok(exception.type == testsd[i].exception,
           "%s(%f) got exception type %d\n", testsd[i].func, testsd[i].x, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == testsd[i].x,
           "%s(%f) got exception arg1 %f\n", testsd[i].func, testsd[i].x, exception.arg1);
    }

    for(i = 0; i < ARRAY_SIZE(tests2d); i++) {
        p_func2d = (void*)GetProcAddress(module, tests2d[i].func);
        errno = -1;
        exception.type = -1;
        p_func2d(tests2d[i].a, tests2d[i].b);
        ok(errno == tests2d[i].error,
           "%s(%f, %f) got errno %d\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, errno);
        ok(exception.type == tests2d[i].exception,
           "%s(%f, %f) got exception type %d\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == tests2d[i].a,
           "%s(%f, %f) got exception arg1 %f\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.arg1);
        ok(exception.arg2 == tests2d[i].b,
           "%s(%f, %f) got exception arg2 %f\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.arg2);
    }

    for(i = 0; i < ARRAY_SIZE(tests3d); i++) {
        p_func3d = (void*)GetProcAddress(module, tests3d[i].func);
        errno = -1;
        exception.type = -1;
        p_func3d(tests3d[i].a, tests3d[i].b, tests3d[i].c);
        ok(errno == tests3d[i].error || errno == -1, /* native is not setting errno if FMA3 is supported */
           "%s(%f, %f, %f) got errno %d\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, errno);
        ok(exception.type == tests3d[i].exception,
           "%s(%f, %f, %f) got exception type %d\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == tests3d[i].a,
           "%s(%f, %f, %f) got exception arg1 %f\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.arg1);
        ok(exception.arg2 == tests3d[i].b,
           "%s(%f, %f, %f) got exception arg2 %f\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.arg2);
    }

    for(i = 0; i < ARRAY_SIZE(testsdl); i++) {
        p_funcdl = (void*)GetProcAddress(module, testsdl[i].func);
        errno = -1;
        exception.type = -1;
        p_funcdl(testsdl[i].a, testsdl[i].b);
        ok(errno == testsdl[i].error,
           "%s(%f, %ld) got errno %d\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, errno);
        ok(exception.type == testsdl[i].exception,
           "%s(%f, %ld) got exception type %d\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == testsdl[i].a,
           "%s(%f, %ld) got exception arg1 %f\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.arg1);
        ok(exception.arg2 == testsdl[i].b,
           "%s(%f, %ld) got exception arg2 %f\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.arg2);
    }

    d = acos(2.0);
    ok(d == -1.0, "failed to change log10 return value: %e\n", d);
}

static void test_asctime(void)
{
    const struct tm epoch = { 0, 0, 0, 1, 0, 70, 4, 0, 0 };
    char *ret;

    ret = asctime(&epoch);
    ok(!strcmp(ret, "Thu Jan  1 00:00:00 1970\n"), "asctime returned %s\n", ret);
}

static void test_strftime(void)
{
    const struct {
       const char *format;
       const char *ret;
       struct tm tm;
       BOOL todo_value;
       BOOL todo_handler;
    } tests[] = {
        {"%C", "", { 0, 0, 0, 1, 0, -2000, 4, 0, 0 }},
        {"%C", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%C", "00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%C", "18", { 0, 0, 0, 1, 0, -1, 4, 0, 0 }},
        {"%C", "19", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%C", "99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%C", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%d", "", { 0, 0, 0, 0, 0, 70, 4, 0, 0 }},
        {"%d", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%d", "31", { 0, 0, 0, 31, 0, 70, 4, 0, 0 }},
        {"%d", "", { 0, 0, 0, 32, 0, 70, 4, 0, 0 }},
        {"%D", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%D", "01/01/00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%D", "01/01/99", { 0, 0, 0, 1, 0, -1, 4, 0, 0 }},
        {"%D", "01/01/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%D", "01/01/99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%D", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%#D", "1/1/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%e", "", { 0, 0, 0, 0, 0, 70, 4, 0, 0 }},
        {"%e", " 1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%e", "31", { 0, 0, 0, 31, 0, 70, 4, 0, 0 }},
        {"%e", "", { 0, 0, 0, 32, 0, 70, 4, 0, 0 }},
        {"%#e", "1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%F", "1970-01-01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#F", "1970-1-1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%R", "00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#R", "0:0", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%T", "00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#T", "0:0:0", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%u", "", { 0, 0, 0, 1, 0, 117, -1, 0, 0 }},
        {"%u", "7", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%u", "1", { 0, 0, 0, 1, 0, 117, 1, 0, 0 }},
        {"%u", "6", { 0, 0, 0, 1, 0, 117, 6, 0, 0 }},
        {"%u", "", { 0, 0, 0, 1, 0, 117, 7, 0, 0 }},
        {"%h", "Jan", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%I", "", { 0, 0, -1, 1, 0, 70, 4, 0, 0 }},
        {"%I", "12", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%I", "01", { 0, 0, 1, 1, 0, 70, 4, 0, 0 }},
        {"%I", "11", { 0, 0, 11, 1, 0, 70, 4, 0, 0 }},
        {"%I", "12", { 0, 0, 12, 1, 0, 70, 4, 0, 0 }},
        {"%I", "01", { 0, 0, 13, 1, 0, 70, 4, 0, 0 }},
        {"%I", "11", { 0, 0, 23, 1, 0, 70, 4, 0, 0 }},
        {"%I", "", { 0, 0, 24, 1, 0, 70, 4, 0, 0 }},
        {"%n", "\n", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%r", "12:00:00 AM", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%r", "02:00:00 PM", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%#r", "12:0:0 AM", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#r", "2:0:0 PM", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%t", "\t", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, -1901, 3, 364, 0 }},
        {"%g", "00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%g", "70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%g", "71", { 0, 0, 0, 2, 0, 72, 0, 1, 0 }},
        {"%g", "72", { 0, 0, 0, 3, 0, 72, 1, 2, 0 }},
        {"%g", "16", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%g", "99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%g", "00", { 0, 0, 0, 1, 0, 8099, 3, 364, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, 8100, 0, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%G", "1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%G", "1971", { 0, 0, 0, 2, 0, 72, 0, 1, 0 }},
        {"%G", "1972", { 0, 0, 0, 3, 0, 72, 1, 2, 0 }},
        {"%G", "2016", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%V", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%V", "52", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%V", "53", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%y", "", { 0, 0, 0, 0, 0, -1901, 0, 0, 0 }},
        {"%y", "00", { 0, 0, 0, 0, 0, -1900, 0, 0, 0 }},
        {"%y", "99", { 0, 0, 0, 0, 0, 8099, 0, 0, 0 }},
        {"%y", "", { 0, 0, 0, 0, 0, 8100, 0, 0, 0 }},
        {"%c", "Thu Jan  1 00:00:00 1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%c", "Thu Feb 30 00:00:00 1970", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#c", "Thursday, January 01, 1970 00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#c", "Thursday, February 30, 1970 00:00:00", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%x", "01/01/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%x", "02/30/70", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#x", "Thursday, January 01, 1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#x", "Thursday, February 30, 1970", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#x", "", { 0, 0, 0, 30, 1, 70, 7, 0, 0 }},
        {"%#x", "", { 0, 0, 0, 30, 12, 70, 4, 0, 0 }},
        {"%X", "00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%X", "14:00:00", { 0, 0, 14, 1, 0, 70, 4, 0, 0 }},
        {"%X", "23:59:60", { 60, 59, 23, 1, 0, 70, 4, 0, 0 }},
    };

    const struct {
        const char *format;
        const char *ret;
        const wchar_t *short_date;
        const wchar_t *date;
        const wchar_t *time;
        struct tm tm;
        BOOL todo;
    } tests_td[] = {
        { "%c", "x z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#c", "y z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "M1", 0, 0, L"MMM", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, L"h", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, L"hh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "h01", 0, 0, L"hhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "hh01", 0, 0, L"hhhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, L"H", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, L"HH", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "H13", 0, 0, L"HHH", { 0, 0, 13, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, L"m", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, L"mm", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "m00", 0, 0, L"mmm", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, L"s", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, L"ss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "s00", 0, 0, L"sss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "T", 0, 0, L"t", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"tt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"ttttttttt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"a", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"aaaaa", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"A", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"AAAAA", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", L"d", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", L"dd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "D1", L"ddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "Day1", L"dddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "dDay1", L"ddddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", L"M", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", L"MM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "M1", L"MMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "Mon1", L"MMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "MMon1", L"MMMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y", L"y", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "70", L"yy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y70", L"yyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1970", L"yyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y1970", L"yyyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "ggggggggggg", L"ggggggggggg", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, L"d", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, L"dd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "D1", 0, L"ddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "Day1", 0, L"dddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "dDay1", 0, L"ddddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, L"M", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, L"MM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "M1", 0, L"MMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "Mon1", 0, L"MMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "MMon1", 0, L"MMMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y", 0, L"y", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "70", 0, L"yy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y70", 0, L"yyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1970", 0, L"yyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y1970", 0, L"yyyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%r", "z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
    };

    const struct {
        int year;
        int yday;
        const char *ret[7];
    } tests_yweek[] = {
        { 100, 0, { "99 52", "00 01", "00 01", "00 01", "00 01", "99 53", "99 52" }},
        { 100, 1, { "99 52", "00 01", "00 01", "00 01", "00 01", "00 01", "99 53" }},
        { 100, 2, { "99 53", "00 01", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 3, { "00 01", "00 01", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 4, { "00 01", "00 02", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 5, { "00 01", "00 02", "00 02", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 6, { "00 01", "00 02", "00 02", "00 02", "00 01", "00 01", "00 01" }},
        { 100, 358, { "00 51", "00 52", "00 52", "00 52", "00 52", "00 52", "00 51" }},
        { 100, 359, { "00 51", "00 52", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 360, { "00 52", "00 52", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 361, { "00 52", "00 53", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 362, { "00 52", "00 53", "00 53", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 363, { "00 52", "01 01", "00 53", "00 53", "00 52", "00 52", "00 52" }},
        { 100, 364, { "00 52", "01 01", "01 01", "00 53", "00 53", "00 52", "00 52" }},
        { 100, 365, { "00 52", "01 01", "01 01", "01 01", "00 53", "00 53", "00 52" }},
        { 101, 0, { "00 52", "01 01", "01 01", "01 01", "01 01", "00 53", "00 53" }},
        { 101, 1, { "00 53", "01 01", "01 01", "01 01", "01 01", "01 01", "00 53" }},
        { 101, 2, { "00 53", "01 01", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 3, { "01 01", "01 01", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 4, { "01 01", "01 02", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 5, { "01 01", "01 02", "01 02", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 6, { "01 01", "01 02", "01 02", "01 02", "01 01", "01 01", "01 01" }},
        { 101, 358, { "01 51", "01 52", "01 52", "01 52", "01 52", "01 52", "01 51" }},
        { 101, 359, { "01 51", "01 52", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 360, { "01 52", "01 52", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 361, { "01 52", "01 53", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 362, { "01 52", "02 01", "01 53", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 363, { "01 52", "02 01", "02 01", "01 53", "01 52", "01 52", "01 52" }},
        { 101, 364, { "01 52", "02 01", "02 01", "02 01", "01 53", "01 52", "01 52" }},
    };

    __lc_time_data time_data = {
        { "d1", "d2", "d3", "d4", "d5", "d6", "d7" },
        { "day1", "day2", "day3", "day4", "day5", "day6", "day7" },
        { "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12" },
        { "mon1", "mon2", "mon3", "mon4", "mon5", "mon6", "mon7", "mon8", "mon9", "mon10", "mon11", "mon12" },
        "tam", "tpm", 0, 0, 0, 1, 0,
        { L"D1", L"D2", L"D3", L"D4", L"D5", L"D6", L"D7" },
        { L"Day1", L"Day2", L"Day3", L"Day4", L"Day5", L"Day6", L"Day7" },
        { L"M1", L"M2", L"M3", L"M4", L"M5", L"M6", L"M7", L"M8", L"M9", L"M10", L"M11", L"M12" },
        { L"Mon1", L"Mon2", L"Mon3", L"Mon4", L"Mon5", L"Mon6", L"Mon7", L"Mon8", L"Mon9", L"Mon10", L"Mon11", L"Mon12" },
        L"TAM", L"TPM"
    };

    const struct tm epoch = { 0, 0, 0, 1, 0, 70, 4, 0, 0 };
    struct tm tm_yweek = { 0, 0, 0, 1, 0, 70, 0, 0, 0 };
    char buf[256];
    int i, ret=0;

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
        todo_wine_if(tests[i].todo_handler) {
            if (!tests[i].ret[0])
                SET_EXPECT(global_invalid_parameter_handler);
            ret = strftime(buf, sizeof(buf), tests[i].format, &tests[i].tm);
            if (!tests[i].ret[0])
                CHECK_CALLED(global_invalid_parameter_handler);
        }

        todo_wine_if(tests[i].todo_value) {
            ok(ret == strlen(tests[i].ret), "%d) ret = %d\n", i, ret);
            ok(!strcmp(buf, tests[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, buf, tests[i].ret);
        }
    }

    ret = strftime(buf, sizeof(buf), "%z", &epoch);
    ok(ret == 5, "expected 5, got %d\n", ret);
    ok((buf[0] == '+' || buf[0] == '-') &&
        isdigit(buf[1]) && isdigit(buf[2]) &&
        isdigit(buf[3]) && isdigit(buf[4]), "got %s\n", buf);

    for (i=0; i<ARRAY_SIZE(tests_td); i++)
    {
        time_data.short_dateW = tests_td[i].short_date;
        time_data.dateW = tests_td[i].date;
        time_data.timeW = tests_td[i].time;
        ret = _Strftime(buf, sizeof(buf), tests_td[i].format, &tests_td[i].tm, &time_data);
        ok(ret == strlen(buf), "%d) ret = %d\n", i, ret);
        todo_wine_if(tests_td[i].todo) {
            ok(!strcmp(buf, tests_td[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, buf, tests_td[i].ret);
        }
    }

    for (i=0; i<ARRAY_SIZE(tests_yweek); i++)
    {
        int j;
        tm_yweek.tm_year = tests_yweek[i].year;
        tm_yweek.tm_yday = tests_yweek[i].yday;
        for (j=0; j<7; j++)
        {
            tm_yweek.tm_wday = j;
            strftime(buf, sizeof(buf), "%g %V", &tm_yweek);
            ok(!strcmp(buf, tests_yweek[i].ret[j]), "%d,%d) buf = \"%s\", expected \"%s\"\n",
                    i, j, buf, tests_yweek[i].ret[j]);
        }
    }

    if(!setlocale(LC_ALL, "fr-FR")) {
        win_skip("fr-FR locale not available\n");
        return;
    }
    ret = strftime(buf, sizeof(buf), "%c", &epoch);
    ok(ret == 19, "ret = %d\n", ret);
    ok(!strcmp(buf, "01/01/1970 00:00:00"), "buf = \"%s\", expected \"%s\"\n", buf, "01/01/1970 00:00:00");
    ret = strftime(buf, sizeof(buf), "%r", &epoch);
    ok(ret == 8, "ret = %d\n", ret);
    ok(!strcmp(buf, "00:00:00"), "buf = \"%s\", expected \"%s\"\n", buf, "00:00:00");
    setlocale(LC_ALL, "C");

    if(!setlocale(LC_TIME, "Japanese_Japan.932")) {
        win_skip("Japanese_Japan.932 locale not available\n");
        return;
    }
    ret = strftime(buf, sizeof(buf), "%a", &epoch);
    ok(ret == 2 || broken(ret == 1), "ret = %d\n", ret);
    ok(!strcmp(buf, "\x96\xd8"), "buf = %s, expected \"\\x96\\xd8\"\n", wine_dbgstr_an(buf, 2));
    setlocale(LC_ALL, "C");
}

static LONG* get_failures_counter(HANDLE *map)
{
    *map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, sizeof(LONG), "winetest_failures_counter");
    return MapViewOfFile(*map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LONG));
}

static void free_failures_counter(LONG *mem, HANDLE map)
{
    UnmapViewOfFile(mem);
    CloseHandle(map);
}

static void set_failures_counter(LONG add)
{
    HANDLE failures_map;
    LONG *failures;

    failures = get_failures_counter(&failures_map);
    *failures = add;
    free_failures_counter(failures, failures_map);
}

static void test_exit(const char *argv0)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup = {0};
    char path[MAX_PATH];
    HANDLE failures_map, exit_event, quick_exit_event;
    LONG *failures;
    DWORD ret;

    exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");
    quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    failures = get_failures_counter(&failures_map);
    sprintf(path, "%s misc exit", argv0);
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &proc);
    ret = WaitForSingleObject(proc.hProcess, INFINITE);
    ok(ret == WAIT_OBJECT_0, "child process wait failed\n");
    GetExitCodeProcess(proc.hProcess, &ret);
    ok(ret == 1, "child process exited with code %ld\n", ret);
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
    ok(!*failures, "%ld tests failed in child process\n", *failures);
    free_failures_counter(failures, failures_map);


    ret = WaitForSingleObject(exit_event, 0);
    ok(ret == WAIT_OBJECT_0, "exit_event was not set (%lx)\n", ret);
    ret = WaitForSingleObject(quick_exit_event, 0);
    ok(ret == WAIT_TIMEOUT, "quick_exit_event should not have be set (%lx)\n", ret);

    CloseHandle(exit_event);
    CloseHandle(quick_exit_event);
}

static int atexit_called;

static void CDECL at_exit_func1(void)
{
    HANDLE exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");

    ok(exit_event != NULL, "CreateEvent failed: %ld\n", GetLastError());
    ok(atexit_called == 1, "atexit_called = %d\n", atexit_called);
    atexit_called++;
    SetEvent(exit_event);
    CloseHandle(exit_event);
    set_failures_counter(winetest_get_failures());
}

static void CDECL at_exit_func2(void)
{
    ok(!atexit_called, "atexit_called = %d\n", atexit_called);
    atexit_called++;
    set_failures_counter(winetest_get_failures());
}

static int atquick_exit_called;

static void CDECL at_quick_exit_func1(void)
{
    HANDLE quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    ok(quick_exit_event != NULL, "CreateEvent failed: %ld\n", GetLastError());
    ok(atquick_exit_called == 1, "atquick_exit_called = %d\n", atquick_exit_called);
    atquick_exit_called++;
    SetEvent(quick_exit_event);
    CloseHandle(quick_exit_event);
    set_failures_counter(winetest_get_failures());
}

static void CDECL at_quick_exit_func2(void)
{
    ok(!atquick_exit_called, "atquick_exit_called = %d\n", atquick_exit_called);
    atquick_exit_called++;
    set_failures_counter(winetest_get_failures());
}

static void test_call_exit(void)
{
    ok(!_crt_atexit(at_exit_func1), "_crt_atexit failed\n");
    ok(!_crt_atexit(at_exit_func2), "_crt_atexit failed\n");

    ok(!_crt_at_quick_exit(at_quick_exit_func1), "_crt_at_quick_exit failed\n");
    ok(!_crt_at_quick_exit(at_quick_exit_func2), "_crt_at_quick_exit failed\n");

    set_failures_counter(winetest_get_failures());
    exit(1);
}

static void test_call_quick_exit(void)
{
    ok(!_crt_atexit(at_exit_func1), "_crt_atexit failed\n");
    ok(!_crt_atexit(at_exit_func2), "_crt_atexit failed\n");

    ok(!_crt_at_quick_exit(at_quick_exit_func1), "_crt_at_quick_exit failed\n");
    ok(!_crt_at_quick_exit(at_quick_exit_func2), "_crt_at_quick_exit failed\n");

    set_failures_counter(winetest_get_failures());
    quick_exit(2);
}

static void test_quick_exit(const char *argv0)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup = {0};
    char path[MAX_PATH];
    HANDLE failures_map, exit_event, quick_exit_event;
    LONG *failures;
    DWORD ret;

    exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");
    quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    failures = get_failures_counter(&failures_map);
    sprintf(path, "%s misc quick_exit", argv0);
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &proc);
    ret = WaitForSingleObject(proc.hProcess, INFINITE);
    ok(ret == WAIT_OBJECT_0, "child process wait failed\n");
    GetExitCodeProcess(proc.hProcess, &ret);
    ok(ret == 2, "child process exited with code %ld\n", ret);
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
    ok(!*failures, "%ld tests failed in child process\n", *failures);
    free_failures_counter(failures, failures_map);

    ret = WaitForSingleObject(quick_exit_event, 0);
    ok(ret == WAIT_OBJECT_0, "quick_exit_event was not set (%lx)\n", ret);
    ret = WaitForSingleObject(exit_event, 0);
    ok(ret == WAIT_TIMEOUT, "exit_event should not have be set (%lx)\n", ret);

    CloseHandle(exit_event);
    CloseHandle(quick_exit_event);
}

static void test__stat32(void)
{
    static const char test_file[] = "\\stat_file.tst";
    static const char test_dir[] = "\\stat_dir.tst";

    char path[2*MAX_PATH];
    struct _stat32 buf;
    int fd, ret;
    DWORD len;

    len = GetTempPathA(MAX_PATH, path);
    ok(len, "GetTempPathA failed\n");

    ret = _stat32("c:", &buf);
    ok(ret == -1, "_stat32('c:') returned %d\n", ret);
    ret = _stat32("c:\\", &buf);
    ok(!ret, "_stat32('c:\\') returned %d\n", ret);

    memcpy(path+len, test_file, sizeof(test_file));
    if((fd = open(path, O_WRONLY | O_CREAT | O_BINARY, _S_IREAD |_S_IWRITE)) >= 0)
    {
        ret = _stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        strcat(path, "\\");
        ret = _stat32(path, &buf);
        ok(ret, "_stat32('%s') returned %d\n", path, ret);
        close(fd);
        remove(path);
    }

    memcpy(path+len, test_dir, sizeof(test_dir));
    if(!mkdir(path))
    {
        ret = _stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        strcat(path, "\\");
        ret = _stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        rmdir(path);
    }
}

static void test__o_malloc(void)
{
    void *m;
    size_t s;

    m = _o_malloc(1);
    ok(m != NULL, "p__o_malloc(1) returned NULL\n");

    s = _msize(m);
    ok(s == 1, "_msize returned %d\n", (int)s);

    free(m);
}

static void test_clock(void)
{
    static const int thresh = 100, max_load_delay = 1000;
    int c, expect_min;
    FILETIME cur;

    GetSystemTimeAsFileTime(&cur);
    c = clock();

    expect_min = (((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime - crt_init_end) / 10000;
    ok(c >= expect_min - thresh && c < expect_min + max_load_delay, "clock() = %d, expected range [%d, %d]\n",
            c, expect_min - thresh, expect_min + max_load_delay);
}

static void __cdecl se_translator(unsigned int u, EXCEPTION_POINTERS *ep)
{
}

static void test_thread_storage(void)
{
    void **current_exception;
    void *processing_throw;

    _set_se_translator(se_translator);
    current_exception = __current_exception();
    processing_throw = __processing_throw();

    ok(current_exception+2 == processing_throw,
            "current_exception = %p, processing_throw = %p\n",
            current_exception, processing_throw);
    ok(current_exception[-2] == se_translator,
            "can't find se_translator in thread storage\n");
}

static unsigned long fenv_encode(unsigned int e)
{
    ok(!(e & ~FE_ALL_EXCEPT), "incorrect argument: %x\n", e);

#if defined(__i386__)
    return e<<24 | e<<16 | e;
#elif defined(__x86_64__)
    return e<<24 | e;
#else
    return e;
#endif
}

static void test_fenv(void)
{
    static const int tests[] = {
        0,
        FE_INEXACT,
        FE_UNDERFLOW,
        FE_OVERFLOW,
        FE_DIVBYZERO,
        FE_INVALID,
        FE_ALL_EXCEPT,
    };
    static const struct {
        fexcept_t except;
        unsigned int flag;
        unsigned int get;
        fexcept_t expect;
    } tests2[] = {
        /* except                   flag                     get             expect */
        { 0,                        0,                       0,              0 },
        { FE_ALL_EXCEPT,            FE_INEXACT,              0,              0 },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_ALL_EXCEPT,  FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_INEXACT,     FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_INEXACT,              FE_OVERFLOW,    0 },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           FE_ALL_EXCEPT,  FE_ALL_EXCEPT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           FE_INEXACT,     FE_INEXACT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           0,              0 },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           ~0,             FE_ALL_EXCEPT },
        { FE_ALL_EXCEPT,            FE_ALL_EXCEPT,           ~FE_ALL_EXCEPT, 0 },
        { FE_INEXACT,               FE_ALL_EXCEPT,           FE_ALL_EXCEPT,  FE_INEXACT },
        { FE_INEXACT,               FE_UNDERFLOW,            FE_ALL_EXCEPT,  0 },
        { FE_UNDERFLOW,             FE_INEXACT,              FE_ALL_EXCEPT,  0 },
        { FE_INEXACT|FE_UNDERFLOW,  FE_UNDERFLOW,            FE_ALL_EXCEPT,  FE_UNDERFLOW },
        { FE_UNDERFLOW,             FE_INEXACT|FE_UNDERFLOW, FE_ALL_EXCEPT,  FE_UNDERFLOW },
    };
    fenv_t env, env2;
    fexcept_t except;
    int i, ret, flags;

    _clearfp();

    ret = fegetenv(&env);
    ok(!ret, "fegetenv returned %x\n", ret);
#if defined(__i386__) || defined(__x86_64__)
    if (env._Fe_ctl >> 24 != (env._Fe_ctl & 0xff))
    {
        win_skip("fenv_t format not supported (too old ucrtbase)\n");
        return;
    }
#endif
    fesetround(FE_UPWARD);
    ok(!env._Fe_stat, "env._Fe_stat = %lx\n", env._Fe_stat);
    ret = fegetenv(&env2);
    ok(!ret, "fegetenv returned %x\n", ret);
    ok(env._Fe_ctl != env2._Fe_ctl, "fesetround didn't change _Fe_ctl (%lx).\n", env._Fe_ctl);
    ret = fesetenv(&env);
    ok(!ret, "fesetenv returned %x\n", ret);
    ret = fegetround();
    ok(ret == FE_TONEAREST, "Got unexpected round mode %#x.\n", ret);

    except = fenv_encode(FE_ALL_EXCEPT);
    ret = fesetexceptflag(&except, FE_INEXACT|FE_UNDERFLOW);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    ret = feclearexcept(~FE_ALL_EXCEPT);
    ok(!ret, "feclearexceptflag returned %x\n", ret);
    except = fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    /* no crash, but no-op */
    ret = fesetexceptflag(NULL, 0);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = fetestexcept(FE_ALL_EXCEPT);
    ok(except == (FE_INEXACT|FE_UNDERFLOW), "expected %x, got %lx\n", FE_INEXACT|FE_UNDERFLOW, except);

    /* zero clears all */
    except = 0;
    ret = fesetexceptflag(&except, FE_ALL_EXCEPT);
    ok(!ret, "fesetexceptflag returned %x\n", ret);
    except = fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);

    ret = fetestexcept(FE_ALL_EXCEPT);
    ok(!ret, "fetestexcept returned %x\n", ret);

    flags = 0;
    /* adding bits with flags */
    for(i=0; i<ARRAY_SIZE(tests); i++) {
        except = fenv_encode(FE_ALL_EXCEPT);
        ret = fesetexceptflag(&except, tests[i]);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = fetestexcept(tests[i]);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);

        flags |= tests[i];
        ret = fetestexcept(FE_ALL_EXCEPT);
        ok(ret == flags, "Test %d: expected %x, got %x\n", i, flags, ret);

        except = ~0;
        ret = fegetexceptflag(&except, ~0);
        ok(!ret, "Test %d: fegetexceptflag returned %x.\n", i, ret);
        ok(except == fenv_encode(flags),
                "Test %d: expected %lx, got %lx\n", i, fenv_encode(flags), except);

        except = ~0;
        ret = fegetexceptflag(&except, tests[i]);
        ok(!ret, "Test %d: fegetexceptflag returned %x.\n", i, ret);
        ok(except == fenv_encode(tests[i]),
                "Test %d: expected %lx, got %lx\n", i, fenv_encode(tests[i]), except);
    }

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        ret = feclearexcept(tests[i]);
        ok(!ret, "Test %d: feclearexceptflag returned %x\n", i, ret);

        flags &= ~tests[i];
        except = fetestexcept(tests[i]);
        ok(!except, "Test %d: expected %x, got %lx\n", i, flags, except);
    }

    except = fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);

    /* setting bits with except */
    for(i=0; i<ARRAY_SIZE(tests); i++) {
        except = fenv_encode(tests[i]);
        ret = fesetexceptflag(&except, FE_ALL_EXCEPT);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = fetestexcept(tests[i]);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);

        ret = fetestexcept(FE_ALL_EXCEPT);
        ok(ret == tests[i], "Test %d: expected %x, got %x\n", i, tests[i], ret);
    }

    for(i=0; i<ARRAY_SIZE(tests2); i++) {
        _clearfp();

        except = fenv_encode(tests2[i].except);
        ret = fesetexceptflag(&except, tests2[i].flag);
        ok(!ret, "Test %d: fesetexceptflag returned %x\n", i, ret);

        ret = fetestexcept(tests2[i].get);
        ok(ret == tests2[i].expect, "Test %d: expected %lx, got %x\n", i, tests2[i].expect, ret);
    }

    ret = feclearexcept(FE_ALL_EXCEPT);
    ok(!ret, "feclearexceptflag returned %x\n", ret);
    except = fetestexcept(FE_ALL_EXCEPT);
    ok(!except, "expected 0, got %lx\n", except);
}

static void test_fopen_exclusive( void )
{
    char path[MAX_PATH*2];
    DWORD len;
    FILE *fp;

    len = GetTempPathA(MAX_PATH, path);
    ok(len, "GetTempPathA failed\n");
    strcat(path, "\\fileexcl.tst");

    SET_EXPECT(global_invalid_parameter_handler);
    fp = fopen(path, "wx");
    if(called_global_invalid_parameter_handler)
    {
        win_skip("skipping fopen x mode tests.\n");
        return;
    }
    expect_global_invalid_parameter_handler = FALSE;
    ok(fp != NULL, "creating file with mode wx failed\n");
    fclose(fp);

    fp = fopen(path, "wx");
    ok(!fp, "overwrote existing file with mode wx\n");
    unlink(path);

    fp = fopen(path, "w+x");
    ok(fp != NULL, "creating file with mode w+x failed\n");
    fclose(fp);

    fp = fopen(path, "w+x");
    ok(!fp, "overwrote existing file with mode w+x\n");

    SET_EXPECT(global_invalid_parameter_handler);
    fp = fopen(path, "rx");
    CHECK_CALLED(global_invalid_parameter_handler);
    ok(!fp, "opening file with mode rx succeeded\n");
    unlink(path);

    SET_EXPECT(global_invalid_parameter_handler);
    fp = fopen(path, "xw");
    CHECK_CALLED(global_invalid_parameter_handler);
    ok(!fp, "creating file with mode xw succeeded\n");

    fp = fopen(path, "wbx");
    ok(fp != NULL, "creating file with mode wbx failed\n");
    fclose(fp);
    unlink(path);
}

#if defined(__i386__)
#include "pshpack1.h"
struct rewind_thunk {
    BYTE push_esp[4]; /* push [esp+0x4] */
    BYTE call_rewind; /* call */
    DWORD rewind_addr; /* relative addr of rewind */
    BYTE pop_eax; /* pop eax */
    BYTE ret; /* ret */
};
#include "poppack.h"

static FILE * (CDECL *test_rewind_wrapper)(FILE *fp);

static void test_rewind_i386_abi(void)
{
    FILE *fp_in, *fp_out;

    struct rewind_thunk *thunk = VirtualAlloc(NULL, sizeof(*thunk), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    thunk->push_esp[0] = 0xff;
    thunk->push_esp[1] = 0x74;
    thunk->push_esp[2] = 0x24;
    thunk->push_esp[3] = 0x04;

    thunk->call_rewind = 0xe8;
    thunk->rewind_addr = (BYTE *) rewind - (BYTE *) (&thunk->rewind_addr + 1);

    thunk->pop_eax = 0x58;
    thunk->ret = 0xc3;

    test_rewind_wrapper = (void *) thunk;

    fp_in = fopen("rewind_abi.tst", "wb");
    fp_out = test_rewind_wrapper(fp_in);
    ok(fp_in == fp_out, "rewind modified the first argument in the stack\n");

    fclose(fp_in);
    unlink("rewind_abi.tst");
}
#endif

static void test_gmtime64(void)
{
    struct tm *ptm, tm;
    __time64_t t;
    int ret;

    t = -1;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = _gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = _gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 23 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43200;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = _gmtime64(&t);
    ok(!!ptm, "got NULL.\n");
    ret = _gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 69 && tm.tm_hour == 12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = _gmtime32((__time32_t *)&t);
    ok(!!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = _gmtime32_s(&tm, (__time32_t *)&t);
    ok(!ret, "got %d.\n", ret);
    todo_wine_if(tm.tm_year == 69 && tm.tm_hour == 12)
    ok(tm.tm_year == 70 && tm.tm_hour == -12 && tm.tm_min == 0 && tm.tm_sec == 0, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = -43201;
    ptm = _gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = _gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
    ptm = _gmtime32((__time32_t *)&t);
    ok(!ptm, "got NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = _gmtime32_s(&tm, (__time32_t *)&t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 1605600;
    memset(&tm, 0xcc, sizeof(tm));
    ptm = _gmtime64(&t);
    ok(!!ptm || broken(!ptm) /* before Win10 1909 */, "got NULL.\n");
    if (!ptm)
    {
        win_skip("Old gmtime64 limits, skipping tests.\n");
        return;
    }
    ret = _gmtime64_s(&tm, &t);
    ok(!ret, "got %d.\n", ret);
    ok(tm.tm_year == 1101 && tm.tm_hour == 21 && tm.tm_min == 59 && tm.tm_sec == 59, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

    t = _MAX__TIME64_T + 1605601;
    ptm = _gmtime64(&t);
    ok(!ptm, "got non-NULL.\n");
    memset(&tm, 0xcc, sizeof(tm));
    ret = _gmtime64_s(&tm, &t);
    ok(ret == EINVAL, "got %d.\n", ret);
    ok(tm.tm_year == -1 && tm.tm_hour == -1 && tm.tm_min == -1 && tm.tm_sec == -1, "got %d, %d, %d, %d.\n",
            tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void test__get_heap_handle(void)
{
    ok((HANDLE)_get_heap_handle() == GetProcessHeap(), "Expected _get_heap_handle() to return GetProcessHeap()\n");
}

START_TEST(misc)
{
    int arg_c;
    char** arg_v;
    FILETIME cur;

    GetSystemTimeAsFileTime(&cur);
    crt_init_end = ((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime;

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c == 3) {
        if(!strcmp(arg_v[2], "cmd"))
            test__get_narrow_winmain_command_line(NULL);
        else if(!strcmp(arg_v[2], "exit"))
            test_call_exit();
        else if(!strcmp(arg_v[2], "quick_exit"))
            test_call_quick_exit();
        return;
    }

    test_invalid_parameter_handler();
    test__initialize_onexit_table();
    test__register_onexit_function();
    test__execute_onexit_table();
    test___fpe_flt_rounds();
    test__control87_2();
    test__get_narrow_winmain_command_line(arg_v[0]);
    test__sopen_dispatch();
    test__sopen_s();
    test_lldiv();
    test_isblank();
    test_math_errors();
    test_asctime();
    test_strftime();
    test_exit(arg_v[0]);
    test_quick_exit(arg_v[0]);
    test__stat32();
    test__o_malloc();
    test_clock();
    test_thread_storage();
    test_fenv();
    test_fopen_exclusive();
#if defined(__i386__)
    test_rewind_i386_abi();
#endif
    test_gmtime64();
    test__get_heap_handle();
}
