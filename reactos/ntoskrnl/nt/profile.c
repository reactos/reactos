/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/Profile.c
 * PURPOSE:         Support for profiling
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateProfile(OUT PHANDLE ProfileHandle, 
				 IN POBJECT_ATTRIBUTES ObjectAttributes,
				 IN ULONG ImageBase, 
				 IN ULONG ImageSize, 
				 IN ULONG Granularity,
				 OUT PVOID Buffer, 
				 IN ULONG ProfilingSize,
				 IN ULONG ClockSource,
				 IN ULONG ProcessorMask)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryIntervalProfile(OUT PULONG Interval,
					OUT PULONG ClockSource)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetIntervalProfile(IN ULONG Interval,
				      IN ULONG ClockSource)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtStartProfile(IN HANDLE ProfileHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtStopProfile(IN HANDLE ProfileHandle)
{
   UNIMPLEMENTED;
}
