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
extern UCHAR KiUnexpectedRange[];
extern UCHAR KiUnexpectedRangeEnd[];
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

    ASSERT(Interrupt->Vector <= MAXIMUM_IDTVECTOR);
    ASSERT(Interrupt->Number < KeNumberProcessors);
    ASSERT(Interrupt->Irql <= HIGH_LEVEL);
    ASSERT(Interrupt->SynchronizeIrql >= Interrupt->Irql);
    ASSERT(Interrupt->Irql == (Interrupt->Vector >> 4));

    /* Make sure the vector is within the allowed range */
    if (Interrupt->Vector < PRIMARY_VECTOR_BASE)
    {
        DPRINT1("FIXME: KeConnectInterrupt() with invalid vector 0x%lx!\n", Interrupt->Vector);
        //__debugbreak();
        //return FALSE;
    }

    /* Check if its already connected */
    if (Interrupt->Connected) return TRUE;

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

        if (!HalEnableSystemInterrupt(Interrupt->Vector,
                                      Interrupt->Irql,
                                      Interrupt->Mode))
        {
            /* Didn't work, restore old handler */
            DPRINT1("HalEnableSystemInterrupt failed\n");
            KeRegisterInterruptHandler(Interrupt->Vector, CurrentHandler);
            return FALSE;
        }

        /* Mark as connected */
        Interrupt->Connected = TRUE;
    }
    else
    {
        // later
        //__debugbreak();
        DPRINT1("FIXME: KeConnectInterrupt() with used vector 0x%lx!\n", Interrupt->Vector);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
KeDisconnectInterrupt(IN PKINTERRUPT Interrupt)
{
    /* If the interrupt wasn't connected, there's nothing to do */
    if (!Interrupt->Connected)
    {
        return FALSE;
    }

    UNIMPLEMENTED;
    __debugbreak();
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
