/*
 * Fast486 386/486 CPU Emulation Library
 * fast486.h
 *
 * Copyright (C) 2015 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _FAST486_H_
#define _FAST486_H_

#pragma once

/* DEFINES ********************************************************************/

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define FAST486_NUM_GEN_REGS    8
#define FAST486_NUM_SEG_REGS    6
#define FAST486_NUM_CTRL_REGS   3
#define FAST486_NUM_DBG_REGS    6
#define FAST486_NUM_FPU_REGS    8

#define FAST486_CR0_PE  (1 << 0)
#define FAST486_CR0_MP  (1 << 1)
#define FAST486_CR0_EM  (1 << 2)
#define FAST486_CR0_TS  (1 << 3)
#define FAST486_CR0_ET  (1 << 4)
#define FAST486_CR0_NE  (1 << 5)
#define FAST486_CR0_WP  (1 << 16)
#define FAST486_CR0_AM  (1 << 18)
#define FAST486_CR0_NW  (1 << 29)
#define FAST486_CR0_CD  (1 << 30)
#define FAST486_CR0_PG  (1 << 31)

#define FAST486_DR4_B0 (1 << 0)
#define FAST486_DR4_B1 (1 << 1)
#define FAST486_DR4_B2 (1 << 2)
#define FAST486_DR4_B3 (1 << 3)
#define FAST486_DR4_BD (1 << 13)
#define FAST486_DR4_BS (1 << 14)
#define FAST486_DR4_BT (1 << 15)

#define FAST486_DR5_L0 (1 << 0)
#define FAST486_DR5_G0 (1 << 1)
#define FAST486_DR5_L1 (1 << 2)
#define FAST486_DR5_G1 (1 << 3)
#define FAST486_DR5_L2 (1 << 4)
#define FAST486_DR5_G2 (1 << 5)
#define FAST486_DR5_L3 (1 << 6)
#define FAST486_DR5_G3 (1 << 7)
#define FAST486_DR5_LE (1 << 8)
#define FAST486_DR5_GE (1 << 9)
#define FAST486_DR5_GD (1 << 13)

#define FAST486_DBG_BREAK_EXEC 0
#define FAST486_DBG_BREAK_WRITE 1
#define FAST486_DBG_BREAK_READWRITE 3

#define FAST486_DR4_RESERVED 0xFFFF1FF0
#define FAST486_DR5_RESERVED 0x0000DC00

#define FAST486_LDT_SIGNATURE       0x02
#define FAST486_TASK_GATE_SIGNATURE 0x05
#define FAST486_IDT_INT_GATE        0x06
#define FAST486_IDT_TRAP_GATE       0x07
#define FAST486_TSS_SIGNATURE       0x09
#define FAST486_BUSY_TSS_SIGNATURE  0x0B
#define FAST486_CALL_GATE_SIGNATURE 0x0C
#define FAST486_IDT_INT_GATE_32     0x0E
#define FAST486_IDT_TRAP_GATE_32    0x0F

#define FAST486_PREFIX_SEG      (1 << 0)
#define FAST486_PREFIX_OPSIZE   (1 << 1)
#define FAST486_PREFIX_ADSIZE   (1 << 2)
#define FAST486_PREFIX_LOCK     (1 << 3)
#define FAST486_PREFIX_REPNZ    (1 << 4)
#define FAST486_PREFIX_REP      (1 << 5)

#define FAST486_FPU_DEFAULT_CONTROL 0x037F

#define FAST486_PAGE_SIZE 4096
#define FAST486_CACHE_SIZE 32

/*
 * These are condiciones sine quibus non that should be respected, because
 * otherwise when fetching DWORDs you would read extra garbage bytes
 * (by reading outside of the prefetch buffer). The prefetch cache must
 * also not cross a page boundary.
 */
C_ASSERT((FAST486_CACHE_SIZE >= sizeof(DWORD))
         && (FAST486_CACHE_SIZE <= FAST486_PAGE_SIZE));

struct _FAST486_STATE;
typedef struct _FAST486_STATE FAST486_STATE, *PFAST486_STATE;

typedef enum _FAST486_GEN_REGS
{
    FAST486_REG_EAX,
    FAST486_REG_ECX,
    FAST486_REG_EDX,
    FAST486_REG_EBX,
    FAST486_REG_ESP,
    FAST486_REG_EBP,
    FAST486_REG_ESI,
    FAST486_REG_EDI
} FAST486_GEN_REGS, *PFAST486_GEN_REGS;

