/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/ipi.c
 * PURPOSE:         IPI Routines (Inter-Processor Interrupts). NT5+
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KSPIN_LOCK KiIpiLock;

/* FUNCTIONS *****************************************************************/

VOID
KiIpiSendRequest(ULONG TargetSet, ULONG IpiRequest)
{
   ULONG i;
   PKPCR Pcr;

   for (i = 0; i < KeNumberProcessors; i++)
   {
      if (TargetSet & (1 << i))
      {
         Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
	 Ke386TestAndSetBit(IpiRequest, &Pcr->Prcb->IpiFrozen);
	 HalRequestIpi(i);
      }
   }
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame,
                    IN PKEXCEPTION_FRAME ExceptionFrame)
{
#ifdef DBG
   LARGE_INTEGER StartTime, CurrentTime, Frequency;
   ULONG Count = 5;
#endif
   PKPRCB Prcb;

   ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

   DPRINT("KiIpiServiceRoutine\n");

   Prcb = KeGetCurrentPrcb();

   if (Ke386TestAndClearBit(IPI_REQUEST_APC, &Prcb->IpiFrozen))
   {
      HalRequestSoftwareInterrupt(APC_LEVEL);
   }

   if (Ke386TestAndClearBit(IPI_REQUEST_DPC, &Prcb->IpiFrozen))
   {
      Prcb->DpcInterruptRequested = TRUE;
      HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
   }

   if (Ke386TestAndClearBit(IPI_REQUEST_FUNCTIONCALL, &Prcb->IpiFrozen))
   {
      InterlockedDecrementUL(&Prcb->SignalDone->CurrentPacket[1]);
      if (InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[2], 0, 0))
      {
#ifdef DBG
         StartTime = KeQueryPerformanceCounter(&Frequency);
#endif
         while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[1], 0, 0))
	 {
#ifdef DBG
            CurrentTime = KeQueryPerformanceCounter(NULL);
	    if (CurrentTime.QuadPart > StartTime.QuadPart + Count * Frequency.QuadPart)
	    {
	       DbgPrint("(%s:%d) CPU%d, waiting longer than %d seconds to start the ipi routine\n", __FILE__,__LINE__, KeGetCurrentProcessorNumber(), Count);
	       KEBUGCHECK(0);
	    }
#endif
         }
      }
      ((VOID STDCALL(*)(PVOID))(Prcb->SignalDone->WorkerRoutine))(Prcb->SignalDone->CurrentPacket[0]);
      Ke386TestAndClearBit(KeGetCurrentProcessorNumber(), &Prcb->SignalDone->TargetSet);
      if (InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[2], 0, 0))
      {
#ifdef DBG
         StartTime = KeQueryPerformanceCounter(&Frequency);
#endif
         while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone->TargetSet, 0, 0))
         {
#ifdef DBG
	    CurrentTime = KeQueryPerformanceCounter(NULL);
	    if (CurrentTime.QuadPart > StartTime.QuadPart + Count * Frequency.QuadPart)
	    {
	       DbgPrint("(%s:%d) CPU%d, waiting longer than %d seconds after executing the ipi routine\n", __FILE__,__LINE__, KeGetCurrentProcessorNumber(), Count);
	       KEBUGCHECK(0);
	    }
#endif
         }
      }
      InterlockedExchangePointer(&Prcb->SignalDone, NULL);
   }
   DPRINT("KiIpiServiceRoutine done\n");
   return TRUE;
}

VOID
STDCALL
KiIpiSendPacket(ULONG TargetSet, VOID STDCALL (*WorkerRoutine)(PVOID), PVOID Argument, ULONG Count, BOOLEAN Synchronize)
{
    ULONG i, Processor, CurrentProcessor;
    PKPRCB Prcb, CurrentPrcb;
    KIRQL oldIrql;


    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

    CurrentPrcb = KeGetCurrentPrcb();
    InterlockedExchangeUL(&CurrentPrcb->TargetSet, TargetSet);
    InterlockedExchangeUL(&CurrentPrcb->WorkerRoutine, (ULONG_PTR)WorkerRoutine);
    InterlockedExchangePointer(&CurrentPrcb->CurrentPacket[0], Argument);
    InterlockedExchangeUL(&CurrentPrcb->CurrentPacket[1], Count);
    InterlockedExchangeUL(&CurrentPrcb->CurrentPacket[2], Synchronize ? 1 : 0);

    CurrentProcessor = 1 << KeGetCurrentProcessorNumber();

    for (i = 0, Processor = 1; i < KeNumberProcessors; i++, Processor <<= 1)
    {
       if (TargetSet & Processor)
       {
	  Prcb = ((PKPCR)(KPCR_BASE + i * PAGE_SIZE))->Prcb;
	  while(0 != InterlockedCompareExchangeUL(&Prcb->SignalDone, (LONG)CurrentPrcb, 0));
	  Ke386TestAndSetBit(IPI_REQUEST_FUNCTIONCALL, &Prcb->IpiFrozen);
	  if (Processor != CurrentProcessor)
	  {
	     HalRequestIpi(i);
	  }
       }
    }
    if (TargetSet & CurrentProcessor)
    {
       KeRaiseIrql(IPI_LEVEL, &oldIrql);
       KiIpiServiceRoutine(NULL, NULL);
       KeLowerIrql(oldIrql);
    }
}

VOID
KeIpiGenericCall(VOID (STDCALL *Function)(PVOID), PVOID Argument)
{
   KIRQL oldIrql;
   ULONG TargetSet;

   DPRINT("KeIpiGenericCall on CPU%d\n", KeGetCurrentProcessorNumber());

   KeRaiseIrql(SYNCH_LEVEL, &oldIrql);

   KiAcquireSpinLock(&KiIpiLock);

   TargetSet = (1 << KeNumberProcessors) - 1;

   KiIpiSendPacket(TargetSet, Function, Argument, KeNumberProcessors, TRUE);

   KiReleaseSpinLock(&KiIpiLock);

   KeLowerIrql(oldIrql);

   DPRINT("KeIpiGenericCall on CPU%d done\n", KeGetCurrentProcessorNumber());
}


/* EOF */
