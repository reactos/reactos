/* $Id: udelay.c,v 1.2 1999/11/24 11:51:50 dwelch Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ke/udelay.c
 * PURPOSE:        Busy waiting
 * PROGRAMMER:     David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                 06/11/99 Created
 */

/* INCLUDES ***************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>
#include <stdio.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS ******************************************************************/

#define MICROSECONDS_PER_TICK (54945)
#define TICKS_TO_CALIBRATE (1)
#define CALIBRATE_PERIOD (MICROSECONDS_PER_TICK * TICKS_TO_CALIBRATE)
#define SYSTEM_TIME_UNITS_PER_MSEC (10000)

static unsigned int loops_per_microsecond = 100;

extern ULONGLONG KiTimerTicks;

/* FUNCTIONS **************************************************************/

VOID KeCalibrateTimerLoop(VOID)
{
//   unsigned int start_tick;
//   unsigned int end_tick;
//   unsigned int nr_ticks;
//   unsigned int i;
//   unsigned int microseconds;
   
   #if 0
   for (i=0;i<20;i++)
     {
   
	start_tick = KiTimerTicks;
        microseconds = 0;
        while (start_tick == KiTimerTicks);
        while (KiTimerTicks == (start_tick+TICKS_TO_CALIBRATE))
        {
                KeStallExecutionProcessor(1);
                microseconds++;
        };

//        DbgPrint("microseconds %d\n",microseconds);

        if (microseconds > (CALIBRATE_PERIOD+1000))
        {
           loops_per_microsecond = loops_per_microsecond + 1;
        }
        if (microseconds < (CALIBRATE_PERIOD-1000))
        {
           loops_per_microsecond = loops_per_microsecond - 1;
        }
//        DbgPrint("loops_per_microsecond %d\n",loops_per_microsecond);
     }
//     for(;;);
   #endif
}

VOID KeStallExecutionProcessor(ULONG MicroSeconds)
{
   unsigned int i;
   for (i=0; i<(loops_per_microsecond*MicroSeconds) ;i++)
     {
	__asm__("nop\n\t");
     }
}

