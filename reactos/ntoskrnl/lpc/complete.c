/* $Id: complete.c,v 1.4 2001/06/23 19:13:33 phreak Exp $
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
EXPORTED NTSTATUS STDCALL
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
  
  KeReleaseSemaphore( &OurPort->OtherPort->Semaphore, IO_NO_INCREMENT, 1, FALSE );
   
  ObDereferenceObject (OurPort);
  
  return (STATUS_SUCCESS);
}


/* EOF */
