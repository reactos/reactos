/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/dbgk/dbgkutil.c
 * PURPOSE:         User-Mode Debugging Support, Internal Debug Functions.
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
DbgkCreateThread(PVOID StartAddress)
{
#if 0
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
#endif
}

/* EOF */
