/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/dbg/kdb_stabs.c
 * PURPOSE:              Stabs functions...
 * PROGRAMMER:           Gregor Anich (blight@blight.eu.org)
 * REVISION HISTORY:
 *              2004/06/27: Created
 */

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/module.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/trap.h>
#include <ntdll/ldr.h>
#include <internal/safe.h>
#include <internal/kd.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <internal/debug.h>

#include "kdb.h"
#include "kdb_stabs.h"


/*! \brief Find a stab entry...
 *
 * Looks through the stab for an entry which matches the specified criteria.
 *
 * \param SymbolInfo       Pointer to the symbol info.
 * \param Type             Type of stab entry to look for.
 * \param RelativeAddress  Relative address of stab to look for.
 * \param StartEntry       Starting stab entry.
 *
 * \returns Pointer to a STAB_ENTRY
 * \retval NULL  No entry found.
 */
PSTAB_ENTRY
KdbpStabFindEntry(IN PIMAGE_SYMBOL_INFO SymbolInfo,
                  IN CHAR Type,
                  IN PVOID RelativeAddress  OPTIONAL,
                  IN PSTAB_ENTRY StartEntry  OPTIONAL)
{
  PSTAB_ENTRY StabEntry, BestStabEntry = NULL;
  PVOID StabsEnd;
  ULONG_PTR AddrFound = 0;

  StabEntry = SymbolInfo->SymbolsBase;
  StabsEnd = (PVOID)((ULONG_PTR)SymbolInfo->SymbolsBase + SymbolInfo->SymbolsLength);
  if (StartEntry != NULL)
    {
      ASSERT((ULONG_PTR)StartEntry >= (ULONG_PTR)StabEntry);
      if ((ULONG_PTR)StartEntry >= (ULONG_PTR)StabsEnd)
        return NULL;
      StabEntry = StartEntry;
    }

  if ( RelativeAddress != NULL )
  {
    for (; (ULONG_PTR)StabEntry < (ULONG_PTR)StabsEnd; StabEntry++)
    {
      ULONG_PTR SymbolRelativeAddress;

      if (StabEntry->n_type != Type)
        continue;

      if (RelativeAddress != NULL)
      {
        if (StabEntry->n_value >= SymbolInfo->ImageSize)
          continue;

        SymbolRelativeAddress = StabEntry->n_value;
        if ((SymbolRelativeAddress <= (ULONG_PTR)RelativeAddress) &&
            (SymbolRelativeAddress > AddrFound))
        {
          AddrFound = SymbolRelativeAddress;
          BestStabEntry = StabEntry;
        }
      }
    }
  }
  else
    BestStabEntry = StabEntry;

  if (BestStabEntry == NULL)
  {
    DPRINT("StabEntry not found!\n");
  }
  else
  {
    DPRINT("StabEntry found!\n");
  }

  return BestStabEntry;
}
