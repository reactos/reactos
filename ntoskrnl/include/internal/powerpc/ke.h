/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#include <ndk/powerpc/ketypes.h>

/* Possible values for KTHREAD's NpxState */
#define KPCR_BASE 0xff000000
#define NPX_STATE_INVALID   0x01
#define NPX_STATE_VALID     0x02
#define NPX_STATE_DIRTY     0x04

#ifndef __ASM__

typedef struct _KIRQ_TRAPFRAME
{
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

extern ULONG KePPCCacheAlignment;

//#define KD_BREAKPOINT_TYPE
//#define KD_BREAKPOINT_SIZE
//#define KD_BREAKPOINT_VALUE

//
// Macro to get the second level cache size field name which differs between
// CISC and RISC architectures, as the former has unified I/D cache
//
#define KiGetSecondLevelDCacheSize() ((PKIPCR)KeGetPcr())->SecondLevelDcacheSize

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Dr0)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Dr0 = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Dr0)

#define KeGetContextReturnRegister(Context) \
    ((Context)->Gpr3)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->Gpr3 = (ReturnValue))

//
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
//#define KeGetTrapFrameInterruptState(TrapFrame) \

#define KePPCRdmsr(msr,val1,val2) __asm__ __volatile__("mfmsr 3")

#define KePPCWrmsr(msr,val1,val2) __asm__ __volatile__("mtmsr 3")

#define PPC_MIN_CACHE_LINE_SIZE 32

FORCEINLINE struct _KPCR * NTHALAPI KeGetCurrentKPCR(
    VOID)
{
    return (struct _KPCR *)__readfsdword(0x1c);
}

FORCEINLINE
VOID
KeFlushProcessTb(VOID)
{
    /* Flush the TLB */
    __asm__("sync\n\tisync\n\t");
}

FORCEINLINE
VOID
KeSweepICache(IN PVOID BaseAddress,
              IN SIZE_T FlushSize)
{
    //
    // Always sweep the whole cache
    //
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(FlushSize);
    __asm__ __volatile__("tlbsync");
}

FORCEINLINE
PRKTHREAD
KeGetCurrentThread(VOID)
{
    /* Return the current thread */
    return KeGetCurrentPrcb()->CurrentThread;
}

FORCEINLINE
VOID
KiRundownThread(IN PKTHREAD Thread)
{
    /* FIXME */
}

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

#endif /* __ASM__ */

/* EOF */
