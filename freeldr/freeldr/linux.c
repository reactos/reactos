/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include "arch.h"
#include "miscboot.h"
#include "rtl.h"
#include "fs.h"
#include "ui.h"
#include "linux.h"
#include "debug.h"
#include "mm.h"
#include "inifile.h"

PLINUX_BOOTSECTOR	LinuxBootSector = NULL;
PLINUX_SETUPSECTOR	LinuxSetupSector = NULL;
ULONG				SetupSectorSize = 0;
BOOL				BigZImageKernel = TRUE;
ULONG				LinuxKernelSize = 0;
UCHAR				LinuxKernelName[260];
UCHAR				LinuxInitrdName[260];
BOOL				LinuxHasInitrd = FALSE;
UCHAR				LinuxCommandLine[260] = "";
ULONG				LinuxCommandLineSize = 0;

VOID LoadAndBootLinux(PUCHAR OperatingSystemName)
{
	PFILE	LinuxKernel = NULL;
	UCHAR	TempString[260];

	DrawBackdrop();

	// Parse the .ini file section
	if (!LinuxParseIniSection(OperatingSystemName))
	{
		goto LinuxBootFailed;
	}

	// Open the boot volume
	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		MessageBox("Failed to open boot drive.");
		goto LinuxBootFailed;
	}

	// Open the kernel
	LinuxKernel = OpenFile(LinuxKernelName);
	if (LinuxKernel == NULL)
	{
		sprintf(TempString, "Linux kernel \'%s\' not found.", LinuxKernelName);
		MessageBox(TempString);
		goto LinuxBootFailed;
	}

	// Read the boot sector
	if (!LinuxReadBootSector(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	// Read the setup sector
	if (!LinuxReadSetupSector(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	// Read the kernel
	if (!LinuxReadKernel(LinuxKernel))
	{
		goto LinuxBootFailed;
	}

	LinuxBootSector->CommandLineMagic = LINUX_COMMAND_LINE_MAGIC;
	LinuxBootSector->CommandLineOffset = 0x9000;

	LinuxSetupSector->TypeOfLoader = LINUX_LOADER_TYPE_FREELOADER;

	RtlCopyMemory((PVOID)0x90000, LinuxBootSector, 512);
	RtlCopyMemory((PVOID)0x90200, LinuxSetupSector, SetupSectorSize);
	RtlCopyMemory((PVOID)0x99000, LinuxCommandLine, LinuxCommandLineSize);

	showcursor();
	clrscr();

	stop_floppy();
	JumpToLinuxBootCode();


LinuxBootFailed:

	if (LinuxKernel != NULL)
	{
		CloseFile(LinuxKernel);
	}

	if (LinuxBootSector != NULL)
	{
		FreeMemory(LinuxBootSector);
	}
	if (LinuxSetupSector != NULL)
	{
		FreeMemory(LinuxSetupSector);
	}

	LinuxBootSector = NULL;
	LinuxSetupSector = NULL;
	SetupSectorSize = 0;
	BigZImageKernel = TRUE;
	LinuxKernelSize = 0;
	LinuxHasInitrd = FALSE;
	strcpy(LinuxCommandLine, "");
	LinuxCommandLineSize = 0;
}

BOOL LinuxParseIniSection(PUCHAR OperatingSystemName)
{
	UCHAR	SettingName[260];
	UCHAR	SettingValue[260];
	ULONG	SectionId;

	// Find all the message box settings and run them
	ShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		MessageBox(SettingName);
		return FALSE;
	}

	if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, 260))
	{
		MessageBox("Boot drive not specified for selected OS!");
		return FALSE;
	}

	BootDrive = atoi(SettingValue);

	BootPartition = 0;
	if (IniReadSettingByName(SectionId, "BootPartition", SettingValue, 260))
	{
		BootPartition = atoi(SettingValue);
	}

	// Get the kernel name
	if (!IniReadSettingByName(SectionId, "Kernel", LinuxKernelName, 260))
	{
		MessageBox("Linux kernel filename not specified for selected OS!");
		return FALSE;
	}

	// Get the initrd name
	if (IniReadSettingByName(SectionId, "Initrd", LinuxInitrdName, 260))
	{
		LinuxHasInitrd = TRUE;
	}

	// Get the command line
	if (IniReadSettingByName(SectionId, "CommandLine", LinuxCommandLine, 260))
	{
		LinuxCommandLineSize = strlen(LinuxCommandLine) + 1;
	}

	return TRUE;
}

