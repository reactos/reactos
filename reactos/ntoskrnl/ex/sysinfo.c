/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtQuerySystemEnvironmentValue(IN PUNICODE_STRING Name,
					       OUT PVOID Value,
					       ULONG Length,
					       PULONG ReturnLength)
{
   return(ZwQuerySystemEnvironmentValue(Name,
					Value,
					Length,
					ReturnLength));
}

NTSTATUS STDCALL ZwQuerySystemEnvironmentValue(IN PUNICODE_STRING Name,
					       OUT PVOID Value,
					       ULONG Length,
					       PULONG ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQuerySystemInformation(IN CINT SystemInformationClass,
					  OUT PVOID SystemInformation,
					  IN ULONG Length,
					  OUT PULONG ResultLength)
{
   return(ZwQuerySystemInformation(SystemInformationClass,
				   SystemInformation,
				   Length,
				   ResultLength));
}

NTSTATUS STDCALL ZwQuerySystemInformation(IN CINT SystemInformationClass,
					  OUT PVOID SystemInformation,
					  IN ULONG Length,
					  OUT PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetSystemEnvironmentValue(IN PUNICODE_STRING VariableName,
					     IN PUNICODE_STRING Value)
{
   return(ZwSetSystemEnvironmentValue(VariableName,Value));
}

NTSTATUS STDCALL ZwSetSystemEnvironmentValue(IN PUNICODE_STRING VariableName,
					     IN PUNICODE_STRING Value)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetSystemInformation(IN CINT SystemInformationClass,
					IN PVOID SystemInformation,
					IN ULONG SystemInformationLength)
{
   return(ZwSetSystemInformation(SystemInformationClass,
				 SystemInformation,
				 SystemInformationLength));
}

NTSTATUS STDCALL ZwSetSystemInformation(IN CINT SystemInformationClass,
					IN PVOID SystemInformation,
					IN ULONG SystemInformationLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtFlushInstructionCache(IN HANDLE ProcessHandle,
					 IN PVOID BaseAddress,
					 IN UINT NumberOfBytesToFlush)
{
   return(ZwFlushInstructionCache(ProcessHandle,
				  BaseAddress,
				  NumberOfBytesToFlush));
}

NTSTATUS STDCALL ZwFlushInstructionCache(IN HANDLE ProcessHandle,
					 IN PVOID BaseAddress,
					 IN UINT NumberOfBytesToFlush)
{
   UNIMPLEMENTED;
}
