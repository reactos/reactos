/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *
 *  $Id: painting.c 16320 2005-06-29 07:09:25Z navaraf $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             subsys/win32k/ntuser/ntuser.c
 *  REVISION HISTORY:
 *       16 July 2005   Created (hardon)
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

ERESOURCE UserLock;

/* FUNCTIONS **********************************************************/


NTSTATUS FASTCALL InitUserImpl(VOID)
{
   NTSTATUS Status;

   ExInitializeResourceLite(&UserLock);

   if (!ObmCreateHandleTable())
   {
      DPRINT1("Failed creating handle table\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Status = InitSessionImpl();
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Error init session impl.\n");
      return Status;
   }

   return STATUS_SUCCESS;
}

/*
RETURN
   True if current thread owns the lock (possibly shared)
*/
BOOL FASTCALL UserIsEntered()
{
   return ExIsResourceAcquiredExclusiveLite(&UserLock)
      || ExIsResourceAcquiredSharedLite(&UserLock);
}

BOOL FASTCALL UserIsEnteredExclusive()
{
   return ExIsResourceAcquiredExclusiveLite(&UserLock);
}

VOID FASTCALL CleanupUserImpl(VOID)
{
   ExDeleteResourceLite(&UserLock);
}

VOID FASTCALL UserEnterShared(VOID)
{
   KeEnterCriticalRegion();
   ExAcquireResourceSharedLite(&UserLock, TRUE);
}

VOID FASTCALL UserEnterExclusive(VOID)
{
   KeEnterCriticalRegion();
   ExAcquireResourceExclusiveLite(&UserLock, TRUE);
}

VOID FASTCALL UserLeave(VOID)
{
   ExReleaseResourceLite(&UserLock);
   KeLeaveCriticalRegion();
}
