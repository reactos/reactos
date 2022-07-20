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

#if defined(_M_IX86) || defined(_M_AMD64)

const CHAR BootSectorFilePrompt[] = "Enter the boot sector file path.\n\nExamples:\n\\BOOTSECT.DOS\n/boot/bootsect.dos";
const CHAR LinuxKernelPrompt[] = "Enter the Linux kernel image path.\n\nExamples:\n/vmlinuz\n/boot/vmlinuz-2.4.18";
const CHAR LinuxInitrdPrompt[] = "Enter the initrd image path.\n\nExamples:\n/initrd.gz\n/boot/root.img.gz\n\nLeave blank for no initial ram disk.";
const CHAR LinuxCommandLinePrompt[] = "Enter the Linux kernel command line.\n\nExamples:\nroot=/dev/hda1\nroot=/dev/fd0 read-only\nroot=/dev/sdb1 init=/sbin/init";

#endif /* _M_IX86 || _M_AMD64 */

const CHAR BootDrivePrompt[] = "Enter the boot drive.\n\nExamples:\nfd0 - first floppy drive\nhd0 - first hard drive\nhd1 - second hard drive\ncd0 - first CD-ROM drive.\n\nBIOS drive numbers may also be used:\n0 - first floppy drive\n0x80 - first hard drive\n0x81 - second hard drive";
const CHAR BootPartitionPrompt[] = "Enter the boot partition.\n\nEnter 0 for the active (bootable) partition.";
const CHAR ARCPathPrompt[] = "Enter the boot ARC path.\n\nExamples:\nmulti(0)disk(0)rdisk(0)partition(1)\nmulti(0)disk(0)fdisk(0)";
const CHAR ReactOSSystemPathPrompt[] = "Enter the path to your ReactOS system directory.\n\nExamples:\n\\REACTOS\n\\ROS";
const CHAR ReactOSOptionsPrompt[] = "Enter the load options you want passed to the kernel.\n\nExamples:\n/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200\n/FASTDETECT /SOS /NOGUIBOOT\n/BASEVIDEO /MAXMEM=64\n/KERNEL=NTKRNLMP.EXE /HAL=HALMPS.DLL";
const CHAR ReactOSSetupOptionsPrompt[] = "Enter additional load options you want passed to the ReactOS Setup.\nThese options will supplement those obtained from the TXTSETUP.SIF\nfile, unless you also specify the /SIFOPTIONSOVERRIDE option switch.\n\nExample:\n/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /NOGUIBOOT";
const CHAR CustomBootPrompt[] = "Press ENTER to boot your custom boot setup.";

/* FUNCTIONS ******************************************************************/

#ifdef HAS_OPTION_MENU_CUSTOM_BOOT

VOID OptionMenuCustomBoot(VOID)
{
    PCSTR CustomBootMenuList[] = {
#if defined(_M_IX86) || defined(_M_AMD64)
        "Disk",
        "Partition",
        "Boot Sector File",
        "Linux",
#endif
        "ReactOS",
        "ReactOS Setup"
        };
    ULONG SelectedMenuItem;
    OperatingSystemItem OperatingSystem;

    if (!UiDisplayMenu("Please choose a boot method:", NULL,
                       FALSE,
                       CustomBootMenuList,
                       RTL_NUMBER_OF(CustomBootMenuList),
                       0, -1,
                       &SelectedMenuItem,
                       TRUE,
                       NULL, NULL))
    {
        /* The user pressed ESC */
        return;
    }

    /* Initialize a new custom OS entry */
    OperatingSystem.SectionId = 0;
    switch (SelectedMenuItem)
    {
#if defined(_M_IX86) || defined(_M_AMD64)
        case 0: // Disk
            EditCustomBootDisk(&OperatingSystem);
            break;
        case 1: // Partition
            EditCustomBootPartition(&OperatingSystem);
            break;
        case 2: // Boot Sector File
            EditCustomBootSectorFile(&OperatingSystem);
            break;
        case 3: // Linux
            EditCustomBootLinux(&OperatingSystem);
            break;
        case 4: // ReactOS
            EditCustomBootReactOS(&OperatingSystem, FALSE);
            break;
        case 5: // ReactOS Setup
            EditCustomBootReactOS(&OperatingSystem, TRUE);
            break;
#else
        case 0: // ReactOS
            EditCustomBootReactOS(&OperatingSystem, FALSE);
            break;
        case 1: // ReactOS Setup
            EditCustomBootReactOS(&OperatingSystem, TRUE);
            break;
#endif /* _M_IX86 || _M_AMD64 */
    }

    /* And boot it */
    if (OperatingSystem.SectionId != 0)
    {
        UiMessageBox(CustomBootPrompt);
        LoadOperatingSystem(&OperatingSystem);
    }
}

