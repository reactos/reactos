/* $Id: create.c,v 1.1 2000/06/04 17:27:39 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/create.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


NTSTATUS
NiCreatePort (
	PVOID			ObjectBody,
	PVOID			Parent,
	PWSTR			RemainingPath,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	NTSTATUS	Status;
   
	if (RemainingPath == NULL)
	{
		return (STATUS_SUCCESS);
	}

	if (wcschr(RemainingPath+1, '\\') != NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
   
	Status = ObReferenceObjectByPointer (
			Parent,
			STANDARD_RIGHTS_REQUIRED,
			ObDirectoryType,
			UserMode
			);
	if (!NT_SUCCESS(Status))
	{
		return (Status);
	}
   
	ObAddEntryDirectory (
		Parent,
		ObjectBody,
		(RemainingPath + 1)
		);
	ObDereferenceObject (Parent);
   
	return (STATUS_SUCCESS);
}


EXPORTED
NTSTATUS
STDCALL
NtCreatePort (
	PHANDLE			PortHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	ULONG			MaxConnectInfoLength,
	ULONG			MaxDataLength,
	ULONG			Reserved
	)
{
	PEPORT		Port;
	NTSTATUS	Status;
   
	DPRINT("NtCreatePort() Name %x\n", ObjectAttributes->ObjectName->Buffer);
   
	Port = ObCreateObject (
			PortHandle,
			PORT_ALL_ACCESS,
			ObjectAttributes,
			ExPortType
			);
	if (Port == NULL)
	{
		return (STATUS_UNSUCCESSFUL);
	}
   
	Status = NiInitializePort (Port);
	Port->MaxConnectInfoLength = 260;
	Port->MaxDataLength = 328;
   
	ObDereferenceObject (Port);
   
	return (Status);
}


/* EOF */
