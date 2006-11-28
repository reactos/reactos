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

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DEPRECATED ****************************************************************/

#include <../hal/halx86/include/halirq.h>
#include <../hal/halx86/include/mps.h>

void irq_handler_0(void);
void irq_handler_1(void);
void irq_handler_2(void);
void irq_handler_3(void);
void irq_handler_4(void);
void irq_handler_5(void);
void irq_handler_6(void);
void irq_handler_7(void);
void irq_handler_8(void);
void irq_handler_9(void);
void irq_handler_10(void);
void irq_handler_11(void);
void irq_handler_12(void);
void irq_handler_13(void);
void irq_handler_14(void);
void irq_handler_15(void);

static unsigned int irq_handler[NR_IRQS]=
{
   (int)&irq_handler_0,
   (int)&irq_handler_1,
   (int)&irq_handler_2,
   (int)&irq_handler_3,
   (int)&irq_handler_4,
   (int)&irq_handler_5,
   (int)&irq_handler_6,
   (int)&irq_handler_7,
   (int)&irq_handler_8,
   (int)&irq_handler_9,
   (int)&irq_handler_10,
   (int)&irq_handler_11,
   (int)&irq_handler_12,
   (int)&irq_handler_13,
   (int)&irq_handler_14,
   (int)&irq_handler_15,
};

/*
 * PURPOSE: Object describing each isr
 * NOTE: The data in this table is only modified at passsive level but can
 * be accessed at any irq level.
 */

typedef struct
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK Lock;
   ULONG Count;
}
ISR_TABLE, *PISR_TABLE;

static ISR_TABLE IsrTable[NR_IRQS];

#define TAG_ISR_LOCK     TAG('I', 'S', 'R', 'L')

#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID
INIT_FUNCTION
NTAPI
KeInitInterrupts (VOID)
{
   int i;

   /*
    * Setup the IDT entries to point to the interrupt handlers
    */
   for (i=0;i<NR_IRQS;i++)
     {
        ((IDT_DESCRIPTOR*)&KiIdt[IRQ_BASE+i])->a=(irq_handler[i]&0xffff)+(KGDT_R0_CODE<<16);
        ((IDT_DESCRIPTOR*)&KiIdt[IRQ_BASE+i])->b=(irq_handler[i]&0xffff0000)+PRESENT+
                            I486_INTERRUPT_GATE;
          {
            InitializeListHead(&IsrTable[i].ListHead);
            KeInitializeSpinLock(&IsrTable[i].Lock);
            IsrTable[i].Count = 0;
          }
     }
}

VOID STDCALL
KiInterruptDispatch2 (ULONG vector, KIRQL old_level)
/*
 * FUNCTION: Calls all the interrupt handlers for a given irq.
 * ARGUMENTS:
 *        vector - The number of the vector to call handlers for.
 *        old_level - The irql of the processor when the irq took place.
 * NOTES: Must be called at DIRQL.
 */
{
  PKINTERRUPT isr;
  PLIST_ENTRY current;
  KIRQL oldlvl;
  PISR_TABLE CurrentIsr;

  /*
   * Iterate the list until one of the isr tells us its device interrupted
   */
  CurrentIsr = &IsrTable[vector - IRQ_BASE];

  KiAcquireSpinLock(&CurrentIsr->Lock);

  CurrentIsr->Count++;
  current = CurrentIsr->ListHead.Flink;

  while (current != &CurrentIsr->ListHead)
    {
      isr = CONTAINING_RECORD(current,KINTERRUPT,InterruptListEntry);
      oldlvl = KeAcquireInterruptSpinLock(isr);
      if (isr->ServiceRoutine(isr, isr->ServiceContext))
        {
          KeReleaseInterruptSpinLock(isr, oldlvl);
          break;
        }
      KeReleaseInterruptSpinLock(isr, oldlvl);
      current = current->Flink;
    }
  KiReleaseSpinLock(&CurrentIsr->Lock);
}

VOID
KiInterruptDispatch3 (ULONG vector, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         irq = IRQ that has interrupted
 */
{
   KIRQL old_level;
   PKTHREAD CurrentThread;

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
   _enable();

   ASSERT (VECTOR2IRQ(vector) != 0);

     /*
      * Actually call the ISR.
      */
     KiInterruptDispatch2(vector, old_level);

   /*
    * End the system interrupt.
    */
   _disable();

   if (old_level==PASSIVE_LEVEL && Trapframe->Cs != KGDT_R0_CODE)
     {
       HalEndSystemInterrupt (APC_LEVEL, 0);

       CurrentThread = KeGetCurrentThread();
       if (CurrentThread!=NULL && CurrentThread->ApcState.UserApcPending)
         {
           ASSERT (CurrentThread->TrapFrame);

           _enable();
           KiDeliverApc(UserMode, NULL, NULL);
           _disable();

           ASSERT(KeGetCurrentThread() == CurrentThread);
           ASSERT (CurrentThread->TrapFrame);
         }
       KeLowerIrql(PASSIVE_LEVEL);
     }
   else
     {
       HalEndSystemInterrupt (old_level, 0);
     }
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
        /* Use the spinlock given to us */
        Interrupt->ActualLock = SpinLock;
    }
    else
    {
        /* This means we'll be using the built-in one */
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
KeConnectInterrupt(IN PKINTERRUPT InterruptObject)
{
   KIRQL oldlvl,synch_oldlvl;
   PKINTERRUPT ListHead;
   ULONG Vector;
   PISR_TABLE CurrentIsr;
   BOOLEAN Result;

   DPRINT("KeConnectInterrupt()\n");

   Vector = InterruptObject->Vector;

   if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
      return FALSE;

   Vector -= IRQ_BASE;

   ASSERT (InterruptObject->Number < KeNumberProcessors);

   KeSetSystemAffinityThread(1 << InterruptObject->Number);

   CurrentIsr = &IsrTable[Vector];

   KeRaiseIrql(VECTOR2IRQL(Vector + IRQ_BASE),&oldlvl);
   KiAcquireSpinLock(&CurrentIsr->Lock);

   /*
    * Check if the vector is already in use that we can share it
    */
   if (!IsListEmpty(&CurrentIsr->ListHead))
   {
      ListHead = CONTAINING_RECORD(CurrentIsr->ListHead.Flink,KINTERRUPT,InterruptListEntry);
      if (InterruptObject->ShareVector == FALSE || ListHead->ShareVector==FALSE)
      {
         KiReleaseSpinLock(&CurrentIsr->Lock);
         KeLowerIrql(oldlvl);
         KeRevertToUserAffinityThread();
         return FALSE;
      }
   }

   synch_oldlvl = KeAcquireInterruptSpinLock(InterruptObject);

   Result = HalEnableSystemInterrupt(Vector + IRQ_BASE, InterruptObject->Irql, InterruptObject->Mode);
   if (Result)
   {
      InsertTailList(&CurrentIsr->ListHead,&InterruptObject->InterruptListEntry);
   }

   InterruptObject->Connected = TRUE;
   KeReleaseInterruptSpinLock(InterruptObject, synch_oldlvl);

   /*
    * Release the table spinlock
    */
   KiReleaseSpinLock(&CurrentIsr->Lock);
   KeLowerIrql(oldlvl);

   KeRevertToUserAffinityThread();

   return Result;
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
