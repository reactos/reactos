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
#define KTRAP_FRAME_TEMPCS       (0x10)
#define KTRAP_FRAME_TEMPEIP      (0x14)
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

#define X86_EFLAGS_VM	    0x00020000 /* Virtual Mode */
#define X86_EFLAGS_ID	    0x00200000 /* CPUID detection flag */

#define X86_CR4_PAE	    0x00000020 /* enable physical address extensions */
#define X86_CR4_PGE	    0x00000080 /* enable global pages */

#define X86_FEATURE_PAE	    0x00000040 /* physical address extension is present */	
#define X86_FEATURE_PGE	    0x00002000 /* Page Global Enable */

#ifndef __ASM__

typedef struct _KTRAP_FRAME
{
   PVOID DebugEbp;
   PVOID DebugEip;
   PVOID DebugArgMark;
   PVOID DebugPointer;
   PVOID TempCs;
   PVOID TempEip;
   ULONG Dr0;
   ULONG Dr1;
   ULONG Dr2;
   ULONG Dr3;
   ULONG Dr6;
   ULONG Dr7;
   USHORT Gs;
   USHORT Reserved1;
   USHORT Es;
   USHORT Reserved2;
   USHORT Ds;
   USHORT Reserved3;
   ULONG Edx;
   ULONG Ecx;
   ULONG Eax;
   ULONG PreviousMode;
   PVOID ExceptionList;
   USHORT Fs;
   USHORT Reserved4;
   ULONG Edi;
   ULONG Esi;
   ULONG Ebx;
   ULONG Ebp;
   ULONG ErrorCode;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
   ULONG Esp;
   USHORT Ss;
   USHORT Reserved5;
   USHORT V86_Es;
   USHORT Reserved6;
   USHORT V86_Ds;
   USHORT Reserved7;
   USHORT V86_Fs;
   USHORT Reserved8;
   USHORT V86_Gs;
   USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _KIRQ_TRAPFRAME
{
   ULONG Magic;
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
ULONG KeAllocateGdtSelector(ULONG Desc[2]);
VOID KeFreeGdtSelector(ULONG Entry);
VOID
NtEarlyInitVdm(VOID);

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
#define Ke386SaveFlags(x)        __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define Ke386RestoreFlags(x)     __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")
#define Ke386GetCr4()            ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%cr4,%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })
#define Ke386SetCr4(X)           __asm__ __volatile__("movl %0,%%cr4": :"r" (X));

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
