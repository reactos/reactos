/* $Id: machpc.h,v 1.4 2004/11/12 17:17:07 gvg Exp $
 *
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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

#ifndef __I386_MACHPC_H_
#define __I386_MACHPC_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

VOID PcMachInit(VOID);

VOID PcConsClearScreenAttr(U8 Attr);
VOID PcConsPutChar(int Ch);
VOID PcConsPutCharAttrAtLoc(int Ch, U8 Attr, unsigned X, unsigned Y);

U32 PcMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

BOOL PcDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer);
BOOL PcDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL PcDiskGetDriveGeometry(U32 DriveNumber, PGEOMETRY DriveGeometry);
U32 PcDiskGetCacheableBlockCount(U32 DriveNumber);

#endif /* __I386_MACHPC_H_ */

/* EOF */
