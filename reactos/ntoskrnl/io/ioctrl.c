/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

ULONG IoGetFunctionCodeFromCtlCode(ULONG ControlCode)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtDeviceIoControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	)
{
}

NTSTATUS
STDCALL
ZwDeviceIoControlFile(
	IN HANDLE DeviceHandle,
	IN HANDLE Event OPTIONAL, 
	IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
	IN PVOID UserApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock, 
	IN ULONG IoControlCode,
	IN PVOID InputBuffer, 
	IN ULONG InputBufferSize,
	OUT PVOID OutputBuffer,
	IN ULONG OutputBufferSize
	)
{
}
