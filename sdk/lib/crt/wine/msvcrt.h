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

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#define _NO_CRT_STDIO_INLINE
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#undef strncpy
#undef wcsncpy

#ifdef __REACTOS__
typedef long __msvcrt_long;
typedef unsigned long __msvcrt_ulong;
#endif

extern BOOL sse2_supported DECLSPEC_HIDDEN;

#define DBL80_MAX_10_EXP 4932
#define DBL80_MIN_10_EXP -4951

typedef void (__cdecl *terminate_function)(void);
typedef void (__cdecl *unexpected_function)(void);
typedef void (__cdecl *_se_translator_function)(unsigned int code, struct _EXCEPTION_POINTERS *info);
void __cdecl terminate(void);

typedef void (__cdecl *MSVCRT_security_error_handler)(int, void *);

typedef struct {ULONG x80[3];} MSVCRT__LDOUBLE; /* Intel 80 bit FP format has sizeof() 12 */

typedef struct __lc_time_data {
    union {
        const char *str[43];
        struct {
            const char *short_wday[7];
            const char *wday[7];
            const char *short_mon[12];
            const char *mon[12];
            const char *am;
            const char *pm;
            const char *short_date;
            const char *date;
            const char *time;
        } names;
    } str;
#if _MSVCR_VER < 110
    LCID lcid;
#endif
    int unk;
    int refcount;
#if _MSVCR_VER == 0 || _MSVCR_VER >= 100
    union {
        const wchar_t *wstr[43];
        struct {
            const wchar_t *short_wday[7];
            const wchar_t *wday[7];
            const wchar_t *short_mon[12];
            const wchar_t *mon[12];
            const wchar_t *am;
            const wchar_t *pm;
            const wchar_t *short_date;
            const wchar_t *date;
            const wchar_t *time;
        } names;
    } wstr;
#endif
#if _MSVCR_VER >= 110
    const wchar_t *locname;
#endif
    char data[1];
} __lc_time_data;

typedef struct threadmbcinfostruct {
    int refcount;
    int mbcodepage;
    int ismbcodepage;
    int mblcid;
    unsigned short mbulinfo[6];
    unsigned char mbctype[257];
    unsigned char mbcasemap[256];
} threadmbcinfo;

typedef struct _frame_info
{
    void *object;
    struct _frame_info *next;
} frame_info;

typedef struct
{
    frame_info frame_info;
    EXCEPTION_RECORD *rec;
    CONTEXT *context;
} cxx_frame_info;

frame_info* __cdecl _CreateFrameInfo(frame_info *fi, void *obj);
BOOL __cdecl __CxxRegisterExceptionObject(EXCEPTION_POINTERS*, cxx_frame_info*);
void __cdecl __CxxUnregisterExceptionObject(cxx_frame_info*, BOOL);
void CDECL __DestructExceptionObject(EXCEPTION_RECORD*);

/* TLS data */
extern DWORD msvcrt_tls_index DECLSPEC_HIDDEN;

#define LOCALE_FREE     0x1
#define LOCALE_THREAD   0x2

