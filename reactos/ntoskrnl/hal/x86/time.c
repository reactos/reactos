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

/* FUNCTIONS *****************************************************************/

/* macro BCD_INT : convert bcd to int */
#define BCD_INT(bcd) ( ((bcd &0xf0)>>4)*10+(bcd &0x0f) )

void HalQueryRealTimeClock(PTIME_FIELDS pTime)
{
//FIXME : wait RTC ok?
  do
  {
   outb(0x70,0);
   pTime->Second=BCD_INT(inb(0x71));
  } while(pTime->Second>60);
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
//FIXME  pTime->MilliSeconds=
}
   
