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
	
#include <freeldr.h>
#include <rtl.h>
#include <ui.h>
#include <options.h>
#include <miscboot.h>
#include <debug.h>
#include <disk.h>
#include <arch.h>
#include <inifile.h>
#include <linux.h>
#include <reactos.h>
#include <drivemap.h>


UCHAR	BootDrivePrompt[] = "Enter the boot drive.\n\nExamples:\nfd0 - first floppy drive\nhd0 - first hard drive\nhd1 - second hard drive\ncd0 - first CD-ROM drive.\n\nBIOS drive numbers may also be used:\n0 - first floppy drive\n0x80 - first hard drive\n0x81 - second hard drive";
UCHAR	BootPartitionPrompt[] = "Enter the boot partition.\n\nEnter 0 for the active (bootable) partition.";
UCHAR	BootSectorFilePrompt[] = "Enter the boot sector file path.\n\nExamples:\n\\BOOTSECT.DOS\n/boot/bootsect.dos";
UCHAR	LinuxKernelPrompt[] = "Enter the Linux kernel image path.\n\nExamples:\n/vmlinuz\n/boot/vmlinuz-2.4.18";
UCHAR	LinuxInitrdPrompt[] = "Enter the initrd image path.\n\nExamples:\n/initrd.gz\n/boot/root.img.gz\n\nLeave blank for no initial ram disk.";
UCHAR	LinuxCommandLinePrompt[] = "Enter the Linux kernel command line.\n\nExamples:\nroot=/dev/hda1\nroot=/dev/fd0 read-only\nroot=/dev/sdb1 init=/sbin/init";
UCHAR	ReactOSSystemPathPrompt[] = "Enter the path to your ReactOS system directory.\n\nExamples:\n\\REACTOS\n\\ROS";
UCHAR	ReactOSOptionsPrompt[] = "Enter the options you want passed to the kernel.\n\nExamples:\n/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200\n/FASTDETECT /SOS /NOGUIBOOT\n/BASEVIDEO /MAXMEM=64\n/KERNEL=NTKRNLMP.EXE /HAL=HALMPS.DLL";

UCHAR	CustomBootPrompt[] = "Press ENTER to boot your custom boot setup.";

VOID OptionMenuCustomBoot(VOID)
{
	PUCHAR	CustomBootMenuList[] = { "Disk", "Partition", "Boot Sector File", "ReactOS", "Linux" };
	U32		CustomBootMenuCount = sizeof(CustomBootMenuList) / sizeof(CustomBootMenuList[0]);
	U32		SelectedMenuItem;

	if (!UiDisplayMenu(CustomBootMenuList, CustomBootMenuCount, 0, -1, &SelectedMenuItem, TRUE, NULL))
	{
		// The user pressed ESC
		return;
	}

	switch (SelectedMenuItem)
	{
	case 0: // Disk
		OptionMenuCustomBootDisk();
		break;
	case 1: // Partition
		OptionMenuCustomBootPartition();
		break;
	case 2: // Boot Sector File
		OptionMenuCustomBootBootSectorFile();
		break;
	case 3: // ReactOS
		OptionMenuCustomBootReactOS();
		break;
	case 4: // Linux
		OptionMenuCustomBootLinux();
		break;
	}
}

VOID OptionMenuCustomBootDisk(VOID)
{
	UCHAR	SectionName[100];
	UCHAR	BootDriveString[20];
	U32		SectionId;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	// Generate a unique section name
	sprintf(SectionName, "CustomBootDisk%d%d%d%d%d%d", getyear(), getday(), getmonth(), gethour(), getminute(), getsecond());

	// Add the section
	if (!IniAddSection(SectionName, &SectionId))
	{
		return;
	}

	// Add the BootType
	if (!IniAddSettingValueToSection(SectionId, "BootType", "Drive"))
	{
		return;
	}

	// Add the BootDrive
	if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
	{
		return;
	}

	UiMessageBox(CustomBootPrompt);

	LoadAndBootDrive(SectionName);
}

VOID OptionMenuCustomBootPartition(VOID)
{
	UCHAR	SectionName[100];
	UCHAR	BootDriveString[20];
	UCHAR	BootPartitionString[20];
	U32		SectionId;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
	RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	if (!UiEditBox(BootPartitionPrompt, BootPartitionString, 20))
	{
		return;
	}

	// Generate a unique section name
	sprintf(SectionName, "CustomBootPartition%d%d%d%d%d%d", getyear(), getday(), getmonth(), gethour(), getminute(), getsecond());

	// Add the section
	if (!IniAddSection(SectionName, &SectionId))
	{
		return;
	}

	// Add the BootType
	if (!IniAddSettingValueToSection(SectionId, "BootType", "Partition"))
	{
		return;
	}

	// Add the BootDrive
	if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
	{
		return;
	}

	// Add the BootPartition
	if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
	{
		return;
	}

	UiMessageBox(CustomBootPrompt);

	LoadAndBootPartition(SectionName);
}

