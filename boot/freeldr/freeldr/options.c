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

/* GLOBALS ********************************************************************/

PCSTR OptionsMenuList[] =
{
    "Safe Mode",
    "Safe Mode with Networking",
    "Safe Mode with Command Prompt",

    NULL,

    "Enable Boot Logging",
    "Enable VGA Mode",
    "Last Known Good Configuration",
    "Directory Services Restore Mode",
    "Debugging Mode",
    "FreeLdr debugging",

    NULL,

    "Start ReactOS normally",
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    "Edit Boot Command Line (F10)",
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
    "Custom Boot",
#endif
#ifdef HAS_OPTION_MENU_REBOOT
    "Reboot",
#endif
};

const
PCSTR FrldrDbgMsg = "Enable FreeLdr debug channels\n"
                    "Acceptable syntax: [level1]#channel1[,[level2]#channel2]\n"
                    "level can be one of: trace,warn,fixme,err\n"
                    "  if the level is omitted all levels\n"
                    "  are enabled for the specified channel\n"
                    "# can be either + or -\n"
                    "channel can be one of the following:\n"
                    "  all,warning,memory,filesystem,inifile,ui,disk,cache,registry,\n"
                    "  reactos,linux,hwdetect,windows,peloader,scsiport,heap\n"
                    "Examples:\n"
                    "  trace+windows,trace+reactos\n"
                    "  +hwdetect,err-disk\n"
                    "  +peloader\n"
                    "NOTE: all letters must be lowercase, no spaces allowed.";

/* The boot options are mutually exclusive */
enum BootOption
{
    NO_OPTION = 0,

    SAFE_MODE,
    SAFE_MODE_WITH_NETWORKING,
    SAFE_MODE_WITH_COMMAND_PROMPT,

    LAST_KNOWN_GOOD_CONFIGURATION,
    DIRECTORY_SERVICES_RESTORE_MODE,
};

static enum BootOption BootOptionChoice = NO_OPTION;
static BOOLEAN BootLogging = FALSE;
static BOOLEAN VgaMode = FALSE;
static BOOLEAN DebuggingMode = FALSE;

/* FUNCTIONS ******************************************************************/

VOID DoOptionsMenu(IN OperatingSystemItem* OperatingSystem)
{
    ULONG SelectedMenuItem;
    CHAR  DebugChannelString[100];

    if (!UiDisplayMenu("Select an option:", NULL,
                       OptionsMenuList,
                       sizeof(OptionsMenuList) / sizeof(OptionsMenuList[0]),
                       11, // Use "Start ReactOS normally" as default; see the switch below.
                       -1,
                       &SelectedMenuItem,
                       TRUE,
                       NULL, NULL))
    {
        /* The user pressed ESC */
        return;
    }

    /* Clear the backdrop */
    UiDrawBackdrop();

    switch (SelectedMenuItem)
    {
        case 0: // Safe Mode
            BootOptionChoice = SAFE_MODE;
            BootLogging = TRUE;
            break;
        case 1: // Safe Mode with Networking
            BootOptionChoice = SAFE_MODE_WITH_NETWORKING;
            BootLogging = TRUE;
            break;
        case 2: // Safe Mode with Command Prompt
            BootOptionChoice = SAFE_MODE_WITH_COMMAND_PROMPT;
            BootLogging = TRUE;
            break;
        // case 3: // Separator
        //     break;
        case 4: // Enable Boot Logging
            BootLogging = TRUE;
            break;
        case 5: // Enable VGA Mode
            VgaMode = TRUE;
            break;
        case 6: // Last Known Good Configuration
            BootOptionChoice = LAST_KNOWN_GOOD_CONFIGURATION;
            break;
        case 7: // Directory Services Restore Mode
            BootOptionChoice = DIRECTORY_SERVICES_RESTORE_MODE;
            break;
        case 8: // Debugging Mode
            DebuggingMode = TRUE;
            break;
        case 9: // FreeLdr debugging
            DebugChannelString[0] = 0;
            if (UiEditBox(FrldrDbgMsg,
                          DebugChannelString,
                          sizeof(DebugChannelString) / sizeof(DebugChannelString[0])))
            {
                DbgParseDebugChannels(DebugChannelString);
            }
            break;
        // case 10: // Separator
        //     break;
        case 11: // Start ReactOS normally
            // Reset all the parameters to their default values.
            BootOptionChoice = NO_OPTION;
            BootLogging = FALSE;
            VgaMode = FALSE;
            DebuggingMode = FALSE;
            break;
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
        case 12: // Edit command line
            EditOperatingSystemEntry(OperatingSystem);
            break;
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
        case 13: // Custom Boot
            OptionMenuCustomBoot();
            break;
#endif
#ifdef HAS_OPTION_MENU_REBOOT
        case 14: // Reboot
            OptionMenuReboot();
            break;
#endif
    }
}

