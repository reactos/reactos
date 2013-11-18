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

// TODO: Should be moved to fast486.h
enum
{
    EMULATOR_REG_EAX,
    EMULATOR_REG_ECX,
    EMULATOR_REG_EDX,
    EMULATOR_REG_EBX,
    EMULATOR_REG_ESP,
    EMULATOR_REG_EBP,
    EMULATOR_REG_ESI,
    EMULATOR_REG_EDI,

    EMULATOR_REG_ES,
    EMULATOR_REG_CS,
    EMULATOR_REG_SS,
    EMULATOR_REG_DS,
    EMULATOR_REG_FS,
    EMULATOR_REG_GS
};

/* PUBLIC FUNCTIONS ***********************************************************/

ULONG EmulatorGetRegister(ULONG Register)
{
    if (Register < EMULATOR_REG_ES)
    {
        return EmulatorContext.GeneralRegs[Register].Long;
    }
    else
    {
        return EmulatorContext.SegmentRegs[Register - EMULATOR_REG_ES].Selector;
    }
}

VOID EmulatorSetRegister(ULONG Register, ULONG Value)
{
    if (Register < EMULATOR_REG_ES)
    {
        EmulatorContext.GeneralRegs[Register].Long = Value;
    }
    else
    {
        Fast486SetSegment(&EmulatorContext, Register - EMULATOR_REG_ES, (USHORT)Value);
    }
}

BOOLEAN EmulatorGetFlag(ULONG Flag)
{
    return (EmulatorContext.Flags.Long & Flag) ? TRUE : FALSE;
}

VOID EmulatorSetFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long |= Flag;
}

VOID EmulatorClearFlag(ULONG Flag)
{
    EmulatorContext.Flags.Long &= ~Flag;
}

VOID EmulatorSetStack(WORD Segment, DWORD Offset)
{
    Fast486SetStack(&EmulatorContext, Segment, Offset);
}



ULONG
CDECL
getEAX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].Long;
}

VOID
CDECL
setEAX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].Long = Value;
}

USHORT
CDECL
getAX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].LowWord;
}

VOID
CDECL
setAX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].LowWord = Value;
}

UCHAR
CDECL
getAH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].HighByte;
}

VOID
CDECL
setAH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].HighByte = Value;
}

UCHAR
CDECL
getAL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].LowByte;
}

VOID
CDECL
setAL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EAX].LowByte = Value;
}

ULONG
CDECL
getEBX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].Long;
}

VOID
CDECL
setEBX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].Long = Value;
}

USHORT
CDECL
getBX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].LowWord;
}

VOID
CDECL
setBX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].LowWord = Value;
}

UCHAR
CDECL
getBH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].HighByte;
}

VOID
CDECL
setBH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].HighByte = Value;
}

UCHAR
CDECL
getBL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].LowByte;
}

VOID
CDECL
setBL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBX].LowByte = Value;
}



ULONG
CDECL
getECX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].Long;
}

VOID
CDECL
setECX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].Long = Value;
}

USHORT
CDECL
getCX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].LowWord;
}

VOID
CDECL
setCX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].LowWord = Value;
}

UCHAR
CDECL
getCH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].HighByte;
}

VOID
CDECL
setCH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].HighByte = Value;
}

UCHAR
CDECL
getCL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].LowByte;
}

VOID
CDECL
setCL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ECX].LowByte = Value;
}



ULONG
CDECL
getEDX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].Long;
}

VOID
CDECL
setEDX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].Long = Value;
}

USHORT
CDECL
getDX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].LowWord;
}

VOID
CDECL
setDX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].LowWord = Value;
}

UCHAR
CDECL
getDH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].HighByte;
}

VOID
CDECL
setDH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].HighByte = Value;
}

UCHAR
CDECL
getDL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].LowByte;
}

VOID
CDECL
setDL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDX].LowByte = Value;
}



ULONG
CDECL
getESP(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_ESP);
}

VOID
CDECL
setESP(ULONG Value)
{
    EmulatorSetStack(EmulatorGetRegister(EMULATOR_REG_SS), Value);
}

