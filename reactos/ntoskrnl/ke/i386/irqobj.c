/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         Manages the Kernel's IRQ support for external drivers,
 *                  for the purposes of connecting, disconnecting and setting
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
extern ULONG NTAPI KiChainedDispatch2ndLvl(VOID);

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiGetVectorDispatch(IN ULONG Vector,
                    IN PDISPATCH_INFO Dispatch)
{
    PKINTERRUPT_ROUTINE Handler;
    PVOID Current;
    UCHAR Type;
    UCHAR Entry;

    /* Check if this is a primary or 2nd-level dispatch */
    Type = HalSystemVectorDispatchEntry(Vector,
                                        &Dispatch->FlatDispatch,
                                        &Dispatch->NoDispatch);
    ASSERT(Type == 0);

    /* Get the IDT entry for this vector */
    Entry = HalVectorToIDTEntry(Vector);

    /* Setup the unhandled dispatch */
    Dispatch->NoDispatch = (PVOID)(((ULONG_PTR)&KiStartUnexpectedRange) +
                                   (Entry - PRIMARY_VECTOR_BASE) *
                                   KiUnexpectedEntrySize);

    /* Setup the handlers */
    Dispatch->InterruptDispatch = (PVOID)KiInterruptDispatch;
    Dispatch->FloatingDispatch = NULL; // Floating Interrupts are not supported
    Dispatch->ChainedDispatch = (PVOID)KiChainedDispatch;
    Dispatch->FlatDispatch = NULL;

    /* Get the current handler */
    Current = KeQueryInterruptHandler(Vector);

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

        /* Read note in trap.s -- patching not needed since JMP is static */

        /* Now set the final handler address */
        ASSERT(Dispatch.FlatDispatch == NULL);
        Handler = (PVOID)&Interrupt->DispatchCode;
    }

    /* Register the interrupt */
    KeRegisterInterruptHandler(Interrupt->Vector, Handler);
}

FORCEINLINE
DECLSPEC_NORETURN
VOID
KiExitInterrupt(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL OldIrql,
                IN BOOLEAN Spurious)
{
    /* Check if this was a real interrupt */
    if (!Spurious)
    {
        /* It was, disable interrupts and restore the IRQL */
        _disable();
        HalEndSystemInterrupt(OldIrql, TrapFrame);
    }

    /* Now exit the trap */
    KiEoiHelper(TrapFrame);
}

DECLSPEC_NORETURN
VOID
__cdecl
KiUnexpectedInterrupt(VOID)
{
    /* Crash the machine */
    KeBugCheck(TRAP_CAUSE_UNKNOWN);
}

VOID
FASTCALL
KiUnexpectedInterruptTailHandler(IN PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

    /* Increase interrupt count */
    KeGetCurrentPrcb()->InterruptCount++;

    /* Start the interrupt */
    if (HalBeginSystemInterrupt(HIGH_LEVEL, TrapFrame->ErrCode, &OldIrql))
    {
        /* Warn user */
        DPRINT1("\n\x7\x7!!! Unexpected Interrupt 0x%02lx !!!\n", TrapFrame->ErrCode);

        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, FALSE);
    }
    else
    {
        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, TRUE);
    }
}

typedef
VOID
(FASTCALL *PKI_INTERRUPT_DISPATCH)(
    IN PKTRAP_FRAME TrapFrame,
    IN PKINTERRUPT Interrupt
);

VOID
FASTCALL
KiInterruptDispatch(IN PKTRAP_FRAME TrapFrame,
                    IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql;

    /* Increase interrupt count */
    KeGetCurrentPrcb()->InterruptCount++;

    /* Begin the interrupt, making sure it's not spurious */
    if (HalBeginSystemInterrupt(Interrupt->SynchronizeIrql,
                                Interrupt->Vector,
                                &OldIrql))
    {
        /* Acquire interrupt lock */
        KxAcquireSpinLock(Interrupt->ActualLock);

        /* Call the ISR */
        Interrupt->ServiceRoutine(Interrupt, Interrupt->ServiceContext);

        /* Release interrupt lock */
        KxReleaseSpinLock(Interrupt->ActualLock);

        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, FALSE);
    }
    else
    {
        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, TRUE);
    }
}

