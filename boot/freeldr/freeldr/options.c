/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FreeLoader Setup and Configuration F2 menu.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2012 Giannis Adamopoulos <gadamopoulos@reactos.org>
 *              Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <debug.h> // For DbgParseDebugChannels()

/* GLOBALS ********************************************************************/

typedef enum _FREELDR_SETUP_ACTION
{
    FreeldrSetupActionDebug,
    FreeldrSetupActionSeparator,
    FreeldrSetupActionEditCmdLine,
    FreeldrSetupActionCustomBoot,
    FreeldrSetupActionReboot,
    FreeldrSetupActionFirmwareSetup,
} FREELDR_SETUP_ACTION;

static PCSTR FrldrDbgMsg =
    "Enable FreeLdr debug channels\n"
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

/* FUNCTIONS ******************************************************************/

VOID
FreeLdrSetupMenu(
    _In_opt_ OperatingSystemItem* OperatingSystem)
{
    PCSTR OptionsMenuList[6];
    FREELDR_SETUP_ACTION OptionsMenuActions[RTL_NUMBER_OF(OptionsMenuList)];
    ULONG SelectedMenuItem = 0;
    ULONG MenuItemCount;

doMenu:
    MenuItemCount = 0;

    OptionsMenuList[MenuItemCount] = "FreeLdr debugging";
    OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionDebug;

    OptionsMenuList[MenuItemCount] = NULL;
    OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionSeparator;

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    OptionsMenuList[MenuItemCount] = "Edit Boot Command Line (F10)";
    OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionEditCmdLine;
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
    OptionsMenuList[MenuItemCount] = "Custom Boot";
    OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionCustomBoot;
#endif
#ifdef HAS_OPTION_MENU_REBOOT
    OptionsMenuList[MenuItemCount] = "Reboot";
    OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionReboot;
#endif
#ifdef UEFIBOOT
    if (UefiFirmwareSetupSupported())
    {
        OptionsMenuList[MenuItemCount] = "Reboot to Firmware Setup";
        OptionsMenuActions[MenuItemCount++] = FreeldrSetupActionFirmwareSetup;
    }
#endif

    if ((SelectedMenuItem >= MenuItemCount) ||
        (OptionsMenuList[SelectedMenuItem] == NULL))
    {
        SelectedMenuItem = 0;
    }

    /* Clear the backdrop */
    UiDrawBackdrop(UiGetScreenHeight());

    if (!UiDisplayMenu(VERSION " Setup and Configuration",
                       OperatingSystem ? NULL : "Press ESC to reboot.",
                       OptionsMenuList,
                       MenuItemCount,
                       SelectedMenuItem, -1,
                       &SelectedMenuItem,
                       TRUE,
                       NULL, NULL))
    {
        /* The user pressed ESC */
        return;
    }

    switch (OptionsMenuActions[SelectedMenuItem])
    {
        case FreeldrSetupActionDebug:
        {
            CHAR DebugChannelString[100] = "";
            // DebugChannelString[0] = ANSI_NULL;
            if (UiEditBox(FrldrDbgMsg,
                          DebugChannelString,
                          RTL_NUMBER_OF(DebugChannelString)))
            {
                DbgParseDebugChannels(DebugChannelString);
            }
            break;
        }
        case FreeldrSetupActionEditCmdLine:
            if (OperatingSystem)
                EditOperatingSystemEntry(OperatingSystem);
            break;
        case FreeldrSetupActionCustomBoot:
            OptionMenuCustomBoot();
            break;
        case FreeldrSetupActionReboot:
            OptionMenuReboot();
            break;
#ifdef UEFIBOOT
        case FreeldrSetupActionFirmwareSetup:
            UefiBootToFirmware();
            break;
#endif
        case FreeldrSetupActionSeparator:
        default:
            break;
    }
    goto doMenu;
}

/*
 * Display selected human-readable boot-option descriptions at the bottom of the screen.
 */
VOID
DisplayBootTimeOptions(
    _In_ OperatingSystemItem* OperatingSystem)
{
    if (!OperatingSystem->AdvBootOptsDesc[0])
        return;

    /* Display the chosen boot options */
    UiDrawText(0,
               UiGetScreenHeight() - 2,
               OperatingSystem->AdvBootOptsDesc,
               ATTR(COLOR_LIGHTBLUE, UiGetMenuBgColor()));
}
