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
    : "=m" (X));

#define Ke386SetGlobalDescriptorTable(X) \
    __asm__("lgdt %0\n\t" \
    : /* no outputs */ \
    : "m" (X));

#define Ke386GetGlobalDescriptorTable(X) \
    __asm__("sgdt %0\n\t" \
    : "=m" (X));

#define Ke386GetLocalDescriptorTable(X) \
    __asm__("sldt %0\n\t" \
    : "=m" (X));

#define Ke386SetLocalDescriptorTable(X) \
    __asm__("lldt %w0\n\t" \
    : /* no outputs */ \
    : "q" (X));

#define Ke386SetTr(X)                   __asm__ __volatile__("ltr %%ax" : :"a" (X));

#define Ke386GetTr(X) \
    __asm__("str %0\n\t" \
    : "=m" (X));

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


static inline void Ki386Cpuid(ULONG Op, PULONG Eax, PULONG Ebx, PULONG Ecx, PULONG Edx)
{
    __asm__("cpuid"
	    : "=a" (*Eax), "=b" (*Ebx), "=c" (*Ecx), "=d" (*Edx)
	    : "0" (Op));
}

#define Ke386Rdmsr(msr,val1,val2) __asm__ __volatile__("rdmsr" : "=a" (val1), "=d" (val2) : "c" (msr))
#define Ke386Wrmsr(msr,val1,val2) __asm__ __volatile__("wrmsr" : /* no outputs */ : "c" (msr), "a" (val1), "d" (val2))

#define Ke386HaltProcessor()        __asm__("hlt\n\t");

#define Ke386FnInit()               __asm__("fninit\n\t");


//
// CR Macros
//
#define Ke386SetCr2(X)              __asm__ __volatile__("movl %0,%%cr2" : :"r" (X));

//
// DR Macros
//
#define Ke386GetDr0()               _Ke386GetDr(0)
#define Ke386GetDr1()               _Ke386GetDr(1)
#define Ke386SetDr0(X)              _Ke386SetDr(0,X)
#define Ke386SetDr1(X)              _Ke386SetDr(1,X)
#define Ke386GetDr2()               _Ke386GetDr(2)
#define Ke386SetDr2(X)              _Ke386SetDr(2,X)
#define Ke386GetDr3()               _Ke386GetDr(3)
#define Ke386SetDr3(X)              _Ke386SetDr(3,X)
#define Ke386GetDr4()               _Ke386GetDr(4)
#define Ke386SetDr4(X)              _Ke386SetDr(4,X)
#define Ke386GetDr6()               _Ke386GetDr(6)
#define Ke386SetDr6(X)              _Ke386SetDr(6,X)
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

FORCEINLINE
VOID
Ke386Wrmsr(IN ULONG Register,
           IN ULONG Var1,
           IN ULONG Var2)
{
    __asm mov eax, Var1;
    __asm mov edx, Var2;
    __asm wrmsr;
}

FORCEINLINE
ULONGLONG
Ke386Rdmsr(IN ULONG Register,
           IN ULONG Var1,
           IN ULONG Var2)
{
    __asm mov eax, Var1;
    __asm mov edx, Var2;
    __asm rdmsr;
}

FORCEINLINE
VOID
Ki386Cpuid(IN ULONG Operation,
           OUT PULONG Var1,
           OUT PULONG Var2,
           OUT PULONG Var3,
           OUT PULONG Var4)
{
    __asm mov eax, Operation;
    __asm cpuid;
    __asm mov [Var1], eax;
    __asm mov [Var2], ebx;
    __asm mov [Var3], ecx;
    __asm mov [Var4], edx;
}

FORCEINLINE
VOID
Ke386FnInit(VOID)
{
    __asm fninit;
}

FORCEINLINE
VOID
Ke386HaltProcessor(VOID)
{
    __asm hlt;
}

FORCEINLINE
VOID
Ke386GetInterruptDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    __asm sidt Descriptor;
}

FORCEINLINE
VOID
Ke386SetInterruptDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    __asm lidt Descriptor;
}

FORCEINLINE
VOID
Ke386GetGlobalDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    __asm sgdt Descriptor;
}

FORCEINLINE
VOID
Ke386SetGlobalDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    __asm lgdt Descriptor;
}

FORCEINLINE
VOID
Ke386GetLocalDescriptorTable(OUT USHORT Descriptor)
{
    __asm sldt Descriptor;
}

FORCEINLINE
VOID
Ke386SetLocalDescriptorTable(IN USHORT Descriptor)
{
    __asm lldt Descriptor;
}

FORCEINLINE
VOID
Ke386SaveFlags(IN ULONG Flags)
{
    __asm pushf;
    __asm pop Flags;
}

FORCEINLINE
VOID
Ke386RestoreFlags(IN ULONG Flags)
{
    __asm push Flags;
    __asm popf;
}

FORCEINLINE
VOID
Ke386SetTr(IN USHORT Tr)
{
    __asm ltr Tr;
}

FORCEINLINE
USHORT
Ke386GetTr(IN USHORT Tr)
{
    __asm str Tr;
}

//
// CR Macros
//
FORCEINLINE
VOID
Ke386SetCr2(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov cr2, eax;
}

//
// DR Macros
//
FORCEINLINE
ULONG
Ke386GetDr0(VOID)
{
    __asm mov eax, dr0;
}

FORCEINLINE
ULONG
Ke386GetDr1(VOID)
{
    __asm mov eax, dr1;
}

FORCEINLINE
ULONG
Ke386GetDr2(VOID)
{
    __asm mov eax, dr2;
}

FORCEINLINE
ULONG
Ke386GetDr3(VOID)
{
    __asm mov eax, dr3;
}

FORCEINLINE
ULONG
Ke386GetDr6(VOID)
{
    __asm mov eax, dr6;
}

FORCEINLINE
ULONG
Ke386GetDr7(VOID)
{
    __asm mov eax, dr7;
}

FORCEINLINE
VOID
Ke386SetDr0(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr0, eax;
}

FORCEINLINE
VOID
Ke386SetDr1(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr1, eax;
}

FORCEINLINE
VOID
Ke386SetDr2(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr2, eax;
}

FORCEINLINE
VOID
Ke386SetDr3(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr3, eax;
}

FORCEINLINE
VOID
Ke386SetDr6(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr6, eax;
}

FORCEINLINE
VOID
Ke386SetDr7(IN ULONG Value)
{
    __asm mov eax, Value;
    __asm mov dr7, eax;
}

//
// Segment Macros
//
FORCEINLINE
USHORT
Ke386GetSs(VOID)
{
    __asm mov ax, ss;
}

FORCEINLINE
USHORT
Ke386GetFs(VOID)
{
    __asm mov ax, fs;
}

FORCEINLINE
USHORT
Ke386GetDs(VOID)
{
    __asm mov ax, ds;
}

FORCEINLINE
USHORT
Ke386GetEs(VOID)
{
    __asm mov ax, es;
}

FORCEINLINE
VOID
Ke386SetSs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov ss, ax;
}

FORCEINLINE
VOID
Ke386SetFs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov fs, ax;
}

FORCEINLINE
VOID
Ke386SetDs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov ds, ax;
}

FORCEINLINE
VOID
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