/* Keep in sync with msvcr90/tests/msvcr90.c */
struct __thread_data {
    DWORD                           tid;
    HANDLE                          handle;
    int                             thread_errno;
    __msvcrt_ulong                  thread_doserrno;
    int                             unk1;
    unsigned int                    random_seed;        /* seed for rand() */
    char                           *strtok_next;        /* next ptr for strtok() */
    wchar_t                        *wcstok_next;        /* next ptr for wcstok() */
    unsigned char                  *mbstok_next;        /* next ptr for mbstok() */
    char                           *strerror_buffer;    /* buffer for strerror */
    wchar_t                        *wcserror_buffer;    /* buffer for wcserror */
    char                           *tmpnam_buffer;      /* buffer for tmpname() */
    wchar_t                        *wtmpnam_buffer;     /* buffer for wtmpname() */
    void                           *unk2[2];
    char                           *asctime_buffer;     /* buffer for asctime */
    wchar_t                        *wasctime_buffer;    /* buffer for wasctime */
    struct tm                      *time_buffer;        /* buffer for localtime/gmtime */
    char                           *efcvt_buffer;       /* buffer for ecvt/fcvt */
    int                             unk3[2];
    void                           *unk4[3];
    EXCEPTION_POINTERS             *xcptinfo;
    int                             fpecode;
    pthreadmbcinfo                  mbcinfo;
    pthreadlocinfo                  locinfo;
    int                             locale_flags;
    int                             unk5[1];
    terminate_function              terminate_handler;
    unexpected_function             unexpected_handler;
    _se_translator_function         se_translator;      /* preserve offset to exc_record and processing_throw */
    void                           *unk6;
    EXCEPTION_RECORD               *exc_record;
    CONTEXT                        *ctx_record;
    int                             processing_throw;
    frame_info                     *frame_info_head;
    void                           *unk8[6];
    LCID                            cached_lcid;
    BOOL                            cached_sname;
    int                             unk9[2];
    DWORD                           cached_cp;
    char                            cached_locale[131];
    void                           *unk10[100];
#if _MSVCR_VER >= 140
    _invalid_parameter_handler      invalid_parameter_handler;
    HMODULE                         module;
#endif
};

typedef struct __thread_data thread_data_t;

extern thread_data_t *CDECL msvcrt_get_thread_data(void) DECLSPEC_HIDDEN;

LCID locale_to_LCID(const char*, unsigned short*, BOOL*) DECLSPEC_HIDDEN;
extern _locale_t MSVCRT_locale DECLSPEC_HIDDEN;
extern __lc_time_data cloc_time_data DECLSPEC_HIDDEN;
extern unsigned int MSVCRT___lc_codepage;
extern int MSVCRT___lc_collate_cp;
extern WORD MSVCRT__ctype [257];
extern BOOL initial_locale DECLSPEC_HIDDEN;
extern WORD *MSVCRT__pwctype;

void msvcrt_set_errno(int) DECLSPEC_HIDDEN;
#if _MSVCR_VER >= 80
void throw_bad_alloc(void) DECLSPEC_HIDDEN;
#endif

void __cdecl _purecall(void);
void __cdecl _amsg_exit(int errnum);

extern char **MSVCRT__environ;
extern wchar_t **MSVCRT__wenviron;

extern char ** msvcrt_SnapshotOfEnvironmentA(char **) DECLSPEC_HIDDEN;
extern wchar_t ** msvcrt_SnapshotOfEnvironmentW(wchar_t **) DECLSPEC_HIDDEN;

wchar_t *msvcrt_wstrdupa(const char *) DECLSPEC_HIDDEN;

extern unsigned int MSVCRT__commode;

/* FIXME: This should be declared in new.h but it's not an extern "C" so
 * it would not be much use anyway. Even for Winelib applications.
 */
void* __cdecl operator_new(size_t);
void __cdecl operator_delete(void*);
int __cdecl _set_new_mode(int mode);

typedef void* (__cdecl *malloc_func_t)(size_t);
typedef void  (__cdecl *free_func_t)(void*);

