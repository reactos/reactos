/*
 * File cpu_x86_64.c
 *
 * Copyright (C) 1999, 2005 Alexandre Julliard
 * Copyright (C) 2009, 2011 Eric Pouech.
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

/* x86-64 unwind information, for PE modules, as described on MSDN */

typedef enum _UNWIND_OP_CODES
{
    UWOP_PUSH_NONVOL = 0,
    UWOP_ALLOC_LARGE,
    UWOP_ALLOC_SMALL,
    UWOP_SET_FPREG,
    UWOP_SAVE_NONVOL,
    UWOP_SAVE_NONVOL_FAR,
    UWOP_SAVE_XMM128,
    UWOP_SAVE_XMM128_FAR,
    UWOP_PUSH_MACHFRAME
} UNWIND_CODE_OPS;

typedef union _UNWIND_CODE
{
    struct
    {
        BYTE CodeOffset;
        BYTE UnwindOp : 4;
        BYTE OpInfo   : 4;
    } u;
    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

typedef struct _UNWIND_INFO
{
    BYTE Version       : 3;
    BYTE Flags         : 5;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegister : 4;
    BYTE FrameOffset   : 4;
    UNWIND_CODE UnwindCode[1]; /* actually CountOfCodes (aligned) */
/*
 *  union
 *  {
 *      OPTIONAL ULONG ExceptionHandler;
 *      OPTIONAL ULONG FunctionEntry;
 *  };
 *  OPTIONAL ULONG ExceptionData[];
 */
} UNWIND_INFO, *PUNWIND_INFO;

static BOOL x86_64_get_addr(HANDLE hThread, const CONTEXT* ctx,
                            enum cpu_addr ca, ADDRESS64* addr)
{
    addr->Mode = AddrModeFlat;
    switch (ca)
    {
#ifdef __x86_64__
    case cpu_addr_pc:    addr->Segment = ctx->SegCs; addr->Offset = ctx->Rip; return TRUE;
    case cpu_addr_stack: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rsp; return TRUE;
    case cpu_addr_frame: addr->Segment = ctx->SegSs; addr->Offset = ctx->Rbp; return TRUE;
#endif
    default: addr->Mode = -1;
        return FALSE;
    }
}

#ifdef __x86_64__

enum st_mode {stm_start, stm_64bit, stm_done};

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentCount    1
/* #define __     2 (unused) */

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_count  (frame->Reserved[__CurrentCount])
/* #define ??? (frame->Reserved[__]) (unused) */

union handler_data
{
    RUNTIME_FUNCTION chain;
    ULONG handler;
};

static void dump_unwind_info(struct cpu_stack_walk* csw, ULONG64 base, RUNTIME_FUNCTION *function)
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    union handler_data handler_data;
    char buffer[sizeof(UNWIND_INFO) + 256 * sizeof(UNWIND_CODE)];
    UNWIND_INFO* info = (UNWIND_INFO*)buffer;
    unsigned int i, count;
    RUNTIME_FUNCTION snext;
    ULONG64 addr;

    TRACE("**** func %x-%x\n", function->BeginAddress, function->EndAddress);
    for (;;)
    {
        if (function->UnwindData & 1)
        {
            if (!sw_read_mem(csw, base + function->UnwindData, &snext, sizeof(snext)))
            {
                TRACE("Couldn't unwind RUNTIME_INFO at %lx\n", base + function->UnwindData);
                return;
            }
            TRACE("unwind info for function %p-%p chained to function %p-%p\n",
                  (char*)base + function->BeginAddress, (char*)base + function->EndAddress,
                  (char*)base + snext.BeginAddress, (char*)base + snext.EndAddress);
            function = &snext;
            continue;
        }
        addr = base + function->UnwindData;
        if (!sw_read_mem(csw, addr, info, FIELD_OFFSET(UNWIND_INFO, UnwindCode)) ||
            !sw_read_mem(csw, addr + FIELD_OFFSET(UNWIND_INFO, UnwindCode),
                         info->UnwindCode, info->CountOfCodes * sizeof(UNWIND_CODE)))
        {
            FIXME("couldn't read memory for UNWIND_INFO at %lx\n", addr);
            return;
        }
        TRACE("unwind info at %p flags %x prolog 0x%x bytes function %p-%p\n",
              (char*)addr, info->Flags, info->SizeOfProlog,
              (char*)base + function->BeginAddress, (char*)base + function->EndAddress);

        if (info->FrameRegister)
            TRACE("    frame register %s offset 0x%x(%%rsp)\n",
                  reg_names[info->FrameRegister], info->FrameOffset * 16);

        for (i = 0; i < info->CountOfCodes; i++)
        {
            TRACE("    0x%x: ", info->UnwindCode[i].u.CodeOffset);
            switch (info->UnwindCode[i].u.UnwindOp)
            {
            case UWOP_PUSH_NONVOL:
                TRACE("pushq %%%s\n", reg_names[info->UnwindCode[i].u.OpInfo]);
                break;
            case UWOP_ALLOC_LARGE:
                if (info->UnwindCode[i].u.OpInfo)
                {
                    count = *(DWORD*)&info->UnwindCode[i+1];
                    i += 2;
                }
                else
                {
                    count = *(USHORT*)&info->UnwindCode[i+1] * 8;
                    i++;
                }
                TRACE("subq $0x%x,%%rsp\n", count);
                break;
            case UWOP_ALLOC_SMALL:
                count = (info->UnwindCode[i].u.OpInfo + 1) * 8;
                TRACE("subq $0x%x,%%rsp\n", count);
                break;
            case UWOP_SET_FPREG:
                TRACE("leaq 0x%x(%%rsp),%s\n",
                      info->FrameOffset * 16, reg_names[info->FrameRegister]);
                break;
            case UWOP_SAVE_NONVOL:
                count = *(USHORT*)&info->UnwindCode[i+1] * 8;
                TRACE("movq %%%s,0x%x(%%rsp)\n", reg_names[info->UnwindCode[i].u.OpInfo], count);
                i++;
                break;
            case UWOP_SAVE_NONVOL_FAR:
                count = *(DWORD*)&info->UnwindCode[i+1];
                TRACE("movq %%%s,0x%x(%%rsp)\n", reg_names[info->UnwindCode[i].u.OpInfo], count);
                i += 2;
                break;
            case UWOP_SAVE_XMM128:
                count = *(USHORT*)&info->UnwindCode[i+1] * 16;
                TRACE("movaps %%xmm%u,0x%x(%%rsp)\n", info->UnwindCode[i].u.OpInfo, count);
                i++;
                break;
            case UWOP_SAVE_XMM128_FAR:
                count = *(DWORD*)&info->UnwindCode[i+1];
                TRACE("movaps %%xmm%u,0x%x(%%rsp)\n", info->UnwindCode[i].u.OpInfo, count);
                i += 2;
                break;
            case UWOP_PUSH_MACHFRAME:
                TRACE("PUSH_MACHFRAME %u\n", info->UnwindCode[i].u.OpInfo);
                break;
            default:
                FIXME("unknown code %u\n", info->UnwindCode[i].u.UnwindOp);
                break;
            }
        }

        addr += FIELD_OFFSET(UNWIND_INFO, UnwindCode) +
            ((info->CountOfCodes + 1) & ~1) * sizeof(UNWIND_CODE);
        if (info->Flags & UNW_FLAG_CHAININFO)
        {
            if (!sw_read_mem(csw, addr, &handler_data, sizeof(handler_data.chain)))
            {
                FIXME("couldn't read memory for handler_data.chain\n");
                return;
            }
            TRACE("    chained to function %p-%p\n",
                  (char*)base + handler_data.chain.BeginAddress,
                  (char*)base + handler_data.chain.EndAddress);
            function = &handler_data.chain;
            continue;
        }
        if (info->Flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
        {
            if (!sw_read_mem(csw, addr, &handler_data, sizeof(handler_data.handler)))
            {
                FIXME("couldn't read memory for handler_data.handler\n");
                return;
            }
            TRACE("    handler %p data at %p\n",
                  (char*)base + handler_data.handler, (char*)addr + sizeof(handler_data.handler));
        }
        break;
    }
}

/* highly derived from dlls/ntdll/signal_x86_64.c */
static ULONG64 get_int_reg(CONTEXT *context, int reg)
{
    return *(&context->Rax + reg);
}

static void set_int_reg(CONTEXT *context, int reg, ULONG64 val)
{
    *(&context->Rax + reg) = val;
}

static void set_float_reg(CONTEXT *context, int reg, M128A val)
{
    *(&context->u.s.Xmm0 + reg) = val;
}

static int get_opcode_size(UNWIND_CODE op)
{
    switch (op.u.UnwindOp)
    {
    case UWOP_ALLOC_LARGE:
        return 2 + (op.u.OpInfo != 0);
    case UWOP_SAVE_NONVOL:
    case UWOP_SAVE_XMM128:
        return 2;
    case UWOP_SAVE_NONVOL_FAR:
    case UWOP_SAVE_XMM128_FAR:
        return 3;
    default:
        return 1;
    }
}

static BOOL is_inside_epilog(struct cpu_stack_walk* csw, DWORD64 pc,
                             DWORD64 base, const RUNTIME_FUNCTION *function )
{
    BYTE op0, op1, op2;
    LONG val32;

    if (!sw_read_mem(csw, pc, &op0, 1)) return FALSE;

    /* add or lea must be the first instruction, and it must have a rex.W prefix */
    if ((op0 & 0xf8) == 0x48)
    {
        if (!sw_read_mem(csw, pc + 1, &op1, 1)) return FALSE;
        if (!sw_read_mem(csw, pc + 2, &op2, 1)) return FALSE;
        switch (op1)
        {
        case 0x81: /* add $nnnn,%rsp */
            if (op0 == 0x48 && op2 == 0xc4)
            {
                pc += 7;
                break;
            }
            return FALSE;
        case 0x83: /* add $n,%rsp */
            if (op0 == 0x48 && op2 == 0xc4)
            {
                pc += 4;
                break;
            }
            return FALSE;
        case 0x8d: /* lea n(reg),%rsp */
            if (op0 & 0x06) return FALSE;  /* rex.RX must be cleared */
            if (((op2 >> 3) & 7) != 4) return FALSE;  /* dest reg mus be %rsp */
            if ((op2 & 7) == 4) return FALSE;  /* no SIB byte allowed */
            if ((op2 >> 6) == 1)  /* 8-bit offset */
            {
                pc += 4;
                break;
            }
            if ((op2 >> 6) == 2)  /* 32-bit offset */
            {
                pc += 7;
                break;
            }
            return FALSE;
        }
    }

    /* now check for various pop instructions */
    for (;;)
    {
        if (!sw_read_mem(csw, pc, &op0, 1)) return FALSE;
        if ((op0 & 0xf0) == 0x40)  /* rex prefix */
        {
            if (!sw_read_mem(csw, ++pc, &op0, 1)) return FALSE;
        }

        switch (op0)
        {
        case 0x58: /* pop %rax/%r8 */
        case 0x59: /* pop %rcx/%r9 */
        case 0x5a: /* pop %rdx/%r10 */
        case 0x5b: /* pop %rbx/%r11 */
        case 0x5c: /* pop %rsp/%r12 */
        case 0x5d: /* pop %rbp/%r13 */
        case 0x5e: /* pop %rsi/%r14 */
        case 0x5f: /* pop %rdi/%r15 */
            pc++;
            continue;
        case 0xc2: /* ret $nn */
        case 0xc3: /* ret */
            return TRUE;
        case 0xe9: /* jmp nnnn */
            if (!sw_read_mem(csw, pc + 1, &val32, sizeof(LONG))) return FALSE;
            pc += 5 + val32;
            if (pc - base >= function->BeginAddress && pc - base < function->EndAddress)
                continue;
            break;
        case 0xeb: /* jmp n */
            if (!sw_read_mem(csw, pc + 1, &op1, 1)) return FALSE;
            pc += 2 + (signed char)op1;
            if (pc - base >= function->BeginAddress && pc - base < function->EndAddress)
                continue;
            break;
        case 0xf3: /* rep; ret (for amd64 prediction bug) */
            if (!sw_read_mem(csw, pc + 1, &op1, 1)) return FALSE;
            return op1 == 0xc3;
        }
        return FALSE;
    }
}

static BOOL interpret_epilog(struct cpu_stack_walk* csw, ULONG64 pc, CONTEXT *context )
{
    BYTE        insn, val8;
    WORD        val16;
    LONG        val32;
    DWORD64     val64;

    for (;;)
    {
        BYTE rex = 0;

        if (!sw_read_mem(csw, pc, &insn, 1)) return FALSE;
        if ((insn & 0xf0) == 0x40)
        {
            rex = insn & 0x0f;  /* rex prefix */
            if (!sw_read_mem(csw, ++pc, &insn, 1)) return FALSE;
        }

        switch (insn)
        {
        case 0x58: /* pop %rax/r8 */
        case 0x59: /* pop %rcx/r9 */
        case 0x5a: /* pop %rdx/r10 */
        case 0x5b: /* pop %rbx/r11 */
        case 0x5c: /* pop %rsp/r12 */
        case 0x5d: /* pop %rbp/r13 */
        case 0x5e: /* pop %rsi/r14 */
        case 0x5f: /* pop %rdi/r15 */
            if (!sw_read_mem(csw, context->Rsp, &val64, sizeof(DWORD64))) return FALSE;
            set_int_reg(context, insn - 0x58 + (rex & 1) * 8, val64);
            context->Rsp += sizeof(ULONG64);
            pc++;
            continue;
        case 0x81: /* add $nnnn,%rsp */
            if (!sw_read_mem(csw, pc + 2, &val32, sizeof(LONG))) return FALSE;
            context->Rsp += val32;
            pc += 2 + sizeof(LONG);
            continue;
        case 0x83: /* add $n,%rsp */
            if (!sw_read_mem(csw, pc + 2, &val8, sizeof(BYTE))) return FALSE;
            context->Rsp += (signed char)val8;
            pc += 3;
            continue;
        case 0x8d:
            if (!sw_read_mem(csw, pc + 1, &insn, sizeof(BYTE))) return FALSE;
            if ((insn >> 6) == 1)  /* lea n(reg),%rsp */
            {
                if (!sw_read_mem(csw, pc + 2, &val8, sizeof(BYTE))) return FALSE;
                context->Rsp = get_int_reg( context, (insn & 7) + (rex & 1) * 8 ) + (signed char)val8;
                pc += 3;
            }
            else  /* lea nnnn(reg),%rsp */
            {
                if (!sw_read_mem(csw, pc + 2, &val32, sizeof(LONG))) return FALSE;
                context->Rsp = get_int_reg( context, (insn & 7) + (rex & 1) * 8 ) + val32;
                pc += 2 + sizeof(LONG);
            }
            continue;
        case 0xc2: /* ret $nn */
            if (!sw_read_mem(csw, context->Rsp, &val64, sizeof(DWORD64))) return FALSE;
            if (!sw_read_mem(csw, pc + 1, &val16, sizeof(WORD))) return FALSE;
            context->Rip = val64;
            context->Rsp += sizeof(ULONG64) + val16;
            return TRUE;
        case 0xc3: /* ret */
        case 0xf3: /* rep; ret */
            if (!sw_read_mem(csw, context->Rsp, &val64, sizeof(DWORD64))) return FALSE;
            context->Rip = val64;
            context->Rsp += sizeof(ULONG64);
            return TRUE;
        case 0xe9: /* jmp nnnn */
            if (!sw_read_mem(csw, pc + 1, &val32, sizeof(LONG))) return FALSE;
            pc += 5 + val32;
            continue;
        case 0xeb: /* jmp n */
            if (!sw_read_mem(csw, pc + 1, &val8, sizeof(BYTE))) return FALSE;
            pc += 2 + (signed char)val8;
            continue;
        }
        FIXME("unsupported insn %x\n", insn);
        return FALSE;
    }
}

static BOOL default_unwind(struct cpu_stack_walk* csw, CONTEXT* context)
{
    if (!sw_read_mem(csw, context->Rsp, &context->Rip, sizeof(DWORD64)))
    {
        WARN("Cannot read new frame offset %s\n", wine_dbgstr_longlong(context->Rsp));
        return FALSE;
    }
    context->Rsp += sizeof(DWORD64);
    return TRUE;
}

static BOOL interpret_function_table_entry(struct cpu_stack_walk* csw,
                                           CONTEXT* context, RUNTIME_FUNCTION* function, DWORD64 base)
{
    char                buffer[sizeof(UNWIND_INFO) + 256 * sizeof(UNWIND_CODE)];
    UNWIND_INFO*        info = (UNWIND_INFO*)buffer;
    unsigned            i;
    DWORD64             newframe, prolog_offset, off, value;
    M128A               floatvalue;
    union handler_data  handler_data;

    /* FIXME: we have some assumptions here */
    assert(context);
    dump_unwind_info(csw, sw_module_base(csw, context->Rip), function);
    newframe = context->Rsp;
    for (;;)
    {
        if (!sw_read_mem(csw, base + function->UnwindData, info, sizeof(*info)) ||
            !sw_read_mem(csw, base + function->UnwindData + FIELD_OFFSET(UNWIND_INFO, UnwindCode),
                         info->UnwindCode, info->CountOfCodes * sizeof(UNWIND_CODE)))
        {
            WARN("Couldn't read unwind_code at %lx\n", base + function->UnwindData);
            return FALSE;
        }

        if (info->Version != 1)
        {
            WARN("unknown unwind info version %u at %lx\n", info->Version, base + function->UnwindData);
            return FALSE;
        }

        if (info->FrameRegister)
            newframe = get_int_reg(context, info->FrameRegister) - info->FrameOffset * 16;

        /* check if in prolog */
        if (context->Rip >= base + function->BeginAddress &&
            context->Rip < base + function->BeginAddress + info->SizeOfProlog)
        {
            prolog_offset = context->Rip - base - function->BeginAddress;
        }
        else
        {
            prolog_offset = ~0;
            if (is_inside_epilog(csw, context->Rip, base, function))
            {
                interpret_epilog(csw, context->Rip, context);
                return TRUE;
            }
        }

        for (i = 0; i < info->CountOfCodes; i += get_opcode_size(info->UnwindCode[i]))
        {
            if (prolog_offset < info->UnwindCode[i].u.CodeOffset) continue; /* skip it */

            switch (info->UnwindCode[i].u.UnwindOp)
            {
            case UWOP_PUSH_NONVOL:  /* pushq %reg */
                if (!sw_read_mem(csw, context->Rsp, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].u.OpInfo, value);
                context->Rsp += sizeof(ULONG64);
                break;
            case UWOP_ALLOC_LARGE:  /* subq $nn,%rsp */
                if (info->UnwindCode[i].u.OpInfo) context->Rsp += *(DWORD*)&info->UnwindCode[i+1];
                else context->Rsp += *(USHORT*)&info->UnwindCode[i+1] * 8;
                break;
            case UWOP_ALLOC_SMALL:  /* subq $n,%rsp */
                context->Rsp += (info->UnwindCode[i].u.OpInfo + 1) * 8;
                break;
            case UWOP_SET_FPREG:  /* leaq nn(%rsp),%framereg */
                context->Rsp = newframe;
                break;
            case UWOP_SAVE_NONVOL:  /* movq %reg,n(%rsp) */
                off = newframe + *(USHORT*)&info->UnwindCode[i+1] * 8;
                if (!sw_read_mem(csw, off, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].u.OpInfo, value);
                break;
            case UWOP_SAVE_NONVOL_FAR:  /* movq %reg,nn(%rsp) */
                off = newframe + *(DWORD*)&info->UnwindCode[i+1];
                if (!sw_read_mem(csw, off, &value, sizeof(DWORD64))) return FALSE;
                set_int_reg(context, info->UnwindCode[i].u.OpInfo, value);
                break;
            case UWOP_SAVE_XMM128:  /* movaps %xmmreg,n(%rsp) */
                off = newframe + *(USHORT*)&info->UnwindCode[i+1] * 16;
                if (!sw_read_mem(csw, off, &floatvalue, sizeof(M128A))) return FALSE;
                set_float_reg(context, info->UnwindCode[i].u.OpInfo, floatvalue);
                break;
            case UWOP_SAVE_XMM128_FAR:  /* movaps %xmmreg,nn(%rsp) */
                off = newframe + *(DWORD*)&info->UnwindCode[i+1];
                if (!sw_read_mem(csw, off, &floatvalue, sizeof(M128A))) return FALSE;
                set_float_reg(context, info->UnwindCode[i].u.OpInfo, floatvalue);
                break;
            case UWOP_PUSH_MACHFRAME:
                FIXME("PUSH_MACHFRAME %u\n", info->UnwindCode[i].u.OpInfo);
                break;
            default:
                FIXME("unknown code %u\n", info->UnwindCode[i].u.UnwindOp);
                break;
            }
        }
        if (!(info->Flags & UNW_FLAG_CHAININFO)) break;
        if (!sw_read_mem(csw, base + function->UnwindData + FIELD_OFFSET(UNWIND_INFO, UnwindCode) +
                                   ((info->CountOfCodes + 1) & ~1) * sizeof(UNWIND_CODE),
                         &handler_data, sizeof(handler_data))) return FALSE;
        function = &handler_data.chain;  /* restart with the chained info */
    }
    return default_unwind(csw, context);
}

/* fetch_next_frame()
 *
 * modify (at least) context.{rip, rsp, rbp} using unwind information
 * either out of PE exception handlers, debug info (dwarf), or simple stack unwind
 */
static BOOL fetch_next_frame(struct cpu_stack_walk* csw, CONTEXT* context,
                             DWORD_PTR curr_pc, void** prtf)
{
    DWORD_PTR               cfa;
    RUNTIME_FUNCTION*       rtf;
    DWORD64                 base;

    if (!curr_pc || !(base = sw_module_base(csw, curr_pc))) return FALSE;
    rtf = sw_table_access(csw, curr_pc);
    if (prtf) *prtf = rtf;
    if (rtf)
    {
        return interpret_function_table_entry(csw, context, rtf, base);
    }
    else if (dwarf2_virtual_unwind(csw, curr_pc, context, &cfa))
    {
        context->Rsp = cfa;
        TRACE("next function rip=%016lx\n", context->Rip);
        TRACE("  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
              context->Rax, context->Rbx, context->Rcx, context->Rdx);
        TRACE("  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
              context->Rsi, context->Rdi, context->Rbp, context->Rsp);
        TRACE("   r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
              context->R8, context->R9, context->R10, context->R11);
        TRACE("  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
              context->R12, context->R13, context->R14, context->R15);
        return TRUE;
    }
    else
        return default_unwind(csw, context);
}

static BOOL x86_64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    unsigned    deltapc = curr_count <= 1 ? 0 : 1;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;
    assert(!csw->is32);

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit",
          wine_dbgstr_longlong(curr_count));

    if (curr_mode == stm_start)
    {
        if ((frame->AddrPC.Mode == AddrModeFlat) &&
            (frame->AddrFrame.Mode != AddrModeFlat))
        {
            WARN("Bad AddrPC.Mode / AddrFrame.Mode combination\n");
            goto done_err;
        }

        /* Init done */
        curr_mode = stm_64bit;
        frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (context->Rsp != frame->AddrStack.Offset) FIXME("inconsistent Stack Pointer\n");
        if (context->Rip != frame->AddrPC.Offset) FIXME("inconsistent Instruction Pointer\n");

        if (frame->AddrReturn.Offset == 0) goto done_err;
        if (!fetch_next_frame(csw, context, frame->AddrPC.Offset - deltapc, &frame->FuncTableEntry))
            goto done_err;
        deltapc = 1;
    }

    memset(&frame->Params, 0, sizeof(frame->Params));

    /* set frame information */
    frame->AddrStack.Offset = context->Rsp;
    frame->AddrFrame.Offset = context->Rbp;
    frame->AddrPC.Offset = context->Rip;
    if (1)
    {
        CONTEXT         newctx = *context;

        if (!fetch_next_frame(csw, &newctx, frame->AddrPC.Offset - deltapc, NULL))
            goto done_err;
        frame->AddrReturn.Mode = AddrModeFlat;
        frame->AddrReturn.Offset = newctx.Rip;
    }

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    curr_count++;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : "64bit",
          wine_dbgstr_longlong(curr_count),
          frame->FuncTableEntry);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}
