/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         Manages the Kernel's IRQ support for external drivers,
 *                  for the purpopses of connecting, disconnecting and setting
 *                  up ISRs for drivers. The backend behind the Io* Interrupt
 *                  routines.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KiISRTimeout = 55;
USHORT KiISROverflow = 30000;
extern ULONG KiChainedDispatch2ndLvl;

/* PRIVATE FUNCTIONS *********************************************************/

BOOLEAN
NTAPI
KeDisableInterrupts(VOID)
{
    ULONG Flags = 0;
    BOOLEAN Return;

    /* Get EFLAGS and check if the interrupt bit is set */
    Ke386SaveFlags(Flags);
    Return = (Flags & EFLAGS_INTERRUPT_MASK) ? TRUE: FALSE;

    /* Disable interrupts */
    _disable();
    return Return;
}

VOID
NTAPI
KiGetVectorDispatch(IN ULONG Vector,
                    IN PDISPATCH_INFO Dispatch)
{
    PKINTERRUPT_ROUTINE Handler;
    ULONG Current;

    /* Setup the unhandled dispatch */
    Dispatch->NoDispatch = (PVOID)(((ULONG_PTR)&KiStartUnexpectedRange) +
                                   (Vector - PRIMARY_VECTOR_BASE) *
                                   KiUnexpectedEntrySize);

    /* Setup the handlers */
    Dispatch->InterruptDispatch = KiInterruptDispatch;
    Dispatch->FloatingDispatch = NULL; // Floating Interrupts are not supported
    Dispatch->ChainedDispatch = KiChainedDispatch;
    Dispatch->FlatDispatch = NULL;

    /* Get the current handler */
    Current = ((((PKIPCR)KeGetPcr())->IDT[Vector].ExtendedOffset << 16)
               & 0xFFFF0000) |
              (((PKIPCR)KeGetPcr())->IDT[Vector].Offset & 0xFFFF);

    /* Set the interrupt */
    Dispatch->Interrupt = CONTAINING_RECORD(Current,
                                            KINTERRUPT,
                                            DispatchCode);

    /* Check what this interrupt is connected to */
    if ((PKINTERRUPT_ROUTINE)Current == Dispatch->NoDispatch)
    {
        /* Not connected */
        Dispatch->Type = NoConnect;
    }
    else
    {
        /* Get the handler */
        Handler = Dispatch->Interrupt->DispatchAddress;
        if (Handler == Dispatch->ChainedDispatch)
        {
            /* It's a chained interrupt */
            Dispatch->Type = ChainConnect;
        }
        else if ((Handler == Dispatch->InterruptDispatch) ||
                 (Handler == Dispatch->FloatingDispatch))
        {
            /* It's unchained */
            Dispatch->Type = NormalConnect;
        }
        else
        {
            /* Unknown */
            Dispatch->Type = UnknownConnect;
        }
    }
}

