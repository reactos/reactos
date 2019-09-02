/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/disk/ramdisk.c
 * PURPOSE:         Implements routines to support booting from a RAM Disk
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Hervé Poussineau
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>

/* GLOBALS ********************************************************************/

PVOID gRamDiskBase;
ULONG gRamDiskSize;
ULONG gRamDiskOffset;

/* FUNCTIONS ******************************************************************/

static ARC_STATUS RamDiskClose(ULONG FileId)
{
    //
    // Nothing to do
    //
    return ESUCCESS;
}

static ARC_STATUS RamDiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    //
    // Give current seek offset and ram disk size to caller
    //
    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.LowPart = gRamDiskSize;
    Information->CurrentAddress.LowPart = gRamDiskOffset;

    return ESUCCESS;
}

static ARC_STATUS RamDiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    //
    // Always return success, as contents are already in memory
    //
    return ESUCCESS;
}

static ARC_STATUS RamDiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PVOID StartAddress;

    //
    // Get actual pointer
    //
    StartAddress = (PVOID)((ULONG_PTR)gRamDiskBase + gRamDiskOffset);

    //
    // Don't allow reads past our image
    //
    if (gRamDiskOffset + N > gRamDiskSize)
    {
        *Count = 0;
        return EIO;
    }

    //
    // Do the read
    //
    RtlCopyMemory(Buffer, StartAddress, N);
    *Count = N;

    return ESUCCESS;
}

static ARC_STATUS RamDiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    //
    // Only accept absolute mode now
    //
    if (SeekMode != SeekAbsolute)
        return EINVAL;

    //
    // Check if we're in the ramdisk
    //
    if (Position->HighPart != 0)
        return EINVAL;
    if (Position->LowPart >= gRamDiskSize)
        return EINVAL;

    //
    // OK, remember seek position
    //
    gRamDiskOffset = Position->LowPart;

    return ESUCCESS;
}

static const DEVVTBL RamDiskVtbl = {
    RamDiskClose,
    RamDiskGetFileInformation,
    RamDiskOpen,
    RamDiskRead,
    RamDiskSeek,
};

VOID
RamDiskInitialize(VOID)
{
    /* Register the RAMDISK device */
    FsRegisterDevice("ramdisk(0)", &RamDiskVtbl);
}

ARC_STATUS
RamDiskLoadVirtualFile(
    IN PCSTR FileName,
    IN PCSTR DefaultPath OPTIONAL)
{
    ARC_STATUS Status;
    ULONG RamFileId;
    ULONG TotalRead, ChunkSize, Count;
    PCHAR MsgBuffer = "Loading RamDisk...";
    ULONG PercentPerChunk, Percent;
    FILEINFORMATION Information;
    LARGE_INTEGER Position;

    /* Display progress */
    UiDrawBackdrop();
    UiDrawProgressBarCenter(1, 100, MsgBuffer);

    /* Try opening the ramdisk file */
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

    /* For now, limit RAM disks to 4GB */
    if (Information.EndingAddress.HighPart != 0)
    {
        UiMessageBox("RAM disk too big.");
        ArcClose(RamFileId);
        return ENOMEM;
    }
    gRamDiskSize = Information.EndingAddress.LowPart;

    /* Allocate memory for it */
    ChunkSize = 8 * 1024 * 1024;
    if (gRamDiskSize < ChunkSize)
        Percent = PercentPerChunk = 0;
    else
        Percent = PercentPerChunk = 100 / (gRamDiskSize / ChunkSize);
    gRamDiskBase = MmAllocateMemoryWithType(gRamDiskSize, LoaderXIPRom);
    if (!gRamDiskBase)
    {
        UiMessageBox("Failed to allocate memory for RAM disk.");
        ArcClose(RamFileId);
        return ENOMEM;
    }

    /*
     * Read it in chunks
     */
    for (TotalRead = 0; TotalRead < gRamDiskSize; TotalRead += ChunkSize)
    {
        /* Check if we're at the last chunk */
        if ((gRamDiskSize - TotalRead) < ChunkSize)
        {
            /* Only need the actual data required */
            ChunkSize = gRamDiskSize - TotalRead;
        }

        /* Draw progress */
        UiDrawProgressBarCenter(Percent, 100, MsgBuffer);
        Percent += PercentPerChunk;

        /* Copy the contents */
        Position.HighPart = 0;
        Position.LowPart = TotalRead;
        Status = ArcSeek(RamFileId, &Position, SeekAbsolute);
        if (Status == ESUCCESS)
        {
            Status = ArcRead(RamFileId,
                             (PVOID)((ULONG_PTR)gRamDiskBase + TotalRead),
                             ChunkSize,
                             &Count);
        }

        /* Check for success */
        if ((Status != ESUCCESS) || (Count != ChunkSize))
        {
            MmFreeMemory(gRamDiskBase);
            gRamDiskBase = NULL;
            gRamDiskSize = 0;
            ArcClose(RamFileId);
            UiMessageBox("Failed to read RAM disk.");
            return ((Status != ESUCCESS) ? Status : EIO);
        }
    }

    ArcClose(RamFileId);

    /* Setup the RAMDISK device */
    RamDiskInitialize();

    return ESUCCESS;
}
