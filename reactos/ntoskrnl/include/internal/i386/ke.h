#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H

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
(STDCALL*PKSYSTEM_ROUTINE)(PKSTART_ROUTINE StartRoutine,
                    PVOID StartContext);

VOID
STDCALL
Ke386InitThreadWithContext(PKTHREAD Thread,
                           PKSYSTEM_ROUTINE SystemRoutine,
                           PKSTART_ROUTINE StartRoutine,
                           PVOID StartContext,
                           PCONTEXT Context);

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
STDCALL
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#define KeGetCurrentIrql() (((PKPCR)KPCR_BASE)->Irql)
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

#endif
#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_KE_H */

/* EOF */
