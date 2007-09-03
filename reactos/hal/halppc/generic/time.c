/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/time.c
 * PURPOSE:         Getting time information
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>


/* MACROS and CONSTANTS ******************************************************/

/* macro BCD_INT : convert bcd to int */
#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

/* macro INT_BCD : convert int to bcd */
#define INT_BCD(int) (((int / 10) << 4) + (int % 10))


#define RTC_REGISTER_A   0x0A
#define   RTC_REG_A_UIP  0x80  /* Update In Progress bit */

#define RTC_REGISTER_B   0x0B

#define RTC_REGISTER_CENTURY   0x32

/* GLOBALS ******************************************************************/

/* FUNCTIONS *****************************************************************/

BOOLEAN STDCALL
HalQueryRealTimeClock(PTIME_FIELDS Time)
{
    return TRUE;
}


VOID STDCALL
HalSetRealTimeClock(PTIME_FIELDS Time)
{
}


BOOLEAN STDCALL
HalGetEnvironmentVariable(PCH Name,
			  USHORT ValueLength,
              PCH Value)
{
    strncpy(Value, "TRUE", ValueLength);
    return TRUE;
}


BOOLEAN STDCALL
HalSetEnvironmentVariable(PCH Name,
			  PCH Value)
{
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
  DPRINT("HalpGetCmosData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  return 0;
}


ULONG STDCALL
HalpSetCmosData(PBUS_HANDLER BusHandler,
		ULONG BusNumber,
		ULONG SlotNumber,
		PVOID Buffer,
		ULONG Offset,
		ULONG Length)
{
  DPRINT("HalpSetCmosData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  return 0;
}

/* EOF */
