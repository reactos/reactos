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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          GUI state check
 * FILE:             subsys/win32k/ntuser/guicheck.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:            The GuiCheck() function performs a few delayed operations:
 *                   1) A GUI process is assigned a window station
 *                   2) A message queue is created for a GUI thread before use
 *                   3) The system window classes are registered for a process
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static LONG NrGuiAppsRunning = 0;

/* FUNCTIONS *****************************************************************/

static BOOL FASTCALL
co_AddGuiApp(PPROCESSINFO W32Data)
{
   W32Data->W32PF_flags |= W32PF_CREATEDWINORDC;
   if (InterlockedIncrement(&NrGuiAppsRunning) == 1)
   {
      BOOL Initialized;

      Initialized = co_IntInitializeDesktopGraphics();

      if (!Initialized)
      {
         W32Data->W32PF_flags &= ~W32PF_CREATEDWINORDC;
         InterlockedDecrement(&NrGuiAppsRunning);
         return FALSE;
      }
   }
   return TRUE;
}

static void FASTCALL
RemoveGuiApp(PPROCESSINFO W32Data)
{
   W32Data->W32PF_flags &= ~W32PF_CREATEDWINORDC;
   if (InterlockedDecrement(&NrGuiAppsRunning) == 0)
   {
      IntEndDesktopGraphics();
   }
}

BOOL FASTCALL
co_IntGraphicsCheck(BOOL Create)
{
   PPROCESSINFO W32Data;

   W32Data = PsGetCurrentProcessWin32Process();
   if (Create)
   {
      if (! (W32Data->W32PF_flags & W32PF_CREATEDWINORDC) && ! (W32Data->W32PF_flags & W32PF_MANUALGUICHECK))
      {
         return co_AddGuiApp(W32Data);
      }
   }
   else
   {
      if ((W32Data->W32PF_flags & W32PF_CREATEDWINORDC) && ! (W32Data->W32PF_flags & W32PF_MANUALGUICHECK))
      {
         RemoveGuiApp(W32Data);
      }
   }

   return TRUE;
}

VOID
FASTCALL
IntUserManualGuiCheck(LONG Check)
{
   PPROCESSINFO W32Data;

   DPRINT("Enter IntUserManualGuiCheck\n");

   W32Data = PsGetCurrentProcessWin32Process();
   if (0 == Check)
   {
      W32Data->W32PF_flags |= W32PF_MANUALGUICHECK;
   }
   else if (0 < Check)
   {
      if (! (W32Data->W32PF_flags & W32PF_CREATEDWINORDC))
      {
         co_AddGuiApp(W32Data);
      }
   }
   else
   {
      if (W32Data->W32PF_flags & W32PF_CREATEDWINORDC)
      {
         RemoveGuiApp(W32Data);
      }
   }

   DPRINT("Leave IntUserManualGuiCheck\n");

}

NTSTATUS FASTCALL
InitGuiCheckImpl (VOID)
{
   return STATUS_SUCCESS;
}

/* EOF */
