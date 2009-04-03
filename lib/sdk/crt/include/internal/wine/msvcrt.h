/*
 * Copyright 2001 Jon Griffiths
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include <stdarg.h>
#include <signal.h>
#include "windef.h"
#include "winbase.h"

#include "eh.h"

typedef unsigned short MSVCRT_wchar_t;
typedef unsigned short MSVCRT_wint_t;
typedef unsigned short MSVCRT_wctype_t;
typedef unsigned short MSVCRT__ino_t;
typedef unsigned long  MSVCRT__fsize_t;
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
typedef int  MSVCRT__off_t;
typedef long MSVCRT_clock_t;
typedef long MSVCRT_time_t;
typedef __int64 MSVCRT___time64_t;
typedef __int64 MSVCRT_fpos_t;

struct MSVCRT_tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

/* TLS data */
extern DWORD MSVCRT_tls_index;

typedef struct __MSVCRT_thread_data
{
    int                             thread_errno;
    unsigned long                   thread_doserrno;
    unsigned int                    random_seed;        /* seed for rand() */
    char                           *strtok_next;        /* next ptr for strtok() */
    unsigned char                  *mbstok_next;        /* next ptr for mbstok() */
    MSVCRT_wchar_t                        *wcstok_next;        /* next ptr for wcstok() */
    char                           *efcvt_buffer;       /* buffer for ecvt/fcvt */
    char                           *asctime_buffer;     /* buffer for asctime */
    MSVCRT_wchar_t                        *wasctime_buffer;    /* buffer for wasctime */
    struct MSVCRT_tm                time_buffer;        /* buffer for localtime/gmtime */
    char                           *strerror_buffer;    /* buffer for strerror */
    int                             fpecode;
    terminate_function              terminate_handler;
    unexpected_function             unexpected_handler;
    _se_translator_function         se_translator;
    EXCEPTION_RECORD               *exc_record;
} MSVCRT_thread_data;

extern MSVCRT_thread_data *msvcrt_get_thread_data(void);

extern int MSVCRT_current_lc_all_cp;

void _purecall(void);
void   MSVCRT__set_errno(int);
char*  msvcrt_strndup(const char*,unsigned int);
#ifndef __REACTOS__
MSVCRT_wchar_t *msvcrt_wstrndup(const MSVCRT_wchar_t*, unsigned int);
#endif
void MSVCRT__amsg_exit(int errnum);

extern char **MSVCRT__environ;
#ifndef __REACTOS__
extern MSVCRT_wchar_t **MSVCRT__wenviron;
extern char ** msvcrt_SnapshotOfEnvironmentA(char **);
extern MSVCRT_wchar_t ** msvcrt_SnapshotOfEnvironmentW(MSVCRT_wchar_t **);
#endif

/* FIXME: This should be declared in new.h but it's not an extern "C" so
 * it would not be much use anyway. Even for Winelib applications.
 */
int    MSVCRT__set_new_mode(int mode);

void* MSVCRT_operator_new(unsigned long size);
void MSVCRT_operator_delete(void*);
#ifndef __REACTOS__
typedef void* (*MSVCRT_malloc_func)(MSVCRT_size_t);
#endif
typedef void (*MSVCRT_free_func)(void*);
#ifndef __REACTOS__
extern char* MSVCRT___unDName(int,const char*,int,MSVCRT_malloc_func,MSVCRT_free_func,unsigned int);
#endif

/* Setup and teardown multi threaded locks */
extern void msvcrt_init_mt_locks(void);
extern void msvcrt_free_mt_locks(void);

extern void msvcrt_init_io(void);
extern void msvcrt_free_io(void);
extern void msvcrt_init_console(void);
extern void msvcrt_free_console(void);
extern void msvcrt_init_args(void);
extern void msvcrt_free_args(void);
extern void msvcrt_init_signals(void);
extern void msvcrt_free_signals(void);

extern unsigned create_io_inherit_block(WORD*, BYTE**);

#define MSVCRT__OUT_TO_DEFAULT 0
#define MSVCRT__REPORT_ERRMODE 3

#ifdef __i386__
struct MSVCRT___JUMP_BUFFER {
    unsigned long Ebp;
    unsigned long Ebx;
    unsigned long Edi;
    unsigned long Esi;
    unsigned long Esp;
    unsigned long Eip;
    unsigned long Registration;
    unsigned long TryLevel;
    /* Start of new struct members */
    unsigned long Cookie;
    unsigned long UnwindFunc;
    unsigned long UnwindData[6];
};
#endif /* __i386__ */

typedef void (*float_handler)(int, int);

void _default_handler(int signal);

typedef struct _sig_element
{
   int signal;
   char *signame;
   __p_sig_fn_t handler;
}sig_element;

typedef void* (*malloc_func_t)(size_t);
typedef void  (*free_func_t)(void*);
#define MSVCRT_malloc malloc
#define MSVCRT_free free
char* _setlocale(int,const char*);
NTSYSAPI VOID NTAPI RtlAssert(PVOID FailedAssertion,PVOID FileName,ULONG LineNumber,PCHAR Message);
extern char* __unDName(char *,const char*,int,malloc_func_t,free_func_t,unsigned short int);


#endif /* __WINE_MSVCRT_H */
