/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 interrupt objects
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*
 * The PCR keeps one entry per GIC INTID, the first connected object heads
 * the list of the ones sharing the line. SGIs belong to the kernel.
 */
#define KI_FIRST_DEVICE_VECTOR  16

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeInitializeInterrupt(
    _Out_ PKINTERRUPT Interrupt,
    _In_ PKSERVICE_ROUTINE ServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _In_opt_ PKSPIN_LOCK SpinLock,
    _In_ ULONG Vector,
    _In_ KIRQL Irql,
    _In_ KIRQL SynchronizeIrql,
    _In_ KINTERRUPT_MODE InterruptMode,
    _In_ BOOLEAN ShareVector,
    _In_ CHAR ProcessorNumber,
    _In_ BOOLEAN FloatingSave)
{
    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    KeInitializeSpinLock(&Interrupt->SpinLock);
    if (!SpinLock)
        SpinLock = &Interrupt->SpinLock;

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

    Interrupt->TickCount = 0;
    Interrupt->Connected = FALSE;
    Interrupt->ServiceCount = 0;
    Interrupt->DispatchCount = 0;
    Interrupt->TrapFrame = NULL;
    Interrupt->Reserved = 0;
    Interrupt->DispatchAddress = NULL;
    InitializeListHead(&Interrupt->InterruptListEntry);
}

BOOLEAN
NTAPI
KeConnectInterrupt(
    _Inout_ PKINTERRUPT Interrupt)
{
    PKINTERRUPT Connected;
    PKIPCR Pcr;
    KIRQL OldIrql;

    if ((Interrupt->Vector < KI_FIRST_DEVICE_VECTOR) ||
        (Interrupt->Vector >= RTL_NUMBER_OF(Pcr->Idt)))
    {
        DPRINT1("Invalid interrupt vector: %lu\n", Interrupt->Vector);
        return FALSE;
    }

    ASSERT(Interrupt->Number < KeNumberProcessors);
    ASSERT(Interrupt->Irql <= HIGH_LEVEL);
    ASSERT(Interrupt->SynchronizeIrql >= Interrupt->Irql);

    if (Interrupt->Connected)
        return TRUE;

    KeSetSystemAffinityThread(AFFINITY_MASK(Interrupt->Number));
    OldIrql = KiAcquireDispatcherLock();

    Pcr = (PKIPCR)KeGetPcr();
    Connected = Pcr->Idt[Interrupt->Vector];

    if (!Connected)
    {
        InitializeListHead(&Interrupt->InterruptListEntry);

        /* Publish the object before the line can fire */
        Pcr->Idt[Interrupt->Vector] = Interrupt;
        if (!HalEnableSystemInterrupt(Interrupt->Vector, Interrupt->Irql, Interrupt->Mode))
        {
            DPRINT1("HalEnableSystemInterrupt failed\n");
            Pcr->Idt[Interrupt->Vector] = NULL;
            goto Cleanup;
        }
    }
    else
    {
        if (!Interrupt->ShareVector ||
            !Connected->ShareVector ||
            (Interrupt->Mode != Connected->Mode))
        {
            goto Cleanup;
        }

        InsertTailList(&Connected->InterruptListEntry, &Interrupt->InterruptListEntry);
    }

    Interrupt->Connected = TRUE;

Cleanup:
    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();
    return Interrupt->Connected;
}

BOOLEAN
NTAPI
KeDisconnectInterrupt(
    _Inout_ PKINTERRUPT Interrupt)
{
    PKINTERRUPT Head, Next;
    PKIPCR Pcr;
    KIRQL OldIrql;

    KeSetSystemAffinityThread(AFFINITY_MASK(Interrupt->Number));
    OldIrql = KiAcquireDispatcherLock();

    if (Interrupt->Connected)
    {
        Pcr = (PKIPCR)KeGetPcr();
        Head = Pcr->Idt[Interrupt->Vector];

        if (IsListEmpty(&Head->InterruptListEntry))
        {
            /* The last one on the line */
            ASSERT(Head == Interrupt);
            HalDisableSystemInterrupt(Interrupt->Vector, Interrupt->Irql);
            Pcr->Idt[Interrupt->Vector] = NULL;
        }
        else if (Head == Interrupt)
        {
            /* The next object heads the list now */
            Next = CONTAINING_RECORD(Interrupt->InterruptListEntry.Flink,
                                     KINTERRUPT,
                                     InterruptListEntry);
            RemoveEntryList(&Interrupt->InterruptListEntry);
            Pcr->Idt[Interrupt->Vector] = Next;
        }
        else
        {
            RemoveEntryList(&Interrupt->InterruptListEntry);
        }

        Interrupt->Connected = FALSE;
    }

    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();
    return TRUE;
}

BOOLEAN
NTAPI
KeSynchronizeExecution(
    _Inout_ PKINTERRUPT Interrupt,
    _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    _In_opt_ PVOID SynchronizeContext)
{
    BOOLEAN Success;
    KIRQL OldIrql;

    OldIrql = KfRaiseIrql(Interrupt->SynchronizeIrql);
    KeAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);

    Success = SynchronizeRoutine(SynchronizeContext);

    KeReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);
    KeLowerIrql(OldIrql);
    return Success;
}
