/*
 * File cpu_arm64.c
 *
 * Copyright (C) 2009 Eric Pouech
 * Copyright (C) 2010-2013 André Hentschel
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

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static BOOL arm64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                           enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    switch (ca)
    {
#ifdef __aarch64__
    case cpu_addr_pc:    addr->Offset = ctx->Pc;  return TRUE;
    case cpu_addr_stack: addr->Offset = ctx->Sp;  return TRUE;
    case cpu_addr_frame: addr->Offset = ctx->Fp; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

#ifdef __aarch64__
enum st_mode {stm_start, stm_arm64, stm_done};

/* indexes in Reserved array */
#define __CurrentModeCount      0

#define curr_mode   (frame->Reserved[__CurrentModeCount] & 0x0F)
#define curr_count  (frame->Reserved[__CurrentModeCount] >> 4)

#define set_curr_mode(m) {frame->Reserved[__CurrentModeCount] &= ~0x0F; frame->Reserved[__CurrentModeCount] |= (m & 0x0F);}
#define inc_curr_count() (frame->Reserved[__CurrentModeCount] += 0x10)

/* fetch_next_frame()
 *
 * modify (at least) context.Pc using unwind information
 * either out of debug info (dwarf), or simple Lr trace
 */
static BOOL fetch_next_frame(struct cpu_stack_walk* csw,
                               CONTEXT* context, DWORD_PTR curr_pc)
{
    DWORD_PTR               xframe;
    DWORD_PTR               oldReturn = context->Lr;

    if (dwarf2_virtual_unwind(csw, curr_pc, context, &xframe))
    {
        context->Sp = xframe;
        context->Pc = oldReturn;
        return TRUE;
    }

    if (context->Pc == context->Lr) return FALSE;
    context->Pc = oldReturn;

    return TRUE;
}

static BOOL arm64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    unsigned deltapc = curr_count <= 1 ? 0 : 4;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "ARM64",
          wine_dbgstr_longlong(curr_count));

    if (curr_mode == stm_start)
    {
        /* Init done */
        set_curr_mode(stm_arm64);
        frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (context->Sp != frame->AddrStack.Offset) FIXME("inconsistent Stack Pointer\n");
        if (context->Pc != frame->AddrPC.Offset) FIXME("inconsistent Program Counter\n");

        if (frame->AddrReturn.Offset == 0) goto done_err;
        if (!fetch_next_frame(csw, context, frame->AddrPC.Offset - deltapc))
            goto done_err;
    }

    memset(&frame->Params, 0, sizeof(frame->Params));

    /* set frame information */
    frame->AddrStack.Offset = context->Sp;
    frame->AddrReturn.Offset = context->Lr;
    frame->AddrFrame.Offset = context->Fp;
    frame->AddrPC.Offset = context->Pc;

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    inc_curr_count();

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "ARM64",
          wine_dbgstr_longlong(curr_count),
          frame->FuncTableEntry);

    return TRUE;
done_err:
    set_curr_mode(stm_done);
    return FALSE;
}
#else
static BOOL arm64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    return FALSE;
}
#endif

static unsigned arm64_map_dwarf_register(unsigned regno, BOOL eh_frame)
{
    if (regno <= 28) return CV_ARM64_X0 + regno;
    if (regno == 29) return CV_ARM64_FP;
    if (regno == 30) return CV_ARM64_LR;
    if (regno == 31) return CV_ARM64_SP;

    FIXME("Don't know how to map register %d\n", regno);
    return CV_ARM64_NOREG;
}

