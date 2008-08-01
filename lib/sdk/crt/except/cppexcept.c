/*
 * msvcrt C++ exception handling
 *
 * Copyright 2002 Alexandre Julliard
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

#include "wine/config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include <internal/wine/msvcrt.h>
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include <internal/wine/cppexcept.h>

#ifdef __i386__  /* CxxFrameHandler is not supported on non-i386 */

WINE_DEFAULT_DEBUG_CHANNEL(seh);

DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               EXCEPTION_REGISTRATION_RECORD* nested_frame, int nested_trylevel );

/* call a function with a given ebp */
static inline void *call_ebp_func( void *func, void *ebp )
{
    void *ret;
    int dummy;
    __asm__ __volatile__ ("pushl %%ebx\n\t"
                          "pushl %%ebp\n\t"
                          "movl %4,%%ebp\n\t"
                          "call *%%eax\n\t"
                          "popl %%ebp\n\t"
                          "popl %%ebx"
                          : "=a" (ret), "=S" (dummy), "=D" (dummy)
                          : "0" (func), "1" (ebp) : "ecx", "edx", "memory" );
    return ret;
}

/* call a copy constructor */
static inline void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
    TRACE( "calling copy ctor %p object %p src %p\n", func, this, src );
    if (has_vbase)
        /* in that case copy ctor takes an extra bool indicating whether to copy the base class */
        __asm__ __volatile__("pushl $1; pushl %2; call *%0"
                             : : "r" (func), "c" (this), "r" (src) : "eax", "edx", "memory" );
    else
        __asm__ __volatile__("pushl %2; call *%0"
                             : : "r" (func), "c" (this), "r" (src) : "eax", "edx", "memory" );
}

/* call the destructor of the exception object */
static inline void call_dtor( void *func, void *object )
{
    __asm__ __volatile__("call *%0" : : "r" (func), "c" (object) : "eax", "edx", "memory" );
}

/* continue execution to the specified address after exception is caught */
static inline void DECLSPEC_NORETURN continue_after_catch( cxx_exception_frame* frame, void *addr )
{
    __asm__ __volatile__("movl -4(%0),%%esp; leal 12(%0),%%ebp; jmp *%1"
                         : : "r" (frame), "a" (addr) );
    for (;;) ; /* unreached */
}

static inline void dump_type( const cxx_type_info *type )
{
    TRACE( "flags %x type %p %s offsets %d,%d,%d size %d copy ctor %p\n",
             type->flags, type->type_info, dbgstr_type_info(type->type_info),
             type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset,
             type->size, type->copy_ctor );
}

static void dump_exception_type( const cxx_exception_type *type )
{
    UINT i;

    TRACE( "flags %x destr %p handler %p type info %p\n",
             type->flags, type->destructor, type->custom_handler, type->type_info_table );
    for (i = 0; i < type->type_info_table->count; i++)
    {
        TRACE( "    %d: ", i );
        dump_type( type->type_info_table->info[i] );
    }
}

