/* $Id: udelay.c,v 1.3 2000/10/07 13:41:50 dwelch Exp $
 *
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/hal/x86/udelay.c
 * PURPOSE:        Busy waiting
 * PROGRAMMER:     David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                 06/11/99 Created
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

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static unsigned int delay_count = 1;

#define MILLISEC     (10)
#define FREQ         (1000/MILLISEC)

#define PRECISION    (8)

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

/* FUNCTIONS **************************************************************/

void init_pit(float h, unsigned char channel)
{
	unsigned int temp=0;
	
	temp = 1193180/h;
	
//	WRITE_PORT_UCHAR((PUCHAR)TMR_CTRL, 
//			 (channel*0x40) + TMR_BOTH + TMR_MD3);
        WRITE_PORT_UCHAR((PUCHAR)TMR_CTRL,
			 (channel*0x40) + TMR_BOTH + TMR_MD2);
	WRITE_PORT_UCHAR((PUCHAR)(0x40+channel), 
			 (unsigned char) temp);
	WRITE_PORT_UCHAR((PUCHAR)(0x40+channel), 
			 (unsigned char) (temp>>8));
}

VOID STDCALL
__KeStallExecutionProcessor(ULONG Loops)
{
   unsigned int i;
   for (i=0; i<Loops;i++);
}

VOID STDCALL KeStallExecutionProcessor(ULONG Microseconds)
{
   return(__KeStallExecutionProcessor((delay_count*(Microseconds/1000))));
}

#define HZ (100)
#define CLOCK_TICK_RATE (1193180)
#define LATCH ((CLOCK_TICK_RATE + HZ/2) / HZ)

VOID HalpCalibrateStallExecution(VOID)
{
   unsigned int prevtick;
   unsigned int i;
   unsigned int calib_bit;
   extern volatile ULONG KiRawTicks;
   
   DbgPrint("Calibrating delay loop... [");
   
   /* Initialise timer interrupt with MILLISECOND ms interval        */
   //init_pit(FREQ, 0);
   WRITE_PORT_UCHAR((PUCHAR)0x43, 0x34);  /* binary, mode 2, LSB/MSB, ch 0 */
   WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH & 0xff); /* LSB */
   WRITE_PORT_UCHAR((PUCHAR)0x40, LATCH >> 8); /* MSB */
   
   /* Stage 1:  Coarse calibration                                   */
   
   do {
      delay_count <<= 1;          /* Next delay count to try        */
      
      prevtick=KiRawTicks;             /* Wait for the start of the next */
      while(prevtick==KiRawTicks);     /* timer tick                     */
      
      prevtick=KiRawTicks;             /* Start measurement now          */
      __KeStallExecutionProcessor(delay_count);       /* Do the delay                   */
   } while(prevtick == KiRawTicks);     /* Until delay is just too big    */
   
   delay_count >>= 1;              /* Get bottom value for delay     */
   
   /* Stage 2:  Fine calibration                                     */
   DbgPrint("delay_count: %d\n", delay_count);
   
   calib_bit = delay_count;        /* Which bit are we going to test */

   for(i=0;i<PRECISION;i++) {
      calib_bit >>= 1;            /* Next bit to calibrate          */
      if(!calib_bit) break;       /* If we have done all bits, stop */
      
      delay_count |= calib_bit;   /* Set the bit in delay_count     */
      
      prevtick=KiRawTicks;             /* Wait for the start of the next */
      while(prevtick==KiRawTicks);     /* timer tick                     */
      
      prevtick=KiRawTicks;             /* Start measurement now          */
      __KeStallExecutionProcessor(delay_count);       /* Do the delay                   */
      
      if(prevtick != KiRawTicks)       /* If a tick has passed, turn the */
	delay_count &= ~calib_bit;     /* calibrated bit back off */
   }
   
   /* We're finished:  Do the finishing touches                      */
   
   delay_count /= MILLISEC;       /* Calculate delay_count for 1ms   */
   
   DbgPrint("]\n");
   DbgPrint("delay_count: %d\n", delay_count);
   DbgPrint("CPU speed: %d\n", delay_count/500);
#if 0
   DbgPrint("About to start delay loop test\n");
   for (i = 0; i < (10*1000*20); i++)
     {
	KeStallExecutionProcessor(50);
     }
   DbgPrint("Waiting for five minutes...");
   KeStallExecutionProcessor(5*60*1000*1000);
   for (i = 0; i < (5*60*1000*20); i++)
     {
	KeStallExecutionProcessor(50);
     }
   DbgPrint("finished\n");
   for(;;);
#endif
}

/* EOF */
