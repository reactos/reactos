/*
 *  FreeLoader
 *
 *  Copyright (C) 2009       Aleksey Bragin  <aleksey@reactos.org>
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

#include <freeldr.h>
#include "winldr.h"

#include <ndk/ldrtypes.h>
#include <arc/setupblk.h>

#include <debug.h>

DBG_DEFAULT_CHANNEL(WINDOWS);
#define TAG_BOOT_OPTIONS 'pOtB'

// TODO: Move to .h
VOID AllocateAndInitLPB(PLOADER_PARAMETER_BLOCK *OutLoaderBlock);

static VOID
SetupLdrLoadNlsData(PLOADER_PARAMETER_BLOCK LoaderBlock, HINF InfHandle, LPCSTR SearchPath)
{
    INFCONTEXT InfContext;
    BOOLEAN Success;
    LPCSTR AnsiName, OemName, LangName;

    /* Get ANSI codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "AnsiCodepage", &InfContext))
    {
        ERR("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &AnsiName))
    {
        ERR("Failed to get load options\n");
        return;
    }

    /* Get OEM codepage file */
    if (!InfFindFirstLine(InfHandle, "NLS", "OemCodepage", &InfContext))
    {
        ERR("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &OemName))
    {
        ERR("Failed to get load options\n");
        return;
    }

    if (!InfFindFirstLine(InfHandle, "NLS", "UnicodeCasetable", &InfContext))
    {
        ERR("Failed to find 'NLS/AnsiCodepage'\n");
        return;
    }
    if (!InfGetDataField(&InfContext, 1, &LangName))
    {
        ERR("Failed to get load options\n");
        return;
    }

    Success = WinLdrLoadNLSData(LoaderBlock, SearchPath, AnsiName, OemName, LangName);
    TRACE("NLS data loading %s\n", Success ? "successful" : "failed");
}

static VOID
SetupLdrScanBootDrivers(PLIST_ENTRY BootDriverListHead, HINF InfHandle, LPCSTR SearchPath)
{
    INFCONTEXT InfContext, dirContext;
    BOOLEAN Success;
    LPCSTR Media, DriverName, dirIndex, ImagePath;
    WCHAR ServiceName[256];
    WCHAR ImagePathW[256];

    /* Open inf section */
    if (!InfFindFirstLine(InfHandle, "SourceDisksFiles", NULL, &InfContext))
        return;

    /* Load all listed boot drivers */
    do
    {
        if (InfGetDataField(&InfContext, 7, &Media) &&
            InfGetDataField(&InfContext, 0, &DriverName) &&
            InfGetDataField(&InfContext, 13, &dirIndex))
        {
            if ((strcmp(Media, "x") == 0) &&
                InfFindFirstLine(InfHandle, "Directories", dirIndex, &dirContext) &&
                InfGetDataField(&dirContext, 1, &ImagePath))
            {
                /* Convert name to widechar */
                swprintf(ServiceName, L"%S", DriverName);

                /* Prepare image path */
                swprintf(ImagePathW, L"%S", ImagePath);
                wcscat(ImagePathW, L"\\");
                wcscat(ImagePathW, ServiceName);

                /* Remove .sys extension */
                ServiceName[wcslen(ServiceName) - 4] = 0;

                /* Add it to the list */
                Success = WinLdrAddDriverToList(BootDriverListHead,
                                                L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\",
                                                ImagePathW,
                                                ServiceName);
                if (!Success)
                {
                    ERR("could not add boot driver %s, %s\n", SearchPath, DriverName);
                    return;
                }
            }
        }
    } while (InfFindNextLine(&InfContext, &InfContext));
}

VOID
LoadReactOSSetup(IN OperatingSystemItem* OperatingSystem,
                 IN USHORT OperatingSystemVersion)
{
    ULONG_PTR SectionId;
    PCSTR SectionName = OperatingSystem->SystemPartition;
    CHAR  SettingsValue[80];
    BOOLEAN HasSection;
    CHAR  BootOptions2[256];
    PCHAR File;
    CHAR FileName[512];
    CHAR BootPath[512];
    LPCSTR LoadOptions;
    LPSTR BootOptions;
    BOOLEAN BootFromFloppy;
    ULONG i, ErrorLine;
    HINF InfHandle;
    INFCONTEXT InfContext;
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PSETUP_LOADER_BLOCK SetupBlock;
    LPCSTR SystemPath;
    LPCSTR SourcePaths[] =
    {
        "", /* Only for floppy boot */
#if defined(_M_IX86)
        "I386\\",
#elif defined(_M_MPPC)
        "PPC\\",
#elif defined(_M_MRX000)
        "MIPS\\",
#endif
        "reactos\\",
        NULL
    };

    /* Get OS setting value */
    SettingsValue[0] = ANSI_NULL;
    IniOpenSection("Operating Systems", &SectionId);
    IniReadSettingByName(SectionId, SectionName, SettingsValue, sizeof(SettingsValue));

    /* Open the operating system section specified in the .ini file */
    HasSection = IniOpenSection(SectionName, &SectionId);

    UiDrawBackdrop();
    UiDrawProgressBarCenter(1, 100, "Loading ReactOS Setup...");

    /* Read the system path is set in the .ini file */
    if (!HasSection ||
        !IniReadSettingByName(SectionId, "SystemPath", BootPath, sizeof(BootPath)))
    {
        /*
         * IMPROVE: I don't want to call MachDiskGetBootPath here as a
         * default choice because I can call it after (see few lines below).
         * Also doing the strcpy call as it is done in winldr.c is not
         * really what we want. Instead I reset BootPath here so that
         * we can build the full path using the general code from below.
         */
        // MachDiskGetBootPath(BootPath, sizeof(BootPath));
        // strcpy(BootPath, SectionName);
        BootPath[0] = '\0';
    }

    /*
     * Check whether BootPath is a full path
     * and if not, create a full boot path.
     *
     * See FsOpenFile for the technique used.
     */
    if (strrchr(BootPath, ')') == NULL)
    {
        /* Temporarily save the boot path */
        strcpy(FileName, BootPath);

        /* This is not a full path. Use the current (i.e. boot) device. */
        MachDiskGetBootPath(BootPath, sizeof(BootPath));

        /* Append a path separator if needed */
        if (FileName[0] != '\\' && FileName[0] != '/')
            strcat(BootPath, "\\");

        /* Append the remaining path */
        strcat(BootPath, FileName);
    }

    /* Append a backslash if needed */
    if ((strlen(BootPath) == 0) || BootPath[strlen(BootPath) - 1] != '\\')
        strcat(BootPath, "\\");

    /* Read booting options */
    if (!HasSection || !IniReadSettingByName(SectionId, "Options", BootOptions2, sizeof(BootOptions2)))
    {
        /* Get options after the title */
        PCSTR p = SettingsValue;
        while (*p == ' ' || *p == '"')
            p++;
        while (*p != '\0' && *p != '"')
            p++;
        strcpy(BootOptions2, p);
        TRACE("BootOptions: '%s'\n", BootOptions2);
    }

    /* Check if a ramdisk file was given */
    File = strstr(BootOptions2, "/RDPATH=");
    if (File)
    {
        /* Copy the file name and everything else after it */
        strcpy(FileName, File + 8);

        /* Null-terminate */
        *strstr(FileName, " ") = ANSI_NULL;

        /* Load the ramdisk */
        if (!RamDiskLoadVirtualFile(FileName))
        {
            UiMessageBox("Failed to load RAM disk file %s", FileName);
            return;
        }
    }

    TRACE("BootPath: '%s'\n", BootPath);

    /* And check if we booted from floppy */
    BootFromFloppy = strstr(BootPath, "fdisk") != NULL;

    /* Open 'txtsetup.sif' from any of source paths */
    File = BootPath + strlen(BootPath);
    for (i = BootFromFloppy ? 0 : 1; ; i++)
    {
        SystemPath = SourcePaths[i];
        if (!SystemPath)
        {
            UiMessageBox("Failed to open txtsetup.sif");
            return;
        }
        strcpy(File, SystemPath);
        strcpy(FileName, BootPath);
        strcat(FileName, "txtsetup.sif");
        if (InfOpenFile(&InfHandle, FileName, &ErrorLine))
        {
            break;
        }
    }

    TRACE("BootPath: '%s', SystemPath: '%s'\n", BootPath, SystemPath);

    /* Get Load options - debug and non-debug */
    if (!InfFindFirstLine(InfHandle, "SetupData", "OsLoadOptions", &InfContext))
    {
        ERR("Failed to find 'SetupData/OsLoadOptions'\n");
        return;
    }

    if (!InfGetDataField(&InfContext, 1, &LoadOptions))
    {
        ERR("Failed to get load options\n");
        return;
    }

#if DBG
    /* Get debug load options and use them */
    if (InfFindFirstLine(InfHandle, "SetupData", "DbgOsLoadOptions", &InfContext))
    {
        LPCSTR DbgLoadOptions;

        if (InfGetDataField(&InfContext, 1, &DbgLoadOptions))
            LoadOptions = DbgLoadOptions;
    }
#endif

    /* Copy loadoptions (original string will be freed) */
    BootOptions = FrLdrTempAlloc(strlen(LoadOptions) + 1, TAG_BOOT_OPTIONS);
    strcpy(BootOptions, LoadOptions);

    TRACE("BootOptions: '%s'\n", BootOptions);

    UiDrawStatusText("Setup is loading...");

    /* Allocate and minimalist-initialize LPB */
    AllocateAndInitLPB(&LoaderBlock);

    /* Allocate and initialize setup loader block */
    SetupBlock = &WinLdrSystemBlock->SetupBlock;
    LoaderBlock->SetupLdrBlock = SetupBlock;

    /* Set textmode setup flag */
    SetupBlock->Flags = SETUPLDR_TEXT_MODE;

    /* Load NLS data, they are in system32 */
    strcpy(FileName, BootPath);
    strcat(FileName, "system32\\");
    SetupLdrLoadNlsData(LoaderBlock, InfHandle, FileName);

    /* Get a list of boot drivers */
    SetupLdrScanBootDrivers(&LoaderBlock->BootDriverListHead, InfHandle, BootPath);

    /* Close the inf file */
    InfCloseFile(InfHandle);

    /* Load ReactOS Setup */
    LoadAndBootWindowsCommon(_WIN32_WINNT_WS03,
                             LoaderBlock,
                             BootOptions,
                             BootPath,
                             TRUE);
}
