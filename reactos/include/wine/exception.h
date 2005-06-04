/*
 * Wine exception handling
 *
 * Copyright (c) 1999 Alexandre Julliard
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

#ifndef __WINE_WINE_EXCEPTION_H
#define __WINE_WINE_EXCEPTION_H

#include <setjmp.h>
#include <windef.h>
#include <excpt.h>
#include <wine/port.h>

/* The following definitions allow using exceptions in Wine and Winelib code
 *
 * They should be used like this:
 *
 * __TRY
 * {
 *     do some stuff that can raise an exception
 * }
 * __EXCEPT(filter_func,param)
 * {
 *     handle the exception here
 * }
 * __ENDTRY
 *
 * or
 *
 * __TRY
 * {
 *     do some stuff that can raise an exception
 * }
 * __FINALLY(finally_func,param)
 *
 * The filter_func must be defined with the WINE_EXCEPTION_FILTER
 * macro, and return one of the EXCEPTION_* code; it can use
 * GetExceptionInformation and GetExceptionCode to retrieve the
 * exception info.
 *
 * The finally_func must be defined with the WINE_FINALLY_FUNC macro.
 *
 * Warning: inside a __TRY or __EXCEPT block, 'break' or 'continue' statements
 *          break out of the current block. You cannot use 'return', 'goto'
 *          or 'longjmp' to leave a __TRY block, as this will surely crash.
 *          You can use them to leave a __EXCEPT block though.
 *
 * -- AJ
 */

/* Define this if you want to use your compiler built-in __try/__except support.
 * This is only useful when compiling to a native Windows binary, as the built-in
 * compiler exceptions will most certainly not work under Winelib.
 */
#ifdef USE_COMPILER_EXCEPTIONS

#define __TRY __try
#define __EXCEPT(func) __except((func)(GetExceptionInformation()))
#define __FINALLY(func) __finally { (func)(!AbnormalTermination()); }
#define __ENDTRY /*nothing*/

#else  /* USE_COMPILER_EXCEPTIONS */

#ifndef __GNUC__
#define __attribute__(x) /* nothing */
#endif

#define __TRY \
    do { __WINE_FRAME __f; \
         int __first = 1; \
         for (;;) if (!__first) \
         { \
             do {

#define __EXCEPT(func) \
             } while(0); \
             __wine_pop_frame( &__f.frame ); \
             break; \
         } else { \
             __f.frame.Handler = __wine_exception_handler; \
             __f.u.filter = (func); \
             __wine_push_frame( &__f.frame ); \
             if (sigsetjmp( __f.jmp, 1 )) { \
                 const __WINE_FRAME * const __eptr __attribute__((unused)) = &__f; \
                 do {

#define __ENDTRY \
                 } while (0); \
                 break; \
             } \
             __first = 0; \
         } \
    } while (0);

#define __FINALLY(func) \
             } while(0); \
             __wine_pop_frame( &__f.frame ); \
             (func)(1); \
             break; \
         } else { \
             __f.frame.Handler = __wine_finally_handler; \
             __f.u.finally_func = (func); \
             __wine_push_frame( &__f.frame ); \
             __first = 0; \
         } \
    } while (0);


typedef DWORD (CALLBACK *__WINE_FILTER)(PEXCEPTION_POINTERS);
typedef void (CALLBACK *__WINE_FINALLY)(BOOL);

#define WINE_EXCEPTION_FILTER(func) DWORD WINAPI func( EXCEPTION_POINTERS *__eptr )
#define WINE_FINALLY_FUNC(func) void WINAPI func( BOOL __normal )

#define GetExceptionInformation() (__eptr)
#define GetExceptionCode()        (__eptr->ExceptionRecord->ExceptionCode)

#undef AbnormalTermination
#define AbnormalTermination()     (!__normal)

typedef struct __tagWINE_FRAME
{
    EXCEPTION_REGISTRATION_RECORD frame;
    union
    {
        /* exception data */
        __WINE_FILTER filter;
        /* finally data */
        __WINE_FINALLY finally_func;
    } u;
    sigjmp_buf jmp;
    /* hack to make GetExceptionCode() work in handler */
    DWORD ExceptionCode;
    const struct __tagWINE_FRAME *ExceptionRecord;
} __WINE_FRAME;

extern DWORD __wine_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_REGISTRATION_RECORD *frame,
                                       CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher );
extern DWORD __wine_finally_handler( PEXCEPTION_RECORD record, EXCEPTION_REGISTRATION_RECORD *frame,
                                     CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher );

#endif /* USE_COMPILER_EXCEPTIONS */

static inline EXCEPTION_REGISTRATION_RECORD *__wine_push_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    EXCEPTION_REGISTRATION_RECORD *prev;
    __asm__ __volatile__(".byte 0x64\n\tmovl (0),%0"
                         "\n\tmovl %0,(%1)"
                         "\n\t.byte 0x64\n\tmovl %1,(0)"
                         : "=&r" (prev) : "r" (frame) : "memory" );
    return prev;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->Prev = (void *)teb->ExceptionList;
    teb->ExceptionList = (void *)frame;
    return frame->Prev;
#endif
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
#if defined(__GNUC__) && defined(__i386__)
    __asm__ __volatile__(".byte 0x64\n\tmovl %0,(0)"
                         : : "r" (frame->prev) : "memory" );
    return frame->prev;

#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    teb->ExceptionList = (void *)frame->Prev;
    return frame->Prev;
#endif
}


/* Wine-specific exceptions codes */

#define EXCEPTION_WINE_STUB       0x80000100  /* stub entry point called */
#define EXCEPTION_WINE_ASSERTION  0x80000101  /* assertion failed */

/* unhandled return status from vm86 mode */
#define EXCEPTION_VM86_INTx       0x80000110
#define EXCEPTION_VM86_STI        0x80000111
#define EXCEPTION_VM86_PICRETURN  0x80000112

extern void __wine_enter_vm86( CONTEXT *context );

#endif  /* __WINE_WINE_EXCEPTION_H */
