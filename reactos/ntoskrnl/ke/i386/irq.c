/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         IRQ handling
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/*
 * NOTE: In general the PIC interrupt priority facilities are used to
 * preserve the NT IRQL semantics, global interrupt disables are only used
 * to keep the PIC in a consistent state
 *
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#include <../hal/halx86/include/halirq.h>
#include <../hal/halx86/include/mps.h>

#define NDEBUG
#include <internal/debug.h>

typedef enum _CONNECT_TYPE
{
    NoConnect,
    NormalConnect,
    ChainConnect,
    UnknownConnect
} CONNECT_TYPE, *PCONNECT_TYPE;

typedef struct _DISPATCH_INFO
{
    CONNECT_TYPE Type;
    PKINTERRUPT Interrupt;
    PKINTERRUPT_ROUTINE NoDispatch;
    PKINTERRUPT_ROUTINE InterruptDispatch;
    PKINTERRUPT_ROUTINE FloatingDispatch;
    PKINTERRUPT_ROUTINE ChainedDispatch;
    PKINTERRUPT_ROUTINE *FlatDispatch;
} DISPATCH_INFO, *PDISPATCH_INFO;

extern ULONG KiInterruptTemplate[KINTERRUPT_DISPATCH_CODES];
extern PULONG KiInterruptTemplateObject;
extern PULONG KiInterruptTemplateDispatch;
extern PULONG KiInterruptTemplate2ndDispatch;
extern ULONG KiUnexpectedEntrySize;

VOID
NTAPI
KiStartUnexpectedRange(VOID);

VOID
NTAPI
KiEndUnexpectedRange(VOID);

VOID
NTAPI
KiInterruptDispatch3(VOID);

VOID
NTAPI
KiChainedDispatch(VOID);

/* DEPRECATED FUNCTIONS ******************************************************/

void irq_handler_0(void);
extern IDT_DESCRIPTOR KiIdt[256];
#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID
INIT_FUNCTION
NTAPI
KeInitInterrupts (VOID)
{

        KiIdt[IRQ_BASE].a=((ULONG)irq_handler_0&0xffff)+(KGDT_R0_CODE<<16);
        KiIdt[IRQ_BASE].b=((ULONG)irq_handler_0&0xffff0000)+PRESENT+
                            I486_INTERRUPT_GATE;
}

STATIC VOID
KeIRQTrapFrameToTrapFrame(PKIRQ_TRAPFRAME IrqTrapFrame,
                          PKTRAP_FRAME TrapFrame)
{
   TrapFrame->SegGs     = (USHORT)IrqTrapFrame->Gs;
   TrapFrame->SegFs     = (USHORT)IrqTrapFrame->Fs;
   TrapFrame->SegEs     = (USHORT)IrqTrapFrame->Es;
   TrapFrame->SegDs     = (USHORT)IrqTrapFrame->Ds;
   TrapFrame->Eax    = IrqTrapFrame->Eax;
   TrapFrame->Ecx    = IrqTrapFrame->Ecx;
   TrapFrame->Edx    = IrqTrapFrame->Edx;
   TrapFrame->Ebx    = IrqTrapFrame->Ebx;
   TrapFrame->HardwareEsp    = IrqTrapFrame->Esp;
   TrapFrame->Ebp    = IrqTrapFrame->Ebp;
   TrapFrame->Esi    = IrqTrapFrame->Esi;
   TrapFrame->Edi    = IrqTrapFrame->Edi;
   TrapFrame->Eip    = IrqTrapFrame->Eip;
   TrapFrame->SegCs     = IrqTrapFrame->Cs;
   TrapFrame->EFlags = IrqTrapFrame->Eflags;
}

extern BOOLEAN KiClockSetupComplete;

VOID
KiInterruptDispatch (ULONG vector, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         irq = IRQ that has interrupted
 */
{
   KIRQL old_level;
   KTRAP_FRAME KernelTrapFrame;
   ASSERT(vector == 0x30);
#if 0
   PULONG Frame;
   DPRINT1("Received Interrupt: %lx\n", vector);
   DPRINT1("My trap frame: %p\n", Trapframe);
   DPRINT1("Stack trace\n");
   __asm__("mov %%ebp, %0" : "=r" (Frame) : );
   DPRINT1("Stack trace: %p %p %p %p\n", Frame, *Frame, Frame[1], *(PULONG)Frame[1]);
   DPRINT1("Clock setup: %lx\n", KiClockSetupComplete);
      if (KiClockSetupComplete) while(TRUE);
#endif

   /*
    * At this point we have interrupts disabled, nothing has been done to
    * the PIC.
    */
   KeGetCurrentPrcb()->InterruptCount++;

   /*
    * Notify the rest of the kernel of the raised irq level. For the
    * default HAL this will send an EOI to the PIC and alter the IRQL.
    */
   if (!HalBeginSystemInterrupt (VECTOR2IRQL(vector),
                                 vector,
                                 &old_level))
     {
       return;
     }


   /*
    * Enable interrupts
    * NOTE: Only higher priority interrupts will get through
    */
   Ke386EnableInterrupts();

       //DPRINT1("Tick\n");
   if (KiClockSetupComplete)
   {
      KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
      KeUpdateSystemTime(&KernelTrapFrame, old_level, 100000);
   }

   /*
    * End the system interrupt.
    */
   Ke386DisableInterrupts();
   HalEndSystemInterrupt (old_level, 0);
}

/* PRIVATE FUNCTIONS *********************************************************/

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
    Dispatch->InterruptDispatch = KiInterruptDispatch3;
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
STDCALL
KeInitializeInterrupt(PKINTERRUPT Interrupt,
                      PKSERVICE_ROUTINE ServiceRoutine,
                      PVOID ServiceContext,
                      PKSPIN_LOCK SpinLock,
                      ULONG Vector,
                      KIRQL Irql,
                      KIRQL SynchronizeIrql,
                      KINTERRUPT_MODE InterruptMode,
                      BOOLEAN ShareVector,
                      CHAR ProcessorNumber,
                      BOOLEAN FloatingSave)
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

    /* Loop the template in memory */
    for (i = 0; i < KINTERRUPT_DISPATCH_CODES; i++)
    {
        /* Copy the dispatch code */
        *DispatchCode++ = KiInterruptTemplate[i];
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
        DPRINT1("Invalid interrupt object\n");
        return FALSE;
    }

    /* Set defaults */
    Connected = FALSE;
    Error = FALSE;

    /* Set the system affinity and acquire the dispatcher lock */
    KeSetSystemAffinityThread(1 << Number);
    OldIrql = KeAcquireDispatcherDatabaseLock();

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
            ASSERT(FALSE); // FIXME: NOT YET SUPPORTED/TESTED
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
    KeReleaseDispatcherDatabaseLock(OldIrql);
    KeRevertToUserAffinityThread();

    /* Return to caller */
    return Connected;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeDisconnectInterrupt(PKINTERRUPT Interrupt)
{
    KIRQL OldIrql, Irql;
    ULONG Vector;
    DISPATCH_INFO Dispatch;
    PKINTERRUPT NextInterrupt;
    BOOLEAN State;

    /* Set the affinity */
    KeSetSystemAffinityThread(1 << Interrupt->Number);

    /* Lock the dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

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
    KeReleaseDispatcherDatabaseLock(OldIrql);
    KeRevertToUserAffinityThread();

    /* Return to caller */
    return State;
}

/* EOF */
