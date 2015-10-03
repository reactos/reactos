/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/cpu/registers.c
 * PURPOSE:         Exported functions for manipulating registers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "x86context.h"

/* PRIVATE VARIABLES **********************************************************/

// This structure must by synced with our CPU context
X86CONTEXT IntelRegPtr;

/* PUBLIC FUNCTIONS ***********************************************************/

PVOID
WINAPI
getIntelRegistersPointer(VOID)
{
    /*
     * Sync the Intel Registers x86 Context with our CPU context
     */

    if (IntelRegPtr.ContextFlags & CONTEXT_DEBUG_REGISTERS)
    {
        IntelRegPtr.Dr0 = EmulatorContext.DebugRegisters[FAST486_REG_DR0];
        IntelRegPtr.Dr1 = EmulatorContext.DebugRegisters[FAST486_REG_DR1];
        IntelRegPtr.Dr2 = EmulatorContext.DebugRegisters[FAST486_REG_DR2];
        IntelRegPtr.Dr3 = EmulatorContext.DebugRegisters[FAST486_REG_DR3];
        IntelRegPtr.Dr6 = EmulatorContext.DebugRegisters[FAST486_REG_DR6];
        IntelRegPtr.Dr7 = EmulatorContext.DebugRegisters[FAST486_REG_DR7];
    }

#ifndef FAST486_NO_FPU
    if (IntelRegPtr.ContextFlags & CONTEXT_FLOATING_POINT)
    {
        // IntelRegPtr.FloatSave = ;
        IntelRegPtr.FloatSave.ControlWord   = EmulatorContext.FpuControl.Value;
        IntelRegPtr.FloatSave.StatusWord    = EmulatorContext.FpuStatus.Value;
        // IntelRegPtr.FloatSave.TagWord       = ;
        // IntelRegPtr.FloatSave.ErrorOffset   = ;
        // IntelRegPtr.FloatSave.ErrorSelector = ;
        // IntelRegPtr.FloatSave.DataOffset    = ;
        // IntelRegPtr.FloatSave.DataSelector  = ;
        // IntelRegPtr.FloatSave.RegisterArea  = ; // This is a region of size SIZE_OF_80387_REGISTERS == 80 bytes
        // IntelRegPtr.FloatSave.Cr0NpxState   = ;
    }
#endif

    if (IntelRegPtr.ContextFlags & CONTEXT_SEGMENTS)
    {
        IntelRegPtr.SegGs = EmulatorContext.SegmentRegs[FAST486_REG_GS].Selector;
        IntelRegPtr.SegFs = EmulatorContext.SegmentRegs[FAST486_REG_FS].Selector;
        IntelRegPtr.SegEs = EmulatorContext.SegmentRegs[FAST486_REG_ES].Selector;
        IntelRegPtr.SegDs = EmulatorContext.SegmentRegs[FAST486_REG_DS].Selector;
    }

    if (IntelRegPtr.ContextFlags & CONTEXT_INTEGER)
    {
        IntelRegPtr.Edi = EmulatorContext.GeneralRegs[FAST486_REG_EDI].Long;
        IntelRegPtr.Esi = EmulatorContext.GeneralRegs[FAST486_REG_ESI].Long;
        IntelRegPtr.Ebx = EmulatorContext.GeneralRegs[FAST486_REG_EBX].Long;
        IntelRegPtr.Edx = EmulatorContext.GeneralRegs[FAST486_REG_EDX].Long;
        IntelRegPtr.Ecx = EmulatorContext.GeneralRegs[FAST486_REG_ECX].Long;
        IntelRegPtr.Eax = EmulatorContext.GeneralRegs[FAST486_REG_EAX].Long;
    }

    if (IntelRegPtr.ContextFlags & CONTEXT_CONTROL)
    {
        IntelRegPtr.Ebp     = EmulatorContext.GeneralRegs[FAST486_REG_EBP].Long;
        IntelRegPtr.Eip     = EmulatorContext.InstPtr.Long;
        IntelRegPtr.SegCs   = EmulatorContext.SegmentRegs[FAST486_REG_CS].Selector;
        IntelRegPtr.EFlags  = EmulatorContext.Flags.Long;
        IntelRegPtr.Esp     = EmulatorContext.GeneralRegs[FAST486_REG_ESP].Long;
        IntelRegPtr.SegSs   = EmulatorContext.SegmentRegs[FAST486_REG_SS].Selector;
    }

    if (IntelRegPtr.ContextFlags & CONTEXT_EXTENDED_REGISTERS)
    {
        // IntelRegPtr.ExtendedRegisters = ;
    }

    /* Return the address of the Intel Registers x86 Context */
    return &IntelRegPtr;
}

