/* $Id: close.c,v 1.6 2001/08/26 17:28:00 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/close.c
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


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID STDCALL
NiClosePort (PVOID	ObjectBody, ULONG	HandleCount)
{
  PEPORT Port = (PEPORT)ObjectBody;
  LPC_MESSAGE Message;
  
  /*
   * If the client has just closed its handle then tell the server what
   * happened and disconnect this port.
   */
  if (HandleCount == 0 && Port->State == EPORT_CONNECTED_CLIENT && 
      ObGetReferenceCount(Port) == 2)
    {
      Message.MessageSize = sizeof(LPC_MESSAGE);
      Message.DataSize = 0;
      EiReplyOrRequestPort (Port->OtherPort,
			    &Message,
			    LPC_PORT_CLOSED,
			    Port);
      Port->OtherPort->OtherPort = NULL;
      Port->OtherPort->State = EPORT_DISCONNECTED;
      KeReleaseSemaphore( &Port->OtherPort->Semaphore,
			  IO_NO_INCREMENT,
			  1,
			  FALSE );
      ObDereferenceObject (Port);
    }

  /*
   * If the server has closed all of its handles then disconnect the port,
   * don't actually notify the client until it attempts an operation.
   */
  if (HandleCount == 0 && Port->State == EPORT_CONNECTED_SERVER && 
      ObGetReferenceCount(Port) == 2)
    {
	Port->OtherPort->OtherPort = NULL;
	Port->OtherPort->State = EPORT_DISCONNECTED;
	ObDereferenceObject(Port->OtherPort);
     }
}


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID STDCALL
NiDeletePort (PVOID	ObjectBody)
{
   //   PEPORT Port = (PEPORT)ObjectBody;
   
   //   DPRINT1("Deleting port %x\n", Port);
}


/* EOF */
