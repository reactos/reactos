/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Implements routines to support booting from a RAM Disk.
 * COPYRIGHT:   Copyright 2008 ReactOS Portable Systems Group
 *              Copyright 2009 Hervé Poussineau
 *              Copyright 2019 Hermes Belusca-Maito
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include "../ntldr/ntldropts.h"

/* GLOBALS ********************************************************************/

PVOID gInitRamDiskBase = NULL;
ULONG gInitRamDiskSize = 0;

static BOOLEAN   RamDiskDeviceRegistered = FALSE;
static PVOID     RamDiskBase;
static ULONGLONG RamDiskFileSize;    // FIXME: RAM disks currently limited to 4GB.
static ULONGLONG RamDiskImageLength; // Size of valid data in the Ramdisk (usually == RamDiskFileSize - RamDiskImageOffset)
static ULONG     RamDiskImageOffset; // Starting offset from the Ramdisk base.
static ULONGLONG RamDiskOffset;      // Current position in the Ramdisk.

/* FUNCTIONS ******************************************************************/

static ARC_STATUS RamDiskClose(ULONG FileId)
{
    /* Nothing to do */
    return ESUCCESS;
}

static ARC_STATUS RamDiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = RamDiskImageLength;
    Information->CurrentAddress.QuadPart = RamDiskOffset;

    return ESUCCESS;
}

static ARC_STATUS RamDiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    /* Always return success, as contents are already in memory */
    return ESUCCESS;
}

static ARC_STATUS RamDiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PVOID StartAddress;

    /* Don't allow reads past our image */
    if (RamDiskOffset >= RamDiskImageLength || RamDiskOffset + N > RamDiskImageLength)
    {
        *Count = 0;
        return EIO;
    }
    // N = min(N, RamdiskImageLength - RamDiskOffset);

    /* Get actual pointer */
    StartAddress = (PVOID)((ULONG_PTR)RamDiskBase + RamDiskImageOffset + (ULONG_PTR)RamDiskOffset);

    /* Do the read */
    RtlCopyMemory(Buffer, StartAddress, N);
    RamDiskOffset += N;
    *Count = N;

    return ESUCCESS;
}

static ARC_STATUS RamDiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += RamDiskOffset;
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart >= RamDiskImageLength)
        return EINVAL;

    RamDiskOffset = NewPosition.QuadPart;
    return ESUCCESS;
}

static const DEVVTBL RamDiskVtbl =
{
    RamDiskClose,
    RamDiskGetFileInformation,
    RamDiskOpen,
    RamDiskRead,
    RamDiskSeek,
};

static ARC_STATUS
RamDiskLoadVirtualFile(
    IN PCSTR FileName,
    IN PCSTR DefaultPath OPTIONAL)
{
    ARC_STATUS Status;
    ULONG RamFileId;
    ULONG ChunkSize, Count;
    ULONGLONG TotalRead;
    ULONG PercentPerChunk, Percent;
    FILEINFORMATION Information;
    LARGE_INTEGER Position;

    /* Display progress */
    UiDrawProgressBarCenter("Loading RamDisk...");

    /* Try opening the Ramdisk file */
    Status = FsOpenFile(FileName, DefaultPath, OpenReadOnly, &RamFileId);
    if (Status != ESUCCESS)
        return Status;

    /* Get the file size */
    Status = ArcGetFileInformation(RamFileId, &Information);
    if (Status != ESUCCESS)
    {
        ArcClose(RamFileId);
        return Status;
    }

    /* FIXME: For now, limit RAM disks to 4GB */
    if (Information.EndingAddress.HighPart != 0)
    {
        ArcClose(RamFileId);
        UiMessageBox("RAM disk too big.");
        return ENOMEM;
    }
    RamDiskFileSize = Information.EndingAddress.QuadPart;
    ASSERT(RamDiskFileSize < 0x100000000); // See FIXME above.

    /* Allocate memory for it */
    ChunkSize = 8 * 1024 * 1024;
    if (RamDiskFileSize < ChunkSize)
        PercentPerChunk = 0;
    else
        PercentPerChunk = 100 * ChunkSize / RamDiskFileSize;
    RamDiskBase = MmAllocateMemoryWithType(RamDiskFileSize, LoaderXIPRom);
    if (!RamDiskBase)
    {
        RamDiskFileSize = 0;
        ArcClose(RamFileId);
        UiMessageBox("Failed to allocate memory for RAM disk.");
        return ENOMEM;
    }

    /*
     * Read it in chunks
     */
    Percent = 0;
    for (TotalRead = 0; TotalRead < RamDiskFileSize; TotalRead += ChunkSize)
    {
        /* Check if we're at the last chunk */
        if ((RamDiskFileSize - TotalRead) < ChunkSize)
        {
            /* Only need the actual data required */
            ChunkSize = (ULONG)(RamDiskFileSize - TotalRead);
        }

        /* Update progress */
        UiUpdateProgressBar(Percent, NULL);
        Percent += PercentPerChunk;

        /* Copy the contents */
        Position.QuadPart = TotalRead;
        Status = ArcSeek(RamFileId, &Position, SeekAbsolute);
        if (Status == ESUCCESS)
        {
            Status = ArcRead(RamFileId,
                             (PVOID)((ULONG_PTR)RamDiskBase + (ULONG_PTR)TotalRead),
                             ChunkSize,
                             &Count);
        }

        /* Check for success */
        if ((Status != ESUCCESS) || (Count != ChunkSize))
        {
            MmFreeMemory(RamDiskBase);
            RamDiskBase = NULL;
            RamDiskFileSize = 0;
            ArcClose(RamFileId);
            UiMessageBox("Failed to read RAM disk.");
            return ((Status != ESUCCESS) ? Status : EIO);
        }
    }
    UiUpdateProgressBar(100, NULL);

    ArcClose(RamFileId);

    return ESUCCESS;
}

