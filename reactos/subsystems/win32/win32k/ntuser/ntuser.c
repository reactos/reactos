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
BOOL gbInitialized;

BOOL
InitSysParams();

/* FUNCTIONS **********************************************************/


NTSTATUS FASTCALL InitUserImpl(VOID)
{
   NTSTATUS Status;

   ExInitializeResourceLite(&UserLock);

   if (!UserCreateHandleTable())
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

   if (!gpsi)
   {
      gpsi = UserHeapAlloc(sizeof(SERVERINFO));
      if (gpsi)
      {
         RtlZeroMemory(gpsi, sizeof(SERVERINFO));
         DPRINT("Global Server Data -> %x\n", gpsi);
      }
   }

   InitSysParams();

   return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
UserInitialize(
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
// Set W32PF_Flags |= (W32PF_READSCREENACCESSGRANTED | W32PF_IOWINSTA)
// Create Object Directory,,, Looks like create workstation. "\\Windows\\WindowStations"
// Create Event for Diconnect Desktop.
// Initialize Video.
// {
//     DrvInitConsole.
//     DrvChangeDisplaySettings.
//     Update Shared Device Caps.
//     Initialize User Screen.
// }
// Create ThreadInfo for this Thread!
// Set Global SERVERINFO Error flags.
// Load Resources.

    NtUserUpdatePerUserSystemParameters(0, TRUE);

    return STATUS_SUCCESS;
}

/*
    Called from win32csr.
 */
NTSTATUS
APIENTRY
NtUserInitialize(
  DWORD   dwWinVersion,
  HANDLE  hPowerRequestEvent,
  HANDLE  hMediaRequestEvent)
{
    NTSTATUS Status;

    DPRINT1("Enter NtUserInitialize(%lx, %p, %p)\n",
            dwWinVersion, hPowerRequestEvent, hMediaRequestEvent);

    /* Check the Windows version */
    if (dwWinVersion != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Acquire exclusive lock */
    UserEnterExclusive();

    /* Check if we are already initialized */
    if (gbInitialized)
    {
        UserLeave();
        return STATUS_UNSUCCESSFUL;
    }

// Initialize Power Request List.
// Initialize Media Change.
// InitializeGreCSRSS();
// {
//    Startup DxGraphics.
//    calls ** IntGdiGetLanguageID() and sets it **.
//    Enables Fonts drivers, Initialize Font table & Stock Fonts.
// }

    /* Initialize USER */
    Status = UserInitialize(hPowerRequestEvent, hMediaRequestEvent);

    /* Set us as initialized */
    gbInitialized = TRUE;

    /* Return */
    UserLeave();
    return Status;
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
