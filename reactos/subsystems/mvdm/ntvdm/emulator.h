/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.h
 * PURPOSE:         Minimal x86 machine emulator for the VDM
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _EMULATOR_H_
#define _EMULATOR_H_

/* INCLUDES *******************************************************************/

#include <fast486.h>

/* DEFINES ********************************************************************/

/* Basic Memory Management */
#define MEM_ALIGN_UP(ptr, align)    MEM_ALIGN_DOWN((ULONG_PTR)(ptr) + (align) - 1l, (align))
#define MEM_ALIGN_DOWN(ptr, align)  (PVOID)((ULONG_PTR)(ptr) & ~((align) - 1l))

#define TO_LINEAR(seg, off) (((seg) << 4) + (off))
#define MAX_SEGMENT 0xFFFF
#define MAX_OFFSET  0xFFFF
#define MAX_ADDRESS 0x1000000 // 16 MB of RAM; see also: kernel32/client/vdm.c!BaseGetVdmConfigInfo

#define FAR_POINTER(x)  \
    (PVOID)((ULONG_PTR)BaseAddress + TO_LINEAR(HIWORD(x), LOWORD(x)))

#define SEG_OFF_TO_PTR(seg, off)    \
    (PVOID)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), (off)))

#define REAL_TO_PHYS(ptr)   (PVOID)((ULONG_PTR)(ptr) + (ULONG_PTR)BaseAddress)
#define PHYS_TO_REAL(ptr)   (PVOID)((ULONG_PTR)(ptr) - (ULONG_PTR)BaseAddress)

#define ARRAY_INDEX(ptr, array) ((ULONG)(((ULONG_PTR)(ptr) - (ULONG_PTR)(array)) / sizeof(*array)))

/* BCD-Binary conversion */

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

/* System I/O ports */
#define CONTROL_SYSTEM_PORT61H  0x61


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

// extern FAST486_STATE EmulatorContext;
extern LPVOID  BaseAddress;
extern BOOLEAN VdmRunning;

/* FUNCTIONS ******************************************************************/

VOID DumpMemory(BOOLEAN TextFormat);

VOID WINAPI EmulatorReadMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID WINAPI EmulatorWriteMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

UCHAR WINAPI EmulatorIntAcknowledge
(
    PFAST486_STATE State
);

VOID WINAPI EmulatorFpu
(
    PFAST486_STATE State
);

VOID EmulatorException(BYTE ExceptionNumber, LPWORD Stack);

VOID EmulatorTerminate(VOID);

VOID EmulatorInterruptSignal(VOID);
VOID EmulatorSetA20(BOOLEAN Enabled);

BOOLEAN EmulatorInitialize(HANDLE ConsoleInput, HANDLE ConsoleOutput);
VOID EmulatorCleanup(VOID);

#endif // _EMULATOR_H_

/* EOF */
