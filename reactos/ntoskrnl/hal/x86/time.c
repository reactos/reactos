/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/time.c
 * PURPOSE:         Getting time information
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

/* MACROS and CONSTANTS ******************************************************/

/* macro BCD_INT : convert bcd to int */
#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

/* macro INT_BCD : convert int to bcd */
#define INT_BCD(int) (((int / 10) << 4) + (int % 10))


#define RTC_REGISTER_A   0x0A
#define   RTC_REG_A_UIP  0x80  /* Update In Progress bit */

#define RTC_REGISTER_B   0x0B

#define RTC_REGISTER_CENTURY   0x32


/* FUNCTIONS *****************************************************************/


static BYTE
HalQueryCMOS (BYTE Reg)
{
    BYTE Val;

    Reg |= 0x80;
    __asm__("cli\n");  // AP unsure as to whether to do this here
    WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
    Val = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0);
    __asm__("sti\n");  // AP unsure about this too..

    return(Val);
}


static VOID
HalSetCMOS (BYTE Reg, BYTE Val)
{
    Reg |= 0x80;
    __asm__("cli\n");  // AP unsure as to whether to do this here
    WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
    WRITE_PORT_UCHAR((PUCHAR)0x71, Val);
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0);
    __asm__("sti\n");  // AP unsure about this too..
}


VOID STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time)
{
    /* check 'Update In Progress' bit */
    while (HalQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    Time->Second = BCD_INT(HalQueryCMOS (0));
    Time->Minute = BCD_INT(HalQueryCMOS (2));
    Time->Hour = BCD_INT(HalQueryCMOS (4));
    Time->Weekday = BCD_INT(HalQueryCMOS (6));
    Time->Day = BCD_INT(HalQueryCMOS (7));
    Time->Month = BCD_INT(HalQueryCMOS (8));
    Time->Year = BCD_INT(HalQueryCMOS (9));

    if (Time->Year > 80)
        Time->Year += 1900;
    else
        Time->Year += 2000;

#if 0
    /* Century */
    Time->Year += BCD_INT(HalQueryCMOS (RTC_REGISTER_CENTURY)) * 100;
#endif

#ifndef NDEBUG
    DbgPrint ("HalQueryRealTimeClock() %d:%d:%d %d/%d/%d\n",
              Time->Hour,
              Time->Minute,
              Time->Second,
              Time->Day,
              Time->Month,
              Time->Year
             );
#endif

    Time->Milliseconds = 0;
}


VOID STDCALL
HalSetRealTimeClock(PTIME_FIELDS Time)
{
    /* check 'Update In Progress' bit */
    while (HalQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    HalSetCMOS (0, INT_BCD(Time->Second));
    HalSetCMOS (2, INT_BCD(Time->Minute));
    HalSetCMOS (4, INT_BCD(Time->Hour));
    HalSetCMOS (6, INT_BCD(Time->Weekday));
    HalSetCMOS (7, INT_BCD(Time->Day));
    HalSetCMOS (8, INT_BCD(Time->Month));
    HalSetCMOS (9, INT_BCD(Time->Year % 100));

#if 0
    /* Century */
    HalSetCMOS (RTC_REGISTER_CENTURY, INT_BCD(Time->Year / 100));
#endif
}


BOOLEAN STDCALL
HalGetEnvironmentVariable(PCH Name,
			  PCH Value,
			  USHORT ValueLength)
{
   if (_stricmp(Name, "LastKnownGood") != 0)
     {
	return FALSE;
     }

   if (HalQueryCMOS(RTC_REGISTER_B) & 0x01)
     {
	strncpy(Value, "FALSE", ValueLength);
     }
   else
     {
	strncpy(Value, "TRUE", ValueLength);
     }

   return TRUE;
}


BOOLEAN STDCALL
HalSetEnvironmentVariable(PCH Name,
			  PCH Value)
{
   UCHAR Val;

   if (_stricmp(Name, "LastKnownGood") != 0)
     {
	return FALSE;
     }

   Val = HalQueryCMOS(RTC_REGISTER_B);

   if (_stricmp(Value, "TRUE") == 0)
     {
	HalSetCMOS(RTC_REGISTER_B, Val | 0x01);
     }
   else if (_stricmp(Value, "FALSE") == 0)
     {
	HalSetCMOS(RTC_REGISTER_B, Val & ~0x01);
     }
   else
     {
	return FALSE;
     }

   return TRUE;
}
