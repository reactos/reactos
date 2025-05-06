/*
 * msvcrt C++ exception handling
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2002 Alexandre Julliard
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
 * NOTES
 * A good reference is the article "How a C++ compiler implements
 * exception handling" by Vishal Kochhar, available on
 * www.thecodeproject.com.
 */

#ifdef __i386__

#include <stdarg.h>
#include <fpieee.h>
#define longjmp ms_longjmp  /* avoid prototype mismatch */
#include <setjmp.h>
#undef longjmp

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/* the exception frame used by CxxFrameHandler */
typedef struct __cxx_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD  frame;    /* the standard exception frame */
    int                            trylevel;
    DWORD                          ebp;
} cxx_exception_frame;

/* exception frame for nested exceptions in catch block */
typedef struct
{
    EXCEPTION_REGISTRATION_RECORD frame;     /* standard exception frame */
    cxx_exception_frame          *cxx_frame; /* frame of parent exception */
    const cxx_function_descr     *descr;     /* descriptor of parent exception */
    int                           trylevel;  /* current try level */
    cxx_frame_info                frame_info;
} catch_func_nested_frame;

typedef struct
{
    cxx_exception_frame *frame;
    const cxx_function_descr *descr;
    catch_func_nested_frame *nested_frame;
} se_translator_ctx;

typedef struct _SCOPETABLE
{
  int previousTryLevel;
  int (*lpfnFilter)(PEXCEPTION_POINTERS);
  void * (*lpfnHandler)(void);
} SCOPETABLE, *PSCOPETABLE;

typedef struct MSVCRT_EXCEPTION_FRAME
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

#define TRYLEVEL_END (-1) /* End of trylevel list */

typedef DWORD (CDECL *cxx_exc_custom_handler)( PEXCEPTION_RECORD, cxx_exception_frame*,
                                               PCONTEXT, EXCEPTION_REGISTRATION_RECORD**,
                                               const cxx_function_descr*, int nested_trylevel,
                                               EXCEPTION_REGISTRATION_RECORD *nested_frame, DWORD unknown );

DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               catch_func_nested_frame* nested_frame );

/* continue execution to the specified address after exception is caught */
extern void DECLSPEC_NORETURN continue_after_catch( cxx_exception_frame* frame, void *addr );

__ASM_GLOBAL_FUNC( continue_after_catch,
                   "movl 4(%esp), %edx\n\t"
                   "movl 8(%esp), %eax\n\t"
                   "movl -4(%edx), %esp\n\t"
                   "leal 12(%edx), %ebp\n\t"
                   "jmp *%eax" );

extern void DECLSPEC_NORETURN call_finally_block( void *code_block, void *base_ptr );

__ASM_GLOBAL_FUNC( call_finally_block,
                   "movl 8(%esp), %ebp\n\t"
                   "jmp *4(%esp)" );

extern int call_filter( int (*func)(PEXCEPTION_POINTERS), void *arg, void *ebp );

__ASM_GLOBAL_FUNC( call_filter,
                   "pushl %ebp\n\t"
                   "pushl 12(%esp)\n\t"
                   "movl 20(%esp), %ebp\n\t"
                   "call *12(%esp)\n\t"
                   "popl %ebp\n\t"
                   "popl %ebp\n\t"
                   "ret" );

extern void *call_handler( void * (*func)(void), void *ebp );

__ASM_GLOBAL_FUNC( call_handler,
                   "pushl %ebp\n\t"
                   "pushl %ebx\n\t"
                   "pushl %esi\n\t"
                   "pushl %edi\n\t"
                   "movl 24(%esp), %ebp\n\t"
                   "call *20(%esp)\n\t"
                   "popl %edi\n\t"
                   "popl %esi\n\t"
                   "popl %ebx\n\t"
                   "popl %ebp\n\t"
                   "ret" );

/* unwind the local function up to a given trylevel */
static void cxx_local_unwind( cxx_exception_frame* frame, const cxx_function_descr *descr, int last_level)
{
    void * (*handler)(void);
    int trylevel = frame->trylevel;

    while (trylevel != last_level)
    {
        if (trylevel < 0 || trylevel >= descr->unwind_count)
        {
            ERR( "invalid trylevel %d\n", trylevel );
            terminate();
        }
        handler = descr->unwind_table[trylevel].handler;
        if (handler)
        {
            TRACE( "calling unwind handler %p trylevel %d last %d ebp %p\n",
                   handler, trylevel, last_level, &frame->ebp );
            call_handler( handler, &frame->ebp );
        }
        trylevel = descr->unwind_table[trylevel].prev;
    }
    frame->trylevel = last_level;
}

