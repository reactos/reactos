/*
 * ReactOS kernel
 * Copyright (C) 2000 David Welch <welch@cwcom.net>
 * Copyright (C) 1999 Gareth Owen <gaz@athene.co.uk>, Ramon von Handel
 * Copyright (C) 1991, 1992 Linus Torvalds
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 */
/* $Id$
 *
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/hal/x86/udelay.c
 * PURPOSE:        Busy waiting
 * PROGRAMMER:     David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                 06/11/99 Created
 */

/* INCLUDES ***************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define MILLISEC        10                     /* Number of millisec between interrupts */
#define HZ              (1000 / MILLISEC)      /* Number of interrupts per second */
#define CLOCK_TICK_RATE 1193182                /* Clock frequency of the timer chip */
#define LATCH           (CLOCK_TICK_RATE / HZ) /* Count to program into the timer chip */
#define PRECISION       8                      /* Number of bits to calibrate for delay loop */

/* GLOBALS *******************************************************************/

BOOLEAN HalpClockSetMSRate;
ULONG HalpCurrentTimeIncrement;
ULONG HalpCurrentRollOver;
ULONG HalpNextMSRate = 14;
ULONG HalpLargestClockMS = 15;

LARGE_INTEGER HalpRolloverTable[15] =
{
    {{1197, 10032}},
    {{2394, 20064}},
    {{3591, 30096}},
    {{4767, 39952}},
    {{5964, 49984}},
    {{7161, 60016}},
    {{8358, 70048}},
    {{9555, 80080}},
    {{10731, 89936}},
    {{11949, 100144}},
    {{13125, 110000}},
    {{14322, 120032}},
    {{15519, 130064}},
    {{16695, 139920}},
    {{17892, 149952}}
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpInitializeClock(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Increment;
    USHORT RollOver;

    /* Check the CPU Type */
    if (Prcb->CpuType <= 4)
    {
        /* 486's or equal can't go higher then 10ms */
        HalpLargestClockMS = 10;
        HalpNextMSRate = 9;
    }

    /* Get increment and rollover for the largest time clock ms possible */
    Increment= HalpRolloverTable[HalpLargestClockMS - 1].HighPart;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].LowPart;

    /* Set the maximum and minimum increment with the kernel */
    HalpCurrentTimeIncrement = Increment;
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].HighPart);

    /* Disable interrupts */
    _disable();

    /* Set the rollover */
    __outbyte(TIMER_CONTROL_PORT, TIMER_SC0 | TIMER_BOTH | TIMER_MD2);
    __outbyte(TIMER_DATA_PORT0, RollOver & 0xFF);
    __outbyte(TIMER_DATA_PORT0, RollOver >> 8);

    /* Restore interrupts */
    _enable();

    /* Save rollover and return */
    HalpCurrentRollOver = RollOver;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
{
    /* Disable interrupts */
    _disable();

    /* Do a decrement for this CPU */
    //_InterlockedDecrement(Count);
    InterlockedDecrement(Count);

    /* Wait for other CPUs */
    while (*Count);

    /* Bring interrupts back */
    _enable();
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    /* Round increment to ms */
    Increment /= 10000;

    /* Normalize between our minimum (1 ms) and maximum (variable) setting */
    if (Increment > HalpLargestClockMS) Increment = HalpLargestClockMS;
    if (Increment < 0) Increment = 1;

    /* Set the rate and tell HAL we want to change it */
    HalpNextMSRate = Increment;
    HalpClockSetMSRate = TRUE;

    /* Return the increment */
    return HalpRolloverTable[Increment - 1].HighPart;
}

/* STUFF *********************************************************************/

ULONG
FORCEINLINE
Read8254Timer(VOID)
{
    ULONG Count;

    /* Disable interrupts */
    _disable();

    /* Set the rollover */
    __outbyte(TIMER_CONTROL_PORT, TIMER_SC0);
    Count = __inbyte(TIMER_DATA_PORT0);
    Count |= __inbyte(TIMER_DATA_PORT0) << 8;

    /* Restore interrupts and return count*/
    _enable();
    return Count;
}

VOID WaitFor8254Wraparound(VOID)
{
  ULONG CurCount, PrevCount = ~0;
  LONG Delta;

  CurCount = Read8254Timer();

  do
    {
      PrevCount = CurCount;
      CurCount = Read8254Timer();
      Delta = CurCount - PrevCount;

      /*
       * This limit for delta seems arbitrary, but it isn't, it's
       * slightly above the level of error a buggy Mercury/Neptune
       * chipset timer can cause.
       */

    }
   while (Delta < 300);
}

VOID HalpCalibrateStallExecution(VOID)
{
  ULONG i;
  ULONG calib_bit;
  ULONG CurCount;
  PKIPCR Pcr;
  LARGE_INTEGER StartCount, EndCount;

  Pcr = (PKIPCR)KeGetPcr();

  if (Pcr->PrcbData.FeatureBits & KF_RDTSC)
  {
      
     WaitFor8254Wraparound();
     StartCount.QuadPart = (LONGLONG)__rdtsc();

     WaitFor8254Wraparound();
     EndCount.QuadPart = (LONGLONG)__rdtsc();

     Pcr->PrcbData.MHz = (ULONG)(EndCount.QuadPart - StartCount.QuadPart) / 10000;
     DPRINT("%luMHz\n", Pcr->PrcbData.MHz);
     return;

  }

  DPRINT("Calibrating delay loop... [");

  /* Stage 1:  Coarse calibration					    */

  WaitFor8254Wraparound();

  Pcr->StallScaleFactor = 1;

  do
    {
      Pcr->StallScaleFactor <<= 1;		/* Next delay count to try  */

      WaitFor8254Wraparound();

      KeStallExecutionProcessor(Pcr->StallScaleFactor);   /* Do the delay */

      CurCount = Read8254Timer();
    }
  while (CurCount > LATCH / 2);

  Pcr->StallScaleFactor >>= 1;		    /* Get bottom value for delay   */

  /* Stage 2:  Fine calibration						    */
  DPRINT("delay_count: %d", Pcr->StallScaleFactor);

  calib_bit = Pcr->StallScaleFactor;	/* Which bit are we going to test   */

  for (i = 0; i < PRECISION; i++)
    {
      calib_bit >>= 1;				/* Next bit to calibrate    */
      if (!calib_bit)
	{
	  break;			/* If we have done all bits, stop   */
	}

      Pcr->StallScaleFactor |= calib_bit;   /* Set the bit in delay_count   */

      WaitFor8254Wraparound();

      KeStallExecutionProcessor(Pcr->StallScaleFactor);   /* Do the delay */

      CurCount = Read8254Timer();
      if (CurCount <= LATCH / 2)	/* If a tick has passed, turn the   */
	{				/* calibrated bit back off	    */
	  Pcr->StallScaleFactor &= ~calib_bit;
	}
    }

  /* We're finished:  Do the finishing touches				    */

  Pcr->StallScaleFactor /= (MILLISEC / 2);  /* Calculate delay_count for 1ms */

  DPRINT("]\n");
  DPRINT("delay_count: %d\n", Pcr->StallScaleFactor);
  DPRINT("CPU speed: %d\n", Pcr->StallScaleFactor / 250);
#if 0
  DbgPrint("About to start delay loop test\n");
  DbgPrint("Waiting for five minutes...");
  for (i = 0; i < (5*60*1000*20); i++)
    {
      KeStallExecutionProcessor(50);
    }
  DbgPrint("finished\n");
  for(;;);
#endif
}

/* EOF */