USHORT
CDECL
getSP(VOID)
{
    return LOWORD(EmulatorGetRegister(EMULATOR_REG_ESP));
}

VOID
CDECL
setSP(USHORT Value)
{
    EmulatorSetStack(EmulatorGetRegister(EMULATOR_REG_SS), Value);
}



ULONG
CDECL
getEBP(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBP].Long;
}

VOID
CDECL
setEBP(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBP].Long = Value;
}

USHORT
CDECL
getBP(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EBP].LowWord;
}

VOID
CDECL
setBP(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EBP].LowWord = Value;
}



ULONG
CDECL
getESI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ESI].Long;
}

VOID
CDECL
setESI(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ESI].Long = Value;
}

USHORT
CDECL
getSI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_ESI].LowWord;
}

VOID
CDECL
setSI(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_ESI].LowWord = Value;
}



ULONG
CDECL
getEDI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDI].Long;
}

VOID
CDECL
setEDI(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDI].Long = Value;
}

USHORT
CDECL
getDI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_EDI].LowWord;
}

VOID
CDECL
setDI(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_EDI].LowWord = Value;
}



ULONG
CDECL
getEIP(VOID)
{
    return EmulatorContext.InstPtr.Long;
}

VOID
CDECL
setEIP(ULONG Value)
{
    EmulatorExecute(EmulatorGetRegister(EMULATOR_REG_CS), Value);
}

USHORT
CDECL
getIP(VOID)
{
    return EmulatorContext.InstPtr.LowWord;
}

VOID
CDECL
setIP(USHORT Value)
{
    EmulatorExecute(EmulatorGetRegister(EMULATOR_REG_CS), Value);
}



USHORT
CDECL
getCS(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_CS);
}

VOID
CDECL
setCS(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_CS, Value);
}

USHORT
CDECL
getSS(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_SS);
}

VOID
CDECL
setSS(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_SS, Value);
}

USHORT
CDECL
getDS(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_DS);
}

VOID
CDECL
setDS(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_DS, Value);
}

USHORT
CDECL
getES(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_ES);
}

VOID
CDECL
setES(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_ES, Value);
}

USHORT
CDECL
getFS(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_FS);
}

VOID
CDECL
setFS(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_FS, Value);
}

USHORT
CDECL
getGS(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_GS);
}

VOID
CDECL
setGS(USHORT Value)
{
    EmulatorSetRegister(EMULATOR_REG_GS, Value);
}



ULONG
CDECL
getCF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_CF);
}

VOID
CDECL
setCF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_CF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_CF);
}

ULONG
CDECL
getPF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_PF);
}

VOID
CDECL
setPF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_PF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_PF);
}

ULONG
CDECL
getAF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_AF);
}

VOID
CDECL
setAF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_AF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_AF);
}

ULONG
CDECL
getZF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_ZF);
}

VOID
CDECL
setZF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_ZF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_ZF);
}

ULONG
CDECL
getSF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_SF);
}

VOID
CDECL
setSF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_SF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_SF);
}

ULONG
CDECL
getIF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_IF);
}

VOID
CDECL
setIF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_IF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_IF);
}

ULONG
CDECL
getDF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_DF);
}

VOID
CDECL
setDF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_DF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_DF);
}

ULONG
CDECL
getOF(VOID)
{
    return EmulatorGetFlag(EMULATOR_FLAG_OF);
}

VOID
CDECL
setOF(ULONG Flag)
{
    if (Flag & 1)
        EmulatorSetFlag(EMULATOR_FLAG_OF);
    else
        EmulatorClearFlag(EMULATOR_FLAG_OF);
}



ULONG
CDECL
getEFLAGS(VOID)
{
    return EmulatorContext.Flags.Long;
}

VOID
CDECL
setEFLAGS(ULONG Flags)
{
    EmulatorContext.Flags.Long = Flags;
}



USHORT
CDECL
getMSW(VOID)
{
    return 0; // UNIMPLEMENTED
}

VOID
CDECL
setMSW(USHORT Value)
{
    // UNIMPLEMENTED
}

/* EOF */
