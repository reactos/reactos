/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/user.c
 * PURPOSE:         User mode debugging
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Unchecked
 */

/* INCLUDES ******************************************************************/

#include <internal/ntoskrnl.h>
#include <napi/dbg.h>
#include <internal/ps.h>
#include <internal/dbg.h>

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
