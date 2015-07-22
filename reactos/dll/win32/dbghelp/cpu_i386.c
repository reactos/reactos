/*
 * File cpu_i386.c
 *
 * Copyright (C) 2009-2009, Eric Pouech.
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

#ifndef DBGHELP_STATIC_LIB
#include <wine/winbase16.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

#define V86_FLAG  0x00020000

#define IS_VM86_MODE(ctx) (ctx->EFlags & V86_FLAG)

#if defined(__i386__) && !defined(DBGHELP_STATIC_LIB)
static ADDRESS_MODE get_selector_type(HANDLE hThread, const CONTEXT* ctx, WORD sel)
{
    LDT_ENTRY	le;

    if (IS_VM86_MODE(ctx)) return AddrModeReal;
    /* null or system selector */
    if (!(sel & 4) || ((sel >> 3) < 17)) return AddrModeFlat;
    if (hThread && GetThreadSelectorEntry(hThread, sel, &le))
        return le.HighWord.Bits.Default_Big ? AddrMode1632 : AddrMode1616;
    /* selector doesn't exist */
    return -1;
}

static BOOL i386_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS64* addr,
                            unsigned seg, unsigned long offset)
{
    addr->Mode    = AddrModeFlat;
    addr->Segment = seg;
    addr->Offset  = offset;
    if (seg)
    {
        switch (addr->Mode = get_selector_type(hThread, ctx, seg))
        {
        case AddrModeReal:
        case AddrMode1616:
            addr->Offset &= 0xffff;
            break;
        case AddrModeFlat:
        case AddrMode1632:
            break;
        default:
            return FALSE;
        }
    }
    return TRUE;
}
#endif

#ifndef DBGHELP_STATIC_LIB
static BOOL i386_get_addr(HANDLE hThread, const CONTEXT* ctx,
                          enum cpu_addr ca, ADDRESS64* addr)
{
#ifdef __i386__
    switch (ca)
    {
    case cpu_addr_pc:    return i386_build_addr(hThread, ctx, addr, ctx->SegCs, ctx->Eip);
    case cpu_addr_stack: return i386_build_addr(hThread, ctx, addr, ctx->SegSs, ctx->Esp);
    case cpu_addr_frame: return i386_build_addr(hThread, ctx, addr, ctx->SegSs, ctx->Ebp);
    }
#endif
    return FALSE;
}
#endif /* DBGHELP_STATIC_LIB */

#if defined(__i386__) && !defined(DBGHELP_STATIC_LIB)
/* fetch_next_frame32()
 *
 * modify (at least) context.{eip, esp, ebp} using unwind information
 * either out of debug info (dwarf, pdb), or simple stack unwind
 */
static BOOL fetch_next_frame32(struct cpu_stack_walk* csw,
                               CONTEXT* context, DWORD_PTR curr_pc)
{
    DWORD_PTR               xframe;
    struct pdb_cmd_pair     cpair[4];
    DWORD                   val32;

    if (dwarf2_virtual_unwind(csw, curr_pc, context, &xframe))
    {
        context->Esp = xframe;
        return TRUE;
    }
    cpair[0].name = "$ebp";      cpair[0].pvalue = &context->Ebp;
    cpair[1].name = "$esp";      cpair[1].pvalue = &context->Esp;
    cpair[2].name = "$eip";      cpair[2].pvalue = &context->Eip;
    cpair[3].name = NULL;        cpair[3].pvalue = NULL;

#ifndef DBGHELP_STATIC_LIB
    if (!pdb_virtual_unwind(csw, curr_pc, context, cpair))
#endif
    {
        /* do a simple unwind using ebp
         * we assume a "regular" prologue in the function has been used
         */
        if (!context->Ebp) return FALSE;
        context->Esp = context->Ebp + 2 * sizeof(DWORD);
        if (!sw_read_mem(csw, context->Ebp + sizeof(DWORD), &val32, sizeof(DWORD)))
        {
            WARN("Cannot read new frame offset %p\n",
                 (void*)(DWORD_PTR)(context->Ebp + (int)sizeof(DWORD)));
            return FALSE;
        }
        context->Eip = val32;
        /* "pop up" previous EBP value */
        if (!sw_read_mem(csw, context->Ebp, &val32, sizeof(DWORD)))
            return FALSE;
        context->Ebp = val32;
    }
    return TRUE;
}
#endif

