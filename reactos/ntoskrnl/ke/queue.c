/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: queue.c,v 1.2 2002/09/07 15:12:57 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 04/01/2002
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL
KeInitializeQueue(IN PKQUEUE Queue,
		  IN ULONG Count OPTIONAL)
{
  KeInitializeDispatcherHeader(&Queue->Header,
			       InternalQueueType,
			       sizeof(KQUEUE)/sizeof(ULONG),
			       0);
  InitializeListHead(&Queue->EntryListHead);
  InitializeListHead(&Queue->ThreadListHead);
  Queue->CurrentCount = 0;
  Queue->MaximumCount = (Count == 0) ? KeNumberProcessors : Count;
}


LONG STDCALL
KeReadStateQueue(IN PKQUEUE Queue)
{
  return(Queue->Header.SignalState);
}


LONG STDCALL
KeInsertHeadQueue(IN PKQUEUE Queue,
		  IN PLIST_ENTRY Entry)
{
  UNIMPLEMENTED;
  return 0;
}


LONG STDCALL
KeInsertQueue(IN PKQUEUE Queue,
	      IN PLIST_ENTRY Entry)
{
  UNIMPLEMENTED;
  return 0;
}


PLIST_ENTRY STDCALL
KeRemoveQueue(IN PKQUEUE Queue,
	      IN KPROCESSOR_MODE WaitMode,
	      IN PLARGE_INTEGER Timeout OPTIONAL)
{
  UNIMPLEMENTED;
  return NULL;
}


PLIST_ENTRY STDCALL
KeRundownQueue(IN PKQUEUE Queue)
{
  UNIMPLEMENTED;
  return NULL;
}

/* EOF */