VOID DisplayBootTimeOptions(VOID)
{
    CHAR BootOptions[260] = "";

    switch (BootOptionChoice)
    {
        case SAFE_MODE:
            strcat(BootOptions, OptionsMenuList[0]);
            break;

        case SAFE_MODE_WITH_NETWORKING:
            strcat(BootOptions, OptionsMenuList[1]);
            break;

        case SAFE_MODE_WITH_COMMAND_PROMPT:
            strcat(BootOptions, OptionsMenuList[2]);
            break;

        case LAST_KNOWN_GOOD_CONFIGURATION:
            strcat(BootOptions, OptionsMenuList[6]);
            break;

        case DIRECTORY_SERVICES_RESTORE_MODE:
            strcat(BootOptions, OptionsMenuList[7]);
            break;

        default:
            break;
    }

    if (BootLogging)
    {
        if ( (BootOptionChoice != SAFE_MODE) &&
             (BootOptionChoice != SAFE_MODE_WITH_NETWORKING) &&
             (BootOptionChoice != SAFE_MODE_WITH_COMMAND_PROMPT) )
        {
            if (BootOptionChoice != NO_OPTION)
            {
                strcat(BootOptions, ", ");
            }
            strcat(BootOptions, OptionsMenuList[4]);
        }
    }

    if (VgaMode)
    {
        if ((BootOptionChoice != NO_OPTION) ||
             BootLogging)
        {
            strcat(BootOptions, ", ");
        }
        strcat(BootOptions, OptionsMenuList[5]);
    }

    if (DebuggingMode)
    {
        if ((BootOptionChoice != NO_OPTION) ||
             BootLogging || VgaMode)
        {
            strcat(BootOptions, ", ");
        }
        strcat(BootOptions, OptionsMenuList[8]);
    }

    /* Display the chosen boot options */
    UiDrawText(0,
               UiGetScreenHeight() - 2,
               BootOptions,
               ATTR(COLOR_LIGHTBLUE, UiGetMenuBgColor()));
}

VOID AppendBootTimeOptions(PCHAR BootOptions)
{
    switch (BootOptionChoice)
    {
        case SAFE_MODE:
            strcat(BootOptions, " /SAFEBOOT:MINIMAL /SOS /NOGUIBOOT");
            break;

        case SAFE_MODE_WITH_NETWORKING:
            strcat(BootOptions, " /SAFEBOOT:NETWORK /SOS /NOGUIBOOT");
            break;

        case SAFE_MODE_WITH_COMMAND_PROMPT:
            strcat(BootOptions, " /SAFEBOOT:MINIMAL(ALTERNATESHELL) /SOS /NOGUIBOOT");
            break;

        case LAST_KNOWN_GOOD_CONFIGURATION:
            DbgPrint("Last known good configuration is not supported yet!\n");
            break;

        case DIRECTORY_SERVICES_RESTORE_MODE:
            strcat(BootOptions, " /SAFEBOOT:DSREPAIR /SOS");
            break;

        default:
            break;
    }

    if (BootLogging)
        strcat(BootOptions, " /BOOTLOG");

    if (VgaMode)
        strcat(BootOptions, " /BASEVIDEO");

    if (DebuggingMode)
        strcat(BootOptions, " /DEBUG");
}
