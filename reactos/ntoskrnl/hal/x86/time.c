/* * COPYRIGHT:       See COPYING in the top level directory * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/time.c
 * PURPOSE:         Getting time information
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
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

#define RTC_REGISTER_CENTURY   0x32


/* FUNCTIONS *****************************************************************/


static BYTE
HalQueryCMOS (BYTE Reg)
{
    BYTE Val;

    Reg |= 0x80;
    __asm__("cli\n");  // AP unsure as to whether to do this here
    outb (0x70, Reg);
    Val=inb (0x71);
    outb (0x70, 0);
    __asm__("sti\n");  // AP unsure about this too..

    return(Val);
}


static VOID
HalSetCMOS (BYTE Reg, BYTE Val)
{
    Reg |= 0x80;
    __asm__("cli\n");  // AP unsure as to whether to do this here
    outb (0x70, Reg);
    outb (0x71, Val);
    outb (0x70, 0);
    __asm__("sti\n");  // AP unsure about this too..
}


VOID
HalQueryRealTimeClock (PTIME_FIELDS pTime)
{
    /* check 'Update In Progress' bit */
    while (HalQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    pTime->Second = BCD_INT(HalQueryCMOS (0));
    pTime->Minute = BCD_INT(HalQueryCMOS (2));
    pTime->Hour = BCD_INT(HalQueryCMOS (4));
    pTime->Weekday = BCD_INT(HalQueryCMOS (6));
    pTime->Day = BCD_INT(HalQueryCMOS (7));
    pTime->Month = BCD_INT(HalQueryCMOS (8));
    pTime->Year = BCD_INT(HalQueryCMOS (9));

    if (pTime->Year > 80)
        pTime->Year += 1900;
    else
        pTime->Year += 2000;

#if 0
    /* Century */
    pTime->Year += BCD_INT(HalQueryCMOS (RTC_REGISTER_CENTURY)) * 100;
#endif

#ifndef NDEBUG
    DbgPrint ("HalQueryRealTimeClock() %d:%d:%d %d/%d/%d\n",
              pTime->Hour,
              pTime->Minute,
              pTime->Second,
              pTime->Day,
              pTime->Month,
              pTime->Year
             );
#endif

    pTime->Milliseconds = 0;
}


VOID
HalSetRealTimeClock (PTIME_FIELDS pTime)
{
    /* check 'Update In Progress' bit */
    while (HalQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    HalSetCMOS (0, INT_BCD(pTime->Second));
    HalSetCMOS (2, INT_BCD(pTime->Minute));
    HalSetCMOS (4, INT_BCD(pTime->Hour));
    HalSetCMOS (6, INT_BCD(pTime->Weekday));
    HalSetCMOS (7, INT_BCD(pTime->Day));
    HalSetCMOS (8, INT_BCD(pTime->Month));
    HalSetCMOS (9, INT_BCD(pTime->Year % 100));

#if 0
    /* Century */
    HalSetCMOS (RTC_REGISTER_CENTURY, INT_BCD(pTime->Year / 100));
#endif
}
