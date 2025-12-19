/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Command-line parsing and global settings management
 * COPYRIGHT:   Copyright 2008-2010 ReactOS Portable Systems Group <ros.arm@reactos.org>
 *              Copyright 2015-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

static CCHAR DebugString[256];
static CCHAR VideoOptions[256];
static CCHAR DefaultOs[256];
BOOTMGRINFO BootMgrInfo = {NULL, NULL, NULL, -1, 0};

/* FUNCTIONS ******************************************************************/

static VOID
CmdLineParse(
    _In_ PCSTR CmdLine)
{
    PCHAR Setting;
    size_t Length;
    ULONG_PTR Offset = 0;

    /*
     * Get the debug string, in the following format:
     * "debug=option1=XXX;option2=YYY;..."
     * and translate it into the format:
     * "OPTION1=XXX OPTION2=YYY ..."
     */
    Setting = strstr(CmdLine, "debug=");
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Setting += sizeof("debug=") - sizeof(ANSI_NULL);
        Length = strcspn(Setting, " \t");

        /* Copy the debug string and upcase it */
        RtlStringCbCopyNA(DebugString, sizeof(DebugString), Setting, Length);
        _strupr(DebugString);

        /* Replace all separators ';' by spaces */
        Setting = DebugString;
        while (*Setting)
        {
            if (*Setting == ';') *Setting = ' ';
            Setting++;
        }

        BootMgrInfo.DebugString = DebugString;
    }

    /* Get the video options */
    Setting = strstr(CmdLine, "video=");
    if (Setting)
    {
        Setting += sizeof("video=") - sizeof(ANSI_NULL);
    }
#if (defined(_M_IX86) || defined(_M_AMD64)) && !defined(SARCH_XBOX) && !defined(SARCH_PC98)
    else
    {
        Setting = strstr(CmdLine, "vga=");
        if (Setting)
            Setting += sizeof("vga=") - sizeof(ANSI_NULL);
    }
#endif
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Length = strcspn(Setting, " \t");

        /* Copy the string */
        RtlStringCbCopyNA(VideoOptions, sizeof(VideoOptions), Setting, Length);
        BootMgrInfo.VideoOptions = VideoOptions;
    }

    /* Get the timeout */
    Setting = strstr(CmdLine, "timeout=");
    if (Setting)
    {
        BootMgrInfo.TimeOut = atoi(Setting +
                                   sizeof("timeout=") - sizeof(ANSI_NULL));
    }

    /* Get the default OS */
    Setting = strstr(CmdLine, "defaultos=");
    if (Setting)
    {
        /* Check if there are more command-line parameters following */
        Setting += sizeof("defaultos=") - sizeof(ANSI_NULL);
        Length = strcspn(Setting, " \t");

        /* Copy the default OS */
        RtlStringCbCopyNA(DefaultOs, sizeof(DefaultOs), Setting, Length);
        BootMgrInfo.DefaultOs = DefaultOs;
    }

    /* Get the ramdisk base address */
    Setting = strstr(CmdLine, "rdbase=");
    if (Setting)
    {
        gInitRamDiskBase =
            (PVOID)(ULONG_PTR)strtoull(Setting +
                                       sizeof("rdbase=") - sizeof(ANSI_NULL),
                                       NULL, 0);
    }

    /* Get the ramdisk size */
    Setting = strstr(CmdLine, "rdsize=");
    if (Setting)
    {
        gInitRamDiskSize = strtoul(Setting +
                                   sizeof("rdsize=") - sizeof(ANSI_NULL),
                                   NULL, 0);
    }

    /* Get the ramdisk offset */
    Setting = strstr(CmdLine, "rdoffset=");
    if (Setting)
    {
        Offset = strtoul(Setting +
                         sizeof("rdoffset=") - sizeof(ANSI_NULL),
                         NULL, 0);
    }

    /* Fix it up */
    gInitRamDiskBase = (PVOID)((ULONG_PTR)gInitRamDiskBase + Offset);
}

VOID
LoadSettings(
    _In_opt_ PCSTR CmdLine)
{
    /* Pre-initialization: The settings originate from the command-line.
     * Main initialization: Overwrite them if needed with those from freeldr.ini */
    if (CmdLine)
    {
        CmdLineParse(CmdLine);
        return;
    }
    else if (IsListEmpty(IniGetFileSectionListHead()))
    {
        // ERR("LoadSettings() called but no freeldr.ini\n");
        return;
    }

    BOOLEAN FoundLoaderSection = FALSE;
    PCSTR LoaderSections[] = {
        "FreeLoader",
        "Boot Loader",
        "FlexBoot",
        "MultiBoot",
    };

    /* Search for the first section in LoaderSections and load the settings, 
     * prioritizing the order in the file.
     * If a section is already loaded, skip further checks. */
    if (!BootMgrInfo.FrLdrSection)
    {
        for (ULONG i = 0; i < sizeof(LoaderSections) / sizeof(LoaderSections[0]); i++)
        {
            PCSTR Section = LoaderSections[i];

            if (IniOpenSection(Section, &BootMgrInfo.FrLdrSection))
            {
                FoundLoaderSection = TRUE;
                break;
            }
        }

        if (!FoundLoaderSection)
        {
            UiMessageBoxCritical("Bootloader Section not found in freeldr.ini");
            return;
        }
    }

    /* Get the debug string. Always override it with the one from freeldr.ini */
    if (IniReadSettingByName(BootMgrInfo.FrLdrSection, "Debug",
                             DebugString, sizeof(DebugString)))
    {
        BootMgrInfo.DebugString = DebugString;
    }

    /* Get the timeout. Keep the existing one if it is valid,
     * otherwise retrieve it from freeldr.ini */
    if (BootMgrInfo.TimeOut < 0)
    {
        CHAR TimeOutText[20];
        BootMgrInfo.TimeOut = -1;
        if (IniReadSettingByName(BootMgrInfo.FrLdrSection, "TimeOut",
                                 TimeOutText, sizeof(TimeOutText)))
        {
            BootMgrInfo.TimeOut = atoi(TimeOutText);
        }
    }

    /* Get the default OS */
    if (!BootMgrInfo.DefaultOs || !*BootMgrInfo.DefaultOs)
    {
        if (IniReadSettingByName(BootMgrInfo.FrLdrSection, "DefaultOS",
                                 DefaultOs, sizeof(DefaultOs)))
        {
            BootMgrInfo.DefaultOs = DefaultOs;
        }
    }
}

PBOOTMGRINFO GetBootMgrInfo(VOID)
{
    return &BootMgrInfo;
}

/* EOF */