enum st_mode {stm_start, stm_32bit, stm_16bit, stm_done};

/* indexes in Reserved array */
#define __CurrentModeCount      0
#define __CurrentSwitch         1
#define __NextSwitch            2

#define curr_mode   (frame->Reserved[__CurrentModeCount] & 0x0F)
#define curr_count  (frame->Reserved[__CurrentModeCount] >> 4)
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

#define set_curr_mode(m) {frame->Reserved[__CurrentModeCount] &= ~0x0F; frame->Reserved[__CurrentModeCount] |= (m & 0x0F);}
#define inc_curr_count() (frame->Reserved[__CurrentModeCount] += 0x10)

#ifndef DBGHELP_STATIC_LIB
static BOOL i386_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    STACK32FRAME        frame32;
    STACK16FRAME        frame16;
    char                ch;
    ADDRESS64           tmp;
    DWORD               p;
    WORD                val16;
    DWORD               val32;
    BOOL                do_switch;
#ifdef __i386__
    unsigned            deltapc;
    CONTEXT             _context;
#endif

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s cSwitch=%p nSwitch=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          wine_dbgstr_longlong(curr_count),
          (void*)(DWORD_PTR)curr_switch, (void*)(DWORD_PTR)next_switch);

#ifdef __i386__
    /* if we're at first call (which doesn't actually unwind, it just computes ReturnPC,
     * or if we're doing the first real unwind (count == 1), then we can directly use
     * eip. otherwise, eip is *after* the insn that actually made the call to
     * previous frame, so decrease eip by delta pc (1!) so that we're inside previous
     * insn.
     * Doing so, we ensure that the pc used for unwinding is always inside the function
     * we want to use for next frame
     */
    deltapc = curr_count <= 1 ? 0 : 1;

    if (!context)
    {
        /* setup a pseudo context for the rest of the code (esp. unwinding) */
        context = &_context;
        memset(context, 0, sizeof(*context));
        context->ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS;
        if (frame->AddrPC.Mode != AddrModeFlat)    context->SegCs = frame->AddrPC.Segment;
        context->Eip = frame->AddrPC.Offset;
        if (frame->AddrFrame.Mode != AddrModeFlat) context->SegSs = frame->AddrFrame.Segment;
        context->Ebp = frame->AddrFrame.Offset;
        if (frame->AddrStack.Mode != AddrModeFlat) context->SegSs = frame->AddrStack.Segment;
        context->Esp = frame->AddrStack.Offset;
    }