#else
static BOOL x86_64_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    return FALSE;
}
#endif

static void*    x86_64_find_runtime_function(struct module* module, DWORD64 addr)
{
#ifdef __x86_64__
    RUNTIME_FUNCTION*   rtf;
    ULONG               size;
    int                 min, max;

    rtf = (RUNTIME_FUNCTION*)pe_map_directory(module, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size);
    if (rtf) for (min = 0, max = size / sizeof(*rtf); min <= max; )
    {
        int pos = (min + max) / 2;
        if (addr < module->module.BaseOfImage + rtf[pos].BeginAddress) max = pos - 1;
        else if (addr >= module->module.BaseOfImage + rtf[pos].EndAddress) min = pos + 1;
        else
        {
            rtf += pos;
            while (rtf->UnwindData & 1)  /* follow chained entry */
            {
                FIXME("RunTime_Function outside IMAGE_DIRECTORY_ENTRY_EXCEPTION unimplemented yet!\n");
                /* we need to read into the other process */
                /* rtf = (RUNTIME_FUNCTION*)(module->module.BaseOfImage + (rtf->UnwindData & ~1)); */
            }
            return rtf;
        }
    }
#endif
    return NULL;
}

static unsigned x86_64_map_dwarf_register(unsigned regno, BOOL eh_frame)
{
    unsigned    reg;

    if (regno >= 17 && regno <= 24)
        reg = CV_AMD64_XMM0 + regno - 17;
    else if (regno >= 25 && regno <= 32)
        reg = CV_AMD64_XMM8 + regno - 25;
    else if (regno >= 33 && regno <= 40)
        reg = CV_AMD64_ST0 + regno - 33;
    else switch (regno)
    {
    case  0: reg = CV_AMD64_RAX;    break;
    case  1: reg = CV_AMD64_RDX;    break;
    case  2: reg = CV_AMD64_RCX;    break;
    case  3: reg = CV_AMD64_RBX;    break;
    case  4: reg = CV_AMD64_RSI;    break;
    case  5: reg = CV_AMD64_RDI;    break;
    case  6: reg = CV_AMD64_RBP;    break;
    case  7: reg = CV_AMD64_RSP;    break;
    case  8: reg = CV_AMD64_R8;     break;
    case  9: reg = CV_AMD64_R9;     break;
    case 10: reg = CV_AMD64_R10;    break;
    case 11: reg = CV_AMD64_R11;    break;
    case 12: reg = CV_AMD64_R12;    break;
    case 13: reg = CV_AMD64_R13;    break;
    case 14: reg = CV_AMD64_R14;    break;
    case 15: reg = CV_AMD64_R15;    break;
    case 16: reg = CV_AMD64_RIP;    break;
    case 49: reg = CV_AMD64_EFLAGS; break;
    case 50: reg = CV_AMD64_ES;     break;
    case 51: reg = CV_AMD64_CS;     break;
    case 52: reg = CV_AMD64_SS;     break;
    case 53: reg = CV_AMD64_DS;     break;
    case 54: reg = CV_AMD64_FS;     break;
    case 55: reg = CV_AMD64_GS;     break;
    case 62: reg = CV_AMD64_TR;     break;
    case 63: reg = CV_AMD64_LDTR;   break;
    case 64: reg = CV_AMD64_MXCSR;  break;
    case 65: reg = CV_AMD64_CTRL;   break;
    case 66: reg = CV_AMD64_STAT;   break;
/*
 * 56-57 reserved
 * 58 %fs.base
 * 59 %gs.base
 * 60-61 reserved
 */
    default:
        FIXME("Don't know how to map register %d\n", regno);
        return 0;
    }
    return reg;
}

