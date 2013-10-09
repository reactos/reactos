/*
 * Soft386 386/486 CPU Emulation Library
 * soft386.h
 *
 * Copyright (C) 2013 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

#ifndef _SOFT386_H_
#define _SOFT386_H_

/* DEFINES ********************************************************************/

#define SOFT386_NUM_GEN_REGS    8
#define SOFT386_NUM_SEG_REGS    6
#define SOFT386_NUM_CTRL_REGS   8
#define SOFT386_NUM_DBG_REGS    8

#define SOFT386_CR0_PE  (1 << 0)
#define SOFT386_CR0_MP  (1 << 1)
#define SOFT386_CR0_EM  (1 << 2)
#define SOFT386_CR0_TS  (1 << 3)
#define SOFT386_CR0_ET  (1 << 4)
#define SOFT386_CR0_NE  (1 << 5)
#define SOFT386_CR0_WP  (1 << 16)
#define SOFT386_CR0_AM  (1 << 18)
#define SOFT386_CR0_NW  (1 << 29)
#define SOFT386_CR0_CD  (1 << 30)
#define SOFT386_CR0_PG  (1 << 31)

#define SOFT386_IDT_TASK_GATE       0x5
#define SOFT386_IDT_INT_GATE        0x6
#define SOFT386_IDT_TRAP_GATE       0x7
#define SOFT386_IDT_INT_GATE_32     0xE
#define SOFT386_IDT_TRAP_GATE_32    0xF

#define SOFT386_PREFIX_SEG      (1 << 0)
#define SOFT386_PREFIX_OPSIZE   (1 << 1)
#define SOFT386_PREFIX_ADSIZE   (1 << 2)
#define SOFT386_PREFIX_LOCK     (1 << 3)
#define SOFT386_PREFIX_REPNZ    (1 << 4)
#define SOFT386_PREFIX_REP      (1 << 5)

struct _SOFT386_STATE;
typedef struct _SOFT386_STATE SOFT386_STATE, *PSOFT386_STATE;

typedef enum _SOFT386_GEN_REGS
{
    SOFT386_REG_EAX,
    SOFT386_REG_ECX,
    SOFT386_REG_EDX,
    SOFT386_REG_EBX,
    SOFT386_REG_ESP,
    SOFT386_REG_EBP,
    SOFT386_REG_ESI,
    SOFT386_REG_EDI
} SOFT386_GEN_REGS, *PSOFT386_GEN_REGS;

typedef enum _SOFT386_SEG_REGS
{
    SOFT386_REG_ES,
    SOFT386_REG_CS,
    SOFT386_REG_SS,
    SOFT386_REG_DS,
    SOFT386_REG_FS,
    SOFT386_REG_GS
} SOFT386_SEG_REGS, *PSOFT386_SEG_REGS;

typedef enum _SOFT386_CTRL_REGS
{
    SOFT386_REG_CR0,
    SOFT386_REG_CR1,
    SOFT386_REG_CR2,
    SOFT386_REG_CR3,
    SOFT386_REG_CR4,
    SOFT386_REG_CR5,
    SOFT386_REG_CR6,
    SOFT386_REG_CR7
} SOFT386_CTRL_REGS, *PSOFT386_CTRL_REGS;

typedef enum _SOFT386_DBG_REGS
{
    SOFT386_REG_DR0,
    SOFT386_REG_DR1,
    SOFT386_REG_DR2,
    SOFT386_REG_DR3,
    SOFT386_REG_DR4,
    SOFT386_REG_DR5,
    SOFT386_REG_DR6,
    SOFT386_REG_DR7
} SOFT386_DBG_REGS, *PSOFT386_DBG_REGS;

typedef enum _SOFT386_EXCEPTIONS
{
    SOFT386_EXCEPTION_DE = 0x00,
    SOFT386_EXCEPTION_DB = 0x01,
    SOFT386_EXCEPTION_BP = 0x03,
    SOFT386_EXCEPTION_OF = 0x04,
    SOFT386_EXCEPTION_BR = 0x05,
    SOFT386_EXCEPTION_UD = 0x06,
    SOFT386_EXCEPTION_NM = 0x07,
    SOFT386_EXCEPTION_DF = 0x08,
    SOFT386_EXCEPTION_TS = 0x0A,
    SOFT386_EXCEPTION_NP = 0x0B,
    SOFT386_EXCEPTION_SS = 0x0C,
    SOFT386_EXCEPTION_GP = 0x0D,
    SOFT386_EXCEPTION_PF = 0x0E,
    SOFT386_EXCEPTION_MF = 0x10,
    SOFT386_EXCEPTION_AC = 0x11,
    SOFT386_EXCEPTION_MC = 0x12
} SOFT386_EXCEPTIONS, *PSOFT386_EXCEPTIONS;

