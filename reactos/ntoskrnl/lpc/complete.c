/* $Id: complete.c,v 1.6 2002/09/07 15:12:58 chorns Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/complete.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

/***********************************************************************
 * NAME							EXPORTED
 *	NtCompleteConnectPort@4
 *
 *
 */
NTSTATUS STDCALL
NtCompleteConnectPort (HANDLE PortHandle)
{
  NTSTATUS	Status;
  PEPORT		OurPort;
  
  DPRINT("NtCompleteConnectPort(PortHandle %x)\n", PortHandle);
  
  Status = ObReferenceObjectByHandle (PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&OurPort,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return (Status);
    }
  
  OurPort->State = EPORT_CONNECTED_SERVER;
  
  KeReleaseSemaphore(&OurPort->OtherPort->Semaphore, IO_NO_INCREMENT, 1, 
		     FALSE);
   
  ObDereferenceObject (OurPort);
  
  return (STATUS_SUCCESS);
}


/* EOF */
