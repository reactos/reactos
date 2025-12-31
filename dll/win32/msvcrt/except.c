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

#include <float.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wincon.h"
#include "wine/exception.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#if _MSVCR_VER>=70 && _MSVCR_VER<=71
static MSVCRT_security_error_handler security_error_handler;
#endif

static __sighandler_t sighandlers[NSIG] = { SIG_DFL };

void dump_function_descr( const cxx_function_descr *descr, uintptr_t base )
{
    unwind_info *unwind_table = rtti_rva( descr->unwind_table, base );
    tryblock_info *tryblock = rtti_rva( descr->tryblock, base );
    ipmap_info *ipmap = rtti_rva( descr->ipmap, base );
    UINT i, j;

    TRACE( "magic %x\n", descr->magic );
    TRACE( "unwind table: %p %d\n", unwind_table, descr->unwind_count );
    for (i = 0; i < descr->unwind_count; i++)
    {
        TRACE("    %d: prev %d func %p\n", i, unwind_table[i].prev,
              unwind_table[i].handler ? rtti_rva( unwind_table[i].handler, base ) : NULL );
    }
    TRACE( "try table: %p %d\n", tryblock, descr->tryblock_count );
    for (i = 0; i < descr->tryblock_count; i++)
    {
        catchblock_info *catchblock = rtti_rva( tryblock[i].catchblock, base );

        TRACE( "    %d: start %d end %d catchlevel %d catch %p %d\n", i,
               tryblock[i].start_level, tryblock[i].end_level,
               tryblock[i].catch_level, catchblock, tryblock[i].catchblock_count);
        for (j = 0; j < tryblock[i].catchblock_count; j++)
        {
            type_info *type_info = catchblock[j].type_info ? rtti_rva( catchblock[j].type_info, base ) : NULL;
            TRACE( "        %d: flags %x offset %d handler %p",
                   j, catchblock[j].flags, catchblock[j].offset,
                   catchblock[j].handler ? rtti_rva(catchblock[j].handler, base) : NULL );
#ifdef _WIN64
            TRACE( " frame %x", catchblock[j].frame );
#endif
            TRACE( " type %p %s\n", type_info, dbgstr_type_info(type_info) );
        }
    }
    TRACE( "ipmap: %p %d\n", ipmap, descr->ipmap_count );
    for (i = 0; i < descr->ipmap_count; i++)
        TRACE( "    %d: ip %x state %d\n", i, ipmap[i].ip, ipmap[i].state );
#ifdef RTTI_USE_RVA
    TRACE( "unwind_help %+d\n", descr->unwind_help );
#endif
    if (descr->magic <= CXX_FRAME_MAGIC_VC6) return;
    TRACE( "expect list: %p\n", rtti_rva( descr->expect_list, base ) );
    if (descr->magic <= CXX_FRAME_MAGIC_VC7) return;
    TRACE( "flags: %08x\n", descr->flags );
}

void *find_catch_handler( void *object, uintptr_t frame, uintptr_t exc_base,
                          const tryblock_info *tryblock,
                          cxx_exception_type *exc_type, uintptr_t image_base )
{
    unsigned int i;
    const catchblock_info *catchblock = rtti_rva( tryblock->catchblock, image_base );
    const cxx_type_info *type;
    const type_info *catch_ti;

    for (i = 0; i < tryblock->catchblock_count; i++)
    {
        if (exc_type)
        {
            catch_ti = catchblock[i].type_info ? rtti_rva( catchblock[i].type_info, image_base ) : NULL;
            type = find_caught_type( exc_type, exc_base, catch_ti, catchblock[i].flags );
            if (!type) continue;

            TRACE( "matched type %p in catchblock %d\n", type, i );

            if (catch_ti && catch_ti->mangled[0] && catchblock[i].offset)
            {
                /* copy the exception to its destination on the stack */
                void **dest = (void **)(frame + catchblock[i].offset);
                copy_exception( object, dest, catchblock[i].flags, type, exc_base );
            }
        }
        else
        {
            /* no CXX_EXCEPTION only proceed with a catch(...) block*/
            if (catchblock[i].type_info) continue;
            TRACE( "found catch(...) block\n" );
        }
        return rtti_rva( catchblock[i].handler, image_base );
    }
    return NULL;
}

