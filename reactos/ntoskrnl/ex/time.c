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

#include <internal/debug.h>


#define TICKSPERMINUTE  600000000

/* GLOBALS ******************************************************************/

static LONG lTimeZoneBias = 0;  /* bias[minutes] = UTC - local time */

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL NtSetSystemTime(IN PLARGE_INTEGER SystemTime,
				 IN PLARGE_INTEGER NewSystemTime OPTIONAL)
{
   return(ZwSetSystemTime(SystemTime,NewSystemTime));
}

NTSTATUS STDCALL ZwSetSystemTime(IN PLARGE_INTEGER SystemTime,
				 IN PLARGE_INTEGER NewSystemTime OPTIONAL)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQuerySystemTime (OUT TIME *CurrentTime)
{
   return(ZwQuerySystemTime(CurrentTime));
}

NTSTATUS STDCALL ZwQuerySystemTime (OUT TIME *CurrentTime)
{
   KeQuerySystemTime((PLARGE_INTEGER)CurrentTime);
   return STATUS_SUCCESS;
//   UNIMPLEMENTED;
}

VOID ExLocalTimeToSystemTime(PLARGE_INTEGER LocalTime, 
			     PLARGE_INTEGER SystemTime)
{
   SystemTime->QuadPart = LocalTime->QuadPart +
                          lTimeZoneBias * TICKSPERMINUTE;
}

VOID ExSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
			     PLARGE_INTEGER LocalTime)
{
   LocalTime->QuadPart = SystemTime->QuadPart -
                         lTimeZoneBias * TICKSPERMINUTE;
}