/* Setup and teardown multi threaded locks */
extern void msvcrt_init_mt_locks(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_locks(void) DECLSPEC_HIDDEN;

extern void msvcrt_init_exception(void*) DECLSPEC_HIDDEN;
extern BOOL msvcrt_init_locale(void) DECLSPEC_HIDDEN;
extern void msvcrt_init_math(void*) DECLSPEC_HIDDEN;
extern void msvcrt_init_io(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_io(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_console(void) DECLSPEC_HIDDEN;
extern void msvcrt_init_args(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_args(void) DECLSPEC_HIDDEN;
extern void msvcrt_init_signals(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_signals(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_popen_data(void) DECLSPEC_HIDDEN;
extern BOOL msvcrt_init_heap(void) DECLSPEC_HIDDEN;
extern void msvcrt_destroy_heap(void) DECLSPEC_HIDDEN;
extern void msvcrt_init_clock(void) DECLSPEC_HIDDEN;

#if _MSVCR_VER >= 100
extern void msvcrt_init_concurrency(void*) DECLSPEC_HIDDEN;
extern void msvcrt_free_concurrency(void) DECLSPEC_HIDDEN;
extern void msvcrt_free_scheduler_thread(void) DECLSPEC_HIDDEN;
#endif

extern BOOL msvcrt_create_io_inherit_block(WORD*, BYTE**) DECLSPEC_HIDDEN;

/* run-time error codes */
#define _RT_STACK       0
#define _RT_NULLPTR     1
#define _RT_FLOAT       2
#define _RT_INTDIV      3
#define _RT_EXECMEM     5
#define _RT_EXECFORM    6
#define _RT_EXECENV     7
#define _RT_SPACEARG    8
#define _RT_SPACEENV    9
#define _RT_ABORT       10
#define _RT_NPTR        12
#define _RT_FPTR        13
#define _RT_BREAK       14
#define _RT_INT         15
#define _RT_THREAD      16
#define _RT_LOCK        17
#define _RT_HEAP        18
#define _RT_OPENCON     19
#define _RT_QWIN        20
#define _RT_NOMAIN      21
#define _RT_NONCONT     22
#define _RT_INVALDISP   23
#define _RT_ONEXIT      24
#define _RT_PUREVIRT    25
#define _RT_STDIOINIT   26
#define _RT_LOWIOINIT   27
#define _RT_HEAPINIT    28
#define _RT_DOMAIN      120
#define _RT_SING        121
#define _RT_TLOSS       122
#define _RT_CRNL        252
#define _RT_BANNER      255

extern FILE MSVCRT__iob[];

#define MSVCRT_NO_CONSOLE_FD (-2)
#define MSVCRT_NO_CONSOLE ((HANDLE)MSVCRT_NO_CONSOLE_FD)

#define MSVCRT_stdin       (MSVCRT__iob+STDIN_FILENO)
#define MSVCRT_stdout      (MSVCRT__iob+STDOUT_FILENO)
#define MSVCRT_stderr      (MSVCRT__iob+STDERR_FILENO)

/* internal file._flag flags */
#define MSVCRT__USERBUF  0x0100
#define MSVCRT__IOCOMMIT 0x4000

#ifdef __REACTOS__
typedef void (__cdecl *__sighandler_t)(int);
#endif

#define _MAX__TIME64_T    (((__time64_t)0x00000007 << 32) | 0x93406FFF)

_locale_t CDECL get_current_locale_noalloc(_locale_t locale) DECLSPEC_HIDDEN;
void CDECL free_locale_noalloc(_locale_t locale) DECLSPEC_HIDDEN;
pthreadlocinfo CDECL get_locinfo(void) DECLSPEC_HIDDEN;
pthreadmbcinfo CDECL get_mbcinfo(void) DECLSPEC_HIDDEN;
threadmbcinfo* create_mbcinfo(int, LCID, threadmbcinfo*) DECLSPEC_HIDDEN;
void free_locinfo(pthreadlocinfo) DECLSPEC_HIDDEN;
void free_mbcinfo(pthreadmbcinfo) DECLSPEC_HIDDEN;
int __cdecl __crtLCMapStringA(LCID, DWORD, const char*, int, char*, int, unsigned int, int) DECLSPEC_HIDDEN;

enum fpmod {
    FP_ROUND_ZERO, /* only used when dropped part contains only zeros */
    FP_ROUND_DOWN,
    FP_ROUND_EVEN,
    FP_ROUND_UP,
    FP_VAL_INFINITY,
    FP_VAL_NAN
};

struct fpnum {
    int sign;
    int exp;
    ULONGLONG m;
    enum fpmod mod;
};
struct fpnum fpnum_parse(wchar_t (*)(void*), void (*)(void*),
        void*, pthreadlocinfo, BOOL) DECLSPEC_HIDDEN;
int fpnum_double(struct fpnum*, double*) DECLSPEC_HIDDEN;
/* Maybe one day we'll enable the invalid parameter handlers with the full set of information (msvcrXXd)
 *      #define MSVCRT_INVALID_PMT(x) MSVCRT_call_invalid_parameter_handler(x, __FUNCTION__, __FILE__, __LINE__, 0)
 *      #define MSVCRT_CHECK_PMT(x)   ((x) ? TRUE : MSVCRT_INVALID_PMT(#x),FALSE)
 * Until this is done, just keep the same semantics for CHECK_PMT(), but without generating / sending
 * any information
 * NB : MSVCRT_call_invalid_parameter_handler is a wrapper around _invalid_parameter in order
 * to do the Ansi to Unicode transformation
 */
#define MSVCRT_INVALID_PMT(x,err)   (*_errno() = (err), _invalid_parameter(NULL, NULL, NULL, 0, 0))
#define MSVCRT_CHECK_PMT_ERR(x,err) ((x) || (MSVCRT_INVALID_PMT( 0, (err) ), FALSE))
#define MSVCRT_CHECK_PMT(x)         MSVCRT_CHECK_PMT_ERR((x), EINVAL)

typedef int (*puts_clbk_a)(void*, int, const char*);
typedef int (*puts_clbk_w)(void*, int, const wchar_t*);
typedef union _printf_arg
{
    void *get_ptr;
    int get_int;
    LONGLONG get_longlong;
    double get_double;
} printf_arg;
typedef printf_arg (*args_clbk)(void*, int, int, va_list*);
int pf_printf_a(puts_clbk_a, void*, const char*, _locale_t,
        DWORD, args_clbk, void*, va_list*) DECLSPEC_HIDDEN;
int pf_printf_w(puts_clbk_w, void*, const wchar_t*, _locale_t,
        DWORD, args_clbk, void*, va_list*) DECLSPEC_HIDDEN;
int create_positional_ctx_a(void*, const char*, va_list) DECLSPEC_HIDDEN;
int create_positional_ctx_w(void*, const wchar_t*, va_list) DECLSPEC_HIDDEN;
printf_arg arg_clbk_valist(void*, int, int, va_list*) DECLSPEC_HIDDEN;
printf_arg arg_clbk_positional(void*, int, int, va_list*) DECLSPEC_HIDDEN;

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

#define UCRTBASE_PRINTF_MASK ( \
        _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION | \
        _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR | \
        _CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS | \
        _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY | \
        _CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS | \
        _CRT_INTERNAL_PRINTF_STANDARD_ROUNDING )

#define MSVCRT_PRINTF_POSITIONAL_PARAMS                  (0x0100)
#define MSVCRT_PRINTF_INVOKE_INVALID_PARAM_HANDLER       (0x0200)

#define UCRTBASE_SCANF_MASK ( \
        _CRT_INTERNAL_SCANF_SECURECRT | \
        _CRT_INTERNAL_SCANF_LEGACY_WIDE_SPECIFIERS | \
        _CRT_INTERNAL_SCANF_LEGACY_MSVCRT_COMPATIBILITY )

#define COOPERATIVE_TIMEOUT_INFINITE ((unsigned int)-1)
#define COOPERATIVE_WAIT_TIMEOUT     ~0

#define INHERIT_THREAD_PRIORITY 0xF000


#ifdef __REACTOS__
#define __wine_longjmp longjmp
#define __wine_jmp_buf _JBTYPE

#ifdef _M_IX86
// ASM wrapper for Wine code. See rosglue_i386.s for implementation.
void
WINAPI
__wine__RtlUnwind(
    struct _EXCEPTION_REGISTRATION_RECORD* pEndFrame,
    PVOID targetIp,
    struct _EXCEPTION_RECORD* pRecord,
    PVOID retval);
#define RtlUnwind __wine__RtlUnwind
#endif /* _M_IX86 */

#endif

#endif /* __WINE_MSVCRT_H */