BOOL LinuxReadBootSector(PFILE LinuxKernelFile)
{
	// Allocate memory for boot sector
	LinuxBootSector = (PLINUX_BOOTSECTOR)AllocateMemory(512);
	if (LinuxBootSector == NULL)
	{
		return FALSE;
	}

	// Read linux boot sector
	SetFilePointer(LinuxKernelFile, 0);
	if (!ReadFile(LinuxKernelFile, 512, NULL, LinuxBootSector))
	{
		return FALSE;
	}

	// Check for validity
	if (LinuxBootSector->BootFlag != LINUX_BOOT_SECTOR_MAGIC)
	{
		MessageBox("Invalid boot sector magic (0xaa55)");
		return FALSE;
	}

	DbgDumpBuffer(DPRINT_LINUX, LinuxBootSector, 512);

	DbgPrint((DPRINT_LINUX, "SetupSectors: %d\n", LinuxBootSector->SetupSectors));
	DbgPrint((DPRINT_LINUX, "RootFlags: 0x%x\n", LinuxBootSector->RootFlags));
	DbgPrint((DPRINT_LINUX, "SystemSize: 0x%x\n", LinuxBootSector->SystemSize));
	DbgPrint((DPRINT_LINUX, "SwapDevice: 0x%x\n", LinuxBootSector->SwapDevice));
	DbgPrint((DPRINT_LINUX, "RamSize: 0x%x\n", LinuxBootSector->RamSize));
	DbgPrint((DPRINT_LINUX, "VideoMode: 0x%x\n", LinuxBootSector->VideoMode));
	DbgPrint((DPRINT_LINUX, "RootDevice: 0x%x\n", LinuxBootSector->RootDevice));
	DbgPrint((DPRINT_LINUX, "BootFlag: 0x%x\n", LinuxBootSector->BootFlag));

	return TRUE;
}

BOOL LinuxReadSetupSector(PFILE LinuxKernelFile)
{
	SetupSectorSize = 512 * LinuxBootSector->SetupSectors;

	// Allocate memory for setup sectors
	LinuxSetupSector = (PLINUX_SETUPSECTOR)AllocateMemory(SetupSectorSize);
	if (LinuxSetupSector == NULL)
	{
		return FALSE;
	}

	// Read linux setup sectors
	SetFilePointer(LinuxKernelFile, 512);
	if (!ReadFile(LinuxKernelFile, SetupSectorSize, NULL, LinuxSetupSector))
	{
		return FALSE;
	}

	// Check for validity
	if (LinuxSetupSector->SetupHeaderSignature != LINUX_SETUP_HEADER_ID)
	{
		MessageBox("Invalid setup magic (HdrS)");
		return FALSE;
	}

	DbgDumpBuffer(DPRINT_LINUX, LinuxSetupSector, SetupSectorSize);

	DbgPrint((DPRINT_LINUX, "SetupHeaderSignature: 0x%x (HdrS)\n", LinuxSetupSector->SetupHeaderSignature));
	DbgPrint((DPRINT_LINUX, "Version: 0x%x\n", LinuxSetupSector->Version));
	DbgPrint((DPRINT_LINUX, "RealModeSwitch: 0x%x\n", LinuxSetupSector->RealModeSwitch));
	DbgPrint((DPRINT_LINUX, "SetupSeg: 0x%x\n", LinuxSetupSector->SetupSeg));
	DbgPrint((DPRINT_LINUX, "StartSystemSeg: 0x%x\n", LinuxSetupSector->StartSystemSeg));
	DbgPrint((DPRINT_LINUX, "KernelVersion: 0x%x\n", LinuxSetupSector->KernelVersion));
	DbgPrint((DPRINT_LINUX, "TypeOfLoader: 0x%x\n", LinuxSetupSector->TypeOfLoader));
	DbgPrint((DPRINT_LINUX, "LoadFlags: 0x%x\n", LinuxSetupSector->LoadFlags));
	DbgPrint((DPRINT_LINUX, "SetupMoveSize: 0x%x\n", LinuxSetupSector->SetupMoveSize));
	DbgPrint((DPRINT_LINUX, "Code32Start: 0x%x\n", LinuxSetupSector->Code32Start));
	DbgPrint((DPRINT_LINUX, "RamdiskAddress: 0x%x\n", LinuxSetupSector->RamdiskAddress));
	DbgPrint((DPRINT_LINUX, "RamdiskSize: 0x%x\n", LinuxSetupSector->RamdiskSize));
	DbgPrint((DPRINT_LINUX, "BootSectKludgeOffset: 0x%x\n", LinuxSetupSector->BootSectKludgeOffset));
	DbgPrint((DPRINT_LINUX, "BootSectKludgeSegment: 0x%x\n", LinuxSetupSector->BootSectKludgeSegment));
	DbgPrint((DPRINT_LINUX, "HeapEnd: 0x%x\n", LinuxSetupSector->HeapEnd));

	return TRUE;
}

BOOL LinuxReadKernel(PFILE LinuxKernelFile)
{
	PVOID	LoadAddress = (PVOID)LINUX_KERNEL_LOAD_ADDRESS;
	ULONG	BytesLoaded;
	UCHAR	StatusText[260];

	sprintf(StatusText, " Loading %s", LinuxKernelName);
	DrawStatusText(StatusText);
	DrawProgressBar(0);

	// Calc kernel size
	LinuxKernelSize = GetFileSize(LinuxKernelFile) - (512 + SetupSectorSize);

	// Read linux kernel to 0x100000 (1mb)
	SetFilePointer(LinuxKernelFile, 512 + SetupSectorSize);
	for (BytesLoaded=0; BytesLoaded<LinuxKernelSize; )
	{
		if (!ReadFile(LinuxKernelFile, 0x4000, NULL, LoadAddress))
		{
			return FALSE;
		}

		BytesLoaded += 0x4000;
		LoadAddress += 0x4000;

		DrawProgressBar( (BytesLoaded * 100) / LinuxKernelSize );
	}

	return TRUE;
}
