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

// ARC Disk Information
ARC_DISK_SIGNATURE reactos_arc_disk_info[32];
ULONG reactos_disk_count = 0;
CHAR reactos_arc_strings[32][256];

typedef
VOID
(*OS_LOADING_METHOD)(IN OperatingSystemItem* OperatingSystem,
                     IN USHORT OperatingSystemVersion);

struct
{
    CHAR BootType[80];
    USHORT OperatingSystemVersion;
    OS_LOADING_METHOD Load;
} OSLoadingMethods[] =
{
    {"ReactOSSetup", 0                , LoadReactOSSetup     },

#ifdef _M_IX86
    {"BootSector"  , 0                , LoadAndBootBootSector},
    {"Drive"       , 0                , LoadAndBootDrive     },
    {"Partition"   , 0                , LoadAndBootPartition },

    {"Linux"       , 0                , LoadAndBootLinux     },

    {"Windows"     , 0                , LoadAndBootWindows   },
    {"WindowsNT40" , _WIN32_WINNT_NT4 , LoadAndBootWindows   },
#endif
    {"Windows2003" , _WIN32_WINNT_WS03, LoadAndBootWindows   },
};

/* FUNCTIONS ******************************************************************/

VOID LoadOperatingSystem(IN OperatingSystemItem* OperatingSystem)
{
    ULONG_PTR SectionId;
    PCSTR SectionName = OperatingSystem->SystemPartition;
    CHAR BootType[80];
    ULONG i;

    /* Try to open the operating system section in the .ini file */
    if (IniOpenSection(SectionName, &SectionId))
    {
        /* Try to read the boot type */
        IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));
    }
    else
    {
        BootType[0] = ANSI_NULL;
    }

    if (BootType[0] == ANSI_NULL && SectionName[0] != ANSI_NULL)
    {
        /* Try to infere the boot type value */
#ifdef _M_IX86
        ULONG FileId;
        if (ArcOpen((PSTR)SectionName, OpenReadOnly, &FileId) == ESUCCESS)
        {
            ArcClose(FileId);
            strcpy(BootType, "BootSector");
        }
        else
#endif
        {
            strcpy(BootType, "Windows");
        }
    }

    /* Install the drive mapper according to this section drive mappings */
#if defined(_M_IX86) && !defined(_MSC_VER)
    DriveMapMapDrivesInSection(SectionName);
#endif

    /* Loop through the OS loading method table and find a suitable OS to boot */
    for (i = 0; i < sizeof(OSLoadingMethods) / sizeof(OSLoadingMethods[0]); ++i)
    {
        if (_stricmp(BootType, OSLoadingMethods[i].BootType) == 0)
        {
            OSLoadingMethods[i].Load(OperatingSystem,
                                     OSLoadingMethods[i].OperatingSystemVersion);
            return;
        }
    }
}

ULONG GetDefaultOperatingSystem(OperatingSystemItem* OperatingSystemList, ULONG OperatingSystemCount)
{
    CHAR      DefaultOSText[80];
    PCSTR     DefaultOSName;
    ULONG_PTR SectionId;
    ULONG     DefaultOS = 0;
    ULONG     Idx;

    if (!IniOpenSection("FreeLoader", &SectionId))
        return 0;

    DefaultOSName = CmdLineGetDefaultOS();
    if (DefaultOSName == NULL)
    {
        if (IniReadSettingByName(SectionId, "DefaultOS", DefaultOSText, sizeof(DefaultOSText)))
        {
            DefaultOSName = DefaultOSText;
        }
    }

    if (DefaultOSName != NULL)
    {
        for (Idx = 0; Idx<OperatingSystemCount; Idx++)
        {
            if (_stricmp(DefaultOSName, OperatingSystemList[Idx].SystemPartition) == 0)
            {
                DefaultOS = Idx;
                break;
            }
        }
    }

    return DefaultOS;
}

LONG GetTimeOut(VOID)
{
    CHAR    TimeOutText[20];
    LONG        TimeOut;
    ULONG_PTR    SectionId;

    TimeOut = CmdLineGetTimeOut();
    if (TimeOut >= 0)
        return TimeOut;

    if (!IniOpenSection("FreeLoader", &SectionId))
        return -1;

    if (IniReadSettingByName(SectionId, "TimeOut", TimeOutText, sizeof(TimeOutText)))
        TimeOut = atoi(TimeOutText);
    else
        TimeOut = -1;

    return TimeOut;
}

BOOLEAN MainBootMenuKeyPressFilter(ULONG KeyPress)
{
    if (KeyPress == KEY_F8)
    {
        DoOptionsMenu();
        return TRUE;
    }

    /* We didn't handle the key */
    return FALSE;
}

VOID RunLoader(VOID)
{
    ULONG_PTR SectionId;
    ULONG     OperatingSystemCount;
    OperatingSystemItem* OperatingSystemList;
    PCSTR*    OperatingSystemDisplayNames;
    ULONG     DefaultOperatingSystem;
    LONG      TimeOut;
    ULONG     SelectedOperatingSystem;
    ULONG     i;

    if (!MachInitializeBootDevices())
    {
        UiMessageBoxCritical("Error when detecting hardware.");
        return;
    }

#ifdef _M_IX86
    /* Load additional SCSI driver (if any) */
    if (LoadBootDeviceDriver() != ESUCCESS)
    {
        UiMessageBoxCritical("Unable to load additional boot device drivers.");
    }
#endif

    if (!IniFileInitialize())
    {
        UiMessageBoxCritical("Error initializing .ini file.");
        return;
    }

    if (!IniOpenSection("FreeLoader", &SectionId))
    {
        UiMessageBoxCritical("Section [FreeLoader] not found in freeldr.ini.");
        return;
    }

    TimeOut = GetTimeOut();

    if (!UiInitialize(TRUE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        return;
    }

    OperatingSystemList = InitOperatingSystemList(&OperatingSystemCount);
    if (!OperatingSystemList)
    {
        UiMessageBox("Unable to read operating systems section in freeldr.ini.\nPress ENTER to reboot.");
        goto Reboot;
    }

    if (OperatingSystemCount == 0)
    {
        UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
        goto Reboot;
    }

    DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemList, OperatingSystemCount);

    /* Create list of display names */
    OperatingSystemDisplayNames = FrLdrTempAlloc(sizeof(PCSTR) * OperatingSystemCount, 'mNSO');
    if (!OperatingSystemDisplayNames)
        goto Reboot;

    for (i = 0; i < OperatingSystemCount; i++)
    {
        OperatingSystemDisplayNames[i] = OperatingSystemList[i].LoadIdentifier;
    }

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInSection("FreeLoader");

    for (;;)
    {
        /* Redraw the backdrop */
        UiDrawBackdrop();

        /* Show the operating system list menu */
        if (!UiDisplayMenu("Please select the operating system to start:",
                           "For troubleshooting and advanced startup options for "
                               "ReactOS, press F8.",
                           TRUE,
                           OperatingSystemDisplayNames,
                           OperatingSystemCount,
                           DefaultOperatingSystem,
                           TimeOut,
                           &SelectedOperatingSystem,
                           FALSE,
                           MainBootMenuKeyPressFilter))
        {
            UiMessageBox("Press ENTER to reboot.");
            goto Reboot;
        }

        TimeOut = -1;

        /* Load the chosen operating system */
        LoadOperatingSystem(&OperatingSystemList[SelectedOperatingSystem]);
    }

Reboot:
    UiUnInitialize("Rebooting...");
    return;
}
