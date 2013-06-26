/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.h
 * PURPOSE:         Minimal x86 machine emulator for the VDM (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _EMULATOR_H_
#define _EMULATOR_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"
#include <softx86/softx86.h>
#include <softx86/softx87.h>

/* DEFINES ********************************************************************/

#define EMULATOR_FLAG_CF (1 << 0)
#define EMULATOR_FLAG_PF (1 << 2)
#define EMULATOR_FLAG_AF (1 << 4)
#define EMULATOR_FLAG_ZF (1 << 6)
#define EMULATOR_FLAG_SF (1 << 7)
#define EMULATOR_FLAG_TF (1 << 8)
#define EMULATOR_FLAG_IF (1 << 9)
#define EMULATOR_FLAG_DF (1 << 10)
#define EMULATOR_FLAG_OF (1 << 11)
#define EMULATOR_FLAG_NT (1 << 14)
#define EMULATOR_FLAG_RF (1 << 16)
#define EMULATOR_FLAG_VM (1 << 17)
#define EMULATOR_FLAG_AC (1 << 18)
#define EMULATOR_FLAG_VIF (1 << 19)
#define EMULATOR_FLAG_VIP (1 << 20)
#define EMULATOR_FLAG_ID (1 << 21)
#define SPECIAL_INT_NUM 0xFF

enum
{
    EMULATOR_EXCEPTION_DIVISION_BY_ZERO,
    EMULATOR_EXCEPTION_DEBUG,
    EMULATOR_EXCEPTION_NMI,
    EMULATOR_EXCEPTION_BREAKPOINT,
    EMULATOR_EXCEPTION_OVERFLOW,
    EMULATOR_EXCEPTION_BOUND,
    EMULATOR_EXCEPTION_INVALID_OPCODE,
    EMULATOR_EXCEPTION_NO_FPU
};

typedef enum
{
    EMULATOR_REG_AX,
    EMULATOR_REG_CX,
    EMULATOR_REG_DX,
    EMULATOR_REG_BX,
    EMULATOR_REG_SI,
    EMULATOR_REG_DI,
    EMULATOR_REG_SP,
    EMULATOR_REG_BP,
    EMULATOR_REG_ES,
    EMULATOR_REG_CS,
    EMULATOR_REG_SS,
    EMULATOR_REG_DS,
} EMULATOR_REGISTER;

/* FUNCTIONS ******************************************************************/

BOOLEAN EmulatorInitialize();
VOID EmulatorSetStack(WORD Segment, WORD Offset);
VOID EmulatorExecute(WORD Segment, WORD Offset);
VOID EmulatorInterrupt(BYTE Number);
VOID EmulatorExternalInterrupt(BYTE Number);
ULONG EmulatorGetRegister(ULONG Register);
VOID EmulatorSetRegister(ULONG Register, ULONG Value);
BOOLEAN EmulatorGetFlag(ULONG Flag);
VOID EmulatorSetFlag(ULONG Flag);
VOID EmulatorClearFlag(ULONG Flag);
VOID EmulatorStep();
VOID EmulatorCleanup();
VOID EmulatorSetA20(BOOLEAN Enabled);

#endif

/* EOF */

