/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
		    Changed NtCreateMailslotFile
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreateMailslotFile (
	OUT	PHANDLE			MailSlotFileHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG			FileAttributes,
	IN	ULONG			ShareAccess,   
	IN	ULONG			MaxMessageSize,
	IN	PLARGE_INTEGER		TimeOut
	)
{
	UNIMPLEMENTED;
}
