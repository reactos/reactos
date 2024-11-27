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

#include <setjmp.h>
#include <stdarg.h>
#include <fpieee.h>

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

/* info about a single catch {} block */
typedef struct __catchblock_info
{
    UINT             flags;         /* flags (see below) */
    const type_info *type_info;     /* C++ type caught by this block */
    int              offset;        /* stack offset to copy exception object to */
    void *         (*handler)(void);/* catch block handler code */
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

/* info about a single try {} block */
typedef struct __tryblock_info
{
    int                    start_level;      /* start trylevel of that block */
    int                    end_level;        /* end trylevel of that block */
    int                    catch_level;      /* initial trylevel of the catch block */
    int                    catchblock_count; /* count of catch blocks in array */
    const catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct __unwind_info
{
    int      prev;          /* prev trylevel unwind handler, to run after this one */
    void * (*handler)(void);/* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct __cxx_function_descr
{
    UINT                 magic;          /* must be CXX_FRAME_MAGIC */
    UINT                 unwind_count;   /* number of unwind handlers */
    const unwind_info   *unwind_table;   /* array of unwind handlers */
    UINT                 tryblock_count; /* number of try blocks */
    const tryblock_info *tryblock;       /* array of try blocks */
    UINT                 ipmap_count;
    const void          *ipmap;
    const void          *expect_list;    /* expected exceptions list when magic >= VC7 */
    UINT                 flags;          /* flags when magic >= VC8 */
} cxx_function_descr;

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

DWORD CDECL cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                               PCONTEXT context, EXCEPTION_REGISTRATION_RECORD** dispatch,
                               const cxx_function_descr *descr,
                               catch_func_nested_frame* nested_frame ) DECLSPEC_HIDDEN;

/* call a copy constructor */
extern void call_copy_ctor( void *func, void *this, void *src, int has_vbase );

__ASM_GLOBAL_FUNC( call_copy_ctor,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp, %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl $1\n\t"
                   "movl 12(%ebp), %ecx\n\t"
                   "pushl 16(%ebp)\n\t"
                   "call *8(%ebp)\n\t"
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" );

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
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE( "expect list: %p\n", descr->expect_list );
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE( "flags: %08x\n", descr->flags );
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static const cxx_type_info *find_caught_type( cxx_exception_type *exc_type,
                                              const type_info *catch_ti, UINT catch_flags )
{
    UINT i;

    for (i = 0; i < exc_type->type_info_table->count; i++)
    {
        const cxx_type_info *type = exc_type->type_info_table->info[i];

        if (!catch_ti) return type;   /* catch(...) matches any type */
        if (catch_ti != type->type_info)
        {
            if (strcmp( catch_ti->mangled, type->type_info->mangled )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
            !(catch_flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catch_flags & TYPE_FLAG_VOLATILE)) continue;
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

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
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
            rec->ExceptionFlags &= ~EH_UNWINDING;
            if(TRACE_ON(seh)) {
                TRACE("detect rethrow: exception code: %x\n", rec->ExceptionCode);
                if(rec->ExceptionCode == CXX_EXCEPTION)
                    TRACE("re-propagate: obj: %lx, type: %lx\n",
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
    int j;
    void *addr, *object = (void *)rec->ExceptionInformation[1];
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

        /* got a try block */
        for (j = 0; j < tryblock->catchblock_count; j++)
        {
            const catchblock_info *catchblock = &tryblock->catchblock[j];
            if(info)
            {
                const cxx_type_info *type = find_caught_type( info,
                        catchblock->type_info, catchblock->flags );
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

            /* Add frame info here so exception is not freed inside RtlUnwind call */
            _CreateFrameInfo(&nested_frame.frame_info.frame_info,
                    (void*)rec->ExceptionInformation[1]);

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
            TRACE( "calling catch block %p addr %p ebp %p\n",
                   catchblock, catchblock->handler, &frame->ebp );

            /* setup an exception block for nested exceptions */
            nested_frame.frame.Handler = catch_function_nested_handler;
            nested_frame.cxx_frame = frame;
            nested_frame.descr     = descr;
            nested_frame.trylevel  = tryblock->end_level + 1;

            __wine_push_frame( &nested_frame.frame );
            addr = call_handler( catchblock->handler, &frame->ebp );
            __wine_pop_frame( &nested_frame.frame );

            ((DWORD*)frame)[-1] = save_esp;
            __CxxUnregisterExceptionObject(&nested_frame.frame_info, FALSE);
            TRACE( "done, continuing at %p\n", addr );

            continue_after_catch( frame, addr );
        }
    }
    data->processing_throw--;
}

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( PEXCEPTION_POINTERS ptrs,
                                const type_info *ti, int flags, void **copy)
{
    const cxx_type_info *type;
    PEXCEPTION_RECORD rec;

    TRACE( "%p %p %x %p\n", ptrs, ti, flags, copy );

    if (!ptrs) return EXCEPTION_CONTINUE_SEARCH;

    /* handle catch(...) */
    if (!ti) return EXCEPTION_EXECUTE_HANDLER;

    rec = ptrs->ExceptionRecord;
    if (rec->ExceptionCode != CXX_EXCEPTION || rec->NumberParameters != 3 ||
            rec->ExceptionInformation[0] < CXX_FRAME_MAGIC_VC6 ||
            rec->ExceptionInformation[0] > CXX_FRAME_MAGIC_VC8)
        return EXCEPTION_CONTINUE_SEARCH;

    if (rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        rec = msvcrt_get_thread_data()->exc_record;
        if (!rec) return EXCEPTION_CONTINUE_SEARCH;
    }

    type = find_caught_type( (cxx_exception_type*)rec->ExceptionInformation[2], ti, flags );
    if (!type) return EXCEPTION_CONTINUE_SEARCH;

    if (copy)
    {
        void *object = (void *)rec->ExceptionInformation[1];

        if (flags & TYPE_FLAG_REFERENCE)
        {
            *copy = get_this_pointer( &type->offsets, object );
        }
        else if (type->flags & CLASS_IS_SIMPLE_TYPE)
        {
            memmove( copy, object, type->size );
            /* if it is a pointer, adjust it */
            if (type->size == sizeof(void*)) *copy = get_this_pointer( &type->offsets, *copy );
        }
        else  /* copy the object */
        {
            if (type->copy_ctor)
                call_copy_ctor( type->copy_ctor, copy, get_this_pointer(&type->offsets,object),
                        (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS) );
            else
                memmove( copy, get_this_pointer(&type->offsets,object), type->size );
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static LONG CALLBACK se_translation_filter( EXCEPTION_POINTERS *ep, void *c )
{
    se_translator_ctx *ctx = (se_translator_ctx *)c;
    EXCEPTION_RECORD *rec = ep->ExceptionRecord;
    cxx_exception_type *exc_type;

    if (rec->ExceptionCode != CXX_EXCEPTION)
    {
        TRACE( "non-c++ exception thrown in SEH handler: %x\n", rec->ExceptionCode );
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

    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
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
        rec->ExceptionFlags &= ~EH_UNWINDING;
        if(TRACE_ON(seh)) {
            TRACE("detect rethrow: exception code: %x\n", rec->ExceptionCode);
            if(rec->ExceptionCode == CXX_EXCEPTION)
                TRACE("re-propagate: obj: %lx, type: %lx\n",
                        rec->ExceptionInformation[1], rec->ExceptionInformation[2]);
        }
    }

    if(rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (rec->ExceptionInformation[0] > CXX_FRAME_MAGIC_VC8 &&
                exc_type->custom_handler)
        {
            return exc_type->custom_handler( rec, frame, context, dispatch, descr,
                                         nested_frame ? nested_frame->trylevel : 0,
                                         nested_frame ? &nested_frame->frame : NULL, 0 );
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
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %x  rec %p frame %p trylevel %d descr %p nested_frame %p\n",
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
                   "pushl $0\n\t"        /* nested_trylevel */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl $0\n\t"        /* nested_frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl %eax\n\t"      /* descr */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* dispatch */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* context */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* frame */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 28(%esp)\n\t"  /* rec */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "call " __ASM_NAME("cxx_frame_handler") "\n\t"
                   "add $28,%esp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -28\n\t")
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
 *		__CppXcptFilter (MSVCRT.@)
 */
int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    /* only filter c++ exceptions */
    if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
    return _XcptFilter( ex, ptr );
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
      rec->ExceptionInformation[0] == CXX_FRAME_MAGIC_VC6 &&
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
  if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
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
#ifndef __REACTOS__
/*******************************************************************
 *		_local_unwind2 (MSVCRT.@)
 */
void CDECL _local_unwind2(MSVCRT_EXCEPTION_FRAME* frame, int trylevel)
{
    msvcrt_local_unwind2( frame, trylevel, &frame->_ebp );
}
#endif
/*******************************************************************
 *		_local_unwind4 (MSVCRT.@)
 */
void CDECL _local_unwind4( ULONG *cookie, MSVCRT_EXCEPTION_FRAME* frame, int trylevel )
{
    msvcrt_local_unwind4( cookie, frame, trylevel, &frame->_ebp );
}

#ifndef __REACTOS__
/*******************************************************************
 *		_global_unwind2 (MSVCRT.@)
 */
void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame)
{
    TRACE("(%p)\n",frame);
    RtlUnwind( frame, 0, 0, 0 );
}
#else
void CDECL _global_unwind2(EXCEPTION_REGISTRATION_RECORD* frame);
#endif

#ifndef __REACTOS__
/*********************************************************************
 *		_except_handler2 (MSVCRT.@)
 */
int CDECL _except_handler2(PEXCEPTION_RECORD rec,
                           EXCEPTION_REGISTRATION_RECORD* frame,
                           PCONTEXT context,
                           EXCEPTION_REGISTRATION_RECORD** dispatcher)
{
  FIXME("exception %x flags=%x at %p handler=%p %p %p stub\n",
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

  TRACE("exception %x flags=%x at %p handler=%p %p %p semi-stub\n",
        rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
        frame->handler, context, dispatcher);

#ifdef _MSC_VER
  __asm{ cld }
#else
  __asm__ __volatile__("cld");
#endif
  if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
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
#endif /* __REACTOS__ */
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
DEFINE_SETJMP_ENTRYPOINT(MSVCRT__setjmp)
int CDECL DECLSPEC_HIDDEN __regs_MSVCRT__setjmp(_JUMP_BUFFER *jmp)
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
DEFINE_SETJMP_ENTRYPOINT( MSVCRT__setjmp3 )
int WINAPIV DECLSPEC_HIDDEN __regs_MSVCRT__setjmp3(_JUMP_BUFFER *jmp, int nb_args, ...)
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
void CDECL MSVCRT_longjmp(_JUMP_BUFFER *jmp, int retval)
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
 *              _fpieee_flt (MSVCRT.@)
 */
int __cdecl _fpieee_flt(__msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
        int (__cdecl *handler)(_FPIEEE_RECORD*))
{
    FLOATING_SAVE_AREA *ctx = &ep->ContextRecord->FloatSave;
    _FPIEEE_RECORD rec;
    int ret;

    TRACE("(%lx %p %p)\n", exception_code, ep, handler);

    switch(exception_code) {
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
    case STATUS_FLOAT_INEXACT_RESULT:
    case STATUS_FLOAT_INVALID_OPERATION:
    case STATUS_FLOAT_OVERFLOW:
    case STATUS_FLOAT_UNDERFLOW:
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }

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

    TRACE("opcode: %x\n", *(ULONG*)ep->ContextRecord->FloatSave.ErrorOffset);

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

    FIXME("unsupported opcode: %x\n", *(ULONG*)ep->ContextRecord->FloatSave.ErrorOffset);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __i386__ */
