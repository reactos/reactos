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

/* PUBLIC FUNCTIONS ***********************************************************/

ULONG
CDECL
getEAX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].Long;
}

VOID
CDECL
setEAX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].Long = Value;
}

USHORT
CDECL
getAX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowWord;
}

VOID
CDECL
setAX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowWord = Value;
}

UCHAR
CDECL
getAH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].HighByte;
}

VOID
CDECL
setAH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].HighByte = Value;
}

UCHAR
CDECL
getAL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowByte;
}

VOID
CDECL
setAL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowByte = Value;
}

ULONG
CDECL
getEBX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].Long;
}

VOID
CDECL
setEBX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].Long = Value;
}

USHORT
CDECL
getBX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowWord;
}

VOID
CDECL
setBX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowWord = Value;
}

UCHAR
CDECL
getBH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].HighByte;
}

VOID
CDECL
setBH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].HighByte = Value;
}

UCHAR
CDECL
getBL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowByte;
}

VOID
CDECL
setBL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowByte = Value;
}



ULONG
CDECL
getECX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].Long;
}

VOID
CDECL
setECX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].Long = Value;
}

USHORT
CDECL
getCX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowWord;
}

VOID
CDECL
setCX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowWord = Value;
}

UCHAR
CDECL
getCH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].HighByte;
}

VOID
CDECL
setCH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].HighByte = Value;
}

UCHAR
CDECL
getCL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowByte;
}

VOID
CDECL
setCL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowByte = Value;
}



ULONG
CDECL
getEDX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].Long;
}

VOID
CDECL
setEDX(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].Long = Value;
}

USHORT
CDECL
getDX(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowWord;
}

VOID
CDECL
setDX(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowWord = Value;
}

UCHAR
CDECL
getDH(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].HighByte;
}

VOID
CDECL
setDH(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].HighByte = Value;
}

UCHAR
CDECL
getDL(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowByte;
}

VOID
CDECL
setDL(UCHAR Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowByte = Value;
}



ULONG
CDECL
getESP(VOID)
{
    return EmulatorGetRegister(EMULATOR_REG_SP);
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
    return LOWORD(EmulatorGetRegister(EMULATOR_REG_SP));
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
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BP].Long;
}

VOID
CDECL
setEBP(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BP].Long = Value;
}

USHORT
CDECL
getBP(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BP].LowWord;
}

VOID
CDECL
setBP(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_BP].LowWord = Value;
}



ULONG
CDECL
getESI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_SI].Long;
}

VOID
CDECL
setESI(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_SI].Long = Value;
}

USHORT
CDECL
getSI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_SI].LowWord;
}

VOID
CDECL
setSI(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_SI].LowWord = Value;
}



ULONG
CDECL
getEDI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DI].Long;
}

VOID
CDECL
setEDI(ULONG Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DI].Long = Value;
}

USHORT
CDECL
getDI(VOID)
{
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DI].LowWord;
}

VOID
CDECL
setDI(USHORT Value)
{
    EmulatorContext.GeneralRegs[EMULATOR_REG_DI].LowWord = Value;
}



ULONG
CDECL
getEIP(VOID)
{
    return EmulatorGetProgramCounter();
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
    return LOWORD(EmulatorGetProgramCounter());
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
