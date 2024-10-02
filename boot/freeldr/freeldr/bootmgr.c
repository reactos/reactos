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

typedef VOID
(*EDIT_OS_ENTRY_PROC)(
    _Inout_ OperatingSystemItem* OperatingSystem);

static VOID
EditCustomBootReactOSSetup(
    _Inout_ OperatingSystemItem* OperatingSystem)
{
    EditCustomBootReactOS(OperatingSystem, TRUE);
}

static VOID
EditCustomBootNTOS(
    _Inout_ OperatingSystemItem* OperatingSystem)
{
    EditCustomBootReactOS(OperatingSystem, FALSE);
}

typedef struct _OS_LOADING_METHOD
{
    PCSTR BootType;
    EDIT_OS_ENTRY_PROC EditOsEntry;
    ARC_ENTRY_POINT OsLoader;
} OS_LOADING_METHOD, *POS_LOADING_METHOD;

static const OS_LOADING_METHOD
OSLoadingMethods[] =
{
    {"ReactOSSetup", EditCustomBootReactOSSetup, LoadReactOSSetup},

#if defined(_M_IX86) || defined(_M_AMD64)
#ifndef UEFIBOOT
    {"BootSector", EditCustomBootSector, LoadAndBootSector},
    {"Linux"     , EditCustomBootLinux , LoadAndBootLinux },
#endif
#endif
#ifdef _M_IX86
    {"WindowsNT40" , EditCustomBootNTOS, LoadAndBootWindows},
#endif
    {"Windows"     , EditCustomBootNTOS, LoadAndBootWindows},
    {"Windows2003" , EditCustomBootNTOS, LoadAndBootWindows},
    {"WindowsVista", EditCustomBootNTOS, LoadAndBootWindows},
};

/* FUNCTIONS ******************************************************************/

#ifdef HAS_DEPRECATED_OPTIONS
/**
 * @brief   Helper for dealing with DEPRECATED features.
 **/
VOID
WarnDeprecated(
    _In_ PCSTR MsgFmt,
    ...)
{
    va_list ap;
    CHAR msgString[300];

    /* If the user didn't cancel the timeout, don't display the warning */
    if (GetBootMgrInfo()->TimeOut >= 0)
        return;

    va_start(ap, MsgFmt);
    RtlStringCbVPrintfA(msgString, sizeof(msgString),
                        MsgFmt, ap);
    va_end(ap);

    UiMessageBox(
        "                           WARNING!\n"
        "\n"
        "%s\n"
        "\n"
        "Should you need assistance, please contact ReactOS developers\n"
        "on the official ReactOS Mattermost server <chat.reactos.org>.",
        msgString);
}
#endif // HAS_DEPRECATED_OPTIONS

static const OS_LOADING_METHOD*
GetOSLoadingMethod(
    _In_ ULONG_PTR SectionId)
{
    ULONG i;
    CHAR BootType[80];

    /* The operating system section has been opened by InitOperatingSystemList() */
    ASSERT(SectionId != 0);

    /* Try to read the boot type. We must have the value (it
     * has been possibly added by InitOperatingSystemList()) */
    *BootType = ANSI_NULL;
    IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));
    ASSERT(*BootType);

////
#ifdef HAS_DEPRECATED_OPTIONS
    if ((_stricmp(BootType, "Drive") == 0) ||
        (_stricmp(BootType, "Partition") == 0))
    {
        /* Display the deprecation warning message */
        WarnDeprecated(
            "The '%s' configuration you are booting into is no longer\n"
            "supported and will be removed in future FreeLoader versions.\n"
            "\n"
            "Please edit FREELDR.INI to replace all occurrences of\n"
            "\n"
            "             %*s        to:\n"
            "    BootType=%s      ------>     BootType=BootSector",
            BootType,
            strlen(BootType), "", // Indentation
            BootType);

        /* Type fixup */
        strcpy(BootType, "BootSector");
        if (!IniModifySettingValue(SectionId, "BootType", BootType))
        {
            ERR("Could not fixup the BootType entry for OS '%s', ignoring.\n",
                ((PINI_SECTION)SectionId)->SectionName);
        }
    }
