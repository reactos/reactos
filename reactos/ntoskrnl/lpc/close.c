/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/close.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
 */
VOID STDCALL
LpcpClosePort (PVOID	ObjectBody, ULONG	HandleCount)
{
  PEPORT Port = (PEPORT)ObjectBody;
  PORT_MESSAGE Message;

  /* FIXME Race conditions here! */

  DPRINT("NiClosePort 0x%p OtherPort 0x%p State %d\n", Port, Port->OtherPort, Port->State);

  /*
   * If the client has just closed its handle then tell the server what
   * happened and disconnect this port.
   */
  if (HandleCount == 1 && Port->State == EPORT_CONNECTED_CLIENT)
    {
      DPRINT("Informing server\n");
      Message.u1.s1.TotalLength = sizeof(PORT_MESSAGE);
      Message.u1.s1.DataLength = 0;
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
  if (HandleCount == 1 && Port->State == EPORT_CONNECTED_SERVER)
    {
        DPRINT("Cleaning up server\n");
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
 */
VOID STDCALL
LpcpDeletePort (PVOID	ObjectBody)
{
   //   PEPORT Port = (PEPORT)ObjectBody;

   //   DPRINT1("Deleting port %x\n", Port);
}


/* EOF */
