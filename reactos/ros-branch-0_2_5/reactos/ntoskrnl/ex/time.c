/* $Id: time.c,v 1.28 2004/12/05 00:20:22 navaraf Exp $
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
TIME_ZONE_INFORMATION ExpTimeZoneInfo;
LARGE_INTEGER ExpTimeZoneBias;
ULONG ExpTimeZoneId;


/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
ExpInitTimeZoneInfo(VOID)
{
  LARGE_INTEGER CurrentTime;
  NTSTATUS Status;

  /* Read time zone information from the registry */
  Status = RtlQueryTimeZoneInformation(&ExpTimeZoneInfo);
  if (!NT_SUCCESS(Status))
    {
      memset(&ExpTimeZoneInfo, 0, sizeof(TIME_ZONE_INFORMATION));

      ExpTimeZoneBias.QuadPart = (LONGLONG)0;
      ExpTimeZoneId = TIME_ZONE_ID_UNKNOWN;
    }
  else
    {
      /* FIXME: Calculate transition dates */

      ExpTimeZoneBias.QuadPart =
	((LONGLONG)(ExpTimeZoneInfo.Bias + ExpTimeZoneInfo.StandardBias)) * TICKSPERMINUTE;
      ExpTimeZoneId = TIME_ZONE_ID_STANDARD;
    }

  SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.u.HighPart;
  SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.u.HighPart;
  SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.u.LowPart;
  SharedUserData->TimeZoneId = ExpTimeZoneId;

  /* Convert boot time from local time to UTC */
  SystemBootTime.QuadPart += ExpTimeZoneBias.QuadPart;

  /* Convert sytem time from local time to UTC */
  do
    {
      CurrentTime.u.HighPart = SharedUserData->SystemTime.High1Time;
      CurrentTime.u.LowPart = SharedUserData->SystemTime.LowPart;
    }
  while (CurrentTime.u.HighPart != SharedUserData->SystemTime.High2Time);

  CurrentTime.QuadPart += ExpTimeZoneBias.QuadPart;

  SharedUserData->SystemTime.LowPart = CurrentTime.u.LowPart;
  SharedUserData->SystemTime.High1Time = CurrentTime.u.HighPart;
  SharedUserData->SystemTime.High2Time = CurrentTime.u.HighPart;
}


NTSTATUS
ExpSetTimeZoneInformation(PTIME_ZONE_INFORMATION TimeZoneInformation)
{
  LARGE_INTEGER LocalTime;
  LARGE_INTEGER SystemTime;
  TIME_FIELDS TimeFields;

  DPRINT("ExpSetTimeZoneInformation() called\n");

  DPRINT("Old time zone bias: %d minutes\n",
	 ExpTimeZoneInfo.Bias);
  DPRINT("Old time zone standard bias: %d minutes\n",
	 ExpTimeZoneInfo.StandardBias);

  DPRINT("New time zone bias: %d minutes\n",
	 TimeZoneInformation->Bias);
  DPRINT("New time zone standard bias: %d minutes\n",
	 TimeZoneInformation->StandardBias);

  /* Get the local time */
  HalQueryRealTimeClock(&TimeFields);
  RtlTimeFieldsToTime(&TimeFields,
		      &LocalTime);

  /* FIXME: Calculate transition dates */

  ExpTimeZoneBias.QuadPart =
    ((LONGLONG)(TimeZoneInformation->Bias + TimeZoneInformation->StandardBias)) * TICKSPERMINUTE;
  ExpTimeZoneId = TIME_ZONE_ID_STANDARD;

  memcpy(&ExpTimeZoneInfo,
	 TimeZoneInformation,
	 sizeof(TIME_ZONE_INFORMATION));

  /* Set the new time zone information */
  SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.u.HighPart;
  SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.u.HighPart;
  SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.u.LowPart;
  SharedUserData->TimeZoneId = ExpTimeZoneId;

  DPRINT("New time zone bias: %I64d minutes\n",
	 ExpTimeZoneBias.QuadPart / TICKSPERMINUTE);

  /* Calculate the new system time */
  ExLocalTimeToSystemTime(&LocalTime,
			  &SystemTime);

  /* Set the new system time */
  KiSetSystemTime(&SystemTime);

  DPRINT("ExpSetTimeZoneInformation() done\n");

  return STATUS_SUCCESS;
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
  LARGE_INTEGER OldSystemTime;
  LARGE_INTEGER NewSystemTime;
  LARGE_INTEGER LocalTime;
  TIME_FIELDS TimeFields;
  NTSTATUS Status;

  /* FIXME: Check for SeSystemTimePrivilege */

  Status = MmCopyFromCaller(&NewSystemTime, UnsafeNewSystemTime,
			    sizeof(NewSystemTime));
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (UnsafeOldSystemTime != NULL)
    {
      KeQuerySystemTime(&OldSystemTime);
    }
  ExSystemTimeToLocalTime(&NewSystemTime,
			  &LocalTime);
  RtlTimeToTimeFields(&LocalTime,
		      &TimeFields);
  HalSetRealTimeClock(&TimeFields);

  /* Set system time */
  KiSetSystemTime(&NewSystemTime);

  if (UnsafeOldSystemTime != NULL)
    {
      Status = MmCopyToCaller(UnsafeOldSystemTime, &OldSystemTime,
			      sizeof(OldSystemTime));
      if (!NT_SUCCESS(Status))
	{
          return Status;
	}
    }

  return STATUS_SUCCESS;
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
  SystemTime->QuadPart =
    LocalTime->QuadPart + ExpTimeZoneBias.QuadPart;
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
  LocalTime->QuadPart =
    SystemTime->QuadPart - ExpTimeZoneBias.QuadPart;
}

/* EOF */