#endif // HAS_DEPRECATED_OPTIONS
////

    /* Find the suitable OS loading method */
    for (i = 0; ; ++i)
    {
        if (i >= RTL_NUMBER_OF(OSLoadingMethods))
        {
            UiMessageBox("Unknown boot entry type '%s'", BootType);
            return NULL;
        }
        if (_stricmp(BootType, OSLoadingMethods[i].BootType) == 0)
            return &OSLoadingMethods[i];
    }
    UNREACHABLE;
}

/**
 * @brief
 * This function converts the list of Key=Value options in the given operating
 * system section into an ARC-compatible argument vector, providing in addition
 * the extra mandatory Software Loading Environment Variables, following the
 * ARC specification.
 **/
static PCHAR*
BuildArgvForOsLoader(
    _In_ PCSTR LoadIdentifier,
    _In_ ULONG_PTR SectionId,
    _Out_ PULONG pArgc)
{
    SIZE_T Size;
    ULONG Count;
    ULONG i;
    ULONG Argc;
    PCHAR* Argv;
    PCHAR* Args;
    PCHAR SettingName, SettingValue;
    PCCHAR BootPath = FrLdrGetBootPath();

    *pArgc = 0;

    ASSERT(SectionId != 0);

    /* Normalize LoadIdentifier to make subsequent tests simpler */
    if (LoadIdentifier && !*LoadIdentifier)
        LoadIdentifier = NULL;

    /* Count the number of operating systems in the section */
    Count = IniGetNumSectionItems(SectionId);

    /*
     * The argument vector contains the program name, the SystemPartition,
     * the LoadIdentifier (optional), and the items in the OS section.
     * For POSIX compliance, a terminating NULL pointer (not counted in Argc)
     * is appended, such that Argv[Argc] == NULL.
     */
    Argc = 2 + (LoadIdentifier ? 1 : 0) + Count;

    /* Calculate the total size needed for the string buffer of the argument vector */
    Size = 0;
    /* i == 0: Program name */
    // TODO: Provide one in the future...
    /* i == 1: SystemPartition : from where FreeLdr has been started */
    Size += (strlen("SystemPartition=") + strlen(BootPath) + 1) * sizeof(CHAR);
    /* i == 2: LoadIdentifier  : ASCII string that may be used
     * to associate an identifier with a set of load parameters */
    if (LoadIdentifier)
    {
        Size += (strlen("LoadIdentifier=") + strlen(LoadIdentifier) + 1) * sizeof(CHAR);
    }
    /* The section items */
    for (i = 0; i < Count; ++i)
    {
        Size += IniGetSectionSettingNameSize(SectionId, i);  // Counts also the NULL-terminator, that we transform into the '=' sign separator.
        Size += IniGetSectionSettingValueSize(SectionId, i); // Counts also the NULL-terminator.
    }
    Size += sizeof(ANSI_NULL); // Final NULL-terminator.

    /* Allocate memory to hold the argument vector: pointers and string buffer */
    Argv = FrLdrHeapAlloc((Argc + 1) * sizeof(PCHAR) + Size, TAG_STRING);
    if (!Argv)
        return NULL;

    /* Initialize the argument vector: loop through the section and copy the Key=Value options */
    SettingName = (PCHAR)((ULONG_PTR)Argv + ((Argc + 1) * sizeof(PCHAR)));
    Args = Argv;
    /* i == 0: Program name */
    *Args++ = NULL; // TODO: Provide one in the future...
    /* i == 1: SystemPartition */
    {
        strcpy(SettingName, "SystemPartition=");
        strcat(SettingName, BootPath);

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
    /* The section items */
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
    /* Terminating NULL pointer */
    *Args = NULL;

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

VOID
LoadOperatingSystem(
    _In_ OperatingSystemItem* OperatingSystem)
{
    ULONG_PTR SectionId = OperatingSystem->SectionId;
    const OS_LOADING_METHOD* OSLoadingMethod;
    ULONG Argc;
    PCHAR* Argv;

    /* Find the suitable OS loader to start */
    OSLoadingMethod = GetOSLoadingMethod(SectionId);
    if (!OSLoadingMethod)
        return;
    ASSERT(OSLoadingMethod->OsLoader);

    /* Build the ARC-compatible argument vector */
    Argv = BuildArgvForOsLoader(OperatingSystem->LoadIdentifier, SectionId, &Argc);
    if (!Argv)
        return; // Unexpected failure.

#ifdef _M_IX86
#ifndef UEFIBOOT
    /* Install the drive mapper according to this section drive mappings */
    DriveMapMapDrivesInSection(SectionId);
#endif
#endif

    /* Start the OS loader */
    OSLoadingMethod->OsLoader(Argc, Argv, NULL);
    FrLdrHeapFree(Argv, TAG_STRING);
}

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
VOID
EditOperatingSystemEntry(
    _Inout_ OperatingSystemItem* OperatingSystem)
{
    /* Find the suitable OS entry editor and open it */
    const OS_LOADING_METHOD* OSLoadingMethod =
        GetOSLoadingMethod(OperatingSystem->SectionId);
    if (OSLoadingMethod)
    {
        ASSERT(OSLoadingMethod->EditOsEntry);
        OSLoadingMethod->EditOsEntry(OperatingSystem);
    }
}
#endif // HAS_OPTION_MENU_EDIT_CMDLINE

BOOLEAN
MainBootMenuKeyPressFilter(
    IN ULONG KeyPress,
    IN ULONG SelectedMenuItem,
    IN PVOID Context OPTIONAL)
{
    /* Any key-press cancels the global timeout */
    GetBootMgrInfo()->TimeOut = -1;

    switch (KeyPress)
    {
    case KEY_F8:
        DoOptionsMenu(&((OperatingSystemItem*)Context)[SelectedMenuItem]);
        DisplayBootTimeOptions();
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
    OperatingSystemItem* OperatingSystemList;
    PCSTR* OperatingSystemDisplayNames;
    ULONG OperatingSystemCount;
    ULONG DefaultOperatingSystem;
    ULONG SelectedOperatingSystem;
    ULONG i;

#ifdef _M_IX86
#ifndef UEFIBOOT
    /* Load additional SCSI driver (if any) */
    if (LoadBootDeviceDriver() != ESUCCESS)
    {
        UiMessageBoxCritical("Unable to load additional boot device drivers.");
    }
#endif
#endif

    /* Open FREELDR.INI and load the global FreeLoader settings */
    if (!IniFileInitialize())
    {
        UiMessageBoxCritical("Error initializing .ini file.");
        return;
    }
    LoadSettings(NULL);
#if 0
    if (FALSE)
    {
        UiMessageBoxCritical("Could not load global FreeLoader settings.");
        return;
    }
#endif

    /* Debugger main initialization */
    DebugInit(GetBootMgrInfo()->DebugString);

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
    UiShowMessageBoxesInSection(GetBootMgrInfo()->FrLdrSection);

    for (;;)
    {
        /* Redraw the backdrop */
        UiDrawBackdrop();

        /* Show the operating system list menu */
        if (!UiDisplayMenu("Please select the operating system to start:",
                           "For troubleshooting and advanced startup options for "
                               "ReactOS, press F8.",
                           OperatingSystemDisplayNames,
                           OperatingSystemCount,
                           DefaultOperatingSystem,
                           GetBootMgrInfo()->TimeOut,
                           &SelectedOperatingSystem,
                           FALSE,
                           MainBootMenuKeyPressFilter,
                           OperatingSystemList))
        {
            UiMessageBox("Press ENTER to reboot.");
            goto Reboot;
        }

        /* Load the chosen operating system */
        LoadOperatingSystem(&OperatingSystemList[SelectedOperatingSystem]);

        GetBootMgrInfo()->TimeOut = -1;

        /* If we get there, the OS loader failed. As it may have
         * messed up the display, re-initialize the UI. */
#ifndef _M_ARM
        UiUnInitialize("");
#endif
        UiInitialize(TRUE);
    }

Reboot:
    UiUnInitialize("Rebooting...");
    IniCleanup();
    return;
}
