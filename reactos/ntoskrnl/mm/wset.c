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
/* $Id: wset.c,v 1.12.2.1 2002/05/13 20:37:01 chorns Exp $
 * 
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/wset.c
 * PURPOSE:         Manages working sets
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
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
MmTrimUserMemory(IN ULONG  Target,
  IN ULONG  Priority,
  IN PULONG  NrFreedPages)
{
#if 1
  ULONG_PTR CurrentPhysicalAddress;
  ULONG_PTR NextPhysicalAddress;
  BOOLEAN Modified;
#endif
  DPRINT("MmTrimUserMemory(Target %d)\n", Target);

  (*NrFreedPages) = 0;
#if 1
  CurrentPhysicalAddress = MmGetLRUFirstUserPage();
  while ((CurrentPhysicalAddress != 0) && (Target > 0))
    {
      NextPhysicalAddress = MmGetLRUNextUserPage(CurrentPhysicalAddress);

      if (MiGetLockCountPage(CurrentPhysicalAddress) == 0)
        {
          /* The physical page may be allocated, but not mapped into the
             memory of any process */
          if (MmGetRmapListHeadPage(CurrentPhysicalAddress) != NULL)
            {
              /* Reference the page so that it does not go away when
                 MmReleasePageMemoryConsumer() is called. If the page is
                 modified, it is put on the modified page list and eventually
                 it will be flushed to secondary storage. If the page is not
                 modified, it is put on the standby page list. It may happen
                 that the page is accessed when on the standby (or modified)
                 page list, in which case the page is moved from these page
                 lists to the working set it came from.
                 FIXME: If a modified page is repeatedly accessed, flushing
                 to secondary storage is postponed. This increases the chance
                 of data loss, so we may need to monitor this and flush the
                 page eventually (e.g. flush the page after having been on the
                 modified page list X times without ever being flushed). */
              MmReferencePage(CurrentPhysicalAddress);
		          MiTransitionAllRmaps(CurrentPhysicalAddress, TRUE, &Modified);
		          MiReclaimPage(CurrentPhysicalAddress, Modified);
              MmReleasePageMemoryConsumer(MC_USER, CurrentPhysicalAddress);
					    Target--;
					    (*NrFreedPages)++;
            }
        }

      CurrentPhysicalAddress = NextPhysicalAddress;
    }
#endif
  DPRINT("MmTrimUserMemory: NrFreedPages %d\n", *NrFreedPages);

  return(STATUS_SUCCESS);
}