static void* arm64_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
#ifdef __aarch64__
    switch (regno)
    {
    case CV_ARM64_PSTATE: *size = sizeof(ctx->Cpsr); return &ctx->Cpsr;
    case CV_ARM64_X0 +  0: *size = sizeof(ctx->X0);  return &ctx->X0;
    case CV_ARM64_X0 +  1: *size = sizeof(ctx->X1);  return &ctx->X1;
    case CV_ARM64_X0 +  2: *size = sizeof(ctx->X2);  return &ctx->X2;
    case CV_ARM64_X0 +  3: *size = sizeof(ctx->X3);  return &ctx->X3;
    case CV_ARM64_X0 +  4: *size = sizeof(ctx->X4);  return &ctx->X4;
    case CV_ARM64_X0 +  5: *size = sizeof(ctx->X5);  return &ctx->X5;
    case CV_ARM64_X0 +  6: *size = sizeof(ctx->X6);  return &ctx->X6;
    case CV_ARM64_X0 +  7: *size = sizeof(ctx->X7);  return &ctx->X7;
    case CV_ARM64_X0 +  8: *size = sizeof(ctx->X8);  return &ctx->X8;
    case CV_ARM64_X0 +  9: *size = sizeof(ctx->X9);  return &ctx->X9;
    case CV_ARM64_X0 + 10: *size = sizeof(ctx->X10); return &ctx->X10;
    case CV_ARM64_X0 + 11: *size = sizeof(ctx->X11); return &ctx->X11;
    case CV_ARM64_X0 + 12: *size = sizeof(ctx->X12); return &ctx->X12;
    case CV_ARM64_X0 + 13: *size = sizeof(ctx->X13); return &ctx->X13;
    case CV_ARM64_X0 + 14: *size = sizeof(ctx->X14); return &ctx->X14;
    case CV_ARM64_X0 + 15: *size = sizeof(ctx->X15); return &ctx->X15;
    case CV_ARM64_X0 + 16: *size = sizeof(ctx->X16); return &ctx->X16;
    case CV_ARM64_X0 + 17: *size = sizeof(ctx->X17); return &ctx->X17;
    case CV_ARM64_X0 + 18: *size = sizeof(ctx->X18); return &ctx->X18;
    case CV_ARM64_X0 + 19: *size = sizeof(ctx->X19); return &ctx->X19;
    case CV_ARM64_X0 + 20: *size = sizeof(ctx->X20); return &ctx->X20;
    case CV_ARM64_X0 + 21: *size = sizeof(ctx->X21); return &ctx->X21;
    case CV_ARM64_X0 + 22: *size = sizeof(ctx->X22); return &ctx->X22;
    case CV_ARM64_X0 + 23: *size = sizeof(ctx->X23); return &ctx->X23;
    case CV_ARM64_X0 + 24: *size = sizeof(ctx->X24); return &ctx->X24;
    case CV_ARM64_X0 + 25: *size = sizeof(ctx->X25); return &ctx->X25;
    case CV_ARM64_X0 + 26: *size = sizeof(ctx->X26); return &ctx->X26;
    case CV_ARM64_X0 + 27: *size = sizeof(ctx->X27); return &ctx->X27;
    case CV_ARM64_X0 + 28: *size = sizeof(ctx->X28); return &ctx->X28;

    case CV_ARM64_FP:     *size = sizeof(ctx->Fp);     return &ctx->Fp;
    case CV_ARM64_LR:     *size = sizeof(ctx->Lr);     return &ctx->Lr;
    case CV_ARM64_SP:     *size = sizeof(ctx->Sp);     return &ctx->Sp;
    case CV_ARM64_PC:     *size = sizeof(ctx->Pc);     return &ctx->Pc;
    }
#endif
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static const char* arm64_fetch_regname(unsigned regno)
{
    switch (regno)
    {
    case CV_ARM64_PSTATE:  return "cpsr";
    case CV_ARM64_X0 +  0: return "x0";
    case CV_ARM64_X0 +  1: return "x1";
    case CV_ARM64_X0 +  2: return "x2";
    case CV_ARM64_X0 +  3: return "x3";
    case CV_ARM64_X0 +  4: return "x4";
    case CV_ARM64_X0 +  5: return "x5";
    case CV_ARM64_X0 +  6: return "x6";
    case CV_ARM64_X0 +  7: return "x7";
    case CV_ARM64_X0 +  8: return "x8";
    case CV_ARM64_X0 +  9: return "x9";
    case CV_ARM64_X0 + 10: return "x10";
    case CV_ARM64_X0 + 11: return "x11";
    case CV_ARM64_X0 + 12: return "x12";
    case CV_ARM64_X0 + 13: return "x13";
    case CV_ARM64_X0 + 14: return "x14";
    case CV_ARM64_X0 + 15: return "x15";
    case CV_ARM64_X0 + 16: return "x16";
    case CV_ARM64_X0 + 17: return "x17";
    case CV_ARM64_X0 + 18: return "x18";
    case CV_ARM64_X0 + 19: return "x19";
    case CV_ARM64_X0 + 20: return "x20";
    case CV_ARM64_X0 + 21: return "x21";
    case CV_ARM64_X0 + 22: return "x22";
    case CV_ARM64_X0 + 23: return "x23";
    case CV_ARM64_X0 + 24: return "x24";
    case CV_ARM64_X0 + 25: return "x25";
    case CV_ARM64_X0 + 26: return "x26";
    case CV_ARM64_X0 + 27: return "x27";
    case CV_ARM64_X0 + 28: return "x28";

    case CV_ARM64_FP:     return "fp";
    case CV_ARM64_LR:     return "lr";
    case CV_ARM64_SP:     return "sp";
    case CV_ARM64_PC:     return "pc";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static BOOL arm64_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __aarch64__
        ULONG base = ctx->Pc <= 0x80 ? 0 : ctx->Pc - 0x80;
        minidump_add_memory_block(dc, base, ctx->Pc + 0x80 - base, 0);
#endif
    }

    return TRUE;
}

static BOOL arm64_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    /* FIXME: actually, we should probably take care of FPO data, unless it's stored in
     * function table minidump stream
     */
    return FALSE;
}

DECLSPEC_HIDDEN struct cpu cpu_arm64 = {
    IMAGE_FILE_MACHINE_ARM64,
    8,
    CV_ARM64_FP,
    arm64_get_addr,
    arm64_stack_walk,
    NULL,
    arm64_map_dwarf_register,
    arm64_fetch_context_reg,
    arm64_fetch_regname,
    arm64_fetch_minidump_thread,
    arm64_fetch_minidump_module,
};