VOID
FASTCALL
KiChainedDispatch(IN PKTRAP_FRAME TrapFrame,
                  IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql, OldInterruptIrql = 0;
    BOOLEAN Handled;
    PLIST_ENTRY NextEntry, ListHead;

    /* Increase interrupt count */
    KeGetCurrentPrcb()->InterruptCount++;

    /* Begin the interrupt, making sure it's not spurious */
    if (HalBeginSystemInterrupt(Interrupt->Irql,
                                Interrupt->Vector,
                                &OldIrql))
    {
        /* Get list pointers */
        ListHead = &Interrupt->InterruptListEntry;
        NextEntry = ListHead; /* The head is an entry! */
        while (TRUE)
        {
            /* Check if this interrupt's IRQL is higher than the current one */
            if (Interrupt->SynchronizeIrql > Interrupt->Irql)
            {
                /* Raise to higher IRQL */
                OldInterruptIrql = KfRaiseIrql(Interrupt->SynchronizeIrql);
            }

            /* Acquire interrupt lock */
            KxAcquireSpinLock(Interrupt->ActualLock);

            /* Call the ISR */
            Handled = Interrupt->ServiceRoutine(Interrupt,
                                                Interrupt->ServiceContext);

            /* Release interrupt lock */
            KxReleaseSpinLock(Interrupt->ActualLock);

            /* Check if this interrupt's IRQL is higher than the current one */
            if (Interrupt->SynchronizeIrql > Interrupt->Irql)
            {
                /* Lower the IRQL back */
                ASSERT(OldInterruptIrql == Interrupt->Irql);
                KfLowerIrql(OldInterruptIrql);
            }

            /* Check if the interrupt got handled and it's level */
            if ((Handled) && (Interrupt->Mode == LevelSensitive)) break;

            /* What's next? */
            NextEntry = NextEntry->Flink;

            /* Is this the last one? */
            if (NextEntry == ListHead)
            {
                /* Level should not have gotten here */
                if (Interrupt->Mode == LevelSensitive) break;

                /* As for edge, we can only exit once nobody can handle the interrupt */
                if (!Handled) break;
            }

            /* Get the interrupt object for the next pass */
            Interrupt = CONTAINING_RECORD(NextEntry, KINTERRUPT, InterruptListEntry);
        }

        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, FALSE);
    }
    else
    {
        /* Now call the epilogue code */
        KiExitInterrupt(TrapFrame, OldIrql, TRUE);
    }
}

VOID
FASTCALL
KiInterruptTemplateHandler(IN PKTRAP_FRAME TrapFrame,
                           IN PKINTERRUPT Interrupt)
{
    /* Enter interrupt frame */
    KiEnterInterruptTrap(TrapFrame);

    /* Call the correct dispatcher */
    ((PKI_INTERRUPT_DISPATCH)Interrupt->DispatchAddress)(TrapFrame, Interrupt);
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
    Interrupt->TickCount = MAXULONG;
    Interrupt->DispatchCount = MAXULONG;

    /* Loop the template in memory */
    for (i = 0; i < DISPATCH_LENGTH; i++)
    {
        /* Copy the dispatch code */
        *DispatchCode++ = ((PULONG)KiInterruptTemplate)[i];
    }

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
            Interrupt->Connected = Connected = TRUE;

            /*
             * Verify the IRQL for chained connect,
             */
#if defined(CONFIG_SMP)
            ASSERT(Irql <= SYNCH_LEVEL);
#elif (NTDDI_VERSION >= NTDDI_WS03)
            ASSERT(Irql <= (IPI_LEVEL - 2));
#else
            ASSERT(Irql <= (IPI_LEVEL - 1));
#endif

            /* Check if this is the first chain */
            if (Dispatch.Type != ChainConnect)
            {
                /* This is not supported */
                ASSERT(Dispatch.Interrupt->Mode != Latched);

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

/*
 * @implemented
 */
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
    KfLowerIrql(OldIrql);

    /* Return status */
    return Success;
}

/* EOF */