#endif // HAS_OPTION_MENU_CUSTOM_BOOT

#if defined(_M_IX86) || defined(_M_AMD64)

VOID
EditCustomBootDisk(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    TIMEINFO* TimeInfo;
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    CHAR SectionName[100];
    /* This construct is a trick for saving some stack space */
    union
    {
        struct
        {
            CHAR Guard1;
            CHAR Drive[20];
            CHAR Guard2;
        };
        CHAR ArcPath[200];
    } BootStrings;

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(&BootStrings, sizeof(BootStrings));

    if (SectionId != 0)
    {
        /* Load the settings */

        /* Check whether we have a "BootPath" value (takes precedence over "BootDrive") */
        *BootStrings.ArcPath = ANSI_NULL;
        IniReadSettingByName(SectionId, "BootPath", BootStrings.ArcPath, sizeof(BootStrings.ArcPath));
        if (!*BootStrings.ArcPath)
        {
            /* We don't, retrieve the boot drive value instead */
            IniReadSettingByName(SectionId, "BootDrive", BootStrings.Drive, sizeof(BootStrings.Drive));
        }
    }

    if (!*BootStrings.ArcPath)
    {
        if (!UiEditBox(BootDrivePrompt, BootStrings.Drive, sizeof(BootStrings.Drive)))
            return;
    }
    if (!*BootStrings.Drive)
    {
        if (!UiEditBox(ARCPathPrompt, BootStrings.ArcPath, sizeof(BootStrings.ArcPath)))
            return;
    }

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        /* Modify the BootPath if we have one */
        if (*BootStrings.ArcPath)
        {
            IniModifySettingValue(SectionId, "BootPath", BootStrings.ArcPath);
        }
        else if (*BootStrings.Drive)
        {
            /* Otherwise, modify the BootDrive */
            IniModifySettingValue(SectionId, "BootDrive", BootStrings.Drive);
        }
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

    /* Add the BootPath if we have one */
    if (*BootStrings.ArcPath)
    {
        if (!IniAddSettingValueToSection(SectionId, "BootPath", BootStrings.ArcPath))
            return;
    }
    else if (*BootStrings.Drive)
    {
        /* Otherwise, add the BootDrive */
        if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootStrings.Drive))
            return;
    }

    OperatingSystem->SectionId = SectionId;
    OperatingSystem->LoadIdentifier = NULL;
}

