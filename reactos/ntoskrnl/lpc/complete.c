/* $Id: complete.c,v 1.1 2000/06/04 17:27:39 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/complete.c
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


/***********************************************************************
 * NAME							EXPORTED
 *	NtCompleteConnectPort@4
 *
 *
 */
EXPORTED
NTSTATUS
STDCALL
NtCompleteConnectPort (HANDLE PortHandle)
{
	NTSTATUS	Status;
	PEPORT		OurPort;
   
	DPRINT("NtCompleteConnectPort(PortHandle %x)\n", PortHandle);
   
	Status = ObReferenceObjectByHandle (
			PortHandle,
			PORT_ALL_ACCESS,
			ExPortType,
			UserMode,
			(PVOID *) & OurPort,
			NULL
			);
	if (!NT_SUCCESS(Status))
	{
		return (Status);
	}
   
	KeSetEvent (
		& OurPort->OtherPort->Event,
		IO_NO_INCREMENT,
		FALSE
		);
   
	OurPort->State = EPORT_CONNECTED_SERVER;
   
	ObDereferenceObject (OurPort);
   
	return (STATUS_SUCCESS);
}


/* EOF */