ULONG
WINAPI
getEAX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EAX].Long;
}

VOID
WINAPI
setEAX(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EAX].Long = Value;
}

USHORT
WINAPI
getAX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EAX].LowWord;
}

VOID
WINAPI
setAX(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EAX].LowWord = Value;
}

UCHAR
WINAPI
getAH(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EAX].HighByte;
}

VOID
WINAPI
setAH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EAX].HighByte = Value;
}

UCHAR
WINAPI
getAL(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EAX].LowByte;
}

VOID
WINAPI
setAL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EAX].LowByte = Value;
}

ULONG
WINAPI
getEBX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBX].Long;
}

VOID
WINAPI
setEBX(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBX].Long = Value;
}

USHORT
WINAPI
getBX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBX].LowWord;
}

VOID
WINAPI
setBX(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBX].LowWord = Value;
}

UCHAR
WINAPI
getBH(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBX].HighByte;
}

VOID
WINAPI
setBH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBX].HighByte = Value;
}

UCHAR
WINAPI
getBL(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBX].LowByte;
}

VOID
WINAPI
setBL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBX].LowByte = Value;
}



ULONG
WINAPI
getECX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ECX].Long;
}

VOID
WINAPI
setECX(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ECX].Long = Value;
}

USHORT
WINAPI
getCX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ECX].LowWord;
}

VOID
WINAPI
setCX(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ECX].LowWord = Value;
}

UCHAR
WINAPI
getCH(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ECX].HighByte;
}

VOID
WINAPI
setCH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ECX].HighByte = Value;
}

UCHAR
WINAPI
getCL(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ECX].LowByte;
}

VOID
WINAPI
setCL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ECX].LowByte = Value;
}



ULONG
WINAPI
getEDX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDX].Long;
}

VOID
WINAPI
setEDX(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDX].Long = Value;
}

USHORT
WINAPI
getDX(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDX].LowWord;
}

VOID
WINAPI
setDX(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDX].LowWord = Value;
}

UCHAR
WINAPI
getDH(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDX].HighByte;
}

VOID
WINAPI
setDH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDX].HighByte = Value;
}

UCHAR
WINAPI
getDL(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDX].LowByte;
}

VOID
WINAPI
setDL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDX].LowByte = Value;
}



ULONG
WINAPI
getESP(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ESP].Long;
}

VOID
WINAPI
setESP(ULONG Value)
{
    Fast486SetStack(&EmulatorContext, getSS(), Value);
}

USHORT
WINAPI
getSP(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ESP].LowWord;
}

VOID
WINAPI
setSP(USHORT Value)
{
    Fast486SetStack(&EmulatorContext, getSS(), Value);
}



ULONG
WINAPI
getEBP(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBP].Long;
}

VOID
WINAPI
setEBP(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBP].Long = Value;
}

USHORT
WINAPI
getBP(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EBP].LowWord;
}

VOID
WINAPI
setBP(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EBP].LowWord = Value;
}



ULONG
WINAPI
getESI(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ESI].Long;
}

VOID
WINAPI
setESI(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ESI].Long = Value;
}

USHORT
WINAPI
getSI(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_ESI].LowWord;
}

VOID
WINAPI
setSI(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_ESI].LowWord = Value;
}



ULONG
WINAPI
getEDI(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDI].Long;
}

VOID
WINAPI
setEDI(ULONG Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDI].Long = Value;
}

USHORT
WINAPI
getDI(VOID)
{
    return EmulatorContext.GeneralRegs[FAST486_REG_EDI].LowWord;
}

VOID
WINAPI
setDI(USHORT Value)
{
    EmulatorContext.GeneralRegs[FAST486_REG_EDI].LowWord = Value;
}



