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
 */
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Thread/process cleanup
 * FILE:             subsys/win32k/ntuser/cleanup.c
 * PROGRAMER:        Gunnar
 * REVISION HISTORY:
 *
 */

/* INCLUDES ******************************************************************/


#include <ddk/ntddk.h>
#include <include/timer.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
W32kCreateProcessNotify(
   IN HANDLE  ParentId,
   IN HANDLE  ProcessId,
   IN BOOLEAN  Create
   )
{
   if (Create){
      return;
   }


}


VOID STDCALL
W32kCreateThreadNotify(
   IN HANDLE  ProcessId,
   IN HANDLE  ThreadId,
   IN BOOLEAN  Create
   )
{
   if (Create){
      return;
   }

   RemoveTimersThread(ThreadId);

}


NTSTATUS FASTCALL
InitCleanupImpl()
{

   NTSTATUS Status;
  
   Status = PsSetCreateThreadNotifyRoutine(W32kCreateThreadNotify);
   
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   Status = PsSetCreateProcessNotifyRoutine(W32kCreateProcessNotify,
                                            FALSE);
   return Status;
}
