/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/iocomp.c
 * PURPOSE:         
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
		    Changed NtQueryIoCompletion 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateIoCompletion(OUT PHANDLE CompletionPort,
				      IN ACCESS_MASK DesiredAccess,
				      OUT PIO_STATUS_BLOCK IoStatusBlock,
				      IN ULONG NumberOfConcurrentThreads)
{
   return(ZwCreateIoCompletion(CompletionPort,
			       DesiredAccess,
			       IoStatusBlock,
			       NumberOfConcurrentThreads));
}

NTSTATUS STDCALL ZwCreateIoCompletion(OUT PHANDLE CompletionPort,
				      IN ACCESS_MASK DesiredAccess,
				      OUT PIO_STATUS_BLOCK IoStatusBlock,
				      IN ULONG NumberOfConcurrentThreads)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtOpenIoCompletion(OUT PHANDLE CompletionPort,
				    IN ACCESS_MASK DesiredAccess,
				    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenIoCompletion(CompletionPort,
			     DesiredAccess,
			     ObjectAttributes));
}

NTSTATUS STDCALL ZwOpenIoCompletion(OUT PHANDLE CompetionPort,
				    IN ACCESS_MASK DesiredAccess,
				    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtQueryIoCompletion(IN HANDLE CompletionPort,
				     IN ULONG CompletionKey,
				     OUT PIO_STATUS_BLOCK IoStatusBlock,
				     OUT PULONG NumberOfBytesTransferred)
{
   return(ZwQueryIoCompletion(CompletionPort,
			      CompletionKey,
			      IoStatusBlock,
			      NumberOfBytesTransferred));
}

NTSTATUS STDCALL ZwQueryIoCompletion(IN HANDLE CompletionPort,
				     IN ULONG CompletionKey,
				     OUT PIO_STATUS_BLOCK IoStatusBlock,
				     OUT PULONG NumberOfBytesTransferred)
{
   UNIMPLEMENTED;
}
   
NTSTATUS STDCALL NtRemoveIoCompletion(IN HANDLE CompletionPort,
				      OUT PULONG CompletionKey,
				      OUT PIO_STATUS_BLOCK IoStatusBlock,
				      OUT PULONG CompletionStatus,
				      PLARGE_INTEGER WaitTime)
{
   return(ZwRemoveIoCompletion(CompletionPort,
			       CompletionKey,
			       IoStatusBlock,
			       CompletionStatus,
			       WaitTime));
}

NTSTATUS STDCALL ZwRemoveIoCompletion(IN HANDLE CompletionPort,
				      OUT PULONG CompletionKey,
				      OUT PIO_STATUS_BLOCK IoStatusBlock,
				      OUT PULONG CompletionStatus,
				      PLARGE_INTEGER WaitTime)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetIoCompletion(IN HANDLE CompletionPort,
				   IN ULONG CompletionKey,
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN ULONG NumberOfBytesToTransfer, 
				   OUT PULONG NumberOfBytesTransferred)
{
   return(ZwSetIoCompletion(CompletionPort,
			    CompletionKey,
			    IoStatusBlock,
			    NumberOfBytesToTransfer,
			    NumberOfBytesTransferred));
}

NTSTATUS STDCALL ZwSetIoCompletion(IN HANDLE CompletionPort,
				   IN ULONG CompletionKey,
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN ULONG NumberOfBytesToTransfer, 
				   OUT PULONG NumberOfBytesTransferred)
{
   UNIMPLEMENTED;
}
