/* $Id: time.c,v 1.13 2002/09/07 15:12:50 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/time.c
 * PURPOSE:         Time
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


#define TICKSPERMINUTE  600000000

/* GLOBALS ******************************************************************/

/* Note: Bias[minutes] = UTC - local time */
TIME_ZONE_INFORMATION _SystemTimeZoneInfo;


/* FUNCTIONS ****************************************************************/

VOID
ExInitTimeZoneInfo (VOID)
{
  /* Initialize system time zone information */
  memset (& _SystemTimeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));

  /* FIXME: Read time zone information from the registry */

}


NTSTATUS STDCALL
NtSetSystemTime (IN	PLARGE_INTEGER	UnsafeNewSystemTime,
		 OUT	PLARGE_INTEGER	UnsafeOldSystemTime	OPTIONAL)
     /*
      * FUNCTION: Sets the system time.
      * PARAMETERS:
      *        NewTime - Points to a variable that specified the new time
      *        of day in the standard time format.
      *        OldTime - Optionally points to a variable that receives the
      *        old time of day in the standard time format.
      * RETURNS: Status
      */
{
  NTSTATUS Status;
  LARGE_INTEGER OldSystemTime;
  LARGE_INTEGER NewSystemTime;

  /* FIXME: Check for SeSystemTimePrivilege */

  Status = MmCopyFromCaller(&NewSystemTime, UnsafeNewSystemTime,
			    sizeof(NewSystemTime));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  
  if (UnsafeOldSystemTime != NULL)
    {
      KeQuerySystemTime(&OldSystemTime);
    }
  HalSetRealTimeClock ((PTIME_FIELDS)&NewSystemTime);

  if (UnsafeOldSystemTime != NULL)
    {
      Status = MmCopyToCaller(UnsafeOldSystemTime, &OldSystemTime,
			      sizeof(OldSystemTime));
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtQuerySystemTime (OUT TIME* UnsafeCurrentTime)
     /*
      * FUNCTION: Retrieves the system time.
      * PARAMETERS:
      *          CurrentTime - Points to a variable that receives the current
      *          time of day in the standard time format.
      */
{
  LARGE_INTEGER CurrentTime;
  NTSTATUS Status;

  KeQuerySystemTime(&CurrentTime);
  Status = MmCopyToCaller(UnsafeCurrentTime, &CurrentTime,
			  sizeof(CurrentTime));
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  return STATUS_SUCCESS;
}


VOID
STDCALL
ExLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime, 
	PLARGE_INTEGER	SystemTime
	)
{
   SystemTime->QuadPart = LocalTime->QuadPart +
                          _SystemTimeZoneInfo.Bias * TICKSPERMINUTE;
}


VOID
STDCALL
ExSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	)
{
   LocalTime->QuadPart = SystemTime->QuadPart -
                         _SystemTimeZoneInfo.Bias * TICKSPERMINUTE;
}

/* EOF */