/* handler for exceptions happening while calling a catch function */
static DWORD catch_function_nested_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                            CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    catch_func_nested_frame *nested_frame = (catch_func_nested_frame *)frame;

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        __CxxUnregisterExceptionObject(&nested_frame->frame_info, FALSE);
        return ExceptionContinueSearch;
    }

    TRACE( "got nested exception in catch function\n" );

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        PEXCEPTION_RECORD prev_rec = msvcrt_get_thread_data()->exc_record;

        if((rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0) ||
                (prev_rec->ExceptionCode == CXX_EXCEPTION &&
                 rec->ExceptionInformation[1] == prev_rec->ExceptionInformation[1] &&
                 rec->ExceptionInformation[2] == prev_rec->ExceptionInformation[2]))
        {
            /* exception was rethrown */
            *rec = *prev_rec;
            rec->ExceptionFlags &= ~EXCEPTION_UNWINDING;
            if(TRACE_ON(seh)) {
                TRACE("detect rethrow: exception code: %lx\n", rec->ExceptionCode);
                if(rec->ExceptionCode == CXX_EXCEPTION)
                    TRACE("re-propagate: obj: %Ix, type: %Ix\n",
                            rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
            }
        }
        else
        {
            TRACE("detect threw new exception in catch block\n");
        }
    }

    return cxx_frame_handler( rec, nested_frame->cxx_frame, context,
                              NULL, nested_frame->descr, nested_frame );
}

/* find and call the appropriate catch block for an exception */
/* returns the address to continue execution to after the catch block was called */
static inline void call_catch_block( PEXCEPTION_RECORD rec, CONTEXT *context,
                                     cxx_exception_frame *frame,
                                     const cxx_function_descr *descr,
                                     catch_func_nested_frame *catch_frame,
                                     cxx_exception_type *info )
{
    UINT i;
    void *addr, *handler, *object = (void *)rec->ExceptionInformation[1];
    catch_func_nested_frame nested_frame;
    int trylevel = frame->trylevel;
    DWORD save_esp = ((DWORD*)frame)[-1];
    thread_data_t *data = msvcrt_get_thread_data();

    data->processing_throw++;
    for (i = 0; i < descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = &descr->tryblock[i];

        /* only handle try blocks inside current catch block */
        if (catch_frame && catch_frame->trylevel > tryblock->start_level) continue;

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        handler = find_catch_handler( object, (uintptr_t)&frame->ebp, 0, tryblock, info, 0 );
        if (!handler) continue;

        /* Add frame info here so exception is not freed inside RtlUnwind call */
        _CreateFrameInfo(&nested_frame.frame_info.frame_info, object);

        /* unwind the stack */
        RtlUnwind( catch_frame ? &catch_frame->frame : &frame->frame, 0, rec, 0 );
        cxx_local_unwind( frame, descr, tryblock->start_level );
        frame->trylevel = tryblock->end_level + 1;

        nested_frame.frame_info.rec = data->exc_record;
        nested_frame.frame_info.context = data->ctx_record;
        data->exc_record = rec;
        data->ctx_record = context;
        data->processing_throw--;

        /* call the catch block */
        TRACE( "calling handler %p ebp %p\n", handler, &frame->ebp );

        /* setup an exception block for nested exceptions */
        nested_frame.frame.Handler = catch_function_nested_handler;
        nested_frame.cxx_frame = frame;
        nested_frame.descr     = descr;
        nested_frame.trylevel  = tryblock->end_level + 1;

        __wine_push_frame( &nested_frame.frame );
        addr = call_handler( handler, &frame->ebp );
        __wine_pop_frame( &nested_frame.frame );

        ((DWORD*)frame)[-1] = save_esp;
        __CxxUnregisterExceptionObject(&nested_frame.frame_info, FALSE);
        TRACE( "done, continuing at %p\n", addr );

        continue_after_catch( frame, addr );
    }
    data->processing_throw--;
}

