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

NTSTATUS STDCALL NtLockFile(IN  HANDLE FileHandle,
			    IN  HANDLE Event OPTIONAL,
			    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
			    IN  PVOID ApcContext OPTIONAL,
			    OUT PIO_STATUS_BLOCK IoStatusBlock,
			    IN  PLARGE_INTEGER ByteOffset,
			    IN  PLARGE_INTEGER Length,
			    IN  PULONG Key,
			    IN  BOOLEAN FailImmediatedly,
			    IN  BOOLEAN ExclusiveLock)
{
   return(ZwLockFile(FileHandle,
		     Event,
		     ApcRoutine,
		     ApcContext,
		     IoStatusBlock,
		     ByteOffset,
		     Length,
		     Key,
		     FailImmediatedly,
		     ExclusiveLock));
}

NTSTATUS STDCALL ZwLockFile(IN  HANDLE FileHandle,
			    IN  HANDLE Event OPTIONAL,
			    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
			    IN  PVOID ApcContext OPTIONAL,
			    OUT PIO_STATUS_BLOCK IoStatusBlock,
			    IN  PLARGE_INTEGER ByteOffset,
			    IN  PLARGE_INTEGER Length,
			    IN  PULONG Key,
			    IN  BOOLEAN FailImmediatedly,
			    IN  BOOLEAN ExclusiveLock)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtUnlockFile(IN HANDLE FileHandle,
			      OUT PIO_STATUS_BLOCK IoStatusBlock,
			      IN PLARGE_INTEGER ByteOffset,
			      IN PLARGE_INTEGER Length,
			      OUT PULONG Key OPTIONAL)
{
   return(ZwUnlockFile(FileHandle,
		       IoStatusBlock,
		       ByteOffset,
		       Length,
		       Key));
}

NTSTATUS STDCALL ZwUnlockFile(IN HANDLE FileHandle,
			      OUT PIO_STATUS_BLOCK IoStatusBlock,
			      IN PLARGE_INTEGER ByteOffset,
			      IN PLARGE_INTEGER Length,
			      OUT PULONG Key OPTIONAL)
{
   UNIMPLEMENTED;
}