VOID
NTAPI
KiConnectVectorToInterrupt(IN PKINTERRUPT Interrupt,
                           IN CONNECT_TYPE Type)
{
    DISPATCH_INFO Dispatch;
    PKINTERRUPT_ROUTINE Handler;
    PULONG Patch = &Interrupt->DispatchCode[0];

    /* Get vector data */
    KiGetVectorDispatch(Interrupt->Vector, &Dispatch);

    /* Check if we're only disconnecting */
    if (Type == NoConnect)
    {
        /* Set the handler to NoDispatch */
        Handler = Dispatch.NoDispatch;
    }
    else
    {
        /* Get the right handler */
        Handler = (Type == NormalConnect) ?
                  Dispatch.InterruptDispatch:
                  Dispatch.ChainedDispatch;
        ASSERT(Interrupt->FloatingSave == FALSE);

        /* Set the handler */
        Interrupt->DispatchAddress = Handler;

        /* Jump to the last 4 bytes */
        Patch = (PULONG)((ULONG_PTR)Patch +
                         ((ULONG_PTR)&KiInterruptTemplateDispatch -
                          (ULONG_PTR)KiInterruptTemplate) - 4);

        /* Apply the patch */
        *Patch = (ULONG)((ULONG_PTR)Handler - ((ULONG_PTR)Patch + 4));

        /* Now set the final handler address */
        ASSERT(Dispatch.FlatDispatch == NULL);
        Handler = (PVOID)&Interrupt->DispatchCode;
    }

    /* Set the pointer in the IDT */
    ((PKIPCR)KeGetPcr())->IDT[Interrupt->Vector].ExtendedOffset =
        (USHORT)(((ULONG_PTR)Handler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[Interrupt->Vector].Offset =
        (USHORT)PtrToUlong(Handler);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeInitializeInterrupt(IN PKINTERRUPT Interrupt,
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
    ULONG i;
    PULONG DispatchCode = &Interrupt->DispatchCode[0], Patch = DispatchCode;

    /* Set the Interrupt Header */
    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    /* Check if we got a spinlock */
    if (SpinLock)
    {
        Interrupt->ActualLock = SpinLock;
    }
    else
    {
        /* This means we'll be usin the built-in one */
        KeInitializeSpinLock(&Interrupt->SpinLock);
        Interrupt->ActualLock = &Interrupt->SpinLock;
    }

    /* Set the other settings */
    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;
    Interrupt->Vector = Vector;
    Interrupt->Irql = Irql;
    Interrupt->SynchronizeIrql = SynchronizeIrql;
    Interrupt->Mode = InterruptMode;
    Interrupt->ShareVector = ShareVector;
    Interrupt->Number = ProcessorNumber;
    Interrupt->FloatingSave = FloatingSave;
    Interrupt->TickCount = (ULONG)-1;
    Interrupt->DispatchCount = (ULONG)-1;

    /* Loop the template in memory */
    for (i = 0; i < KINTERRUPT_DISPATCH_CODES; i++)
    {
        /* Copy the dispatch code */
        *DispatchCode++ = KiInterruptTemplate[i];
    }

    /* Sanity check */
    ASSERT((ULONG_PTR)&KiChainedDispatch2ndLvl -
           (ULONG_PTR)KiInterruptTemplate <= (KINTERRUPT_DISPATCH_CODES * 4));

    /* Jump to the last 4 bytes */
    Patch = (PULONG)((ULONG_PTR)Patch +
                     ((ULONG_PTR)&KiInterruptTemplateObject -
                      (ULONG_PTR)KiInterruptTemplate) - 4);

    /* Apply the patch */
    *Patch = PtrToUlong(Interrupt);

    /* Disconnect it at first */
    Interrupt->Connected = FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeConnectInterrupt(IN PKINTERRUPT Interrupt)
{
    BOOLEAN Connected, Error, Status;
    KIRQL Irql, OldIrql;
    UCHAR Number;
    ULONG Vector;
    DISPATCH_INFO Dispatch;

    /* Get data from interrupt */
    Number = Interrupt->Number;
    Vector = Interrupt->Vector;
    Irql = Interrupt->Irql;

    /* Validate the settings */
    if ((Irql > HIGH_LEVEL) ||
        (Number >= KeNumberProcessors) ||
        (Interrupt->SynchronizeIrql < Irql) ||
        (Interrupt->FloatingSave))
    {
        return FALSE;
    }

    /* Set defaults */
    Connected = FALSE;
    Error = FALSE;

    /* Set the system affinity and acquire the dispatcher lock */
    KeSetSystemAffinityThread(1 << Number);
    OldIrql = KiAcquireDispatcherLock();

    /* Check if it's already been connected */
    if (!Interrupt->Connected)
    {
        /* Get vector dispatching information */
        KiGetVectorDispatch(Vector, &Dispatch);

        /* Check if the vector is already connected */
        if (Dispatch.Type == NoConnect)
        {
            /* Do the connection */
            Interrupt->Connected = Connected = TRUE;

            /* Initialize the list */
            InitializeListHead(&Interrupt->InterruptListEntry);

            /* Connect and enable the interrupt */
            KiConnectVectorToInterrupt(Interrupt, NormalConnect);
            Status = HalEnableSystemInterrupt(Vector, Irql, Interrupt->Mode);
            if (!Status) Error = TRUE;
        }
        else if ((Dispatch.Type != UnknownConnect) &&
                (Interrupt->ShareVector) &&
                (Dispatch.Interrupt->ShareVector) &&
                (Dispatch.Interrupt->Mode == Interrupt->Mode))
        {
            /* The vector is shared and the interrupts are compatible */
            while (TRUE); // FIXME: NOT YET SUPPORTED/TESTED
            Interrupt->Connected = Connected = TRUE;
            ASSERT(Irql <= SYNCH_LEVEL);

            /* Check if this is the first chain */
            if (Dispatch.Type != ChainConnect)
            {
                /* Setup the chainned handler */
                KiConnectVectorToInterrupt(Dispatch.Interrupt, ChainConnect);
            }

            /* Insert into the interrupt list */
            InsertTailList(&Dispatch.Interrupt->InterruptListEntry,
                           &Interrupt->InterruptListEntry);
        }
    }

    /* Unlock the dispatcher and revert affinity */
    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();

    /* Check if we failed while trying to connect */
    if ((Connected) && (Error))
    {
        DPRINT1("HalEnableSystemInterrupt failed\n");
        KeDisconnectInterrupt(Interrupt);
        Connected = FALSE;
    }

    /* Return to caller */
    return Connected;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDisconnectInterrupt(IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql, Irql;
    ULONG Vector;
    DISPATCH_INFO Dispatch;
    PKINTERRUPT NextInterrupt;
    BOOLEAN State;

    /* Set the affinity */
    KeSetSystemAffinityThread(1 << Interrupt->Number);

    /* Lock the dispatcher */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if it's actually connected */
    State = Interrupt->Connected;
    if (State)
    {
        /* Get the vector and IRQL */
        Irql = Interrupt->Irql;
        Vector = Interrupt->Vector;

        /* Get vector dispatch data */
        KiGetVectorDispatch(Vector, &Dispatch);

        /* Check if it was chained */
        if (Dispatch.Type == ChainConnect)
        {
            /* Check if the top-level interrupt is being removed */
            ASSERT(Irql <= SYNCH_LEVEL);
            if (Interrupt == Dispatch.Interrupt)
            {
                /* Get the next one */
                Dispatch.Interrupt = CONTAINING_RECORD(Dispatch.Interrupt->
                                                       InterruptListEntry.Flink,
                                                       KINTERRUPT,
                                                       InterruptListEntry);

                /* Reconnect it */
                KiConnectVectorToInterrupt(Dispatch.Interrupt, ChainConnect);
            }

            /* Remove it */
            RemoveEntryList(&Interrupt->InterruptListEntry);

            /* Get the next one */
            NextInterrupt = CONTAINING_RECORD(Dispatch.Interrupt->
                                              InterruptListEntry.Flink,
                                              KINTERRUPT,
                                              InterruptListEntry);

            /* Check if this is the only one left */
            if (Dispatch.Interrupt == NextInterrupt)
            {
                /* Connect it in non-chained mode */
                KiConnectVectorToInterrupt(Dispatch.Interrupt, NormalConnect);
            }
        }
        else
        {
            /* Only one left, disable and remove it */
            HalDisableSystemInterrupt(Interrupt->Vector, Irql);
            KiConnectVectorToInterrupt(Interrupt, NoConnect);
        }

        /* Disconnect it */
        Interrupt->Connected = FALSE;
    }

    /* Unlock the dispatcher and revert affinity */
    KiReleaseDispatcherLock(OldIrql);
    KeRevertToUserAffinityThread();

    /* Return to caller */
    return State;
}

/* EOF */
