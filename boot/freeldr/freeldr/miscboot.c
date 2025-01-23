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

#if defined(_M_IX86) || defined(_M_AMD64)

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Loads and boots a disk MBR, a partition VBR or a file boot sector.
 **/
ARC_STATUS
LoadAndBootSector(
    _In_ ULONG Argc,
    _In_ PCHAR Argv[],
    _In_ PCHAR Envp[])
{
    ARC_STATUS Status;
    PCSTR ArgValue;
    PCSTR BootPath;
    PCSTR FileName;
    UCHAR BiosDriveNumber = 0;
    ULONG PartitionNumber = 0;
    ULONG LoadAddress;
    ULONG FileId;
    ULONG BytesRead;
    CHAR ArcPath[MAX_PATH];

#if DBG
    /* Ensure the boot type is the one expected */
    ArgValue = GetArgumentValue(Argc, Argv, "BootType");
    if (!ArgValue || !*ArgValue || _stricmp(ArgValue, "BootSector") != 0)
    {
        ERR("Unexpected boot type '%s', aborting\n", ArgValue ? ArgValue : "n/a");
        return EINVAL;
    }
#endif

    /* Find all the message box settings and run them */
    UiShowMessageBoxesInArgv(Argc, Argv);

    /*
     * Check whether we have a "BootPath" value (takes precedence
     * over both "BootDrive" and "BootPartition").
     */
    BootPath = GetArgumentValue(Argc, Argv, "BootPath");
    if (!BootPath || !*BootPath)
    {
        /* We don't have one, check whether we use "BootDrive" and "BootPartition" */

        /* Retrieve the boot drive (optional, fall back to using default path otherwise) */
        ArgValue = GetArgumentValue(Argc, Argv, "BootDrive");
        if (ArgValue && *ArgValue)
        {
            BiosDriveNumber = DriveMapGetBiosDriveNumber(ArgValue);

            /* Retrieve the boot partition (optional, fall back to zero otherwise) */
            PartitionNumber = 0;
            ArgValue = GetArgumentValue(Argc, Argv, "BootPartition");
            if (ArgValue && *ArgValue)
                PartitionNumber = atoi(ArgValue);
        }
        else
        {
            /* Fall back to using the system partition as default path */
            BootPath = GetArgumentValue(Argc, Argv, "SystemPartition");
        }
    }

    /*
     * The ARC "BootPath" value takes precedence over
     * both the BiosDriveNumber and PartitionNumber options.
     */
    if (BootPath && *BootPath)
    {
        /*
         * Retrieve the BIOS drive and partition numbers; verify also that the
         * path is "valid" in the sense that it must not contain any file name.
         */
        FileName = NULL;
        if (!DissectArcPath(BootPath, &FileName, &BiosDriveNumber, &PartitionNumber) ||
            (FileName && *FileName))
        {
            UiMessageBox("Currently unsupported BootPath value:\n%s", BootPath);
            return EINVAL;
        }
    }
    else
    {
        /* We don't have one, so construct the corresponding ARC path */
        ConstructArcPath(ArcPath, "", BiosDriveNumber, PartitionNumber);
        *strrchr(ArcPath, '\\') = ANSI_NULL; // Trim the trailing path separator.
        BootPath = ArcPath;
    }

    FileName = NULL;
    if (strstr(BootPath, ")partition()") || strstr(BootPath, ")partition(0)"))
    {
        /*
         * The partition specifier is zero i.e. the device is accessed
         * in an unpartitioned fashion, do not retrieve a file name.
         *
         * NOTE: If we access a floppy drive, we would not have a
         * partition specifier, and PartitionNumber would be == 0,
         * so don't check explicitly for PartitionNumber because
         * we want to retrieve a file name.
         */
    }
    else
    {
        /* Retrieve the file name, if any, and normalize
         * the pointer to make subsequent tests simpler */
        FileName = GetArgumentValue(Argc, Argv, "BootSectorFile");
        if (FileName && !*FileName)
            FileName = NULL;
    }


    /* If we load a boot sector file, reset the drive number
     * so as to use the original boot drive/partition */
    if (FileName)
        BiosDriveNumber = 0;
    if (!BiosDriveNumber)
    {
        BiosDriveNumber = FrldrGetBootDrive();
        PartitionNumber = FrldrGetBootPartition();
    }


    /* Open the boot sector file or the volume */
    if (FileName)
        Status = FsOpenFile(FileName, BootPath, OpenReadOnly, &FileId);
    else
        Status = ArcOpen((PSTR)BootPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        UiMessageBox("Unable to open %s", FileName ? FileName : BootPath);
        return Status;
    }

    LoadAddress = MachGetBootSectorLoadAddress(BiosDriveNumber);

    /*
     * Now try to load the boot sector: disk MBR (when PartitionNumber == 0),
     * partition VBR or boot sector file. If this fails, abort.
     */
    Status = ArcRead(FileId, UlongToPtr(LoadAddress), 512, &BytesRead);
    ArcClose(FileId);
    if ((Status != ESUCCESS) || (BytesRead != 512))
    {
        PCSTR WhatFailed;

        if (FileName)
            WhatFailed = "boot sector file";
        else if (PartitionNumber != 0)
            WhatFailed = "partition's boot sector";
        else
            WhatFailed = "MBR boot sector";

        UiMessageBox("Unable to load %s.", WhatFailed);
        return EIO;
    }

    /* Check for validity */
    if (*(USHORT*)UlongToPtr(LoadAddress + 0x1FE) != 0xAA55)
    {
        UiMessageBox("Invalid boot sector magic (0xAA55)");
        return ENOEXEC;
    }

    UiUnInitialize("Booting...");
    IniCleanup();

#ifndef UEFIBOOT
    /* Boot the loaded sector code */
    ChainLoadBiosBootSectorCode(BiosDriveNumber, PartitionNumber);
#endif
    /* Must not return! */
    return ESUCCESS;
}

#endif /* _M_IX86 || _M_AMD64 */

/* EOF */
