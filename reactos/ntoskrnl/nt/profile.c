/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/Profile.c
 * PURPOSE:         
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *                  
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS 
STDCALL
NtCreateProfile(
	OUT PHANDLE ProfileHandle, 
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG ImageBase, 
	IN ULONG ImageSize, 
	IN ULONG Granularity,
	OUT PVOID Buffer, 
	IN ULONG ProfilingSize,
	IN ULONG ClockSource,
	IN ULONG ProcessorMask 
	)
{
}

NTSTATUS
STDCALL
NtQueryIntervalProfile(
	OUT PULONG Interval,
	OUT PULONG ClockSource
	)
{
}

NTSTATUS 
STDCALL
NtSetIntervalProfile(
	IN ULONG Interval,
	IN ULONG ClockSource
	)
{
}

NTSTATUS
STDCALL
NtStartProfile(
	IN HANDLE ProfileHandle
	)
{
}

NTSTATUS
STDCALL
NtStopProfile(
	IN HANDLE ProfileHandle
	)
{
}
