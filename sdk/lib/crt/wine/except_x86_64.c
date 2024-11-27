/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
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

#ifdef __x86_64__

#include <setjmp.h>
#include <stdarg.h>
#include <fpieee.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef struct
{
    int  prev;
    UINT handler;
} unwind_info;

typedef struct
{
    UINT flags;
    UINT type_info;
    int  offset;
    UINT handler;
    UINT frame;
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

typedef struct
{
    int  start_level;
    int  end_level;
    int  catch_level;
    int  catchblock_count;
    UINT catchblock;
} tryblock_info;

typedef struct
{
    int ip;
    int state;
} ipmap_info;

typedef struct __cxx_function_descr
{
    UINT magic;
    UINT unwind_count;
    UINT unwind_table;
    UINT tryblock_count;
    UINT tryblock;
    UINT ipmap_count;
    UINT ipmap;
    UINT unwind_help;
    UINT expect_list;
    UINT flags;
} cxx_function_descr;

typedef struct
{
    cxx_frame_info frame_info;
    BOOL rethrow;
    EXCEPTION_RECORD *prev_rec;
} cxx_catch_ctx;

typedef struct
{
    ULONG64 dest_frame;
    ULONG64 orig_frame;
    EXCEPTION_RECORD *seh_rec;
    DISPATCHER_CONTEXT *dispatch;
    const cxx_function_descr *descr;
} se_translator_ctx;

static inline void* rva_to_ptr(UINT rva, ULONG64 base)
{
    return rva ? (void*)(base+rva) : NULL;
}

static inline void dump_type(UINT type_rva, ULONG64 base)
{
    const cxx_type_info *type = rva_to_ptr(type_rva, base);

    TRACE("flags %x type %x %s offsets %d,%d,%d size %d copy ctor %x(%p)\n",
            type->flags, type->type_info, dbgstr_type_info(rva_to_ptr(type->type_info, base)),
            type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset,
            type->size, type->copy_ctor, rva_to_ptr(type->copy_ctor, base));
}

static void dump_exception_type(const cxx_exception_type *type, ULONG64 base)
{
    const cxx_type_info_table *type_info_table = rva_to_ptr(type->type_info_table, base);
    UINT i;

    TRACE("flags %x destr %x(%p) handler %x(%p) type info %x(%p)\n",
            type->flags, type->destructor, rva_to_ptr(type->destructor, base),
            type->custom_handler, rva_to_ptr(type->custom_handler, base),
            type->type_info_table, type_info_table);
    for (i = 0; i < type_info_table->count; i++)
    {
        TRACE("    %d: ", i);
        dump_type(type_info_table->info[i], base);
    }
}

static void dump_function_descr(const cxx_function_descr *descr, ULONG64 image_base)
{
    unwind_info *unwind_table = rva_to_ptr(descr->unwind_table, image_base);
    tryblock_info *tryblock = rva_to_ptr(descr->tryblock, image_base);
    ipmap_info *ipmap = rva_to_ptr(descr->ipmap, image_base);
    UINT i, j;

    TRACE("magic %x\n", descr->magic);
    TRACE("unwind table: %x(%p) %d\n", descr->unwind_table, unwind_table, descr->unwind_count);
    for (i=0; i<descr->unwind_count; i++)
    {
        TRACE("    %d: prev %d func %x(%p)\n", i, unwind_table[i].prev,
                unwind_table[i].handler, rva_to_ptr(unwind_table[i].handler, image_base));
    }
    TRACE("try table: %x(%p) %d\n", descr->tryblock, tryblock, descr->tryblock_count);
    for (i=0; i<descr->tryblock_count; i++)
    {
        catchblock_info *catchblock = rva_to_ptr(tryblock[i].catchblock, image_base);

        TRACE("    %d: start %d end %d catchlevel %d catch %x(%p) %d\n", i,
                tryblock[i].start_level, tryblock[i].end_level,
                tryblock[i].catch_level, tryblock[i].catchblock,
                catchblock, tryblock[i].catchblock_count);
        for (j=0; j<tryblock[i].catchblock_count; j++)
        {
            TRACE("        %d: flags %x offset %d handler %x(%p) frame %x type %x %s\n",
                    j, catchblock[j].flags, catchblock[j].offset, catchblock[j].handler,
                    rva_to_ptr(catchblock[j].handler, image_base), catchblock[j].frame,
                    catchblock[j].type_info,
                    dbgstr_type_info(rva_to_ptr(catchblock[j].type_info, image_base)));
        }
    }
    TRACE("ipmap: %x(%p) %d\n", descr->ipmap, ipmap, descr->ipmap_count);
    for (i=0; i<descr->ipmap_count; i++)
    {
        TRACE("    %d: ip %x state %d\n", i, ipmap[i].ip, ipmap[i].state);
    }
    TRACE("unwind_help %d\n", descr->unwind_help);
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE("expect list: %x\n", descr->expect_list);
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE("flags: %08x\n", descr->flags);
}

static inline int ip_to_state(ipmap_info *ipmap, UINT count, int ip)
{
    UINT low = 0, high = count-1, med;

    while (low < high) {
        med = low + (high-low)/2;

        if (ipmap[med].ip <= ip && ipmap[med+1].ip > ip)
        {
            low = med;
            break;
        }
        if (ipmap[med].ip < ip) low = med+1;
        else high = med-1;
    }

    TRACE("%x -> %d\n", ip, ipmap[low].state);
    return ipmap[low].state;
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static const cxx_type_info *find_caught_type(cxx_exception_type *exc_type, ULONG64 exc_base,
                                             const type_info *catch_ti, UINT catch_flags)
{
    const cxx_type_info_table *type_info_table = rva_to_ptr(exc_type->type_info_table, exc_base);
    UINT i;

    for (i = 0; i < type_info_table->count; i++)
    {
        const cxx_type_info *type = rva_to_ptr(type_info_table->info[i], exc_base);
        const type_info *ti = rva_to_ptr(type->type_info, exc_base);

        if (!catch_ti) return type;   /* catch(...) matches any type */
        if (catch_ti != ti)
        {
            if (strcmp( catch_ti->mangled, ti->mangled )) continue;
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

static inline void copy_exception(void *object, ULONG64 frame,
                                  DISPATCHER_CONTEXT *dispatch,
                                  const catchblock_info *catchblock,
                                  const cxx_type_info *type, ULONG64 exc_base)
{
    const type_info *catch_ti = rva_to_ptr(catchblock->type_info, dispatch->ImageBase);
    void **dest = rva_to_ptr(catchblock->offset, frame);

    if (!catch_ti || !catch_ti->mangled[0]) return;
    if (!catchblock->offset) return;

    if (catchblock->flags & TYPE_FLAG_REFERENCE)
    {
        *dest = get_this_pointer(&type->offsets, object);
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove(dest, object, type->size);
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void*)) *dest = get_this_pointer(&type->offsets, *dest);
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
        {
            if (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS)
            {
                void (__cdecl *copy_ctor)(void*, void*, int) =
                    rva_to_ptr(type->copy_ctor, exc_base);
                copy_ctor(dest, get_this_pointer(&type->offsets, object), 1);
            }
            else
            {
                void (__cdecl *copy_ctor)(void*, void*) =
                    rva_to_ptr(type->copy_ctor, exc_base);
                copy_ctor(dest, get_this_pointer(&type->offsets, object));
            }
        }
        else
            memmove(dest, get_this_pointer(&type->offsets,object), type->size);
    }
}

static void cxx_local_unwind(ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
                             const cxx_function_descr *descr, int last_level)
{
    const unwind_info *unwind_table = rva_to_ptr(descr->unwind_table, dispatch->ImageBase);
    void (__cdecl *handler)(ULONG64 unk, ULONG64 rbp);
    int *unwind_help = rva_to_ptr(descr->unwind_help, frame);
    int trylevel;

    if (unwind_help[0] == -2)
    {
        trylevel = ip_to_state(rva_to_ptr(descr->ipmap, dispatch->ImageBase),
                descr->ipmap_count, dispatch->ControlPc-dispatch->ImageBase);
    }
    else
    {
        trylevel = unwind_help[0];
    }

    TRACE("current level: %d, last level: %d\n", trylevel, last_level);
    while (trylevel > last_level)
    {
        if (trylevel<0 || trylevel>=descr->unwind_count)
        {
            ERR("invalid trylevel %d\n", trylevel);
            terminate();
        }
        handler = rva_to_ptr(unwind_table[trylevel].handler, dispatch->ImageBase);
        if (handler)
        {
            TRACE("handler: %p\n", handler);
            handler(0, frame);
        }
        trylevel = unwind_table[trylevel].prev;
    }
    unwind_help[0] = trylevel;
}

static LONG CALLBACK cxx_rethrow_filter(PEXCEPTION_POINTERS eptrs, void *c)
{
    EXCEPTION_RECORD *rec = eptrs->ExceptionRecord;
    cxx_catch_ctx *ctx = c;

    if (rec->ExceptionCode != CXX_EXCEPTION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2])
        return EXCEPTION_EXECUTE_HANDLER;
    if (rec->ExceptionInformation[1] == ctx->prev_rec->ExceptionInformation[1])
        ctx->rethrow = TRUE;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void CALLBACK cxx_catch_cleanup(BOOL normal, void *c)
{
    cxx_catch_ctx *ctx = c;
    __CxxUnregisterExceptionObject(&ctx->frame_info, ctx->rethrow);
}

static void* WINAPI call_catch_block(EXCEPTION_RECORD *rec)
{
    ULONG64 frame = rec->ExceptionInformation[1];
    const cxx_function_descr *descr = (void*)rec->ExceptionInformation[2];
    EXCEPTION_RECORD *prev_rec = (void*)rec->ExceptionInformation[4];
    EXCEPTION_RECORD *untrans_rec = (void*)rec->ExceptionInformation[6];
    CONTEXT *context = (void*)rec->ExceptionInformation[7];
    void* (__cdecl *handler)(ULONG64 unk, ULONG64 rbp) = (void*)rec->ExceptionInformation[5];
    int *unwind_help = rva_to_ptr(descr->unwind_help, frame);
    EXCEPTION_POINTERS ep = { prev_rec, context };
    cxx_catch_ctx ctx;
    void *ret_addr = NULL;

    TRACE("calling handler %p\n", handler);

    ctx.rethrow = FALSE;
    ctx.prev_rec = prev_rec;
    __CxxRegisterExceptionObject(&ep, &ctx.frame_info);
    msvcrt_get_thread_data()->processing_throw--;
    __TRY
    {
        __TRY
        {
            ret_addr = handler(0, frame);
        }
        __EXCEPT_CTX(cxx_rethrow_filter, &ctx)
        {
            TRACE("detect rethrow: exception code: %x\n", prev_rec->ExceptionCode);
            ctx.rethrow = TRUE;

            if (untrans_rec)
            {
                __DestructExceptionObject(prev_rec);
                RaiseException(untrans_rec->ExceptionCode, untrans_rec->ExceptionFlags,
                        untrans_rec->NumberParameters, untrans_rec->ExceptionInformation);
            }
            else
            {
                RaiseException(prev_rec->ExceptionCode, prev_rec->ExceptionFlags,
                        prev_rec->NumberParameters, prev_rec->ExceptionInformation);
            }
        }
        __ENDTRY
    }
    __FINALLY_CTX(cxx_catch_cleanup, &ctx)

    unwind_help[0] = -2;
    unwind_help[1] = -1;
    return ret_addr;
}

static inline BOOL cxx_is_consolidate(const EXCEPTION_RECORD *rec)
{
    return rec->ExceptionCode==STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters==8 &&
           rec->ExceptionInformation[0]==(ULONG_PTR)call_catch_block;
}

static inline void find_catch_block(EXCEPTION_RECORD *rec, CONTEXT *context,
                                    EXCEPTION_RECORD *untrans_rec,
                                    ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
                                    const cxx_function_descr *descr,
                                    cxx_exception_type *info, ULONG64 orig_frame)
{
    ULONG64 exc_base = (rec->NumberParameters == 4 ? rec->ExceptionInformation[3] : 0);
    int trylevel = ip_to_state(rva_to_ptr(descr->ipmap, dispatch->ImageBase),
            descr->ipmap_count, dispatch->ControlPc-dispatch->ImageBase);
    thread_data_t *data = msvcrt_get_thread_data();
    const tryblock_info *in_catch;
    EXCEPTION_RECORD catch_record;
    CONTEXT ctx;
    UINT i, j;
    INT *unwind_help;

    data->processing_throw++;
    for (i=descr->tryblock_count; i>0; i--)
    {
        in_catch = rva_to_ptr(descr->tryblock, dispatch->ImageBase);
        in_catch = &in_catch[i-1];

        if (trylevel>in_catch->end_level && trylevel<=in_catch->catch_level)
            break;
    }
    if (!i)
        in_catch = NULL;

    unwind_help = rva_to_ptr(descr->unwind_help, orig_frame);
    if (trylevel > unwind_help[1])
        unwind_help[0] = unwind_help[1] = trylevel;
    else
        trylevel = unwind_help[1];
    TRACE("current trylevel: %d\n", trylevel);

    for (i=0; i<descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = rva_to_ptr(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        if (in_catch)
        {
            if(tryblock->start_level <= in_catch->end_level) continue;
            if(tryblock->end_level > in_catch->catch_level) continue;
        }

        /* got a try block */
        for (j=0; j<tryblock->catchblock_count; j++)
        {
            const catchblock_info *catchblock = rva_to_ptr(tryblock->catchblock, dispatch->ImageBase);
            catchblock = &catchblock[j];

            if (info)
            {
                const cxx_type_info *type = find_caught_type(info, exc_base,
                        rva_to_ptr(catchblock->type_info, dispatch->ImageBase),
                        catchblock->flags);
                if (!type) continue;

                TRACE("matched type %p in tryblock %d catchblock %d\n", type, i, j);

                /* copy the exception to its destination on the stack */
                copy_exception((void*)rec->ExceptionInformation[1],
                        orig_frame, dispatch, catchblock, type, exc_base);
            }
            else
            {
                /* no CXX_EXCEPTION only proceed with a catch(...) block*/
                if (catchblock->type_info)
                    continue;
                TRACE("found catch(...) block\n");
            }

            /* unwind stack and call catch */
            memset(&catch_record, 0, sizeof(catch_record));
            catch_record.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
            catch_record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            catch_record.NumberParameters = 8;
            catch_record.ExceptionInformation[0] = (ULONG_PTR)call_catch_block;
            catch_record.ExceptionInformation[1] = orig_frame;
            catch_record.ExceptionInformation[2] = (ULONG_PTR)descr;
            catch_record.ExceptionInformation[3] = tryblock->start_level;
            catch_record.ExceptionInformation[4] = (ULONG_PTR)rec;
            catch_record.ExceptionInformation[5] =
                (ULONG_PTR)rva_to_ptr(catchblock->handler, dispatch->ImageBase);
            catch_record.ExceptionInformation[6] = (ULONG_PTR)untrans_rec;
            catch_record.ExceptionInformation[7] = (ULONG_PTR)context;
            RtlUnwindEx((void*)frame, (void*)dispatch->ControlPc, &catch_record, NULL, &ctx, NULL);
        }
    }

    TRACE("no matching catch block found\n");
    data->processing_throw--;
}

static LONG CALLBACK se_translation_filter(EXCEPTION_POINTERS *ep, void *c)
{
    se_translator_ctx *ctx = (se_translator_ctx *)c;
    EXCEPTION_RECORD *rec = ep->ExceptionRecord;
    cxx_exception_type *exc_type;

    if (rec->ExceptionCode != CXX_EXCEPTION)
    {
        TRACE("non-c++ exception thrown in SEH handler: %x\n", rec->ExceptionCode);
        terminate();
    }

    exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];
    find_catch_block(rec, ep->ContextRecord, ctx->seh_rec, ctx->dest_frame, ctx->dispatch,
                     ctx->descr, exc_type, ctx->orig_frame);

    __DestructExceptionObject(rec);
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

static DWORD cxx_frame_handler(EXCEPTION_RECORD *rec, ULONG64 frame,
                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                               const cxx_function_descr *descr)
{
    int trylevel = ip_to_state(rva_to_ptr(descr->ipmap, dispatch->ImageBase),
            descr->ipmap_count, dispatch->ControlPc-dispatch->ImageBase);
    cxx_exception_type *exc_type;
    ULONG64 orig_frame = frame;
    ULONG64 throw_base;
    DWORD throw_func_off;
    void *throw_func;
    UINT i, j;
    int unwindlevel = -1;

    if (descr->magic<CXX_FRAME_MAGIC_VC6 || descr->magic>CXX_FRAME_MAGIC_VC8)
    {
        FIXME("unhandled frame magic %x\n", descr->magic);
        return ExceptionContinueSearch;
    }

    if (descr->magic >= CXX_FRAME_MAGIC_VC8 &&
        (descr->flags & FUNC_DESCR_SYNCHRONOUS) &&
        (rec->ExceptionCode != CXX_EXCEPTION &&
        !cxx_is_consolidate(rec) &&
        rec->ExceptionCode != STATUS_LONGJUMP))
        return ExceptionContinueSearch;  /* handle only c++ exceptions */

    /* update orig_frame if it's a nested exception */
    throw_func_off = RtlLookupFunctionEntry(dispatch->ControlPc, &throw_base, NULL)->BeginAddress;
    throw_func = rva_to_ptr(throw_func_off, throw_base);
    TRACE("reconstructed handler pointer: %p\n", throw_func);
    for (i=descr->tryblock_count; i>0; i--)
    {
        const tryblock_info *tryblock = rva_to_ptr(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i-1];

        if (trylevel>tryblock->end_level && trylevel<=tryblock->catch_level)
        {
            for (j=0; j<tryblock->catchblock_count; j++)
            {
                const catchblock_info *catchblock = rva_to_ptr(tryblock->catchblock, dispatch->ImageBase);
                catchblock = &catchblock[j];

                if (rva_to_ptr(catchblock->handler, dispatch->ImageBase) == throw_func)
                {
                    TRACE("nested exception detected\n");
                    unwindlevel = tryblock->end_level;
                    orig_frame = *(ULONG64*)rva_to_ptr(catchblock->frame, frame);
                    TRACE("setting orig_frame to %lx\n", orig_frame);
                }
            }
        }
    }

    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
    {
        if (rec->ExceptionFlags & EH_TARGET_UNWIND)
            cxx_local_unwind(orig_frame, dispatch, descr,
                cxx_is_consolidate(rec) ? rec->ExceptionInformation[3] : trylevel);
        else
            cxx_local_unwind(orig_frame, dispatch, descr, unwindlevel);
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count)
    {
        check_noexcept(rec, descr, orig_frame != frame);
        return ExceptionContinueSearch;
    }

    if (rec->ExceptionCode == CXX_EXCEPTION)
    {
        if (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2])
        {
            TRACE("rethrow detected.\n");
            *rec = *msvcrt_get_thread_data()->exc_record;
        }

        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %lx descr %p\n", rec, frame,  descr);
            dump_exception_type(exc_type, rec->ExceptionInformation[3]);
            dump_function_descr(descr, dispatch->ImageBase);
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %x rec %p frame %lx descr %p\n",
                rec->ExceptionCode, rec, frame, descr);

        if (data->se_translator) {
            EXCEPTION_POINTERS except_ptrs;
            se_translator_ctx ctx;

            ctx.dest_frame = frame;
            ctx.orig_frame = orig_frame;
            ctx.seh_rec    = rec;
            ctx.dispatch   = dispatch;
            ctx.descr      = descr;
            __TRY
            {
                except_ptrs.ExceptionRecord = rec;
                except_ptrs.ContextRecord = context;
                data->se_translator(rec->ExceptionCode, &except_ptrs);
            }
            __EXCEPT_CTX(se_translation_filter, &ctx)
            {
            }
            __ENDTRY
        }
    }

    find_catch_block(rec, context, NULL, frame, dispatch, descr, exc_type, orig_frame);
    check_noexcept(rec, descr, orig_frame != frame);
    return ExceptionContinueSearch;
}

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( PEXCEPTION_POINTERS ptrs,
                                const type_info *ti, int flags, void **copy )
{
    FIXME( "%p %p %x %p: not implemented\n", ptrs, ti, flags, copy );
    return EXCEPTION_CONTINUE_SEARCH;
}

/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
EXCEPTION_DISPOSITION CDECL __CxxFrameHandler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "%p %lx %p %p\n", rec, frame, context, dispatch );
    return cxx_frame_handler( rec, frame, context, dispatch,
            rva_to_ptr(*(UINT*)dispatch->HandlerData, dispatch->ImageBase) );
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
        rec->NumberParameters == 4 &&
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


#ifndef __REACTOS__
/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( MSVCRT__setjmp,
                   "jmp " __ASM_NAME("__wine_setjmpex") );
#endif

/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
void __cdecl MSVCRT_longjmp( _JUMP_BUFFER *jmp, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;
    if (jmp->Frame)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.ExceptionFlags = 0;
        rec.ExceptionRecord = NULL;
        rec.ExceptionAddress = NULL;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD_PTR)jmp;
        RtlUnwind( (void *)jmp->Frame, (void *)jmp->Rip, &rec, IntToPtr(retval) );
    }
    __wine_longjmp( (__wine_jmp_buf *)jmp, retval );
}

