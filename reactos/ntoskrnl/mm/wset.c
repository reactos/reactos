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
/* $Id: wset.c,v 1.11 2001/12/31 01:53:45 dwelch Exp $
 * 
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/wset.c
 * PURPOSE:         Manages working sets
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmTrimUserMemory(ULONG Target, ULONG Priority, PULONG NrFreedPages)
{
  PVOID CurrentPhysicalAddress;
  PVOID NextPhysicalAddress;
  NTSTATUS Status;

  (*NrFreedPages) = 0;

  CurrentPhysicalAddress = MmGetLRUFirstUserPage();
  while (CurrentPhysicalAddress != NULL && Target > 0)
    {
      NextPhysicalAddress = MmGetLRUNextUserPage(CurrentPhysicalAddress);

      Status = MmPageOutPhysicalAddress(CurrentPhysicalAddress);
      if (NT_SUCCESS(Status))
	{
	  DPRINT("Succeeded\n");
	  Target--;
	  (*NrFreedPages)++;
	}

      CurrentPhysicalAddress = NextPhysicalAddress;
    }
  return(STATUS_SUCCESS);
}
