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

#define _Ke386GetCr(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%cr" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })

#define _Ke386GetDr(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%dr" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })

#define _Ke386SetCr(N,X)         __asm__ __volatile__("movl %0,%%cr" #N : :"r" (X));
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
#define Ke386WbInvd()     __asm__("wbinvd\n\t");

#elif defined(_MSC_VER)

VOID
FORCEINLINE
Ke386WbInvd(VOID)
{
    __asm wbinvd;
}

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

#else
#error Unknown compiler for inline assembler
#endif

//
// CR Macros
//
#define Ke386GetCr0()               _Ke386GetCr(0)
#define Ke386SetCr0(X)              _Ke386SetCr(0,X)
#define Ke386GetCr2()               _Ke386GetCr(2)
#define Ke386SetCr2(X)              _Ke386SetCr(2,X)
#define Ke386GetCr3()               _Ke386GetCr(3)
#define Ke386SetCr3(X)              _Ke386SetCr(3,X)
#define Ke386GetCr4()               _Ke386GetCr(4)
#define Ke386SetCr4(X)              _Ke386SetCr(4,X)

//
// DR Macros
//
#define Ke386GetDr0()               _Ke386GetDr(0)
#define Ke386SetDr0(X)              _Ke386SetDr(0,X)
#define Ke386GetDr2()               _Ke386GetDr(2)
#define Ke386SetDr2(X)              _Ke386SetDr(2,X)
#define Ke386GetDr4()               _Ke386GetDr(4)
#define Ke386SetDr4(X)              _Ke386SetDr(4,X)

//
// Segment Macros
//
#define Ke386GetSs()                _Ke386GetSeg(ss)
#define Ke386GetFs()                _Ke386GetSeg(fs)
#define Ke386SetFs(X)               _Ke386SetSeg(fs, X)
#define Ke386SetDs(X)               _Ke386SetSeg(ds, X)
#define Ke386SetEs(X)               _Ke386SetSeg(es, X)

#endif

/* EOF */
