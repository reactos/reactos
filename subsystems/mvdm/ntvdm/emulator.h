/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/emulator.h
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _EMULATOR_H_
#define _EMULATOR_H_

/* INCLUDES *******************************************************************/

#include <fast486.h>

/* DEFINES ********************************************************************/

/*
 * Basic Memory Management
 */
#define NULL32  ((ULONG)0)

#define MEM_ALIGN_DOWN(ptr, align)  (PVOID)((ULONG_PTR)(ptr) & ~((align) - 1l))
#define MEM_ALIGN_UP(ptr, align)    MEM_ALIGN_DOWN((ULONG_PTR)(ptr) + (align) - 1l, (align))

#define TO_LINEAR(seg, off) (((seg) << 4) + (off))
#define MAX_SEGMENT 0xFFFF
#define MAX_OFFSET  0xFFFF
#define MAX_ADDRESS 0x1000000 // 16 MB of RAM; see also: kernel32/client/vdm.c!BaseGetVdmConfigInfo
C_ASSERT(0x100000 <= MAX_ADDRESS);  // A minimum of 1 MB is required for PC emulation.

#define SEG_OFF_TO_PTR(seg, off)    \
    (PVOID)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), (off)))

#define FAR_POINTER(x)      SEG_OFF_TO_PTR(HIWORD(x), LOWORD(x))

#define REAL_TO_PHYS(ptr)   (PVOID)((ULONG_PTR)(ptr) + (ULONG_PTR)BaseAddress)
#define PHYS_TO_REAL(ptr)   (PVOID)((ULONG_PTR)(ptr) - (ULONG_PTR)BaseAddress)

#define ARRAY_INDEX(ptr, array) ((ULONG)(((ULONG_PTR)(ptr) - (ULONG_PTR)(array)) / sizeof(*array)))

/*
 * BCD-Binary conversion
 */

FORCEINLINE
USHORT
BINARY_TO_BCD(USHORT Value)
{
    USHORT Result;

    Result = (Value / 1000) << 12;
    Value %= 1000;
    Result |= (Value / 100) << 8;
    Value %= 100;
    Result |= (Value / 10) << 4;
    Value %= 10;
    Result |= Value;

    return Result;
}

FORCEINLINE
USHORT
BCD_TO_BINARY(USHORT Value)
{
    USHORT Result;

    Result = Value & 0xF;
    Value >>= 4;
    Result += (Value & 0xF) * 10;
    Value >>= 4;
    Result += (Value & 0xF) * 100;
    Value >>= 4;
    Result += Value * 1000;

    return Result;
}


/*
 * Emulator state
 */

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

extern FAST486_STATE EmulatorContext;
extern LPVOID  BaseAddress;
extern BOOLEAN VdmRunning;


/* FUNCTIONS ******************************************************************/

VOID DumpMemory(BOOLEAN TextFormat);

VOID MountFloppy(IN ULONG DiskNumber);
VOID EjectFloppy(IN ULONG DiskNumber);

UCHAR FASTCALL EmulatorIntAcknowledge
(
    PFAST486_STATE State
);

VOID FASTCALL EmulatorFpu
(
    PFAST486_STATE State
);

VOID EmulatorInterruptSignal(VOID);
VOID EmulatorException(BYTE ExceptionNumber, LPWORD Stack);

VOID EmulatorPause(VOID);
VOID EmulatorResume(VOID);
VOID EmulatorTerminate(VOID);

BOOLEAN EmulatorInitialize(HANDLE ConsoleInput, HANDLE ConsoleOutput);
VOID EmulatorCleanup(VOID);

#endif // _EMULATOR_H_

/* EOF */
