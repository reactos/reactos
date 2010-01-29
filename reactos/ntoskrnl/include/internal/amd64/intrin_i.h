#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

#define KeSetCurrentIrql(x) __writecr8(x)

PKGDTENTRY64
FORCEINLINE
KiGetGdtEntry(PVOID pGdt, USHORT Selector)
{
    return (PKGDTENTRY64)((ULONG64)pGdt + (Selector & ~RPL_MASK));
}

PVOID
FORCEINLINE
KiGetGdtDescriptorBase(PKGDTENTRY Entry)
{
    return (PVOID)((ULONG64)Entry->BaseLow |
                   (ULONG64)Entry->Bytes.BaseMiddle << 16 |
                   (ULONG64)Entry->Bytes.BaseHigh << 24 |
                   (ULONG64)Entry->BaseUpper << 32);
}

VOID
FORCEINLINE
KiSetGdtDescriptorBase(PKGDTENTRY Entry, ULONG64 Base)
{
    Entry->BaseLow = Base & 0xffff;
    Entry->Bits.BaseMiddle = (Base >> 16) & 0xff;
    Entry->Bits.BaseHigh = (Base >> 24) & 0xff;
    Entry->BaseUpper = Base >> 32;
}

VOID
FORCEINLINE
KiSetGdtDescriptorLimit(PKGDTENTRY Entry, ULONG Limit)
{
    Entry->LimitLow = Limit & 0xffff;
    Entry->Bits.LimitHigh = Limit >> 16;
}

VOID
FORCEINLINE
KiInitGdtEntry(PKGDTENTRY64 Entry, ULONG64 Base, ULONG Size, UCHAR Type, UCHAR Dpl)
{
    KiSetGdtDescriptorBase(Entry, Base);
    KiSetGdtDescriptorLimit(Entry, Size - 1);
    Entry->Bits.Type = Type;
    Entry->Bits.Dpl = Dpl;
    Entry->Bits.Present = 1;
    Entry->Bits.System = 0;
    Entry->Bits.LongMode = 0;
    Entry->Bits.DefaultBig = 0;
    Entry->Bits.Granularity = 0;
    Entry->MustBeZero = 0;
}

#if defined(__GNUC__)

static __inline__ __attribute__((always_inline)) void __lgdt(void *Source)
{
	__asm__ __volatile__("lgdt %0" : : "m"(*(short*)Source));
}

static __inline__ __attribute__((always_inline)) void __sgdt(void *Destination)
{
	__asm__ __volatile__("sgdt %0" : : "m"(*(short*)Destination) : "memory");
}

static __inline__ __attribute__((always_inline)) void __lldt(unsigned short Value)
{
	__asm__ __volatile__("lldt %0" : : "rm"(Value));
}

static __inline__ __attribute__((always_inline)) void __sldt(void *Destination)
{
	__asm__ __volatile__("sldt %0" : : "m"(*(short*)Destination) : "memory");
}

static __inline__ __attribute__((always_inline)) void __ldmxcsr(unsigned long *Source)
{
	__asm__ __volatile__("ldmxcsr %0" : : "m"(*Source));
}

static __inline__ __attribute__((always_inline)) void __stmxcsr(unsigned long *Destination)
{
	__asm__ __volatile__("stmxcsr %0" : : "m"(*Destination) : "memory");
}

static __inline__ __attribute__((always_inline)) void __ltr(unsigned short Source)
{
	__asm__ __volatile__("ltr %0" : : "rm"(Source));
}

static __inline__ __attribute__((always_inline)) void __str(unsigned short *Destination)
{
	__asm__ __volatile__("str %0" : : "m"(*Destination) : "memory");
}


#define _Ke386GetSeg(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                 })

#define _Ke386SetSeg(N,X)         __asm__ __volatile__("movl %0,%%" #N : :"r" (X));

#define _Ke386GetDr(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movq %%dr" #N ",%0\n\t" :"=r" (__d)); \
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
#define Ke386SetCr2(X)              __asm__ __volatile__("movq %0,%%cr2" : :"r" ((void*)X));

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
#define Ke386SetGs(X)               _Ke386SetSeg(gs, X)
#define Ke386SetDs(X)               _Ke386SetSeg(ds, X)
#define Ke386SetEs(X)               _Ke386SetSeg(es, X)
#define Ke386SetSs(X)               _Ke386SetSeg(ss, X)

#elif defined(_MSC_VER)

VOID
FORCEINLINE
Ke386Wrmsr(IN ULONG Register,
           IN ULONG Var1,
           IN ULONG Var2)
{
    __writemsr(Register, (ULONG64)Var1 <<32 | Var2);
}

ULONGLONG
FORCEINLINE
Ke386Rdmsr(IN ULONG Register,
           IN ULONG Var1,
           IN ULONG Var2)
{
    return __readmsr(Register);
}

#define UNIMPLEMENTED DbgPrint("unimplemented!!!\n");

VOID
FORCEINLINE
Ki386Cpuid(IN ULONG Operation,
           OUT PULONG Var1,
           OUT PULONG Var2,
           OUT PULONG Var3,
           OUT PULONG Var4)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386FnInit(VOID)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386HaltProcessor(VOID)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386GetInterruptDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetInterruptDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386GetGlobalDescriptorTable(OUT KDESCRIPTOR Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetGlobalDescriptorTable(IN KDESCRIPTOR Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386GetLocalDescriptorTable(OUT USHORT Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetLocalDescriptorTable(IN USHORT Descriptor)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SaveFlags(IN ULONG Flags)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386RestoreFlags(IN ULONG Flags)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetTr(IN USHORT Tr)
{
    UNIMPLEMENTED;
}

USHORT
FORCEINLINE
Ke386GetTr(IN USHORT Tr)
{
    UNIMPLEMENTED;
}

//
// CR Macros
//
VOID
FORCEINLINE
Ke386SetCr2(IN ULONG Value)
{
    UNIMPLEMENTED;
}

//
// DR Macros
//
ULONG
FORCEINLINE
Ke386GetDr0(VOID)
{
    UNIMPLEMENTED;
}

ULONG
FORCEINLINE
Ke386GetDr1(VOID)
{
    UNIMPLEMENTED;
}

ULONG
FORCEINLINE
Ke386GetDr2(VOID)
{
    UNIMPLEMENTED;
}

ULONG
FORCEINLINE
Ke386GetDr3(VOID)
{
    UNIMPLEMENTED;
}

ULONG
FORCEINLINE
Ke386GetDr6(VOID)
{
    UNIMPLEMENTED;
}

ULONG
FORCEINLINE
Ke386GetDr7(VOID)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr0(IN ULONG Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr1(IN ULONG Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr2(IN ULONG Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr3(IN ULONG Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr6(IN ULONG Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDr7(IN ULONG Value)
{
    UNIMPLEMENTED;
}

//
// Segment Macros
//
USHORT
FORCEINLINE
Ke386GetSs(VOID)
{
    UNIMPLEMENTED;
}

USHORT
FORCEINLINE
Ke386GetFs(VOID)
{
    UNIMPLEMENTED;
}

USHORT
FORCEINLINE
Ke386GetDs(VOID)
{
    UNIMPLEMENTED;
}

USHORT
FORCEINLINE
Ke386GetEs(VOID)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetSs(IN USHORT Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetFs(IN USHORT Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetDs(IN USHORT Value)
{
    UNIMPLEMENTED;
}

VOID
FORCEINLINE
Ke386SetEs(IN USHORT Value)
{
    UNIMPLEMENTED;
}

#else
#error Unknown compiler for inline assembler
#endif

#endif

/* EOF */
