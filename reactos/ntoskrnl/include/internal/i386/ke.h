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
#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#define KTRAP_FRAME_DEBUGEBP     (0x0)
#define KTRAP_FRAME_DEBUGEIP     (0x4)
#define KTRAP_FRAME_DEBUGARGMARK (0x8)
#define KTRAP_FRAME_DEBUGPOINTER (0xC)
#define KTRAP_FRAME_TEMPSS       (0x10)
#define KTRAP_FRAME_TEMPESP      (0x14)
#define KTRAP_FRAME_DR0          (0x18)
#define KTRAP_FRAME_DR1          (0x1C)
#define KTRAP_FRAME_DR2          (0x20)
#define KTRAP_FRAME_DR3          (0x24)
#define KTRAP_FRAME_DR6          (0x28)
#define KTRAP_FRAME_DR7          (0x2C)
#define KTRAP_FRAME_GS           (0x30)
#define KTRAP_FRAME_RESERVED1    (0x32)
#define KTRAP_FRAME_ES           (0x34)
#define KTRAP_FRAME_RESERVED2    (0x36)
#define KTRAP_FRAME_DS           (0x38)
#define KTRAP_FRAME_RESERVED3    (0x3A)
#define KTRAP_FRAME_EDX          (0x3C)
#define KTRAP_FRAME_ECX          (0x40)
#define KTRAP_FRAME_EAX          (0x44)
#define KTRAP_FRAME_PREVIOUS_MODE (0x48)
#define KTRAP_FRAME_EXCEPTION_LIST (0x4C)
#define KTRAP_FRAME_FS             (0x50)
#define KTRAP_FRAME_RESERVED4      (0x52)
#define KTRAP_FRAME_EDI            (0x54)
#define KTRAP_FRAME_ESI            (0x58)
#define KTRAP_FRAME_EBX            (0x5C)
#define KTRAP_FRAME_EBP            (0x60)
#define KTRAP_FRAME_ERROR_CODE     (0x64)
#define KTRAP_FRAME_EIP            (0x68)
#define KTRAP_FRAME_CS             (0x6C)
#define KTRAP_FRAME_EFLAGS         (0x70)
#define KTRAP_FRAME_ESP            (0x74)
#define KTRAP_FRAME_SS             (0x78)
#define KTRAP_FRAME_RESERVED5      (0x7A)
#define KTRAP_FRAME_V86_ES         (0x7C)
#define KTRAP_FRAME_RESERVED6      (0x7E)
#define KTRAP_FRAME_V86_DS         (0x80)
#define KTRAP_FRAME_RESERVED7      (0x82)
#define KTRAP_FRAME_V86_FS         (0x84)
#define KTRAP_FRAME_RESERVED8      (0x86)
#define KTRAP_FRAME_V86_GS         (0x88)
#define KTRAP_FRAME_RESERVED9      (0x8A)
#define KTRAP_FRAME_SIZE           (0x8C)

#define X86_EFLAGS_TF           0x00000100 /* Trap flag */
#define X86_EFLAGS_IF           0x00000200 /* Interrupt Enable flag */
#define X86_EFLAGS_IOPL         0x00003000 /* I/O Privilege Level bits */
#define X86_EFLAGS_NT           0x00004000 /* Nested Task flag */
#define X86_EFLAGS_RF           0x00010000 /* Resume flag */
#define X86_EFLAGS_VM           0x00020000 /* Virtual Mode */
#define X86_EFLAGS_ID           0x00200000 /* CPUID detection flag */

#define X86_CR0_PE              0x00000001 /* enable Protected Mode */
#define X86_CR0_NE              0x00000020 /* enable native FPU error reporting */
#define X86_CR0_TS              0x00000008 /* enable exception on FPU instruction for task switch */
#define X86_CR0_EM              0x00000004 /* enable FPU emulation (disable FPU) */
#define X86_CR0_MP              0x00000002 /* enable FPU monitoring */
#define X86_CR0_WP              0x00010000 /* enable Write Protect (copy on write) */
#define X86_CR0_PG              0x80000000 /* enable Paging */

