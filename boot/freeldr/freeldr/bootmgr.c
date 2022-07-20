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
DBG_DEFAULT_CHANNEL(WARNING);

/* GLOBALS ********************************************************************/

typedef
VOID
(*EDIT_OS_ENTRY_PROC)(
    IN OUT OperatingSystemItem* OperatingSystem);

static VOID
EditCustomBootReactOSSetup(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    EditCustomBootReactOS(OperatingSystem, TRUE);
}

static VOID
EditCustomBootNTOS(
    IN OUT OperatingSystemItem* OperatingSystem)
{
    EditCustomBootReactOS(OperatingSystem, FALSE);
}

static const struct
{
    PCSTR BootType;
    EDIT_OS_ENTRY_PROC EditOsEntry;
    ARC_ENTRY_POINT OsLoader;
} OSLoadingMethods[] =
{
    {"ReactOSSetup", EditCustomBootReactOSSetup, LoadReactOSSetup},

#if defined(_M_IX86) || defined(_M_AMD64)
    {"Drive"       , EditCustomBootDisk      , LoadAndBootDevice},
    {"Partition"   , EditCustomBootPartition , LoadAndBootDevice},
    {"BootSector"  , EditCustomBootSectorFile, LoadAndBootDevice},
    {"Linux"       , EditCustomBootLinux, LoadAndBootLinux  },
#endif
#ifdef _M_IX86
    {"WindowsNT40" , EditCustomBootNTOS , LoadAndBootWindows},
#endif
    {"Windows"     , EditCustomBootNTOS , LoadAndBootWindows},
    {"Windows2003" , EditCustomBootNTOS , LoadAndBootWindows},
};

/* FUNCTIONS ******************************************************************/

/*
 * This function converts the list of key=value options in the given operating
 * system section into an ARC-compatible argument vector, providing in addition
 * the extra mandatory Software Loading Environment Variables, following the
 * ARC specification.
 */
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

    *pArgc = 0;

    ASSERT(SectionId != 0);

    /* Validate the LoadIdentifier (to make tests simpler later) */
    if (LoadIdentifier && !*LoadIdentifier)
        LoadIdentifier = NULL;

    /* Count the number of operating systems in the section */
    Count = IniGetNumSectionItems(SectionId);

    /*
     * The argument vector contains the program name, the SystemPartition,
     * the LoadIdentifier (optional), and the items in the OS section.
     */
    Argc = 2 + (LoadIdentifier ? 1 : 0) + Count;

    /* Calculate the total size needed for the string buffer of the argument vector */
    Size = 0;
    /* i == 0: Program name */
    /* i == 1: SystemPartition : from where FreeLdr has been started */
    Size += (strlen("SystemPartition=") + strlen(FrLdrBootPath) + 1) * sizeof(CHAR);
    /* i == 2: LoadIdentifier  : ASCII string that may be used to associate an identifier with a set of load parameters */
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
    /* i == 1: SystemPartition */
    {
        strcpy(SettingName, "SystemPartition=");
        strcat(SettingName, FrLdrBootPath);

        *Args++ = SettingName;
        SettingName += (strlen(SettingName) + 1);
    }
    /* i == 2: LoadIdentifier */
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
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    ULONG i;
    ULONG Argc;
    PCHAR* Argv;
    CHAR BootType[80];

    /* The operating system section has been opened by InitOperatingSystemList() */
    ASSERT(SectionId != 0);

    /* Try to read the boot type */
    *BootType = ANSI_NULL;
    IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));

    /* We must have the "BootType" value (it has been possibly added by InitOperatingSystemList()) */
    ASSERT(*BootType);

#ifdef _M_IX86
    /* Install the drive mapper according to this section drive mappings */
    DriveMapMapDrivesInSection(SectionId);
