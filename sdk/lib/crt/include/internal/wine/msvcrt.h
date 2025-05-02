/*
 * Copyright 2001 Jon Griffiths
 * Copyright 2004 Dimitrie O. Paun
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
 *
 * NOTES
 *   Naming conventions
 *	- Symbols are prefixed with MSVCRT_ if they conflict
 *        with libc symbols
 *      - Internal symbols are usually prefixed by msvcrt_.
 *      - Exported symbols that are not present in the public
 *        headers are usually kept the same as the original.
 *   Other conventions
 *      - To avoid conflicts with the standard C library,
 *        no msvcrt headers are included in the implementation.
 *      - Instead, symbols are duplicated here, prefixed with
 *        MSVCRT_, as explained above.
 *      - To avoid inconsistencies, a test for each symbol is
 *        added into tests/headers.c. Please always add a
 *        corresponding test when you add a new symbol!
 */

#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include <stdarg.h>
#include <signal.h>
#include "windef.h"
#include "winbase.h"

#define MSVCRT_LONG_MAX    0x7fffffffL
#define MSVCRT_ULONG_MAX   0xffffffffUL
#define MSVCRT_I64_MAX    (((__int64)0x7fffffff << 32) | 0xffffffff)
#define MSVCRT_I64_MIN    (-MSVCRT_I64_MAX-1)
#define MSVCRT_UI64_MAX   (((unsigned __int64)0xffffffff << 32) | 0xffffffff)

#define MSVCRT__MAX_DRIVE  3
#define MSVCRT__MAX_DIR    256
#define MSVCRT__MAX_FNAME  256
#define MSVCRT__MAX_EXT    256

typedef unsigned short MSVCRT_wchar_t;
typedef unsigned short MSVCRT_wint_t;
typedef unsigned short MSVCRT_wctype_t;
typedef unsigned short MSVCRT__ino_t;
typedef unsigned int   MSVCRT__fsize_t;
typedef int            MSVCRT_long;
typedef unsigned int   MSVCRT_ulong;
#ifdef _WIN64
typedef unsigned __int64 MSVCRT_size_t;
typedef __int64 MSVCRT_intptr_t;
typedef unsigned __int64 MSVCRT_uintptr_t;
#else
typedef unsigned int MSVCRT_size_t;
typedef int MSVCRT_intptr_t;
typedef unsigned int MSVCRT_uintptr_t;
#endif
typedef unsigned int   MSVCRT__dev_t;
typedef int MSVCRT__off_t;
typedef int MSVCRT_clock_t;
typedef int MSVCRT___time32_t;
typedef __int64 DECLSPEC_ALIGN(8) MSVCRT___time64_t;
typedef __int64 DECLSPEC_ALIGN(8) MSVCRT_fpos_t;
typedef int MSVCRT_mbstate_t;

extern unsigned int __lc_codepage;
extern int __lc_collate_cp;
extern int __mb_cur_max;
extern const unsigned short _ctype [257];

void __cdecl _purecall(void);
__declspec(noreturn) void __cdecl _amsg_exit(int errnum);

extern char **_environ;
extern wchar_t **_wenviron;
extern char ** SnapshotOfEnvironmentA(char **);
extern wchar_t ** SnapshotOfEnvironmentW(wchar_t **);

/* Application type flags */
#define _UNKNOWN_APP    0
#define _CONSOLE_APP    1
#define _GUI_APP        2

/* I/O Streamming flags missing from stdio.h */
#define _IOYOURBUF      0x0100
#define _IOAPPEND       0x0200
#define _IOSETVBUF      0x0400
#define _IOFEOF         0x0800
#define _IOFLRTN        0x1000
#define _IOCTRLZ        0x2000
#define _IOCOMMIT       0x4000
#define _IOFREE         0x10000

//wchar_t *wstrdupa(const char *);
//
///* FIXME: This should be declared in new.h but it's not an extern "C" so
// * it would not be much use anyway. Even for Winelib applications.
// */
//int __cdecl _set_new_mode(int mode);
//
void* __cdecl MSVCRT_operator_new(size_t);
void __cdecl MSVCRT_operator_delete(void*);
typedef void* (__cdecl *malloc_func_t)(size_t);
typedef void  (__cdecl *free_func_t)(void*);

/* Setup and teardown multi threaded locks */
extern void msvcrt_init_mt_locks(void);
extern void msvcrt_free_mt_locks(void);

extern BOOL msvcrt_init_locale(void);
extern void msvcrt_init_math(void);
extern void msvcrt_init_io(void);
extern void msvcrt_free_io(void);
extern void msvcrt_init_console(void);
extern void msvcrt_free_console(void);
extern void msvcrt_init_args(void);
extern void msvcrt_free_args(void);
extern void msvcrt_init_signals(void);
extern void msvcrt_free_signals(void);
extern void msvcrt_free_popen_data(void);

extern unsigned create_io_inherit_block(WORD*, BYTE**);

/* _set_abort_behavior codes */
#define MSVCRT__WRITE_ABORT_MSG    1
#define MSVCRT__CALL_REPORTFAULT   2

