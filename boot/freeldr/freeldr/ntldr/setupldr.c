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

    TRACE("NLS data '%s' '%s' '%s'\n", AnsiName, OemName, LangName);

#if DBG
    {
        BOOLEAN Success = WinLdrLoadNLSData(LoaderBlock, SearchPath, AnsiName, OemName, LangName);
        TRACE("NLS data loading %s\n", Success ? "successful" : "failed");
    }    
#else
    WinLdrLoadNLSData(LoaderBlock, SearchPath, AnsiName, OemName, LangName);
#endif

    /* TODO: Load OEM HAL font */
    // Value "OemHalFont"
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
                    ERR("Could not add boot driver '%s', '%s'\n", SearchPath, DriverName);
                    return;
                }
            }
        }
    } while (InfFindNextLine(&InfContext, &InfContext));
}


/* SETUP STARTER **************************************************************/

ARC_STATUS
LoadReactOSSetup(
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[])
{
    PCSTR ArgValue;
    PCHAR File;
    CHAR FileName[512];
    CHAR BootPath[512];
    CHAR BootOptions2[256];
    LPCSTR LoadOptions;
    LPSTR BootOptions;
    BOOLEAN BootFromFloppy;
    BOOLEAN Success;
    ULONG i, ErrorLine;
    HINF InfHandle;
    INFCONTEXT InfContext;
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PSETUP_LOADER_BLOCK SetupBlock;
    LPCSTR SystemPath;

    static LPCSTR SourcePaths[] =
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

    UiDrawStatusText("Setup is loading...");

    UiDrawBackdrop();
    UiDrawProgressBarCenter(1, 100, "Loading ReactOS Setup...");

    /* Retrieve the system path */
    *BootPath = ANSI_NULL;
    ArgValue = GetArgumentValue(Argc, Argv, "SystemPath");
    if (ArgValue)
    {
        RtlStringCbCopyA(BootPath, sizeof(BootPath), ArgValue);
    }
    else
    {
        /*
         * IMPROVE: I don't want to call MachDiskGetBootPath here as a
         * default choice because I can call it after (see few lines below).
         * Instead I reset BootPath here so that we can build the full path
         * using the general code from below.
         */
        // MachDiskGetBootPath(BootPath, sizeof(BootPath));
        // RtlStringCbCopyA(BootPath, sizeof(BootPath), ArgValue);
        *BootPath = ANSI_NULL;
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
        RtlStringCbCopyA(FileName, sizeof(FileName), BootPath);

        /* This is not a full path. Use the current (i.e. boot) device. */
        MachDiskGetBootPath(BootPath, sizeof(BootPath));

        /* Append a path separator if needed */
        if (*FileName != '\\' && *FileName != '/')
            RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

        /* Append the remaining path */
        RtlStringCbCatA(BootPath, sizeof(BootPath), FileName);
    }

    /* Append a backslash if needed */
    if (!*BootPath || BootPath[strlen(BootPath) - 1] != '\\')
        RtlStringCbCatA(BootPath, sizeof(BootPath), "\\");

    TRACE("BootPath: '%s'\n", BootPath);

    /* Retrieve the boot options */
    *BootOptions2 = ANSI_NULL;
    ArgValue = GetArgumentValue(Argc, Argv, "Options");
    if (ArgValue)
        RtlStringCbCopyA(BootOptions2, sizeof(BootOptions2), ArgValue);

    TRACE("BootOptions: '%s'\n", BootOptions2);

    /* Check if a ramdisk file was given */
    File = strstr(BootOptions2, "/RDPATH=");
    if (File)
    {
        /* Copy the file name and everything else after it */
        RtlStringCbCopyA(FileName, sizeof(FileName), File + 8);

        /* Null-terminate */
        *strstr(FileName, " ") = ANSI_NULL;

        /* Load the ramdisk */
        if (!RamDiskLoadVirtualFile(FileName))
        {
            UiMessageBox("Failed to load RAM disk file %s", FileName);
            return ENOENT;
        }
    }

    /* Check if we booted from floppy */
    BootFromFloppy = strstr(BootPath, "fdisk") != NULL;

    /* Open 'txtsetup.sif' from any of source paths */
    File = BootPath + strlen(BootPath);
    for (i = BootFromFloppy ? 0 : 1; ; i++)
    {
        SystemPath = SourcePaths[i];
        if (!SystemPath)
        {
            UiMessageBox("Failed to open txtsetup.sif");
            return ENOENT;
        }
        RtlStringCbCopyA(File, sizeof(BootPath) - (File - BootPath)*sizeof(CHAR), SystemPath);
        RtlStringCbCopyA(FileName, sizeof(FileName), BootPath);
        RtlStringCbCatA(FileName, sizeof(FileName), "txtsetup.sif");
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
        return EINVAL;
    }

    if (!InfGetDataField(&InfContext, 1, &LoadOptions))
    {
        ERR("Failed to get load options\n");
        return EINVAL;
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
    ASSERT(BootOptions);
    strcpy(BootOptions, LoadOptions);

    TRACE("BootOptions: '%s'\n", BootOptions);

    /* Allocate and minimalist-initialize LPB */
    AllocateAndInitLPB(&LoaderBlock);

    /* Allocate and initialize setup loader block */
    SetupBlock = &WinLdrSystemBlock->SetupBlock;
    LoaderBlock->SetupLdrBlock = SetupBlock;

    /* Set textmode setup flag */
    SetupBlock->Flags = SETUPLDR_TEXT_MODE;

    /* Load the system hive "setupreg.hiv" for setup */
    UiDrawBackdrop();
    UiDrawProgressBarCenter(15, 100, "Loading setup system hive...");
    Success = WinLdrInitSystemHive(LoaderBlock, BootPath, TRUE);
    TRACE("Setup SYSTEM hive %s\n", (Success ? "loaded" : "not loaded"));
    /* Bail out if failure */
    if (!Success)
        return ENOEXEC;

    /* Load NLS data, they are in the System32 directory of the installation medium */
    RtlStringCbCopyA(FileName, sizeof(FileName), BootPath);
    RtlStringCbCatA(FileName, sizeof(FileName), "system32\\");
    SetupLdrLoadNlsData(LoaderBlock, InfHandle, FileName);

    // UiDrawStatusText("Press F6 if you need to install a 3rd-party SCSI or RAID driver...");

    /* Get a list of boot drivers */
    SetupLdrScanBootDrivers(&LoaderBlock->BootDriverListHead, InfHandle, BootPath);

    /* Close the inf file */
    InfCloseFile(InfHandle);

    UiDrawStatusText("The Setup program is starting...");

    /* Load ReactOS Setup */
    return LoadAndBootWindowsCommon(_WIN32_WINNT_WS03,
                                    LoaderBlock,
                                    BootOptions,
                                    BootPath,
                                    TRUE);
}
