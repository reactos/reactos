/*
 * PROJECT:     NT-compatible ReactOS/Windows OS Loader
 * LICENSE:     Dual-licensed:
 *              GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Advanced Boot Options F8 menu.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2012-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include "ntldropts.h"

/* GLOBALS ********************************************************************/

/* Named menu IDs */
typedef enum _ADVOPTS_ACTION
{
    ActionSeparator = -1,
    ActionSafeBoot,
    ActionSafeBootNetwork,
    ActionSafeBootAltShell,
    ActionSafeBootDSRepair,
    ActionBootLog,
    ActionVGAMode,
    ActionLKGConfig,
    ActionDebugMode,
#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
    ActionBootAcpiApic,
    ActionBootAcpiSmp,
#endif
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    ActionEditBootCmdLine,
#endif
    ActionStartNormally,
    ActionReboot,
    ActionBackToPrevMenu,
} ADVOPTS_ACTION;

typedef struct _ADVBOOT_OPTIONS
{
    ULONG Id;
    PCSTR Item;
} ADVBOOT_OPTIONS;

static ADVBOOT_OPTIONS AdvBootOptions[] = // OptionsMenuList
{
    {ActionSafeBoot,            "Safe Mode"},
    {ActionSafeBootNetwork,     "Safe Mode with Networking"},
    {ActionSafeBootAltShell,    "Safe Mode with Command Prompt"},

    {ActionSeparator, NULL},

    {ActionBootLog,             "Enable Boot Logging"},
    {ActionVGAMode,             "Enable VGA Mode"},
    {ActionLKGConfig,           "Last Known Good Configuration"},
    {ActionSafeBootDSRepair,    "Directory Services Restore Mode"}, // "(ReactOS domain controllers only)"
    {ActionDebugMode,           "Debugging Mode"},

#if DBG && defined(_M_IX86)
    /* For x86 *ONLY*, allow the user to override HAL/Kernel auto-detection.
     * NOTE: This can always be done by manually editing the command-line if wanted. */
    {ActionBootAcpiApic, NULL},
    {ActionBootAcpiApic,        "Boot with ACPI APIC"},
    {ActionBootAcpiSmp,         "Boot with ACPI Multiprocessor"},
#endif

    {ActionSeparator, NULL},

#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
    {ActionEditBootCmdLine,     "Edit Boot Command Line (F10)"},
#endif
    {ActionStartNormally,       "Start ReactOS normally"},
    {ActionReboot,              "Reboot"},
    {ActionBackToPrevMenu,      "Return to OS Choices menu"},
};

/* Advanced NT boot options */
enum SAFEBOOT_MODE BootOptionChoice = NO_OPTION;
#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
UCHAR HALAutoDetectMode = BOOT_AUTODETECT;
#endif
LOGICAL BootFlags = 0;

/* FUNCTIONS ******************************************************************/

static VOID
GetBootOptionsDescription(
    _Inout_z_bytecount_(BootOptsDescSize)
         PSTR BootOptsDesc,
    _In_ SIZE_T BootOptsDescSize)
{
    /* NOTE: Keep in sync with the 'enum SAFEBOOT_MODE'
     * in winldr.h and the AdvBootOptions above. */
    static const PCSTR* OptionNames[] =
    {
        /* NO_OPTION         */ NULL,
        /* SAFEBOOT          */ &AdvBootOptions[0].Item,
        /* SAFEBOOT_NETWORK  */ &AdvBootOptions[1].Item,
        /* SAFEBOOT_ALTSHELL */ &AdvBootOptions[2].Item,
        /* SAFEBOOT_DSREPAIR */ &AdvBootOptions[7].Item,
        /* LKG_CONFIG        */ &AdvBootOptions[6].Item,
    };

    if (BootOptsDescSize < sizeof(CHAR))
        return;

    *BootOptsDesc = ANSI_NULL;

#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
    if ((HALAutoDetectMode != BOOT_AUTODETECT) && (HALAutoDetectMode <= BOOT_ACPI_SMP))
    {
        static const PCSTR AutoDetectNames[] = {"", "ACPI APIC", "ACPI Multiprocessor"};
        RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, AutoDetectNames[HALAutoDetectMode]);
    }
