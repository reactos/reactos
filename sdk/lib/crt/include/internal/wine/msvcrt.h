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

#endif /* __WINE_MSVCRT_H */