static LONG CALLBACK se_translation_filter( EXCEPTION_POINTERS *ep, void *c )
{
    se_translator_ctx *ctx = (se_translator_ctx *)c;
    EXCEPTION_RECORD *rec = ep->ExceptionRecord;
    cxx_exception_type *exc_type;

    if (rec->ExceptionCode != CXX_EXCEPTION)
    {
        TRACE( "non-c++ exception thrown in SEH handler: %lx\n", rec->ExceptionCode );
        terminate();
    }

    exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];
    call_catch_block( rec, ep->ContextRecord, ctx->frame, ctx->descr,
            ctx->nested_frame, exc_type );

    __DestructExceptionObject( rec );
    return ExceptionContinueSearch;
}

static void check_noexcept( PEXCEPTION_RECORD rec,
        const cxx_function_descr *descr, BOOL nested )
{
    if (!nested && rec->ExceptionCode == CXX_EXCEPTION &&
            descr->magic >= CXX_FRAME_MAGIC_VC8 &&
            (descr->flags & FUNC_DESCR_NOEXCEPT))
    {
        ERR("noexcept function propagating exception\n");
        terminate();
    }
}

/*********************************************************************
 *		cxx_frame_handler
 *
 * Implementation of __CxxFrameHandler.
 */
DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               catch_func_nested_frame* nested_frame )
{
    cxx_exception_type *exc_type;

    if (descr->magic < CXX_FRAME_MAGIC_VC6 || descr->magic > CXX_FRAME_MAGIC_VC8)
    {
        ERR( "invalid frame magic %x\n", descr->magic );
        return ExceptionContinueSearch;
    }
    if (descr->magic >= CXX_FRAME_MAGIC_VC8 &&
        (descr->flags & FUNC_DESCR_SYNCHRONOUS) &&
        (rec->ExceptionCode != CXX_EXCEPTION))
        return ExceptionContinueSearch;  /* handle only c++ exceptions */

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND))
    {
        if (descr->unwind_count && !nested_frame) cxx_local_unwind( frame, descr, -1 );
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count)
    {
        check_noexcept(rec, descr, nested_frame != NULL);
        return ExceptionContinueSearch;
    }

    if(rec->ExceptionCode == CXX_EXCEPTION &&
            rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        *rec = *msvcrt_get_thread_data()->exc_record;
        rec->ExceptionFlags &= ~EXCEPTION_UNWINDING;
        if(TRACE_ON(seh)) {
            TRACE("detect rethrow: exception code: %lx\n", rec->ExceptionCode);
            if(rec->ExceptionCode == CXX_EXCEPTION)
                TRACE("re-propagate: obj: %Ix, type: %Ix\n",
                        rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
        }
    }

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (rec->ExceptionInformation[0] > CXX_FRAME_MAGIC_VC8 &&
                exc_type->custom_handler)
        {
            cxx_exc_custom_handler handler = exc_type->custom_handler;
            return handler( rec, frame, context, dispatch, descr,
                            nested_frame ? nested_frame->trylevel : 0,
                            nested_frame ? &nested_frame->frame : NULL, 0 );
        }

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %p trylevel %d descr %p nested_frame %p\n",
                  rec, frame, frame->trylevel, descr, nested_frame );
            TRACE_EXCEPTION_TYPE( exc_type, 0 );
            dump_function_descr( descr, 0 );
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %lx  rec %p frame %p trylevel %d descr %p nested_frame %p\n",
              rec->ExceptionCode,  rec, frame, frame->trylevel, descr, nested_frame );

        if (data->se_translator) {
            EXCEPTION_POINTERS except_ptrs;
            se_translator_ctx ctx;

            ctx.frame = frame;
            ctx.descr = descr;
            ctx.nested_frame = nested_frame;
            __TRY
            {
                except_ptrs.ExceptionRecord = rec;
                except_ptrs.ContextRecord = context;
                data->se_translator( rec->ExceptionCode, &except_ptrs );
            }
            __EXCEPT_CTX(se_translation_filter, &ctx)
            {
            }
            __ENDTRY
        }
    }

    call_catch_block( rec, context, frame, descr,
            nested_frame, exc_type );
    check_noexcept(rec, descr, nested_frame != NULL);
    return ExceptionContinueSearch;
}


/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
extern DWORD CDECL __CxxFrameHandler( PEXCEPTION_RECORD rec, EXCEPTION_REGISTRATION_RECORD* frame,
                                      PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch );
__ASM_GLOBAL_FUNC( __CxxFrameHandler,
                   "pushl $0\n\t"        /* nested_frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl %eax\n\t"      /* descr */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 24(%esp)\n\t"  /* dispatch */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 24(%esp)\n\t"  /* context */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 24(%esp)\n\t"  /* frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 24(%esp)\n\t"  /* rec */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "call " __ASM_NAME("cxx_frame_handler") "\n\t"
                   "add $24,%esp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -24\n\t")
                   "ret" )


/*********************************************************************
 *		__CxxLongjmpUnwind (MSVCRT.@)
 *
 * Callback meant to be used as UnwindFunc for setjmp/longjmp.
 */
void __stdcall __CxxLongjmpUnwind( const _JUMP_BUFFER *buf )
{
    cxx_exception_frame *frame = (cxx_exception_frame *)buf->Registration;
    const cxx_function_descr *descr = (const cxx_function_descr *)buf->UnwindData[0];

    TRACE( "unwinding frame %p descr %p trylevel %ld\n", frame, descr, buf->TryLevel );
    cxx_local_unwind( frame, descr, buf->TryLevel );
}

/*********************************************************************
 *		_EH_prolog (MSVCRT.@)
 */

/* Provided for VC++ binary compatibility only */
__ASM_GLOBAL_FUNC(_EH_prolog,
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")  /* skip ret addr */
                  "pushl $-1\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl %fs:0\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "movl  %esp, %fs:0\n\t"
                  "movl  12(%esp), %eax\n\t"
                  "movl  %ebp, 12(%esp)\n\t"
                  "leal  12(%esp), %ebp\n\t"
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "ret")

static const SCOPETABLE_V4 *get_scopetable_v4( MSVCRT_EXCEPTION_FRAME *frame, ULONG_PTR cookie )
{
    return (const SCOPETABLE_V4 *)((ULONG_PTR)frame->scopetable ^ cookie);
}

static DWORD MSVCRT_nested_handler(PEXCEPTION_RECORD rec,
                                   EXCEPTION_REGISTRATION_RECORD* frame,
                                   PCONTEXT context,
                                   EXCEPTION_REGISTRATION_RECORD** dispatch)
{
  if (!(rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)))
    return ExceptionContinueSearch;
  *dispatch = frame;
  return ExceptionCollidedUnwind;
}

static void msvcrt_local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel, void *ebp)
{
  EXCEPTION_REGISTRATION_RECORD reg;

  TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

  /* Register a handler in case of a nested exception */
  reg.Handler = MSVCRT_nested_handler;
  reg.Prev = NtCurrentTeb()->Tib.ExceptionList;
  __wine_push_frame(&reg);

  while (frame->trylevel != TRYLEVEL_END && frame->trylevel != trylevel)
  {
      int level = frame->trylevel;
      frame->trylevel = frame->scopetable[level].previousTryLevel;
      if (!frame->scopetable[level].lpfnFilter)
      {
          TRACE( "__try block cleanup level %d handler %p ebp %p\n",
                 level, frame->scopetable[level].lpfnHandler, ebp );
          call_handler( frame->scopetable[level].lpfnHandler, ebp );
      }
  }
  __wine_pop_frame(&reg);
  TRACE("unwound OK\n");
}

static void msvcrt_local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel, void *ebp )
{
    EXCEPTION_REGISTRATION_RECORD reg;
    const SCOPETABLE_V4 *scopetable = get_scopetable_v4( frame, *cookie );

    TRACE("(%p,%d,%d)\n",frame, frame->trylevel, trylevel);

    /* Register a handler in case of a nested exception */
    reg.Handler = MSVCRT_nested_handler;
    reg.Prev = NtCurrentTeb()->Tib.ExceptionList;
    __wine_push_frame(&reg);

    while (frame->trylevel != -2 && frame->trylevel != trylevel)
    {
        int level = frame->trylevel;
        frame->trylevel = scopetable->entries[level].previousTryLevel;
        if (!scopetable->entries[level].lpfnFilter)
        {
            TRACE( "__try block cleanup level %d handler %p ebp %p\n",
                   level, scopetable->entries[level].lpfnHandler, ebp );
            call_handler( scopetable->entries[level].lpfnHandler, ebp );
        }
    }
    __wine_pop_frame(&reg);
    TRACE("unwound OK\n");
}