VOID
EditCustomBootPartition(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    TIMEINFO* TimeInfo;
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    CHAR SectionName[100];
    /* This construct is a trick for saving some stack space */
    union
    {
        struct
        {
            CHAR Guard1;
            CHAR Drive[20];
            CHAR Partition[20];
            CHAR Guard2;
        };
        CHAR ArcPath[200];
    } BootStrings;

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(&BootStrings, sizeof(BootStrings));

    if (SectionId != 0)
    {
        /* Load the settings */

        /*
         * Check whether we have a "BootPath" value (takes precedence
         * over both "BootDrive" and "BootPartition").
         */
        *BootStrings.ArcPath = ANSI_NULL;
        IniReadSettingByName(SectionId, "BootPath", BootStrings.ArcPath, sizeof(BootStrings.ArcPath));
        if (!*BootStrings.ArcPath)
        {
            /* We don't, retrieve the boot drive and partition values instead */
            IniReadSettingByName(SectionId, "BootDrive", BootStrings.Drive, sizeof(BootStrings.Drive));
            IniReadSettingByName(SectionId, "BootPartition", BootStrings.Partition, sizeof(BootStrings.Partition));
        }
    }

    if (!*BootStrings.ArcPath)
    {
        if (!UiEditBox(BootDrivePrompt, BootStrings.Drive, sizeof(BootStrings.Drive)))
            return;

        if (*BootStrings.Drive)
        {
            if (!UiEditBox(BootPartitionPrompt, BootStrings.Partition, sizeof(BootStrings.Partition)))
                return;
        }
    }
    if (!*BootStrings.Drive)
    {
        if (!UiEditBox(ARCPathPrompt, BootStrings.ArcPath, sizeof(BootStrings.ArcPath)))
            return;
    }

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        /* Modify the BootPath if we have one */
        if (*BootStrings.ArcPath)
        {
            IniModifySettingValue(SectionId, "BootPath", BootStrings.ArcPath);
        }
        else if (*BootStrings.Drive)
        {
            /* Otherwise, modify the BootDrive and BootPartition */
            IniModifySettingValue(SectionId, "BootDrive", BootStrings.Drive);
            IniModifySettingValue(SectionId, "BootPartition", BootStrings.Partition);
        }
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

    /* Add the BootPath if we have one */
    if (*BootStrings.ArcPath)
    {
        if (!IniAddSettingValueToSection(SectionId, "BootPath", BootStrings.ArcPath))
            return;
    }
    else if (*BootStrings.Drive)
    {
        /* Otherwise, add the BootDrive and BootPartition */
        if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootStrings.Drive))
            return;

        if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootStrings.Partition))
            return;
    }

    OperatingSystem->SectionId = SectionId;
    OperatingSystem->LoadIdentifier = NULL;
}

VOID
EditCustomBootSectorFile(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    TIMEINFO* TimeInfo;
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    CHAR SectionName[100];
    /* This construct is a trick for saving some stack space */
    union
    {
        struct
        {
            CHAR Guard1;
            CHAR Drive[20];
            CHAR Partition[20];
            CHAR Guard2;
        };
        CHAR ArcPath[200];
    } BootStrings;
    CHAR BootSectorFileString[200];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(&BootStrings, sizeof(BootStrings));
    RtlZeroMemory(BootSectorFileString, sizeof(BootSectorFileString));

    if (SectionId != 0)
    {
        /* Load the settings */

        /*
         * Check whether we have a "BootPath" value (takes precedence
         * over both "BootDrive" and "BootPartition").
         */
        *BootStrings.ArcPath = ANSI_NULL;
        IniReadSettingByName(SectionId, "BootPath", BootStrings.ArcPath, sizeof(BootStrings.ArcPath));
        if (!*BootStrings.ArcPath)
        {
            /* We don't, retrieve the boot drive and partition values instead */
            IniReadSettingByName(SectionId, "BootDrive", BootStrings.Drive, sizeof(BootStrings.Drive));
            IniReadSettingByName(SectionId, "BootPartition", BootStrings.Partition, sizeof(BootStrings.Partition));
        }

        IniReadSettingByName(SectionId, "BootSectorFile", BootSectorFileString, sizeof(BootSectorFileString));
    }

    if (!*BootStrings.ArcPath)
    {
        if (!UiEditBox(BootDrivePrompt, BootStrings.Drive, sizeof(BootStrings.Drive)))
            return;

        if (*BootStrings.Drive)
        {
            if (!UiEditBox(BootPartitionPrompt, BootStrings.Partition, sizeof(BootStrings.Partition)))
                return;
        }
    }
    if (!*BootStrings.Drive)
    {
        if (!UiEditBox(ARCPathPrompt, BootStrings.ArcPath, sizeof(BootStrings.ArcPath)))
            return;
    }

    if (!UiEditBox(BootSectorFilePrompt, BootSectorFileString, sizeof(BootSectorFileString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        /* Modify the BootPath if we have one */
        if (*BootStrings.ArcPath)
        {
            IniModifySettingValue(SectionId, "BootPath", BootStrings.ArcPath);
        }
        else if (*BootStrings.Drive)
        {
            /* Otherwise, modify the BootDrive and BootPartition */
            IniModifySettingValue(SectionId, "BootDrive", BootStrings.Drive);
            IniModifySettingValue(SectionId, "BootPartition", BootStrings.Partition);
        }
        else
        {
            /*
             * Otherwise, zero out all values: BootSectorFile will be
             * relative to the default system partition.
             */
            IniModifySettingValue(SectionId, "BootPath", "");
            IniModifySettingValue(SectionId, "BootDrive", "");
            IniModifySettingValue(SectionId, "BootPartition", "");
        }

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

    /* Add the BootPath if we have one */
    if (*BootStrings.ArcPath)
    {
        if (!IniAddSettingValueToSection(SectionId, "BootPath", BootStrings.ArcPath))
            return;
    }
    else if (*BootStrings.Drive)
    {
        /* Otherwise, add the BootDrive and BootPartition */
        if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootStrings.Drive))
            return;

        if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootStrings.Partition))
            return;
    }

    /* Add the BootSectorFile */
    if (!IniAddSettingValueToSection(SectionId, "BootSectorFile", BootSectorFileString))
        return;

    OperatingSystem->SectionId = SectionId;
    OperatingSystem->LoadIdentifier = NULL;
}

