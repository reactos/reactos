/*
 * Stack walking
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Eric Youngdale
 * Copyright 1999 Ove Kåven
 * Copyright 2004 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dbghelp_private.h"
#include "winreg.h"
#include "thread.h" /* FIXME: must be included before winternl.h */
#include "wine/debug.h"
#include "stackframe.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

enum st_mode {stm_start, stm_32bit, stm_16bit, stm_done};

static const char* wine_dbgstr_addr(const ADDRESS* addr)
{
    if (!addr) return "(null)";
    switch (addr->Mode)
    {
    case AddrModeFlat:
        return wine_dbg_sprintf("flat<%08lx>", addr->Offset);
    case AddrMode1616:
        return wine_dbg_sprintf("1616<%04x:%04lx>", addr->Segment, addr->Offset);
    case AddrMode1632:
        return wine_dbg_sprintf("1632<%04x:%08lx>", addr->Segment, addr->Offset);
    case AddrModeReal:
        return wine_dbg_sprintf("real<%04x:%04lx>", addr->Segment, addr->Offset);
    default:
        return "unknown";
    }
}

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentSwitch   1
#define __NextSwitch      2

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

/***********************************************************************
 *		StackWalk (DBGHELP.@)
 */
BOOL WINAPI StackWalk(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                      LPSTACKFRAME frame, LPVOID ctx,
                      PREAD_PROCESS_MEMORY_ROUTINE f_read_mem,
                      PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                      PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                      PTRANSLATE_ADDRESS_ROUTINE f_xlat_adr)
{
    STACK32FRAME        frame32;
    STACK16FRAME        frame16;
    char                ch;
    ADDRESS             tmp;
    DWORD               p;
    WORD                val;
    BOOL                do_switch;

    TRACE("(%ld, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (MachineType != IMAGE_FILE_MACHINE_I386)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    if (!f_read_mem) f_read_mem = ReadProcessMemory;
    if (!f_xlat_adr) f_xlat_adr = addr_to_linear;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%08lx nSwitch=%08lx\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          curr_switch, next_switch);

    if (curr_mode == stm_start)
    {
        /*THREAD_BASIC_INFORMATION info;*/

        if ((frame->AddrPC.Mode == AddrModeFlat) &&
            (frame->AddrFrame.Mode != AddrModeFlat))
        {
            WARN("Bad AddrPC.Mode / AddrFrame.Mode combination\n");
            goto done_err;
        }

        /* Init done */
        curr_mode = (frame->AddrPC.Mode == AddrModeFlat) ?
            stm_32bit : stm_16bit;

        /* cur_switch holds address of curr_stack's field in TEB in debuggee
         * address space
         */
        /*
        if (NtQueryInformationThread(hThread, ThreadBasicInformation, &info,
                                     sizeof(info), NULL) != STATUS_SUCCESS)
            goto done_err;
        curr_switch = (unsigned long)info.TebBaseAddress + FIELD_OFFSET(TEB, cur_stack); */
        if (!f_read_mem(hProcess, (void*)curr_switch, &next_switch,
                        sizeof(next_switch), NULL))
        {
            WARN("Can't read TEB:cur_stack\n");
            goto done_err;
        }
        if (curr_mode == stm_16bit)
        {
            if (!f_read_mem(hProcess, (void*)next_switch, &frame32,
                            sizeof(frame32), NULL))
            {
                WARN("Bad stack frame 0x%08lx\n", next_switch);
                goto done_err;
            }
            curr_switch = (DWORD)frame32.frame16;
            tmp.Mode    = AddrMode1616;
            tmp.Segment = SELECTOROF(curr_switch);
            tmp.Offset  = OFFSETOF(curr_switch);
            if (!f_read_mem(hProcess, (void*)f_xlat_adr(hProcess, hThread, &tmp),
                            &ch, sizeof(ch), NULL))
                curr_switch = 0xFFFFFFFF;
            frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrMode1616;
        }
        else
        {
            tmp.Mode    = AddrMode1616;
            tmp.Segment = SELECTOROF(next_switch);
            tmp.Offset  = OFFSETOF(next_switch);
            p = f_xlat_adr(hProcess, hThread, &tmp);
            if (!f_read_mem(hProcess, (void*)p, &frame16, sizeof(frame16), NULL))
            {
                WARN("Bad stack frame 0x%08lx\n", p);
                goto done_err;
            }
            curr_switch = (DWORD)frame16.frame32;

            if (!f_read_mem(hProcess, (void*)curr_switch, &ch, sizeof(ch), NULL))
                curr_switch = 0xFFFFFFFF;
            frame->AddrReturn.Mode = frame->AddrStack.Mode = AddrModeFlat;
        }
        /* don't set up AddrStack on first call. Either the caller has set it up, or
         * we will get it in the next frame
         */
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
            do_switch = OFFSETOF(curr_switch) &&
                frame->AddrFrame.Segment == SELECTOROF(curr_switch) &&
                frame->AddrFrame.Offset >= OFFSETOF(curr_switch);
        }

        if (do_switch)
        {
            if (curr_mode == stm_16bit)
            {
                if (!f_read_mem(hProcess, (void*)next_switch, &frame32,
                                sizeof(frame32), NULL))
                {
                    WARN("Bad stack frame 0x%08lx\n", next_switch);
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
                p = f_xlat_adr(hProcess, hThread, &tmp);

                if (!f_read_mem(hProcess, (void*)p, &frame16, sizeof(frame16), NULL))
                {
                    WARN("Bad stack frame 0x%08lx\n", p);
                    goto done_err;
                }
                curr_switch = (DWORD)frame16.frame32;
                curr_mode = stm_32bit;
                if (!f_read_mem(hProcess, (void*)curr_switch, &ch, sizeof(ch), NULL))
                    curr_switch = 0xFFFFFFFF;
            }
            else
            {
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = f_xlat_adr(hProcess, hThread, &tmp);

                if (!f_read_mem(hProcess, (void*)p, &frame16, sizeof(frame16), NULL))
                {
                    WARN("Bad stack frame 0x%08lx\n", p);
                    goto done_err;
                }

                TRACE("Got a 16 bit stack switch:"
                      "\n\tframe32: %08lx"
                      "\n\tedx:%08lx ecx:%08lx ebp:%08lx"
                      "\n\tds:%04x es:%04x fs:%04x gs:%04x"
                      "\n\tcall_from_ip:%08lx module_cs:%04lx relay=%08lx"
                      "\n\tentry_ip:%04x entry_point:%08lx"
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
                if (!f_read_mem(hProcess, (void*)next_switch, &frame32, sizeof(frame32),
                                NULL))
                {
                    WARN("Bad stack frame 0x%08lx\n", next_switch);
                    goto done_err;
                }
                curr_switch = (DWORD)frame32.frame16;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(curr_switch);
                tmp.Offset  = OFFSETOF(curr_switch);

                if (!f_read_mem(hProcess, (void*)f_xlat_adr(hProcess, hThread, &tmp),
                                &ch, sizeof(ch), NULL))
                    curr_switch = 0xFFFFFFFF;
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
                if (!f_read_mem(hProcess,
                                (void*)f_xlat_adr(hProcess, hThread, &frame->AddrFrame),
                                &val, sizeof(WORD), NULL))
                    goto done_err;
                frame->AddrFrame.Offset = val;
            }
            else
            {
                frame->AddrStack.Offset = frame->AddrFrame.Offset + 2 * sizeof(DWORD);
                /* "pop up" previous EBP value */
                if (!f_read_mem(hProcess, (void*)frame->AddrFrame.Offset,
                                &frame->AddrFrame.Offset, sizeof(DWORD), NULL))
                    goto done_err;
            }
        }
    }

    if (curr_mode == stm_16bit)
    {
        int     i;

        p = f_xlat_adr(hProcess, hThread, &frame->AddrFrame);
        if (!f_read_mem(hProcess, (void*)(p + sizeof(WORD)), &val, sizeof(WORD), NULL))
            goto done_err;
        frame->AddrReturn.Offset = val;
        /* get potential cs if a far call was used */
        if (!f_read_mem(hProcess, (void*)(p + 2 * sizeof(WORD)),
                        &val, sizeof(WORD), NULL))
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

                if (GetThreadSelectorEntry(hThread, val, &le) &&
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
            f_read_mem(hProcess, (void*)(p + (2 + i) * sizeof(WORD)),
                       &val, sizeof(val), NULL);
            frame->Params[i] = val;
        }
    }
    else
    {
        if (!f_read_mem(hProcess,
                        (void*)(frame->AddrFrame.Offset + sizeof(DWORD)),
                        &frame->AddrReturn.Offset, sizeof(DWORD), NULL))
            goto done_err;
        f_read_mem(hProcess,
                   (void*)(frame->AddrFrame.Offset + 2 * sizeof(DWORD)),
                   frame->Params, sizeof(frame->Params), NULL);
    }

    frame->Far = FALSE;
    frame->Virtual = FALSE;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%08lx nSwitch=%08lx\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack),
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          curr_switch, next_switch);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}