/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void CDECL _local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel)
{
    msvcrt_local_unwind2( frame, trylevel, &frame->_ebp );
}

/*******************************************************************
 *		_local_unwind4 (MSVCRT.@)
 */
void CDECL _local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel )
{
    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );
}

/*******************************************************************
 *		_global_unwind2 (MSVCRT.@)
 */
void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}

/*********************************************************************
 *		_except_handler2 (MSVCRT.@)
 */
int CDECL _except_handler2(PEXCEPTION_RECORD rec,
                           EXCEPTION_REGISTRATION_RECORD* frame,
                           PCONTEXT context,
                           EXCEPTION_REGISTRATION_RECORD** dispatcher)
{
  FIXME("exception %lx flags=%lx at %p handler=%p %p %p stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->Handler, context, dispatcher);
  return ExceptionContinueSearch;
}

/*********************************************************************
 *		_except_handler3 (MSVCRT.@)
 */
int CDECL _except_handler3(PEXCEPTION_RECORD rec,
                           MSVCRT_EXCEPTION_FRAME* frame,
                           PCONTEXT context, void* dispatcher)
{
  int retval, trylevel;
  EXCEPTION_POINTERS exceptPtrs;
  PSCOPETABLE pScopeTable;

  TRACE("exception %lx flags=%lx at %p handler=%p %p %p semi-stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);

  __asm__ __volatile__ ("cld");

  if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
  {
    /* Unwinding the current frame */
    msvcrt_local_unwind2(frame, TRYLEVEL_END, &frame->_ebp);
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
    pScopeTable = frame->scopetable;

    while (trylevel != TRYLEVEL_END)
    {
      TRACE( "level %d prev %d filter %p\n", trylevel, pScopeTable[trylevel].previousTryLevel,
             pScopeTable[trylevel].lpfnFilter );
      if (pScopeTable[trylevel].lpfnFilter)
      {
        retval = call_filter( pScopeTable[trylevel].lpfnFilter, &exceptPtrs, &frame->_ebp );

        TRACE("filter returned %s\n", retval == EXCEPTION_CONTINUE_EXECUTION ?
              "CONTINUE_EXECUTION" : retval == EXCEPTION_EXECUTE_HANDLER ?
              "EXECUTE_HANDLER" : "CONTINUE_SEARCH");

        if (retval == EXCEPTION_CONTINUE_EXECUTION)
          return ExceptionContinueExecution;

        if (retval == EXCEPTION_EXECUTE_HANDLER)
        {
          /* Unwind all higher frames, this one will handle the exception */
          _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
          msvcrt_local_unwind2(frame, trylevel, &frame->_ebp);

          /* Set our trylevel to the enclosing block, and call the __finally
           * code, which won't return
           */
          frame->trylevel = pScopeTable[trylevel].previousTryLevel;
          TRACE("__finally block %p\n",pScopeTable[trylevel].lpfnHandler);
          call_finally_block(pScopeTable[trylevel].lpfnHandler, &frame->_ebp);
       }
      }
      trylevel = pScopeTable[trylevel].previousTryLevel;
    }
  }
  TRACE("reached TRYLEVEL_END, returning ExceptionContinueSearch\n");
  return ExceptionContinueSearch;
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

    TRACE( "exception %lx flags=%lx at %p handler=%p %p %p cookie=%lx scope table=%p cookies=%d/%lx,%d/%lx\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           frame->handler, context, dispatcher, *cookie, scope_table,
           scope_table->gs_cookie_offset, scope_table->gs_cookie_xor,
           scope_table->eh_cookie_offset, scope_table->eh_cookie_xor );

    /* FIXME: no cookie validation yet */

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
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
                    __DestructExceptionObject(rec);

                    /* Unwind all higher frames, this one will handle the exception */
                    _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)frame);
                    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );

                    /* Set our trylevel to the enclosing block, and call the __finally
                     * code, which won't return
                     */
                    frame->trylevel = scope_table->entries[trylevel].previousTryLevel;
                    TRACE("__finally block %p\n",scope_table->entries[trylevel].lpfnHandler);
                    call_finally_block(scope_table->entries[trylevel].lpfnHandler, &frame->_ebp);
                }
            }
            trylevel = scope_table->entries[trylevel].previousTryLevel;
        }
    }
    TRACE("reached -2, returning ExceptionContinueSearch\n");
    return ExceptionContinueSearch;
}


