/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/cpu/x86context.h
 * PURPOSE:         x86 CPU Context Frame definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE: Taken from the PSDK.
 */

#ifndef __X86CONTEXT_H__
#define __X86CONTEXT_H__

#pragma once

/* Clean everything that may have been defined before */
#undef SIZE_OF_80387_REGISTERS
#undef MAXIMUM_SUPPORTED_EXTENSION
#undef CONTEXT_i386
#undef CONTEXT_i486
#undef CONTEXT_CONTROL
#undef CONTEXT_INTEGER
#undef CONTEXT_SEGMENTS
#undef CONTEXT_FLOATING_POINT
#undef CONTEXT_DEBUG_REGISTERS
#undef CONTEXT_EXTENDED_REGISTERS
#undef CONTEXT_FULL
#undef CONTEXT_ALL
#undef CONTEXT_DEBUGGER
#undef CONTEXT_XSTATE



#define SIZE_OF_80387_REGISTERS         80
#define MAXIMUM_SUPPORTED_EXTENSION     512

#define CONTEXT_i386               0x00010000
#define CONTEXT_i486               0x00010000

#define CONTEXT_CONTROL            (CONTEXT_i386|0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT_INTEGER            (CONTEXT_i386|0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT_SEGMENTS           (CONTEXT_i386|0x00000004L) // DS, ES, FS, GS
#define CONTEXT_FLOATING_POINT     (CONTEXT_i386|0x00000008L) // 387 state
#define CONTEXT_DEBUG_REGISTERS    (CONTEXT_i386|0x00000010L) // DB 0-3,6,7
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L) // CPU-specific extensions

#define CONTEXT_FULL (CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define CONTEXT_ALL  (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS |  \
                      CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS |      \
                      CONTEXT_EXTENDED_REGISTERS)

#define CONTEXT_DEBUGGER        (CONTEXT_FULL | CONTEXT_FLOATING_POINT)
#define CONTEXT_XSTATE          (CONTEXT_i386 | 0x00000040L)


typedef struct _X87FLOATING_SAVE_AREA
{
    ULONG ControlWord;
    ULONG StatusWord;
    ULONG TagWord;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG Cr0NpxState;
} X87FLOATING_SAVE_AREA, *PX87FLOATING_SAVE_AREA;

#include "pshpack4.h"
/*
 * x86 CPU Context Frame
 */
typedef struct _X86CONTEXT
{
    /*
     * The flags values within this flag control the contents of
     * a CONTEXT record.
     */
    ULONG ContextFlags;

    /*
     * Section specified/returned if CONTEXT_DEBUG_REGISTERS
     * is set in ContextFlags.
     */
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;

    /*
     * Section specified/returned if CONTEXT_FLOATING_POINT
     * is set in ContextFlags.
     */
    X87FLOATING_SAVE_AREA FloatSave;

    /*
     * Section specified/returned if CONTEXT_SEGMENTS
     * is set in ContextFlags.
     */
    ULONG SegGs;
    ULONG SegFs;
    ULONG SegEs;
    ULONG SegDs;

    /*
     * Section specified/returned if CONTEXT_INTEGER
     * is set in ContextFlags.
     */
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;

    /*
     * Section specified/returned if CONTEXT_CONTROL
     * is set in ContextFlags.
     */
    ULONG Ebp;
    ULONG Eip;
    ULONG SegCs;
    ULONG EFlags;
    ULONG Esp;
    ULONG SegSs;

    /*
     * Section specified/returned if CONTEXT_EXTENDED_REGISTERS
     * is set in ContextFlags. The format and contexts are processor specific.
     */
    UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} X86CONTEXT;
#include "poppack.h"

#endif /* __X86CONTEXT_H__ */