#ifndef __i386__  /* i386 implementation is in except_i386.c */

typedef struct
{
    cxx_frame_info frame_info;
    BOOL rethrow;
    EXCEPTION_RECORD *prev_rec;
} cxx_catch_ctx;

typedef struct
{
    ULONG_PTR dest_frame;
    ULONG_PTR orig_frame;
    EXCEPTION_RECORD *seh_rec;
    DISPATCHER_CONTEXT *dispatch;
    const cxx_function_descr *descr;
} se_translator_ctx;

static inline int ip_to_state( const cxx_function_descr *descr, uintptr_t ip, uintptr_t base )
{
    const ipmap_info *ipmap = rtti_rva( descr->ipmap, base );
    unsigned int i;
    int ret;

    for (i = 0; i < descr->ipmap_count; i++) if (base + ipmap[i].ip > ip) break;
    ret = i ? ipmap[i - 1].state : -1;
    TRACE( "%Ix -> %d\n", ip, ret );
    return ret;
}

static void cxx_local_unwind(ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch,
                             const cxx_function_descr *descr, int last_level)
{
    const unwind_info *unwind_table = rtti_rva(descr->unwind_table, dispatch->ImageBase);
    int *unwind_help = (int *)(frame + descr->unwind_help);
    int trylevel = unwind_help[0];

    if (trylevel == -2) trylevel = ip_to_state( descr, get_exception_pc(dispatch), dispatch->ImageBase );

    TRACE("current level: %d, last level: %d\n", trylevel, last_level);
    while (trylevel > last_level)
    {
        if (trylevel<0 || trylevel>=descr->unwind_count)
        {
            ERR("invalid trylevel %d\n", trylevel);
            terminate();
        }
        if (unwind_table[trylevel].handler)
        {
            void *handler = rtti_rva( unwind_table[trylevel].handler, dispatch->ImageBase );
            call_unwind_handler( handler, frame, dispatch );
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
    ULONG_PTR frame = rec->ExceptionInformation[1];
    const cxx_function_descr *descr = (void*)rec->ExceptionInformation[2];
    EXCEPTION_RECORD *untrans_rec = (void*)rec->ExceptionInformation[4];
    EXCEPTION_RECORD *prev_rec = (void*)rec->ExceptionInformation[6];
    CONTEXT *context = (void*)rec->ExceptionInformation[7];
    int *unwind_help = (int *)(frame + descr->unwind_help);
    EXCEPTION_POINTERS ep = { prev_rec, context };
    cxx_catch_ctx ctx;
    void *ret_addr = NULL;

    ctx.rethrow = FALSE;
    ctx.prev_rec = prev_rec;
    __CxxRegisterExceptionObject(&ep, &ctx.frame_info);
    msvcrt_get_thread_data()->processing_throw--;
    __TRY
    {
        __TRY
        {
            ret_addr = call_catch_handler( rec );
        }
        __EXCEPT_CTX(cxx_rethrow_filter, &ctx)
        {
            TRACE("detect rethrow: exception code: %lx\n", prev_rec->ExceptionCode);
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
    return (rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE &&
            rec->NumberParameters > 10 &&
            rec->ExceptionInformation[0] == (ULONG_PTR)call_catch_block);
}

static inline void find_catch_block(EXCEPTION_RECORD *rec, CONTEXT *context,
                                    EXCEPTION_RECORD *untrans_rec,
                                    ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch,
                                    const cxx_function_descr *descr,
                                    cxx_exception_type *info, ULONG_PTR orig_frame)
{
    ULONG_PTR exc_base = (rec->NumberParameters == 4 ? rec->ExceptionInformation[3] : 0);
    void *handler, *object = (void *)rec->ExceptionInformation[1];
    int trylevel = ip_to_state( descr, get_exception_pc(dispatch), dispatch->ImageBase );
    thread_data_t *data = msvcrt_get_thread_data();
    const tryblock_info *in_catch;
    EXCEPTION_RECORD catch_record;
    CONTEXT ctx;
    UINT i;
    INT *unwind_help;

    data->processing_throw++;
    for (i=descr->tryblock_count; i>0; i--)
    {
        in_catch = rtti_rva(descr->tryblock, dispatch->ImageBase);
        in_catch = &in_catch[i-1];

        if (trylevel>in_catch->end_level && trylevel<=in_catch->catch_level)
            break;
    }
    if (!i)
        in_catch = NULL;

    unwind_help = (int *)(orig_frame + descr->unwind_help);
    if (trylevel > unwind_help[1])
        unwind_help[0] = unwind_help[1] = trylevel;
    else
        trylevel = unwind_help[1];
    TRACE("current trylevel: %d\n", trylevel);

    for (i=0; i<descr->tryblock_count; i++)
    {
        const tryblock_info *tryblock = rtti_rva(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        if (in_catch)
        {
            if(tryblock->start_level <= in_catch->end_level) continue;
            if(tryblock->end_level > in_catch->catch_level) continue;
        }

        handler = find_catch_handler( object, orig_frame, exc_base, tryblock, info, dispatch->ImageBase );
        if (!handler) continue;

        /* unwind stack and call catch */
        memset(&catch_record, 0, sizeof(catch_record));
        catch_record.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        catch_record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        catch_record.NumberParameters = 11;
        catch_record.ExceptionInformation[0] = (ULONG_PTR)call_catch_block;
        catch_record.ExceptionInformation[1] = orig_frame;
        catch_record.ExceptionInformation[2] = (ULONG_PTR)descr;
        catch_record.ExceptionInformation[3] = tryblock->start_level;
        catch_record.ExceptionInformation[4] = (ULONG_PTR)untrans_rec;
        catch_record.ExceptionInformation[5] = (ULONG_PTR)handler;
        /* tyFlow expects ExceptionInformation[6] to contain exception record */
        catch_record.ExceptionInformation[6] = (ULONG_PTR)rec;
        catch_record.ExceptionInformation[7] = (ULONG_PTR)context;
        catch_record.ExceptionInformation[10] = -1;
        RtlUnwindEx((void*)frame, (void*)dispatch->ControlPc, &catch_record, NULL, &ctx, NULL);
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
        TRACE("non-c++ exception thrown in SEH handler: %lx\n", rec->ExceptionCode);
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

static DWORD cxx_frame_handler(EXCEPTION_RECORD *rec, ULONG_PTR frame,
                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                               const cxx_function_descr *descr)
{
    int trylevel = ip_to_state( descr, get_exception_pc(dispatch), dispatch->ImageBase );
    cxx_exception_type *exc_type;
    ULONG_PTR orig_frame = frame;
    ULONG_PTR throw_base;
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
    throw_func = rtti_rva(throw_func_off, throw_base);
    TRACE("reconstructed handler pointer: %p\n", throw_func);
    for (i=descr->tryblock_count; i>0; i--)
    {
        const tryblock_info *tryblock = rtti_rva(descr->tryblock, dispatch->ImageBase);
        tryblock = &tryblock[i-1];

        if (trylevel>tryblock->end_level && trylevel<=tryblock->catch_level)
        {
            for (j=0; j<tryblock->catchblock_count; j++)
            {
                const catchblock_info *catchblock = rtti_rva(tryblock->catchblock, dispatch->ImageBase);
                catchblock = &catchblock[j];

                if (rtti_rva(catchblock->handler, dispatch->ImageBase) == throw_func)
                {
                    unwindlevel = tryblock->end_level;
#ifdef _WIN64
                    orig_frame = *(ULONG_PTR *)(frame + catchblock->frame);
#else
                    orig_frame = *(ULONG_PTR *)frame;
#endif
                    TRACE("nested exception detected, setting orig_frame to %Ix\n", orig_frame);
                }
            }
        }
    }

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND))
    {
        if (rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND)
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

    if (rec->ExceptionCode == CXX_EXCEPTION &&
        (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2]))
    {
        TRACE("rethrow detected.\n");
        *rec = *msvcrt_get_thread_data()->exc_record;
    }
    if (rec->ExceptionCode == CXX_EXCEPTION)
    {
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %Ix descr %p\n", rec, frame,  descr);
            TRACE_EXCEPTION_TYPE(exc_type, rec->ExceptionInformation[3]);
            dump_function_descr(descr, dispatch->ImageBase);
        }
    }
    else
    {
        thread_data_t *data = msvcrt_get_thread_data();

        exc_type = NULL;
        TRACE("handling C exception code %lx rec %p frame %Ix descr %p\n",
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
 *		__CxxFrameHandler (MSVCRT.@)
 */
EXCEPTION_DISPOSITION CDECL __CxxFrameHandler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "%p %Ix %p %p\n", rec, frame, context, dispatch );
    return cxx_frame_handler( rec, frame, context, dispatch,
                              rtti_rva(*(UINT *)dispatch->HandlerData, dispatch->ImageBase) );
}

#endif  /* __i386__ */

static BOOL WINAPI msvcrt_console_handler(DWORD ctrlType)
{
    BOOL ret = FALSE;

    switch (ctrlType)
    {
    case CTRL_C_EVENT:
        if (sighandlers[SIGINT])
        {
            if (sighandlers[SIGINT] != SIG_IGN)
                sighandlers[SIGINT](SIGINT);
            ret = TRUE;
        }
        break;
    }
    return ret;
}

/*********************************************************************
 *              __pxcptinfoptrs (MSVCRT.@)
 */
void** CDECL __pxcptinfoptrs(void)
{
    return (void**)&msvcrt_get_thread_data()->xcptinfo;
}

typedef void (CDECL *float_handler)(int, int);

/* The exception codes are actually NTSTATUS values */
static const struct
{
    NTSTATUS status;
    int signal;
} float_exception_map[] = {
 { EXCEPTION_FLT_DENORMAL_OPERAND, _FPE_DENORMAL },
 { EXCEPTION_FLT_DIVIDE_BY_ZERO, _FPE_ZERODIVIDE },
 { EXCEPTION_FLT_INEXACT_RESULT, _FPE_INEXACT },
 { EXCEPTION_FLT_INVALID_OPERATION, _FPE_INVALID },
 { EXCEPTION_FLT_OVERFLOW, _FPE_OVERFLOW },
 { EXCEPTION_FLT_STACK_CHECK, _FPE_STACKOVERFLOW },
 { EXCEPTION_FLT_UNDERFLOW, _FPE_UNDERFLOW },
};

static LONG msvcrt_exception_filter(struct _EXCEPTION_POINTERS *except)
{
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    __sighandler_t handler;

    if (!except || !except->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if ((handler = sighandlers[SIGSEGV]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

                old_ep = *ep;
                *ep = except;
                sighandlers[SIGSEGV] = SIG_DFL;
                handler(SIGSEGV);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    /* According to msdn,
     * the FPE signal handler takes as a second argument the type of
     * floating point exception.
     */
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        if ((handler = sighandlers[SIGFPE]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;
                unsigned int i;
                int float_signal = _FPE_INVALID;

                sighandlers[SIGFPE] = SIG_DFL;
                for (i = 0; i < ARRAY_SIZE(float_exception_map); i++)
                {
                    if (float_exception_map[i].status ==
                        except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                }

                old_ep = *ep;
                *ep = except;
                ((float_handler)handler)(SIGFPE, float_signal);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
        if ((handler = sighandlers[SIGILL]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

                old_ep = *ep;
                *ep = except;
                sighandlers[SIGILL] = SIG_DFL;
                handler(SIGILL);
                *ep = old_ep;
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    }
    return ret;
}

void msvcrt_init_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, TRUE);
}

void msvcrt_free_signals(void)
{
    SetConsoleCtrlHandler(msvcrt_console_handler, FALSE);
}

/*********************************************************************
 *		signal (MSVCRT.@)
 * Some signals may never be generated except through an explicit call to
 * raise.
 */
__sighandler_t CDECL signal(int sig, __sighandler_t func)
{
    __sighandler_t ret = SIG_ERR;

    TRACE("(%d, %p)\n", sig, func);

    if (func == SIG_ERR) return SIG_ERR;

    switch (sig)
    {
    /* Cases handled internally.  Note SIGTERM is never generated by Windows,
     * so we effectively mask it.
     */
    case SIGABRT:
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGINT:
    case SIGTERM:
    case SIGBREAK:
        ret = sighandlers[sig];
        sighandlers[sig] = func;
        break;
    default:
        ret = SIG_ERR;
    }
    return ret;
}

/*********************************************************************
 *		raise (MSVCRT.@)
 */
int CDECL raise(int sig)
{
    __sighandler_t handler;

    TRACE("(%d)\n", sig);

    switch (sig)
    {
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
        handler = sighandlers[sig];
        if (handler == SIG_DFL) _exit(3);
        if (handler != SIG_IGN)
        {
            EXCEPTION_POINTERS **ep = (EXCEPTION_POINTERS**)__pxcptinfoptrs(), *old_ep;

            sighandlers[sig] = SIG_DFL;

            old_ep = *ep;
            *ep = NULL;
            if (sig == SIGFPE)
                ((float_handler)handler)(sig, _FPE_EXPLICITGEN);
            else
                handler(sig);
            *ep = old_ep;
        }
        break;
    case SIGABRT:
    case SIGINT:
    case SIGTERM:
    case SIGBREAK:
        handler = sighandlers[sig];
        if (handler == SIG_DFL) _exit(3);
        if (handler != SIG_IGN)
        {
            sighandlers[sig] = SIG_DFL;
            handler(sig);
        }
        break;
    default:
        return -1;
    }
    return 0;
}

/*********************************************************************
 *		_XcptFilter (MSVCRT.@)
 */
int CDECL _XcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    TRACE("(%08lx,%p)\n", ex, ptr);
    /* I assume ptr->ExceptionRecord->ExceptionCode is the same as ex */
    return msvcrt_exception_filter(ptr);
}

/*********************************************************************
 *		__CppXcptFilter (MSVCRT.@)
 */
int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
    /* only filter c++ exceptions */
    if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
    return _XcptFilter(ex, ptr);
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
    if (is_cxx_exception( rec ) && rec->ExceptionInformation[2])
    {
        ptrs->ExceptionRecord = msvcrt_get_thread_data()->exc_record;
        return TRUE;
    }
    return (msvcrt_get_thread_data()->exc_record == rec);
}

/*********************************************************************
 *		__CxxExceptionFilter (MSVCRT.@)
 */
int CDECL __CxxExceptionFilter( EXCEPTION_POINTERS *ptrs, const type_info *ti, UINT flags, void **copy)
{
    const cxx_type_info *type;
    EXCEPTION_RECORD *rec;
    uintptr_t exc_base;

    TRACE( "%p %p %x %p\n", ptrs, ti, flags, copy );

    if (!ptrs) return EXCEPTION_CONTINUE_SEARCH;

    /* handle catch(...) */
    if (!ti) return EXCEPTION_EXECUTE_HANDLER;

    rec = ptrs->ExceptionRecord;
    if (!is_cxx_exception( rec )) return EXCEPTION_CONTINUE_SEARCH;

    if (rec->ExceptionInformation[1] == 0 && rec->ExceptionInformation[2] == 0)
    {
        rec = msvcrt_get_thread_data()->exc_record;
        if (!rec) return EXCEPTION_CONTINUE_SEARCH;
    }

    exc_base = rec->ExceptionInformation[3];
    type = find_caught_type( (cxx_exception_type *)rec->ExceptionInformation[2], exc_base, ti, flags );
    if (!type) return EXCEPTION_CONTINUE_SEARCH;

    if (copy) copy_exception( (void *)rec->ExceptionInformation[1], copy, flags, type, exc_base );

    return EXCEPTION_EXECUTE_HANDLER;
}

/*********************************************************************
 *		__CxxQueryExceptionSize (MSVCRT.@)
 */
unsigned int CDECL __CxxQueryExceptionSize(void)
{
    return sizeof(cxx_exception_type);
}

/*********************************************************************
 *		_abnormal_termination (MSVCRT.@)
 */
int CDECL __intrinsic_abnormal_termination(void)
{
  FIXME("(void)stub\n");
  return 0;
}

/******************************************************************
 *		__uncaught_exception (MSVCRT.@)
 */
BOOL CDECL __uncaught_exception(void)
{
    return msvcrt_get_thread_data()->processing_throw != 0;
}

/*********************************************************************
 *              _fpieee_flt (MSVCRT.@)
 */
int __cdecl _fpieee_flt( __msvcrt_ulong code, EXCEPTION_POINTERS *ep,
                         int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    switch (code)
    {
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
    case STATUS_FLOAT_INEXACT_RESULT:
    case STATUS_FLOAT_INVALID_OPERATION:
    case STATUS_FLOAT_OVERFLOW:
    case STATUS_FLOAT_UNDERFLOW:
        return handle_fpieee_flt( code, ep, handler );
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

#if _MSVCR_VER>=70 && _MSVCR_VER<=71

/*********************************************************************
 *		_set_security_error_handler (MSVCR70.@)
 */
MSVCRT_security_error_handler CDECL _set_security_error_handler(
    MSVCRT_security_error_handler handler )
{
    MSVCRT_security_error_handler old = security_error_handler;

    TRACE("(%p)\n", handler);

    security_error_handler = handler;
    return old;
}

/*********************************************************************
 *		__security_error_handler (MSVCR70.@)
 */
void CDECL __security_error_handler(int code, void *data)
{
    if(security_error_handler)
        security_error_handler(code, data);
    else
        FIXME("(%d, %p) stub\n", code, data);

    _exit(3);
}

#endif /* _MSVCR_VER>=70 && _MSVCR_VER<=71 */

#if _MSVCR_VER>=110
/*********************************************************************
 *  __crtSetUnhandledExceptionFilter (MSVCR110.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER CDECL __crtSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
{
    return SetUnhandledExceptionFilter(filter);
}
#endif

/*********************************************************************
 * _CreateFrameInfo (MSVCR80.@)
 */
frame_info* CDECL _CreateFrameInfo(frame_info *fi, void *obj)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p, %p)\n", fi, obj);

    fi->next = data->frame_info_head;
    data->frame_info_head = fi;
    fi->object = obj;
    return fi;
}

/*********************************************************************
 * _FindAndUnlinkFrame (MSVCR80.@)
 */
void CDECL _FindAndUnlinkFrame(frame_info *fi)
{
    thread_data_t *data = msvcrt_get_thread_data();
    frame_info *cur = data->frame_info_head;

    TRACE("(%p)\n", fi);

    if (cur == fi)
    {
        data->frame_info_head = cur->next;
        return;
    }

    for (; cur->next; cur = cur->next)
    {
        if (cur->next == fi)
        {
            cur->next = fi->next;
            return;
        }
    }

    ERR("frame not found, native crashes in this case\n");
}

/*********************************************************************
 *              _IsExceptionObjectToBeDestroyed (MSVCR80.@)
 */
BOOL __cdecl _IsExceptionObjectToBeDestroyed(const void *obj)
{
    frame_info *cur;

    TRACE( "%p\n", obj );

    for (cur = msvcrt_get_thread_data()->frame_info_head; cur; cur = cur->next)
    {
        if (cur->object == obj)
            return FALSE;
    }

    return TRUE;
}

/*********************************************************************
 * __DestructExceptionObject (MSVCRT.@)
 */
void CDECL __DestructExceptionObject(EXCEPTION_RECORD *rec)
{
    cxx_exception_type *info = (cxx_exception_type*) rec->ExceptionInformation[2];
    void *object = (void*)rec->ExceptionInformation[1];

    TRACE("(%p)\n", rec);

    if (!is_cxx_exception( rec )) return;

    if (!info || !info->destructor)
        return;

    call_dtor( rtti_rva( info->destructor, rec->ExceptionInformation[3] ), object );
}

/*********************************************************************
 *  __CxxRegisterExceptionObject (MSVCRT.@)
 */
BOOL CDECL __CxxRegisterExceptionObject(EXCEPTION_POINTERS *ep, cxx_frame_info *frame_info)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p, %p)\n", ep, frame_info);

    if (!ep || !ep->ExceptionRecord)
    {
        frame_info->rec = (void*)-1;
        frame_info->context = (void*)-1;
        return TRUE;
    }

    frame_info->rec = data->exc_record;
    frame_info->context = data->ctx_record;
    data->exc_record = ep->ExceptionRecord;
    data->ctx_record = ep->ContextRecord;
    _CreateFrameInfo(&frame_info->frame_info, (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    return TRUE;
}

/*********************************************************************
 *  __CxxUnregisterExceptionObject (MSVCRT.@)
 */
void CDECL __CxxUnregisterExceptionObject(cxx_frame_info *frame_info, BOOL in_use)
{
    thread_data_t *data = msvcrt_get_thread_data();

    TRACE("(%p)\n", frame_info);

    if(frame_info->rec == (void*)-1)
        return;

    _FindAndUnlinkFrame(&frame_info->frame_info);
    if(data->exc_record->ExceptionCode == CXX_EXCEPTION && !in_use
            && _IsExceptionObjectToBeDestroyed((void*)data->exc_record->ExceptionInformation[1]))
        __DestructExceptionObject(data->exc_record);
    data->exc_record = frame_info->rec;
    data->ctx_record = frame_info->context;
}

struct __std_exception_data {
    char *what;
    char dofree;
};

#if _MSVCR_VER>=140

/*********************************************************************
 *  __std_exception_copy (UCRTBASE.@)
 */
void CDECL __std_exception_copy(const struct __std_exception_data *src,
                                       struct __std_exception_data *dst)
{
    TRACE("(%p %p)\n", src, dst);

    if(src->dofree && src->what) {
        dst->what   = _strdup(src->what);
        dst->dofree = 1;
    } else {
        dst->what   = src->what;
        dst->dofree = 0;
    }
}

/*********************************************************************
 *  __std_exception_destroy (UCRTBASE.@)
 */
void CDECL __std_exception_destroy(struct __std_exception_data *data)
{
    TRACE("(%p)\n", data);

    if(data->dofree)
        free(data->what);
    data->what   = NULL;
    data->dofree = 0;
}

/*********************************************************************
 *  __current_exception (UCRTBASE.@)
 */
void** CDECL __current_exception(void)
{
    TRACE("()\n");
    return (void**)&msvcrt_get_thread_data()->exc_record;
}

/*********************************************************************
 *  __current_exception_context (UCRTBASE.@)
 */
void** CDECL __current_exception_context(void)
{
    TRACE("()\n");
    return (void**)&msvcrt_get_thread_data()->ctx_record;
}

/*********************************************************************
 *  __processing_throw (UCRTBASE.@)
 */
int* CDECL __processing_throw(void)
{
    TRACE("()\n");
    return &msvcrt_get_thread_data()->processing_throw;
}

#endif /* _MSVCR_VER>=140 */
