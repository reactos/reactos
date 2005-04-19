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

extern void i386DivideByZero();
extern void i386DebugException();
extern void i386NMIException();
extern void i386Breakpoint();
extern void i386Overflow();
extern void i386BoundException();
extern void i386InvalidOpcode();
extern void i386FPUNotAvailable();
extern void i386DoubleFault();
extern void i386CoprocessorSegment();
extern void i386InvalidTSS();
extern void i386SegmentNotPresent();
extern void i386StackException();
extern void i386GeneralProtectionFault();
extern void i386PageFault();
extern void i386CoprocessorError();
extern void i386AlignmentCheck();
extern void i386MachineCheck();

extern void (*i386TrapSaveDRHook)(unsigned long *DRRegs);

extern ULONG i386BootDrive;
extern ULONG i386BootPartition;

extern BOOL i386DiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector,
                                  PULONGLONG SectorCount, int *FsType);
extern BOOL i386DiskGetSystemVolume(char *SystemPath, char *RemainingPath,
                                    PULONG Device, PULONG DriveNumber,
                                    PULONGLONG StartSector,
                                    PULONGLONG SectorCount, int *FsType);
extern BOOL i386DiskGetBootPath(char *BootPath, unsigned Size);
extern VOID i386DiskGetBootDevice(PULONG BootDevice);
extern BOOL i386DiskBootingFromFloppy(VOID);

#endif /* __I386_I386_H_ */

/* EOF */
