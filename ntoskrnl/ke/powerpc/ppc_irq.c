/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/powerpc/ppc_irq.c
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
#include <ppcmmu/mmu.h>

#define NDEBUG
#include <debug.h>

KDPC KiExpireTimerDpc;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;
extern LONG KiTickOffset;
extern ULONG KeMaximumIncrement;
extern ULONG KeMinimumIncrement;
extern ULONG KeTimeAdjustment;

extern void PearPCDebug(int ch);

/* GLOBALS *****************************************************************/

/* Interrupt handler list */

#define NR_TRAPS 16
#ifdef CONFIG_SMP

#define INT_NAME2(intnum) KiUnexpectedInterrupt##intnum

#define BUILD_INTERRUPT_HANDLER(intnum) \
VOID INT_NAME2(intnum)(VOID);

#define D(x,y) \
  BUILD_INTERRUPT_HANDLER(x##y)

#define D16(x) \
  D(x,0) D(x,1) D(x,2) D(x,3) \
  D(x,4) D(x,5) D(x,6) D(x,7) \
  D(x,8) D(x,9) D(x,A) D(x,B) \
  D(x,C) D(x,D) D(x,E) D(x,F)

D16(3) D16(4) D16(5) D16(6)
D16(7) D16(8) D16(9) D16(A)
D16(B) D16(C) D16(D) D16(E)
D16(F)

#define L(x,y) \
  (ULONG)& INT_NAME2(x##y)

#define L16(x) \
        L(x,0), L(x,1), L(x,2), L(x,3), \
        L(x,4), L(x,5), L(x,6), L(x,7), \
        L(x,8), L(x,9), L(x,A), L(x,B), \
        L(x,C), L(x,D), L(x,E), L(x,F)

static ULONG irq_handler[ROUND_UP(NR_TRAPS, 16)] = {
  L16(3), L16(4), L16(5), L16(6),
  L16(7), L16(8), L16(9), L16(A),
  L16(B), L16(C), L16(D), L16(E)
};

#undef L
#undef L16
#undef D
#undef D16

#else /* CONFIG_SMP */

void trap_handler_0(void);
void trap_handler_1(void);
void trap_handler_2(void);
void trap_handler_3(void);
void trap_handler_4(void);
void trap_handler_5(void);
void trap_handler_6(void);
void trap_handler_7(void);
void trap_handler_8(void);
void trap_handler_9(void);
void trap_handler_10(void);
void trap_handler_11(void);
void trap_handler_12(void);
void trap_handler_13(void);
void trap_handler_14(void);
void trap_handler_15(void);

static unsigned int trap_handler[NR_TRAPS] __attribute__((unused)) =
{
   (int)&trap_handler_0,
   (int)&trap_handler_1,
   (int)&trap_handler_2,
   (int)&trap_handler_3,
   (int)&trap_handler_4,
   (int)&trap_handler_5,
   (int)&trap_handler_6,
   (int)&trap_handler_7,
   (int)&trap_handler_8,
   (int)&trap_handler_9,
   (int)&trap_handler_10,
   (int)&trap_handler_11,
   (int)&trap_handler_12,
   (int)&trap_handler_13,
   (int)&trap_handler_14,
   (int)&trap_handler_15,
};

#endif /* CONFIG_SMP */

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

#ifdef CONFIG_SMP
static ISR_TABLE IsrTable[NR_TRAPS][MAXIMUM_PROCESSORS];
#else
static ISR_TABLE IsrTable[NR_TRAPS][1];
#endif

#define TAG_ISR_LOCK     'LRSI'

/* FUNCTIONS ****************************************************************/

VOID
INIT_FUNCTION
NTAPI
KeInitInterrupts (VOID)
{
   int i, j;

   /*
    * Setup the IDT entries to point to the interrupt handlers
    */
   for (i=0;i<NR_TRAPS;i++)
     {
#ifdef CONFIG_SMP
        for (j = 0; j < MAXIMUM_PROCESSORS; j++)
#else
        j = 0;
#endif
          {
            InitializeListHead(&IsrTable[i][j].ListHead);
            KeInitializeSpinLock(&IsrTable[i][j].Lock);
            IsrTable[i][j].Count = 0;
          }
     }
}

static VOID
KeIRQTrapFrameToTrapFrame(PKIRQ_TRAPFRAME IrqTrapFrame,
                          PKTRAP_FRAME TrapFrame)
{
}

static VOID
KeTrapFrameToIRQTrapFrame(PKTRAP_FRAME TrapFrame,
                          PKIRQ_TRAPFRAME IrqTrapFrame)
{
}

/*
 * NOTE: On Windows this function takes exactly one parameter and EBP is
 *       guaranteed to point to KTRAP_FRAME. The function is used only
 *       by HAL, so there's no point in keeping that prototype.
 *
 * @implemented
 */
VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME  TrapFrame,
                IN KIRQL  Irql)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD CurrentThread;
    PKPROCESS CurrentProcess;

    /* Make sure we don't go further if we're in early boot phase. */
    if (!(Prcb) || !(Prcb->CurrentThread)) return;

    /* Get the current thread and process */
    CurrentThread = Prcb->CurrentThread;
    CurrentProcess = CurrentThread->ApcState.Process;

    /* Check if we came from user mode */
    if (TrapFrame->PreviousMode != KernelMode)
    {
        /* Update user times */
        CurrentThread->UserTime++;
        InterlockedIncrement((PLONG)&CurrentProcess->UserTime);
        Prcb->UserTime++;
    }
    else
    {
        /* Check IRQ */
        if (Irql > DISPATCH_LEVEL)
        {
            /* This was an interrupt */
            Prcb->InterruptTime++;
        }
        else if ((Irql < DISPATCH_LEVEL) || !(Prcb->DpcRoutineActive))
        {
            /* This was normal kernel time */
            CurrentThread->KernelTime++;
            InterlockedIncrement((PLONG)&CurrentProcess->KernelTime);
        }
        else if (Irql == DISPATCH_LEVEL)
        {
            /* This was DPC time */
            Prcb->DpcTime++;
        }

        /* Update CPU kernel time in all cases */
        Prcb->KernelTime++;
   }

    /* Set the last DPC Count and request rate */
    Prcb->DpcLastCount = Prcb->DpcData[0].DpcCount;
    Prcb->DpcRequestRate = ((Prcb->DpcData[0].DpcCount - Prcb->DpcLastCount) +
                             Prcb->DpcRequestRate) / 2;

    /* Check if we should request a DPC */
    if ((Prcb->DpcData[0].DpcQueueDepth) && !(Prcb->DpcRoutineActive))
    {
        /* Request one */
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);

        /* Update the depth if needed */
        if ((Prcb->DpcRequestRate < KiIdealDpcRate) &&
            (Prcb->MaximumDpcQueueDepth > 1))
        {
            /* Decrease the maximum depth by one */
            Prcb->MaximumDpcQueueDepth--;
        }
    }
    else
    {
        /* Decrease the adjustment threshold */
        if (!(--Prcb->AdjustDpcThreshold))
        {
            /* We've hit 0, reset it */
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

            /* Check if we've hit queue maximum */
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
            {
                /* Increase maximum by one */
                Prcb->MaximumDpcQueueDepth++;
            }
        }
    }

   /*
    * If we're at end of quantum request software interrupt. The rest
    * is handled in KiDispatchInterrupt.
    *
    * NOTE: If one stays at DISPATCH_LEVEL for a long time the DPC routine
    * which checks for quantum end will not be executed and decrementing
    * the quantum here can result in overflow. This is not a problem since
    * we don't care about the quantum value anymore after the QuantumEnd
    * flag is set.
    */
    if ((CurrentThread->Quantum -= 3) <= 0)
    {
        Prcb->QuantumEnd = TRUE;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }
}


