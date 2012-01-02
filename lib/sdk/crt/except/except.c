/*
 * msvcrt.dll exception handling
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2005 Juan Lang
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
 * FIXME: Incomplete support for nested exceptions/try block cleanup.
 */

#include <precomp.h>
#include "excpt.h"
#include <wine/exception.h>

void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame);

typedef void (__cdecl *MSVCRT_security_error_handler)(int, void *);
static MSVCRT_security_error_handler security_error_handler;

/* VC++ extensions to Win32 SEH */
typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  int (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct _MSVCRT_EXCEPTION_FRAME
{
  EXCEPTION_REGISTRATION_RECORD *prev;
  void (*handler)(PEXCEPTION_RECORD, EXCEPTION_REGISTRATION_RECORD*,
                  PCONTEXT, PEXCEPTION_RECORD);
  PSCOPETABLE scopetable;
  int trylevel;
  int _ebp;
  PEXCEPTION_POINTERS xpointers;
} MSVCRT_EXCEPTION_FRAME;

typedef struct
{
  int   gs_cookie_offset;
  ULONG gs_cookie_xor;
  int   eh_cookie_offset;
  ULONG eh_cookie_xor;
  SCOPETABLE entries[1];
} SCOPETABLE_V4;

#ifdef __i386__

static const SCOPETABLE_V4 *get_scopetable_v4( MSVCRT_EXCEPTION_FRAME *frame, ULONG_PTR cookie )
{
    return (const SCOPETABLE_V4 *)((ULONG_PTR)frame->scopetable ^ cookie);
}

#if defined(__GNUC__)
static inline void call_finally_block( void *code_block, void *base_ptr )
{
    __asm__ __volatile__ ("movl %1,%%ebp; call *%%eax"
                          : : "a" (code_block), "g" (base_ptr));
}

static inline int call_filter( int (*func)(PEXCEPTION_POINTERS), void *arg, void *ebp )
{
    int ret;
    __asm__ __volatile__ ("pushl %%ebp; pushl %3; movl %2,%%ebp; call *%%eax; popl %%ebp; popl %%ebp"
                          : "=a" (ret)
                          : "0" (func), "r" (ebp), "r" (arg)
                          : "ecx", "edx", "memory" );
    return ret;
}
static inline int call_unwind_func( int (*func)(void), void *ebp )
{
    int ret;
    __asm__ __volatile__ ("pushl %%ebp\n\t"
                          "pushl %%ebx\n\t"
                          "pushl %%esi\n\t"
                          "pushl %%edi\n\t"
                          "movl %2,%%ebp\n\t"
                          "call *%0\n\t"
                          "popl %%edi\n\t"
                          "popl %%esi\n\t"
                          "popl %%ebx\n\t"
                          "popl %%ebp"
                          : "=a" (ret)
                          : "0" (func), "r" (ebp)
                          : "ecx", "edx", "memory" );
    return ret;
}
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4731) // Don't complain about changing ebp
void __inline call_finally_block( void *code_block, void *base_ptr )
{
    __asm
    {
        mov eax, code_block
        mov ebp, base_ptr
        call [eax]
    }
}

int __inline call_filter( int (*func)(PEXCEPTION_POINTERS), void *arg, void *_ebp )
{
    int _ret;
    __asm
    {
        push ebp
        mov eax, arg
        push eax
        mov ebp, _ebp
        mov eax, func
        call [eax]
        mov _ret, eax
        pop ebp
        pop ebp
    }
    return _ret;
}
int __inline call_unwind_func( int (*func)(void), void *_ebp )
{
    int _ret;

    __asm
    {
        push ebp
        push ebx
        push esi
        push edi
        mov ebp, _ebp
        call dword ptr [func]
        mov _ret, eax
        pop edi
        pop esi
        pop ebx
        pop ebp
    }
    return _ret;
}
#pragma warning(pop)
#endif

static DWORD MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                   EXCEPTION_REGISTRATION_RECORD* frame,
                                   PCONTEXT context,
                                   EXCEPTION_REGISTRATION_RECORD** dispatch)
{
  if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
    return ExceptionContinueSearch;
  *dispatch = frame;
  return ExceptionCollidedUnwind;
}

