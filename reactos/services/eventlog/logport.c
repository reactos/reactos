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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/eventlog/logport.c
 * PURPOSE:          Event logger service
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <string.h>

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
  LPC_MAX_MESSAGE Request;
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
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtAcceptConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  Status = NtCompleteConnectPort(MessagePortHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCompleteConnectPort() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

ByeBye:
  if (!NT_SUCCESS(Status))
    {
      if (ConnectPortHandle != NULL)
	NtClose(ConnectPortHandle);

      if (MessagePortHandle != NULL)
	NtClose(MessagePortHandle);
    }

  return Status;
}


static NTSTATUS
ProcessPortMessage(VOID)
{
  LPC_MAX_MESSAGE Request;
  PIO_ERROR_LOG_MESSAGE Message;
//#ifndef NDEBUG
  ULONG i;
  PWSTR p;
//#endif
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

      DPRINT("Received message\n");

      if (Request.Header.MessageType == LPC_PORT_CLOSED)
	{
	  DPRINT("Port closed\n");

	  return STATUS_SUCCESS;
	}
      if (Request.Header.MessageType == LPC_REQUEST)
	{
	  DPRINT("Received request\n");

	}
      else if (Request.Header.MessageType == LPC_DATAGRAM)
	{
	  DPRINT("Received datagram\n");


	  Message = (PIO_ERROR_LOG_MESSAGE)&Request.Data;

	  DPRINT("Message->Type %hx\n", Message->Type);
	  DPRINT("Message->Size %hu\n", Message->Size);

//#ifndef NDEBUG
	  DbgPrint("\n Error mesage:\n");
	  DbgPrint("Error code: %lx\n", Message->EntryData.ErrorCode);
	  DbgPrint("Retry count: %u\n", Message->EntryData.RetryCount);
	  DbgPrint("Sequence number: %lu\n", Message->EntryData.SequenceNumber);

	  if (Message->DriverNameLength != 0)
	    {
	      DbgPrint("Driver name: %.*S\n",
		       Message->DriverNameLength / sizeof(WCHAR),
		       (PWCHAR)((ULONG_PTR)Message + Message->DriverNameOffset));
	    }

	  if (Message->EntryData.NumberOfStrings != 0)
	    {
	      p = (PWSTR)((ULONG_PTR)&Message->EntryData + Message->EntryData.StringOffset);
	      for (i = 0; i < Message->EntryData.NumberOfStrings; i++)
		{
		  DbgPrint("String %lu: %S\n", i, p);
		  p += wcslen(p) + 1;
		}
	      DbgPrint("\n");
	    }

//#endif

	  /* FIXME: Enqueue message */

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
    return Status;

  while (NT_SUCCESS(Status))
    {
      Status = ProcessPortMessage();
    }

  if (ConnectPortHandle != NULL)
    NtClose(ConnectPortHandle);

  if (MessagePortHandle != NULL)
    NtClose(MessagePortHandle);

  return Status;
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

  return (PortThreadHandle != NULL);
}

/* EOF */
