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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

enum st_mode {stm_start, stm_32bit, stm_16bit, stm_done};

static const char* wine_dbgstr_addr(const ADDRESS* addr)
{
    if (!addr) return "(null)";
    switch (addr->Mode)
    {
    case AddrModeFlat:
        return wine_dbg_sprintf("flat<%08x>", addr->Offset);
    case AddrMode1616:
        return wine_dbg_sprintf("1616<%04x:%04x>", addr->Segment, addr->Offset);
    case AddrMode1632:
        return wine_dbg_sprintf("1632<%04x:%08x>", addr->Segment, addr->Offset);
    case AddrModeReal:
        return wine_dbg_sprintf("real<%04x:%04x>", addr->Segment, addr->Offset);
    default:
        return "unknown";
    }
}

static BOOL CALLBACK read_mem(HANDLE hProcess, DWORD addr, void* buffer,
                              DWORD size, LPDWORD nread)
{
    SIZE_T      r;
    if (!ReadProcessMemory(hProcess, (void*)addr, buffer, size, &r)) return FALSE;
    if (nread) *nread = r;
    return TRUE;
}

static BOOL CALLBACK read_mem64(HANDLE hProcess, DWORD64 addr, void* buffer,
                                DWORD size, LPDWORD nread)
{
    SIZE_T      r;
    if (!ReadProcessMemory(hProcess, (void*)(DWORD_PTR)addr, buffer, size, &r)) return FALSE;
    if (nread) *nread = r;
    return TRUE;
}

/* indexes in Reserved array */
#define __CurrentMode     0
#define __CurrentSwitch   1
#define __NextSwitch      2

#define curr_mode   (frame->Reserved[__CurrentMode])
#define curr_switch (frame->Reserved[__CurrentSwitch])
#define next_switch (frame->Reserved[__NextSwitch])

struct stack_walk_callback
{
    HANDLE      hProcess;
    HANDLE      hThread;
    BOOL        is32;
    union
    {
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE        f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE          f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE      f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE            f_modl_bas;
        } s32;
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE64      f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE64        f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE64    f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE64          f_modl_bas;
        } s64;
    } u;
};

static inline void addr_32to64(const ADDRESS* addr32, ADDRESS64* addr64)
{
    addr64->Offset = (ULONG64)addr32->Offset;
    addr64->Segment = addr32->Segment;
    addr64->Mode = addr32->Mode;
}

static inline void addr_64to32(const ADDRESS64* addr64, ADDRESS* addr32)
{
    addr32->Offset = (ULONG)addr64->Offset;
    addr32->Segment = addr64->Segment;
    addr32->Mode = addr64->Mode;
}

static inline BOOL sw_read_mem(struct stack_walk_callback* cb, DWORD addr, void* ptr, DWORD sz)
{
    if (cb->is32)
        return cb->u.s32.f_read_mem(cb->hProcess, addr, ptr, sz, NULL);
    else
        return cb->u.s64.f_read_mem(cb->hProcess, addr, ptr, sz, NULL);
}

static inline DWORD sw_xlat_addr(struct stack_walk_callback* cb, ADDRESS* addr)
{
    if (addr->Mode == AddrModeFlat) return addr->Offset;
    if (cb->is32) return cb->u.s32.f_xlat_adr(cb->hProcess, cb->hThread, addr);
    if (cb->u.s64.f_xlat_adr)
    {
        ADDRESS64       addr64;

        addr_32to64(addr, &addr64);
        return cb->u.s64.f_xlat_adr(cb->hProcess, cb->hThread, &addr64);
    }
    return addr_to_linear(cb->hProcess, cb->hThread, addr);
}

static inline void* sw_tabl_acs(struct stack_walk_callback* cb, DWORD addr)
{
    if (cb->is32)
        return cb->u.s32.f_tabl_acs(cb->hProcess, addr);
    else
        return cb->u.s64.f_tabl_acs(cb->hProcess, addr);
}