void msvcrt_local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel, void *ebp )
{
    EXCEPTION_REGISTRATION_RECORD reg;
    const SCOPETABLE_V4 *scopetable = get_scopetable_v4( frame, *cookie );

    TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

    /* Register a handler in case of a nested exception */
    reg.Handler = (PEXCEPTION_ROUTINE)MSVCRT_nested_handler;
    reg.Prev = NtCurrentTeb()->NtTib.ExceptionList;
    __wine_push_frame(&reg);

    while (frame->trylevel != -2 && frame->trylevel != trylevel)
    {
        int level = frame->trylevel;
        frame->trylevel = scopetable->entries[level].previousTryLevel;
        if (!scopetable->entries[level].lpfnFilter)
        {
            TRACE( "__try block cleanup level %d handler %p ebp %p\n",
                   level, scopetable->entries[level].lpfnHandler, ebp );
            call_unwind_func( scopetable->entries[level].lpfnHandler, ebp );
        }
    }
    __wine_pop_frame(&reg);
    TRACE("unwound OK\n");
}

/*********************************************************************
 *		_except_handler4_common (MSVCRT.@)
 */
int CDECL _except_handler4_common( ULONG *cookie, void (*check_cookie)(void),
                                   EXCEPTION_RECORD *rec, MSVCRT_EXCEPTION_FRAME *frame,
                                   CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    int retval, trylevel;
    EXCEPTION_POINTERS exceptPtrs;
    const SCOPETABLE_V4 *scope_table = get_scopetable_v4( frame, *cookie );

    TRACE( "exception %x flags=%x at %p handler=%p %p %p cookie=%x scope table=%p cookies=%d/%x,%d/%x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           frame->handler, context, dispatcher, *cookie, scope_table,
           scope_table->gs_cookie_offset, scope_table->gs_cookie_xor,
           scope_table->eh_cookie_offset, scope_table->eh_cookie_xor );

    /* FIXME: no cookie validation yet */

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        /* Unwinding the current frame */
        msvcrt_local_unwind4( cookie, frame, -2, &frame->_ebp );
        TRACE("unwound current frame, returning ExceptionContinueSearch\n");
        return ExceptionContinueSearch;
    }
    else
    {
        /* Hunting for handler */
        exceptPtrs.ExceptionRecord = rec;
        exceptPtrs.ContextRecord = context;
        *((DWORD *)frame-1) = (DWORD)&exceptPtrs;
        trylevel = frame->trylevel;

        while (trylevel != -2)
        {
            TRACE( "level %d prev %d filter %p\n", trylevel,
                   scope_table->entries[trylevel].previousTryLevel,
                   scope_table->entries[trylevel].lpfnFilter );
            if (scope_table->entries[trylevel].lpfnFilter)
            {
                retval = call_filter( scope_table->entries[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp );

                TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
                      "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
                      "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

                if (retval == EXCEPTION_CONTINUE_EXECUTION)
                    return ExceptionContinueExecution;

                if (retval == EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Unwind all higher frames, this one will handle the exception */
                    _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
                    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );

                    /* Set our trylevel to the enclosing block, and call the __finally
                     * code, which won't return
                     */
                    frame->trylevel = scope_table->entries[trylevel].previousTryLevel;
                    TRACE("__finally block %p\n",scope_table->entries[trylevel].lpfnHandler);
                    call_finally_block(scope_table->entries[trylevel].lpfnHandler, &frame->_ebp);
                    ERR("Returned from __finally block - expect crash!\n");
                }
            }
            trylevel = scope_table->entries[trylevel].previousTryLevel;
        }
    }
    TRACE("reached -2, returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
}

/*******************************************************************
 *		_local_unwind4 (MSVCRT.@)
 */
void CDECL _local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel )
{
    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );
}

/*********************************************************************
 *		_seh_longjmp_unwind4 (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind4(struct __JUMP_BUFFER *jmp)
{
    msvcrt_local_unwind4( (void *)jmp->Cookie, (MSVCRT_EXCEPTION_FRAME *)jmp->Registration,
                          jmp->TryLevel, (void *)jmp->Ebp );
}

#endif

/******************************************************************
 *		__uncaught_exception
 */
BOOL CDECL __uncaught_exception(void)
{
    return FALSE;
}

/* _set_security_error_handler - not exported in native msvcrt, added in msvcr70 */
MSVCRT_security_error_handler CDECL _set_security_error_handler(
    MSVCRT_security_error_handler handler )
{
    MSVCRT_security_error_handler old = security_error_handler;

    TRACE("(%p)\n", handler);

    security_error_handler = handler;
    return old;
}
