/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/excutive/time.c
 * PURPOSE:         Time
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ex.h>
#include <string.h>

#include <internal/debug.h>


#define TICKSPERMINUTE  600000000

/* GLOBALS ******************************************************************/

/* Note: Bias[minutes] = UTC - local time */
TIME_ZONE_INFORMATION SystemTimeZoneInfo;


/* FUNCTIONS ****************************************************************/

VOID
ExInitTimeZoneInfo (VOID)
{
  /* Initialize system time zone information */
  memset (&SystemTimeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));

  /* FIXME: Read time zone information from the registry */

}


NTSTATUS
STDCALL
NtSetSystemTime (
	IN	PLARGE_INTEGER	SystemTime,
	IN	PLARGE_INTEGER	NewSystemTime	OPTIONAL
	)
{
        HalSetRealTimeClock ((PTIME)SystemTime);
//        UNIMPLEMENTED;
        return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
NtQuerySystemTime (
	OUT	TIME	* CurrentTime
	)
{
	KeQuerySystemTime((PLARGE_INTEGER)CurrentTime);
	return STATUS_SUCCESS;
}


VOID
ExLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime, 
	PLARGE_INTEGER	SystemTime
	)
{
   SystemTime->QuadPart = LocalTime->QuadPart +
                          SystemTimeZoneInfo.Bias * TICKSPERMINUTE;
}


VOID
ExSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	)
{
   LocalTime->QuadPart = SystemTime->QuadPart -
                         SystemTimeZoneInfo.Bias * TICKSPERMINUTE;
}

/* EOF */
