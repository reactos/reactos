/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <fs.h>

#ifndef __LINUX_H
#define __LINUX_H


#define LINUX_LOADER_TYPE_LILO			0x01
#define LINUX_LOADER_TYPE_LOADLIN		0x11
#define LINUX_LOADER_TYPE_BOOTSECT		0x21
#define LINUX_LOADER_TYPE_SYSLINUX		0x31
#define	LINUX_LOADER_TYPE_ETHERBOOT		0x41
#define LINUX_LOADER_TYPE_FREELOADER	0x81

#define LINUX_COMMAND_LINE_MAGIC		0xA33F

#define LINUX_SETUP_HEADER_ID			0x53726448			// 'HdrS'

#define LINUX_BOOT_SECTOR_MAGIC			0xAA55

#define LINUX_KERNEL_LOAD_ADDRESS		0x100000

#define LINUX_FLAG_LOAD_HIGH			0x01
#define LINUX_FLAG_CAN_USE_HEAP			0x80

typedef struct
{
	U8		BootCode1[0x20];

	U16		CommandLineMagic;
	U16		CommandLineOffset;

	U8		BootCode2[0x1CD];

	U8		SetupSectors;
	U16		RootFlags;
	U16		SystemSize;
	U16		SwapDevice;
	U16		RamSize;
	U16		VideoMode;
	U16		RootDevice;
	U16		BootFlag;			// 0xAA55

} PACKED LINUX_BOOTSECTOR, *PLINUX_BOOTSECTOR;

typedef struct
{
	U8		JumpInstruction[2];
	U32		SetupHeaderSignature;	// Signature for SETUP-header
	U16		Version;				// Version number of header format
	U16		RealModeSwitch;			// Default switch
	U16		SetupSeg;				// SETUPSEG
	U16		StartSystemSeg;
	U16		KernelVersion;			// Offset to kernel version string
	U8		TypeOfLoader;			// Loader ID
									// =0, old one (LILO, Loadlin,
									//     Bootlin, SYSLX, bootsect...)
									// else it is set by the loader:
									// 0xTV: T=0 for LILO
									//       T=1 for Loadlin
									//       T=2 for bootsect-loader
									//       T=3 for SYSLX
									//       T=4 for ETHERBOOT
									//       V = version

	U8		LoadFlags;				// flags, unused bits must be zero (RFU)
									// LOADED_HIGH = 1
									// bit within loadflags,
									// if set, then the kernel is loaded high
									// CAN_USE_HEAP = 0x80
									// if set, the loader also has set heap_end_ptr
									// to tell how much space behind setup.S
									// can be used for heap purposes.
									// Only the loader knows what is free!

	U16		SetupMoveSize;			// size to move, when we (setup) are not
									// loaded at 0x90000. We will move ourselves
									// to 0x90000 then just before jumping into
									// the kernel. However, only the loader
									// know how much of data behind us also needs
									// to be loaded.

	U32		Code32Start;			// here loaders can put a different
									// start address for 32-bit code.
									// 
									//   0x1000 = default for zImage
									// 
									// 0x100000 = default for big kernel

	U32		RamdiskAddress;			// address of loaded ramdisk image
									// Here the loader (or kernel generator) puts
									// the 32-bit address were it loaded the image.
	U32		RamdiskSize;			// its size in bytes

	U16		BootSectKludgeOffset;
	U16		BootSectKludgeSegment;
	U16		HeapEnd;				// space from here (exclusive) down to
									// end of setup code can be used by setup
									// for local heap purposes.


} PACKED LINUX_SETUPSECTOR, *PLINUX_SETUPSECTOR;

VOID	BootNewLinuxKernel(VOID);				// Implemented in linux.S
VOID	BootOldLinuxKernel(U32 KernelSize);		// Implemented in linux.S

VOID	LoadAndBootLinux(PUCHAR OperatingSystemName, PUCHAR Description);

BOOL	LinuxParseIniSection(PUCHAR OperatingSystemName);
BOOL	LinuxReadBootSector(PFILE LinuxKernelFile);
BOOL	LinuxReadSetupSector(PFILE LinuxKernelFile);
BOOL	LinuxReadKernel(PFILE LinuxKernelFile);
BOOL	LinuxCheckKernelVersion(VOID);
BOOL	LinuxReadInitrd(PFILE LinuxInitrdFile);

#endif // defined __LINUX_H
