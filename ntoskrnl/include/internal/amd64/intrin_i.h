#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

FORCEINLINE
VOID
KeSetCurrentIrql(KIRQL Irql)
{
    __writecr8(Irql);
}

FORCEINLINE
PKGDTENTRY64
KiGetGdtEntry(PVOID pGdt, USHORT Selector)
{
    return (PKGDTENTRY64)((ULONG64)pGdt + (Selector & ~RPL_MASK));
}

FORCEINLINE
PVOID
KiGetGdtDescriptorBase(PKGDTENTRY Entry)
{
    return (PVOID)((ULONG64)Entry->BaseLow |
                   (ULONG64)Entry->Bytes.BaseMiddle << 16 |
                   (ULONG64)Entry->Bytes.BaseHigh << 24 |
                   (ULONG64)Entry->BaseUpper << 32);
}

FORCEINLINE
VOID
KiSetGdtDescriptorBase(PKGDTENTRY Entry, ULONG64 Base)
{
    Entry->BaseLow = Base & 0xffff;
    Entry->Bits.BaseMiddle = (Base >> 16) & 0xff;
    Entry->Bits.BaseHigh = (Base >> 24) & 0xff;
    Entry->BaseUpper = Base >> 32;
}

FORCEINLINE
VOID
KiSetGdtDescriptorLimit(PKGDTENTRY Entry, ULONG Limit)
{
    Entry->LimitLow = Limit & 0xffff;
    Entry->Bits.LimitHigh = Limit >> 16;
}

FORCEINLINE
VOID
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


#elif defined(_MSC_VER)

void __lgdt(void *Source);

void __sgdt(void *Destination);

void __lldt(unsigned short Value);

void __sldt(void *Destination);

void __ltr(unsigned short Source);

void __str(unsigned short *Destination);


#else
#error Unknown compiler for inline assembler
#endif

#endif

/* EOF */