static void* x86_64_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
#ifdef __x86_64__
    switch (regno)
    {
    case CV_AMD64_RAX: *size = sizeof(ctx->Rax); return &ctx->Rax;
    case CV_AMD64_RDX: *size = sizeof(ctx->Rdx); return &ctx->Rdx;
    case CV_AMD64_RCX: *size = sizeof(ctx->Rcx); return &ctx->Rcx;
    case CV_AMD64_RBX: *size = sizeof(ctx->Rbx); return &ctx->Rbx;
    case CV_AMD64_RSI: *size = sizeof(ctx->Rsi); return &ctx->Rsi;
    case CV_AMD64_RDI: *size = sizeof(ctx->Rdi); return &ctx->Rdi;
    case CV_AMD64_RBP: *size = sizeof(ctx->Rbp); return &ctx->Rbp;
    case CV_AMD64_RSP: *size = sizeof(ctx->Rsp); return &ctx->Rsp;
    case CV_AMD64_R8:  *size = sizeof(ctx->R8);  return &ctx->R8;
    case CV_AMD64_R9:  *size = sizeof(ctx->R9);  return &ctx->R9;
    case CV_AMD64_R10: *size = sizeof(ctx->R10); return &ctx->R10;
    case CV_AMD64_R11: *size = sizeof(ctx->R11); return &ctx->R11;
    case CV_AMD64_R12: *size = sizeof(ctx->R12); return &ctx->R12;
    case CV_AMD64_R13: *size = sizeof(ctx->R13); return &ctx->R13;
    case CV_AMD64_R14: *size = sizeof(ctx->R14); return &ctx->R14;
    case CV_AMD64_R15: *size = sizeof(ctx->R15); return &ctx->R15;
    case CV_AMD64_RIP: *size = sizeof(ctx->Rip); return &ctx->Rip;

    case CV_AMD64_XMM0 + 0: *size = sizeof(ctx->u.s.Xmm0 ); return &ctx->u.s.Xmm0;
    case CV_AMD64_XMM0 + 1: *size = sizeof(ctx->u.s.Xmm1 ); return &ctx->u.s.Xmm1;
    case CV_AMD64_XMM0 + 2: *size = sizeof(ctx->u.s.Xmm2 ); return &ctx->u.s.Xmm2;
    case CV_AMD64_XMM0 + 3: *size = sizeof(ctx->u.s.Xmm3 ); return &ctx->u.s.Xmm3;
    case CV_AMD64_XMM0 + 4: *size = sizeof(ctx->u.s.Xmm4 ); return &ctx->u.s.Xmm4;
    case CV_AMD64_XMM0 + 5: *size = sizeof(ctx->u.s.Xmm5 ); return &ctx->u.s.Xmm5;
    case CV_AMD64_XMM0 + 6: *size = sizeof(ctx->u.s.Xmm6 ); return &ctx->u.s.Xmm6;
    case CV_AMD64_XMM0 + 7: *size = sizeof(ctx->u.s.Xmm7 ); return &ctx->u.s.Xmm7;
    case CV_AMD64_XMM8 + 0: *size = sizeof(ctx->u.s.Xmm8 ); return &ctx->u.s.Xmm8;
    case CV_AMD64_XMM8 + 1: *size = sizeof(ctx->u.s.Xmm9 ); return &ctx->u.s.Xmm9;
    case CV_AMD64_XMM8 + 2: *size = sizeof(ctx->u.s.Xmm10); return &ctx->u.s.Xmm10;
    case CV_AMD64_XMM8 + 3: *size = sizeof(ctx->u.s.Xmm11); return &ctx->u.s.Xmm11;
    case CV_AMD64_XMM8 + 4: *size = sizeof(ctx->u.s.Xmm12); return &ctx->u.s.Xmm12;
    case CV_AMD64_XMM8 + 5: *size = sizeof(ctx->u.s.Xmm13); return &ctx->u.s.Xmm13;
    case CV_AMD64_XMM8 + 6: *size = sizeof(ctx->u.s.Xmm14); return &ctx->u.s.Xmm14;
    case CV_AMD64_XMM8 + 7: *size = sizeof(ctx->u.s.Xmm15); return &ctx->u.s.Xmm15;

    case CV_AMD64_ST0 + 0: *size = sizeof(ctx->u.s.Legacy[0]); return &ctx->u.s.Legacy[0];
    case CV_AMD64_ST0 + 1: *size = sizeof(ctx->u.s.Legacy[1]); return &ctx->u.s.Legacy[1];
    case CV_AMD64_ST0 + 2: *size = sizeof(ctx->u.s.Legacy[2]); return &ctx->u.s.Legacy[2];
    case CV_AMD64_ST0 + 3: *size = sizeof(ctx->u.s.Legacy[3]); return &ctx->u.s.Legacy[3];
    case CV_AMD64_ST0 + 4: *size = sizeof(ctx->u.s.Legacy[4]); return &ctx->u.s.Legacy[4];
    case CV_AMD64_ST0 + 5: *size = sizeof(ctx->u.s.Legacy[5]); return &ctx->u.s.Legacy[5];
    case CV_AMD64_ST0 + 6: *size = sizeof(ctx->u.s.Legacy[6]); return &ctx->u.s.Legacy[6];
    case CV_AMD64_ST0 + 7: *size = sizeof(ctx->u.s.Legacy[7]); return &ctx->u.s.Legacy[7];

    case CV_AMD64_EFLAGS: *size = sizeof(ctx->EFlags); return &ctx->EFlags;
    case CV_AMD64_ES: *size = sizeof(ctx->SegEs); return &ctx->SegEs;
    case CV_AMD64_CS: *size = sizeof(ctx->SegCs); return &ctx->SegCs;
    case CV_AMD64_SS: *size = sizeof(ctx->SegSs); return &ctx->SegSs;
    case CV_AMD64_DS: *size = sizeof(ctx->SegDs); return &ctx->SegDs;
    case CV_AMD64_FS: *size = sizeof(ctx->SegFs); return &ctx->SegFs;
    case CV_AMD64_GS: *size = sizeof(ctx->SegGs); return &ctx->SegGs;

    }
