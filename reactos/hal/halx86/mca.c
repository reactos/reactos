/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: mca.c,v 1.2 2002/10/03 09:11:00 ekohl Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        hal/halx86/mca.c
 * PURPOSE:     Interfaces to the MicroChannel bus
 * PROGRAMMER:  Eric Kohl (ekohl@rz-online.de)
 */

/*
 * TODO:
 *   What Adapter ID is read from an empty slot?
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <bus.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

ULONG STDCALL
HalpGetMicroChannelData(PBUS_HANDLER BusHandler,
			ULONG BusNumber,
			ULONG SlotNumber,
			PVOID Buffer,
			ULONG Offset,
			ULONG Length)
{
  PCM_MCA_POS_DATA PosData = (PCM_MCA_POS_DATA)Buffer;

  DPRINT("HalpGetMicroChannelData() called.\n");
  DPRINT("  BusNumber %lu\n", BusNumber);
  DPRINT("  SlotNumber %lu\n", SlotNumber);
  DPRINT("  Offset 0x%lx\n", Offset);
  DPRINT("  Length 0x%lx\n", Length);

  if ((BusNumber != 0) ||
      (SlotNumber == 0) || (SlotNumber > 8) ||
      (Length < sizeof(CM_MCA_POS_DATA)))
    return(0);

  /* Enter Setup-Mode for given slot */
  WRITE_PORT_UCHAR((PUCHAR)0x96, ((UCHAR)(SlotNumber - 1) & 0x07) | 0x08);

  /* Read POS data */
  PosData->AdapterId = (READ_PORT_UCHAR((PUCHAR)0x101) << 8) +
			READ_PORT_UCHAR((PUCHAR)0x100);
  PosData->PosData1 = READ_PORT_UCHAR((PUCHAR)0x102);
  PosData->PosData2 = READ_PORT_UCHAR((PUCHAR)0x103);
  PosData->PosData3 = READ_PORT_UCHAR((PUCHAR)0x104);
  PosData->PosData4 = READ_PORT_UCHAR((PUCHAR)0x105);

  /* Leave Setup-Mode for given slot */
  WRITE_PORT_UCHAR((PUCHAR)0x96, (UCHAR)(SlotNumber - 1) & 0x07);

  return(sizeof(CM_MCA_POS_DATA));
}

/* EOF */