VOID
EditCustomBootLinux(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    TIMEINFO* TimeInfo;
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    CHAR SectionName[100];
    /* This construct is a trick for saving some stack space */
    union
    {
        struct
        {
            CHAR Guard1;
            CHAR Drive[20];
            CHAR Partition[20];
            CHAR Guard2;
        };
        CHAR ArcPath[200];
    } BootStrings;
    CHAR LinuxKernelString[200];
    CHAR LinuxInitrdString[200];
    CHAR LinuxCommandLineString[200];

    RtlZeroMemory(SectionName, sizeof(SectionName));
    RtlZeroMemory(&BootStrings, sizeof(BootStrings));
    RtlZeroMemory(LinuxKernelString, sizeof(LinuxKernelString));
    RtlZeroMemory(LinuxInitrdString, sizeof(LinuxInitrdString));
    RtlZeroMemory(LinuxCommandLineString, sizeof(LinuxCommandLineString));

    if (SectionId != 0)
    {
        /* Load the settings */

        /*
         * Check whether we have a "BootPath" value (takes precedence
         * over both "BootDrive" and "BootPartition").
         */
        *BootStrings.ArcPath = ANSI_NULL;
        IniReadSettingByName(SectionId, "BootPath", BootStrings.ArcPath, sizeof(BootStrings.ArcPath));
        if (!*BootStrings.ArcPath)
        {
            /* We don't, retrieve the boot drive and partition values instead */
            IniReadSettingByName(SectionId, "BootDrive", BootStrings.Drive, sizeof(BootStrings.Drive));
            IniReadSettingByName(SectionId, "BootPartition", BootStrings.Partition, sizeof(BootStrings.Partition));
        }

        IniReadSettingByName(SectionId, "Kernel", LinuxKernelString, sizeof(LinuxKernelString));
        IniReadSettingByName(SectionId, "Initrd", LinuxInitrdString, sizeof(LinuxInitrdString));
        IniReadSettingByName(SectionId, "CommandLine", LinuxCommandLineString, sizeof(LinuxCommandLineString));
    }

    if (!*BootStrings.ArcPath)
    {
        if (!UiEditBox(BootDrivePrompt, BootStrings.Drive, sizeof(BootStrings.Drive)))
            return;

        if (*BootStrings.Drive)
        {
            if (!UiEditBox(BootPartitionPrompt, BootStrings.Partition, sizeof(BootStrings.Partition)))
                return;
        }
    }
    if (!*BootStrings.Drive)
    {
        if (!UiEditBox(ARCPathPrompt, BootStrings.ArcPath, sizeof(BootStrings.ArcPath)))
            return;
    }

    if (!UiEditBox(LinuxKernelPrompt, LinuxKernelString, sizeof(LinuxKernelString)))
        return;

    if (!UiEditBox(LinuxInitrdPrompt, LinuxInitrdString, sizeof(LinuxInitrdString)))
        return;

    if (!UiEditBox(LinuxCommandLinePrompt, LinuxCommandLineString, sizeof(LinuxCommandLineString)))
        return;

    /* Modify the settings values and return if we were in edit mode */
    if (SectionId != 0)
    {
        /* Modify the BootPath if we have one */
        if (*BootStrings.ArcPath)
        {
            IniModifySettingValue(SectionId, "BootPath", BootStrings.ArcPath);
        }
        else if (*BootStrings.Drive)
        {
            /* Otherwise, modify the BootDrive and BootPartition */
            IniModifySettingValue(SectionId, "BootDrive", BootStrings.Drive);
            IniModifySettingValue(SectionId, "BootPartition", BootStrings.Partition);
        }
        else
        {
            /*
             * Otherwise, zero out all values: BootSectorFile will be
             * relative to the default system partition.
             */
            IniModifySettingValue(SectionId, "BootPath", "");
            IniModifySettingValue(SectionId, "BootDrive", "");
            IniModifySettingValue(SectionId, "BootPartition", "");
        }

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

    /* Add the BootPath if we have one */
    if (*BootStrings.ArcPath)
    {
        if (!IniAddSettingValueToSection(SectionId, "BootPath", BootStrings.ArcPath))
            return;
    }
    else if (*BootStrings.Drive)
    {
        /* Otherwise, add the BootDrive and BootPartition */
        if (!IniAddSettingValueToSection(SectionId, "BootDrive", BootStrings.Drive))
            return;

        if (!IniAddSettingValueToSection(SectionId, "BootPartition", BootStrings.Partition))
            return;
    }

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

    OperatingSystem->SectionId = SectionId;
    OperatingSystem->LoadIdentifier = "Custom Linux Setup";
}

#endif /* _M_IX86 || _M_AMD64 */

VOID
EditCustomBootReactOS(
    IN OUT OperatingSystemItem* OperatingSystem,
    IN BOOLEAN IsSetup)
{
    TIMEINFO* TimeInfo;
    ULONG_PTR SectionId = OperatingSystem->SectionId;
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

    if (!UiEditBox(IsSetup ? ReactOSSetupOptionsPrompt : ReactOSOptionsPrompt, ReactOSOptions, sizeof(ReactOSOptions)))
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
    if (!IniAddSettingValueToSection(SectionId, "BootType", IsSetup ? "ReactOSSetup" : "Windows2003"))
        return;

    /* Construct the ReactOS ARC system path */
    ConstructArcPath(ReactOSARCPath, ReactOSSystemPath,
                     DriveMapGetBiosDriveNumber(BootDriveString),
                     atoi(BootPartitionString));

    /* Add the system path */
    if (!IniAddSettingValueToSection(SectionId, "SystemPath", ReactOSARCPath))
        return;

    /* Add the CommandLine */
    if (!IniAddSettingValueToSection(SectionId, "Options", ReactOSOptions))
        return;

    OperatingSystem->SectionId = SectionId;
    OperatingSystem->LoadIdentifier = NULL;
}

#ifdef HAS_OPTION_MENU_REBOOT

VOID OptionMenuReboot(VOID)
{
    UiMessageBox("The system will now reboot.");
    Reboot();
}

#endif // HAS_OPTION_MENU_REBOOT
