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
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_AX].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].Long;
#endif
}

VOID
CDECL
setEAX(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_AX].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].Long = Value;
#endif
}

USHORT
CDECL
getAX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_AX].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowWord;
#endif
}

VOID
CDECL
setAX(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_AX].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowWord = Value;
#endif
}

UCHAR
CDECL
getAH(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_AX].b.hi;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].HighByte;
#endif
}

VOID
CDECL
setAH(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_AX].b.hi = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].HighByte = Value;
#endif
}

UCHAR
CDECL
getAL(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_AX].b.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowByte;
#endif
}

VOID
CDECL
setAL(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_AX].b.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_AX].LowByte = Value;
#endif
}

ULONG
CDECL
getEBX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BX].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].Long;
#endif
}

VOID
CDECL
setEBX(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BX].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].Long = Value;
#endif
}

USHORT
CDECL
getBX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BX].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowWord;
#endif
}

VOID
CDECL
setBX(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BX].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowWord = Value;
#endif
}

UCHAR
CDECL
getBH(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BX].b.hi;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].HighByte;
#endif
}

VOID
CDECL
setBH(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BX].b.hi = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].HighByte = Value;
#endif
}

UCHAR
CDECL
getBL(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BX].b.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowByte;
#endif
}

VOID
CDECL
setBL(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BX].b.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BX].LowByte = Value;
#endif
}



ULONG
CDECL
getECX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_CX].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].Long;
#endif
}

VOID
CDECL
setECX(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_CX].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].Long = Value;
#endif
}

USHORT
CDECL
getCX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_CX].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowWord;
#endif
}

VOID
CDECL
setCX(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_CX].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowWord = Value;
#endif
}

UCHAR
CDECL
getCH(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_CX].b.hi;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].HighByte;
#endif
}

VOID
CDECL
setCH(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_CX].b.hi = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].HighByte = Value;
#endif
}

UCHAR
CDECL
getCL(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_CX].b.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowByte;
#endif
}

VOID
CDECL
setCL(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_CX].b.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_CX].LowByte = Value;
#endif
}



ULONG
CDECL
getEDX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DX].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].Long;
#endif
}

VOID
CDECL
setEDX(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DX].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].Long = Value;
#endif
}

USHORT
CDECL
getDX(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DX].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowWord;
#endif
}

VOID
CDECL
setDX(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DX].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowWord = Value;
#endif
}

UCHAR
CDECL
getDH(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DX].b.hi;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].HighByte;
#endif
}

VOID
CDECL
setDH(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DX].b.hi = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].HighByte = Value;
#endif
}

UCHAR
CDECL
getDL(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DX].b.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowByte;
#endif
}

VOID
CDECL
setDL(UCHAR Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DX].b.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DX].LowByte = Value;
#endif
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
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BP].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BP].Long;
#endif
}

VOID
CDECL
setEBP(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BP].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BP].Long = Value;
#endif
}

USHORT
CDECL
getBP(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_BP].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_BP].LowWord;
#endif
}

VOID
CDECL
setBP(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_BP].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_BP].LowWord = Value;
#endif
}



ULONG
CDECL
getESI(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_SI].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_SI].Long;
#endif
}

VOID
CDECL
setESI(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_SI].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_SI].Long = Value;
#endif
}

USHORT
CDECL
getSI(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_SI].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_SI].LowWord;
#endif
}

VOID
CDECL
setSI(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_SI].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_SI].LowWord = Value;
#endif
}



ULONG
CDECL
getEDI(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DI].val;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DI].Long;
#endif
}

VOID
CDECL
setEDI(ULONG Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DI].val = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DI].Long = Value;
#endif
}

USHORT
CDECL
getDI(VOID)
{
#ifndef NEW_EMULATOR
    return EmulatorContext.state->general_reg[EMULATOR_REG_DI].w.lo;
#else
    return EmulatorContext.GeneralRegs[EMULATOR_REG_DI].LowWord;
#endif
}

VOID
CDECL
setDI(USHORT Value)
{
#ifndef NEW_EMULATOR
    EmulatorContext.state->general_reg[EMULATOR_REG_DI].w.lo = Value;
#else
    EmulatorContext.GeneralRegs[EMULATOR_REG_DI].LowWord = Value;
#endif
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