typedef
BOOLEAN
(NTAPI *SOFT386_MEM_READ_PROC)
(
    PSOFT386_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef
BOOLEAN
(NTAPI *SOFT386_MEM_WRITE_PROC)
(
    PSOFT386_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

typedef
VOID
(NTAPI *SOFT386_IO_READ_PROC)
(
    PSOFT386_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG Size
);

typedef
VOID
(NTAPI *SOFT386_IO_WRITE_PROC)
(
    PSOFT386_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG Size
);

typedef
VOID
(NTAPI *SOFT386_IDLE_PROC)
(
    PSOFT386_STATE State
);

typedef
VOID
(NTAPI *SOFT386_BOP_PROC)
(
    PSOFT386_STATE State,
    USHORT BopCode
);

typedef union _SOFT386_REG
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
} SOFT386_REG, *PSOFT386_REG;

typedef struct _SOFT386_SEG_REG
{
    USHORT Selector;

    /* Descriptor cache */
    ULONG Accessed      : 1;
    ULONG ReadWrite     : 1;
    ULONG DirConf       : 1;
    ULONG Executable    : 1;
    ULONG SystemType    : 1;
    ULONG Dpl           : 2;
    ULONG Present       : 1;
    ULONG Size          : 1;
    ULONG Limit;
    ULONG Base;
} SOFT386_SEG_REG, *PSOFT386_SEG_REG;

typedef struct
{
    ULONG Limit         : 16;
    ULONG Base          : 24;
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
} SOFT386_GDT_ENTRY, *PSOFT386_GDT_ENTRY;

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
} SOFT386_IDT_ENTRY, *PSOFT386_IDT_ENTRY;

typedef struct _SOFT386_TABLE_REG
{
    USHORT Size;
    ULONG Address;
} SOFT386_TABLE_REG, *PSOFT386_TABLE_REG;

typedef union _SOFT386_FLAGS_REG
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
        ULONG Vif       : 1;
        ULONG Vip       : 1;
        ULONG Id        : 1;

        // ULONG Reserved : 10;
    };
} SOFT386_FLAGS_REG, *PSOFT386_FLAGS_REG;

typedef struct _SOFT386_TSS
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
} SOFT386_TSS, *PSOFT386_TSS;

struct _SOFT386_STATE
{
    SOFT386_MEM_READ_PROC MemReadCallback;
    SOFT386_MEM_WRITE_PROC MemWriteCallback;
    SOFT386_IO_READ_PROC IoReadCallback;
    SOFT386_IO_WRITE_PROC IoWriteCallback;
    SOFT386_IDLE_PROC IdleCallback;
    SOFT386_BOP_PROC BopCallback;
    SOFT386_REG GeneralRegs[SOFT386_NUM_GEN_REGS];
    SOFT386_SEG_REG SegmentRegs[SOFT386_NUM_SEG_REGS];
    SOFT386_REG InstPtr;
    SOFT386_FLAGS_REG Flags;
    SOFT386_TABLE_REG Gdtr, Idtr, Ldtr, Tss;
    ULONGLONG TimeStampCounter;
    ULONG ControlRegisters[SOFT386_NUM_CTRL_REGS];
    ULONG DebugRegisters[SOFT386_NUM_DBG_REGS];
    ULONG ExceptionCount;
    ULONG PrefixFlags;
    SOFT386_SEG_REGS SegmentOverride;
    BOOLEAN HardwareInt;
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
Soft386Continue(PSOFT386_STATE State);

VOID
NTAPI
Soft386StepInto(PSOFT386_STATE State);

VOID
NTAPI
Soft386StepOver(PSOFT386_STATE State);

VOID
NTAPI
Soft386StepOut(PSOFT386_STATE State);

VOID
NTAPI
Soft386DumpState(PSOFT386_STATE State);

VOID
NTAPI
Soft386Reset(PSOFT386_STATE State);

VOID
NTAPI
Soft386Interrupt(PSOFT386_STATE State, UCHAR Number);

VOID
NTAPI
Soft386ExecuteAt(PSOFT386_STATE State, USHORT Segment, ULONG Offset);

VOID
NTAPI
Soft386SetStack(PSOFT386_STATE State, USHORT Segment, ULONG Offset);

VOID
NTAPI
Soft386SetSegment
(
    PSOFT386_STATE State,
    SOFT386_SEG_REGS Segment,
    USHORT Selector
);

#endif // _SOFT386_H_

/* EOF */
