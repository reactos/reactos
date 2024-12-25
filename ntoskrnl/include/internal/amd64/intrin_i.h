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

// FIXME: these should go to immintrin.h
void __cdecl _fxsave64(void *);
void __cdecl _xsave64(void *, unsigned __int64);
void __cdecl _xsaveopt64(void *, unsigned __int64);
void __cdecl _xsavec64(void *, unsigned __int64);
void __cdecl _xsaves64(void *, unsigned __int64);
void __cdecl _fxrstor64(void const *);
void __cdecl _xrstor64(void *, unsigned __int64);
void __cdecl _xrstors64(void *, unsigned __int64);
void __cdecl _xrstors64(void *, unsigned __int64);
unsigned __int64 __cdecl _xgetbv(unsigned int);
void __cdecl _xsetbv(unsigned int, unsigned __int64);

extern ULONG64 KeFeatureBits;

FORCEINLINE
VOID
KiSaveXState(
    _Out_ PVOID Buffer,
    _In_ ULONG64 ComponentMask)
{
    ULONG64 npxState = ComponentMask & ~2;
    if (KeFeatureBits & KF_XSAVES)
    {
        _xsaves64(Buffer, npxState);
    }
    else if (KeFeatureBits & KF_XSAVEOPT)
    {
        _xsaveopt64(Buffer, npxState);
    }
    else if (KeFeatureBits & KF_XSTATE)
    {
        _xsave64(Buffer, npxState);
    }
    else
    {
        _fxsave64(Buffer);
    }
}

FORCEINLINE
VOID
KiRestoreXState(
    _Inout_ PVOID Buffer,
    _In_ ULONG64 ComponentMask)
{
    ULONG64 npxState = ComponentMask & ~2;
    if (KeFeatureBits & KF_XSAVES)
    {
        _xrstors64(Buffer, npxState);
    }
    else if (KeFeatureBits & KF_XSTATE)
    {
        _xrstor64(Buffer, npxState);
    }
    else
    {
        _fxrstor64(Buffer);
    }
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

static __inline__ __attribute__((always_inline)) void __ltr(unsigned short Source)
{
	__asm__ __volatile__("ltr %0" : : "rm"(Source));
}

static __inline__ __attribute__((always_inline)) void __str(unsigned short *Destination)
{
	__asm__ __volatile__("str %0" : : "m"(*Destination) : "memory");
}

static __inline__ __attribute__((always_inline)) void __swapgs(void)
{
	__asm__ __volatile__("swapgs" : : : "memory");
}

#elif defined(_MSC_VER)

void __lgdt(void *Source);

void __sgdt(void *Destination);

void __lldt(unsigned short Value);

void __sldt(void *Destination);

void __ltr(unsigned short Source);

void __str(unsigned short *Destination);

void __swapgs(void);

#else
#error Unknown compiler for inline assembler
#endif

#endif

/* EOF */