ARC_STATUS
RamDiskInitialize(
    IN BOOLEAN InitRamDisk,
    IN PCSTR LoadOptions OPTIONAL,
    IN PCSTR DefaultPath OPTIONAL)
{
    /* Reset the RAMDISK device */
    if ((RamDiskBase != gInitRamDiskBase) &&
        (RamDiskFileSize != gInitRamDiskSize) &&
        (gInitRamDiskSize != 0))
    {
        /* This is not the initial Ramdisk, so we can free the allocated memory */
        MmFreeMemory(RamDiskBase);
    }
    RamDiskBase = NULL;
    RamDiskFileSize = 0;
    RamDiskImageLength = 0;
    RamDiskImageOffset = 0;
    RamDiskOffset = 0;

    if (InitRamDisk)
    {
        /* We initialize the initial Ramdisk: it should be present in memory */
        if (!gInitRamDiskBase || gInitRamDiskSize == 0)
            return ENODEV;

        // TODO: Handle SDI image.

        RamDiskBase = gInitRamDiskBase;
        RamDiskFileSize = gInitRamDiskSize;
        ASSERT(RamDiskFileSize < 0x100000000); // See FIXME about 4GB support in RamDiskLoadVirtualFile().
    }
    else
    {
        /* We initialize the Ramdisk from the load options */
        ARC_STATUS Status;
        CHAR FileName[MAX_PATH] = "";

        /* If we don't have any load options, initialize an empty Ramdisk */
        if (LoadOptions)
        {
            PCSTR Option;
            ULONG FileNameLength;

            /* Ramdisk image file name */
            Option = NtLdrGetOptionEx(LoadOptions, "RDPATH=", &FileNameLength);
            if (Option && (FileNameLength > 7))
            {
                /* Copy the file name */
                Option += 7; FileNameLength -= 7;
                RtlStringCbCopyNA(FileName, sizeof(FileName),
                                  Option, FileNameLength * sizeof(CHAR));
            }

            /* Ramdisk image length */
            Option = NtLdrGetOption(LoadOptions, "RDIMAGELENGTH=");
            if (Option)
            {
                RamDiskImageLength = _atoi64(Option + 14);
            }

            /* Ramdisk image offset */
            Option = NtLdrGetOption(LoadOptions, "RDIMAGEOFFSET=");
            if (Option)
            {
                RamDiskImageOffset = atol(Option + 14);
            }
        }

        if (*FileName)
        {
            Status = RamDiskLoadVirtualFile(FileName, DefaultPath);
            if (Status != ESUCCESS)
                return Status;
        }
    }

    /* Adjust the Ramdisk image length if needed */
    if (!RamDiskImageLength || (RamDiskImageLength > RamDiskFileSize - RamDiskImageOffset))
        RamDiskImageLength = RamDiskFileSize - RamDiskImageOffset;

    /* Register the RAMDISK device */
    if (!RamDiskDeviceRegistered)
    {
        FsRegisterDevice("ramdisk(0)", &RamDiskVtbl);
        RamDiskDeviceRegistered = TRUE;
    }

    return ESUCCESS;
}
