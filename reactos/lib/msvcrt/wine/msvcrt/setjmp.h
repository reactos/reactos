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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_SETJMP_H
#define __WINE_SETJMP_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif


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

#endif /* __i386__ */

#ifndef USE_MSVCRT_PREFIX
#define _JBLEN                     16
#define _JBTYPE                    int
typedef _JBTYPE                    jmp_buf[_JBLEN];
#else
#define MSVCRT__JBLEN              16
#define MSVCRT__JBTYPE             int
typedef MSVCRT__JBTYPE             MSVCRT_jmp_buf[MSVCRT__JBLEN];
#endif


#ifdef __cplusplus
extern "C" {
#endif

int         MSVCRT(_setjmp)(MSVCRT(jmp_buf));
int         MSVCRT(longjmp)(MSVCRT(jmp_buf),int);

#ifdef __cplusplus
}
#endif

#ifndef USE_MSVCRT_PREFIX
#define setjmp _setjmp
#endif

#endif /* __WINE_SETJMP_H */