#endif

    ASSERT(BootOptionChoice < RTL_NUMBER_OF(OptionNames));
    if (BootOptionChoice != NO_OPTION) // && BootOptionChoice < RTL_NUMBER_OF(OptionNames)
    {
        if (*BootOptsDesc)
            RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, ", ");
        RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, *OptionNames[BootOptionChoice]);
    }

    if (BootFlags & BOOT_LOGGING)
    {
        /* Since these safe mode options come by default with boot logging,
         * don't show "Boot Logging" when one of these is selected;
         * instead just show the corresponding safe mode option name. */
        if ( (BootOptionChoice != SAFEBOOT) &&
             (BootOptionChoice != SAFEBOOT_NETWORK) &&
             (BootOptionChoice != SAFEBOOT_ALTSHELL) )
        {
            if (*BootOptsDesc)
                RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, ", ");
            RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, AdvBootOptions[4].Item);
        }
    }

    if (BootFlags & BOOT_VGA_MODE)
    {
        if (*BootOptsDesc)
            RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, ", ");
        RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, AdvBootOptions[5].Item);
    }

    if (BootFlags & BOOT_DEBUGGING)
    {
        if (*BootOptsDesc)
            RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, ", ");
        RtlStringCbCatA(BootOptsDesc, BootOptsDescSize, AdvBootOptions[8].Item);
    }
}

