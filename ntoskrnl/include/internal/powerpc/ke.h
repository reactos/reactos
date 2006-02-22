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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

/* Possible values for KTHREAD's NpxState */
#define NPX_STATE_INVALID   0x01
#define NPX_STATE_VALID     0x02
#define NPX_STATE_DIRTY     0x04

#ifndef __ASM__

typedef struct _KIRQ_TRAPFRAME
{
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

extern ULONG KePPCCacheAlignment;

struct _KPCR;
VOID
KiInitializeGdt(struct _KPCR* Pcr);
VOID
KiPPCApplicationProcessorInitializeTSS(VOID);
VOID
KiPPCBootInitializeTSS(VOID);
VOID
KiGdtPrepareForApplicationProcessorInit(ULONG Id);
VOID
KiPPCInitializeLdt(VOID);
VOID
KiPPCSetProcessorFeatures(VOID);
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
VOID
NtEarlyInitVdm(VOID);

#ifdef CONFIG_SMP
#define LOCK "isync ; "
#else
#define LOCK ""
#endif


static inline LONG KePPCTestAndClearBit(ULONG BitPos, volatile PULONG Addr)
{
    ULONG OldValue, NewValue;

    __asm__ __volatile__ ("lwarx %0,0,%1"
                          : "=r" (OldValue), "=r" (*Addr)
                          :
                          : "memory");

    NewValue = OldValue & ~(1<<BitPos);

    __asm__ __volatile__ ("stwcx. %0,0,%3\n\t"
                          "beq success\n\t"
                          "add    %2,0,%1\n"
                          "success:\n\t"
                          "isync\n\t"
                          : "=r" (NewValue), "=r" (OldValue)
                          : "w" (NewValue), "w" (*Addr)
                          : "memory");

    return NewValue & (1 << BitPos);
}

static inline LONG KePPCTestAndSetBit(ULONG BitPos, volatile PULONG Addr)
{
    ULONG OldValue, NewValue;

    __asm__ __volatile__ ("lwarx %0,0,%1"
                          : "=r" (OldValue), "=r" (*Addr)
                          :
                          : "memory");

    NewValue = OldValue | (1<<BitPos);

    __asm__ __volatile__ ("stwcx. %0,0,%3\n\t"
                          "beq success\n\t"
                          "add    %2,0,%1\n"
                          "success:\n\t"
                          "isync\n\t"
                          : "=r" (NewValue), "=r" (OldValue)
                          : "w" (NewValue), "w" (*Addr)
                          : "memory");

    return NewValue & (1 << BitPos);
}

#define KePPCRdmsr(msr,val1,val2) __asm__ __volatile__("mfmsr 2")

#define KePPCWrmsr(msr,val1,val2) __asm__ __volatile__("mtmsr 3")


#define KePPCDisableInterrupts() \
__asm__ __volatile__("mfmsr 0\n\t" \
                     "li    8,0x7fff\n\t" \
                     "and   0,8,0\n\t" \
                     "mtmsr 0\n\t")

#define KePPCEnableInterrupts() \
 __asm__ __volatile__("mfmsr 0\n\t" \
                      "li    8,0x8000\n\t" \
                      "or    0,8,0\n\t" \
                      "mtmsr 0\n\t")

#define KePPCHaltProcessor()     ;

#endif /* __ASM__ */

#define KeArchEraseFlags()
#define KeArchDisableInterrupts() KePPCDisableInterrupts()

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H */

/* EOF */