typedef enum _FAST486_SEG_REGS
{
    FAST486_REG_ES,
    FAST486_REG_CS,
    FAST486_REG_SS,
    FAST486_REG_DS,
    FAST486_REG_FS,
    FAST486_REG_GS
} FAST486_SEG_REGS, *PFAST486_SEG_REGS;

typedef enum _FAST486_CTRL_REGS
{
    FAST486_REG_CR0 = 0,
    FAST486_REG_CR2 = 1,
    FAST486_REG_CR3 = 2,
} FAST486_CTRL_REGS, *PFAST486_CTRL_REGS;

typedef enum _FAST486_DBG_REGS
{
    FAST486_REG_DR0 = 0,
    FAST486_REG_DR1 = 1,
    FAST486_REG_DR2 = 2,
    FAST486_REG_DR3 = 3,
    FAST486_REG_DR4 = 4,
    FAST486_REG_DR5 = 5,
    FAST486_REG_DR6 = 4, // alias to DR4
    FAST486_REG_DR7 = 5  // alias to DR5
} FAST486_DBG_REGS, *PFAST486_DBG_REGS;

typedef enum _FAST486_EXCEPTIONS
{
    FAST486_EXCEPTION_DE = 0x00,
    FAST486_EXCEPTION_DB = 0x01,
    FAST486_EXCEPTION_BP = 0x03,
    FAST486_EXCEPTION_OF = 0x04,
    FAST486_EXCEPTION_BR = 0x05,
    FAST486_EXCEPTION_UD = 0x06,
    FAST486_EXCEPTION_NM = 0x07,
    FAST486_EXCEPTION_DF = 0x08,
    FAST486_EXCEPTION_TS = 0x0A,
    FAST486_EXCEPTION_NP = 0x0B,
    FAST486_EXCEPTION_SS = 0x0C,
    FAST486_EXCEPTION_GP = 0x0D,
    FAST486_EXCEPTION_PF = 0x0E,
    FAST486_EXCEPTION_MF = 0x10,
    FAST486_EXCEPTION_AC = 0x11,
    FAST486_EXCEPTION_MC = 0x12
} FAST486_EXCEPTIONS, *PFAST486_EXCEPTIONS;