VOID
MenuNTOptions(
    _Inout_opt_ OperatingSystemItem* OperatingSystem)
{
    ADVOPTS_ACTION MenuActionsMap[RTL_NUMBER_OF(AdvBootOptions)];
    PCSTR OptionsMenuList[RTL_NUMBER_OF(AdvBootOptions)]; // FIXME!
    ULONG i, MenuItemCount;
    ULONG SelectedMenuItem = 0;

    /* Build the menu, filtering out any item that may not be applicable,
     * and set the "Start ReactOS normally" as default. */
    for (i = 0, MenuItemCount = 0; i < RTL_NUMBER_OF(AdvBootOptions); ++i)
    {
        /* Hide the "Return to OS Choices" item if no OS entry is selected */
        if ((AdvBootOptions[i].Id == ActionBackToPrevMenu) && !OperatingSystem)
            continue;
#if DBG && defined(_M_IX86)
        /* Filter out ActionBootAcpiApic/ActionBootAcpiSmp
         * if we aren't running on standard BIOS-based PC */
        if (AdvBootOptions[i].Id == ActionBootAcpiApic ||
            AdvBootOptions[i].Id == ActionBootAcpiSmp)
        {
#if defined(SARCH_XBOX) || defined(SARCH_PC98) || defined(UEFIBOOT)
            continue;
#endif
        }
#endif

        /* Set the default option */
        if (AdvBootOptions[i].Id == ActionStartNormally)
            SelectedMenuItem = MenuItemCount;

        MenuActionsMap[MenuItemCount] = AdvBootOptions[i].Id;
        OptionsMenuList[MenuItemCount++] = AdvBootOptions[i].Item;
    }

doMenu:
    /* Redraw the backdrop, but don't overwrite boot options */
    UiDrawBackdrop(UiGetScreenHeight() - 2);
    if (OperatingSystem)
        DisplayBootTimeOptions(OperatingSystem);

    if (!UiDisplayMenu("Please select an option:",
                       NULL,
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
        case ActionSafeBoot:
            BootOptionChoice = SAFEBOOT;
            BootFlags |= BOOT_LOGGING;
            break;
        case ActionSafeBootNetwork:
            BootOptionChoice = SAFEBOOT_NETWORK;
            BootFlags |= BOOT_LOGGING;
            break;
        case ActionSafeBootAltShell:
            BootOptionChoice = SAFEBOOT_ALTSHELL;
            BootFlags |= BOOT_LOGGING;
            break;
        case ActionBootLog:
            BootFlags |= BOOT_LOGGING;
            break;
        case ActionVGAMode:
            BootFlags |= BOOT_VGA_MODE;
            break;
        case ActionLKGConfig:
            BootOptionChoice = LKG_CONFIG;
            break;
        case ActionSafeBootDSRepair:
            BootOptionChoice = SAFEBOOT_DSREPAIR;
            break;
        case ActionDebugMode:
            BootFlags |= BOOT_DEBUGGING;
            break;
#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
        case ActionBootAcpiApic:
            HALAutoDetectMode = BOOT_ACPI_APIC;
            break;
        case ActionBootAcpiSmp:
            HALAutoDetectMode = BOOT_ACPI_SMP;
            break;
#endif
#ifdef HAS_OPTION_MENU_EDIT_CMDLINE
        case ActionEditBootCmdLine:
            if (OperatingSystem)
                EditOperatingSystemEntry(OperatingSystem);
            goto doMenu;
#endif
        case ActionStartNormally:
            /* Reset all the parameters to their default values */
            BootOptionChoice = NO_OPTION;
#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
            HALAutoDetectMode = BOOT_AUTODETECT;
#endif
            BootFlags = 0;
            break;
        case ActionReboot:
            OptionMenuReboot();
            return;
        case ActionBackToPrevMenu:
            return; /* Just return to the caller */
        DEFAULT_UNREACHABLE;
    }

    /* Update the human-readable boot-option description string */
    if (OperatingSystem)
    {
        GetBootOptionsDescription(OperatingSystem->AdvBootOptsDesc,
                                  sizeof(OperatingSystem->AdvBootOptsDesc));
    }
}

VOID
AppendBootTimeOptions(
    _Inout_z_bytecount_(BootOptionsSize)
         PSTR BootOptions,
    _In_ SIZE_T BootOptionsSize)
{
    /* NOTE: Keep in sync with the 'enum SAFEBOOT_MODE' in winldr.h */
    static const PCSTR OptionsStr[] =
    {
        /* NO_OPTION         */ NULL,
        /* SAFEBOOT          */ "SAFEBOOT:MINIMAL SOS NOGUIBOOT",
        /* SAFEBOOT_NETWORK  */ "SAFEBOOT:NETWORK SOS NOGUIBOOT",
        /* SAFEBOOT_ALTSHELL */ "SAFEBOOT:MINIMAL(ALTERNATESHELL) SOS NOGUIBOOT",
        /* SAFEBOOT_DSREPAIR */ "SAFEBOOT:DSREPAIR SOS",
        /* LKG_CONFIG        */ NULL,
    };

    PCSTR OptionsToAdd[2] = {NULL};
    PCSTR OptionsToRemove[2] = {NULL};

    if (BootOptionsSize < sizeof(CHAR))
        return;

#if DBG && defined(_M_IX86) // x86 *ONLY*: HAL/Kernel auto-detection override
    if ((HALAutoDetectMode != BOOT_AUTODETECT) && (HALAutoDetectMode <= BOOT_ACPI_SMP))
    {
        static const PCSTR AutoDetectOptions[] =
        {
            /* BOOT_AUTODETECT */ NULL,
            /* BOOT_ACPI_APIC  */ "HAL=halaacpi.dll",
            /* BOOT_ACPI_SMP   */ "HAL=halmacpi.dll KERNEL=ntkrnlmp.exe",
        };
        PCSTR OptionsToRemove[] = {"/HAL=/KERNEL=", "/DETECTHAL", NULL};

        OptionsToAdd[0] = AutoDetectOptions[HALAutoDetectMode];
        NtLdrUpdateOptions(BootOptions, BootOptionsSize, FALSE,
                           OptionsToAdd, OptionsToRemove);
    }
#endif

    switch (BootOptionChoice)
    {
        case SAFEBOOT:
        case SAFEBOOT_NETWORK:
        case SAFEBOOT_ALTSHELL:
        case SAFEBOOT_DSREPAIR:
        {
            ASSERT(BootOptionChoice < RTL_NUMBER_OF(OptionsStr));

            /* SAFEBOOT(:) options are self-excluding */
            OptionsToAdd[0] = OptionsStr[BootOptionChoice];
            OptionsToRemove[0] = "/SAFEBOOT/SAFEBOOT:";
            NtLdrUpdateOptions(BootOptions, BootOptionsSize, TRUE,
                               OptionsToAdd, OptionsToRemove);
            break;
        }

        case LKG_CONFIG:
            DbgPrint("Last known good configuration is not yet supported!\n");
            break;

        default:
            break;
    }

    if (BootFlags & BOOT_LOGGING)
        NtLdrAddOptions(BootOptions, BootOptionsSize, TRUE, "BOOTLOG");

    if (BootFlags & BOOT_VGA_MODE)
        NtLdrAddOptions(BootOptions, BootOptionsSize, TRUE, "BASEVIDEO");

    if (BootFlags & BOOT_DEBUGGING)
    {
        /* Remove NODEBUG if present, since we want to have debugging enabled */
        OptionsToAdd[0] = "DEBUG";
        OptionsToRemove[0] = "NODEBUG";
        NtLdrUpdateOptions(BootOptions, BootOptionsSize, TRUE,
                           OptionsToAdd, OptionsToRemove);
    }
}
