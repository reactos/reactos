#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

#if defined(__GNUC__)

#define Ke386SetInterruptDescriptorTable(X) \
    __asm__("lidt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386GetInterruptDescriptorTable(X) \
    __asm__("sidt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386SetGlobalDescriptorTable(X) \
    __asm__("lgdt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386GetGlobalDescriptorTable(X) \
    __asm__("sgdt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386GetLocalDescriptorTable(X) \
    __asm__("sldt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386SetLocalDescriptorTable(X) \
    __asm__("lldt %w0\n\t" \
    : /* no outputs */ \
    : "q" (X));

#define Ke386SaveFlags(x)        __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define Ke386RestoreFlags(x)     __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

#define _Ke386GetSeg(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })

#define _Ke386SetSeg(N,X)         __asm__ __volatile__("movl %0,%%" #N : :"r" (X));

#define _Ke386GetDr(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%dr" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })

#define _Ke386SetDr(N,X)         __asm__ __volatile__("movl %0,%%dr" #N : :"r" (X));

#define Ke386SetTr(X)         __asm__ __volatile__("ltr %%ax" : :"a" (X));
#define Ke386GetTr(X)         __asm__ __volatile__("str %%ax" : :"a" (X));

static inline void Ki386Cpuid(ULONG Op, PULONG Eax, PULONG Ebx, PULONG Ecx, PULONG Edx)
{
    __asm__("cpuid"
	    : "=a" (*Eax), "=b" (*Ebx), "=c" (*Ecx), "=d" (*Edx)
	    : "0" (Op));
}

#define Ke386Rdmsr(msr,val1,val2) __asm__ __volatile__("rdmsr" : "=a" (val1), "=d" (val2) : "c" (msr))

#define Ke386Wrmsr(msr,val1,val2) __asm__ __volatile__("wrmsr" : /* no outputs */ : "c" (msr), "a" (val1), "d" (val2))

#define FLUSH_TLB   {				\
    unsigned int tmp;	\
    __asm__ __volatile__(	\
    "movl %%cr3,%0\n\t"	\
    "movl %0,%%cr3\n\t"	\
    : "=r" (tmp)	\
    :: "memory");	\
}

#define FLUSH_TLB_ONE(addr) __asm__ __volatile__(		\
    "invlpg %0"				\
    :					\
    : "m" (*(volatile long *) (addr)))

#define Ke386HaltProcessor()     __asm__("hlt\n\t");
#define Ke386FnInit()     __asm__("fninit\n\t");

//
// DR Macros
//
#define Ke386GetDr0()               _Ke386GetDr(0)
#define Ke386GetDr1()               _Ke386GetDr(1)
#define Ke386SetDr0(X)              _Ke386SetDr(0,X)
#define Ke386GetDr2()               _Ke386GetDr(2)
#define Ke386SetDr2(X)              _Ke386SetDr(2,X)
#define Ke386GetDr3()               _Ke386GetDr(3)
#define Ke386GetDr4()               _Ke386GetDr(4)
#define Ke386SetDr4(X)              _Ke386SetDr(4,X)
#define Ke386GetDr6()               _Ke386GetDr(6)
#define Ke386GetDr7()               _Ke386GetDr(7)
#define Ke386SetDr7(X)              _Ke386SetDr(7,X)

//
// Segment Macros
//
#define Ke386GetSs()                _Ke386GetSeg(ss)
#define Ke386GetFs()                _Ke386GetSeg(fs)
#define Ke386SetFs(X)               _Ke386SetSeg(fs, X)
#define Ke386SetDs(X)               _Ke386SetSeg(ds, X)
#define Ke386SetEs(X)               _Ke386SetSeg(es, X)

#elif defined(_MSC_VER)

VOID
FORCEINLINE
Ke386FnInit(VOID)
{
    __asm fninit;
}

VOID
FORCEINLINE
Ke386HaltProcessor(VOID)
{
    __asm hlt;
}

VOID
FORCEINLINE
Ke386GetInterruptDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    __asm sidt Descriptor;
}

VOID
FORCEINLINE
Ke386SetInterruptDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    __asm lidt Descriptor;
}

VOID
FORCEINLINE
Ke386GetGlobalDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    __asm sgdt Descriptor;
}

VOID
FORCEINLINE
Ke386SetGlobalDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    __asm lgdt Descriptor;
}

VOID
FORCEINLINE
Ke386GetLocalDescriptorTable(OUT USHORT Descriptor)
{
    __asm sldt Descriptor;
}

VOID
FORCEINLINE
Ke386SetLocalDescriptorTable(IN USHORT Descriptor)
{
    __asm lldt Descriptor;
}

VOID
FORCEINLINE
Ke386SaveFlags(IN ULONG Flags)
{
    __asm pushf;
    __asm pop Flags;
}

VOID
FORCEINLINE
Ke386RestoreFlags(IN ULONG Flags)
{
    __asm push Flags;
    __asm popf;
}

VOID
FORCEINLINE
Ke386SetTr(IN USHORT Tr)
{
    __asm ltr Tr;
}

USHORT
FORCEINLINE
Ke386GetTr(IN USHORT Tr)
{
    __asm str Tr;
}

//
// DR Macros
//
ULONG
FORCEINLINE
Ke386GetDr0(VOID)
{
    __asm mov eax, dr0;
}

ULONG
FORCEINLINE
Ke386GetDr1(VOID)
{
    __asm mov eax, dr1;
}

ULONG
FORCEINLINE
Ke386GetDr2(VOID)
{
    __asm mov eax, dr2;
}

ULONG
FORCEINLINE
Ke386GetDr3(VOID)
{
    __asm mov eax, dr3;
}

ULONG
FORCEINLINE
Ke386GetDr6(VOID)
{
    __asm mov eax, dr6;
}

ULONG
FORCEINLINE
Ke386GetDr7(VOID)
{
    __asm mov eax, dr7;
}

VOID
FORCEINLINE
Ke386SetDr0(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr0, eax;
}

VOID
FORCEINLINE
Ke386SetDr2(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr2, eax;
}

VOID
FORCEINLINE
Ke386SetDr3(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr3, eax;
}

VOID
FORCEINLINE
Ke386SetDr6(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr6, eax;
}

VOID
FORCEINLINE
Ke386SetDr7(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr7, eax;
}

//
// Segment Macros
//
USHORT
FORCEINLINE
Ke386GetSs(VOID)
{
    __asm mov ax, ss;
}

USHORT
FORCEINLINE
Ke386GetFs(VOID)
{
    __asm mov ax, fs;
}

USHORT
FORCEINLINE
Ke386GetDs(VOID)
{
    __asm mov ax, ds;
}

USHORT
FORCEINLINE
Ke386GetEs(VOID)
{
    __asm mov ax, es;
}

VOID
FORCEINLINE
Ke386SetSs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov ss, ax;
}

VOID
FORCEINLINE
Ke386SetFs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov fs, ax;
}

VOID
FORCEINLINE
Ke386SetDs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov ds, ax;
}

VOID
FORCEINLINE
Ke386SetEs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov es, ax;
}

#else
#error Unknown compiler for inline assembler
#endif

#endif

/* EOF */