#endif

    /* Find the suitable OS loader to start */
    for (i = 0; ; ++i)
    {
        if (i >= RTL_NUMBER_OF(OSLoadingMethods))
            return;
        if (_stricmp(BootType, OSLoadingMethods[i].BootType) == 0)
            break;
    }

    /* Build the ARC-compatible argument vector */
    Argv = BuildArgvForOsLoader(OperatingSystem->LoadIdentifier, SectionId, &Argc);
    if (!Argv)
        return; // Unexpected failure.

    /* Start the OS loader */
    OSLoadingMethods[i].OsLoader(Argc, Argv, NULL);
    FrLdrHeapFree(Argv, TAG_STRING);
}

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE

VOID EditOperatingSystemEntry(IN OperatingSystemItem* OperatingSystem)
{
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    ULONG i;
    CHAR BootType[80];

    /* The operating system section has been opened by InitOperatingSystemList() */
    ASSERT(SectionId != 0);

    /* Try to read the boot type */
    *BootType = ANSI_NULL;
    IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));

    /* We must have the "BootType" value (it has been possibly added by InitOperatingSystemList()) */
    ASSERT(*BootType);

    /* Find the suitable OS entry editor */
    for (i = 0; ; ++i)
    {
        if (i >= RTL_NUMBER_OF(OSLoadingMethods))
            return;
        if (_stricmp(BootType, OSLoadingMethods[i].BootType) == 0)
            break;
    }

    /* Run it */
    OSLoadingMethods[i].EditOsEntry(OperatingSystem);
}

#endif // HAS_OPTION_MENU_EDIT_CMDLINE

static LONG
GetTimeOut(
    IN ULONG_PTR FrLdrSectionId)
{
    LONG TimeOut = -1;
    CHAR TimeOutText[20];

    TimeOut = CmdLineGetTimeOut();
    if (TimeOut >= 0)
        return TimeOut;

    TimeOut = -1;

    if ((FrLdrSectionId != 0) &&
        IniReadSettingByName(FrLdrSectionId, "TimeOut", TimeOutText, sizeof(TimeOutText)))
    {
        TimeOut = atoi(TimeOutText);
    }

    return TimeOut;
}

BOOLEAN
MainBootMenuKeyPressFilter(
    IN ULONG KeyPress,
    IN ULONG SelectedMenuItem,
    IN PVOID Context OPTIONAL)
{
    switch (KeyPress)
    {
    case KEY_F8:
        DoOptionsMenu(&((OperatingSystemItem*)Context)[SelectedMenuItem]);
        return TRUE;

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    case KEY_F10:
        EditOperatingSystemEntry(&((OperatingSystemItem*)Context)[SelectedMenuItem]);
        return TRUE;
#endif

    default:
        /* We didn't handle the key */
        return FALSE;
    }
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

    /* Open the [FreeLoader] section */
    if (!IniOpenSection("FreeLoader", &SectionId))
    {
        UiMessageBoxCritical("Section [FreeLoader] not found in freeldr.ini.");
        return;
    }

    /* Debugger main initialization */
    DebugInit(SectionId);

    /* Retrieve the default timeout */
    TimeOut = GetTimeOut(SectionId);

    /* UI main initialization */
    if (!UiInitialize(TRUE))
    {
        UiMessageBoxCritical("Unable to initialize UI.");
        return;
    }

    OperatingSystemList = InitOperatingSystemList(SectionId,
                                                  &OperatingSystemCount,
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
    UiShowMessageBoxesInSection(SectionId);

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
                           MainBootMenuKeyPressFilter,
                           OperatingSystemList))
        {
            UiMessageBox("Press ENTER to reboot.");
            goto Reboot;
        }

        TimeOut = -1;

        /* Load the chosen operating system */
        LoadOperatingSystem(&OperatingSystemList[SelectedOperatingSystem]);

        /* If we get there, the OS loader failed. As it may have
         * messed up the display, re-initialize the UI. */
#ifndef _M_ARM
        UiVtbl.UnInitialize();
#endif
        UiInitialize(TRUE);
    }

Reboot:
    UiUnInitialize("Rebooting...");
    IniCleanup();
    return;
}
