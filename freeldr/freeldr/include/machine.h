/* $Id: machine.h,v 1.2 2004/11/09 23:36:20 gvg Exp $
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

#ifndef __MACHINE_H_
#define __MACHINE_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

typedef struct tagMACHVTBL
{
  VOID (*ClearScreenAttr)(U8 Attr);
  VOID (*PutChar)(int Ch);
  VOID (*PutCharAttrAtLoc)(int Ch, U8 Attr, unsigned X, unsigned Y);

  U32 (*GetMemoryMap)(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

  BOOL (*DiskReadLogicalSectors)(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer);
} MACHVTBL, *PMACHVTBL;

VOID MachInit(VOID);

extern MACHVTBL MachVtbl;

#define MachClearScreenAttr(Attr)		MachVtbl.ClearScreenAttr(Attr)
#define MachPutChar(Ch)				MachVtbl.PutChar(Ch)
#define MachPutCharAttrAtLoc(Ch, Attr, X, Y)	MachVtbl.PutCharAttrAtLoc((Ch), (Attr), (X), (Y))
#define MachGetMemoryMap(MMap, Size)		MachVtbl.GetMemoryMap((MMap), (Size))
#define MachDiskReadLogicalSectors(Drive, Start, Count, Buf)	MachVtbl.DiskReadLogicalSectors((Drive), (Start), (Count), (Buf))

#endif /* __MACHINE_H_ */

/* EOF */
