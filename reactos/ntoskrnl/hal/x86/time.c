/* * COPYRIGHT:       See COPYING in the top level directory * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/time.c
 * PURPOSE:         Getting time information
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/stddef.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/bitops.h>
#include <internal/linkage.h>
#include <string.h>
#include <internal/string.h>

#include <internal/i386/segment.h>
#include <internal/halio.h>

#define NDEBUG
#include <internal/debug.h>

/* MACROS and CONSTANTS ******************************************************/

/* macro BCD_INT : convert bcd to int */
#define BCD_INT(bcd) ( ((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f) )

/* macro INT_BCD : convert int to bcd */
#define INT_BCD(int) ( ((int / 10) << 4) + (int % 10) )


#define RTC_REGISTER_A   0x0A
#define   RTC_REG_A_UIP  0x80  /* Update In Progress bit */


/* FUNCTIONS *****************************************************************/

VOID
HalQueryRealTimeClock(PTIME_FIELDS pTime)
{
//FIXME : wait RTC ok?
        UCHAR uip;  // Update In Progress
#if 0
  do
  {
   outb(0x70,0);
   pTime->Second=BCD_INT(inb(0x71));
  } while(pTime->Second>60);
#endif

  do
  {
   outb(0x70, RTC_REGISTER_A);
   uip = inb(0x71) & RTC_REG_A_UIP;
  } while(uip);

  outb(0x70,0);
  pTime->Second=BCD_INT(inb(0x71));

  outb(0x70,2);
  pTime->Minute=BCD_INT(inb(0x71));
  outb(0x70,4);
  pTime->Hour=BCD_INT(inb(0x71));
  outb(0x70,6);
  pTime->Weekday=BCD_INT(inb(0x71));
  outb(0x70,7);
  pTime->Day=BCD_INT(inb(0x71));
  outb(0x70,8);
  pTime->Month=BCD_INT(inb(0x71));
  outb_p(0x70,9);
  pTime->Year=BCD_INT(inb_p(0x71));
  if(pTime->Year>80) pTime->Year +=1900;
  else pTime->Year +=2000;

#if 0
        /* Century */
        outb_p(0x70,0x32);
        pTime->Year += BCD_INT(inb_p(0x71)) * 100;
#endif

  pTime->Milliseconds=0;
}


VOID
HalSetRealTimeClock(PTIME_FIELDS pTime)
{
        UCHAR uip;  // Update In Progress

        do
        {
                outb(0x70,0xA);
                uip = inb(0x71) & RTC_REG_A_UIP;
        } while(uip);


        outb(0x70,0);
        outb(0x71, INT_BCD(pTime->Second));

        outb(0x70,2);
        outb(0x71, INT_BCD(pTime->Minute));

        outb(0x70,4);
        outb(0x71, INT_BCD(pTime->Hour));

        outb(0x70,6);
        outb(0x71, INT_BCD(pTime->Weekday));

        outb(0x70,7);
        outb(0x71, INT_BCD(pTime->Day));

        outb(0x70,8);
        outb(0x71, INT_BCD(pTime->Month));

        outb_p(0x70,9);
        outb(0x71, INT_BCD(pTime->Year % 100));


        /* Century */
#if 0
        outb_p(0x70,0x32);
        outb(0x71, INT_BCD(pTime->Year / 100));
#endif
}
