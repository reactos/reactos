/*
 * Fast486 386/486 CPU Emulation Library
 * common.h
 *
 * Copyright (C) 2014 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

#ifndef _COMMON_H_
#define _COMMON_H_

#pragma once

/* DEFINES ********************************************************************/

#define SIGN_FLAG_BYTE  0x80
#define SIGN_FLAG_WORD  0x8000
#define SIGN_FLAG_LONG  0x80000000
#define REAL_MODE_FLAGS_MASK 0x57FD5
#define PROT_MODE_FLAGS_MASK 0x50DD5

/* Block size for string operations */
#define STRING_BLOCK_SIZE 4096

#define GET_SEGMENT_RPL(s)          ((s) & 3)
#define GET_SEGMENT_INDEX(s)        ((s) & 0xFFF8)
#define SEGMENT_TABLE_INDICATOR     (1 << 2)
#define EXCEPTION_HAS_ERROR_CODE(x) (((x) == 8) || ((x) >= 10 && (x) <= 14))

#define NO_LOCK_PREFIX()\
if (State->PrefixFlags & FAST486_PREFIX_LOCK)\
{\
    Fast486Exception(State, FAST486_EXCEPTION_UD);\
    return;\
}

#define TOGGLE_OPSIZE(x)\
    if (State->PrefixFlags & FAST486_PREFIX_OPSIZE) x = !x;

#define TOGGLE_ADSIZE(x)\
    if (State->PrefixFlags & FAST486_PREFIX_ADSIZE) x = !x;

#define SWAP(x, y) { (x) ^= (y); (y) ^= (x); (x) ^= (y); }

#define ALIGNMENT_CHECK(x, a) if (State->Flags.Ac \
                                  && (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_AM)\
                                  && (State->Cpl == 3)\
                                  && (((x) % (a)) != 0))\
{\
    Fast486Exception(State, FAST486_EXCEPTION_AC);\
    return FALSE;\
}

#define PAGE_ALIGN(x)   ((x) & 0xFFFFF000)
#define PAGE_OFFSET(x)  ((x) & 0x00000FFF)
#define GET_ADDR_PDE(x) ((x) >> 22)
#define GET_ADDR_PTE(x) (((x) >> 12) & 0x3FF)
#define INVALID_TLB_FIELD 0xFFFFFFFF
#define NUM_TLB_ENTRIES 0x100000

typedef struct _FAST486_MOD_REG_RM
{
    FAST486_GEN_REGS Register;
    BOOLEAN Memory;
    union
    {
        FAST486_GEN_REGS SecondRegister;
        ULONG MemoryAddress;
    };
} FAST486_MOD_REG_RM, *PFAST486_MOD_REG_RM;

typedef enum _FAST486_TASK_SWITCH_TYPE
{
    FAST486_TASK_JUMP,
    FAST486_TASK_CALL,
    FAST486_TASK_RETURN
} FAST486_TASK_SWITCH_TYPE, *PFAST486_TASK_SWITCH_TYPE;

#include <pshpack1.h>

typedef union _FAST486_PAGE_DIR
{
    struct
    {
        ULONG Present       : 1;
        ULONG Writeable     : 1;
        ULONG Usermode      : 1;
        ULONG WriteThrough  : 1;
        ULONG NoCache       : 1;
        ULONG Accessed      : 1;
        ULONG AlwaysZero    : 1;
        ULONG Size          : 1;
        ULONG Unused        : 4;
        ULONG TableAddress  : 20;
    };
    ULONG Value;
} FAST486_PAGE_DIR, *PFAST486_PAGE_DIR;

C_ASSERT(sizeof(FAST486_PAGE_DIR) == sizeof(ULONG));

typedef union _FAST486_PAGE_TABLE
{
    struct
    {
        ULONG Present       : 1;
        ULONG Writeable     : 1;
        ULONG Usermode      : 1;
        ULONG WriteThrough  : 1;
        ULONG NoCache       : 1;
        ULONG Accessed      : 1;
        ULONG Dirty         : 1;
        ULONG AlwaysZero    : 1;
        ULONG Global        : 1;
        ULONG Unused        : 3;
        ULONG Address       : 20;
    };
    ULONG Value;
} FAST486_PAGE_TABLE, *PFAST486_PAGE_TABLE;

C_ASSERT(sizeof(FAST486_PAGE_DIR) == sizeof(ULONG));

#include <poppack.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
Fast486ReadMemory
(
    PFAST486_STATE State,
    FAST486_SEG_REGS SegmentReg,
    ULONG Offset,
    BOOLEAN InstFetch,
    PVOID Buffer,
    ULONG Size
);

BOOLEAN
Fast486WriteMemory
(
    PFAST486_STATE State,
    FAST486_SEG_REGS SegmentReg,
    ULONG Offset,
    PVOID Buffer,
    ULONG Size
);

BOOLEAN
FASTCALL
Fast486PerformInterrupt
(
    PFAST486_STATE State,
    UCHAR Number
);

VOID
FASTCALL
Fast486ExceptionWithErrorCode
(
    PFAST486_STATE State,
    FAST486_EXCEPTIONS ExceptionCode,
    ULONG ErrorCode
);

BOOLEAN
FASTCALL
Fast486TaskSwitch
(
    PFAST486_STATE State,
    FAST486_TASK_SWITCH_TYPE Type,
    USHORT Selector
);

/* INLINED FUNCTIONS **********************************************************/

#include "common.inl"

#endif // _COMMON_H_

/* EOF */
