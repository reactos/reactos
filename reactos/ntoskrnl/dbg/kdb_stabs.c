/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/kdb_stabs.c
 * PURPOSE:         Stabs functions...
 * 
 * PROGRAMMERS:     Gregor Anich (blight@blight.eu.org)
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

  StabEntry = SymbolInfo->StabsBase;
  StabsEnd = (PVOID)((ULONG_PTR)SymbolInfo->StabsBase + SymbolInfo->StabsLength);
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
