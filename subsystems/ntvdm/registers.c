/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            registers.c
 * PURPOSE:         Exported functions for manipulating registers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "registers.h"

/* PUBLIC FUNCTIONS ***********************************************************/

static inline BOOLEAN EmulatorGetFlag(ULONG Flag)
{
    return (EmulatorContext.Flags.Long & Flag) ? TRUE : FALSE;
}

static inline VOID EmulatorSetFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long |= Flag;
}

static inline VOID EmulatorClearFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long &= ~Flag;
}

VOID EmulatorSetStack(WORD Segment, DWORD Offset)
{
    Fast486SetStack(&EmulatorContext, Segment, Offset);
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
    EmulatorSetStack(getSS(), Value);
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
    EmulatorSetStack(getSS(), Value);
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
    EmulatorExecute(getCS(), Value);
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
    EmulatorExecute(getCS(), Value);
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
    return EmulatorGetFlag(EMULATOR_FLAG_CF);
}

VOID
WINAPI
setCF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_CF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_CF);
}

ULONG
WINAPI
getPF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_PF);
}

VOID
WINAPI
setPF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_PF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_PF);
}

ULONG
WINAPI
getAF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_AF);
}

VOID
WINAPI
setAF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_AF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_AF);
}

ULONG
WINAPI
getZF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_ZF);
}

VOID
WINAPI
setZF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_ZF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_ZF);
}

ULONG
WINAPI
getSF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_SF);
}

VOID
WINAPI
setSF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_SF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_SF);
}

ULONG
WINAPI
getIF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_IF);
}

VOID
WINAPI
setIF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_IF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_IF);
}

ULONG
WINAPI
getDF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_DF);
}

VOID
WINAPI
setDF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_DF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_DF);
}

ULONG
WINAPI
getOF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_OF);
}

VOID
WINAPI
setOF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_OF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_OF);
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