#define X86_CR4_PAE             0x00000020 /* enable physical address extensions */
#define X86_CR4_PGE             0x00000080 /* enable global pages */
#define X86_CR4_OSFXSR          0x00000200 /* enable FXSAVE/FXRSTOR instructions */
#define X86_CR4_OSXMMEXCPT      0x00000400 /* enable #XF exception */

#define X86_FEATURE_TSC         0x00000010 /* time stamp counters are present */
#define X86_FEATURE_PAE         0x00000040 /* physical address extension is present */
#define X86_FEATURE_CX8         0x00000100 /* CMPXCHG8B instruction present */
#define X86_FEATURE_SYSCALL     0x00000800 /* SYSCALL/SYSRET support present */
#define X86_FEATURE_PGE         0x00002000 /* Page Global Enable */
#define X86_FEATURE_MMX         0x00800000 /* MMX extension present */
#define X86_FEATURE_FXSR        0x01000000 /* FXSAVE/FXRSTOR instructions present */
#define X86_FEATURE_SSE         0x02000000 /* SSE extension present */
#define X86_FEATURE_SSE2        0x04000000 /* SSE2 extension present */
#define X86_FEATURE_HT          0x10000000 /* Hyper-Threading present */

#define X86_EXT_FEATURE_SSE3    0x00000001 /* SSE3 extension present */
#define X86_EXT_FEATURE_3DNOW   0x40000000 /* 3DNOW! extension present */

/* Possible values for KTHREAD's NpxState */
#define NPX_STATE_INVALID   0x01
#define NPX_STATE_VALID     0x02
#define NPX_STATE_DIRTY     0x04

#ifndef __ASM__

