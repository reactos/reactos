/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/interrupt.c
 * PURPOSE:         Manages the Kernel's IRQ support for external drivers,
 *                  for the purpopses of connecting, disconnecting and setting
 *                  up ISRs for drivers. The backend behind the Io* Interrupt
 *                  routines.
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern UCHAR KiInterruptDispatchTemplate[16];
extern KI_INTERRUPT_DISPATCH_ENTRY KiUnexpectedRange[256];
extern KI_INTERRUPT_DISPATCH_ENTRY KiUnexpectedRangeEnd[];
void KiInterruptDispatch(void);


/* FUNCTIONS ****************************************************************/

VOID
NTAPI
KeInitializeInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN CHAR ProcessorNumber,
    IN BOOLEAN FloatingSave)
{

    /* Initialize the header */
    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    /* If no Spinlock is given, use the internal */
    if (!SpinLock) SpinLock = &Interrupt->SpinLock;
    KeInitializeSpinLock(&Interrupt->SpinLock);

    /* Set the given parameters */
    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;
    Interrupt->ActualLock = SpinLock;
    Interrupt->Vector = Vector;
    Interrupt->Irql = Irql;
    Interrupt->SynchronizeIrql = SynchronizeIrql;
    Interrupt->Mode = InterruptMode;
    Interrupt->ShareVector = ShareVector;
    Interrupt->Number = ProcessorNumber;
    Interrupt->FloatingSave = FloatingSave;

    /* Set initial values */
    Interrupt->TickCount = 0;
    Interrupt->Connected = FALSE;
    Interrupt->ServiceCount = 0;
    Interrupt->DispatchCount = 0;
    Interrupt->TrapFrame = NULL;
    Interrupt->Reserved = 0;

    /* Copy the dispatch code (its location independent, no need to patch it) */
    RtlCopyMemory(Interrupt->DispatchCode,
                  KiInterruptDispatchTemplate,
                  sizeof(Interrupt->DispatchCode));

    Interrupt->DispatchAddress = 0;
}

BOOLEAN
NTAPI
KeConnectInterrupt(IN PKINTERRUPT Interrupt)
{
    PVOID CurrentHandler;
    PKINTERRUPT ConnectedInterrupt;
    KIRQL OldIrql;

    ASSERT(Interrupt->Vector >= PRIMARY_VECTOR_BASE);
    ASSERT(Interrupt->Vector <= MAXIMUM_IDTVECTOR);
    ASSERT(Interrupt->Number < KeNumberProcessors);
    ASSERT(Interrupt->Irql <= HIGH_LEVEL);
    ASSERT(Interrupt->SynchronizeIrql >= Interrupt->Irql);
    ASSERT(Interrupt->Irql == (Interrupt->Vector >> 4));

    /* Check if its already connected */
    if (Interrupt->Connected) return TRUE;

    /* Set the system affinity and acquire the dispatcher lock */
    KeSetSystemAffinityThread(1ULL << Interrupt->Number);
    OldIrql = KiAcquireDispatcherLock();

    /* Query the current handler */
    CurrentHandler = KeQueryInterruptHandler(Interrupt->Vector);

    /* Check if the vector is unused */
    if ((CurrentHandler >= (PVOID)KiUnexpectedRange) &&
        (CurrentHandler <= (PVOID)KiUnexpectedRangeEnd))
    {
        /* Initialize the list for chained interrupts */
        InitializeListHead(&Interrupt->InterruptListEntry);

        /* Set normal dispatch address */
        Interrupt->DispatchAddress = KiInterruptDispatch;

        /* Set the new handler */
        KeRegisterInterruptHandler(Interrupt->Vector,
                                   Interrupt->DispatchCode);

        /* Enable the interrupt */
        if (!HalEnableSystemInterrupt(Interrupt->Vector,
                                      Interrupt->Irql,
                                      Interrupt->Mode))
        {
            /* Didn't work, restore old handler */
            DPRINT1("HalEnableSystemInterrupt failed\n");
            KeRegisterInterruptHandler(Interrupt->Vector, CurrentHandler);
            goto Cleanup;
        }
    }
    else
    {
        /* Get the connected interrupt */
        ConnectedInterrupt = CONTAINING_RECORD(CurrentHandler, KINTERRUPT, DispatchCode);

        /* Check if sharing is ok */
        if ((Interrupt->ShareVector == 0) ||
            (ConnectedInterrupt->ShareVector == 0) ||
            (Interrupt->Mode != ConnectedInterrupt->Mode))
        {
            goto Cleanup;
        }

        /* Insert the new interrupt into the connected interrupt's list */
        InsertTailList(&ConnectedInterrupt->InterruptListEntry,
                       &Interrupt->InterruptListEntry);
    }

    /* Mark as connected */
    Interrupt->Connected = TRUE;

Cleanup:
    /* Release the dispatcher lock and restore the thread affinity */
    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();
    return Interrupt->Connected;
}

