/*
 * C++ exception handling (ver. 4)
 *
 * Copyright 2020 Piotr Caban
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

#include <corecrt.h>

#if defined(__x86_64__) && _MSVCR_VER>=140

#include <stdarg.h>
#include <stdlib.h>

#include "wine/exception.h"
#include "wine/debug.h"
#include "cppexcept.h"
#include "msvcrt.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

static DWORD fls_index;

typedef struct
{
    BYTE header;
    UINT bbt_flags;
    UINT unwind_count;
    UINT unwind_map;
    UINT tryblock_count;
    UINT tryblock_map;
    UINT ip_count;
    UINT ip_map;
    UINT frame;
} cxx_function_descr_v4;
#define FUNC_DESCR_IS_CATCH     0x01
#define FUNC_DESCR_IS_SEPARATED 0x02
#define FUNC_DESCR_BBT          0x04
#define FUNC_DESCR_UNWIND_MAP   0x08
#define FUNC_DESCR_TRYBLOCK_MAP 0x10
#define FUNC_DESCR_EHS          0x20
#define FUNC_DESCR_NO_EXCEPT    0x40
#define FUNC_DESCR_RESERVED     0x80

typedef struct
{
    UINT type;
    BYTE *prev;
    UINT handler;
    UINT object;
} unwind_info_v4;

typedef struct
{
    BYTE header;
    UINT flags;
    UINT type_info;
    int offset;
    UINT handler;
    UINT ret_addr[2];
} catchblock_info_v4;
#define CATCHBLOCK_FLAGS     0x01
#define CATCHBLOCK_TYPE_INFO 0x02
#define CATCHBLOCK_OFFSET    0x04
#define CATCHBLOCK_SEPARATED 0x08
#define CATCHBLOCK_RET_ADDR_MASK 0x30
#define CATCHBLOCK_RET_ADDR      0x10
#define CATCHBLOCK_TWO_RET_ADDRS 0x20

#define UNWIND_TYPE_NO_HANDLER 0
#define UNWIND_TYPE_DTOR_OBJ   1
#define UNWIND_TYPE_DTOR_PTR   2
#define UNWIND_TYPE_FRAME      3

#define CONSOLIDATE_UNWIND_PARAMETER_COUNT 10

typedef struct
{
    cxx_frame_info frame_info;
    BOOL rethrow;
    INT search_state;
    INT unwind_state;
    EXCEPTION_RECORD *prev_rec;
} cxx_catch_ctx;

typedef struct
{
    ULONG64 dest_frame;
    ULONG64 orig_frame;
    EXCEPTION_RECORD *seh_rec;
    DISPATCHER_CONTEXT *dispatch;
    const cxx_function_descr_v4 *descr;
    int trylevel;
} se_translator_ctx;

static UINT decode_uint(BYTE **b)
{
    UINT ret;
    BYTE *p = *b;

    if ((*p & 1) == 0)
    {
        ret = p[0] >> 1;
        p += 1;
    }
    else if ((*p & 3) == 1)
    {
        ret = (p[0] >> 2) + (p[1] << 6);
        p += 2;
    }
    else if ((*p & 7) == 3)
    {
        ret = (p[0] >> 3) + (p[1] << 5) + (p[2] << 13);
        p += 3;
    }
    else if ((*p & 15) == 7)
    {
        ret = (p[0] >> 4) + (p[1] << 4) + (p[2] << 12) + (p[3] << 20);
        p += 4;
    }
    else
    {
        FIXME("not implemented - expect crash\n");
        ret = 0;
        p += 5;
    }

    *b = p;
    return ret;
}

static UINT read_rva(BYTE **b)
{
    UINT ret = *(UINT*)(*b);
    *b += sizeof(UINT);
    return ret;
}

static void read_unwind_info(BYTE **b, unwind_info_v4 *ui)
{
    BYTE *p = *b;

    memset(ui, 0, sizeof(*ui));
    ui->type = decode_uint(b);
    ui->prev = p - (ui->type >> 2);
    ui->type &= 0x3;

    switch (ui->type)
    {
    case UNWIND_TYPE_NO_HANDLER:
        break;
    case UNWIND_TYPE_DTOR_OBJ:
        ui->handler = read_rva(b);
        ui->object = decode_uint(b); /* frame offset to object */
        break;
    case UNWIND_TYPE_DTOR_PTR:
        ui->handler = read_rva(b);
        ui->object = decode_uint(b); /* frame offset to pointer to object */
        break;
    case UNWIND_TYPE_FRAME:
        ui->handler = read_rva(b);
        break;
    }
}