/*
 * NOTE: On Windows this function takes exactly zero parameters and EBP is
 *       guaranteed to point to KTRAP_FRAME. Also [esp+0] contains an IRQL.
 *       The function is used only by HAL, so there's no point in keeping
 *       that prototype.
 *
 * @implemented
 */
VOID
NTAPI
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN KIRQL Irql,
                   IN ULONG Increment)
{
    LONG OldOffset;
    LARGE_INTEGER Time;
    ASSERT(KeGetCurrentIrql() == PROFILE_LEVEL);

    /* Update interrupt time */
    Time.LowPart = SharedUserData->InterruptTime.LowPart;
    Time.HighPart = SharedUserData->InterruptTime.High1Time;
    Time.QuadPart += Increment;
    SharedUserData->InterruptTime.High2Time = Time.u.HighPart;
    SharedUserData->InterruptTime.LowPart = Time.u.LowPart;
    SharedUserData->InterruptTime.High1Time = Time.u.HighPart;

    /* Increase the tick offset */
    KiTickOffset -= Increment;
    OldOffset = KiTickOffset;

    /* Check if this isn't a tick yet */
    if (KiTickOffset > 0)
    {
        /* Expire timers */
        KeInsertQueueDpc(&KiExpireTimerDpc, 0, 0);
    }
    else
    {
        /* Setup time structure for system time */
        Time.LowPart = SharedUserData->SystemTime.LowPart;
        Time.HighPart = SharedUserData->SystemTime.High1Time;
        Time.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemTime.High2Time = Time.HighPart;
        SharedUserData->SystemTime.LowPart = Time.LowPart;
        SharedUserData->SystemTime.High1Time = Time.HighPart;

        /* Setup time structure for tick time */
        Time.LowPart = KeTickCount.LowPart;
        Time.HighPart = KeTickCount.High1Time;
        Time.QuadPart += 1;
        KeTickCount.High2Time = Time.HighPart;
        KeTickCount.LowPart = Time.LowPart;
        KeTickCount.High1Time = Time.HighPart;
        SharedUserData->TickCount.High2Time = Time.HighPart;
        SharedUserData->TickCount.LowPart = Time.LowPart;
        SharedUserData->TickCount.High1Time = Time.HighPart;

        /* Queue a DPC that will expire timers */
        KeInsertQueueDpc(&KiExpireTimerDpc, 0, 0);
    }

    /* Update process and thread times */
    if (OldOffset <= 0)
    {
        /* This was a tick, calculate the next one */
        KiTickOffset += KeMaximumIncrement;
        KeUpdateRunTime(TrapFrame, Irql);
    }
}

