#pragma once

#if defined(__GNUC__)

#define Ke386SetGlobalDescriptorTable(X) \
    __asm__("lgdt %0\n\t" \
    : /* no outputs */ \
    : "m" (*X));

#define Ke386GetGlobalDescriptorTable(X) \
    __asm__("sgdt %0\n\t" \
    : "=m" (*X) \
    : /* no input */ \
    : "memory");

FORCEINLINE
VOID
Ke386FxStore(IN PFX_SAVE_AREA SaveArea)
{
    asm volatile ("fxrstor (%0)" : : "r"(SaveArea));
}

FORCEINLINE
VOID
Ke386FxSave(IN PFX_SAVE_AREA SaveArea)
{
    asm volatile ("fxsave (%0)" : : "r"(SaveArea));
}


FORCEINLINE
VOID
Ke386FnSave(IN PFLOATING_SAVE_AREA SaveArea)
{
    asm volatile ("fnsave (%0); wait" : : "r"(SaveArea));
}

FORCEINLINE
VOID
Ke386SaveFpuState(IN PFX_SAVE_AREA SaveArea)
{
    extern ULONG KeI386FxsrPresent;
    if (KeI386FxsrPresent)
    {
        __asm__ __volatile__ ("fxsave %0\n" : : "m"(*SaveArea));
    }
    else
    {
        __asm__ __volatile__ ("fnsave %0\n wait\n" : : "m"(*SaveArea));
    }
}

FORCEINLINE
USHORT
Ke386GetLocalDescriptorTable()
{
    USHORT Ldt;
    __asm__("sldt %0\n\t"
    : "=m" (Ldt)
    : /* no input */
    : "memory");
    return Ldt;
}

#define Ke386SetLocalDescriptorTable(X) \
    __asm__("lldt %w0\n\t" \
    : /* no outputs */ \
    : "q" (X));

#define Ke386SetTr(X)                   __asm__ __volatile__("ltr %%ax" : :"a" (X));

FORCEINLINE
USHORT
Ke386GetTr(VOID)
{
    USHORT Tr;
    __asm__("str %0\n\t"
    : "=m" (Tr));
    return Tr;
}

#define _Ke386GetSeg(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                  })

#define _Ke386SetSeg(N,X)         __asm__ __volatile__("movl %0,%%" #N : :"r" (X));

#define Ke386FnInit()               __asm__("fninit\n\t");
#define Ke386ClearDirectionFlag()   __asm__ __volatile__ ("cld")


//
// CR Macros
//
#define Ke386SetCr2(X)              __asm__ __volatile__("movl %0,%%cr2" : :"r" (X));

//
// Segment Macros
//
#define Ke386GetSs()                _Ke386GetSeg(ss)
#define Ke386GetFs()                _Ke386GetSeg(fs)
#define Ke386GetDs()                _Ke386GetSeg(ds)
#define Ke386GetEs()                _Ke386GetSeg(es)
#define Ke386GetGs()                _Ke386GetSeg(gs)
#define Ke386SetFs(X)               _Ke386SetSeg(fs, X)
#define Ke386SetDs(X)               _Ke386SetSeg(ds, X)
#define Ke386SetEs(X)               _Ke386SetSeg(es, X)
#define Ke386SetSs(X)               _Ke386SetSeg(ss, X)
#define Ke386SetGs(X)               _Ke386SetSeg(gs, X)

#elif defined(_MSC_VER)

FORCEINLINE
VOID
Ke386FnInit(VOID)
{
    __asm fninit;
}

FORCEINLINE
VOID
__sgdt(OUT PVOID Descriptor)
{
    __asm
    {
        mov eax, Descriptor
        sgdt [eax]
    }
}

FORCEINLINE
VOID
__fxsave(OUT PFX_SAVE_AREA SaveArea)
{
    __asm mov eax, SaveArea
    __asm fxsave [eax]
}

FORCEINLINE
VOID
__fxrstor(IN PFX_SAVE_AREA SaveArea)
{
    __asm mov eax, SaveArea
    __asm fxrstor [eax]
}

FORCEINLINE
VOID
__fnsave(OUT PFLOATING_SAVE_AREA SaveArea)
{
    __asm mov eax, SaveArea
    __asm fnsave [eax]
    __asm wait;
}

#define Ke386GetGlobalDescriptorTable __sgdt

FORCEINLINE
VOID
__lgdt(IN PVOID Descriptor)
{
    __asm
    {
        mov eax, Descriptor
        lgdt [eax]
    }
}
#define Ke386SetGlobalDescriptorTable __lgdt

FORCEINLINE
USHORT
Ke386GetLocalDescriptorTable(VOID)
{
    __asm sldt ax;
}

FORCEINLINE
VOID
Ke386SetLocalDescriptorTable(IN USHORT Descriptor)
{
    __asm lldt Descriptor;
}

FORCEINLINE
VOID
Ke386SetTr(IN USHORT Tr)
{
    __asm ltr Tr;
}

FORCEINLINE
USHORT
Ke386GetTr(VOID)
{
    __asm str ax;
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

FORCEINLINE
VOID
Ke386SetGs(IN USHORT Value)
{
    __asm mov ax, Value;
    __asm mov gs, ax;
}

extern ULONG KeI386FxsrPresent;

FORCEINLINE
VOID
Ke386SaveFpuState(IN PVOID SaveArea)
{
    if (KeI386FxsrPresent)
    {
        __fxsave(SaveArea);
    }
    else
    {
        __fnsave(SaveArea);
    }
}

#define Ke386FnSave __fnsave
#define Ke386FxSave __fxsave
// The name suggest, that the original author didn't understand what frstor means
#define Ke386FxStore __fxrstor


#else
#error Unknown compiler for inline assembler
#endif

/* EOF */
