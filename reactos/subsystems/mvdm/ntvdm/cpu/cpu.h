/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.h
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _CPU_H_
#define _CPU_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"
#include <fast486.h>

/* DEFINES ********************************************************************/

/* FLAGS */
#define EMULATOR_FLAG_CF    (1 << 0)
#define EMULATOR_FLAG_PF    (1 << 2)
#define EMULATOR_FLAG_AF    (1 << 4)
#define EMULATOR_FLAG_ZF    (1 << 6)
#define EMULATOR_FLAG_SF    (1 << 7)
#define EMULATOR_FLAG_TF    (1 << 8)
#define EMULATOR_FLAG_IF    (1 << 9)
#define EMULATOR_FLAG_DF    (1 << 10)
#define EMULATOR_FLAG_OF    (1 << 11)
#define EMULATOR_FLAG_NT    (1 << 14)
#define EMULATOR_FLAG_RF    (1 << 16)
#define EMULATOR_FLAG_VM    (1 << 17)
#define EMULATOR_FLAG_AC    (1 << 18)
#define EMULATOR_FLAG_VIF   (1 << 19)
#define EMULATOR_FLAG_VIP   (1 << 20)
#define EMULATOR_FLAG_ID    (1 << 21)

#if 0
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
#endif

extern BOOLEAN CpuRunning;
extern FAST486_STATE EmulatorContext;

/* FUNCTIONS ******************************************************************/

#if 0
VOID EmulatorException(BYTE ExceptionNumber, LPWORD Stack);
#endif

VOID CpuExecute(WORD Segment, WORD Offset);
VOID CpuStep(VOID);
VOID CpuSimulate(VOID);
VOID CpuUnsimulate(VOID);
#if 0
VOID EmulatorTerminate(VOID);
#endif

BOOLEAN CpuInitialize(VOID);
VOID CpuCleanup(VOID);

#endif // _CPU_H_

/* EOF */
