/*++
Copyright (c) 1997-8  Microsoft Corporation

Module Name:

    pat.c

Abstract:

    This module implements interfaces that set the Page Attribute
    Table. These entry points only exist on i386 machines.

Author:

    Shivnandan Kaushik (Intel Corp.)

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "pat.h"

#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif

// Structure used for PAT initialization

typedef struct _NEW_PAT {

    PAT                 Attributes;

    // IPI context to coordinate concurrent PAT update

    ULONG               Processor;
    volatile ULONG      TargetCount;
    volatile ULONG      *TargetPhase;

} NEW_PAT, *PNEW_PAT;

// Prototypes

VOID
KeRestorePAT (
    VOID
    );

VOID
KiInitializePAT (
    VOID
    );

VOID
KiLoadPAT (
    IN PNEW_PAT Context
    );

VOID
KiLoadPATTarget (
    IN PKIPI_CONTEXT    SignalDone,
    IN PVOID            Context,
    IN PVOID            Parameter2,
    IN PVOID            Parameter3
    );

VOID
KiSynchronizePATLoad (
    IN PNEW_PAT   Context
    );

#if DBG
VOID
KiDumpPAT (
    PUCHAR      DebugString,
    PAT         Attributes
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK,KiInitializePAT)
#pragma alloc_text(PAGELK,KiLoadPAT)
#pragma alloc_text(PAGELK,KiLoadPATTarget)
#pragma alloc_text(PAGELK,KiSynchronizePATLoad)
#endif

VOID
KeRestorePAT (
    VOID
    )
/*++
Routine Description:

    Reinitialize the Page Attribute Table (PAT) on all processors.

    N.B. The caller must have the PAGELK code locked

  Arguments:

    None.

Return Value:

    None.
--*/
{
    if (KeFeatureBits & KF_PAT) {
        KiInitializePAT();
    }
}

VOID
KiInitializePAT (
    VOID
    )
/*++

Routine Description:

    Initialize the Page Attribute Table (PAT) on all processors. PAT
    is setup to provide WB, WC, STRONG_UC and WEAK_UC as the memory
    types such that mm macros for enabling/disabling/querying caching
    (MI_DISABLE_CACHING, MI_ENABLE_CACHING and MI_IS_CACHING_ENABLED)
    are unaffected.

    PAT_Entry   PAT Index   PCD PWT     Memory Type
    0            0           0   0       WB
    1            0           0   1       WC *
    2            0           1   0       WEAK_UC
    3            0           1   1       STRONG_UC
    4            1           0   0       WB
    5            1           0   1       WC *
    6            1           1   0       WEAK_UC
    7            1           1   1       STRONG_UC

    N.B. The caller must have the PAGELK code locked and ensure that the
    PAT feature is supported.

  Arguments:

    None.

Return Value:

    None.

--*/
{
    PAT         PatAttributes;
    ULONG       Size;
    KIRQL       OldIrql, NewIrql;
    PKPRCB      Prcb;
    NEW_PAT     NewPAT;
    KAFFINITY   TargetProcessors;

    ASSERT ((KeFeatureBits & KF_PAT) != 0);

    //
    // Initialize the PAT
    //

    PatAttributes.hw.Pat[0] = PAT_TYPE_WB;
    PatAttributes.hw.Pat[1] = PAT_TYPE_USWC;
    PatAttributes.hw.Pat[2] = PAT_TYPE_WEAK_UC;
    PatAttributes.hw.Pat[3] = PAT_TYPE_STRONG_UC;
    PatAttributes.hw.Pat[4] = PAT_TYPE_WB;
    PatAttributes.hw.Pat[5] = PAT_TYPE_USWC;
    PatAttributes.hw.Pat[6] = PAT_TYPE_WEAK_UC;
    PatAttributes.hw.Pat[7] = PAT_TYPE_STRONG_UC;

    //
    // Synchronize with other IPI functions which may stall
    //

    KiLockContextSwap(&OldIrql);

    Prcb = KeGetCurrentPrcb();

    NewPAT.Attributes = PatAttributes;
    NewPAT.TargetCount = 0;
    NewPAT.TargetPhase = &Prcb->ReverseStall;
    NewPAT.Processor = Prcb->Number;


#if !defined(NT_UP)

    //
    // Collect all the (other) processors
    //

    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {

        KiIpiSendSynchronousPacket (
            Prcb,
            TargetProcessors,
            KiLoadPATTarget,
            (PVOID) (&NewPAT),
            NULL,
            NULL
            );

        //
        // Wait for all processors to be collected
        //

        KiIpiStallOnPacketTargets(TargetProcessors);

        //
        // All processors are now waiting.  Raise to high level to
        // ensure this processor doesn't enter the debugger due to
        // some interrupt service routine.
        //

        KeRaiseIrql (HIGH_LEVEL, &NewIrql);

        //
        // There's no reason for any debug events now, so signal
        // the other processors that they can all begin the PAT update
        //

        Prcb->ReverseStall += 1;
    }

#endif

    //
    // Update PAT
    //

    KiLoadPAT(&NewPAT);

    //
    // Release ContextSwap lock and lower to initial irql
    //

    KiUnlockContextSwap(OldIrql);
    MmEnablePAT();
    return;
}

