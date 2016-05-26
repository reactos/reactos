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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(HWDETECT);

static BOOLEAN
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
      TRACE("Found APM BIOS\n");
      TRACE("AH: %x\n", RegsOut.b.ah);
      TRACE("AL: %x\n", RegsOut.b.al);
      TRACE("BH: %x\n", RegsOut.b.bh);
      TRACE("BL: %x\n", RegsOut.b.bl);
      TRACE("CX: %x\n", RegsOut.w.cx);

      return TRUE;
    }

  TRACE("No APM BIOS found\n");

  return FALSE;
}


VOID
DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    ULONG Size;

    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    if (FindApmBios())
    {
        /* Create 'Configuration Data' value */
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        memset(PartialResourceList, 0, Size);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 0;

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "APM",
                               PartialResourceList,
                               Size,
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }

    /* FIXME: Add configuration data */
}

/* EOF */
