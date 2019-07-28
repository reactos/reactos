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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

#ifdef _M_IX86

const CHAR BootSectorFilePrompt[] = "Enter the boot sector file path.\n\nExamples:\n\\BOOTSECT.DOS\n/boot/bootsect.dos";
const CHAR LinuxKernelPrompt[] = "Enter the Linux kernel image path.\n\nExamples:\n/vmlinuz\n/boot/vmlinuz-2.4.18";
const CHAR LinuxInitrdPrompt[] = "Enter the initrd image path.\n\nExamples:\n/initrd.gz\n/boot/root.img.gz\n\nLeave blank for no initial ram disk.";
const CHAR LinuxCommandLinePrompt[] = "Enter the Linux kernel command line.\n\nExamples:\nroot=/dev/hda1\nroot=/dev/fd0 read-only\nroot=/dev/sdb1 init=/sbin/init";

#endif // _M_IX86

const CHAR BootDrivePrompt[] = "Enter the boot drive.\n\nExamples:\nfd0 - first floppy drive\nhd0 - first hard drive\nhd1 - second hard drive\ncd0 - first CD-ROM drive.\n\nBIOS drive numbers may also be used:\n0 - first floppy drive\n0x80 - first hard drive\n0x81 - second hard drive";
const CHAR BootPartitionPrompt[] = "Enter the boot partition.\n\nEnter 0 for the active (bootable) partition.";
const CHAR ReactOSSystemPathPrompt[] = "Enter the path to your ReactOS system directory.\n\nExamples:\n\\REACTOS\n\\ROS";
const CHAR ReactOSOptionsPrompt[] = "Enter the options you want passed to the kernel.\n\nExamples:\n/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200\n/FASTDETECT /SOS /NOGUIBOOT\n/BASEVIDEO /MAXMEM=64\n/KERNEL=NTKRNLMP.EXE /HAL=HALMPS.DLL";
const CHAR CustomBootPrompt[] = "Press ENTER to boot your custom boot setup.";

/* FUNCTIONS ******************************************************************/

#ifdef HAS_OPTION_MENU_CUSTOM_BOOT

VOID OptionMenuCustomBoot(VOID)
{
    PCSTR CustomBootMenuList[] = {
#ifdef _M_IX86
        "Disk",
        "Partition",
        "Boot Sector File",
        "Linux",
#endif
        "ReactOS"
        };
    ULONG SelectedMenuItem;

    if (!UiDisplayMenu("Please choose a boot method:", NULL,
                       FALSE,
                       CustomBootMenuList,
                       sizeof(CustomBootMenuList) / sizeof(CustomBootMenuList[0]),
                       0, -1,
                       &SelectedMenuItem,
                       TRUE,
                       NULL, NULL))
    {
        /* The user pressed ESC */
        return;
    }

    switch (SelectedMenuItem)
    {
#ifdef _M_IX86
        case 0: // Disk
            EditCustomBootDisk(0);
            break;
        case 1: // Partition
            EditCustomBootPartition(0);
            break;
        case 2: // Boot Sector File
            EditCustomBootSectorFile(0);
            break;
        case 3: // Linux
            EditCustomBootLinux(0);
            break;
        case 4: // ReactOS
#else
        case 0:
#endif
            EditCustomBootReactOS(0);
            break;
    }
}

#endif // HAS_OPTION_MENU_CUSTOM_BOOT

#ifdef _M_IX86