#define MSVCRT_EPERM   1
#define MSVCRT_ENOENT  2
#define MSVCRT_ESRCH   3
#define MSVCRT_EINTR   4
#define MSVCRT_EIO     5
#define MSVCRT_ENXIO   6
#define MSVCRT_E2BIG   7
#define MSVCRT_ENOEXEC 8
#define MSVCRT_EBADF   9
#define MSVCRT_ECHILD  10
#define MSVCRT_EAGAIN  11
#define MSVCRT_ENOMEM  12
#define MSVCRT_EACCES  13
#define MSVCRT_EFAULT  14
#define MSVCRT_EBUSY   16
#define MSVCRT_EEXIST  17
#define MSVCRT_EXDEV   18
#define MSVCRT_ENODEV  19
#define MSVCRT_ENOTDIR 20
#define MSVCRT_EISDIR  21
#define MSVCRT_EINVAL  22
#define MSVCRT_ENFILE  23
#define MSVCRT_EMFILE  24
#define MSVCRT_ENOTTY  25
#define MSVCRT_EFBIG   27
#define MSVCRT_ENOSPC  28
#define MSVCRT_ESPIPE  29
#define MSVCRT_EROFS   30
#define MSVCRT_EMLINK  31
#define MSVCRT_EPIPE   32
#define MSVCRT_EDOM    33
#define MSVCRT_ERANGE  34
#define MSVCRT_EDEADLK 36
#define MSVCRT_EDEADLOCK MSVCRT_EDEADLK
#define MSVCRT_ENAMETOOLONG 38
#define MSVCRT_ENOLCK  39
#define MSVCRT_ENOSYS  40
#define MSVCRT_ENOTEMPTY 41
#define MSVCRT_EILSEQ    42
#define MSVCRT_STRUNCATE 80

#define MSVCRT_LC_ALL      LC_ALL
#define MSVCRT_LC_COLLATE  LC_COLLATE
#define MSVCRT_LC_CTYPE    LC_CTYPE
#define MSVCRT_LC_MONETARY LC_MONETARY
#define MSVCRT_LC_NUMERIC  LC_NUMERIC
#define MSVCRT_LC_TIME     LC_TIME
#define MSVCRT_LC_MIN      LC_MIN
#define MSVCRT_LC_MAX      LC_MAX

#define MSVCRT__OUT_TO_DEFAULT 0
#define MSVCRT__OUT_TO_STDERR  1
#define MSVCRT__OUT_TO_MSGBOX  2
#define MSVCRT__REPORT_ERRMODE 3

#define MSVCRT__EM_INVALID    0x00000010
#define MSVCRT__EM_DENORMAL   0x00080000
#define MSVCRT__EM_ZERODIVIDE 0x00000008
#define MSVCRT__EM_OVERFLOW   0x00000004
#define MSVCRT__EM_UNDERFLOW  0x00000002
#define MSVCRT__EM_INEXACT    0x00000001

#define MSVCRT__TRUNCATE ((MSVCRT_size_t)-1)

#define MSVCRT__ARGMAX 100

extern char* __cdecl __unDName(char *,const char*,int,malloc_func_t,free_func_t,unsigned short int);

/* __unDName/__unDNameEx flags */
#define UNDNAME_COMPLETE                 (0x0000)
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001) /* Don't show __ in calling convention */
#define UNDNAME_NO_MS_KEYWORDS           (0x0002) /* Don't show calling convention at all */
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004) /* Don't show function/method return value */
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)
#define UNDNAME_NO_MS_THISTYPE           (0x0020)
#define UNDNAME_NO_CV_THISTYPE           (0x0040)
#define UNDNAME_NO_THISTYPE              (0x0060)
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080) /* Don't show access specifier (public/protected/private) */
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)
#define UNDNAME_NO_MEMBER_TYPE           (0x0200) /* Don't show static/virtual specifier */
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)
#define UNDNAME_32_BIT_DECODE            (0x0800)
#define UNDNAME_NAME_ONLY                (0x1000) /* Only report the variable/method name */
#define UNDNAME_NO_ARGUMENTS             (0x2000) /* Don't show method arguments */
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)
#define UNDNAME_NO_COMPLEX_TYPE          (0x8000)

typedef void (*float_handler)(int, int);
void _default_handler(int signal);
typedef struct _sig_element
{
   int signal;
   char *signame;
   __p_sig_fn_t handler;
}sig_element;

#define MSVCRT_malloc malloc
#define MSVCRT_free free
char* _setlocale(int,const char*);
NTSYSAPI VOID NTAPI RtlAssert(PVOID FailedAssertion,PVOID FileName,ULONG LineNumber,PCHAR Message);

/* ioinfo structure size is different in msvcrXX.dll's */
typedef struct {
    HANDLE              handle;
    unsigned char       wxflag;
    char                lookahead[3];
    int                 exflag;
    CRITICAL_SECTION    crit;
} ioinfo;

typedef int (*puts_clbk_a)(void*, int, const char*);
typedef int (*puts_clbk_w)(void*, int, const wchar_t*);
typedef union _printf_arg
{
    void *get_ptr;
    int get_int;
    LONGLONG get_longlong;
    double get_double;
} printf_arg;
typedef printf_arg (*args_clbk)(void*, int, size_t, __ms_va_list*);
int pf_printf_a(puts_clbk_a, void*, const char*, _locale_t,
        BOOL, BOOL, args_clbk, void*, __ms_va_list);
int pf_printf_w(puts_clbk_w, void*, const wchar_t*, _locale_t,
        BOOL, BOOL, args_clbk, void*, __ms_va_list);

#endif /* __WINE_MSVCRT_H */