static void read_tryblock_info(BYTE **b, tryblock_info *ti, ULONG64 image_base)
{
    BYTE *count, *count_end;

    ti->start_level = decode_uint(b);
    ti->end_level = decode_uint(b);
    ti->catch_level = decode_uint(b);
    ti->catchblock = read_rva(b);

    if (ti->catchblock)
    {
        count = count_end = rtti_rva(ti->catchblock, image_base);
        ti->catchblock_count = decode_uint(&count_end);
        ti->catchblock += count_end - count;
    }
    else
    {
        ti->catchblock_count = 0;
    }
}

static BOOL read_catchblock_info(BYTE **b, catchblock_info_v4 *ci, DWORD func_rva)
{
    BYTE ret_addr_type;
    memset(ci, 0, sizeof(*ci));
    ci->header = **b;
    (*b)++;
    if (ci->header & ~(CATCHBLOCK_FLAGS | CATCHBLOCK_TYPE_INFO | CATCHBLOCK_OFFSET |
                CATCHBLOCK_SEPARATED | CATCHBLOCK_RET_ADDR_MASK))
    {
        FIXME("unknown header: %x\n", ci->header);
        return FALSE;
    }
    ret_addr_type = ci->header & CATCHBLOCK_RET_ADDR_MASK;
    if (ret_addr_type == (CATCHBLOCK_RET_ADDR | CATCHBLOCK_TWO_RET_ADDRS))
    {
        FIXME("unsupported ret addr type.\n");
        return FALSE;
    }

    if (ci->header & CATCHBLOCK_FLAGS) ci->flags = decode_uint(b);
    if (ci->header & CATCHBLOCK_TYPE_INFO) ci->type_info = read_rva(b);
    if (ci->header & CATCHBLOCK_OFFSET) ci->offset = decode_uint(b);
    ci->handler = read_rva(b);
    if (ci->header & CATCHBLOCK_SEPARATED)
    {
        if (ret_addr_type == CATCHBLOCK_RET_ADDR || ret_addr_type == CATCHBLOCK_TWO_RET_ADDRS)
            ci->ret_addr[0] = read_rva(b);
        if (ret_addr_type == CATCHBLOCK_TWO_RET_ADDRS)
            ci->ret_addr[1] = read_rva(b);
    }
    else
    {
        if (ret_addr_type == CATCHBLOCK_RET_ADDR || ret_addr_type == CATCHBLOCK_TWO_RET_ADDRS)
            ci->ret_addr[0] = decode_uint(b) + func_rva;
        if (ret_addr_type == CATCHBLOCK_TWO_RET_ADDRS)
            ci->ret_addr[1] = decode_uint(b) + func_rva;
    }

    return TRUE;
}

static void read_ipmap_info(BYTE **b, ipmap_info *ii)
{
    ii->ip = decode_uint(b);
    ii->state = (INT)decode_uint(b) - 1;
}

