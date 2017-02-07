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
 */

#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include <stdarg.h>
#include <signal.h>
#include "windef.h"
#include "winbase.h"

extern int __lc_codepage;
extern int __lc_collate_cp;
extern int __mb_cur_max;
extern const unsigned short _ctype [257];

void __cdecl _purecall(void);
void __cdecl _amsg_exit(int errnum);

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

extern char* __cdecl __unDName(char *,const char*,int,malloc_func_t,free_func_t,unsigned short int);
extern char* __cdecl __unDNameEx(char *,const char*,int,malloc_func_t,free_func_t,void *,unsigned short int);

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

extern unsigned create_io_inherit_block(WORD*, BYTE**);

/* _set_abort_behavior codes */
#define MSVCRT__WRITE_ABORT_MSG    1
#define MSVCRT__CALL_REPORTFAULT   2

#define MSVCRT__OUT_TO_DEFAULT 0
#define MSVCRT__OUT_TO_STDERR  1
#define MSVCRT__OUT_TO_MSGBOX  2
#define MSVCRT__REPORT_ERRMODE 3

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

#endif /* __WINE_MSVCRT_H */
