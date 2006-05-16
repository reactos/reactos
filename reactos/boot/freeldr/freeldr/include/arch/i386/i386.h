/*
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

#ifndef __I386_I386_H_
#define __I386_I386_H_


extern ULONG i386BootDrive;
extern ULONG i386BootPartition;

extern BOOLEAN i386DiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector,
                                  PULONGLONG SectorCount, int *FsType);
extern BOOLEAN i386DiskGetSystemVolume(char *SystemPath, char *RemainingPath,
                                    PULONG Device, PULONG DriveNumber,
                                    PULONGLONG StartSector,
                                    PULONGLONG SectorCount, int *FsType);
extern BOOLEAN i386DiskGetBootPath(char *BootPath, unsigned Size);
extern VOID i386DiskGetBootDevice(PULONG BootDevice);
extern BOOLEAN i386DiskBootingFromFloppy(VOID);
extern BOOLEAN i386DiskNormalizeSystemPath(char *SystemPath, unsigned Size);

#endif /* __I386_I386_H_ */

/* EOF */