VOID EditCustomBootDisk(IN ULONG_PTR SectionId OPTIONAL)
{
    TIMEINFO* TimeInfo;
    OperatingSystemItem OperatingSystem;
    CHAR SectionName[100];
    CHAR BootDriveString[20];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(BootDriveString, sizeof(BootDriveString));

    if (SectionId != 0)
    {
        /* Load the settings */
        IniReadSettingByName(SectionId, "BootDrive", BootDriveString, sizeof(BootDriveString));
    }

    if (!UiEditBox(BootDrivePrompt, BootDriveString, sizeof(BootDriveString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        IniModifySettingValue(SectionId, "BootDrive", BootDriveString);
        return;
    }

    /* Generate a unique section name */
    TimeInfo = ArcGetTime();
    RtlStringCbPrintfA(SectionName, sizeof(SectionName),
                       "CustomBootDisk%u%u%u%u%u%u",
                       TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                       TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

    /* Add the section */
    if (!IniAddSection(SectionName, &SectionId))
        return;

    /* Add the BootType */
    if (!IniAddSettingValueToSection(SectionId, "BootType", "Drive"))
        return;

    /* Add the BootDrive */
    if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
        return;

    UiMessageBox(CustomBootPrompt);

    OperatingSystem.SectionName = SectionName;
    OperatingSystem.LoadIdentifier = NULL;
    LoadOperatingSystem(&OperatingSystem);
}

VOID EditCustomBootPartition(IN ULONG_PTR SectionId OPTIONAL)
{
    TIMEINFO* TimeInfo;
    OperatingSystemItem OperatingSystem;
    CHAR SectionName[100];
    CHAR BootDriveString[20];
    CHAR BootPartitionString[20];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
    RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));

    if (SectionId != 0)
    {
        /* Load the settings */
        IniReadSettingByName(SectionId, "BootDrive", BootDriveString, sizeof(BootDriveString));
        IniReadSettingByName(SectionId, "BootPartition", BootPartitionString, sizeof(BootPartitionString));
    }

    if (!UiEditBox(BootDrivePrompt, BootDriveString, sizeof(BootDriveString)))
        return;

    if (!UiEditBox(BootPartitionPrompt, BootPartitionString, sizeof(BootPartitionString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        IniModifySettingValue(SectionId, "BootDrive", BootDriveString);
        IniModifySettingValue(SectionId, "BootPartition", BootPartitionString);
        return;
    }

    /* Generate a unique section name */
    TimeInfo = ArcGetTime();
    RtlStringCbPrintfA(SectionName, sizeof(SectionName),
                       "CustomBootPartition%u%u%u%u%u%u",
                       TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                       TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

    /* Add the section */
    if (!IniAddSection(SectionName, &SectionId))
        return;

    /* Add the BootType */
    if (!IniAddSettingValueToSection(SectionId, "BootType", "Partition"))
        return;

    /* Add the BootDrive */
    if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
        return;

    /* Add the BootPartition */
    if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
        return;

    UiMessageBox(CustomBootPrompt);

    OperatingSystem.SectionName = SectionName;
    OperatingSystem.LoadIdentifier = NULL;
    LoadOperatingSystem(&OperatingSystem);
}

VOID EditCustomBootSectorFile(IN ULONG_PTR SectionId OPTIONAL)
{
    TIMEINFO* TimeInfo;
    OperatingSystemItem OperatingSystem;
    CHAR SectionName[100];
    CHAR BootDriveString[20];
    CHAR BootPartitionString[20];
    CHAR BootSectorFileString[200];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
    RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
    RtlZeroMemory(BootSectorFileString, sizeof(BootSectorFileString));

    if (SectionId != 0)
    {
        /* Load the settings */
        IniReadSettingByName(SectionId, "BootDrive", BootDriveString, sizeof(BootDriveString));
        IniReadSettingByName(SectionId, "BootPartition", BootPartitionString, sizeof(BootPartitionString));
        IniReadSettingByName(SectionId, "BootSectorFile", BootSectorFileString, sizeof(BootSectorFileString));
    }

    if (!UiEditBox(BootDrivePrompt, BootDriveString, sizeof(BootDriveString)))
        return;

    if (!UiEditBox(BootPartitionPrompt, BootPartitionString, sizeof(BootPartitionString)))
        return;

    if (!UiEditBox(BootSectorFilePrompt, BootSectorFileString, sizeof(BootSectorFileString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        IniModifySettingValue(SectionId, "BootDrive", BootDriveString);
        IniModifySettingValue(SectionId, "BootPartition", BootPartitionString);
        IniModifySettingValue(SectionId, "BootSectorFile", BootSectorFileString);
        return;
    }

    /* Generate a unique section name */
    TimeInfo = ArcGetTime();
    RtlStringCbPrintfA(SectionName, sizeof(SectionName),
                       "CustomBootSectorFile%u%u%u%u%u%u",
                       TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                       TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

    /* Add the section */
    if (!IniAddSection(SectionName, &SectionId))
        return;

    /* Add the BootType */
    if (!IniAddSettingValueToSection(SectionId, "BootType", "BootSector"))
        return;

    /* Add the BootDrive */
    if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
        return;

    /* Add the BootPartition */
    if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
        return;

    /* Add the BootSectorFile */
    if (!IniAddSettingValueToSection(SectionId, "BootSectorFile", BootSectorFileString))
        return;

    UiMessageBox(CustomBootPrompt);

    OperatingSystem.SectionName = SectionName;
    OperatingSystem.LoadIdentifier = NULL;
    LoadOperatingSystem(&OperatingSystem);
}

VOID EditCustomBootLinux(IN ULONG_PTR SectionId OPTIONAL)
{
    TIMEINFO* TimeInfo;
    OperatingSystemItem OperatingSystem;
    CHAR SectionName[100];
    CHAR BootDriveString[20];
    CHAR BootPartitionString[20];
    CHAR LinuxKernelString[200];
    CHAR LinuxInitrdString[200];
    CHAR LinuxCommandLineString[200];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
    RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
    RtlZeroMemory(LinuxKernelString, sizeof(LinuxKernelString));
    RtlZeroMemory(LinuxInitrdString, sizeof(LinuxInitrdString));
    RtlZeroMemory(LinuxCommandLineString, sizeof(LinuxCommandLineString));

    if (SectionId != 0)
    {
        /* Load the settings */
        IniReadSettingByName(SectionId, "BootDrive", BootDriveString, sizeof(BootDriveString));
        IniReadSettingByName(SectionId, "BootPartition", BootPartitionString, sizeof(BootPartitionString));
        IniReadSettingByName(SectionId, "Kernel", LinuxKernelString, sizeof(LinuxKernelString));
        IniReadSettingByName(SectionId, "Initrd", LinuxInitrdString, sizeof(LinuxInitrdString));
        IniReadSettingByName(SectionId, "CommandLine", LinuxCommandLineString, sizeof(LinuxCommandLineString));
    }

    if (!UiEditBox(BootDrivePrompt, BootDriveString, sizeof(BootDriveString)))
        return;

    if (!UiEditBox(BootPartitionPrompt, BootPartitionString, sizeof(BootPartitionString)))
        return;

    if (!UiEditBox(LinuxKernelPrompt, LinuxKernelString, sizeof(LinuxKernelString)))
        return;

    if (!UiEditBox(LinuxInitrdPrompt, LinuxInitrdString, sizeof(LinuxInitrdString)))
        return;

    if (!UiEditBox(LinuxCommandLinePrompt, LinuxCommandLineString, sizeof(LinuxCommandLineString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        IniModifySettingValue(SectionId, "BootDrive", BootDriveString);
        IniModifySettingValue(SectionId, "BootPartition", BootPartitionString);
        IniModifySettingValue(SectionId, "Kernel", LinuxKernelString);
        IniModifySettingValue(SectionId, "Initrd", LinuxInitrdString);
        IniModifySettingValue(SectionId, "CommandLine", LinuxCommandLineString);
        return;
    }

    /* Generate a unique section name */
    TimeInfo = ArcGetTime();
    RtlStringCbPrintfA(SectionName, sizeof(SectionName),
                       "CustomLinux%u%u%u%u%u%u",
                       TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                       TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

    /* Add the section */
    if (!IniAddSection(SectionName, &SectionId))
        return;

    /* Add the BootType */
    if (!IniAddSettingValueToSection(SectionId, "BootType", "Linux"))
        return;

    /* Add the BootDrive */
    if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootDriveString))
        return;

    /* Add the BootPartition */
    if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootPartitionString))
        return;

    /* Add the Kernel */
    if (!IniAddSettingValueToSection(SectionId, "Kernel", LinuxKernelString))
        return;

    /* Add the Initrd */
    if (*LinuxInitrdString)
    {
        if (!IniAddSettingValueToSection(SectionId, "Initrd", LinuxInitrdString))
            return;
    }

    /* Add the CommandLine */
    if (!IniAddSettingValueToSection(SectionId, "CommandLine", LinuxCommandLineString))
        return;

    UiMessageBox(CustomBootPrompt);

    OperatingSystem.SectionName = SectionName;
    OperatingSystem.LoadIdentifier = "Custom Linux Setup";
    LoadOperatingSystem(&OperatingSystem);
}

#endif // _M_IX86

VOID EditCustomBootReactOS(IN ULONG_PTR SectionId OPTIONAL)
{
    TIMEINFO* TimeInfo;
    OperatingSystemItem OperatingSystem;
    CHAR SectionName[100];
    CHAR BootDriveString[20];
    CHAR BootPartitionString[20];
    CHAR ReactOSSystemPath[200];
    CHAR ReactOSARCPath[200];
    CHAR ReactOSOptions[200];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(BootDriveString, sizeof(BootDriveString));
    RtlZeroMemory(BootPartitionString, sizeof(BootPartitionString));
    RtlZeroMemory(ReactOSSystemPath, sizeof(ReactOSSystemPath));
    RtlZeroMemory(ReactOSARCPath, sizeof(ReactOSARCPath));
    RtlZeroMemory(ReactOSOptions, sizeof(ReactOSOptions));

    if (SectionId != 0)
    {
        /* Load the settings */
        // TODO? Maybe use DissectArcPath(CHAR *ArcPath, CHAR *BootPath, UCHAR* BootDrive, ULONG* BootPartition) to get back to the small elements.
        IniReadSettingByName(SectionId, "SystemPath", ReactOSARCPath, sizeof(ReactOSARCPath));
        IniReadSettingByName(SectionId, "Options", ReactOSOptions, sizeof(ReactOSOptions));
    }

    if (SectionId == 0)
    {
        if (!UiEditBox(BootDrivePrompt, BootDriveString, sizeof(BootDriveString)))
            return;

        if (!UiEditBox(BootPartitionPrompt, BootPartitionString, sizeof(BootPartitionString)))
            return;

        if (!UiEditBox(ReactOSSystemPathPrompt, ReactOSSystemPath, sizeof(ReactOSSystemPath)))
            return;
    }
    else
    {
        if (!UiEditBox(ReactOSSystemPathPrompt, ReactOSARCPath, sizeof(ReactOSARCPath)))
            return;
    }

    if (!UiEditBox(ReactOSOptionsPrompt, ReactOSOptions, sizeof(ReactOSOptions)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        IniModifySettingValue(SectionId, "SystemPath", ReactOSARCPath);
        IniModifySettingValue(SectionId, "Options", ReactOSOptions);
        return;
    }

    /* Generate a unique section name */
    TimeInfo = ArcGetTime();
    RtlStringCbPrintfA(SectionName, sizeof(SectionName),
                       "CustomReactOS%u%u%u%u%u%u",
                       TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                       TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);

    /* Add the section */
    if (!IniAddSection(SectionName, &SectionId))
        return;

    /* Add the BootType */
    if (!IniAddSettingValueToSection(SectionId, "BootType", "Windows2003"))
        return;

    /* Construct the ReactOS ARC system path */
    ConstructArcPath(ReactOSARCPath, ReactOSSystemPath, DriveMapGetBiosDriveNumber(BootDriveString), atoi(BootPartitionString));

    /* Add the system path */
    if (!IniAddSettingValueToSection(SectionId, "SystemPath", ReactOSARCPath))
        return;

    /* Add the CommandLine */
    if (!IniAddSettingValueToSection(SectionId, "Options", ReactOSOptions))
        return;

    UiMessageBox(CustomBootPrompt);

    OperatingSystem.SectionName = SectionName;
    OperatingSystem.LoadIdentifier = NULL;
    LoadOperatingSystem(&OperatingSystem);
}

#ifdef HAS_OPTION_MENU_REBOOT

VOID OptionMenuReboot(VOID)
{
    UiMessageBox("The system will now reboot.");

#if defined(__i386__) || defined(_M_AMD64)
    DiskStopFloppyMotor();
#endif
    Reboot();
}

#endif // HAS_OPTION_MENU_REBOOT
