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
FindApmBios(VOID)
{
  REGS  RegsIn;
  REGS  RegsOut;

  RegsIn.b.ah = 0x53;
  RegsIn.b.al = 0x00;
  RegsIn.w.bx = 0x0000;

  Int386(0x15, &RegsIn, &RegsOut);

  if (INT386_SUCCESS(RegsOut))
    {
      DbgPrint((DPRINT_HWDETECT, "Found APM BIOS\n"));
      DbgPrint((DPRINT_HWDETECT, "AH: %x\n", RegsOut.b.ah));
      DbgPrint((DPRINT_HWDETECT, "AL: %x\n", RegsOut.b.al));
      DbgPrint((DPRINT_HWDETECT, "BH: %x\n", RegsOut.b.bh));
      DbgPrint((DPRINT_HWDETECT, "BL: %x\n", RegsOut.b.bl));
      DbgPrint((DPRINT_HWDETECT, "CX: %x\n", RegsOut.w.cx));

      return TRUE;
    }

  printf("No APM BIOS found\n");

  return FALSE;
}


VOID
DetectApmBios(HKEY SystemKey, U32 *BusNumber)
{
  char Buffer[80];
  HKEY BiosKey;
  S32 Error;

  if (FindApmBios())
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
			  (PU8)"APM",
			  4);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", (int)Error));
	  return;
	}

    }

  /* FIXME: Add congiguration data */
}

/* EOF */