BOOLEAN
NTAPI
KeDisconnectInterrupt(IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql;
    PVOID VectorHandler, UnexpectedHandler;
    PKINTERRUPT VectorFirstInterrupt, NextInterrupt;
    PLIST_ENTRY HandlerHead;

    /* Set the system affinity and acquire the dispatcher lock */
    KeSetSystemAffinityThread(1ULL << Interrupt->Number);
    OldIrql = KiAcquireDispatcherLock();

    /* Check if the interrupt was connected - otherwise there's nothing to do */
    if (Interrupt->Connected)
    {
        /* Get the handler for this interrupt vector */
        VectorHandler = KeQueryInterruptHandler(Interrupt->Vector);

        /* Get the first interrupt for this handler */
        VectorFirstInterrupt = CONTAINING_RECORD(VectorHandler, KINTERRUPT, DispatchCode);

        /* The first interrupt list entry is the interrupt list head */
        HandlerHead = &VectorFirstInterrupt->InterruptListEntry;

        /* If the list is empty, this is the only interrupt for this vector */
        if (IsListEmpty(HandlerHead))
        {
            /* If the list is empty, and the head is not from this interrupt,
             * this interrupt is somehow incorrectly connected */
            ASSERT(VectorFirstInterrupt == Interrupt);

            UnexpectedHandler = &KiUnexpectedRange[Interrupt->Vector]._Op_push;

            /* This is the only interrupt, the handler can be disconnected */
            HalDisableSystemInterrupt(Interrupt->Vector, Interrupt->Irql);
            KeRegisterInterruptHandler(Interrupt->Vector, UnexpectedHandler);
        }
        /* If the interrupt to be disconnected is the list head, but some others follow */
        else if (VectorFirstInterrupt == Interrupt)
        {
            /* Relocate the head to the next element */
            HandlerHead = HandlerHead->Flink;
            RemoveTailList(HandlerHead);

            /* Get the next interrupt from the list head */
            NextInterrupt = CONTAINING_RECORD(HandlerHead,
                                              KINTERRUPT,
                                              InterruptListEntry);

            /* Set the next interrupt as the handler for this vector */
            KeRegisterInterruptHandler(Interrupt->Vector,
                                       NextInterrupt->DispatchCode);
        }
        /* If the interrupt to be disconnected is not the list head */
        else
        {
            /* Remove the to be disconnected interrupt from the interrupt list */
            RemoveEntryList(&Interrupt->InterruptListEntry);
        }

        /* Mark as not connected */
        Interrupt->Connected = FALSE;
    }

    /* Release the dispatcher lock and restore the thread affinity */
    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();

    return TRUE;
}

BOOLEAN
NTAPI
KeSynchronizeExecution(IN OUT PKINTERRUPT Interrupt,
                       IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                       IN PVOID SynchronizeContext OPTIONAL)
{
    BOOLEAN Success;
    KIRQL OldIrql;

    /* Raise IRQL */
    OldIrql = KfRaiseIrql(Interrupt->SynchronizeIrql);

    /* Acquire interrupt spinlock */
    KeAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);

    /* Call the routine */
    Success = SynchronizeRoutine(SynchronizeContext);

    /* Release lock */
    KeReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);

    /* Lower IRQL */
    KeLowerIrql(OldIrql);

    /* Return status */
    return Success;
}
