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

/* FUNCTIONS *****************************************************************/

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
   UNIMPLEMENTED;
}

VOID ExLocalTimeToSystemTime(PLARGE_INTEGER LocalTime, 
			     PLARGE_INTEGER SystemTime)
{
   UNIMPLEMENTED;
}

VOID ExSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
			     PLARGE_INTEGER LocalTime)
{
   UNIMPLEMENTED;
}
