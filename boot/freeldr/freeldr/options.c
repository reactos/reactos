/*
 * PROJECT:     FreeLoader
 * LICENSE:     Dual-licensed:
 *              GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     FreeLoader Setup and Configuration F2 menu.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2012 Giannis Adamopoulos <gadamopoulos@reactos.org>
 *              Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <debug.h> // For DbgParseDebugChannels()

/* GLOBALS ********************************************************************/

/* Named menu IDs */
typedef enum _FRLDR_SETUP_ACTION
{
    ActionSeparator = -1,
    ActionDebugging,
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    ActionEditBootCmdLine,
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
    ActionCustomBoot,
#endif
    ActionReboot,
    ActionBackToPrevMenu,
} FRLDR_SETUP_ACTION;

typedef struct _FRLDR_OPTIONS
{
    ULONG Id;
    PCSTR Item;
} FRLDR_OPTIONS;

static FRLDR_OPTIONS FrLdrOptions[] = // OptionsMenuList
{
    {ActionDebugging,       "FreeLdr debugging"},

    {ActionSeparator, NULL},

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    {ActionEditBootCmdLine, "Edit Boot Command Line (F10)"},
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
    {ActionCustomBoot,      "Custom Boot"},
#endif
    {ActionReboot,          "Reboot"},
    {ActionBackToPrevMenu,  "Return to OS Choices menu"},
};

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
    FRLDR_SETUP_ACTION MenuActionsMap[RTL_NUMBER_OF(FrLdrOptions)];
    PCSTR OptionsMenuList[RTL_NUMBER_OF(FrLdrOptions)]; // FIXME!
    ULONG i, MenuItemCount;
    ULONG SelectedMenuItem = 0;

    /* Build the menu, filtering out any item that may not be applicable */
    for (i = 0, MenuItemCount = 0; i < RTL_NUMBER_OF(FrLdrOptions); ++i)
    {
        /* Hide the "Return to OS Choices" item if no OS entry is selected */
        if ((FrLdrOptions[i].Id == ActionBackToPrevMenu) && !OperatingSystem)
            continue;
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
        if ((FrLdrOptions[i].Id == ActionEditBootCmdLine) && !OperatingSystem)
            continue;
#endif
        MenuActionsMap[MenuItemCount] = FrLdrOptions[i].Id;
        OptionsMenuList[MenuItemCount++] = FrLdrOptions[i].Item;
    }

doMenu:
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

    switch (MenuActionsMap[SelectedMenuItem])
    {
        case ActionDebugging:
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
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
        case ActionEditBootCmdLine:
            if (OperatingSystem)
                EditOperatingSystemEntry(OperatingSystem);
            break;
#endif
#ifdef HAS_OPTION_MENU_CUSTOM_BOOT
        case ActionCustomBoot:
            OptionMenuCustomBoot();
            break;
#endif
        case ActionReboot:
            OptionMenuReboot();
            return;
        case ActionBackToPrevMenu:
            // if (OperatingSystem)
            return; /* Just return to the caller */
        DEFAULT_UNREACHABLE;
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

VOID OptionMenuReboot(VOID)
{
    UiMessageBox("The system will now reboot.");
    Reboot();
}