VOID NTAPI
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

  DPRINT("I(0x%.08x, 0x%.08x)\n", vector, old_level);

  /*
   * Iterate the list until one of the isr tells us its device interrupted
   */
  CurrentIsr = &IsrTable[vector][(ULONG)KeGetCurrentProcessorNumber()];

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
   KTRAP_FRAME KernelTrapFrame;
   PKTHREAD CurrentThread;
   PKTRAP_FRAME OldTrapFrame=NULL;

   /*
    * At this point we have interrupts disabled, nothing has been done to
    * the PIC.
    */

   KeGetCurrentPrcb()->InterruptCount++;

   /*
    * Notify the rest of the kernel of the raised irq level. For the
    * default HAL this will send an EOI to the PIC and alter the IRQL.
    */
   if (!HalBeginSystemInterrupt (vector,
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

#ifndef CONFIG_SMP
   if (vector == 0)
   {
      KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
      KeUpdateSystemTime(&KernelTrapFrame, old_level, 100000);
   }
   else
#endif
   {
     /*
      * Actually call the ISR.
      */
     KiInterruptDispatch2(vector, old_level);
   }

   /*
    * End the system interrupt.
    */
   _disable();

   if (old_level==PASSIVE_LEVEL)
     {
       HalEndSystemInterrupt (APC_LEVEL, 0);

       CurrentThread = KeGetCurrentThread();
       if (CurrentThread!=NULL && CurrentThread->ApcState.UserApcPending)
         {
           if (CurrentThread->TrapFrame == NULL)
             {
               OldTrapFrame = CurrentThread->TrapFrame;
               KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
               CurrentThread->TrapFrame = &KernelTrapFrame;
             }

	   _enable();
           KiDeliverApc(UserMode, NULL, NULL);
	   _disable();

           ASSERT(KeGetCurrentThread() == CurrentThread);
           if (CurrentThread->TrapFrame == &KernelTrapFrame)
             {
               KeTrapFrameToIRQTrapFrame(&KernelTrapFrame, Trapframe);
               CurrentThread->TrapFrame = OldTrapFrame;
             }
         }
       KeLowerIrql(PASSIVE_LEVEL);
     }
   else
     {
       HalEndSystemInterrupt (old_level, 0);
     }

}

static VOID
KeDumpIrqList(VOID)
{
   PKINTERRUPT current;
   PLIST_ENTRY current_entry;
   LONG i, j;
   KIRQL oldlvl;
   BOOLEAN printed;

   for (i=0;i<NR_TRAPS;i++)
     {
        printed = FALSE;
        KeRaiseIrql(i,&oldlvl);

        for (j=0; j < KeNumberProcessors; j++)
          {
            KiAcquireSpinLock(&IsrTable[i][j].Lock);

            current_entry = IsrTable[i][j].ListHead.Flink;
            current = CONTAINING_RECORD(current_entry,KINTERRUPT,InterruptListEntry);
            while (current_entry!=&(IsrTable[i][j].ListHead))
              {
                if (printed == FALSE)
                  {
                    printed = TRUE;
                    DPRINT("For irq %x:\n",i);
                  }
                DPRINT("   Isr %x\n",current);
                current_entry = current_entry->Flink;
                current = CONTAINING_RECORD(current_entry,KINTERRUPT,InterruptListEntry);
              }
            KiReleaseSpinLock(&IsrTable[i][j].Lock);
          }
        KeLowerIrql(oldlvl);
     }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeConnectInterrupt(PKINTERRUPT InterruptObject)
{
   KIRQL oldlvl,synch_oldlvl;
   PKINTERRUPT ListHead;
   ULONG Vector;
   PISR_TABLE CurrentIsr;
   BOOLEAN Result;

   DPRINT("KeConnectInterrupt()\n");

   Vector = InterruptObject->Vector;

   if (Vector < 0 || Vector >= NR_TRAPS)
      return FALSE;

   ASSERT (InterruptObject->Number < KeNumberProcessors);

   KeSetSystemAffinityThread(1 << InterruptObject->Number);

   CurrentIsr = &IsrTable[Vector][(ULONG)InterruptObject->Number];

   KeRaiseIrql(Vector,&oldlvl);
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

   DPRINT("%x %x\n",CurrentIsr->ListHead.Flink, CurrentIsr->ListHead.Blink);

   Result = HalEnableSystemInterrupt(Vector, InterruptObject->Irql, InterruptObject->Mode);
   if (Result)
   {
      InsertTailList(&CurrentIsr->ListHead,&InterruptObject->InterruptListEntry);
      DPRINT("%x %x\n",InterruptObject->InterruptListEntry.Flink, InterruptObject->InterruptListEntry.Blink);
   }

   InterruptObject->Connected = TRUE;
   KeReleaseInterruptSpinLock(InterruptObject, synch_oldlvl);

   /*
    * Release the table spinlock
    */
   KiReleaseSpinLock(&CurrentIsr->Lock);
   KeLowerIrql(oldlvl);

   KeDumpIrqList();

   KeRevertToUserAffinityThread();

   return Result;
}

/*
 * @implemented
 *
 * FUNCTION: Releases a drivers isr
 * ARGUMENTS:
 *        InterruptObject = isr to release
 */
BOOLEAN
NTAPI
KeDisconnectInterrupt(PKINTERRUPT InterruptObject)
{
    KIRQL oldlvl,synch_oldlvl;
    PISR_TABLE CurrentIsr;
    BOOLEAN State;

    DPRINT1("KeDisconnectInterrupt\n");
    ASSERT (InterruptObject->Number < KeNumberProcessors);

    /* Set the affinity */
    KeSetSystemAffinityThread(1 << InterruptObject->Number);

    /* Get the ISR Tabe */
    CurrentIsr = &IsrTable[InterruptObject->Vector]
                          [(ULONG)InterruptObject->Number];

    /* Raise IRQL to required level and lock table */
    KeRaiseIrql(InterruptObject->Vector,&oldlvl);
    KiAcquireSpinLock(&CurrentIsr->Lock);

    /* Check if it's actually connected */
    if ((State = InterruptObject->Connected))
    {
        /* Lock the Interrupt */
        synch_oldlvl = KeAcquireInterruptSpinLock(InterruptObject);

        /* Remove this one, and check if all are gone */
        RemoveEntryList(&InterruptObject->InterruptListEntry);
        if (IsListEmpty(&CurrentIsr->ListHead))
        {
            /* Completely Disable the Interrupt */
            HalDisableSystemInterrupt(InterruptObject->Vector, InterruptObject->Irql);
        }

        /* Disconnect it */
        InterruptObject->Connected = FALSE;

        /* Release the interrupt lock */
        KeReleaseInterruptSpinLock(InterruptObject, synch_oldlvl);
    }
    /* Release the table spinlock */
    KiReleaseSpinLock(&CurrentIsr->Lock);
    KeLowerIrql(oldlvl);

    /* Go back to default affinity */
    KeRevertToUserAffinityThread();

    /* Return Old Interrupt State */
    return State;
}

/*
 * @implemented
 */
VOID
NTAPI
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

    /* Disconnect it at first */
    Interrupt->Connected = FALSE;
}

VOID KePrintInterruptStatistic(VOID)
{
   LONG i, j;

   for (j = 0; j < KeNumberProcessors; j++)
   {
      DPRINT1("CPU%d:\n", j);
      for (i = 0; i < NR_TRAPS; i++)
      {
         if (IsrTable[i][j].Count)
         {
             DPRINT1("  Irq %x(%d): %d\n", i, i, IsrTable[i][j].Count);
         }
      }
   }
}

BOOLEAN
NTAPI
KeDisableInterrupts(VOID)
{
    ULONG Flags = 0;
    BOOLEAN Return;

    Flags = __readmsr();
    Return = (Flags & 0x8000) ? TRUE: FALSE;

    /* Disable interrupts */
    _disable();
    return Return;
}

ULONG
NTAPI
KdpServiceDispatcher(ULONG Service, PCHAR Buffer, ULONG Length);

typedef ULONG (*PSYSCALL_FUN)
(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG);

VOID
NTAPI
KiSystemService(ppc_trap_frame_t *trap_frame)
{
    int i;
    PKSYSTEM_ROUTINE SystemRoutine;
    PSYSCALL_FUN SyscallFunction;

    switch(trap_frame->gpr[0])
    {
    case 0x10000: /* DebugService */
	for( i = 0; i < trap_frame->gpr[5]; i++ )
        {
            PearPCDebug(((PCHAR)trap_frame->gpr[4])[i]);
            WRITE_PORT_UCHAR((PVOID)0x800003f8, ((PCHAR)trap_frame->gpr[4])[i]);
        }
	trap_frame->gpr[3] = KdpServiceDispatcher
	    (trap_frame->gpr[3],
	     (PCHAR)trap_frame->gpr[4],
	     trap_frame->gpr[5]);
	break;
    case 0xf0000: /* Thread startup */
        /* XXX how to use UserThread (gpr[6]) */
        SystemRoutine = (PKSYSTEM_ROUTINE)trap_frame->gpr[3];
        SystemRoutine((PKSTART_ROUTINE)trap_frame->gpr[4], 
                      (PVOID)trap_frame->gpr[5]);
        break;

        /* Handle a normal system call */
    default:
        SyscallFunction = 
            ((PSYSCALL_FUN*)KeServiceDescriptorTable
             [trap_frame->gpr[0] >> 12].Base)[trap_frame->gpr[0] & 0xfff];
        trap_frame->gpr[3] = SyscallFunction
            (trap_frame->gpr[3],
             trap_frame->gpr[4],
             trap_frame->gpr[5],
             trap_frame->gpr[6],
             trap_frame->gpr[7],
             trap_frame->gpr[8],
             trap_frame->gpr[9],
             trap_frame->gpr[10],
             trap_frame->gpr[11],
             trap_frame->gpr[12]);
        break;
    }
}

/* EOF */
