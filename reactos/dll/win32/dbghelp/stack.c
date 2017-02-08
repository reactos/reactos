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

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static DWORD64 WINAPI addr_to_linear(HANDLE hProcess, HANDLE hThread, ADDRESS64* addr)
{
    LDT_ENTRY	le;

    switch (addr->Mode)
    {
    case AddrMode1616:
        if (GetThreadSelectorEntry(hThread, addr->Segment, &le))
            return (le.HighWord.Bits.BaseHi << 24) +
                (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + LOWORD(addr->Offset);
        break;
    case AddrMode1632:
        if (GetThreadSelectorEntry(hThread, addr->Segment, &le))
            return (le.HighWord.Bits.BaseHi << 24) +
                (le.HighWord.Bits.BaseMid << 16) + le.BaseLow + addr->Offset;
        break;
    case AddrModeReal:
        return (DWORD)(LOWORD(addr->Segment) << 4) + addr->Offset;
    case AddrModeFlat:
        return addr->Offset;
    default:
        FIXME("Unsupported (yet) mode (%x)\n", addr->Mode);
        return 0;
    }
    FIXME("Failed to linearize address %04x:%s (mode %x)\n",
          addr->Segment, wine_dbgstr_longlong(addr->Offset), addr->Mode);
    return 0;
}

static BOOL CALLBACK read_mem(HANDLE hProcess, DWORD addr, void* buffer,
                              DWORD size, LPDWORD nread)
{
    SIZE_T      r;
    if (!ReadProcessMemory(hProcess, (void*)(DWORD_PTR)addr, buffer, size, &r)) return FALSE;
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

BOOL sw_read_mem(struct cpu_stack_walk* csw, DWORD64 addr, void* ptr, DWORD sz)
{
    DWORD bytes_read = 0;
    if (csw->is32)
        return csw->u.s32.f_read_mem(csw->hProcess, addr, ptr, sz, &bytes_read);
    else
        return csw->u.s64.f_read_mem(csw->hProcess, addr, ptr, sz, &bytes_read);
}

DWORD64 sw_xlat_addr(struct cpu_stack_walk* csw, ADDRESS64* addr)
{
    if (addr->Mode == AddrModeFlat) return addr->Offset;
    if (csw->is32)
    {
        ADDRESS         addr32;

        addr_64to32(addr, &addr32);
        return csw->u.s32.f_xlat_adr(csw->hProcess, csw->hThread, &addr32);
    }
    else if (csw->u.s64.f_xlat_adr)
        return csw->u.s64.f_xlat_adr(csw->hProcess, csw->hThread, addr);
    return addr_to_linear(csw->hProcess, csw->hThread, addr);
}

void* sw_table_access(struct cpu_stack_walk* csw, DWORD64 addr)
{
    if (csw->is32)
        return csw->u.s32.f_tabl_acs(csw->hProcess, addr);
    else
        return csw->u.s64.f_tabl_acs(csw->hProcess, addr);
}

DWORD64 sw_module_base(struct cpu_stack_walk* csw, DWORD64 addr)
{
    if (csw->is32)
        return csw->u.s32.f_modl_bas(csw->hProcess, addr);
    else
        return csw->u.s64.f_modl_bas(csw->hProcess, addr);
}

/***********************************************************************
 *		StackWalk (DBGHELP.@)
 */
BOOL WINAPI StackWalk(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                      LPSTACKFRAME frame32, PVOID ctx,
                      PREAD_PROCESS_MEMORY_ROUTINE f_read_mem,
                      PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                      PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                      PTRANSLATE_ADDRESS_ROUTINE f_xlat_adr)
{
    struct cpu_stack_walk       csw;
    STACKFRAME64                frame64;
    BOOL                        ret;
    struct cpu*                 cpu;

    TRACE("(%d, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame32, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (!(cpu = cpu_find(MachineType)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    addr_32to64(&frame32->AddrPC,     &frame64.AddrPC);
    addr_32to64(&frame32->AddrReturn, &frame64.AddrReturn);
    addr_32to64(&frame32->AddrFrame,  &frame64.AddrFrame);
    addr_32to64(&frame32->AddrStack,  &frame64.AddrStack);
    addr_32to64(&frame32->AddrBStore, &frame64.AddrBStore);
    frame64.FuncTableEntry = frame32->FuncTableEntry; /* FIXME */
    frame64.Far = frame32->Far;
    frame64.Virtual = frame32->Virtual;
    frame64.Reserved[0] = frame32->Reserved[0];
    frame64.Reserved[1] = frame32->Reserved[1];
    frame64.Reserved[2] = frame32->Reserved[2];
    /* we don't handle KdHelp */

    csw.hProcess = hProcess;
    csw.hThread = hThread;
    csw.is32 = TRUE;
    /* sigh... MS isn't even consistent in the func prototypes */
    csw.u.s32.f_read_mem = (f_read_mem) ? f_read_mem : read_mem;
    csw.u.s32.f_xlat_adr = f_xlat_adr;
    csw.u.s32.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess;
    csw.u.s32.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase;

    if ((ret = cpu->stack_walk(&csw, &frame64, ctx)))
    {
        addr_64to32(&frame64.AddrPC,     &frame32->AddrPC);
        addr_64to32(&frame64.AddrReturn, &frame32->AddrReturn);
        addr_64to32(&frame64.AddrFrame,  &frame32->AddrFrame);
        addr_64to32(&frame64.AddrStack,  &frame32->AddrStack);
        addr_64to32(&frame64.AddrBStore, &frame32->AddrBStore);
        frame32->FuncTableEntry = frame64.FuncTableEntry; /* FIXME */
        frame32->Params[0] = frame64.Params[0];
        frame32->Params[1] = frame64.Params[1];
        frame32->Params[2] = frame64.Params[2];
        frame32->Params[3] = frame64.Params[3];
        frame32->Far = frame64.Far;
        frame32->Virtual = frame64.Virtual;
        frame32->Reserved[0] = frame64.Reserved[0];
        frame32->Reserved[1] = frame64.Reserved[1];
        frame32->Reserved[2] = frame64.Reserved[2];
    }

    return ret;
}


/***********************************************************************
 *		StackWalk64 (DBGHELP.@)
 */
BOOL WINAPI StackWalk64(DWORD MachineType, HANDLE hProcess, HANDLE hThread,
                        LPSTACKFRAME64 frame, PVOID ctx,
                        PREAD_PROCESS_MEMORY_ROUTINE64 f_read_mem,
                        PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                        PTRANSLATE_ADDRESS_ROUTINE64 f_xlat_adr)
{
    struct cpu_stack_walk       csw;
    struct cpu*                 cpu;

    TRACE("(%d, %p, %p, %p, %p, %p, %p, %p, %p)\n",
          MachineType, hProcess, hThread, frame, ctx,
          f_read_mem, FunctionTableAccessRoutine,
          GetModuleBaseRoutine, f_xlat_adr);

    if (!(cpu = cpu_find(MachineType)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    csw.hProcess = hProcess;
    csw.hThread = hThread;
    csw.is32 = FALSE;
    /* sigh... MS isn't even consistent in the func prototypes */
    csw.u.s64.f_read_mem = (f_read_mem) ? f_read_mem : read_mem64;
    csw.u.s64.f_xlat_adr = (f_xlat_adr) ? f_xlat_adr : addr_to_linear;
    csw.u.s64.f_tabl_acs = (FunctionTableAccessRoutine) ? FunctionTableAccessRoutine : SymFunctionTableAccess64;
    csw.u.s64.f_modl_bas = (GetModuleBaseRoutine) ? GetModuleBaseRoutine : SymGetModuleBase64;

    if (!cpu->stack_walk(&csw, frame, ctx)) return FALSE;

    /* we don't handle KdHelp */

    return TRUE;
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
