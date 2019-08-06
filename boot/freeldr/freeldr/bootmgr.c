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
#include <debug.h>

DBG_DEFAULT_CHANNEL(INIFILE);

/* GLOBALS ********************************************************************/

static const struct
{
    PCSTR BootType;
    ARC_ENTRY_POINT OsLoader;
} OSLoadingMethods[] =
{
    {"ReactOSSetup", LoadReactOSSetup     },

#ifdef _M_IX86
    {"BootSector"  , LoadAndBootBootSector},
    {"Drive"       , LoadAndBootDrive     },
    {"Partition"   , LoadAndBootPartition },

    {"Linux"       , LoadAndBootLinux     },

    {"Windows"     , LoadAndBootWindows   },
    {"WindowsNT40" , LoadAndBootWindows   },
#endif
    {"Windows2003" , LoadAndBootWindows   },
};

/* FUNCTIONS ******************************************************************/

PCHAR*
BuildArgvForOsLoader(
    IN PCSTR LoadIdentifier,
    IN ULONG_PTR SectionId,
    OUT PULONG pArgc)
{
    SIZE_T Size;
    ULONG Count;
    ULONG i;
    ULONG Argc;
    PCHAR* Argv;
    PCHAR* Args;
    PCHAR SettingName, SettingValue;

    /*
     * Convert the list of key=value options in the given operating system section
     * into a ARC-compatible argument vector.
     */

    *pArgc = 0;

    /* Validate the LoadIdentifier (to make tests simpler later) */
    if (LoadIdentifier && !*LoadIdentifier)
        LoadIdentifier = NULL;

    /* Count the number of operating systems in the section */
    Count = IniGetNumSectionItems(SectionId);

    /* The argument vector contains the program name, the LoadIdentifier (optional), and the items in the OS section */
    Argc = 1 + Count;
    if (LoadIdentifier)
        ++Argc;

    /* Calculate the total size needed for the string buffer of the argument vector */
    Size = 0;
    /* i == 0: Program name */
    /* i == 1: LoadIdentifier */
    if (LoadIdentifier)
    {
        Size += (strlen("LoadIdentifier=") + strlen(LoadIdentifier) + 1) * sizeof(CHAR);
    }
    for (i = 0; i < Count; ++i)
    {
        Size += IniGetSectionSettingNameSize(SectionId, i);  // Counts also the NULL-terminator, that we transform into the '=' sign separator.
        Size += IniGetSectionSettingValueSize(SectionId, i); // Counts also the NULL-terminator.
    }
    Size += sizeof(ANSI_NULL); // Final NULL-terminator.

    /* Allocate memory to hold the argument vector: pointers and string buffer */
    Argv = FrLdrHeapAlloc(Argc * sizeof(PCHAR) + Size, TAG_STRING);
    if (!Argv)
        return NULL;

    /* Initialize the argument vector: loop through the section and copy the key=value options */
    SettingName = (PCHAR)((ULONG_PTR)Argv + (Argc * sizeof(PCHAR)));
    Args = Argv;
    /* i == 0: Program name */
    *Args++ = NULL;
    /* i == 1: LoadIdentifier */
    if (LoadIdentifier)
    {
        strcpy(SettingName, "LoadIdentifier=");
        strcat(SettingName, LoadIdentifier);

        *Args++ = SettingName;
        SettingName += (strlen(SettingName) + 1);
    }
    for (i = 0; i < Count; ++i)
    {
        Size = IniGetSectionSettingNameSize(SectionId, i);
        SettingValue = SettingName + Size;
        IniReadSettingByNumber(SectionId, i,
                               SettingName, Size,
                               SettingValue, IniGetSectionSettingValueSize(SectionId, i));
        SettingName[Size - 1] = '=';

        *Args++ = SettingName;
        SettingName += (strlen(SettingName) + 1);
    }

#if DBG
    /* Dump the argument vector for debugging */
    for (i = 0; i < Argc; ++i)
    {
        TRACE("Argv[%lu]: '%s'\n", i, Argv[i]);
    }
#endif

    *pArgc = Argc;
    return Argv;
}

VOID LoadOperatingSystem(IN OperatingSystemItem* OperatingSystem)
{
    ULONG_PTR SectionId;
    PCSTR SectionName = OperatingSystem->SectionName;
    ULONG i;
    ULONG Argc;
    PCHAR* Argv;
    CHAR BootType[80];

    /* Try to open the operating system section in the .ini file */
    if (!IniOpenSection(SectionName, &SectionId))
    {
        UiMessageBox("Section [%s] not found in freeldr.ini.", SectionName);
        return;
    }

    /* Try to read the boot type */
    *BootType = ANSI_NULL;
    IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));

    /* We must have the "BootType" value (it has been possibly added by InitOperatingSystemList()) */
    ASSERT(*BootType);

#if defined(_M_IX86)
    /* Install the drive mapper according to this section drive mappings */
    DriveMapMapDrivesInSection(SectionName);
#endif

    /* Loop through the OS loading method table and find a suitable OS to boot */
    for (i = 0; i < sizeof(OSLoadingMethods) / sizeof(OSLoadingMethods[0]); ++i)
    {
        if (_stricmp(BootType, OSLoadingMethods[i].BootType) == 0)
        {
            Argv = BuildArgvForOsLoader(OperatingSystem->LoadIdentifier, SectionId, &Argc);
            if (Argv)
            {
                OSLoadingMethods[i].OsLoader(Argc, Argv, NULL);
                FrLdrHeapFree(Argv, TAG_STRING);
            }
            return;
        }
    }
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
    LONG      TimeOut;
    ULONG     OperatingSystemCount;
    OperatingSystemItem* OperatingSystemList;
    PCSTR*    OperatingSystemDisplayNames;
    ULONG     DefaultOperatingSystem;
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

    /* Debugger main initialization */
    DebugInit(TRUE);

    if (!IniOpenSection("FreeLoader", &SectionId))
    {
        UiMessageBoxCritical("Section [FreeLoader] not found in freeldr.ini.");
        return;
    }

    TimeOut = GetTimeOut();

    /* UI main initialization */
    if (!UiInitialize(TRUE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        return;
    }

    OperatingSystemList = InitOperatingSystemList(&OperatingSystemCount,
                                                  &DefaultOperatingSystem);
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
    IniCleanup();
    return;
}
