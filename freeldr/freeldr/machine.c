/* $Id: machine.c,v 1.3 2004/11/10 23:45:37 gvg Exp $
 *
 *  FreeLoader
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

#include "freeldr.h"
#include "machine.h"

#undef MachClearScreenAttr
#undef MachPutChar
#undef MachPutCharAttrAtLoc
#undef MachGetMemoryMap
#undef MachDiskReadLogicalSectors
#undef MachDiskGetPartitionEntry

MACHVTBL MachVtbl;

void 
MachClearScreenAttr(U8 Attr)
{
  MachVtbl.ClearScreenAttr(Attr);
}

void
MachPutChar(int Ch)
{
  MachVtbl.PutChar(Ch);
}

void
MachPutCharAttrAtLoc(int Ch, U8 Attr, unsigned X, unsigned Y)
{
  MachVtbl.PutCharAttrAtLoc(Ch, Attr, X, Y);
}

U32
MachGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize)
{
  return MachVtbl.GetMemoryMap(BiosMemoryMap, MaxMemoryMapSize);
}

BOOL
MachDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{
  return MachVtbl.DiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

BOOL
MachDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  return MachVtbl.DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

/* EOF */