static void dump_function_descr( const cxx_function_descr *descr )
{
    UINT i;
    int j;

    TRACE( "magic %x\n", descr->magic );
    TRACE( "unwind table: %p %d\n", descr->unwind_table, descr->unwind_count );
    for (i = 0; i < descr->unwind_count; i++)
    {
        TRACE( "    %d: prev %d func %p\n", i,
                 descr->unwind_table[i].prev, descr->unwind_table[i].handler );
    }
    TRACE( "try table: %p %d\n", descr->tryblock, descr->tryblock_count );
    for (i = 0; i < descr->tryblock_count; i++)
    {
        TRACE( "    %d: start %d end %d catchlevel %d catch %p %d\n", i,
                 descr->tryblock[i].start_level, descr->tryblock[i].end_level,
                 descr->tryblock[i].catch_level, descr->tryblock[i].catchblock,
                 descr->tryblock[i].catchblock_count );
        for (j = 0; j < descr->tryblock[i].catchblock_count; j++)
        {
            const catchblock_info *ptr = &descr->tryblock[i].catchblock[j];
            TRACE( "        %d: flags %x offset %d handler %p type %p %s\n",
                     j, ptr->flags, ptr->offset, ptr->handler,
                     ptr->type_info, dbgstr_type_info( ptr->type_info ) );
        }
    }
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static const cxx_type_info *find_caught_type( cxx_exception_type *exc_type,
                                              const catchblock_info *catchblock )
{
    UINT i;

    for (i = 0; i < exc_type->type_info_table->count; i++)
    {
        const cxx_type_info *type = exc_type->type_info_table->info[i];

        if (!catchblock->type_info) return type;   /* catch(...) matches any type */
        if (catchblock->type_info != type->type_info)
        {
            if (strcmp( catchblock->type_info->mangled, type->type_info->mangled )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
            !(catchblock->flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catchblock->flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }
    return NULL;
}


/* copy the exception object where the catch block wants it */
static void copy_exception( void *object, cxx_exception_frame *frame,
                            const catchblock_info *catchblock, const cxx_type_info *type )
{
    void **dest_ptr;

    if (!catchblock->type_info || !catchblock->type_info->mangled[0]) return;
    if (!catchblock->offset) return;
    dest_ptr = (void **)((char *)&frame->ebp + catchblock->offset);

    if (catchblock->flags & TYPE_FLAG_REFERENCE)
    {
        *dest_ptr = get_this_pointer( &type->offsets, object );
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove( dest_ptr, object, type->size );
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void *)) *dest_ptr = get_this_pointer( &type->offsets, *dest_ptr );
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
            call_copy_ctor( type->copy_ctor, dest_ptr, get_this_pointer(&type->offsets,object),
                            (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS) );
        else
            memmove( dest_ptr, get_this_pointer(&type->offsets,object), type->size );
    }
}

/* unwind the local function up to a given trylevel */
static void cxx_local_unwind( cxx_exception_frame* frame, const cxx_function_descr *descr, int last_level)
{
    void (*handler)();
    int trylevel = frame->trylevel;

    while (trylevel != last_level)
    {
        if (trylevel < 0 || trylevel >= descr->unwind_count)
        {
            ERR( "invalid trylevel %d\n", trylevel );
            MSVCRT_terminate();
        }
        handler = descr->unwind_table[trylevel].handler;
        if (handler)
        {
            TRACE( "calling unwind handler %p trylevel %d last %d ebp %p\n",
                   handler, trylevel, last_level, &frame->ebp );
            call_ebp_func( handler, &frame->ebp );
        }
        trylevel = descr->unwind_table[trylevel].prev;
    }
    frame->trylevel = last_level;
}

/* exception frame for nested exceptions in catch block */
struct catch_func_nested_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;     /* standard exception frame */
    EXCEPTION_RECORD             *prev_rec;  /* previous record to restore in thread data */
    cxx_exception_frame          *cxx_frame; /* frame of parent exception */
    const cxx_function_descr     *descr;     /* descriptor of parent exception */
    int                           trylevel;  /* current try level */
    EXCEPTION_RECORD             *rec;       /* rec associated with frame */
};

/* handler for exceptions happening while calling a catch function */
static DWORD catch_function_nested_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                            CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct catch_func_nested_frame *nested_frame = (struct catch_func_nested_frame *)frame;

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        msvcrt_get_thread_data()->exc_record = nested_frame->prev_rec;
        return ExceptionContinueSearch;
    }

    TRACE( "got nested exception in catch function\n" );

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        PEXCEPTION_RECORD prev_rec = nested_frame->rec;
        if(rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
        {
            /* exception was rethrown */
            rec->ExceptionInformation[1] = prev_rec->ExceptionInformation[1];
            rec->ExceptionInformation[2] = prev_rec->ExceptionInformation[2];
            TRACE("detect rethrow: re-propagate: obj: %lx, type: %lx\n",
                    rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
        }
        else {
            /* new exception in exception handler, destroy old */
            void *object = (void*)prev_rec->ExceptionInformation[1];
            cxx_exception_type *info = (cxx_exception_type*) prev_rec->ExceptionInformation[2];
            TRACE("detect threw new exception in catch block - destroy old(obj: %p type: %p)\n",
                    object, info);
            if(info && info->destructor)
                call_dtor( info->destructor, object );
        }
    }

    return cxx_frame_handler( rec, nested_frame->cxx_frame, context,
                              NULL, nested_frame->descr, &nested_frame->frame,
                              nested_frame->trylevel );
}

