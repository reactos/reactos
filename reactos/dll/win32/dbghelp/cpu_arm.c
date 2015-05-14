/*
 * File cpu_arm.c
 *
 * Copyright (C) 2009 Eric Pouech
 * Copyright (C) 2010, 2011 André Hentschel
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

static BOOL arm_get_addr(HANDLE hThread, const CONTEXT* ctx,
                         enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = 0; /* don't need segment */
    switch (ca)
    {
#ifdef __arm__
    case cpu_addr_pc:    addr->Offset = ctx->Pc; return TRUE;
    case cpu_addr_stack: addr->Offset = ctx->Sp; return TRUE;
    case cpu_addr_frame: addr->Offset = ctx->Fpscr; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

#ifdef __arm__
enum st_mode {stm_start, stm_arm, stm_done};

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
    DWORD                   oldReturn = context->Lr;

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

static BOOL arm_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    unsigned    deltapc = curr_count <= 1 ? 0 : 4;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "ARM",
          wine_dbgstr_longlong(curr_count));

    if (curr_mode == stm_start)
    {
        /* Init done */
        set_curr_mode(stm_arm);
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
    frame->AddrFrame.Offset = context->Fpscr;
    frame->AddrPC.Offset = context->Pc;

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    inc_curr_count();

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "ARM",
          wine_dbgstr_longlong(curr_count),
          frame->FuncTableEntry);

    return TRUE;
done_err:
    set_curr_mode(stm_done);
    return FALSE;
}
#else
static BOOL arm_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    return FALSE;
}
#endif

static unsigned arm_map_dwarf_register(unsigned regno)
{
    if (regno <= 15) return CV_ARM_R0 + regno;
    if (regno == 128) return CV_ARM_CPSR;

    FIXME("Don't know how to map register %d\n", regno);
    return CV_ARM_NOREG;
}

static void* arm_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
#ifdef __arm__
    switch (regno)
    {
    case CV_ARM_R0 +  0: *size = sizeof(ctx->R0); return &ctx->R0;
    case CV_ARM_R0 +  1: *size = sizeof(ctx->R1); return &ctx->R1;
    case CV_ARM_R0 +  2: *size = sizeof(ctx->R2); return &ctx->R2;
    case CV_ARM_R0 +  3: *size = sizeof(ctx->R3); return &ctx->R3;
    case CV_ARM_R0 +  4: *size = sizeof(ctx->R4); return &ctx->R4;
    case CV_ARM_R0 +  5: *size = sizeof(ctx->R5); return &ctx->R5;
    case CV_ARM_R0 +  6: *size = sizeof(ctx->R6); return &ctx->R6;
    case CV_ARM_R0 +  7: *size = sizeof(ctx->R7); return &ctx->R7;
    case CV_ARM_R0 +  8: *size = sizeof(ctx->R8); return &ctx->R8;
    case CV_ARM_R0 +  9: *size = sizeof(ctx->R9); return &ctx->R9;
    case CV_ARM_R0 + 10: *size = sizeof(ctx->R10); return &ctx->R10;
    case CV_ARM_R0 + 11: *size = sizeof(ctx->Fpscr); return &ctx->Fpscr;
    //case CV_ARM_R0 + 12: *size = sizeof(ctx->Ip); return &ctx->Ip;

    case CV_ARM_SP: *size = sizeof(ctx->Sp); return &ctx->Sp;
    case CV_ARM_LR: *size = sizeof(ctx->Lr); return &ctx->Lr;
    case CV_ARM_PC: *size = sizeof(ctx->Pc); return &ctx->Pc;
    case CV_ARM_CPSR: *size = sizeof(ctx->Cpsr); return &ctx->Cpsr;
    }
#endif
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static const char* arm_fetch_regname(unsigned regno)
{
    switch (regno)
    {
    case CV_ARM_R0 +  0: return "r0";
    case CV_ARM_R0 +  1: return "r1";
    case CV_ARM_R0 +  2: return "r2";
    case CV_ARM_R0 +  3: return "r3";
    case CV_ARM_R0 +  4: return "r4";
    case CV_ARM_R0 +  5: return "r5";
    case CV_ARM_R0 +  6: return "r6";
    case CV_ARM_R0 +  7: return "r7";
    case CV_ARM_R0 +  8: return "r8";
    case CV_ARM_R0 +  9: return "r9";
    case CV_ARM_R0 + 10: return "r10";
    case CV_ARM_R0 + 11: return "r11";
    case CV_ARM_R0 + 12: return "r12";

    case CV_ARM_SP: return "sp";
    case CV_ARM_LR: return "lr";
    case CV_ARM_PC: return "pc";
    case CV_ARM_CPSR: return "cpsr";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static BOOL arm_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __arm__
        ULONG base = ctx->Pc <= 0x80 ? 0 : ctx->Pc - 0x80;
        minidump_add_memory_block(dc, base, ctx->Pc + 0x80 - base, 0);
#endif
    }

    return TRUE;
}

static BOOL arm_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    /* FIXME: actually, we should probably take care of FPO data, unless it's stored in
     * function table minidump stream
     */
    return FALSE;
}

DECLSPEC_HIDDEN struct cpu cpu_arm = {
    IMAGE_FILE_MACHINE_ARMNT,
    4,
    CV_ARM_R0 + 11,
    arm_get_addr,
    arm_stack_walk,
    NULL,
    arm_map_dwarf_register,
    arm_fetch_context_reg,
    arm_fetch_regname,
    arm_fetch_minidump_thread,
    arm_fetch_minidump_module,
};
