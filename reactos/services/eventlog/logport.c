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
/* $Id: logport.c,v 1.7 2003/11/20 11:09:49 ekohl Exp $
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
#include <rosrtl/string.h>

#include "eventlog.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

HANDLE PortThreadHandle = NULL;
HANDLE ConnectPortHandle = NULL;
HANDLE MessagePortHandle = NULL;


/* FUNCTIONS ****************************************************************/

static NTSTATUS
InitLogPort (VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING PortName;
  LPC_MAX_MESSAGE Request;
  NTSTATUS Status;

  ConnectPortHandle = NULL;
  MessagePortHandle = NULL;

  RtlRosInitUnicodeStringFromLiteral(&PortName,
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
    {
      DPRINT1("NtCreatePort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtListenPort(ConnectPortHandle,
			&Request.Header);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtListenPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtAcceptConnectPort(&MessagePortHandle,
			       ConnectPortHandle,
			       NULL,
			       TRUE,
			       NULL,
			       NULL);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1("NtAcceptConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtCompleteConnectPort (MessagePortHandle);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1("NtCompleteConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

ByeBye:
  if (!NT_SUCCESS (Status))
    {
      if (ConnectPortHandle != NULL)
	NtClose (ConnectPortHandle);

      if (MessagePortHandle != NULL)
	NtClose (MessagePortHandle);
    }

  return Status;
}


static NTSTATUS
ProcessPortMessage(VOID)
{
  LPC_MAX_MESSAGE Request;
  PIO_ERROR_LOG_MESSAGE Message;
  NTSTATUS Status;


  DPRINT1("ProcessPortMessage() called\n");

  Status = STATUS_SUCCESS;

  while (TRUE)
    {
      Status = NtReplyWaitReceivePort(MessagePortHandle,
				      0,
				      NULL,
				      &Request.Header);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtReplyWaitReceivePort() failed (Status %lx)\n", Status);
	  break;
	}

      DPRINT1 ("Received message\n");

      if (Request.Header.MessageType == LPC_PORT_CLOSED)
	{
	  DPRINT1 ("Port closed\n");

	  return (STATUS_UNSUCCESSFUL);
	}
      if (Request.Header.MessageType == LPC_REQUEST)
	{
	  DPRINT1("Received request\n");

	}
      else if (Request.Header.MessageType == LPC_DATAGRAM)
	{
	  DPRINT1 ("Received datagram\n");


	  Message = (PIO_ERROR_LOG_MESSAGE)&Request.Data;

	  DPRINT1("Message->Type %hx\n", Message->Type);
	  DPRINT1("Message->Size %hu\n", Message->Size);

	  DPRINT1("Sequence number %lx\n", Message->EntryData.SequenceNumber);

	}
    }

  return Status;
}


static NTSTATUS STDCALL
PortThreadRoutine(PVOID Param)
{
  NTSTATUS Status = STATUS_SUCCESS;

  Status = InitLogPort();
  if (!NT_SUCCESS(Status))
    return(Status);

  while (NT_SUCCESS(Status))
    {
      Status = ProcessPortMessage();
    }

  if (ConnectPortHandle != NULL)
    NtClose (ConnectPortHandle);

  if (MessagePortHandle != NULL)
    NtClose (MessagePortHandle);

  return(Status);
}


BOOL
StartPortThread(VOID)
{
  DWORD ThreadId;

  PortThreadHandle = CreateThread(NULL,
				  0x1000,
				  (LPTHREAD_START_ROUTINE)PortThreadRoutine,
				  NULL,
				  0,
				  &ThreadId);

  return((PortThreadHandle != NULL));
}

/* EOF */