VOID OptionMenuCustomBootBootSectorFile(VOID)
{
	UCHAR	SectionName[100];
	UCHAR	BootDriveString[20];
	UCHAR	BootPartitionString[20];
	UCHAR	BootSectorFileString[200];
	U32		SectionId;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
	RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
	RtlZeroMemory(BootSectorFileString, sizeof(BootSectorFileString));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	if (!UiEditBox(BootPartitionPrompt, BootPartitionString, 20))
	{
		return;
	}

	if (!UiEditBox(BootSectorFilePrompt, BootSectorFileString, 200))
	{
		return;
	}

	// Generate a unique section name
	sprintf(SectionName, "CustomBootSectorFile%d%d%d%d%d%d", getyear(), getday(), getmonth(), gethour(), getminute(), getsecond());

	// Add the section
	if (!IniAddSection(SectionName, &SectionId))
	{
		return;
	}

	// Add the BootType
	if (!IniAddSettingValueToSection(SectionId, "BootType", "BootSector"))
	{
		return;
	}

	// Add the BootDrive
	if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
	{
		return;
	}

	// Add the BootPartition
	if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
	{
		return;
	}

	// Add the BootSectorFile
	if (!IniAddSettingValueToSection(SectionId, "BootSectorFile", BootSectorFileString))
	{
		return;
	}

	UiMessageBox(CustomBootPrompt);

	LoadAndBootBootSector(SectionName);
}

VOID OptionMenuCustomBootReactOS(VOID)
{
	UCHAR	SectionName[100];
	UCHAR	BootDriveString[20];
	UCHAR	BootPartitionString[20];
	UCHAR	ReactOSSystemPath[200];
	UCHAR	ReactOSARCPath[200];
	UCHAR	ReactOSOptions[200];
	U32		SectionId;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
	RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
	RtlZeroMemory(ReactOSSystemPath, sizeof(ReactOSSystemPath));
	RtlZeroMemory(ReactOSOptions, sizeof(ReactOSOptions));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	if (!UiEditBox(BootPartitionPrompt, BootPartitionString, 20))
	{
		return;
	}

	if (!UiEditBox(ReactOSSystemPathPrompt, ReactOSSystemPath, 200))
	{
		return;
	}

	if (!UiEditBox(ReactOSOptionsPrompt, ReactOSOptions, 200))
	{
		return;
	}

	// Generate a unique section name
	sprintf(SectionName, "CustomReactOS%d%d%d%d%d%d", getyear(), getday(), getmonth(), gethour(), getminute(), getsecond());

	// Add the section
	if (!IniAddSection(SectionName, &SectionId))
	{
		return;
	}

	// Add the BootType
	if (!IniAddSettingValueToSection(SectionId, "BootType", "ReactOS"))
	{
		return;
	}

	// Construct the ReactOS ARC system path
	ConstructArcPath(ReactOSARCPath, ReactOSSystemPath, DriveMapGetBiosDriveNumber(BootDriveString), atoi(BootPartitionString));

	// Add the system path
	if (!IniAddSettingValueToSection(SectionId, "SystemPath", ReactOSARCPath))
	{
		return;
	}

	// Add the CommandLine
	if (!IniAddSettingValueToSection(SectionId, "Options", ReactOSOptions))
	{
		return;
	}

	UiMessageBox(CustomBootPrompt);

	LoadAndBootReactOS(SectionName);
}

VOID OptionMenuCustomBootLinux(VOID)
{
	UCHAR	SectionName[100];
	UCHAR	BootDriveString[20];
	UCHAR	BootPartitionString[20];
	UCHAR	LinuxKernelString[200];
	UCHAR	LinuxInitrdString[200];
	UCHAR	LinuxCommandLineString[200];
	U32		SectionId;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
	RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
	RtlZeroMemory(LinuxKernelString, sizeof(LinuxKernelString));
	RtlZeroMemory(LinuxInitrdString, sizeof(LinuxInitrdString));
	RtlZeroMemory(LinuxCommandLineString, sizeof(LinuxCommandLineString));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	if (!UiEditBox(BootPartitionPrompt, BootPartitionString, 20))
	{
		return;
	}

	if (!UiEditBox(LinuxKernelPrompt, LinuxKernelString, 200))
	{
		return;
	}

	if (!UiEditBox(LinuxInitrdPrompt, LinuxInitrdString, 200))
	{
		return;
	}

	if (!UiEditBox(LinuxCommandLinePrompt, LinuxCommandLineString, 200))
	{
		return;
	}

	// Generate a unique section name
	sprintf(SectionName, "CustomLinux%d%d%d%d%d%d", getyear(), getday(), getmonth(), gethour(), getminute(), getsecond());

	// Add the section
	if (!IniAddSection(SectionName, &SectionId))
	{
		return;
	}

	// Add the BootType
	if (!IniAddSettingValueToSection(SectionId, "BootType", "Linux"))
	{
		return;
	}

	// Add the BootDrive
	if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
	{
		return;
	}

	// Add the BootPartition
	if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
	{
		return;
	}

	// Add the Kernel
	if (!IniAddSettingValueToSection(SectionId, "Kernel", LinuxKernelString))
	{
		return;
	}

	// Add the Initrd
	if (strlen(LinuxInitrdString) > 0)
	{
		if (!IniAddSettingValueToSection(SectionId, "Initrd", LinuxInitrdString))
		{
			return;
		}
	}

	// Add the CommandLine
	if (!IniAddSettingValueToSection(SectionId, "CommandLine", LinuxCommandLineString))
	{
		return;
	}

	UiMessageBox(CustomBootPrompt);

	LoadAndBootLinux(SectionName, "Custom Linux Setup");
}
