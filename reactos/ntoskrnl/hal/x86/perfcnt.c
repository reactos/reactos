/* $Id: perfcnt.c,v 1.2 2001/01/14 15:36:55 ekohl Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/hal/x86/perfcnt.c
 * PURPOSE:        Performance counter functions
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 *                 Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                 09/06/2000: Created
 */


/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>

/* FUNCTIONS **************************************************************/


VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count)
{
   ULONG i;

   /* save flags and disable interrupts */
   __asm__("pushf\n\t" \
	   "cli\n\t");

   for (i = 0; i < Count; i++);

   /* restore flags */
   __asm__("popf\n\t");
}


LARGE_INTEGER STDCALL
KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFreq)
/*
 * FUNCTION: Queries the finest grained running count avaiable in the system
 * ARGUMENTS:
 *         PerformanceFreq (OUT) = The routine stores the number of 
 *                                 performance counters tick per second here
 * RETURNS: The performance counter value in HERTZ
 * NOTE: Returns the system tick count or the time-stamp on the pentium
 */
{
  if (PerformanceFreq != NULL)
    {
      PerformanceFreq->QuadPart = 0;
    }

  return *PerformanceFreq;
}

/* EOF */
