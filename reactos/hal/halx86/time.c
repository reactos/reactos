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
#include <mps.h>
#include <bus.h>

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


static UCHAR
HalpQueryCMOS(UCHAR Reg)
{
  UCHAR Val;
  ULONG Flags;

  Reg |= 0x80;
  pushfl(Flags);
  __asm__("cli\n");  // AP unsure as to whether to do this here
  WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
  Val = READ_PORT_UCHAR((PUCHAR)0x71);
  WRITE_PORT_UCHAR((PUCHAR)0x70, 0);
  popfl(Flags);

  return(Val);
}


static VOID
HalpSetCMOS(UCHAR Reg,
	    UCHAR Val)
{
  ULONG Flags;

  Reg |= 0x80;
  pushfl(Flags);
  __asm__("cli\n");  // AP unsure as to whether to do this here
  WRITE_PORT_UCHAR((PUCHAR)0x70, Reg);
  WRITE_PORT_UCHAR((PUCHAR)0x71, Val);
  WRITE_PORT_UCHAR((PUCHAR)0x70, 0);
  popfl(Flags);
}


static UCHAR
HalpQueryECMOS(USHORT Reg)
{
  UCHAR Val;
  ULONG Flags;

  pushfl(Flags);
  __asm__("cli\n");  // AP unsure as to whether to do this here
  WRITE_PORT_UCHAR((PUCHAR)0x74, (UCHAR)(Reg & 0x00FF));
  WRITE_PORT_UCHAR((PUCHAR)0x75, (UCHAR)(Reg>>8));
  Val = READ_PORT_UCHAR((PUCHAR)0x76);
  popfl(Flags);

  return(Val);
}


static VOID
HalpSetECMOS(USHORT Reg,
	     UCHAR Val)
{
  ULONG Flags;

  pushfl(Flags);
  __asm__("cli\n");  // AP unsure as to whether to do this here
  WRITE_PORT_UCHAR((PUCHAR)0x74, (UCHAR)(Reg & 0x00FF));
  WRITE_PORT_UCHAR((PUCHAR)0x75, (UCHAR)(Reg>>8));
  WRITE_PORT_UCHAR((PUCHAR)0x76, Val);
  popfl(Flags);
}


VOID STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time)
{
    /* check 'Update In Progress' bit */
    while (HalpQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    Time->Second = BCD_INT(HalpQueryCMOS (0));
    Time->Minute = BCD_INT(HalpQueryCMOS (2));
    Time->Hour = BCD_INT(HalpQueryCMOS (4));
    Time->Weekday = BCD_INT(HalpQueryCMOS (6));
    Time->Day = BCD_INT(HalpQueryCMOS (7));
    Time->Month = BCD_INT(HalpQueryCMOS (8));
    Time->Year = BCD_INT(HalpQueryCMOS (9));

    if (Time->Year > 80)
        Time->Year += 1900;
    else
        Time->Year += 2000;

#if 0
    /* Century */
    Time->Year += BCD_INT(HalpQueryCMOS (RTC_REGISTER_CENTURY)) * 100;
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
    while (HalpQueryCMOS (RTC_REGISTER_A) & RTC_REG_A_UIP)
        ;

    HalpSetCMOS (0, INT_BCD(Time->Second));
    HalpSetCMOS (2, INT_BCD(Time->Minute));
    HalpSetCMOS (4, INT_BCD(Time->Hour));
    HalpSetCMOS (6, INT_BCD(Time->Weekday));
    HalpSetCMOS (7, INT_BCD(Time->Day));
    HalpSetCMOS (8, INT_BCD(Time->Month));
    HalpSetCMOS (9, INT_BCD(Time->Year % 100));

#if 0
    /* Century */
    HalpSetCMOS (RTC_REGISTER_CENTURY, INT_BCD(Time->Year / 100));
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

   if (HalpQueryCMOS(RTC_REGISTER_B) & 0x01)
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
    return FALSE;

  Val = HalpQueryCMOS(RTC_REGISTER_B);

  if (_stricmp(Value, "TRUE") == 0)
    HalpSetCMOS(RTC_REGISTER_B, Val | 0x01);
  else if (_stricmp(Value, "FALSE") == 0)
    HalpSetCMOS(RTC_REGISTER_B, Val & ~0x01);
  else
    return FALSE;

   return TRUE;
}


ULONG STDCALL
HalpGetCmosData(PBUS_HANDLER BusHandler,
		ULONG BusNumber,
		ULONG SlotNumber,
		PVOID Buffer,
		ULONG Offset,
		ULONG Length)
{
  PUCHAR Ptr = Buffer;
  ULONG Address = SlotNumber;
  ULONG Len = Length;

  DPRINT("HalpGetCmosData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if (Length == 0)
    return 0;

  if (BusNumber == 0)
    {
      /* CMOS */
      while ((Len > 0) && (Address < 0x100))
	{
	  *Ptr = HalpQueryCMOS((UCHAR)Address);
	  Ptr = Ptr + 1;
	  Address++;
	  Len--;
	}
    }
  else if (BusNumber == 1)
    {
      /* Extended CMOS */
      while ((Len > 0) && (Address < 0x1000))
	{
	  *Ptr = HalpQueryECMOS((USHORT)Address);
	  Ptr = Ptr + 1;
	  Address++;
	  Len--;
	}
    }

  return(Length - Len);
}


ULONG STDCALL
HalpSetCmosData(PBUS_HANDLER BusHandler,
		ULONG BusNumber,
		ULONG SlotNumber,
		PVOID Buffer,
		ULONG Offset,
		ULONG Length)
{
  PUCHAR Ptr = (PUCHAR)Buffer;
  ULONG Address = SlotNumber;
  ULONG Len = Length;

  DPRINT("HalpSetCmosData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if (Length == 0)
    return 0;

  if (BusNumber == 0)
    {
      /* CMOS */
      while ((Len > 0) && (Address < 0x100))
	{
	  HalpSetCMOS((UCHAR)Address, *Ptr);
	  Ptr = Ptr + 1;
	  Address++;
	  Len--;
	}
    }
  else if (BusNumber == 1)
    {
      /* Extended CMOS */
      while ((Len > 0) && (Address < 0x1000))
	{
	  HalpSetECMOS((USHORT)Address, *Ptr);
	  Ptr = Ptr + 1;
	  Address++;
	  Len--;
	}
    }

  return(Length - Len);
}

/* EOF */
