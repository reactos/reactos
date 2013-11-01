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
#include <fast486.h>

/* DEFINES ********************************************************************/

/* FLAGS */
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

/* Common definitions */
#define EMULATOR_BOP        0xC4C4
#define EMULATOR_INT_BOP    0xFF

#define STACK_COUNTER   0
#define STACK_INT_NUM   1
#define STACK_IP        2
#define STACK_CS        3
#define STACK_FLAGS     4

enum
{
    EMULATOR_EXCEPTION_DIVISION_BY_ZERO,
    EMULATOR_EXCEPTION_DEBUG,
    EMULATOR_EXCEPTION_NMI,
    EMULATOR_EXCEPTION_BREAKPOINT,
    EMULATOR_EXCEPTION_OVERFLOW,
    EMULATOR_EXCEPTION_BOUND,
    EMULATOR_EXCEPTION_INVALID_OPCODE,
    EMULATOR_EXCEPTION_NO_FPU,
    EMULATOR_EXCEPTION_DOUBLE_FAULT,
    EMULATOR_EXCEPTION_FPU_SEGMENT,
    EMULATOR_EXCEPTION_INVALID_TSS,
    EMULATOR_EXCEPTION_NO_SEGMENT,
    EMULATOR_EXCEPTION_STACK_SEGMENT,
    EMULATOR_EXCEPTION_GPF,
    EMULATOR_EXCEPTION_PAGE_FAULT
};

enum
{
    EMULATOR_REG_AX,
    EMULATOR_REG_CX,
    EMULATOR_REG_DX,
    EMULATOR_REG_BX,
    EMULATOR_REG_SP,
    EMULATOR_REG_BP,
    EMULATOR_REG_SI,
    EMULATOR_REG_DI,
    EMULATOR_REG_ES,
    EMULATOR_REG_CS,
    EMULATOR_REG_SS,
    EMULATOR_REG_DS,
    EMULATOR_REG_FS,
    EMULATOR_REG_GS
};

extern FAST486_STATE EmulatorContext;

/* FUNCTIONS ******************************************************************/

BOOLEAN EmulatorInitialize(VOID);
VOID EmulatorSetStack(WORD Segment, DWORD Offset);
VOID EmulatorExecute(WORD Segment, WORD Offset);
VOID EmulatorInterrupt(BYTE Number);
VOID EmulatorInterruptSignal(VOID);
ULONG EmulatorGetRegister(ULONG Register);
ULONG EmulatorGetProgramCounter(VOID);
VOID EmulatorSetRegister(ULONG Register, ULONG Value);
BOOLEAN EmulatorGetFlag(ULONG Flag);
VOID EmulatorSetFlag(ULONG Flag);
VOID EmulatorClearFlag(ULONG Flag);
VOID EmulatorStep(VOID);
VOID EmulatorCleanup(VOID);
VOID EmulatorSetA20(BOOLEAN Enabled);

#endif // _EMULATOR_H_

/* EOF */