VOID
KiLoadPATTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID NewPAT,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )
/*++

Routine Description:

    Synchronize with target processors prior to PAT modification.

Arguments:

    Context     - Context which includes the PAT to load

Return Value:

    None

--*/
{
    PNEW_PAT Context;

    Context = (PNEW_PAT) NewPAT;

    //
    // Wait for all processors to be ready
    //

    KiIpiSignalPacketDoneAndStall (SignalDone, Context->TargetPhase);

    //
    // Update PAT
    //

    KiLoadPAT (Context);
}

VOID
KiLoadPAT (
    IN PNEW_PAT Context
    )
/*++

Routine Description:

    This function loads the PAT to all processors.

Arguments:

    Context - Context which includes new PAT to load

Return Value:

    PAT on all processors programmed to new values

--*/
{
    BOOLEAN             Enable;
    ULONG               HldCr0, HldCr4, Index;

    //
    // Disable interrupts
    //

    Enable = KiDisableInterrupts();

    //
    // Synchronize all processors
    //

    KiSynchronizePATLoad (Context);

    _asm {
        ;
        ; Get current CR0
        ;

        mov     eax, cr0
        mov     HldCr0, eax

        ;
        ; Disable caching & line fill
        ;

        and     eax, not CR0_NW
        or      eax, CR0_CD
        mov     cr0, eax

        ;
        ; Flush caches
        ;

        ;
        ; wbinvd
        ;

        _emit 0Fh
        _emit 09h

        ;
        ; Get current cr4
        ;

        _emit  0Fh
        _emit  20h
        _emit  0E0h             ; mov eax, cr4
        mov     HldCr4, eax

        ;
        ; Disable global page
        ;

        and     eax, not CR4_PGE
        _emit  0Fh
        _emit  22h
        _emit  0E0h             ; mov cr4, eax

        ;
        ; Flush TLB
        ;

        mov     eax, cr3
        mov     cr3, eax
    }

    //
    // Synchronize all processors
    //

    KiSynchronizePATLoad (Context);

    //
    // Load new PAT
    //

    WRMSR (PAT_MSR, Context->Attributes.QuadPart);

    //
    // Synchronize all processors
    //

    KiSynchronizePATLoad (Context);

    _asm {

        ;
        ; Flush caches.
        ;

        ;
        ; wbinvd
        ;

        _emit 0Fh
        _emit 09h

        ;
        ; Flush TLBs
        ;

        mov     eax, cr3
        mov     cr3, eax
    }

    //
    // Synchronize all processors
    //

    KiSynchronizePATLoad (Context);

    _asm {
        ;
        ; Restore CR4 (global page enable)
        ;

        mov     eax, HldCr4
        _emit  0Fh
        _emit  22h
        _emit  0E0h             ; mov cr4, eax

        ;
        ; Restore CR0 (cache enable)
        ;

        mov     eax, HldCr0
        mov     cr0, eax
    }

    //
    // Restore interrupts and return
    //

    KiRestoreInterrupts (Enable);
}

VOID
KiSynchronizePATLoad (
    IN PNEW_PAT   Context
    )
/*++

Routine Description:

    This function synchronizes all processors during the various phases
    of modifying the PAT.

Arguments:

    Context - Context which includes the barrier synchronization counter.

Return Value:

    PAT on all processors programmed to new values

--*/
{
    ULONG               CurrentPhase;
    volatile ULONG      *TargetPhase;
    PKPRCB              Prcb;

    TargetPhase = Context->TargetPhase;
    Prcb = KeGetCurrentPrcb();

    if (Prcb->Number == (CCHAR) Context->Processor) {

        //
        // Wait for all processors to signal
        //

        while (Context->TargetCount != (ULONG) KeNumberProcessors - 1) ;

        //
        // Reset count for next time
        //

        Context->TargetCount = 0;

        //
        // Let waiting processor go to next synchronization point
        //

        InterlockedIncrement ((PULONG) TargetPhase);


    } else {

        //
        // Get current phase
        //

        CurrentPhase = *TargetPhase;

        //
        // Signal that we have completed the current phase
        //

        InterlockedIncrement ((PULONG) &Context->TargetCount);

        //
        // Wait for new phase to begin
        //

        while (*TargetPhase == CurrentPhase) ;
    }
}
