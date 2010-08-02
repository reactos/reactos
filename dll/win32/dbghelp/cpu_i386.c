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

#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

#define STEP_FLAG 0x00000100 /* single step flag */
#define V86_FLAG  0x00020000

#define IS_VM86_MODE(ctx) (ctx->EFlags & V86_FLAG)

#ifdef __i386__
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

static unsigned i386_build_addr(HANDLE hThread, const CONTEXT* ctx, ADDRESS64* addr,
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

static unsigned i386_get_addr(HANDLE hThread, const CONTEXT* ctx,
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

enum st_mode {stm_start, stm_32bit, stm_16bit, stm_done};

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentSwitch   1
#define __NextSwitch      2

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

static BOOL i386_stack_walk(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context)
{
    STACK32FRAME        frame32;
    STACK16FRAME        frame16;
    char                ch;
    ADDRESS64           tmp;
    DWORD               p;
    WORD                val;
    BOOL                do_switch;
    unsigned            deltapc = 1;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%p nSwitch=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          (void*)(DWORD_PTR)curr_switch, (void*)(DWORD_PTR)next_switch);

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
        curr_mode = (frame->AddrPC.Mode == AddrModeFlat) ? stm_32bit : stm_16bit;
        deltapc = 0;

        /* cur_switch holds address of WOW32Reserved field in TEB in debuggee
         * address space
         */
        if (NtQueryInformationThread(csw->hThread, ThreadBasicInformation, &info,
                                     sizeof(info), NULL) == STATUS_SUCCESS)
        {
            curr_switch = (unsigned long)info.TebBaseAddress + FIELD_OFFSET(TEB, WOW32Reserved);
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
#ifdef __i386__
        if (curr_mode == stm_32bit)
        {
            DWORD_PTR       xframe;

            if (dwarf2_virtual_unwind(csw, frame->AddrPC.Offset - deltapc, context, &xframe))
            {
                frame->AddrStack.Mode = frame->AddrFrame.Mode = frame->AddrReturn.Mode = AddrModeFlat;
                frame->AddrStack.Offset = context->Esp = xframe;
                frame->AddrFrame.Offset = context->Ebp;
                frame->AddrReturn.Offset = context->Eip;
                goto done_pep;
            }
        }
#endif
    }
    else
    {
        if (frame->AddrFrame.Offset == 0) goto done_err;
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
                curr_mode = stm_32bit;
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
                      "\n\tframe32: %08lx"
                      "\n\tedx:%08x ecx:%08x ebp:%08x"
                      "\n\tds:%04x es:%04x fs:%04x gs:%04x"
                      "\n\tcall_from_ip:%08x module_cs:%04x relay=%08x"
                      "\n\tentry_ip:%04x entry_point:%08x"
                      "\n\tbp:%04x ip:%04x cs:%04x\n",
                      (unsigned long)frame16.frame32,
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
                curr_mode = stm_16bit;
            }
        }
        else
        {
            frame->AddrPC = frame->AddrReturn;
            if (curr_mode == stm_16bit)
            {
                frame->AddrStack.Offset = frame->AddrFrame.Offset + 2 * sizeof(WORD);
                /* "pop up" previous BP value */
                if (!sw_read_mem(csw, sw_xlat_addr(csw, &frame->AddrFrame),
                                 &val, sizeof(WORD)))
                    goto done_err;
                frame->AddrFrame.Offset = val;
            }
            else
            {
#ifdef __i386__
                DWORD_PTR       xframe;

                if (dwarf2_virtual_unwind(csw, frame->AddrPC.Offset - deltapc, context, &xframe))
                {
                    frame->AddrStack.Mode = frame->AddrFrame.Mode = frame->AddrReturn.Mode = AddrModeFlat;
                    frame->AddrStack.Offset = context->Esp = xframe;
                    frame->AddrFrame.Offset = context->Ebp;
                    frame->AddrReturn.Offset = context->Eip;
                    goto done_pep;
                }
#endif
                frame->AddrStack.Offset = frame->AddrFrame.Offset + 2 * sizeof(DWORD);
                /* "pop up" previous EBP value */
                if (!sw_read_mem(csw, frame->AddrFrame.Offset,
                                 &frame->AddrFrame.Offset, sizeof(DWORD)))
                    goto done_err;
            }
        }
    }

    if (curr_mode == stm_16bit)
    {
        unsigned int     i;

        p = sw_xlat_addr(csw, &frame->AddrFrame);
        if (!sw_read_mem(csw, p + sizeof(WORD), &val, sizeof(WORD)))
            goto done_err;
        frame->AddrReturn.Offset = val;
        /* get potential cs if a far call was used */
        if (!sw_read_mem(csw, p + 2 * sizeof(WORD), &val, sizeof(WORD)))
            goto done_err;
        if (frame->AddrFrame.Offset & 1)
            frame->AddrReturn.Segment = val; /* far call assumed */
        else
        {
            /* not explicitly marked as far call,
             * but check whether it could be anyway
             */
            if ((val & 7) == 7 && val != frame->AddrReturn.Segment)
            {
                LDT_ENTRY	le;

                if (GetThreadSelectorEntry(csw->hThread, val, &le) &&
                    (le.HighWord.Bits.Type & 0x08)) /* code segment */
                {
                    /* it is very uncommon to push a code segment cs as
                     * a parameter, so this should work in most cases
                     */
                    frame->AddrReturn.Segment = val;
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
            sw_read_mem(csw, p + (2 + i) * sizeof(WORD), &val, sizeof(val));
            frame->Params[i] = val;
        }
    }
    else
    {
        if (!sw_read_mem(csw, frame->AddrFrame.Offset + sizeof(DWORD),
                         &frame->AddrReturn.Offset, sizeof(DWORD)))
        {
            WARN("Cannot read new frame offset %p\n",
                 (void*)(DWORD_PTR)(frame->AddrFrame.Offset + (int)sizeof(DWORD)));
            goto done_err;
        }
        sw_read_mem(csw, frame->AddrFrame.Offset + 2 * sizeof(DWORD),
                    frame->Params, sizeof(frame->Params));
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
done_pep:
#endif

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    p = sw_xlat_addr(csw, &frame->AddrPC);
    if (p && sw_module_base(csw, p))
        frame->FuncTableEntry = sw_table_access(csw, p);
    else
        frame->FuncTableEntry = NULL;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%p nSwitch=%p FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          (void*)(DWORD_PTR)curr_switch, (void*)(DWORD_PTR)next_switch, frame->FuncTableEntry);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}

static unsigned i386_map_dwarf_register(unsigned regno)
{
    unsigned    reg;

    switch (regno)
    {
    case  0: reg = CV_REG_EAX; break;
    case  1: reg = CV_REG_ECX; break;
    case  2: reg = CV_REG_EDX; break;
    case  3: reg = CV_REG_EBX; break;
    case  4: reg = CV_REG_ESP; break;
    case  5: reg = CV_REG_EBP; break;
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
/*
reg: fiseg 27
reg: fioff 28
reg: foseg 29
reg: fooff 30
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

    case CV_REG_ST0 + 0: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[0*sizeof(long double)];
    case CV_REG_ST0 + 1: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[1*sizeof(long double)];
    case CV_REG_ST0 + 2: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[2*sizeof(long double)];
    case CV_REG_ST0 + 3: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[3*sizeof(long double)];
    case CV_REG_ST0 + 4: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[4*sizeof(long double)];
    case CV_REG_ST0 + 5: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[5*sizeof(long double)];
    case CV_REG_ST0 + 6: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[6*sizeof(long double)];
    case CV_REG_ST0 + 7: *size = sizeof(long double); return &ctx->FloatSave.RegisterArea[7*sizeof(long double)];

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
    }
    FIXME("Unknown register %x\n", regno);
    return NULL;
}

struct cpu cpu_i386 = {
    IMAGE_FILE_MACHINE_I386,
    4,
    i386_get_addr,
    i386_stack_walk,
    NULL,
    i386_map_dwarf_register,
    i386_fetch_context_reg,
    i386_fetch_regname,
};
