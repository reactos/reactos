/* $Id: complete.c,v 1.11 2004/02/02 23:48:42 ea Exp $
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

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/***********************************************************************
 * NAME							EXPORTED
 *	NtCompleteConnectPort/1
 *
 * DESCRIPTION
 * 	Wake up the client thread that issued the NtConnectPort call
 * 	this server-side port was created for communicating with.
 *	To be used in LPC servers processes on reply ports only.
 *
 * ARGUMENTS
 *	hServerSideCommPort: a reply port handle returned by
 *	NtAcceptConnectPort.
 *
 * RETURN VALUE
 *	STATUS_SUCCESS or an error code from Ob.
 */
NTSTATUS STDCALL
NtCompleteConnectPort (HANDLE hServerSideCommPort)
{
  NTSTATUS	Status;
  PEPORT	ReplyPort;
  
  DPRINT("NtCompleteConnectPort(hServerSideCommPort %x)\n", hServerSideCommPort);
 
  /*
   * Ask Ob to translate the port handle to EPORT
   */
  Status = ObReferenceObjectByHandle (hServerSideCommPort,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&ReplyPort,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return (Status);
    }
  /*
   * Verify EPORT type is a server-side reply port;
   * otherwise tell the caller the port handle is not
   * valid.
   */
  if (ReplyPort->Type != EPORT_TYPE_SERVER_COMM_PORT) 
    {
       ObDereferenceObject (ReplyPort);
       return STATUS_INVALID_PORT_HANDLE;
    }
  
  ReplyPort->State = EPORT_CONNECTED_SERVER;
  /*
   * Wake up the client thread that issued NtConnectPort.
   */ 
  KeReleaseSemaphore(&ReplyPort->OtherPort->Semaphore, IO_NO_INCREMENT, 1, 
		     FALSE);
  /*
   * Tell Ob we are no more interested in ReplyPort
   */   
  ObDereferenceObject (ReplyPort);
  
  return (STATUS_SUCCESS);
}


/* EOF */