#endif
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static const char* x86_64_fetch_regname(unsigned regno)
{
    switch (regno)
    {
    case CV_AMD64_RAX:          return "rax";
    case CV_AMD64_RDX:          return "rdx";
    case CV_AMD64_RCX:          return "rcx";
    case CV_AMD64_RBX:          return "rbx";
    case CV_AMD64_RSI:          return "rsi";
    case CV_AMD64_RDI:          return "rdi";
    case CV_AMD64_RBP:          return "rbp";
    case CV_AMD64_RSP:          return "rsp";
    case CV_AMD64_R8:           return "r8";
    case CV_AMD64_R9:           return "r9";
    case CV_AMD64_R10:          return "r10";
    case CV_AMD64_R11:          return "r11";
    case CV_AMD64_R12:          return "r12";
    case CV_AMD64_R13:          return "r13";
    case CV_AMD64_R14:          return "r14";
    case CV_AMD64_R15:          return "r15";
    case CV_AMD64_RIP:          return "rip";

    case CV_AMD64_XMM0 + 0:     return "xmm0";
    case CV_AMD64_XMM0 + 1:     return "xmm1";
    case CV_AMD64_XMM0 + 2:     return "xmm2";
    case CV_AMD64_XMM0 + 3:     return "xmm3";
    case CV_AMD64_XMM0 + 4:     return "xmm4";
    case CV_AMD64_XMM0 + 5:     return "xmm5";
    case CV_AMD64_XMM0 + 6:     return "xmm6";
    case CV_AMD64_XMM0 + 7:     return "xmm7";
    case CV_AMD64_XMM8 + 0:     return "xmm8";
    case CV_AMD64_XMM8 + 1:     return "xmm9";
    case CV_AMD64_XMM8 + 2:     return "xmm10";
    case CV_AMD64_XMM8 + 3:     return "xmm11";
    case CV_AMD64_XMM8 + 4:     return "xmm12";
    case CV_AMD64_XMM8 + 5:     return "xmm13";
    case CV_AMD64_XMM8 + 6:     return "xmm14";
    case CV_AMD64_XMM8 + 7:     return "xmm15";

    case CV_AMD64_ST0 + 0:      return "st0";
    case CV_AMD64_ST0 + 1:      return "st1";
    case CV_AMD64_ST0 + 2:      return "st2";
    case CV_AMD64_ST0 + 3:      return "st3";
    case CV_AMD64_ST0 + 4:      return "st4";
    case CV_AMD64_ST0 + 5:      return "st5";
    case CV_AMD64_ST0 + 6:      return "st6";
    case CV_AMD64_ST0 + 7:      return "st7";

    case CV_AMD64_EFLAGS:       return "eflags";
    case CV_AMD64_ES:           return "es";
    case CV_AMD64_CS:           return "cs";
    case CV_AMD64_SS:           return "ss";
    case CV_AMD64_DS:           return "ds";
    case CV_AMD64_FS:           return "fs";
    case CV_AMD64_GS:           return "gs";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static BOOL x86_64_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __x86_64__
        ULONG64 base = ctx->Rip <= 0x80 ? 0 : ctx->Rip - 0x80;
        minidump_add_memory_block(dc, base, ctx->Rip + 0x80 - base, 0);
#endif
    }

    return TRUE;
}

