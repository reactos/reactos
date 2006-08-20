/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/fpu.c
 * PURPOSE:         Handles the FPU
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Gregor Anich
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/* This is a rather naive implementation of Ke(Save/Restore)FloatingPointState
   which will not work for WDM drivers. Please feel free to improve */

NTSTATUS STDCALL
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState;

    ASSERT_IRQL(DISPATCH_LEVEL);

    /* check if we are doing software emulation */
    if (!KeI386NpxPresent)
    {
        return STATUS_ILLEGAL_FLOAT_CONTEXT;
    }

    FpState = ExAllocatePool(NonPagedPool, sizeof (FNSAVE_FORMAT));
    if (NULL == FpState)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *((PVOID *) Save) = FpState;

#if defined(__GNUC__)
    asm volatile("fnsave %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
    __asm mov eax, FpState;
    __asm fsave [eax];
#else
#error Unknown compiler for inline assembler
#endif

    KeGetCurrentThread()->DispatcherHeader.NpxIrql = KeGetCurrentIrql();

    return STATUS_SUCCESS;
}


NTSTATUS STDCALL
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    PFNSAVE_FORMAT FpState = *((PVOID *) Save);

    if (KeGetCurrentThread()->DispatcherHeader.NpxIrql != KeGetCurrentIrql())
    {
        KEBUGCHECK(UNDEFINED_BUG_CODE);
    }

#if defined(__GNUC__)
    asm volatile("fnclex\n\t");
    asm volatile("frstor %0\n\t" : "=m" (*FpState));
#elif defined(_MSC_VER)
    __asm mov eax, FpState;
    __asm frstor [eax];
#else
#error Unknown compiler for inline assembler
#endif

    ExFreePool(FpState);

    return STATUS_SUCCESS;
}
