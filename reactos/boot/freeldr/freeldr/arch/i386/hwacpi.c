/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
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

#include <freeldr.h>
#include <arch.h>
#include <rtl.h>
#include <debug.h>
#include <mm.h>
#include <portio.h>

#include "../../reactos/registry.h"
#include "hardware.h"


static BOOL
FindAcpiBios(VOID)
{
  PU8 Ptr;

  /* Find the 'Root System Descriptor Table Pointer' */
  Ptr = (PU8)0xE0000;
  while ((U32)Ptr < 0x100000)
    {
      if (!memcmp(Ptr, "RSD PTR ", 8))
	{
	  DbgPrint((DPRINT_HWDETECT, "ACPI supported\n"));

	  return TRUE;
	}

      Ptr = (PU8)((U32)Ptr + 0x10);
    }

  DbgPrint((DPRINT_HWDETECT, "ACPI not supported\n"));

  return FALSE;
}


VOID
DetectAcpiBios(HKEY SystemKey, U32 *BusNumber)
{
  char Buffer[80];
  HKEY BiosKey;
  S32 Error;

  if (FindAcpiBios())
    {
      /* Create new bus key */
      sprintf(Buffer,
	      "MultifunctionAdapter\\%u", *BusNumber);
      Error = RegCreateKey(SystemKey,
			   Buffer,
			   &BiosKey);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", (int)Error));
	  return;
	}

#if 0
      /* Set 'Component Information' */
      SetComponentInformation(BiosKey,
                              0x0,
                              0x0,
                              0xFFFFFFFF);
#endif

      /* Increment bus number */
      (*BusNumber)++;

      /* Set 'Identifier' value */
      Error = RegSetValue(BiosKey,
			  "Identifier",
			  REG_SZ,
			  (PU8)"ACPI BIOS",
			  10);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  return;
	}

    }
}

/* EOF */
