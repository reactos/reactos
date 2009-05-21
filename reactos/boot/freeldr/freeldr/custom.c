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


const CHAR	BootDrivePrompt[] = "Enter the boot drive.\n\nExamples:\nfd0 - first floppy drive\nhd0 - first hard drive\nhd1 - second hard drive\ncd0 - first CD-ROM drive.\n\nBIOS drive numbers may also be used:\n0 - first floppy drive\n0x80 - first hard drive\n0x81 - second hard drive";
const CHAR	BootPartitionPrompt[] = "Enter the boot partition.\n\nEnter 0 for the active (bootable) partition.";
const CHAR	BootSectorFilePrompt[] = "Enter the boot sector file path.\n\nExamples:\n\\BOOTSECT.DOS\n/boot/bootsect.dos";
const CHAR	LinuxKernelPrompt[] = "Enter the Linux kernel image path.\n\nExamples:\n/vmlinuz\n/boot/vmlinuz-2.4.18";
const CHAR 	LinuxInitrdPrompt[] = "Enter the initrd image path.\n\nExamples:\n/initrd.gz\n/boot/root.img.gz\n\nLeave blank for no initial ram disk.";
const CHAR	LinuxCommandLinePrompt[] = "Enter the Linux kernel command line.\n\nExamples:\nroot=/dev/hda1\nroot=/dev/fd0 read-only\nroot=/dev/sdb1 init=/sbin/init";
const CHAR	ReactOSSystemPathPrompt[] = "Enter the path to your ReactOS system directory.\n\nExamples:\n\\REACTOS\n\\ROS";
const CHAR	ReactOSOptionsPrompt[] = "Enter the options you want passed to the kernel.\n\nExamples:\n/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200\n/FASTDETECT /SOS /NOGUIBOOT\n/BASEVIDEO /MAXMEM=64\n/KERNEL=NTKRNLMP.EXE /HAL=HALMPS.DLL";

const CHAR	CustomBootPrompt[] = "Press ENTER to boot your custom boot setup.";

VOID OptionMenuCustomBoot(VOID)
{
	PCSTR	CustomBootMenuList[] = {
#ifdef __i386__
		"Disk",
		"Partition",
		"Boot Sector File",
		"ReactOS",
		"Linux"
#endif /* __i386__ */
		};
	ULONG		CustomBootMenuCount = sizeof(CustomBootMenuList) / sizeof(CustomBootMenuList[0]);
	ULONG		SelectedMenuItem;

	if (!UiDisplayMenu(CustomBootMenuList, CustomBootMenuCount, 0, -1, &SelectedMenuItem, TRUE, NULL))
	{
		// The user pressed ESC
		return;
	}

	switch (SelectedMenuItem)
	{
#ifdef __i386__
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
#endif /* __i386__ */
	}
}

#ifdef __i386__
VOID OptionMenuCustomBootDisk(VOID)
{
	CHAR	SectionName[100];
	CHAR	BootDriveString[20];
	ULONG	SectionId;
	TIMEINFO*	TimeInfo;

	RtlZeroMemory(SectionName, sizeof(SectionName));
	RtlZeroMemory(BootDriveString, sizeof(BootDriveString));

	if (!UiEditBox(BootDrivePrompt, BootDriveString, 20))
	{
		return;
	}

	// Generate a unique section name
	TimeInfo = ArcGetTime();
	sprintf(SectionName, "CustomBootDisk%u%u%u%u%u%u", TimeInfo->Year, TimeInfo->Day, TimeInfo->Month, TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

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
	CHAR	SectionName[100];
	CHAR	BootDriveString[20];
	CHAR	BootPartitionString[20];
	ULONG	SectionId;
	TIMEINFO*	TimeInfo;

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
	TimeInfo = ArcGetTime();
	sprintf(SectionName, "CustomBootPartition%u%u%u%u%u%u", TimeInfo->Year, TimeInfo->Day, TimeInfo->Month, TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

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
	CHAR	SectionName[100];
	CHAR	BootDriveString[20];
	CHAR	BootPartitionString[20];
	CHAR	BootSectorFileString[200];
	ULONG	SectionId;
	TIMEINFO*	TimeInfo;

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
	TimeInfo = ArcGetTime();
	sprintf(SectionName, "CustomBootSectorFile%u%u%u%u%u%u", TimeInfo->Year, TimeInfo->Day, TimeInfo->Month, TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

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
	CHAR	SectionName[100];
	CHAR	BootDriveString[20];
	CHAR	BootPartitionString[20];
	CHAR	ReactOSSystemPath[200];
	CHAR	ReactOSARCPath[200];
	CHAR	ReactOSOptions[200];
	ULONG	SectionId;
	TIMEINFO*	TimeInfo;

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
	TimeInfo = ArcGetTime();
	sprintf(SectionName, "CustomReactOS%u%u%u%u%u%u", TimeInfo->Year, TimeInfo->Day, TimeInfo->Month, TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

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
	CHAR	SectionName[100];
	CHAR	BootDriveString[20];
	CHAR	BootPartitionString[20];
	CHAR	LinuxKernelString[200];
	CHAR	LinuxInitrdString[200];
	CHAR	LinuxCommandLineString[200];
	ULONG	SectionId;
	TIMEINFO*	TimeInfo;

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
	TimeInfo = ArcGetTime();
	sprintf(SectionName, "CustomLinux%u%u%u%u%u%u", TimeInfo->Year, TimeInfo->Day, TimeInfo->Month, TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

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
#endif /* __i386__ */
