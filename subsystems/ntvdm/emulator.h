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

#ifndef NEW_EMULATOR
#include <softx86/softx86.h>
#include <softx86/softx87.h>
#endif

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

/* CR0 */
#define EMULATOR_CR0_PE (1 << 0)
#define EMULATOR_CR0_MP (1 << 1)
#define EMULATOR_CR0_EM (1 << 2)
#define EMULATOR_CR0_TS (1 << 3)
#define EMULATOR_CR0_ET (1 << 4)
#define EMULATOR_CR0_NE (1 << 5)
#define EMULATOR_CR0_WP (1 << 16)
#define EMULATOR_CR0_AM (1 << 18)
#define EMULATOR_CR0_NW (1 << 29)
#define EMULATOR_CR0_CD (1 << 30)
#define EMULATOR_CR0_PG (1 << 31)

/* GDT Access byte */
#define GDT_SEG_ACCESSED (1 << 0)
#define GDT_DATA_WRITEABLE (1 << 1)
#define GDT_CODE_READABLE (1 << 1)
#define GDT_CONFORMING (1 << 2)
#define GDT_DIRECTION (1 << 2)
#define GDT_CODE_SEGMENT (1 << 3)
#define GDT_PRESENT (1 << 7)

/* GDT flags */
#define GDT_32BIT_SEGMENT (1 << 2)
#define GDT_PAGE_GRANULARITY (1 << 3)

/* Common definitions */
#define EMULATOR_NUM_GENERAL_REGS 8
#define EMULATOR_NUM_SEGMENT_REGS 6
#define EMULATOR_NUM_CONTROL_REGS 8
#define EMULATOR_NUM_DEBUG_REGS 8
#define MAX_GDT_ENTRIES 8192
#define EMULATOR_BOP 0xC4C4
#define EMULATOR_INT_BOP 0xBEEF
#define STACK_INT_NUM 0
#define STACK_IP 1
#define STACK_CS 2
#define STACK_FLAGS 3

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

typedef union
{
    struct
    {
        BYTE LowByte;
        BYTE HighByte;
    };
    WORD LowWord;
    DWORD Long;
} EMULATOR_REGISTER, *PEMULATOR_REGISTER;

typedef struct
{
    ULONG Limit : 16;
    ULONG Base : 24;
    ULONG AccessByte : 8;
    ULONG LimitHigh : 4;
    ULONG Flags : 4;
    ULONG BaseHigh : 8;
} EMULATOR_GDT_ENTRY;

typedef struct
{
    ULONG Offset : 16;
    ULONG Selector : 16;
    ULONG Zero : 8;
    ULONG TypeAndAttributes : 8;
    ULONG OffsetHigh : 16;
} EMULATOR_IDT_ENTRY;

typedef struct
{
    WORD Size;
    DWORD Address;
} EMULATOR_TABLE_REGISTER;

typedef struct
{
    EMULATOR_REGISTER Registers[EMULATOR_NUM_GENERAL_REGS
                                + EMULATOR_NUM_SEGMENT_REGS];
    EMULATOR_REGISTER Flags;
    EMULATOR_REGISTER InstructionPointer;
    EMULATOR_REGISTER ControlRegisters[EMULATOR_NUM_CONTROL_REGS];
    EMULATOR_REGISTER DebugRegisters[EMULATOR_NUM_DEBUG_REGS];
    ULONGLONG TimeStampCounter;
    BOOLEAN OperandSizeOverload;
    BOOLEAN AddressSizeOverload;
    EMULATOR_TABLE_REGISTER Gdtr, Idtr;
    EMULATOR_GDT_ENTRY CachedDescriptors[EMULATOR_NUM_SEGMENT_REGS];
    UINT ExceptionCount;
} EMULATOR_CONTEXT, *PEMULATOR_CONTEXT;

typedef VOID (*EMULATOR_OPCODE_HANDLER)(PEMULATOR_CONTEXT Context, BYTE Opcode);

#ifndef NEW_EMULATOR
extern softx86_ctx EmulatorContext;
extern softx87_ctx FpuEmulatorContext;
#else
extern EMULATOR_CONTEXT EmulatorContext;
#endif

/* FUNCTIONS ******************************************************************/

BOOLEAN EmulatorInitialize();
VOID EmulatorSetStack(WORD Segment, DWORD Offset);
VOID EmulatorExecute(WORD Segment, WORD Offset);
VOID EmulatorInterrupt(BYTE Number);
VOID EmulatorExternalInterrupt(BYTE Number);
ULONG EmulatorGetRegister(ULONG Register);
ULONG EmulatorGetProgramCounter(VOID);
VOID EmulatorSetRegister(ULONG Register, ULONG Value);
BOOLEAN EmulatorGetFlag(ULONG Flag);
VOID EmulatorSetFlag(ULONG Flag);
VOID EmulatorClearFlag(ULONG Flag);
VOID EmulatorStep();
VOID EmulatorCleanup();
VOID EmulatorSetA20(BOOLEAN Enabled);

#endif // _EMULATOR_H_

/* EOF */