/* find and call the appropriate catch block for an exception */
/* returns the address to continue execution to after the catch block was called */
static inline void call_catch_block( PEXCEPTION_RECORD rec, cxx_exception_frame *frame,
                                     const cxx_function_descr *descr, int nested_trylevel,
                                     cxx_exception_type *info )
{
    UINT i;
    int j;
    void *addr, *object = (void *)rec->ExceptionInformation[1];
    struct catch_func_nested_frame nested_frame;
    int trylevel = frame->trylevel;
    MSVCRT_thread_data *thread_data = msvcrt_get_thread_data();
    DWORD save_esp = ((DWORD*)frame)[-1];

    for (i = 0; i < descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = &descr->tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        /* got a try block */
        for (j = 0; j < tryblock->catchblock_count; j++)
        {
            const catchblock_info *catchblock = &tryblock->catchblock[j];
            if(info)
            {
                const cxx_type_info *type = find_caught_type( info, catchblock );
                if (!type) continue;

                TRACE( "matched type %p in tryblock %d catchblock %d\n", type, i, j );

                /* copy the exception to its destination on the stack */
                copy_exception( object, frame, catchblock, type );
            }
            else
            {
                /* no CXX_EXCEPTION only proceed with a catch(...) block*/
                if(catchblock->type_info)
                    continue;
                TRACE("found catch(...) block\n");
            }

            /* unwind the stack */
            RtlUnwind( frame, 0, rec, 0 );
            cxx_local_unwind( frame, descr, tryblock->start_level );
            frame->trylevel = tryblock->end_level + 1;

            /* call the catch block */
            TRACE( "calling catch block %p addr %p ebp %p\n",
                   catchblock, catchblock->handler, &frame->ebp );

            /* setup an exception block for nested exceptions */

            nested_frame.frame.Handler = (PEXCEPTION_HANDLER)catch_function_nested_handler;
            nested_frame.prev_rec  = thread_data->exc_record;
            nested_frame.cxx_frame = frame;
            nested_frame.descr     = descr;
            nested_frame.trylevel  = nested_trylevel + 1;
            nested_frame.rec = rec;

            __wine_push_frame( &nested_frame.frame );
            thread_data->exc_record = rec;
            addr = call_ebp_func( catchblock->handler, &frame->ebp );
            thread_data->exc_record = nested_frame.prev_rec;
            __wine_pop_frame( &nested_frame.frame );

            ((DWORD*)frame)[-1] = save_esp;
            if (info && info->destructor) call_dtor( info->destructor, object );
            TRACE( "done, continuing at %p\n", addr );

            continue_after_catch( frame, addr );
        }
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
                               EXCEPTION_REGISTRATION_RECORD* nested_frame,
                               int nested_trylevel )
{
    cxx_exception_type *exc_type;

    if (descr->magic != CXX_FRAME_MAGIC)
    {
        ERR( "invalid frame magic %x\n", descr->magic );
        return ExceptionContinueSearch;
    }
    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
    {
        if (descr->unwind_count && !nested_trylevel) cxx_local_unwind( frame, descr, -1 );
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count) return ExceptionContinueSearch;

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (rec->ExceptionInformation[0] > CXX_FRAME_MAGIC &&
                exc_type->custom_handler)
        {
            return exc_type->custom_handler( rec, frame, context, dispatch,
                                         descr, nested_trylevel, nested_frame, 0 );
        }

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %p trylevel %d descr %p nested_frame %p\n",
                  rec, frame, frame->trylevel, descr, nested_frame );
            dump_exception_type( exc_type );
            dump_function_descr( descr );
        }
    }
    else
    {
        exc_type = NULL;
        TRACE("handling C exception code %x  rec %p frame %p trylevel %d descr %p nested_frame %p\n",
              rec->ExceptionCode,  rec, frame, frame->trylevel, descr, nested_frame );
    }

    call_catch_block( rec, frame, descr, frame->trylevel, exc_type );
    return ExceptionContinueSearch;
}


/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
extern DWORD CDECL __CxxFrameHandler( PEXCEPTION_RECORD rec, EXCEPTION_REGISTRATION_RECORD* frame,
                                      PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch );
__ASM_GLOBAL_FUNC( __CxxFrameHandler,
                   "pushl $0\n\t"        /* nested_trylevel */
                   "pushl $0\n\t"        /* nested_frame */
                   "pushl %eax\n\t"      /* descr */
                   "pushl 28(%esp)\n\t"  /* dispatch */
                   "pushl 28(%esp)\n\t"  /* context */
                   "pushl 28(%esp)\n\t"  /* frame */
                   "pushl 28(%esp)\n\t"  /* rec */
                   "call " __ASM_NAME("cxx_frame_handler") "\n\t"
                   "add $28,%esp\n\t"
                   "ret" )


/*********************************************************************
 *		__CxxLongjmpUnwind (MSVCRT.@)
 *
 * Callback meant to be used as UnwindFunc for setjmp/longjmp.
 */
void __stdcall __CxxLongjmpUnwind( const struct MSVCRT___JUMP_BUFFER *buf )
{
    cxx_exception_frame *frame = (cxx_exception_frame *)buf->Registration;
    const cxx_function_descr *descr = (const cxx_function_descr *)buf->UnwindData[0];

    TRACE( "unwinding frame %p descr %p trylevel %ld\n", frame, descr, buf->TryLevel );
    cxx_local_unwind( frame, descr, buf->TryLevel );
}

#endif  /* __i386__ */

/*********************************************************************
 *		_CxxThrowException (MSVCRT.@)
 */
void CDECL _CxxThrowException( exception *object, const cxx_exception_type *type )
{
    ULONG_PTR args[3];

    args[0] = CXX_FRAME_MAGIC;
    args[1] = (ULONG_PTR)object;
    args[2] = (ULONG_PTR)type;
    RaiseException( CXX_EXCEPTION, EH_NONCONTINUABLE, 3, args );
}

/*********************************************************************
 *		__CxxDetectRethrow (MSVCRT.@)
 */
BOOL CDECL __CxxDetectRethrow(PEXCEPTION_POINTERS ptrs)
{
  PEXCEPTION_RECORD rec;

  if (!ptrs)
    return FALSE;

  rec = ptrs->ExceptionRecord;

  if (rec->ExceptionCode == CXX_EXCEPTION &&
      rec->NumberParameters == 3 &&
      rec->ExceptionInformation[0] == CXX_FRAME_MAGIC &&
      rec->ExceptionInformation[2])
  {
    ptrs->ExceptionRecord = msvcrt_get_thread_data()->exc_record;
    return TRUE;
  }
  return (msvcrt_get_thread_data()->exc_record == rec);
}

/*********************************************************************
 *		__CxxQueryExceptionSize (MSVCRT.@)
 */
unsigned int CDECL __CxxQueryExceptionSize(void)
{
  return sizeof(cxx_exception_type);
}