static BOOL validate_cxx_function_descr4(const cxx_function_descr_v4 *descr, DISPATCHER_CONTEXT *dispatch)
{
    ULONG64 image_base = dispatch->ImageBase;
    BYTE *unwind_map = rtti_rva(descr->unwind_map, image_base);
    BYTE *tryblock_map = rtti_rva(descr->tryblock_map, image_base);
    BYTE *ip_map = rtti_rva(descr->ip_map, image_base);
    UINT i, j;
    char *ip;

    TRACE("header 0x%x\n", descr->header);
    TRACE("basic block transformations flags: 0x%x\n", descr->bbt_flags);

    TRACE("unwind table: 0x%x(%p) %d\n", descr->unwind_map, unwind_map, descr->unwind_count);
    for (i = 0; i < descr->unwind_count; i++)
    {
        BYTE *entry = unwind_map;
        unwind_info_v4 ui;

        read_unwind_info(&unwind_map, &ui);
        if (ui.prev < (BYTE*)rtti_rva(descr->unwind_map, image_base)) ui.prev = NULL;
        TRACE("    %d (%p): type 0x%x prev %p func 0x%x object 0x%x\n",
                i, entry, ui.type, ui.prev, ui.handler, ui.object);
    }

    TRACE("try table: 0x%x(%p) %d\n", descr->tryblock_map, tryblock_map, descr->tryblock_count);
    for (i = 0; i < descr->tryblock_count; i++)
    {
        tryblock_info ti;
        BYTE *catchblock;

        read_tryblock_info(&tryblock_map, &ti, image_base);
        catchblock = rtti_rva(ti.catchblock, image_base);
        TRACE("    %d: start %d end %d catchlevel %d catch 0x%x(%p) %d\n",
                i, ti.start_level, ti.end_level, ti.catch_level,
                ti.catchblock, catchblock, ti.catchblock_count);
        for (j = 0; j < ti.catchblock_count; j++)
        {
            catchblock_info_v4 ci;
            if (!read_catchblock_info(&catchblock, &ci,
                        dispatch->FunctionEntry->BeginAddress)) return FALSE;
            TRACE("        %d: header 0x%x offset %d handler 0x%x "
                    "ret addr[0] %#x ret_addr[1] %#x type %#x %s\n", j, ci.header, ci.offset,
                    ci.handler, ci.ret_addr[0], ci.ret_addr[1], ci.type_info,
                    dbgstr_type_info(ci.type_info ? rtti_rva(ci.type_info, image_base) : NULL));
        }
    }

    TRACE("ipmap: 0x%x(%p) %d\n", descr->ip_map, ip_map, descr->ip_count);
    ip = rtti_rva(dispatch->FunctionEntry->BeginAddress, image_base);
    for (i = 0; i < descr->ip_count; i++)
    {
        ipmap_info ii;

        read_ipmap_info(&ip_map, &ii);
        ip += ii.ip;
        TRACE("    %d: ip offset 0x%x (%p) state %d\n", i, ii.ip, ip, ii.state);
    }

    TRACE("establisher frame: %x\n", descr->frame);
    return TRUE;
}

static inline int ip_to_state4(const cxx_function_descr_v4 *descr, DISPATCHER_CONTEXT *dispatch, ULONG64 ip)
{
    BYTE *ip_map = rtti_rva( descr->ip_map, dispatch->ImageBase );
    ULONG64 state_ip;
    ipmap_info ii;
    int ret = -1;
    UINT i;

    state_ip = dispatch->ImageBase + dispatch->FunctionEntry->BeginAddress;
    for (i = 0; i < descr->ip_count; i++)
    {
        read_ipmap_info(&ip_map, &ii);
        state_ip += ii.ip;
        if (ip < state_ip) break;
        ret = ii.state;
    }

    TRACE("%I64x -> state %d\n", ip, ret);
    return ret;
}

