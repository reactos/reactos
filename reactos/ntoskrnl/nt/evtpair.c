/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateEventPair(OUT PHANDLE EventPairHandle,
				   IN ACCESS_MASK DesiredAccess,
				   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwCreateEventPair(EventPairHandle,
			    DesiredAccess,
			    ObjectAttributes));
}

NTSTATUS STDCALL ZwCreateEventPair(OUT PHANDLE EventPairHandle,
				   IN ACCESS_MASK DesiredAccess,
				   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetHighEventPair(IN HANDLE EventPairHandle)
{
   return(ZwSetHighEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwSetHighEventPair(IN HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
   return(ZwSetHighWaitLowEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtSetLowEventPair(HANDLE EventPairHandle)
{
   return(ZwSetLowEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwSetLowEventPair(HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtSetLowWaitHighEventPair(HANDLE EventPairHandle)
{
   return(ZwSetLowWaitHighEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwSetLowWaitHighEventPair(HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
   return(ZwWaitLowEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwWaitLowEventPair(IN HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtOpenEventPair(OUT PHANDLE EventPairHandle,
				 IN ACCESS_MASK DesiredAccess,
				 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenEventPair(EventPairHandle,
			  DesiredAccess,
			  ObjectAttributes));
}

NTSTATUS STDCALL ZwOpenEventPair(OUT PHANDLE EventPairHandle,
				 IN ACCESS_MASK DesiredAccess,
				 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
   return(ZwWaitHighEventPair(EventPairHandle));
}

NTSTATUS STDCALL ZwWaitHighEventPair(IN HANDLE EventPairHandle)
{
   UNIMPLEMENTED;
}