#ifndef __REACTOS__ // different file for ntdll
/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    RtlUnwind( frame, target, NULL, 0 );
}
#endif /* __REACTOS__ */

/*********************************************************************
 *              _fpieee_flt (MSVCRT.@)
 */
int __cdecl _fpieee_flt(__msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
        int (__cdecl *handler)(_FPIEEE_RECORD*))
{
    FIXME("(%lx %p %p) opcode: %s\n", exception_code, ep, handler,
            wine_dbgstr_longlong(*(ULONG64*)ep->ContextRecord->Rip));
    return EXCEPTION_CONTINUE_SEARCH;
}

#if _MSVCR_VER>=110 && _MSVCR_VER<=120
/*********************************************************************
 *  __crtCapturePreviousContext (MSVCR110.@)
 */
void __cdecl get_prev_context(CONTEXT *ctx, DWORD64 rip)
{
    ULONG64 frame, image_base;
    RUNTIME_FUNCTION *rf;
    void *data;

    TRACE("(%p)\n", ctx);

    rf = RtlLookupFunctionEntry(ctx->Rip, &image_base, NULL);
    if(!rf) {
        FIXME("RtlLookupFunctionEntry failed\n");
        return;
    }

    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, ctx->Rip,
            rf, ctx, &data, &frame, NULL);
}

__ASM_GLOBAL_FUNC( __crtCapturePreviousContext,
                   "movq %rcx,8(%rsp)\n\t"
                   "call " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "movq 8(%rsp),%rcx\n\t"     /* context */
                   "leaq 8(%rsp),%rax\n\t"
                   "movq %rax,0x98(%rcx)\n\t"  /* context->Rsp */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0xf8(%rcx)\n\t"  /* context->Rip */
                   "jmp " __ASM_NAME("get_prev_context") )
#endif

#endif  /* __x86_64__ */