/*
 * setjmp/longjmp implementation
 */

#define MSVCRT_JMP_MAGIC 0x56433230 /* ID value for new jump structure */
typedef void (__stdcall *MSVCRT_unwind_function)(const _JUMP_BUFFER *);

/* define an entrypoint for setjmp/setjmp3 that stores the registers in the jmp buf */
/* and then jumps to the C backend function */
#define DEFINE_SETJMP_ENTRYPOINT(name) \
    __ASM_GLOBAL_FUNC( name, \
                       "movl 4(%esp),%ecx\n\t"   /* jmp_buf */      \
                       "movl %ebp,0(%ecx)\n\t"   /* jmp_buf.Ebp */  \
                       "movl %ebx,4(%ecx)\n\t"   /* jmp_buf.Ebx */  \
                       "movl %edi,8(%ecx)\n\t"   /* jmp_buf.Edi */  \
                       "movl %esi,12(%ecx)\n\t"  /* jmp_buf.Esi */  \
                       "movl %esp,16(%ecx)\n\t"  /* jmp_buf.Esp */  \
                       "movl 0(%esp),%eax\n\t"                      \
                       "movl %eax,20(%ecx)\n\t"  /* jmp_buf.Eip */  \
                       "jmp " __ASM_NAME("__regs_") # name )

/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
#undef _setjmp
DEFINE_SETJMP_ENTRYPOINT( _setjmp )
int CDECL __regs__setjmp(_JUMP_BUFFER *jmp)
{
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == ~0UL)
        jmp->TryLevel = TRYLEVEL_END;
    else
        jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*******************************************************************
 *		_setjmp3 (MSVCRT.@)
 */
DEFINE_SETJMP_ENTRYPOINT( _setjmp3 )
int WINAPIV __regs__setjmp3(_JUMP_BUFFER *jmp, int nb_args, ...)
{
    jmp->Cookie = MSVCRT_JMP_MAGIC;
    jmp->UnwindFunc = 0;
    jmp->Registration = (unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    if (jmp->Registration == ~0UL)
    {
        jmp->TryLevel = TRYLEVEL_END;
    }
    else
    {
        int i;
        va_list args;

        va_start( args, nb_args );
        if (nb_args > 0) jmp->UnwindFunc = va_arg( args, unsigned long );
        if (nb_args > 1) jmp->TryLevel = va_arg( args, unsigned long );
        else jmp->TryLevel = ((MSVCRT_EXCEPTION_FRAME*)jmp->Registration)->trylevel;
        for (i = 0; i < 6 && i < nb_args - 2; i++)
            jmp->UnwindData[i] = va_arg( args, unsigned long );
        va_end( args );
    }

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration );
    return 0;
}

/*********************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl longjmp(_JUMP_BUFFER *jmp, int retval)
{
    unsigned long cur_frame = 0;

    TRACE("buf=%p ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx eip=%08lx frame=%08lx retval=%08x\n",
          jmp, jmp->Ebx, jmp->Esi, jmp->Edi, jmp->Ebp, jmp->Esp, jmp->Eip, jmp->Registration, retval );

    cur_frame=(unsigned long)NtCurrentTeb()->Tib.ExceptionList;
    TRACE("cur_frame=%lx\n",cur_frame);

    if (cur_frame != jmp->Registration)
        _global_unwind2((EXCEPTION_REGISTRATION_RECORD*)jmp->Registration);

    if (jmp->Registration)
    {
        if (IsBadReadPtr(&jmp->Cookie, sizeof(long)) || jmp->Cookie != MSVCRT_JMP_MAGIC)
        {
            msvcrt_local_unwind2((MSVCRT_EXCEPTION_FRAME*)jmp->Registration,
                                 jmp->TryLevel, (void *)jmp->Ebp);
        }
        else if(jmp->UnwindFunc)
        {
            MSVCRT_unwind_function unwind_func;

            unwind_func=(MSVCRT_unwind_function)jmp->UnwindFunc;
            unwind_func(jmp);
        }
    }

    if (!retval)
        retval = 1;

    __wine_longjmp( (__wine_jmp_buf *)jmp, retval );
}

/*********************************************************************
 *		_seh_longjmp_unwind (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind(_JUMP_BUFFER *jmp)
{
    msvcrt_local_unwind2( (MSVCRT_EXCEPTION_FRAME *)jmp->Registration, jmp->TryLevel, (void *)jmp->Ebp );
}

/*********************************************************************
 *		_seh_longjmp_unwind4 (MSVCRT.@)
 */
void __stdcall _seh_longjmp_unwind4(_JUMP_BUFFER *jmp)
{
    msvcrt_local_unwind4( (ULONG *)&jmp->Cookie, (MSVCRT_EXCEPTION_FRAME *)jmp->Registration,
                          jmp->TryLevel, (void *)jmp->Ebp );
}

/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FLOATING_SAVE_AREA *ctx = &ep->ContextRecord->FloatSave;
    _FPIEEE_RECORD rec;
    int ret;

    memset(&rec, 0, sizeof(rec));
    rec.RoundingMode = ctx->ControlWord >> 10;
    switch((ctx->ControlWord >> 8) & 0x3) {
    case 0: rec.Precision = 2; break;
    case 1: rec.Precision = 3; break;
    case 2: rec.Precision = 1; break;
    case 3: rec.Precision = 0; break;
    }
    rec.Status.InvalidOperation = ctx->StatusWord & 0x1;
    rec.Status.ZeroDivide = ((ctx->StatusWord & 0x4) != 0);
    rec.Status.Overflow = ((ctx->StatusWord & 0x8) != 0);
    rec.Status.Underflow = ((ctx->StatusWord & 0x10) != 0);
    rec.Status.Inexact = ((ctx->StatusWord & 0x20) != 0);
    rec.Enable.InvalidOperation = ((ctx->ControlWord & 0x1) == 0);
    rec.Enable.ZeroDivide = ((ctx->ControlWord & 0x4) == 0);
    rec.Enable.Overflow = ((ctx->ControlWord & 0x8) == 0);
    rec.Enable.Underflow = ((ctx->ControlWord & 0x10) == 0);
    rec.Enable.Inexact = ((ctx->ControlWord & 0x20) == 0);
    rec.Cause.InvalidOperation = rec.Enable.InvalidOperation & rec.Status.InvalidOperation;
    rec.Cause.ZeroDivide = rec.Enable.ZeroDivide & rec.Status.ZeroDivide;
    rec.Cause.Overflow = rec.Enable.Overflow & rec.Status.Overflow;
    rec.Cause.Underflow = rec.Enable.Underflow & rec.Status.Underflow;
    rec.Cause.Inexact = rec.Enable.Inexact & rec.Status.Inexact;

    TRACE("code %lx handler %p opcode %lx\n", exception_code, handler,
          *(ULONG*)ep->ContextRecord->FloatSave.ErrorOffset);

    if(*(WORD*)ctx->ErrorOffset == 0x35dc) { /* fdiv m64fp */
        if(exception_code==STATUS_FLOAT_DIVIDE_BY_ZERO || exception_code==STATUS_FLOAT_INVALID_OPERATION) {
            rec.Operand1.OperandValid = 1;
            rec.Result.OperandValid = 0;
        } else {
            rec.Operand1.OperandValid = 0;
            rec.Result.OperandValid = 1;
        }
        rec.Operand2.OperandValid = 1;
        rec.Operation = _FpCodeDivide;
        rec.Operand1.Format = _FpFormatFp80;
        memcpy(&rec.Operand1.Value.Fp80Value, ctx->RegisterArea, sizeof(rec.Operand1.Value.Fp80Value));
        rec.Operand2.Format = _FpFormatFp64;
        rec.Operand2.Value.Fp64Value = *(double*)ctx->DataOffset;
        rec.Result.Format = _FpFormatFp80;
        memcpy(&rec.Result.Value.Fp80Value, ctx->RegisterArea, sizeof(rec.Operand1.Value.Fp80Value));

        ret = handler(&rec);

        if(ret == EXCEPTION_CONTINUE_EXECUTION)
            memcpy(ctx->RegisterArea, &rec.Result.Value.Fp80Value, sizeof(rec.Operand1.Value.Fp80Value));
        return ret;
    }

    FIXME("unsupported opcode: %lx\n", *(ULONG*)ep->ContextRecord->FloatSave.ErrorOffset);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __i386__ */
