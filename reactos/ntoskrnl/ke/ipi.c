/* $Id: ipi.c,v 1.5 2004/12/24 17:06:58 navaraf Exp $
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
	 Pcr->PrcbData.IpiFrozen |= IpiRequest;
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
   ULONG TargetSet, Processor;

   PKPCR Pcr;

   ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

   DPRINT("KiIpiServiceRoutine\n");

   Pcr = KeGetCurrentKPCR();

   if (Pcr->PrcbData.IpiFrozen & IPI_REQUEST_APC)
   {
      Pcr->PrcbData.IpiFrozen &= ~IPI_REQUEST_APC;
      HalRequestSoftwareInterrupt(APC_LEVEL);
   }

   if (Pcr->PrcbData.IpiFrozen & IPI_REQUEST_DPC)
   {
      Pcr->PrcbData.IpiFrozen &= ~IPI_REQUEST_DPC;
      Pcr->PrcbData.DpcInterruptRequested = TRUE;
      HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
   }

   if (Pcr->PrcbData.IpiFrozen & IPI_REQUEST_FUNCTIONCALL)
   {
      InterlockedDecrementUL(&Pcr->PrcbData.SignalDone->CurrentPacket[1]);
      if (Pcr->PrcbData.SignalDone->CurrentPacket[2])
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
	       DPRINT1("Waiting longer than %d seconds to start the ipi routine\n", Count);
	       KEBUGCHECK(0);
	    }
#endif	 
         }
      }
      ((VOID STDCALL(*)(PVOID))(Pcr->PrcbData.SignalDone->WorkerRoutine))(Pcr->PrcbData.SignalDone->CurrentPacket[0]);
      do
      {
         Processor = 1 << KeGetCurrentProcessorNumber();
	 TargetSet = Pcr->PrcbData.SignalDone->TargetSet;
      } while (Processor & InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone->TargetSet, TargetSet & ~Processor, TargetSet)); 
      if (Pcr->PrcbData.SignalDone->CurrentPacket[2])
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
	       DPRINT1("Waiting longer than %d seconds after executing the ipi routine\n", Count);
	       KEBUGCHECK(0);
	    }
#endif	 
         }
      }
      InterlockedExchangePointer(&Pcr->PrcbData.SignalDone, NULL);
      Pcr->PrcbData.IpiFrozen &= ~IPI_REQUEST_FUNCTIONCALL;
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
    CurrentPcr->PrcbData.TargetSet = TargetSet;
    CurrentPcr->PrcbData.WorkerRoutine = (ULONG_PTR)WorkerRoutine;
    CurrentPcr->PrcbData.CurrentPacket[0] = Argument;
    CurrentPcr->PrcbData.CurrentPacket[1] = (PVOID)Count;
    CurrentPcr->PrcbData.CurrentPacket[2] = (PVOID)(ULONG)Synchronize;

    CurrentProcessor = 1 << KeGetCurrentProcessorNumber();

    for (i = 0, Processor = 1; i < KeNumberProcessors; i++, Processor <<= 1)
    {
       if (TargetSet & Processor)
       {
          Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
          while(0 != InterlockedCompareExchangeUL(&Pcr->PrcbData.SignalDone, (LONG)&CurrentPcr->PrcbData, 0));
	  Pcr->PrcbData.IpiFrozen |= IPI_REQUEST_FUNCTIONCALL;
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
