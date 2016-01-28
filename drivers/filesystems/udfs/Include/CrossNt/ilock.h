#ifndef __CROSS_NT_INTERLOCKED__H__
#define __CROSS_NT_INTERLOCKED__H__

LONG
__fastcall
CrNtInterlockedIncrement_impl_i386_MP(
    IN OUT PLONG Addend
    );

LONG
__fastcall
CrNtInterlockedIncrement_impl_i386_UP(
    IN OUT PLONG Addend
    );

/********************************************************/

LONG
__fastcall
CrNtInterlockedDecrement_impl_i386_MP(
    IN OUT PLONG Addend
    );

LONG
__fastcall
CrNtInterlockedDecrement_impl_i386_UP(
    IN OUT PLONG Addend
    );

/********************************************************/

LONG
__fastcall
CrNtInterlockedExchangeAdd_impl_i386_MP(
    IN OUT PLONG Addend,
    IN LONG Increment
    );

LONG
__fastcall
CrNtInterlockedExchangeAdd_impl_i386_UP(
    IN OUT PLONG Addend,
    IN LONG Increment
    );

/********************************************************/

PVOID
__fastcall
CrNtInterlockedCompareExchange_impl_i386_MP(
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    );

PVOID
__fastcall
CrNtInterlockedCompareExchange_impl_i386_UP(
    IN OUT PVOID *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    );

#endif __CROSS_NT_INTERLOCKED__H__