static void cxx_local_unwind4(ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
        const cxx_function_descr_v4 *descr, int trylevel, int last_level)
{
    void (__cdecl *handler_dtor)(void *obj, ULONG64 frame);
    BYTE *unwind_data, *last;
    unwind_info_v4 ui;
    void *obj;
    int i;

    if (trylevel == -2) trylevel = ip_to_state4( descr, dispatch, get_exception_pc(dispatch) );

    TRACE("current level: %d, last level: %d\n", trylevel, last_level);

    if (trylevel<-1 || trylevel>=(int)descr->unwind_count)
    {
        ERR("invalid trylevel %d\n", trylevel);
        terminate();
    }

    if (trylevel <= last_level) return;

    unwind_data = rtti_rva(descr->unwind_map, dispatch->ImageBase);
    last = unwind_data - 1;
    for (i = 0; i < trylevel; i++)
    {
        BYTE *addr = unwind_data;
        read_unwind_info(&unwind_data, &ui);
        if (i == last_level) last = addr;
    }

    while (unwind_data > last)
    {
        read_unwind_info(&unwind_data, &ui);
        unwind_data = ui.prev;

        if (ui.handler)
        {
            handler_dtor = rtti_rva(ui.handler, dispatch->ImageBase);
            obj = (void *)(frame + ui.object);
            if(ui.type == UNWIND_TYPE_DTOR_PTR)
                obj = *(void**)obj;
            TRACE("handler: %p object: %p\n", handler_dtor, obj);
            handler_dtor(obj, frame);
        }
    }
}