typedef
VOID
(NTAPI *FAST486_MEM_READ_PROC)
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef
VOID
(NTAPI *FAST486_MEM_WRITE_PROC)
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef
VOID
(NTAPI *FAST486_IO_READ_PROC)
(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

typedef
VOID
(NTAPI *FAST486_IO_WRITE_PROC)
(
    PFAST486_STATE State,
    USHORT Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

typedef
VOID
(NTAPI *FAST486_BOP_PROC)
(
    PFAST486_STATE State,
    UCHAR BopCode
);

typedef
UCHAR
(NTAPI *FAST486_INT_ACK_PROC)
(
    PFAST486_STATE State
);

typedef
VOID
(NTAPI *FAST486_FPU_PROC)
(
    PFAST486_STATE State
);

typedef union _FAST486_REG
{
    union
    {
        struct
        {
            UCHAR LowByte;
            UCHAR HighByte;
        };
        USHORT LowWord;
    };
    ULONG Long;
} FAST486_REG, *PFAST486_REG;

typedef struct _FAST486_SEG_REG
{
    USHORT Selector;

    /* Descriptor cache */
    ULONG Accessed      : 1;
    ULONG ReadWrite     : 1;
    ULONG DirConf       : 1;
    ULONG Executable    : 1;
    ULONG SystemType    : 1;
    ULONG Rpl           : 2;
    ULONG Dpl           : 2;
    ULONG Present       : 1;
    ULONG Size          : 1;
    ULONG Limit;
    ULONG Base;
} FAST486_SEG_REG, *PFAST486_SEG_REG;

typedef struct _FAST486_LDT_REG
{
    USHORT Selector;
    ULONG Base;
    ULONG Limit;
} FAST486_LDT_REG, *PFAST486_LDT_REG;

typedef struct _FAST486_TASK_REG
{
    USHORT Selector;
    ULONG Base;
    ULONG Limit;
} FAST486_TASK_REG, *PFAST486_TASK_REG;

#include <pshpack1.h>

typedef struct
{
    ULONG Limit         : 16;
    ULONG Base          : 16;
    ULONG BaseMid       : 8;
    ULONG Accessed      : 1;
    ULONG ReadWrite     : 1;
    ULONG DirConf       : 1;
    ULONG Executable    : 1;
    ULONG SystemType    : 1;
    ULONG Dpl           : 2;
    ULONG Present       : 1;
    ULONG LimitHigh     : 4;
    ULONG Avl           : 1;
    ULONG Reserved      : 1;
    ULONG Size          : 1;
    ULONG Granularity   : 1;
    ULONG BaseHigh      : 8;
} FAST486_GDT_ENTRY, *PFAST486_GDT_ENTRY;

/* Verify the structure size */
C_ASSERT(sizeof(FAST486_GDT_ENTRY) == sizeof(ULONGLONG));

typedef struct
{
    ULONG Limit         : 16;
    ULONG Base          : 16;
    ULONG BaseMid       : 8;
    ULONG Signature     : 5;
    ULONG Dpl           : 2;
    ULONG Present       : 1;
    ULONG LimitHigh     : 4;
    ULONG Avl           : 1;
    ULONG Reserved      : 2;
    ULONG Granularity   : 1;
    ULONG BaseHigh      : 8;
} FAST486_SYSTEM_DESCRIPTOR, *PFAST486_SYSTEM_DESCRIPTOR;

/* Verify the structure size */
C_ASSERT(sizeof(FAST486_SYSTEM_DESCRIPTOR) == sizeof(ULONGLONG));

typedef struct
{
    ULONG Offset : 16;
    ULONG Selector : 16;
    ULONG ParamCount : 5;
    ULONG Reserved : 3;
    ULONG Type : 4;
    ULONG SystemType : 1;
    ULONG Dpl : 2;
    ULONG Present : 1;
    ULONG OffsetHigh : 16;
} FAST486_CALL_GATE, *PFAST486_CALL_GATE;

/* Verify the structure size */
C_ASSERT(sizeof(FAST486_CALL_GATE) == sizeof(ULONGLONG));

typedef struct
{
    ULONG Offset        : 16;
    ULONG Selector      : 16;
    ULONG Zero          : 8;
    ULONG Type          : 4;
    ULONG Storage       : 1;
    ULONG Dpl           : 2;
    ULONG Present       : 1;
    ULONG OffsetHigh    : 16;
} FAST486_IDT_ENTRY, *PFAST486_IDT_ENTRY;

/* Verify the structure size */
C_ASSERT(sizeof(FAST486_IDT_ENTRY) == sizeof(ULONGLONG));

#include <poppack.h>

typedef struct _FAST486_TABLE_REG
{
    USHORT Size;
    ULONG Address;
} FAST486_TABLE_REG, *PFAST486_TABLE_REG;

typedef union _FAST486_FLAGS_REG
{
    USHORT LowWord;
    ULONG Long;

    struct
    {
        ULONG Cf        : 1;
        ULONG AlwaysSet : 1;
        ULONG Pf        : 1;
        ULONG Reserved0 : 1;
        ULONG Af        : 1;
        ULONG Reserved1 : 1;
        ULONG Zf        : 1;
        ULONG Sf        : 1;
        ULONG Tf        : 1;
        ULONG If        : 1;
        ULONG Df        : 1;
        ULONG Of        : 1;
        ULONG Iopl      : 2;
        ULONG Nt        : 1;
        ULONG Reserved2 : 1;
        ULONG Rf        : 1;
        ULONG Vm        : 1;
        ULONG Ac        : 1;

        // ULONG Reserved : 13;
    };
} FAST486_FLAGS_REG, *PFAST486_FLAGS_REG;

typedef struct _FAST486_TSS
{
    ULONG Link;
    ULONG Esp0;
    ULONG Ss0;
    ULONG Esp1;
    ULONG Ss1;
    ULONG Esp2;
    ULONG Ss2;
    ULONG Cr3;
    ULONG Eip;
    ULONG Eflags;
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Esp;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    ULONG Es;
    ULONG Cs;
    ULONG Ss;
    ULONG Ds;
    ULONG Fs;
    ULONG Gs;
    ULONG Ldtr;
    ULONG IopbOffset;
} FAST486_TSS, *PFAST486_TSS;

typedef struct _FAST486_FPU_DATA_REG
{
    ULONGLONG Mantissa;
    USHORT Exponent;
    UCHAR Sign;
} FAST486_FPU_DATA_REG, *PFAST486_FPU_DATA_REG;

typedef const FAST486_FPU_DATA_REG *PCFAST486_FPU_DATA_REG;

typedef union _FAST486_FPU_STATUS_REG
{
    USHORT Value;

    struct
    {
        ULONG Ie : 1;
        ULONG De : 1;
        ULONG Ze : 1;
        ULONG Oe : 1;
        ULONG Ue : 1;
        ULONG Pe : 1;
        ULONG Sf : 1;
        ULONG Es : 1;
        ULONG Code0 : 1;
        ULONG Code1 : 1;
        ULONG Code2 : 1;
        ULONG Top : 3;
        ULONG Code3 : 1;
        ULONG Busy : 1;
    };
} FAST486_FPU_STATUS_REG, *PFAST486_FPU_STATUS_REG;

typedef union _FAST486_FPU_CONTROL_REG
{
    USHORT Value;

    struct
    {
        ULONG Im : 1;
        ULONG Dm : 1;
        ULONG Zm : 1;
        ULONG Om : 1;
        ULONG Um : 1;
        ULONG Pm : 1;
        ULONG Reserved : 2;
        ULONG Pc : 2;
        ULONG Rc : 2;
        ULONG Inf : 1;
        // ULONG Reserved1 : 3;
    };
} FAST486_FPU_CONTROL_REG, *PFAST486_FPU_CONTROL_REG;

struct _FAST486_STATE
{
    FAST486_MEM_READ_PROC MemReadCallback;
    FAST486_MEM_WRITE_PROC MemWriteCallback;
    FAST486_IO_READ_PROC IoReadCallback;
    FAST486_IO_WRITE_PROC IoWriteCallback;
    FAST486_BOP_PROC BopCallback;
    FAST486_INT_ACK_PROC IntAckCallback;
    FAST486_FPU_PROC FpuCallback;
    FAST486_REG GeneralRegs[FAST486_NUM_GEN_REGS];
    FAST486_SEG_REG SegmentRegs[FAST486_NUM_SEG_REGS];
    FAST486_REG InstPtr, SavedInstPtr;
    FAST486_FLAGS_REG Flags;
    FAST486_TABLE_REG Gdtr, Idtr;
    FAST486_LDT_REG Ldtr;
    FAST486_TASK_REG TaskReg;
    UCHAR Cpl;
    ULONG ControlRegisters[FAST486_NUM_CTRL_REGS];
    ULONG DebugRegisters[FAST486_NUM_DBG_REGS];
    ULONG ExceptionCount;
    ULONG PrefixFlags;
    FAST486_SEG_REGS SegmentOverride;
    BOOLEAN Halted;
    BOOLEAN IntSignaled;
    BOOLEAN DoNotInterrupt;
    PULONG Tlb;
#ifndef FAST486_NO_PREFETCH
    BOOLEAN PrefetchValid;
    ULONG PrefetchAddress;
    UCHAR PrefetchCache[FAST486_CACHE_SIZE];
#endif
#ifndef FAST486_NO_FPU
    FAST486_FPU_DATA_REG FpuRegisters[FAST486_NUM_FPU_REGS];
    FAST486_FPU_STATUS_REG FpuStatus;
    FAST486_FPU_CONTROL_REG FpuControl;
    USHORT FpuTag;
    FAST486_REG FpuLastInstPtr;
    USHORT FpuLastCodeSel;
    FAST486_REG FpuLastOpPtr;
    USHORT FpuLastDataSel;
#endif
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
Fast486Initialize(PFAST486_STATE         State,
                  FAST486_MEM_READ_PROC  MemReadCallback,
                  FAST486_MEM_WRITE_PROC MemWriteCallback,
                  FAST486_IO_READ_PROC   IoReadCallback,
                  FAST486_IO_WRITE_PROC  IoWriteCallback,
                  FAST486_BOP_PROC       BopCallback,
                  FAST486_INT_ACK_PROC   IntAckCallback,
                  FAST486_FPU_PROC       FpuCallback,
                  PULONG                 Tlb);

VOID
NTAPI
Fast486Reset(PFAST486_STATE State);

VOID
NTAPI
Fast486Continue(PFAST486_STATE State);

VOID
NTAPI
Fast486StepInto(PFAST486_STATE State);

VOID
NTAPI
Fast486StepOver(PFAST486_STATE State);

VOID
NTAPI
Fast486StepOut(PFAST486_STATE State);

VOID
NTAPI
Fast486DumpState(PFAST486_STATE State);

VOID
NTAPI
Fast486InterruptSignal(PFAST486_STATE State);

VOID
NTAPI
Fast486ExecuteAt(PFAST486_STATE State, USHORT Segment, ULONG Offset);

VOID
NTAPI
Fast486SetStack(PFAST486_STATE State, USHORT Segment, ULONG Offset);

VOID
NTAPI
Fast486SetSegment
(
    PFAST486_STATE State,
    FAST486_SEG_REGS Segment,
    USHORT Selector
);

VOID
NTAPI
Fast486Rewind(PFAST486_STATE State);

#endif // _FAST486_H_

/* EOF */