static BOOL x86_64_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    /* FIXME: not sure about the flags... */
    if (1)
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __x86_64__
        struct process*         pcs;
        struct module*          module;
        const RUNTIME_FUNCTION* rtf;
        ULONG                   size;

        if (!(pcs = process_find_by_handle(dc->hProcess)) ||
            !(module = module_find_by_addr(pcs, dc->modules[index].base, DMT_UNKNOWN)))
            return FALSE;
        rtf = (const RUNTIME_FUNCTION*)pe_map_directory(module, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size);
        if (rtf)
        {
            const RUNTIME_FUNCTION* end = (const RUNTIME_FUNCTION*)((const char*)rtf + size);
            UNWIND_INFO ui;

            while (rtf + 1 < end)
            {
                while (rtf->UnwindData & 1)  /* follow chained entry */
                {
                    FIXME("RunTime_Function outside IMAGE_DIRECTORY_ENTRY_EXCEPTION unimplemented yet!\n");
                    return FALSE;
                    /* we need to read into the other process */
                    /* rtf = (RUNTIME_FUNCTION*)(module->module.BaseOfImage + (rtf->UnwindData & ~1)); */
                }
                if (ReadProcessMemory(dc->hProcess,
                                      (void*)(dc->modules[index].base + rtf->UnwindData),
                                      &ui, sizeof(ui), NULL))
                    minidump_add_memory_block(dc, dc->modules[index].base + rtf->UnwindData,
                                              FIELD_OFFSET(UNWIND_INFO, UnwindCode) + ui.CountOfCodes * sizeof(UNWIND_CODE), 0);
                rtf++;
            }
        }
#endif
    }

    return TRUE;
}

DECLSPEC_HIDDEN struct cpu cpu_x86_64 = {
    IMAGE_FILE_MACHINE_AMD64,
    8,
    CV_AMD64_RSP,
    x86_64_get_addr,
    x86_64_stack_walk,
    x86_64_find_runtime_function,
    x86_64_map_dwarf_register,
    x86_64_fetch_context_reg,
    x86_64_fetch_regname,
    x86_64_fetch_minidump_thread,
    x86_64_fetch_minidump_module,
};
