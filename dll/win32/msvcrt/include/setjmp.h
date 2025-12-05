/*
 * Setjmp/Longjmp definitions
 *
 * Copyright 2001 Francois Gouget.
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
#ifndef __WINE_SETJMP_H
#define __WINE_SETJMP_H

#include <corecrt.h>

#include <pshpack8.h>

#ifdef __i386__

typedef struct __JUMP_BUFFER
{
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
} _JUMP_BUFFER;

#define _JBLEN 16
#define _JBTYPE int

#elif defined(__x86_64__)

typedef DECLSPEC_ALIGN(16) struct _SETJMP_FLOAT128
{
    unsigned __int64 Part[2];
} SETJMP_FLOAT128;

typedef DECLSPEC_ALIGN(16) struct _JUMP_BUFFER
{
    unsigned __int64 Frame;
    unsigned __int64 Rbx;
    unsigned __int64 Rsp;
    unsigned __int64 Rbp;
    unsigned __int64 Rsi;
    unsigned __int64 Rdi;
    unsigned __int64 R12;
    unsigned __int64 R13;
    unsigned __int64 R14;
    unsigned __int64 R15;
    unsigned __int64 Rip;
    unsigned long MxCsr;
    unsigned short FpCsr;
    unsigned short Spare;
    SETJMP_FLOAT128  Xmm6;
    SETJMP_FLOAT128  Xmm7;
    SETJMP_FLOAT128  Xmm8;
    SETJMP_FLOAT128  Xmm9;
    SETJMP_FLOAT128  Xmm10;
    SETJMP_FLOAT128  Xmm11;
    SETJMP_FLOAT128  Xmm12;
    SETJMP_FLOAT128  Xmm13;
    SETJMP_FLOAT128  Xmm14;
    SETJMP_FLOAT128  Xmm15;
} _JUMP_BUFFER;

#define _JBLEN  16
typedef SETJMP_FLOAT128 _JBTYPE;

#elif defined(__arm__)

typedef struct _JUMP_BUFFER
{
    unsigned long Frame;
    unsigned long R4;
    unsigned long R5;
    unsigned long R6;
    unsigned long R7;
    unsigned long R8;
    unsigned long R9;
    unsigned long R10;
    unsigned long R11;
    unsigned long Sp;
    unsigned long Pc;
    unsigned long Fpscr;
    unsigned long long D[8];
} _JUMP_BUFFER;

#define _JBLEN  28
#define _JBTYPE int

#elif defined(__aarch64__)

typedef struct _JUMP_BUFFER
{
    unsigned __int64 Frame;
    unsigned __int64 Reserved;
    unsigned __int64 X19;
    unsigned __int64 X20;
    unsigned __int64 X21;
    unsigned __int64 X22;
    unsigned __int64 X23;
    unsigned __int64 X24;
    unsigned __int64 X25;
    unsigned __int64 X26;
    unsigned __int64 X27;
    unsigned __int64 X28;
    unsigned __int64 Fp;
    unsigned __int64 Lr;
    unsigned __int64 Sp;
    unsigned long Fpcr;
    unsigned long Fpsr;
    double D[8];
} _JUMP_BUFFER;

#define _JBLEN  24
#define _JBTYPE unsigned __int64

#else

#define _JBLEN 1
#define _JBTYPE int

#endif

typedef _JBTYPE jmp_buf[_JBLEN];

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP void __cdecl longjmp(jmp_buf,int);

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#ifdef _UCRT
# ifdef __i386__
#  define _setjmp __intrinsic_setjmp
# else
#  define _setjmpex __intrinsic_setjmpex
# endif
#endif

#ifdef __i386__
_ACRTIMP int __cdecl __attribute__((__nothrow__,__returns_twice__)) _setjmp(jmp_buf);
# define setjmp(buf) _setjmp((buf))
#elif !defined(_setjmpex) && __has_builtin(_setjmpex)
_ACRTIMP int __cdecl __attribute__((__nothrow__,__returns_twice__)) _setjmpex(jmp_buf);
# define setjmp(buf) _setjmpex(buf)
#else
_ACRTIMP int __cdecl __attribute__((__nothrow__,__returns_twice__)) _setjmpex(jmp_buf,void*);
# if __has_builtin(__builtin_sponentry)
#  define setjmp(buf) _setjmpex((buf), __builtin_sponentry())
# elif __has_builtin(__builtin_frame_address)
#  define setjmp(buf) _setjmpex((buf), __builtin_frame_address(0))
# else
#  define setjmp(buf) _setjmpex((buf), NULL)
# endif
#endif  /* __i386__ */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_SETJMP_H */