static LONG CALLBACK cxx_rethrow_filter(PEXCEPTION_POINTERS eptrs, void *c)
{
    EXCEPTION_RECORD *rec = eptrs->ExceptionRecord;
    cxx_catch_ctx *ctx = c;

    if (rec->ExceptionCode == CXX_EXCEPTION && !rec->ExceptionInformation[1] && !rec->ExceptionInformation[2])
        return EXCEPTION_EXECUTE_HANDLER;

    FlsSetValue(fls_index, (void*)(DWORD_PTR)ctx->search_state);
    if (rec->ExceptionCode != CXX_EXCEPTION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (rec->ExceptionInformation[1] == ctx->prev_rec->ExceptionInformation[1])
        ctx->rethrow = TRUE;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void CALLBACK cxx_catch_cleanup(BOOL normal, void *c)
{
    cxx_catch_ctx *ctx = c;
    __CxxUnregisterExceptionObject(&ctx->frame_info, ctx->rethrow);

    FlsSetValue(fls_index, (void*)(DWORD_PTR)ctx->unwind_state);
}

static void* WINAPI call_catch_block4(EXCEPTION_RECORD *rec)
{
    EXCEPTION_RECORD *untrans_rec = (void*)rec->ExceptionInformation[4];
    EXCEPTION_RECORD *prev_rec = (void*)rec->ExceptionInformation[6];
    CONTEXT *context = (void*)rec->ExceptionInformation[7];
    EXCEPTION_POINTERS ep = { prev_rec, context };
    cxx_catch_ctx ctx;
    void *ret_addr = NULL;

    ctx.rethrow = FALSE;
    __CxxRegisterExceptionObject(&ep, &ctx.frame_info);
    ctx.search_state = rec->ExceptionInformation[2];
    ctx.unwind_state = rec->ExceptionInformation[3];
    ctx.prev_rec = prev_rec;
    (*__processing_throw())--;
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
            FlsSetValue(fls_index, (void*)(DWORD_PTR)ctx.search_state);

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

    FlsSetValue(fls_index, (void*)-2);
    TRACE("handler returned %p, ret_addr[0] %#Ix, ret_addr[1] %#Ix.\n",
          ret_addr, rec->ExceptionInformation[8], rec->ExceptionInformation[9]);

    if (rec->ExceptionInformation[9])
    {
        if ((ULONG_PTR)ret_addr > 1)
        {
            ERR("unexpected handler result %p.\n", ret_addr);
            abort();
        }
        return (void*)rec->ExceptionInformation[8 + (ULONG_PTR)ret_addr];
    }
    return rec->ExceptionInformation[8] ? (void *)rec->ExceptionInformation[8] : ret_addr;
}

static inline BOOL cxx_is_consolidate(const EXCEPTION_RECORD *rec)
{
    return rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE
        && rec->NumberParameters == CONSOLIDATE_UNWIND_PARAMETER_COUNT
        && rec->ExceptionInformation[0] == (ULONG_PTR)call_catch_block4;
}

static inline void find_catch_block4(EXCEPTION_RECORD *rec, CONTEXT *context,
        EXCEPTION_RECORD *untrans_rec, ULONG64 frame, DISPATCHER_CONTEXT *dispatch,
        const cxx_function_descr_v4 *descr, cxx_exception_type *info,
        ULONG64 orig_frame, int trylevel)
{
    ULONG64 exc_base = (rec->NumberParameters == 4 ? rec->ExceptionInformation[3] : 0);
    int *processing_throw = __processing_throw();
    EXCEPTION_RECORD catch_record;
    BYTE *tryblock_map;
    CONTEXT ctx;
    UINT i, j;

    (*processing_throw)++;

    if (trylevel == -2) trylevel = ip_to_state4( descr, dispatch, get_exception_pc(dispatch) );
    TRACE("current trylevel: %d\n", trylevel);

    tryblock_map = rtti_rva(descr->tryblock_map, dispatch->ImageBase);
    for (i=0; i<descr->tryblock_count; i++)
    {
        tryblock_info tryblock;
        BYTE *catchblock;

        read_tryblock_info(&tryblock_map, &tryblock, dispatch->ImageBase);

        if (trylevel < tryblock.start_level) continue;
        if (trylevel > tryblock.end_level) continue;

        /* got a try block */
        catchblock = rtti_rva(tryblock.catchblock, dispatch->ImageBase);
        for (j=0; j<tryblock.catchblock_count; j++)
        {
            catchblock_info_v4 ci;

            read_catchblock_info(&catchblock, &ci, dispatch->FunctionEntry->BeginAddress);

            if (info)
            {
                const type_info *catch_ti = NULL;
                const cxx_type_info *type;

                if (ci.type_info) catch_ti = rtti_rva( ci.type_info, dispatch->ImageBase );
                if (!(type = find_caught_type( info, exc_base, catch_ti, ci.flags ))) continue;

                TRACE("matched type %p in tryblock %d catchblock %d\n", type, i, j);

                if (catch_ti && catch_ti->mangled[0] && ci.offset)
                {
                    /* copy the exception to its destination on the stack */
                    void **dest = (void **)(orig_frame + ci.offset);
                    copy_exception( (void *)rec->ExceptionInformation[1], dest, ci.flags, type, exc_base );
                }
            }
            else
            {
                /* no CXX_EXCEPTION only proceed with a catch(...) block*/
                if (ci.type_info)
                    continue;
                TRACE("found catch(...) block\n");
            }

            /* unwind stack and call catch */
            memset(&catch_record, 0, sizeof(catch_record));
            catch_record.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
            catch_record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            catch_record.NumberParameters = CONSOLIDATE_UNWIND_PARAMETER_COUNT;
            catch_record.ExceptionInformation[0] = (ULONG_PTR)call_catch_block4;
            catch_record.ExceptionInformation[1] = orig_frame;
            catch_record.ExceptionInformation[2] = tryblock.catch_level;
            catch_record.ExceptionInformation[3] = tryblock.start_level;
            catch_record.ExceptionInformation[4] = (ULONG_PTR)untrans_rec;
            catch_record.ExceptionInformation[5] = (ULONG_PTR)rtti_rva(ci.handler, dispatch->ImageBase);
            /* 3ds Max plugin expect ExceptionInformation[6] to contain exception record */
            catch_record.ExceptionInformation[6] = (ULONG_PTR)rec;
            catch_record.ExceptionInformation[7] = (ULONG_PTR)context;
            if (ci.ret_addr[0])
            {
                catch_record.ExceptionInformation[8] = (ULONG_PTR)rtti_rva(
                        ci.ret_addr[0], dispatch->ImageBase);
            }
            if (ci.ret_addr[1])
            {
                catch_record.ExceptionInformation[9] = (ULONG_PTR)rtti_rva(
                        ci.ret_addr[1], dispatch->ImageBase);
            }
            RtlUnwindEx((void*)frame, (void*)dispatch->ControlPc, &catch_record, NULL, &ctx, NULL);
        }
    }

    TRACE("no matching catch block found\n");
    (*processing_throw)--;
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
    find_catch_block4(rec, ep->ContextRecord, ctx->seh_rec, ctx->dest_frame, ctx->dispatch,
            ctx->descr, exc_type, ctx->orig_frame, ctx->trylevel);

    __DestructExceptionObject(rec);
    return ExceptionContinueSearch;
}

/* Hacky way to obtain se_translator */
static inline _se_translator_function get_se_translator(void)
{
    return __current_exception()[-2];
}

static void check_noexcept( PEXCEPTION_RECORD rec, const cxx_function_descr_v4 *descr )
{
    if (!(descr->header & FUNC_DESCR_IS_CATCH) &&
            rec->ExceptionCode == CXX_EXCEPTION &&
            (descr->header & FUNC_DESCR_NO_EXCEPT))
    {
        ERR("noexcept function propagating exception\n");
        terminate();
    }
}

static DWORD cxx_frame_handler4(EXCEPTION_RECORD *rec, ULONG64 frame,
        CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
        const cxx_function_descr_v4 *descr, int trylevel)
{
    cxx_exception_type *exc_type;
    ULONG64 orig_frame = frame;

    if (descr->header & FUNC_DESCR_IS_CATCH)
    {
        TRACE("nested exception detected\n");
        orig_frame = *(ULONG64 *)(frame + descr->frame);
        TRACE("setting orig_frame to %Ix\n", orig_frame);
    }

    if (rec->ExceptionFlags & (EXCEPTION_UNWINDING|EXCEPTION_EXIT_UNWIND))
    {
        int last_level = -1;
        if ((rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND) && cxx_is_consolidate(rec))
            last_level = rec->ExceptionInformation[3];
        else if ((rec->ExceptionFlags & EXCEPTION_TARGET_UNWIND) && rec->ExceptionCode == STATUS_LONGJUMP)
            last_level = ip_to_state4( descr, dispatch, dispatch->TargetIp );

        cxx_local_unwind4(orig_frame, dispatch, descr, trylevel, last_level);
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_map)
    {
        check_noexcept(rec, descr);
        return ExceptionContinueSearch;
    }

    if (rec->ExceptionCode == CXX_EXCEPTION)
    {
        if (!rec->ExceptionInformation[1] && !rec->ExceptionInformation[2])
        {
            TRACE("rethrow detected.\n");
            *rec = *(EXCEPTION_RECORD*)*__current_exception();
        }

        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];

        if (TRACE_ON(seh))
        {
            TRACE("handling C++ exception rec %p frame %Ix descr %p\n", rec, frame,  descr);
            TRACE_EXCEPTION_TYPE(exc_type, rec->ExceptionInformation[3]);
        }
    }
    else
    {
        _se_translator_function se_translator = get_se_translator();

        exc_type = NULL;
        TRACE("handling C exception code %lx rec %p frame %Ix descr %p\n",
                rec->ExceptionCode, rec, frame, descr);

        if (se_translator) {
            EXCEPTION_POINTERS except_ptrs;
            se_translator_ctx ctx;

            ctx.dest_frame = frame;
            ctx.orig_frame = orig_frame;
            ctx.seh_rec    = rec;
            ctx.dispatch   = dispatch;
            ctx.descr      = descr;
            ctx.trylevel   = trylevel;
            __TRY
            {
                except_ptrs.ExceptionRecord = rec;
                except_ptrs.ContextRecord = context;
                se_translator(rec->ExceptionCode, &except_ptrs);
            }
            __EXCEPT_CTX(se_translation_filter, &ctx)
            {
            }
            __ENDTRY
        }
    }

    find_catch_block4(rec, context, NULL, frame, dispatch, descr, exc_type, orig_frame, trylevel);
    check_noexcept(rec, descr);
    return ExceptionContinueSearch;
}

EXCEPTION_DISPOSITION __cdecl __CxxFrameHandler4(EXCEPTION_RECORD *rec,
        ULONG64 frame, CONTEXT *context, DISPATCHER_CONTEXT *dispatch)
{
    cxx_function_descr_v4 descr;
    BYTE *p, *count, *count_end;
    int trylevel;

    TRACE("%p %Ix %p %p\n", rec, frame, context, dispatch);

    trylevel = (DWORD_PTR)FlsGetValue(fls_index);
    FlsSetValue(fls_index, (void*)-2);

    memset(&descr, 0, sizeof(descr));
    p = rtti_rva(*(UINT*)dispatch->HandlerData, dispatch->ImageBase);
    descr.header = *p++;

    if ((descr.header & FUNC_DESCR_EHS) &&
            rec->ExceptionCode != CXX_EXCEPTION &&
            !cxx_is_consolidate(rec) &&
            rec->ExceptionCode != STATUS_LONGJUMP)
        return ExceptionContinueSearch;  /* handle only c++ exceptions */

    if (descr.header & ~(FUNC_DESCR_IS_CATCH | FUNC_DESCR_IS_SEPARATED |
                FUNC_DESCR_UNWIND_MAP | FUNC_DESCR_TRYBLOCK_MAP | FUNC_DESCR_EHS |
                FUNC_DESCR_NO_EXCEPT))
    {
        FIXME("unsupported flags: %x\n", descr.header);
        return ExceptionContinueSearch;
    }

    if (descr.header & FUNC_DESCR_BBT) descr.bbt_flags = decode_uint(&p);
    if (descr.header & FUNC_DESCR_UNWIND_MAP)
    {
        descr.unwind_map = read_rva(&p);
        count_end = count = rtti_rva(descr.unwind_map, dispatch->ImageBase);
        descr.unwind_count = decode_uint(&count_end);
        descr.unwind_map += count_end - count;
    }
    if (descr.header & FUNC_DESCR_TRYBLOCK_MAP)
    {
        descr.tryblock_map = read_rva(&p);
        count_end = count = rtti_rva(descr.tryblock_map, dispatch->ImageBase);
        descr.tryblock_count = decode_uint(&count_end);
        descr.tryblock_map += count_end - count;
    }
    descr.ip_map = read_rva(&p);
    if (descr.header & FUNC_DESCR_IS_SEPARATED)
    {
        UINT i, num, func;
        BYTE *map = rtti_rva(descr.ip_map, dispatch->ImageBase);

        num = decode_uint(&map);
        for (i = 0; i < num; i++)
        {
            func = read_rva(&map);
            descr.ip_map = read_rva(&map);
            if (func == dispatch->FunctionEntry->BeginAddress)
                break;
        }
        if (i == num)
        {
            FIXME("function ip_map not found\n");
            return ExceptionContinueSearch;
        }
    }
    count_end = count = rtti_rva(descr.ip_map, dispatch->ImageBase);
    descr.ip_count = decode_uint(&count_end);
    descr.ip_map += count_end - count;
    if (descr.header & FUNC_DESCR_IS_CATCH) descr.frame = decode_uint(&p);

    if (!validate_cxx_function_descr4(&descr, dispatch))
        return ExceptionContinueSearch;

    return cxx_frame_handler4(rec, frame, context, dispatch, &descr, trylevel);
}

BOOL msvcrt_init_handler4(void)
{
    fls_index = FlsAlloc(NULL);
    if (fls_index == FLS_OUT_OF_INDEXES)
        return FALSE;
    msvcrt_attach_handler4();
    return TRUE;
}

void msvcrt_attach_handler4(void)
{
    FlsSetValue(fls_index, (void*)-2);
}

void msvcrt_free_handler4(void)
{
    FlsFree(fls_index);
}

#endif