ULONG
WINAPI
getEIP(VOID)
{
    return EmulatorContext.InstPtr.Long;
}

VOID
WINAPI
setEIP(ULONG Value)
{
    CpuExecute(getCS(), Value);
}

USHORT
WINAPI
getIP(VOID)
{
    return EmulatorContext.InstPtr.LowWord;
}

VOID
WINAPI
setIP(USHORT Value)
{
    CpuExecute(getCS(), Value);
}



USHORT
WINAPI
getCS(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_CS].Selector;
}

VOID
WINAPI
setCS(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_CS, Value);
}

USHORT
WINAPI
getSS(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_SS].Selector;
}

VOID
WINAPI
setSS(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_SS, Value);
}

USHORT
WINAPI
getDS(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_DS].Selector;
}

VOID
WINAPI
setDS(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_DS, Value);
}

USHORT
WINAPI
getES(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_ES].Selector;
}

VOID
WINAPI
setES(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_ES, Value);
}

USHORT
WINAPI
getFS(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_FS].Selector;
}

VOID
WINAPI
setFS(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_FS, Value);
}

USHORT
WINAPI
getGS(VOID)
{
    return EmulatorContext.SegmentRegs[FAST486_REG_GS].Selector;
}

VOID
WINAPI
setGS(USHORT Value)
{
    Fast486SetSegment(&EmulatorContext, FAST486_REG_GS, Value);
}



ULONG
WINAPI
getCF(VOID)
{
    return EmulatorContext.Flags.Cf;
}

VOID
WINAPI
setCF(ULONG Flag)
{
    EmulatorContext.Flags.Cf = !!(Flag & 1);
}

ULONG
WINAPI
getPF(VOID)
{
    return EmulatorContext.Flags.Pf;
}

VOID
WINAPI
setPF(ULONG Flag)
{
    EmulatorContext.Flags.Pf = !!(Flag & 1);
}

ULONG
WINAPI
getAF(VOID)
{
    return EmulatorContext.Flags.Af;
}

VOID
WINAPI
setAF(ULONG Flag)
{
    EmulatorContext.Flags.Af = !!(Flag & 1);
}

ULONG
WINAPI
getZF(VOID)
{
    return EmulatorContext.Flags.Zf;
}

VOID
WINAPI
setZF(ULONG Flag)
{
    EmulatorContext.Flags.Zf = !!(Flag & 1);
}

ULONG
WINAPI
getSF(VOID)
{
    return EmulatorContext.Flags.Sf;
}

VOID
WINAPI
setSF(ULONG Flag)
{
    EmulatorContext.Flags.Sf = !!(Flag & 1);
}

ULONG
WINAPI
getIF(VOID)
{
    return EmulatorContext.Flags.If;
}

VOID
WINAPI
setIF(ULONG Flag)
{
    EmulatorContext.Flags.If = !!(Flag & 1);
}

ULONG
WINAPI
getDF(VOID)
{
    return EmulatorContext.Flags.Df;
}

VOID
WINAPI
setDF(ULONG Flag)
{
    EmulatorContext.Flags.Df = !!(Flag & 1);
}

ULONG
WINAPI
getOF(VOID)
{
    return EmulatorContext.Flags.Of;
}

VOID
WINAPI
setOF(ULONG Flag)
{
    EmulatorContext.Flags.Of = !!(Flag & 1);
}



ULONG
WINAPI
getEFLAGS(VOID)
{
    return EmulatorContext.Flags.Long;
}

VOID
WINAPI
setEFLAGS(ULONG Flags)
{
    EmulatorContext.Flags.Long = Flags;
}



USHORT
WINAPI
getMSW(VOID)
{
    return LOWORD(EmulatorContext.ControlRegisters[FAST486_REG_CR0]);
}

VOID
WINAPI
setMSW(USHORT Value)
{
    /* Set the lower 16 bits (Machine Status Word) of CR0 */
    EmulatorContext.ControlRegisters[FAST486_REG_CR0] &= 0xFFFF0000;
    EmulatorContext.ControlRegisters[FAST486_REG_CR0] |= Value & 0xFFFF;
}

/* EOF */