typedef struct _KIRQ_TRAPFRAME
{
   ULONG Magic;
   ULONG Gs;
   ULONG Fs;
   ULONG Es;
   ULONG Ds;
   ULONG Eax;
   ULONG Ecx;
   ULONG Edx;
   ULONG Ebx;
   ULONG Esp;
   ULONG Ebp;
   ULONG Esi;
   ULONG Edi;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

typedef struct _KGDTENTRY {
    USHORT LimitLow;
    USHORT BaseLow;
    union {
        struct {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct {
            ULONG BaseMid       : 8;
            ULONG Type          : 5;
            ULONG Dpl           : 2;
            ULONG Pres          : 1;
            ULONG LimitHi       : 4;
            ULONG Sys           : 1;
            ULONG Reserved_0    : 1;
            ULONG Default_Big   : 1;
            ULONG Granularity   : 1;
            ULONG BaseHi        : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

typedef struct _KIDTENTRY {
    USHORT Offset;
    USHORT Selector;
    USHORT Access;
    USHORT ExtendedOffset;
} KIDTENTRY, *PKIDTENTRY;

extern ULONG Ke386CacheAlignment;

struct _KPCR;
VOID
KiInitializeGdt(struct _KPCR* Pcr);
VOID
Ki386ApplicationProcessorInitializeTSS(VOID);
VOID
Ki386BootInitializeTSS(VOID);
VOID
KiGdtPrepareForApplicationProcessorInit(ULONG Id);
VOID
Ki386InitializeLdt(VOID);
VOID
Ki386SetProcessorFeatures(VOID);
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
VOID
NtEarlyInitVdm(VOID);
VOID
KeApplicationProcessorInitDispatcher(VOID);
VOID
KeCreateApplicationProcessorIdleThread(ULONG Id);

typedef
VOID
STDCALL
(*PKSYSTEM_ROUTINE)(PKSTART_ROUTINE StartRoutine,
                    PVOID StartContext);

VOID
STDCALL
Ke386InitThreadWithContext(PKTHREAD Thread,
                           PKSYSTEM_ROUTINE SystemRoutine,
                           PKSTART_ROUTINE StartRoutine,
                           PVOID StartContext,
                           PCONTEXT Context);

VOID
STDCALL
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#define KeGetCurrentIrql(X) (((PKPCR)KPCR_BASE)->Irql)
#endif

#if defined(__GNUC__)
#define Ke386DisableInterrupts() __asm__("cli\n\t");
#define Ke386EnableInterrupts()  __asm__("sti\n\t");
#define Ke386HaltProcessor()     __asm__("hlt\n\t");
#define Ke386GetPageTableDirectory(X) \
                                 __asm__("movl %%cr3,%0\n\t" : "=d" (X));
#define Ke386SetPageTableDirectory(X) \
                                 __asm__("movl %0,%%cr3\n\t" \
                                     : /* no outputs */ \
                                     : "r" (X));
#define Ke386SetFileSelector(X) \
                                 __asm__("movl %0,%%cr3\n\t" \
                                     : /* no outputs */ \
                                     : "r" (X));
#define Ke386SetLocalDescriptorTable(X) \
                                 __asm__("lldt %0\n\t" \
                                     : /* no outputs */ \
                                     : "m" (X));
#define Ke386SetGlobalDescriptorTable(X) \
                                 __asm__("lgdt %0\n\t" \
                                     : /* no outputs */ \
                                     : "m" (X));
#define Ke386SaveFlags(x)        __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define Ke386RestoreFlags(x)     __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

#define _Ke386GetCr(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%cr" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })
#define _Ke386SetCr(N,X)         __asm__ __volatile__("movl %0,%%cr" #N : :"r" (X));

#define Ke386GetCr0()            _Ke386GetCr(0)
#define Ke386SetCr0(X)           _Ke386SetCr(0,X)
#define Ke386GetCr2()            _Ke386GetCr(2)
#define Ke386SetCr2(X)           _Ke386SetCr(2,X)
#define Ke386GetCr4()            _Ke386GetCr(4)
#define Ke386SetCr4(X)           _Ke386SetCr(4,X)

static inline LONG Ke386TestAndClearBit(ULONG BitPos, volatile PULONG Addr)
{
	LONG OldBit;

	__asm__ __volatile__(LOCK
	                     "btrl %2,%1\n\t"
	                     "sbbl %0,%0\n\t"
		             :"=r" (OldBit),"=m" (*Addr)
		             :"Ir" (BitPos)
			     : "memory");
	return OldBit;
}

static inline LONG Ke386TestAndSetBit(ULONG BitPos, volatile PULONG Addr)
{
	LONG OldBit;

	__asm__ __volatile__(LOCK
	                     "btsl %2,%1\n\t"
	                     "sbbl %0,%0\n\t"
		             :"=r" (OldBit),"=m" (*Addr)
		             :"Ir" (BitPos)
			     : "memory");
	return OldBit;
}


static inline void Ki386Cpuid(ULONG Op, PULONG Eax, PULONG Ebx, PULONG Ecx, PULONG Edx)
{
    __asm__("cpuid"
	    : "=a" (*Eax), "=b" (*Ebx), "=c" (*Ecx), "=d" (*Edx)
	    : "0" (Op));
}

#define Ke386Rdmsr(msr,val1,val2) __asm__ __volatile__("rdmsr" : "=a" (val1), "=d" (val2) : "c" (msr))

#define Ke386Wrmsr(msr,val1,val2) __asm__ __volatile__("wrmsr" : /* no outputs */ : "c" (msr), "a" (val1), "d" (val2))


#elif defined(_MSC_VER)

#define Ke386DisableInterrupts() __asm cli
#define Ke386EnableInterrupts()  __asm sti
#define Ke386HaltProcessor()     __asm hlt
#define Ke386GetPageTableDirectory(X) \
                                __asm mov eax, cr3; \
                                __asm mov X, eax;
#define Ke386SetPageTableDirectory(X) \
                                __asm mov eax, X; \
	                        __asm mov cr3, eax;
#else
#error Unknown compiler for inline assembler
#endif

#endif /* __ASM__ */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H */

/* EOF */
