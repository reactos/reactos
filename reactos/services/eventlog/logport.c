/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: logport.c,v 1.1 2002/06/25 21:10:14 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/eventlog/logport.c
 * PURPOSE:          Event logger service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define UNICODE

#define NTOS_MODE_USER
#include <ntos.h>
#include <napi/lpc.h>
#include <windows.h>

#include "eventlog.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

HANDLE PortThreadHandle = NULL;
HANDLE ConnectPortHandle = NULL;
HANDLE MessagePortHandle = NULL;


/* FUNCTIONS ****************************************************************/

static NTSTATUS
InitLogPort(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING PortName;
  LPC_MESSAGE Message;
  NTSTATUS Status;

  ConnectPortHandle = NULL;
  MessagePortHandle = NULL;

  RtlInitUnicodeString(&PortName,
		       L"\\ErrorLogPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &PortName,
			     0,
			     NULL,
			     NULL);

  Status = NtCreatePort(&ConnectPortHandle,
			&ObjectAttributes,
			0,
			0x100,
			0x2000);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  Message.DataSize = sizeof(LPC_MESSAGE);
  Message.MessageSize = 0;

  Status = NtListenPort(ConnectPortHandle,
			&Message);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  Status = NtAcceptConnectPort(&MessagePortHandle,
			       0,
			       &Message,
			       1,
			       0,
			       0);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  Status = NtCompleteConnectPort(MessagePortHandle);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

ByeBye:
  if (ConnectPortHandle != NULL)
    NtClose(ConnectPortHandle);

  if (MessagePortHandle != NULL)
    NtClose(MessagePortHandle);

  return(Status);
}


static NTSTATUS
ProcessPortMessage(VOID)
{
  PLPC_MAX_MESSAGE Request;
  LPC_MESSAGE Reply;
  BOOL ReplyReady = FALSE;
  NTSTATUS Status;

  Request = HeapAlloc(GetProcessHeap(),
		      HEAP_ZERO_MEMORY,
		      sizeof(LPC_MAX_MESSAGE));
  if (Request == NULL)
    return(STATUS_NO_MEMORY);

  while (TRUE)
    {
      Status = NtReplyWaitReceivePort(MessagePortHandle,
				      0,
				      (ReplyReady)? &Reply : NULL,
				      (PLPC_MESSAGE)Request);
      if (!NT_SUCCESS(Status))
	{
	  HeapFree(GetProcessHeap(),
		   0,
		   Request);
	  return(Status);
	}

      ReplyReady = FALSE;
      if (Request->Header.MessageType == LPC_REQUEST)
	{
	  DPRINT1("Received request\n");

	  ReplyReady = FALSE;
	}
      else if (Request->Header.MessageType == LPC_DATAGRAM)
	{
	  DPRINT1("Received datagram\n");
	}
    }
}


static NTSTATUS STDCALL
PortThreadRoutine(PVOID Param)
{
  NTSTATUS Status = STATUS_SUCCESS;

  Status = InitLogPort();
  if (!NT_SUCCESS(Status))
    return(Status);

  while (!NT_SUCCESS(Status))
    {
      Status = ProcessPortMessage();
    }

  return(Status);
}


BOOL
StartPortThread(VOID)
{
  DWORD ThreadId;

  PortThreadHandle = CreateThread(NULL,
				  0x1000,
				  PortThreadRoutine,
				  NULL,
				  0,
				  &ThreadId);

  return((PortThreadHandle != NULL));
}

/* EOF */
