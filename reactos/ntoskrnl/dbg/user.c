/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/user.c
 * PURPOSE:         User mode debugging
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
DbgkCreateThread(PVOID StartAddress)
{
  LPC_DBG_MESSAGE Message;
  LPC_DBG_MESSAGE Reply;
  NTSTATUS Status;

  if (PsGetCurrentThread()->ThreadsProcess->DebugPort == NULL)
    {
      return;
    }

  Message.Header.MessageSize = sizeof(LPC_DBG_MESSAGE);
  Message.Header.DataSize = sizeof(LPC_DBG_MESSAGE) - 
    sizeof(LPC_MESSAGE);
  Message.Type = DBG_EVENT_CREATE_THREAD;
  Message.Status = STATUS_SUCCESS;
  Message.Data.CreateThread.Reserved = 0;
  Message.Data.CreateThread.StartAddress = StartAddress;
  
  /* FIXME: Freeze all threads in process */

  /* Send the message to the process's debug port and wait for a reply */
  Status = 
    LpcSendDebugMessagePort(PsGetCurrentThread()->ThreadsProcess->DebugPort,
			    &Message,
			    &Reply);
  if (!NT_SUCCESS(Status))
    {
      return;
    }

  /* FIXME: Examine reply */
  return;
}

ULONG
DbgkForwardException(EXCEPTION_RECORD Er, ULONG FirstChance)
{
  LPC_DBG_MESSAGE Message;
  LPC_DBG_MESSAGE Reply;
  NTSTATUS Status;

  if (PsGetCurrentThread()->ThreadsProcess->DebugPort == NULL)
    {
      return(0);
    }

  Message.Header.MessageSize = sizeof(LPC_DBG_MESSAGE);
  Message.Header.DataSize = sizeof(LPC_DBG_MESSAGE) - 
    sizeof(LPC_MESSAGE);
  Message.Type = DBG_EVENT_EXCEPTION;
  Message.Status = STATUS_SUCCESS;
  Message.Data.Exception.ExceptionRecord = Er;
  Message.Data.Exception.FirstChance = FirstChance;
  
  /* FIXME: Freeze all threads in process */

  /* Send the message to the process's debug port and wait for a reply */
  Status = 
    LpcSendDebugMessagePort(PsGetCurrentThread()->ThreadsProcess->DebugPort,
			    &Message,
			    &Reply);
  if (!NT_SUCCESS(Status))
    {
      return(0);
    }

  /* FIXME: Examine reply */
  return(0);
}

/* EOF */
