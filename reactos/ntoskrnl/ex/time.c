/* $Id: time.c,v 1.23 2004/11/05 17:42:20 ekohl Exp $
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
#include <internal/debug.h>


#define TICKSPERMINUTE  600000000

/* GLOBALS ******************************************************************/

/* Note: Bias[minutes] = UTC - local time */
TIME_ZONE_INFORMATION ExpTimeZoneInfo;


/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
ExpInitTimeZoneInfo(VOID)
{
  NTSTATUS Status;

  /* Read time zone information from the registry */
  Status = RtlQueryTimeZoneInformation(&ExpTimeZoneInfo);
  if (!NT_SUCCESS(Status))
    {
      memset(&ExpTimeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));

    }
}


/*
 * FUNCTION: Sets the system time.
 * PARAMETERS:
 *        NewTime - Points to a variable that specified the new time
 *        of day in the standard time format.
 *        OldTime - Optionally points to a variable that receives the
 *        old time of day in the standard time format.
 * RETURNS: Status
 */
NTSTATUS STDCALL
NtSetSystemTime(IN PLARGE_INTEGER UnsafeNewSystemTime,
		OUT PLARGE_INTEGER UnsafeOldSystemTime OPTIONAL)
{
  NTSTATUS Status;
  LARGE_INTEGER OldSystemTime;
  LARGE_INTEGER NewSystemTime;
  TIME_FIELDS TimeFields;

  /* FIXME: Check for SeSystemTimePrivilege */

  if (UnsafeNewSystemTime == NULL)
    {
      /* FIXME: update time zone settings */

      return STATUS_SUCCESS;
    }

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
  RtlTimeToTimeFields (&NewSystemTime, &TimeFields);
  HalSetRealTimeClock (&TimeFields);

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


/*
 * FUNCTION: Retrieves the system time.
 * PARAMETERS:
 *          CurrentTime - Points to a variable that receives the current
 *          time of day in the standard time format.
 */
NTSTATUS STDCALL
NtQuerySystemTime(OUT PLARGE_INTEGER UnsafeCurrentTime)
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


/*
 * @implemented
 */
VOID
STDCALL
ExLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	)
{
   SystemTime->QuadPart = LocalTime->QuadPart;
#if 0
 +
                          ExpTimeZoneInfo.Bias * TICKSPERMINUTE;
#endif
}


/*
 * @unimplemented
 */
VOID
STDCALL
ExSetTimerResolution (
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution
    )
{
	UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID
STDCALL
ExSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	)
{
   LocalTime->QuadPart = SystemTime->QuadPart;
#if 0
 -
                         ExpTimeZoneInfo.Bias * TICKSPERMINUTE;
#endif
}

/* EOF */
