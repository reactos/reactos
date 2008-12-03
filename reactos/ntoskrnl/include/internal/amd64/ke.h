#ifndef __NTOSKRNL_INCLUDE_INTERNAL_AMD64_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_AMD64_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

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

#define X86_FEATURE_VME         0x00000002 /* Virtual 8086 Extensions are present */
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

#define FRAME_EDITED        0xFFF8

#define X86_MSR_GSBASE          0xC0000101
#define X86_MSR_KERNEL_GSBASE   0xC0000102
#define X86_MSR_STAR            0xC0000081
#define X86_MSR_LSTAR           0xC0000082
#define X86_MSR_CSTAR           0xC0000083
#define X86_MSR_SFMASK          0xC0000084

#ifndef __ASM__

#include "intrin_i.h"

typedef struct _KIDT_INIT
{
    UCHAR InterruptId;
    UCHAR Dpl;
    UCHAR IstIndex;
    PVOID ServiceRoutine;
} KIDT_INIT, *PKIDT_INIT;

//#define KeArchFnInit() Ke386FnInit()
#define KeArchFnInit() DbgPrint("KeArchFnInit is unimplemented!\n");
#define KeArchHaltProcessor() Ke386HaltProcessor()
#define KfLowerIrql KeLowerIrql
#define KfAcquireSpinLock KeAcquireSpinLock
#define KfReleaseSpinLock KeReleaseSpinLock

extern ULONG Ke386CacheAlignment;

struct _KPCR;
VOID
KiInitializeGdt(struct _KPCR* Pcr);
VOID
Ki386ApplicationProcessorInitializeTSS(VOID);

VOID
FASTCALL
Ki386InitializeTss(
    IN PKTSS Tss,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt,
    IN UINT64 Stack
);

VOID KiDivideErrorFault();
VOID KiDebugTrapOrFault();
VOID KiNmiInterrupt();
VOID KiBreakpointTrap();
VOID KiOverflowTrap();
VOID KiBoundFault();
VOID KiInvalidOpcodeFault();
VOID KiNpxNotAvailableFault();
VOID KiDoubleFaultAbort();
VOID KiNpxSegmentOverrunAbort();
VOID KiInvalidTssFault();
VOID KiSegmentNotPresentFault();
VOID KiStackFault();
VOID KiGeneralProtectionFault();
VOID KiPageFault();
VOID KiFloatingErrorFault();
VOID KiAlignmentFault();
VOID KiMcheckAbort();
VOID KiXmmException();
VOID KiApcInterrupt();
VOID KiRaiseAssertion();
VOID KiDebugServiceTrap();
VOID KiDpcInterrupt();
VOID KiIpiInterrupt();

VOID
KiGdtPrepareForApplicationProcessorInit(ULONG Id);
VOID
Ki386InitializeLdt(VOID);
VOID
Ki386SetProcessorFeatures(VOID);

VOID
NTAPI
KiGetCacheInformation(VOID);

BOOLEAN
NTAPI
KiIsNpxPresent(
    VOID
);

BOOLEAN
NTAPI
KiIsNpxErrataPresent(
    VOID
);

VOID
NTAPI
KiSetProcessorType(VOID);

ULONG
NTAPI
KiGetFeatureBits(VOID);

VOID
NTAPI
KiInitializeCpuFeatures();

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
(NTAPI*PKSYSTEM_ROUTINE)(PKSTART_ROUTINE StartRoutine,
                    PVOID StartContext);

VOID
NTAPI
Ke386InitThreadWithContext(PKTHREAD Thread,
                           PKSYSTEM_ROUTINE SystemRoutine,
                           PKSTART_ROUTINE StartRoutine,
                           PVOID StartContext,
                           PCONTEXT Context);
#define KeArchInitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context) \
  Ke386InitThreadWithContext(Thread,SystemRoutine,StartRoutine,StartContext,Context)

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

#endif
#endif /* __NTOSKRNL_INCLUDE_INTERNAL_AMD64_KE_H */

/* EOF */
