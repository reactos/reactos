/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/ipi.c
 * PURPOSE:         IPI Routines (Inter-Processor Interrupts). NT5+
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Hartmut Birr    
 * UPDATE HISTORY:
 *                  Created 11/08/2004
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
	 Ke386TestAndSetBit(IpiRequest, &Pcr->PrcbData.IpiFrozen);
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
   PKPCR Pcr;

   ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

   DPRINT("KiIpiServiceRoutine\n");

   Pcr = KeGetCurrentKPCR();

   if (Ke386TestAndClearBit(IPI_REQUEST_APC, &Pcr->PrcbData.IpiFrozen))
   {
      HalRequestSoftwareInterrupt(APC_LEVEL);
   }

   if (Ke386TestAndClearBit(IPI_REQUEST_DPC, &Pcr->PrcbData.IpiFrozen))
   {
      Pcr->PrcbData.DpcInterruptRequested = TRUE;
      HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
   }

   if (Ke386TestAndClearBit(IPI_REQUEST_FUNCTIONCALL, &Pcr->PrcbData.IpiFrozen))
   {
      InterlockedDecrementUL(&Pcr->PrcbData.SignalDone->CurrentPacket[1]);
      if (InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone->CurrentPacket[2], 0, 0))
      {
#ifdef DBG      	
         StartTime = KeQueryPerformanceCounter(&Frequency);
#endif         
         while (0 != InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone->CurrentPacket[1], 0, 0))
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
      ((VOID STDCALL(*)(PVOID))(Pcr->PrcbData.SignalDone->WorkerRoutine))(Pcr->PrcbData.SignalDone->CurrentPacket[0]);
      Ke386TestAndClearBit(KeGetCurrentProcessorNumber(), &Pcr->PrcbData.SignalDone->TargetSet);
      if (InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone->CurrentPacket[2], 0, 0))
      {
#ifdef DBG      	
         StartTime = KeQueryPerformanceCounter(&Frequency);
#endif         
         while (0 != InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone->TargetSet, 0, 0))
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
      InterlockedExchangePointer(&Pcr->PrcbData.SignalDone, NULL);
   }
   DPRINT("KiIpiServiceRoutine done\n");
   return TRUE;
}

VOID
STDCALL
KiIpiSendPacket(ULONG TargetSet, VOID STDCALL (*WorkerRoutine)(PVOID), PVOID Argument, ULONG Count, BOOLEAN Synchronize)
{
    ULONG i, Processor, CurrentProcessor;
    PKPCR Pcr, CurrentPcr;
    KIRQL oldIrql;


    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

    CurrentPcr = KeGetCurrentKPCR();
    InterlockedExchangeUL(&CurrentPcr->PrcbData.TargetSet, TargetSet);
    InterlockedExchangeUL(&CurrentPcr->PrcbData.WorkerRoutine, (ULONG_PTR)WorkerRoutine);
    InterlockedExchangePointer(&CurrentPcr->PrcbData.CurrentPacket[0], Argument);
    InterlockedExchangeUL(&CurrentPcr->PrcbData.CurrentPacket[1], Count);
    InterlockedExchangeUL(&CurrentPcr->PrcbData.CurrentPacket[2], Synchronize ? 1 : 0);

    CurrentProcessor = 1 << KeGetCurrentProcessorNumber();

    for (i = 0, Processor = 1; i < KeNumberProcessors; i++, Processor <<= 1)
    {
       if (TargetSet & Processor)
       {
          Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
          while(0 != InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone, (LONG)&CurrentPcr->PrcbData, 0));
	  Ke386TestAndSetBit(IPI_REQUEST_FUNCTIONCALL, &Pcr->PrcbData.IpiFrozen);
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
KeIpiGenericCall(VOID STDCALL (*Function)(PVOID), PVOID Argument)
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
