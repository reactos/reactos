/* $Id: ipi.c,v 1.3 2004/11/14 20:00:06 hbirr Exp $
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

struct
{
   VOID STDCALL (*Function)(PVOID);
   PVOID Argument;
   BOOLEAN Synchronize;
   ULONG StartCount;
   ULONG EndCount;
} KiIpiInfo;

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame,
                    IN PKEXCEPTION_FRAME ExceptionFrame)
{
    LARGE_INTEGER StartTime, CurrentTime, Frequency;

    ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

    DPRINT("KiIpiServiceRoutine\n");


    if (KiIpiInfo.Synchronize)
    {
       InterlockedDecrement(&KiIpiInfo.StartCount);
       StartTime = KeQueryPerformanceCounter(&Frequency);
       while (0 != InterlockedCompareExchange(&KiIpiInfo.StartCount, 0, 0))
       {
          CurrentTime = KeQueryPerformanceCounter(NULL);
	  if (CurrentTime.QuadPart > StartTime.QuadPart + Frequency.QuadPart)
	  {
	     DPRINT1("Waiting longer than 1 seconds to start the ipi routine\n");
	     KEBUGCHECK(0);
	  }
       }
    }
    KiIpiInfo.Function(KiIpiInfo.Argument);
    if (KiIpiInfo.Synchronize)
    {
       InterlockedDecrement(&KiIpiInfo.EndCount);
       StartTime = KeQueryPerformanceCounter(&Frequency);
       while (0 != InterlockedCompareExchange(&KiIpiInfo.EndCount, 0, 0))
       {
          CurrentTime = KeQueryPerformanceCounter(NULL);
	  if (CurrentTime.QuadPart > StartTime.QuadPart + Frequency.QuadPart)
	  {
	     DPRINT1("Waiting longer than 1 seconds after executing the ipi routine\n");
	     KEBUGCHECK(0);
	  }
       }
    }
    return TRUE;
}

VOID
STDCALL
KiIpiSendPacket(ULONG Processors, VOID STDCALL (*Function)(PVOID), PVOID Argument, ULONG Count, BOOLEAN Synchronize)
{
    ULONG i;

    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

    /* 
     * FIXME
     *   M$ puts the ipi information anywhere into the KPCR of the requestor.
     *   The KPCR of the target contains a pointer of the KPCR of the requestor.
     */

    KiIpiInfo.Function = Function;
    KiIpiInfo.Argument = Argument;
    if (Synchronize)
    {
       KiIpiInfo.StartCount = Count;
       KiIpiInfo.EndCount = Count;
    }
    KiIpiInfo.Synchronize = Synchronize;

    for (i = 0; i < KeNumberProcessors; i++)
    {
       if (Processors & (1 << i))
       {
	  HalRequestIpi(i);
       }
    }
}

VOID
STDCALL
KeIpiGenericCall(VOID STDCALL (*Function)(PVOID), PVOID Argument)
{
   KIRQL oldIrql, oldIrql2;
   ULONG Count, i;
   ULONG Processors = 0;

   DPRINT("KeIpiGenericCall on CPU%d\n", KeGetCurrentProcessorNumber());

   KeRaiseIrql(SYNCH_LEVEL, &oldIrql);

   Count = KeNumberProcessors;
   for (i = 0; i < KeNumberProcessors; i++)
   {
      if (KeGetCurrentProcessorNumber() != i)
      {
         Processors |= (1 << i);
      }
   }

   KiAcquireSpinLock(&KiIpiLock);

   KiIpiSendPacket(Processors, Function, Argument, Count, TRUE);
   
   KeRaiseIrql(IPI_LEVEL, &oldIrql2);
   
   KiIpiServiceRoutine(NULL, NULL);
   
   KeLowerIrql(oldIrql2);

   KiReleaseSpinLock(&KiIpiLock);
   
   KeLowerIrql(oldIrql);
   
   DPRINT("KeIpiGenericCall on CPU%d done\n", KeGetCurrentProcessorNumber());
}

VOID
STDCALL
KeIpiCallForBootProcessor(VOID STDCALL (*Function)(PVOID), PVOID Argument)
{
   KIRQL oldIrql, oldIrql2;
   LARGE_INTEGER StartCount, CurrentCount;
   LARGE_INTEGER Frequency;

   KeRaiseIrql(SYNCH_LEVEL, &oldIrql);

   ASSERT (KeGetCurrentProcessorNumber() != 0);

   KiAcquireSpinLock(&KiIpiLock);

   KiIpiSendPacket(1, Function, Argument, 1, TRUE);
   
   KeRaiseIrql(IPI_LEVEL, &oldIrql2);

   StartCount = KeQueryPerformanceCounter(&Frequency);
   while (0 != InterlockedCompareExchange(&KiIpiInfo.EndCount, 0, 0))
   {
      CurrentCount = KeQueryPerformanceCounter(NULL);
      if (CurrentCount.QuadPart > StartCount.QuadPart + Frequency.QuadPart)
      {
         DPRINT1("Waiting longer than 1 second after sending the ipi to the boot processor\n");
	 KEBUGCHECK(0);
      }
   }

   KeLowerIrql(oldIrql2);

   KiReleaseSpinLock(&KiIpiLock);
   
   KeLowerIrql(oldIrql);

}

/* EOF */