#endif
    if (curr_mode == stm_start)
    {
        THREAD_BASIC_INFORMATION info;

        if ((frame->AddrPC.Mode == AddrModeFlat) &&
            (frame->AddrFrame.Mode != AddrModeFlat))
        {
            WARN("Bad AddrPC.Mode / AddrFrame.Mode combination\n");
            goto done_err;
        }

        /* Init done */
        set_curr_mode((frame->AddrPC.Mode == AddrModeFlat) ? stm_32bit : stm_16bit);

        /* cur_switch holds address of WOW32Reserved field in TEB in debuggee
         * address space
         */
        if (NtQueryInformationThread(csw->hThread, ThreadBasicInformation, &info,
                                     sizeof(info), NULL) == STATUS_SUCCESS)
        {
            curr_switch = (DWORD_PTR)info.TebBaseAddress + FIELD_OFFSET(TEB, WOW32Reserved);
            if (!sw_read_mem(csw, curr_switch, &p, sizeof(p)))
            {
                WARN("Can't read TEB:WOW32Reserved\n");
                goto done_err;
            }
            next_switch = p;
            if (!next_switch)  /* no 16-bit stack */
            {
                curr_switch = 0;
            }
            else if (curr_mode == stm_16bit)
            {
                if (!sw_read_mem(csw, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame %p\n", (void*)(DWORD_PTR)next_switch);
                    goto done_err;
                }
                curr_switch = (DWORD)frame32.frame16;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(curr_switch);
                tmp.Offset  = OFFSETOF(curr_switch);
                if (!sw_read_mem(csw, sw_xlat_addr(csw, &tmp), &ch, sizeof(ch)))
                    curr_switch = 0xFFFFFFFF;
            }
            else
            {
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = sw_xlat_addr(csw, &tmp);
                if (!sw_read_mem(csw, p, &frame16, sizeof(frame16)))
                {
                    WARN("Bad stack frame 0x%08x\n", p);
                    goto done_err;
                }
                curr_switch = (DWORD_PTR)frame16.frame32;
                if (!sw_read_mem(csw, curr_switch, &ch, sizeof(ch)))
                    curr_switch = 0xFFFFFFFF;
            }
        }
        else
            /* FIXME: this will allow to work when we're not attached to a live target,
             * but the 16 <=> 32 switch facility won't be available.
             */
            curr_switch = 0;
        frame->AddrReturn.Mode = frame->AddrStack.Mode = (curr_mode == stm_16bit) ? AddrMode1616 : AddrModeFlat;
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
        memset(&frame->AddrBStore, 0, sizeof(frame->AddrBStore));
    }
    else
    {
        if (frame->AddrFrame.Mode == AddrModeFlat)
        {
            assert(curr_mode == stm_32bit);
            do_switch = curr_switch && frame->AddrFrame.Offset >= curr_switch;
        }
        else
        {
            assert(curr_mode == stm_16bit);
            do_switch = curr_switch &&
                frame->AddrFrame.Segment == SELECTOROF(curr_switch) &&
                frame->AddrFrame.Offset >= OFFSETOF(curr_switch);
        }

        if (do_switch)
        {
            if (curr_mode == stm_16bit)
            {
                if (!sw_read_mem(csw, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame %p\n", (void*)(DWORD_PTR)next_switch);
                    goto done_err;
                }

                frame->AddrPC.Mode        = AddrModeFlat;
                frame->AddrPC.Segment     = 0;
                frame->AddrPC.Offset      = frame32.retaddr;
                frame->AddrFrame.Mode     = AddrModeFlat;
                frame->AddrFrame.Segment  = 0;
                frame->AddrFrame.Offset   = frame32.ebp;

                frame->AddrStack.Mode     = AddrModeFlat;
                frame->AddrStack.Segment  = 0;
                frame->AddrReturn.Mode    = AddrModeFlat;
                frame->AddrReturn.Segment = 0;

                next_switch = curr_switch;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = sw_xlat_addr(csw, &tmp);

                if (!sw_read_mem(csw, p, &frame16, sizeof(frame16)))
                {
                    WARN("Bad stack frame 0x%08x\n", p);
                    goto done_err;
                }
                curr_switch = (DWORD_PTR)frame16.frame32;
                set_curr_mode(stm_32bit);
                if (!sw_read_mem(csw, curr_switch, &ch, sizeof(ch)))
                    curr_switch = 0;
            }
            else
            {
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = sw_xlat_addr(csw, &tmp);

                if (!sw_read_mem(csw, p, &frame16, sizeof(frame16)))
                {
                    WARN("Bad stack frame 0x%08x\n", p);
                    goto done_err;
                }

                TRACE("Got a 16 bit stack switch:"
                      "\n\tframe32: %p"
                      "\n\tedx:%08x ecx:%08x ebp:%08x"
                      "\n\tds:%04x es:%04x fs:%04x gs:%04x"
                      "\n\tcall_from_ip:%08x module_cs:%04x relay=%08x"
                      "\n\tentry_ip:%04x entry_point:%08x"
                      "\n\tbp:%04x ip:%04x cs:%04x\n",
                      frame16.frame32,
                      frame16.edx, frame16.ecx, frame16.ebp,
                      frame16.ds, frame16.es, frame16.fs, frame16.gs,
                      frame16.callfrom_ip, frame16.module_cs, frame16.relay,
                      frame16.entry_ip, frame16.entry_point,
                      frame16.bp, frame16.ip, frame16.cs);

                frame->AddrPC.Mode       = AddrMode1616;
                frame->AddrPC.Segment    = frame16.cs;
                frame->AddrPC.Offset     = frame16.ip;

                frame->AddrFrame.Mode    = AddrMode1616;
                frame->AddrFrame.Segment = SELECTOROF(next_switch);
                frame->AddrFrame.Offset  = frame16.bp;

                frame->AddrStack.Mode    = AddrMode1616;
                frame->AddrStack.Segment = SELECTOROF(next_switch);

                frame->AddrReturn.Mode    = AddrMode1616;
                frame->AddrReturn.Segment = frame16.cs;

                next_switch = curr_switch;
                if (!sw_read_mem(csw, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame %p\n", (void*)(DWORD_PTR)next_switch);
                    goto done_err;
                }
                curr_switch = (DWORD)frame32.frame16;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(curr_switch);
                tmp.Offset  = OFFSETOF(curr_switch);

                if (!sw_read_mem(csw, sw_xlat_addr(csw, &tmp), &ch, sizeof(ch)))
                    curr_switch = 0;
                set_curr_mode(stm_16bit);
            }
        }
        else
        {
            if (curr_mode == stm_16bit)
            {
                frame->AddrPC = frame->AddrReturn;
                frame->AddrStack.Offset = frame->AddrFrame.Offset + 2 * sizeof(WORD);
                /* "pop up" previous BP value */
                if (!frame->AddrFrame.Offset ||
                    !sw_read_mem(csw, sw_xlat_addr(csw, &frame->AddrFrame),
                                 &val16, sizeof(WORD)))
                    goto done_err;
                frame->AddrFrame.Offset = val16;
            }
            else
            {
#ifdef __i386__
                if (!fetch_next_frame32(csw, context, sw_xlat_addr(csw, &frame->AddrPC) - deltapc))
                    goto done_err;

                frame->AddrStack.Mode = frame->AddrFrame.Mode = frame->AddrPC.Mode = AddrModeFlat;
                frame->AddrStack.Offset = context->Esp;
                frame->AddrFrame.Offset = context->Ebp;
                if (frame->AddrReturn.Offset != context->Eip)
                    FIXME("new PC=%s different from Eip=%x\n",
                          wine_dbgstr_longlong(frame->AddrReturn.Offset), context->Eip);
                frame->AddrPC.Offset = context->Eip;
#endif
            }
        }
    }

    if (curr_mode == stm_16bit)
    {
        unsigned int     i;

        p = sw_xlat_addr(csw, &frame->AddrFrame);
        if (!sw_read_mem(csw, p + sizeof(WORD), &val16, sizeof(WORD)))
            goto done_err;
        frame->AddrReturn.Offset = val16;
        /* get potential cs if a far call was used */
        if (!sw_read_mem(csw, p + 2 * sizeof(WORD), &val16, sizeof(WORD)))
            goto done_err;
        if (frame->AddrFrame.Offset & 1)
            frame->AddrReturn.Segment = val16; /* far call assumed */
        else
        {
            /* not explicitly marked as far call,
             * but check whether it could be anyway
             */
            if ((val16 & 7) == 7 && val16 != frame->AddrReturn.Segment)
            {
                LDT_ENTRY	le;

                if (GetThreadSelectorEntry(csw->hThread, val16, &le) &&
                    (le.HighWord.Bits.Type & 0x08)) /* code segment */
                {
                    /* it is very uncommon to push a code segment cs as
                     * a parameter, so this should work in most cases
                     */
                    frame->AddrReturn.Segment = val16;
                }
	    }
	}
        frame->AddrFrame.Offset &= ~1;
        /* we "pop" parameters as 16 bit entities... of course, this won't
         * work if the parameter is in fact bigger than 16bit, but
         * there's no way to know that here
         */
        for (i = 0; i < sizeof(frame->Params) / sizeof(frame->Params[0]); i++)
        {
            sw_read_mem(csw, p + (2 + i) * sizeof(WORD), &val16, sizeof(val16));
            frame->Params[i] = val16;
        }
#ifdef __i386__
        if (context)
        {
#define SET(field, seg, reg) \
            switch (frame->field.Mode) \
            { \
            case AddrModeFlat: context->reg = frame->field.Offset; break; \
            case AddrMode1616: context->seg = frame->field.Segment; context->reg = frame->field.Offset; break; \
            default: assert(0); \
            }
            SET(AddrStack,  SegSs, Esp);
            SET(AddrFrame,  SegSs, Ebp);
            SET(AddrReturn, SegCs, Eip);
#undef SET
        }
#endif
    }
    else
    {
        unsigned int    i;
#ifdef __i386__
        CONTEXT         newctx = *context;

        if (!fetch_next_frame32(csw, &newctx, frame->AddrPC.Offset - deltapc))
            goto done_err;
        frame->AddrReturn.Mode = AddrModeFlat;
        frame->AddrReturn.Offset = newctx.Eip;
#endif
        for (i = 0; i < sizeof(frame->Params) / sizeof(frame->Params[0]); i++)
        {
            sw_read_mem(csw, frame->AddrFrame.Offset + (2 + i) * sizeof(DWORD), &val32, sizeof(val32));
            frame->Params[i] = val32;
        }
    }

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    p = sw_xlat_addr(csw, &frame->AddrPC);
    if (p && sw_module_base(csw, p))
        frame->FuncTableEntry = sw_table_access(csw, p);
    else
        frame->FuncTableEntry = NULL;

    inc_curr_count();
    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s Count=%s cSwitch=%p nSwitch=%p FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          wine_dbgstr_longlong(curr_count),
          (void*)(DWORD_PTR)curr_switch, (void*)(DWORD_PTR)next_switch, frame->FuncTableEntry);

    return TRUE;
done_err:
    set_curr_mode(stm_done);
    return FALSE;
}
#endif /* DBGHELP_STATIC_LIB */

static unsigned i386_map_dwarf_register(unsigned regno, BOOL eh_frame)
{
    unsigned    reg;

    switch (regno)
    {
    case  0: reg = CV_REG_EAX; break;
    case  1: reg = CV_REG_ECX; break;
    case  2: reg = CV_REG_EDX; break;
    case  3: reg = CV_REG_EBX; break;
    case  4:
    case  5:
#ifdef __APPLE__
        /* On OS X, DWARF eh_frame uses a different mapping for the registers.  It's
           apparently the mapping as emitted by GCC, at least at some point in its history. */
        if (eh_frame)
            reg = (regno == 4) ? CV_REG_EBP : CV_REG_ESP;
        else
#endif
            reg = (regno == 4) ? CV_REG_ESP : CV_REG_EBP;
        break;
    case  6: reg = CV_REG_ESI; break;
    case  7: reg = CV_REG_EDI; break;
    case  8: reg = CV_REG_EIP; break;
    case  9: reg = CV_REG_EFLAGS; break;
    case 10: reg = CV_REG_CS;  break;
    case 11: reg = CV_REG_SS;  break;
    case 12: reg = CV_REG_DS;  break;
    case 13: reg = CV_REG_ES;  break;
    case 14: reg = CV_REG_FS;  break;
    case 15: reg = CV_REG_GS;  break;
    case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23:
        reg = CV_REG_ST0 + regno - 16; break;
    case 24: reg = CV_REG_CTRL; break;
    case 25: reg = CV_REG_STAT; break;
    case 26: reg = CV_REG_TAG; break;
    case 27: reg = CV_REG_FPCS; break;
    case 28: reg = CV_REG_FPIP; break;
    case 29: reg = CV_REG_FPDS; break;
    case 30: reg = CV_REG_FPDO; break;
/*
reg: fop   31
*/
    case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39:
        reg = CV_REG_XMM0 + regno - 32; break;
    case 40: reg = CV_REG_MXCSR; break;
    default:
        FIXME("Don't know how to map register %d\n", regno);
        return 0;
    }
    return reg;
}

static void* i386_fetch_context_reg(CONTEXT* ctx, unsigned regno, unsigned* size)
{
#ifdef __i386__
    switch (regno)
    {
    case CV_REG_EAX: *size = sizeof(ctx->Eax); return &ctx->Eax;
    case CV_REG_EDX: *size = sizeof(ctx->Edx); return &ctx->Edx;
    case CV_REG_ECX: *size = sizeof(ctx->Ecx); return &ctx->Ecx;
    case CV_REG_EBX: *size = sizeof(ctx->Ebx); return &ctx->Ebx;
    case CV_REG_ESI: *size = sizeof(ctx->Esi); return &ctx->Esi;
    case CV_REG_EDI: *size = sizeof(ctx->Edi); return &ctx->Edi;
    case CV_REG_EBP: *size = sizeof(ctx->Ebp); return &ctx->Ebp;
    case CV_REG_ESP: *size = sizeof(ctx->Esp); return &ctx->Esp;
    case CV_REG_EIP: *size = sizeof(ctx->Eip); return &ctx->Eip;

    /* These are x87 floating point registers... They do not match a C type in
     * the Linux ABI, so hardcode their 80-bitness. */
    case CV_REG_ST0 + 0: *size = 10; return &ctx->FloatSave.RegisterArea[0*10];
    case CV_REG_ST0 + 1: *size = 10; return &ctx->FloatSave.RegisterArea[1*10];
    case CV_REG_ST0 + 2: *size = 10; return &ctx->FloatSave.RegisterArea[2*10];
    case CV_REG_ST0 + 3: *size = 10; return &ctx->FloatSave.RegisterArea[3*10];
    case CV_REG_ST0 + 4: *size = 10; return &ctx->FloatSave.RegisterArea[4*10];
    case CV_REG_ST0 + 5: *size = 10; return &ctx->FloatSave.RegisterArea[5*10];
    case CV_REG_ST0 + 6: *size = 10; return &ctx->FloatSave.RegisterArea[6*10];
    case CV_REG_ST0 + 7: *size = 10; return &ctx->FloatSave.RegisterArea[7*10];

    case CV_REG_CTRL: *size = sizeof(DWORD); return &ctx->FloatSave.ControlWord;
    case CV_REG_STAT: *size = sizeof(DWORD); return &ctx->FloatSave.StatusWord;
    case CV_REG_TAG:  *size = sizeof(DWORD); return &ctx->FloatSave.TagWord;
    case CV_REG_FPCS: *size = sizeof(DWORD); return &ctx->FloatSave.ErrorSelector;
    case CV_REG_FPIP: *size = sizeof(DWORD); return &ctx->FloatSave.ErrorOffset;
    case CV_REG_FPDS: *size = sizeof(DWORD); return &ctx->FloatSave.DataSelector;
    case CV_REG_FPDO: *size = sizeof(DWORD); return &ctx->FloatSave.DataOffset;

    case CV_REG_EFLAGS: *size = sizeof(ctx->EFlags); return &ctx->EFlags;
    case CV_REG_ES: *size = sizeof(ctx->SegEs); return &ctx->SegEs;
    case CV_REG_CS: *size = sizeof(ctx->SegCs); return &ctx->SegCs;
    case CV_REG_SS: *size = sizeof(ctx->SegSs); return &ctx->SegSs;
    case CV_REG_DS: *size = sizeof(ctx->SegDs); return &ctx->SegDs;
    case CV_REG_FS: *size = sizeof(ctx->SegFs); return &ctx->SegFs;
    case CV_REG_GS: *size = sizeof(ctx->SegGs); return &ctx->SegGs;

    }
#endif
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

static const char* i386_fetch_regname(unsigned regno)
{
    switch (regno)
    {
    case CV_REG_EAX: return "eax";
    case CV_REG_EDX: return "edx";
    case CV_REG_ECX: return "ecx";
    case CV_REG_EBX: return "ebx";
    case CV_REG_ESI: return "esi";
    case CV_REG_EDI: return "edi";
    case CV_REG_EBP: return "ebp";
    case CV_REG_ESP: return "esp";
    case CV_REG_EIP: return "eip";

    case CV_REG_ST0 + 0: return "st0";
    case CV_REG_ST0 + 1: return "st1";
    case CV_REG_ST0 + 2: return "st2";
    case CV_REG_ST0 + 3: return "st3";
    case CV_REG_ST0 + 4: return "st4";
    case CV_REG_ST0 + 5: return "st5";
    case CV_REG_ST0 + 6: return "st6";
    case CV_REG_ST0 + 7: return "st7";

    case CV_REG_EFLAGS: return "eflags";
    case CV_REG_ES: return "es";
    case CV_REG_CS: return "cs";
    case CV_REG_SS: return "ss";
    case CV_REG_DS: return "ds";
    case CV_REG_FS: return "fs";
    case CV_REG_GS: return "gs";

    case CV_REG_CTRL: return "fpControl";
    case CV_REG_STAT: return "fpStatus";
    case CV_REG_TAG:  return "fpTag";
    case CV_REG_FPCS: return "fpCS";
    case CV_REG_FPIP: return "fpIP";
    case CV_REG_FPDS: return "fpDS";
    case CV_REG_FPDO: return "fpData";

    case CV_REG_XMM0 + 0: return "xmm0";
    case CV_REG_XMM0 + 1: return "xmm1";
    case CV_REG_XMM0 + 2: return "xmm2";
    case CV_REG_XMM0 + 3: return "xmm3";
    case CV_REG_XMM0 + 4: return "xmm4";
    case CV_REG_XMM0 + 5: return "xmm5";
    case CV_REG_XMM0 + 6: return "xmm6";
    case CV_REG_XMM0 + 7: return "xmm7";

    case CV_REG_MXCSR: return "MxCSR";
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

#ifndef DBGHELP_STATIC_LIB
static BOOL i386_fetch_minidump_thread(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx)
{
    if (ctx->ContextFlags && (flags & ThreadWriteInstructionWindow))
    {
        /* FIXME: crop values across module boundaries, */
#ifdef __i386__
        ULONG base = ctx->Eip <= 0x80 ? 0 : ctx->Eip - 0x80;
        minidump_add_memory_block(dc, base, ctx->Eip + 0x80 - base, 0);
#endif
    }

    return TRUE;
}
#endif

static BOOL i386_fetch_minidump_module(struct dump_context* dc, unsigned index, unsigned flags)
{
    /* FIXME: actually, we should probably take care of FPO data, unless it's stored in
     * function table minidump stream
     */
    return FALSE;
}

DECLSPEC_HIDDEN struct cpu cpu_i386 = {
    IMAGE_FILE_MACHINE_I386,
    4,
    CV_REG_EBP,
#ifndef DBGHELP_STATIC_LIB
    i386_get_addr,
    i386_stack_walk,
#else
    NULL,
    NULL,
#endif
    NULL,
    i386_map_dwarf_register,
    i386_fetch_context_reg,
    i386_fetch_regname,
#ifndef DBGHELP_STATIC_LIB
    i386_fetch_minidump_thread,
    i386_fetch_minidump_module,
#else
    NULL,
    NULL,
#endif
};
