/*
 * COPYRIGHT:       Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 * LICENSE:         GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdgdb/hwbp_x86.c
 * PURPOSE:         Hardware debug register support for i386 and amd64.
 *
 * x86 has a single pool of four debug registers, each of which can serve any
 * of the GDB hardware breakpoint types, selected by the R/W field of DR7.
 */

#include "kdgdb.h"

#define GDB_DR7_GLOBAL_ENABLE(Slot) (2ULL << ((Slot) * 2))
#define GDB_DR7_CONTROL_SHIFT(Slot) (16 + ((Slot) * 4))
#define GDB_DR7_RESERVED_BIT 0x400ULL

const ULONG gdb_hw_breakpoint_count = 4;

static
ULONG64
hardware_breakpoint_control(
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoint)
{
    ULONG64 Access;
    ULONG64 Length;

    if (Breakpoint->Type == GDB_HW_EXECUTE)
        return 0;

    /* x86 has no read-only encoding, so Z3 uses read/write to avoid missing reads. */
    Access = (Breakpoint->Type == GDB_HW_WRITE) ? 1 : 3;
    switch (Breakpoint->Kind)
    {
        case 1: Length = 0; break;
        case 2: Length = 1; break;
        case 4: Length = 3; break;
        case 8: Length = 2; break;
        default: return ~(ULONG64)0;
    }

    return Access | (Length << 2);
}

BOOLEAN
gdb_arch_hw_breakpoint_valid(
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoint)
{
    if (Breakpoint->Type < GDB_HW_EXECUTE ||
        Breakpoint->Type > GDB_HW_ACCESS)
    {
        return FALSE;
    }

    if (Breakpoint->Type == GDB_HW_EXECUTE)
        return Breakpoint->Kind == 1;

    return hardware_breakpoint_control(Breakpoint) != ~(ULONG64)0 &&
           (Breakpoint->Kind == 1 ||
            (Breakpoint->Address & (Breakpoint->Kind - 1)) == 0);
}

static
ULONG64
hardware_breakpoint_dr7(
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoints)
{
    ULONG64 Dr7 = 0;
    ULONG i;

    for (i = 0; i < gdb_hw_breakpoint_count; i++)
    {
        ULONG64 Control;

        if (!Breakpoints[i].Active)
            continue;

        Control = hardware_breakpoint_control(&Breakpoints[i]);
        Dr7 |= GDB_DR7_GLOBAL_ENABLE(i) | (Control << GDB_DR7_CONTROL_SHIFT(i));
    }

    return Dr7 ? Dr7 | GDB_DR7_RESERVED_BIT : 0;
}

VOID
gdb_arch_program_hw_breakpoints(
    _Inout_ PKSPECIAL_REGISTERS Registers,
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoints)
{
    ULONG_PTR Address[4];
    ULONG Slot;

    for (Slot = 0; Slot < gdb_hw_breakpoint_count; Slot++)
        Address[Slot] = Breakpoints[Slot].Active ? (ULONG_PTR)Breakpoints[Slot].Address : 0;

    Registers->KernelDr0 = Address[0];
    Registers->KernelDr1 = Address[1];
    Registers->KernelDr2 = Address[2];
    Registers->KernelDr3 = Address[3];
    Registers->KernelDr6 = 0;
    Registers->KernelDr7 = (ULONG_PTR)hardware_breakpoint_dr7(Breakpoints);
}

VOID
gdb_arch_report_hw_breakpoints(
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoints)
{
    CurrentStateChange.ControlReport.Dr6 = 0;
    CurrentStateChange.ControlReport.Dr7 = hardware_breakpoint_dr7(Breakpoints);
}

BOOLEAN
gdb_arch_hw_breakpoint_hit(
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoints,
    _In_ ULONG Slot)
{
    UNREFERENCED_PARAMETER(Breakpoints);

    /* DR6 reports the slots that fired as a bitmask */
    return (CurrentStateChange.ControlReport.Dr6 & (1ULL << Slot)) != 0;
}

VOID
gdb_arch_set_continue_control(
    _Inout_ DBGKD_MANIPULATE_STATE64* State,
    _In_ const GDB_HARDWARE_BREAKPOINT* Breakpoints)
{
    State->u.Continue2.ControlSet.TraceFlag = (CurrentContext.EFlags & EFLAGS_TF) != 0;
    State->u.Continue2.ControlSet.Dr7 = hardware_breakpoint_dr7(Breakpoints);
}
