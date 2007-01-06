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
/* $Id: timer.c 23907 2006-09-04 05:52:23Z arty $
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

#define 	TMR_CTRL	0x43	/*	I/O for control		*/
#define		TMR_CNT0	0x40	/*	I/O for counter 0	*/
#define		TMR_CNT1	0x41	/*	I/O for counter 1	*/
#define		TMR_CNT2	0x42	/*	I/O for counter 2	*/

#define		TMR_SC0		0	/*	Select channel 0 	*/
#define		TMR_SC1		0x40	/*	Select channel 1 	*/
#define		TMR_SC2		0x80	/*	Select channel 2 	*/

#define		TMR_LOW		0x10	/*	RW low byte only 	*/
#define		TMR_HIGH	0x20	/*	RW high byte only 	*/
#define		TMR_BOTH	0x30	/*	RW both bytes 		*/

#define		TMR_MD0		0	/*	Mode 0 			*/
#define		TMR_MD1		0x2	/*	Mode 1 			*/
#define		TMR_MD2		0x4	/*	Mode 2 			*/
#define		TMR_MD3		0x6	/*	Mode 3 			*/
#define		TMR_MD4		0x8	/*	Mode 4 			*/
#define		TMR_MD5		0xA	/*	Mode 5 			*/

#define		TMR_BCD		1	/*	BCD mode 		*/

#define		TMR_LATCH	0	/*	Latch command 		*/

#define		TMR_READ	0xF0	/*    Read command 		*/
#define		TMR_CNT		0x20	/*    CNT bit  (Active low, subtract it) */
#define		TMR_STAT	0x10	/*    Status bit  (Active low, subtract it) */
#define		TMR_CH2		0x8	/*    Channel 2 bit 		*/
#define		TMR_CH1		0x4	/*    Channel 1 bit 		*/
#define		TMR_CH0		0x2	/*    Channel 0 bit 		*/

#define MILLISEC        10                     /* Number of millisec between interrupts */
#define HZ              (1000 / MILLISEC)      /* Number of interrupts per second */
#define CLOCK_TICK_RATE 1193182                /* Clock frequency of the timer chip */
#define LATCH           (CLOCK_TICK_RATE / HZ) /* Count to program into the timer chip */
#define PRECISION       8                      /* Number of bits to calibrate for delay loop */

static BOOLEAN UdelayCalibrated = FALSE;

/* FUNCTIONS **************************************************************/

static BOOLEAN PPCGetEEBit()
{
    ULONG Msr;
    __asm__("mfmsr %0" : "=r" (Msr));
    return (Msr & 0x8000) != 0;
}

/*
 * NOTE: This function MUST NOT be optimized by the compiler!
 * If it is, it obviously will not delay AT ALL, and the system
 * will appear completely frozen at boot since
 * HalpCalibrateStallExecution will never return.
 * There are three options to stop optimization:
 * 1. Use a volatile automatic variable. Making it delay quite a bit
 *    due to memory accesses, and keeping the code portable. However,
 *    as this involves memory access it depends on both the CPU cache,
 *    e.g. if the stack used is already in a cache line or not, and
 *    whether or not we're MP. If MP, another CPU could (probably would)
 *    also access RAM at the same time - making the delay imprecise.
 * 2. Use compiler-specific #pragma's to disable optimization.
 * 3. Use inline assembly, making it equally unportable as #2.
 * For supported compilers we use inline assembler. For the others,
 * portable plain C.
 */
DECLSPEC_NOINLINE VOID STDCALL
__KeStallExecutionProcessor(ULONG Loops)
{
    ULONG DecInit, DecCount;
    if (!Loops)
    {
	return;
    }
    __asm__("mfdec %0" : "=r" (DecInit));
    do {
	__asm__("mfdec %0" : "=r" (DecCount));
    } while((DecCount - DecInit) < Loops);
}

VOID
STDCALL
KeStallExecutionProcessor(ULONG Microseconds)
{
   PKIPCR Pcr = (PKIPCR)KeGetPcr();
   LARGE_INTEGER EndCount, CurrentCount;
   ULONG NewCount;
   __asm__("mfdec %0" : "=r" (NewCount));
   EndCount.QuadPart = NewCount + Microseconds * (ULONGLONG)Pcr->Prcb->MHz;
   do
   {
       __asm__("mfdec %0" : "=r" (NewCount));
       if(NewCount < CurrentCount.LowPart)
	   CurrentCount.HighPart++;
       CurrentCount.LowPart = NewCount;
   }
   while (CurrentCount.QuadPart < EndCount.QuadPart);
}

VOID HalpCalibrateStallExecution(VOID)
{
  PKIPCR Pcr;

  if (UdelayCalibrated)
    {
      return;
    }

  UdelayCalibrated = TRUE;
  Pcr = (PKIPCR)KeGetPcr();
  
  // XXX arty FIXME
  /* Pcr->Prcb.MHz = (ULONG)(EndCount.QuadPart - StartCount.QuadPart) / 10000; */
  Pcr->Prcb->MHz = 300;
  DPRINT1("%luMHz\n", Pcr->Prcb->MHz);
}


VOID STDCALL
HalCalibratePerformanceCounter(ULONG Count)
{
   BOOLEAN InterruptsEnabled = PPCGetEEBit();

   /* save flags and disable interrupts */
   _disable();

   __KeStallExecutionProcessor(Count);

   /* restore flags */
   if(InterruptsEnabled)
       _enable();
}


LARGE_INTEGER
STDCALL
KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFreq)
/*
 * FUNCTION: Queries the finest grained running count available in the system
 * ARGUMENTS:
 *         PerformanceFreq (OUT) = The routine stores the number of 
 *                                 performance counter ticks per second here
 * RETURNS: The number of performance counter ticks since boot
 */
{
  PKIPCR Pcr;
  LARGE_INTEGER Value;
  BOOLEAN InterruptsEnabled = PPCGetEEBit();

  _disable();

  Pcr = (PKIPCR)KeGetPcr();

  if (NULL != PerformanceFreq)
  {
      PerformanceFreq->QuadPart = Pcr->Prcb->MHz * (ULONGLONG)1000000;   
  }
  __asm__("mfdec %0" : "=r" (Value.LowPart));

  if(InterruptsEnabled)
      _enable();

  return Value;
}

/* EOF */