static inline DWORD sw_modl_bas(struct stack_walk_callback* cb, DWORD addr)
{
    if (cb->is32)
        return cb->u.s32.f_modl_bas(cb->hProcess, addr);
    else
        return cb->u.s64.f_modl_bas(cb->hProcess, addr);
}

static BOOL stack_walk(struct stack_walk_callback* cb, LPSTACKFRAME frame)
{
    STACK32FRAME        frame32;
    STACK16FRAME        frame16;
    char                ch;
    ADDRESS             tmp;
    DWORD               p;
    WORD                val;
    BOOL                do_switch;

    /* sanity check */
    if (curr_mode >= stm_done) return FALSE;

    TRACE("Enter: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%08x nSwitch=%08x\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack), 
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          curr_switch, next_switch);

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
        curr_mode = (frame->AddrPC.Mode == AddrModeFlat) ? 
            stm_32bit : stm_16bit;

        /* cur_switch holds address of WOW32Reserved field in TEB in debuggee
         * address space
         */
        if (NtQueryInformationThread(cb->hThread, ThreadBasicInformation, &info,
                                     sizeof(info), NULL) == STATUS_SUCCESS)
        {
            curr_switch = (unsigned long)info.TebBaseAddress + FIELD_OFFSET(TEB, WOW32Reserved);
            if (!sw_read_mem(cb, curr_switch, &next_switch, sizeof(next_switch)))
            {
                WARN("Can't read TEB:WOW32Reserved\n");
                goto done_err;
            }
            if (curr_mode == stm_16bit)
            {
                if (!sw_read_mem(cb, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame 0x%08x\n", next_switch);
                    goto done_err;
                }
                curr_switch = (DWORD)frame32.frame16;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(curr_switch);
                tmp.Offset  = OFFSETOF(curr_switch);
                if (!sw_read_mem(cb, sw_xlat_addr(cb, &tmp), &ch, sizeof(ch)))
                    curr_switch = 0xFFFFFFFF;
            }
            else
            {
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = sw_xlat_addr(cb, &tmp);
                if (!sw_read_mem(cb, p, &frame16, sizeof(frame16)))
                {
                    WARN("Bad stack frame 0x%08x\n", p);
                    goto done_err;
                }
                curr_switch = (DWORD)frame16.frame32;

                if (!sw_read_mem(cb, curr_switch, &ch, sizeof(ch)))
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
                if (!sw_read_mem(cb, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame 0x%08x\n", next_switch);
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
                p = sw_xlat_addr(cb, &tmp);

                if (!sw_read_mem(cb, p, &frame16, sizeof(frame16)))
                {
                    WARN("Bad stack frame 0x%08x\n", p);
                    goto done_err;
                }
                curr_switch = (DWORD)frame16.frame32;
                curr_mode = stm_32bit;
                if (!sw_read_mem(cb, curr_switch, &ch, sizeof(ch)))
                    curr_switch = 0;
            }
            else
            {
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(next_switch);
                tmp.Offset  = OFFSETOF(next_switch);
                p = sw_xlat_addr(cb, &tmp);

                if (!sw_read_mem(cb, p, &frame16, sizeof(frame16)))
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
                if (!sw_read_mem(cb, next_switch, &frame32, sizeof(frame32)))
                {
                    WARN("Bad stack frame 0x%08x\n", next_switch);
                    goto done_err;
                }
                curr_switch = (DWORD)frame32.frame16;
                tmp.Mode    = AddrMode1616;
                tmp.Segment = SELECTOROF(curr_switch);
                tmp.Offset  = OFFSETOF(curr_switch);

                if (!sw_read_mem(cb, sw_xlat_addr(cb, &tmp), &ch, sizeof(ch)))
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
                if (!sw_read_mem(cb, sw_xlat_addr(cb, &frame->AddrFrame),
                                 &val, sizeof(WORD)))
                    goto done_err;
                frame->AddrFrame.Offset = val;
            }
            else
            {
                frame->AddrStack.Offset = frame->AddrFrame.Offset + 2 * sizeof(DWORD);
                /* "pop up" previous EBP value */
                if (!sw_read_mem(cb, frame->AddrFrame.Offset, 
                                 &frame->AddrFrame.Offset, sizeof(DWORD)))
                    goto done_err;
            }
        }
    }

    if (curr_mode == stm_16bit)
    {
        unsigned int     i;

        p = sw_xlat_addr(cb, &frame->AddrFrame);
        if (!sw_read_mem(cb, p + sizeof(WORD), &val, sizeof(WORD)))
            goto done_err;
        frame->AddrReturn.Offset = val;
        /* get potential cs if a far call was used */
        if (!sw_read_mem(cb, p + 2 * sizeof(WORD), &val, sizeof(WORD)))
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

                if (GetThreadSelectorEntry(cb->hThread, val, &le) &&
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
            sw_read_mem(cb, p + (2 + i) * sizeof(WORD), &val, sizeof(val));
            frame->Params[i] = val;
        }
    }
    else
    {
        if (!sw_read_mem(cb, frame->AddrFrame.Offset + sizeof(DWORD),
                         &frame->AddrReturn.Offset, sizeof(DWORD)))
        {
            WARN("Cannot read new frame offset %08x\n", frame->AddrFrame.Offset + (int)sizeof(DWORD));
            goto done_err;
        }
        sw_read_mem(cb, frame->AddrFrame.Offset + 2 * sizeof(DWORD),
                    frame->Params, sizeof(frame->Params));
    }

    frame->Far = TRUE;
    frame->Virtual = TRUE;
    p = sw_xlat_addr(cb, &frame->AddrPC);
    if (p && sw_modl_bas(cb, p))
        frame->FuncTableEntry = sw_tabl_acs(cb, p);
    else
        frame->FuncTableEntry = NULL;

    TRACE("Leave: PC=%s Frame=%s Return=%s Stack=%s Mode=%s cSwitch=%08x nSwitch=%08x FuncTable=%p\n",
          wine_dbgstr_addr(&frame->AddrPC),
          wine_dbgstr_addr(&frame->AddrFrame),
          wine_dbgstr_addr(&frame->AddrReturn),
          wine_dbgstr_addr(&frame->AddrStack), 
          curr_mode == stm_start ? "start" : (curr_mode == stm_16bit ? "16bit" : "32bit"),
          curr_switch, next_switch, frame->FuncTableEntry);

    return TRUE;
done_err:
    curr_mode = stm_done;
    return FALSE;
}

/***********************************************************************
 *		StackWalk (DBGHELP.@)
 */
BOOL WINAPI StackWalk(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                      LPSTACKFRAME frame, PVOID ctx,
                      PREAD_PROCESS_MEMORY_ROUTINE f_read_mem,
                      PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                      PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                      PTRANSLATE_ADDRESS_ROUTINE f_xlat_adr)
{
    struct stack_walk_callback  swcb;

    TRACE("(%d, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (MachineType != IMAGE_FILE_MACHINE_I386)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    swcb.hProcess = hProcess;
    swcb.hThread = hThread;
    swcb.is32 = TRUE;
    /* sigh... MS isn't even consistent in the func prototypes */
    swcb.u.s32.f_read_mem = (f_read_mem) ? f_read_mem : read_mem;
    swcb.u.s32.f_xlat_adr = (f_xlat_adr) ? f_xlat_adr : addr_to_linear;
    swcb.u.s32.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess;
    swcb.u.s32.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase;

    return stack_walk(&swcb, frame);
}


/***********************************************************************
 *		StackWalk64 (DBGHELP.@)
 */
BOOL WINAPI StackWalk64(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                        LPSTACKFRAME64 frame64, PVOID ctx,
                        PREAD_PROCESS_MEMORY_ROUTINE64 f_read_mem,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                        PTRANSLATE_ADDRESS_ROUTINE64 f_xlat_adr)
{
    struct stack_walk_callback  swcb;
    STACKFRAME                  frame32;
    BOOL                        ret;

    TRACE("(%d, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame64, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (MachineType != IMAGE_FILE_MACHINE_I386)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    addr_64to32(&frame64->AddrPC,     &frame32.AddrPC);
    addr_64to32(&frame64->AddrReturn, &frame32.AddrReturn);
    addr_64to32(&frame64->AddrFrame,  &frame32.AddrFrame);
    addr_64to32(&frame64->AddrStack,  &frame32.AddrStack);
    addr_64to32(&frame64->AddrBStore, &frame32.AddrBStore);
    frame32.FuncTableEntry = frame64->FuncTableEntry; /* FIXME */
    frame32.Far = frame64->Far;
    frame32.Virtual = frame64->Virtual;
    frame32.Reserved[0] = (ULONG)frame64->Reserved[0];
    frame32.Reserved[1] = (ULONG)frame64->Reserved[1];
    frame32.Reserved[2] = (ULONG)frame64->Reserved[2];
    /* we don't handle KdHelp */

    swcb.hProcess = hProcess;
    swcb.hThread = hThread;
    swcb.is32 = FALSE;
    /* sigh... MS isn't even consistent in the func prototypes */
    swcb.u.s64.f_read_mem = (f_read_mem) ? f_read_mem : read_mem64;
    swcb.u.s64.f_xlat_adr = f_xlat_adr;
    swcb.u.s64.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess64;
    swcb.u.s64.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase64;

    ret = stack_walk(&swcb, &frame32);

    addr_32to64(&frame32.AddrPC,     &frame64->AddrPC);
    addr_32to64(&frame32.AddrReturn, &frame64->AddrReturn);
    addr_32to64(&frame32.AddrFrame,  &frame64->AddrFrame);
    addr_32to64(&frame32.AddrStack,  &frame64->AddrStack);
    addr_32to64(&frame32.AddrBStore, &frame64->AddrBStore);
    frame64->FuncTableEntry = frame32.FuncTableEntry; /* FIXME */
    frame64->Params[0] = frame32.Params[0];
    frame64->Params[1] = frame32.Params[1];
    frame64->Params[2] = frame32.Params[2];
    frame64->Params[3] = frame32.Params[3];
    frame64->Far = frame32.Far;
    frame64->Virtual = frame32.Virtual;
    frame64->Reserved[0] = frame32.Reserved[0];
    frame64->Reserved[1] = frame32.Reserved[1];
    frame64->Reserved[2] = frame32.Reserved[2];
    /* we don't handle KdHelp */
    frame64->KdHelp.Thread = 0xC000FADE;
    frame64->KdHelp.ThCallbackStack = 0x10;
    frame64->KdHelp.ThCallbackBStore = 0;
    frame64->KdHelp.NextCallback = 0;
    frame64->KdHelp.FramePointer = 0;
    frame64->KdHelp.KiCallUserMode = 0xD000DAFE;
    frame64->KdHelp.KeUserCallbackDispatcher = 0xE000F000;
    frame64->KdHelp.SystemRangeStart = 0xC0000000;
    frame64->KdHelp.Reserved[0] /* KiUserExceptionDispatcher */ = 0xE0005000;

    return ret;
}

/******************************************************************
 *		SymRegisterFunctionEntryCallback (DBGHELP.@)
 *
 *
 */
BOOL WINAPI SymRegisterFunctionEntryCallback(HANDLE hProc,
                                             PSYMBOL_FUNCENTRY_CALLBACK cb, PVOID user)
{
    FIXME("(%p %p %p): stub!\n", hProc, cb, user);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************
 *		SymRegisterFunctionEntryCallback64 (DBGHELP.@)
 *
 *
 */
BOOL WINAPI SymRegisterFunctionEntryCallback64(HANDLE hProc,
                                               PSYMBOL_FUNCENTRY_CALLBACK64 cb,
                                               ULONG64 user)
{
    FIXME("(%p %p %s): stub!\n", hProc, cb, wine_dbgstr_longlong(user));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
