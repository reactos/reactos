/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/disk.c
 * PURPOSE:         Generic Disk Controller (Floppy, Hard Disk, ...)
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE 1: This file is meant to be splitted into FDC and HDC
 *         when its code will grow out of control!
 *
 * NOTE 2: This is poor-man implementation, i.e. at the moment this file
 *         contains an API for manipulating the disks for the rest of NTVDM,
 *         but does not implement real hardware emulation (IO ports, etc...).
 *         One will have to progressively transform it into a real HW emulation
 *         and, in case the disk APIs are needed, move them elsewhere.
 *
 * FIXME:  The big endian support (which is hardcoded here for machines
 *         in little endian) *MUST* be fixed!
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "disk.h"

// #include "io.h"
#include "memory.h"

#include "utils.h"


/**************** HARD DRIVES -- VHD FIXED DISK FORMAT SUPPORT ****************/

// https://web.archive.org/web/20160131080555/http://citrixblogger.org/2008/12/01/dynamic-vhd-walkthrough/
// https://www.microsoft.com/en-us/download/details.aspx?id=23850
// https://projects.honeynet.org/svn/sebek/virtualization/qebek/trunk/block/vpc.c (DEAD_LINK)
// https://git.virtualopensystems.com/trescca/qemu/raw/40645c7bfd7c4d45381927e1e80081fa827c368a/block/vpc.c
// https://gitweb.gentoo.org/proj/qemu-kvm.git/tree/block/vpc.c?h=qemu-kvm-0.12.4-gentoo&id=827dccd6740639c64732418539bf17e6e4c99d77

#pragma pack(push, 1)

enum VHD_TYPE
{
    VHD_FIXED           = 2,
    VHD_DYNAMIC         = 3,
    VHD_DIFFERENCING    = 4,
};

// Seconds since Jan 1, 2000 0:00:00 (UTC)
#define VHD_TIMESTAMP_BASE 946684800

// Always in BIG-endian format!
typedef struct _VHD_FOOTER
{
    CHAR        creator[8]; // "conectix"
    ULONG       features;
    ULONG       version;

    // Offset of next header structure, 0xFFFFFFFF if none
    ULONG64     data_offset;

    // Seconds since Jan 1, 2000 0:00:00 (UTC)
    ULONG       timestamp;

    CHAR        creator_app[4]; // "vpc "; "win"
    USHORT      major;
    USHORT      minor;
    CHAR        creator_os[4]; // "Wi2k"

    ULONG64     orig_size;
    ULONG64     size;

    USHORT      cyls;
    BYTE        heads;
    BYTE        secs_per_cyl;

    ULONG       type;   // VHD_TYPE

    // Checksum of the Hard Disk Footer ("one's complement of the sum of all
    // the bytes in the footer without the checksum field")
    ULONG       checksum;

    // UUID used to identify a parent hard disk (backing file)
    BYTE        uuid[16];

    BYTE        in_saved_state;

    BYTE Padding[0x200-0x55];

} VHD_FOOTER, *PVHD_FOOTER;
C_ASSERT(sizeof(VHD_FOOTER) == 0x200);

#pragma pack(pop)

#if 0
/*
 * Calculates the number of cylinders, heads and sectors per cylinder
 * based on a given number of sectors. This is the algorithm described
 * in the VHD specification.
 *
 * Note that the geometry doesn't always exactly match total_sectors but
 * may round it down.
 *
 * Returns TRUE on success, FALSE if the size is larger than 127 GB
 */
static BOOLEAN
calculate_geometry(ULONG64 total_sectors, PUSHORT cyls,
                   PBYTE heads, PBYTE secs_per_cyl)
{
    ULONG cyls_times_heads;

    if (total_sectors > 65535 * 16 * 255)
        return FALSE;

    if (total_sectors > 65535 * 16 * 63)
    {
        *secs_per_cyl = 255;
        *heads = 16;
        cyls_times_heads = total_sectors / *secs_per_cyl;
    }
    else
    {
        *secs_per_cyl = 17;
        cyls_times_heads = total_sectors / *secs_per_cyl;
        *heads = (cyls_times_heads + 1023) / 1024;

        if (*heads < 4)
            *heads = 4;

        if (cyls_times_heads >= (*heads * 1024) || *heads > 16)
        {
            *secs_per_cyl = 31;
            *heads = 16;
            cyls_times_heads = total_sectors / *secs_per_cyl;
        }

        if (cyls_times_heads >= (*heads * 1024))
        {
            *secs_per_cyl = 63;
            *heads = 16;
            cyls_times_heads = total_sectors / *secs_per_cyl;
        }
    }

    *cyls = cyls_times_heads / *heads;

    return TRUE;
}
#endif



/*************************** FLOPPY DISK CONTROLLER ***************************/

// A Floppy Controller can support up to 4 floppy drives.
static DISK_IMAGE XDCFloppyDrive[4];

// Taken from DOSBox
typedef struct _DISK_GEO
{
    DWORD ksize;     /* Size in kilobytes */
    WORD  secttrack; /* Sectors per track */
    WORD  headscyl;  /* Heads per cylinder */
    WORD  cylcount;  /* Cylinders per side */
    WORD  biosval;   /* Type to return from BIOS & CMOS */
} DISK_GEO, *PDISK_GEO;

// FIXME: At the moment, all of our diskettes have 512 bytes per sector...
static WORD HackSectorSize = 512;
static DISK_GEO DiskGeometryList[] =
{
    { 160,  8, 1, 40, 0},
    { 180,  9, 1, 40, 0},
    { 200, 10, 1, 40, 0},
    { 320,  8, 2, 40, 1},
    { 360,  9, 2, 40, 1},
    { 400, 10, 2, 40, 1},
    { 720,  9, 2, 80, 3},
    {1200, 15, 2, 80, 2},
    {1440, 18, 2, 80, 4},
    {2880, 36, 2, 80, 6},
};

static BOOLEAN
MountFDI(IN PDISK_IMAGE DiskImage,
         IN HANDLE hFile)
{
    ULONG  FileSize;
    USHORT i;

    /*
     * Retrieve the size of the file. In NTVDM we will handle files
     * of maximum 1Mb so we can largely use GetFileSize only.
     */
    FileSize = GetFileSize(hFile, NULL);
    if (FileSize == INVALID_FILE_SIZE && GetLastError() != ERROR_SUCCESS)
    {
        /* We failed, bail out */
        DisplayMessage(L"MountFDI: Error when retrieving file size, or size too large (%d).", FileSize);
        return FALSE;
    }

    /* Convert the size in kB */
    FileSize /= 1024;

    /* Find the floppy format in the list, and mount it if found */
    for (i = 0; i < ARRAYSIZE(DiskGeometryList); ++i)
    {
        if (DiskGeometryList[i].ksize     == FileSize ||
            DiskGeometryList[i].ksize + 1 == FileSize)
        {
            /* Found, mount it */
            DiskImage->DiskType = DiskGeometryList[i].biosval;
            DiskImage->DiskInfo.Cylinders = DiskGeometryList[i].cylcount;
            DiskImage->DiskInfo.Heads     = DiskGeometryList[i].headscyl;
            DiskImage->DiskInfo.Sectors   = DiskGeometryList[i].secttrack;
            DiskImage->DiskInfo.SectorSize = HackSectorSize;

            /* Set the file pointer to the beginning */
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

            DiskImage->hDisk = hFile;
            return TRUE;
        }
    }

    /* If we are here, we failed to find a suitable format. Bail out. */
    DisplayMessage(L"MountFDI: Floppy image of invalid size %d.", FileSize);
    return FALSE;
}


/************************** IDE HARD DISK CONTROLLER **************************/

// An IDE Hard Disk Controller can support up to 4 drives:
// Primary Master Drive, Primary Slave Drive,
// Secondary Master Drive, Secondary Slave Drive.
static DISK_IMAGE XDCHardDrive[4];

static BOOLEAN
MountHDD(IN PDISK_IMAGE DiskImage,
         IN HANDLE hFile)
{
    /**** Support for VHD fixed disks ****/
    DWORD FilePointer, BytesToRead;
    VHD_FOOTER vhd_footer;

    /* Go to the end of the file and retrieve the footer */
    FilePointer = SetFilePointer(hFile, -(LONG)sizeof(VHD_FOOTER), NULL, FILE_END);
    if (FilePointer == INVALID_SET_FILE_POINTER)
    {
        DPRINT1("MountHDD: Error when seeking HDD footer, last error = %d\n", GetLastError());
        return FALSE;
    }

    /* Read footer */
    // FIXME: We may consider just mapping section to the file...
    BytesToRead = sizeof(VHD_FOOTER);
    if (!ReadFile(hFile, &vhd_footer, BytesToRead, &BytesToRead, NULL))
    {
        DPRINT1("MountHDD: Error when reading HDD footer, last error = %d\n", GetLastError());
        return FALSE;
    }

    /* Perform validity checks */
    if (RtlCompareMemory(vhd_footer.creator, "conectix",
                         sizeof(vhd_footer.creator)) != sizeof(vhd_footer.creator))
    {
        DisplayMessage(L"MountHDD: Invalid HDD image (expected VHD).");
        return FALSE;
    }
    if (vhd_footer.version != 0x00000100 &&
        vhd_footer.version != 0x00000500) // FIXME: Big endian!
    {
        DisplayMessage(L"MountHDD: VHD HDD image of unexpected version %d.", vhd_footer.version);
        return FALSE;
    }
    if (RtlUlongByteSwap(vhd_footer.type) != VHD_FIXED) // FIXME: Big endian!
    {
        DisplayMessage(L"MountHDD: Only VHD HDD fixed images are supported.");
        return FALSE;
    }
    if (vhd_footer.data_offset != 0xFFFFFFFFFFFFFFFF)
    {
        DisplayMessage(L"MountHDD: Unexpected data offset for VHD HDD fixed image.");
        return FALSE;
    }
    if (vhd_footer.orig_size != vhd_footer.size)
    {
        DisplayMessage(L"MountHDD: VHD HDD fixed image size should be the same as its original size.");
        return FALSE;
    }
    // FIXME: Checksum!

    /* Found, mount it */
    DiskImage->DiskType = 0;
    DiskImage->DiskInfo.Cylinders = RtlUshortByteSwap(vhd_footer.cyls); // FIXME: Big endian!
    DiskImage->DiskInfo.Heads     = vhd_footer.heads;
    DiskImage->DiskInfo.Sectors   = vhd_footer.secs_per_cyl;
    DiskImage->DiskInfo.SectorSize = RtlUlonglongByteSwap(vhd_footer.size) / // FIXME: Big endian!
                                     DiskImage->DiskInfo.Cylinders /
                                     DiskImage->DiskInfo.Heads / DiskImage->DiskInfo.Sectors;

    /* Set the file pointer to the beginning */
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    DiskImage->hDisk = hFile;
    return TRUE;
}



/************************ GENERIC DISK CONTROLLER API *************************/

BOOLEAN
IsDiskPresent(IN PDISK_IMAGE DiskImage)
{
    ASSERT(DiskImage);
    return (DiskImage->hDisk != INVALID_HANDLE_VALUE && DiskImage->hDisk != NULL);
}

BYTE
SeekDisk(IN PDISK_IMAGE DiskImage,
         IN WORD Cylinder,
         IN BYTE Head,
         IN BYTE Sector)
{
    DWORD FilePointer;

    /* Check that the sector number is 1-based */
    // FIXME: Or do it in the caller?
    if (Sector == 0)
    {
        /* Return error */
        return 0x01;
    }

    /* Set position */
    // Compute the offset
    FilePointer = (DWORD)((DWORD)((DWORD)Cylinder * DiskImage->DiskInfo.Heads + Head)
                    * DiskImage->DiskInfo.Sectors + (Sector - 1))
                    * DiskImage->DiskInfo.SectorSize;
    FilePointer = SetFilePointer(DiskImage->hDisk, FilePointer, NULL, FILE_BEGIN);
    if (FilePointer == INVALID_SET_FILE_POINTER)
    {
        /* Return error */
        return 0x40;
    }

    return 0x00;
}

BYTE
ReadDisk(IN PDISK_IMAGE DiskImage,
         IN WORD Cylinder,
         IN BYTE Head,
         IN BYTE Sector,
         IN BYTE NumSectors)
{
    BYTE Result;
    DWORD BytesToRead;

    PVOID LocalBuffer;
    BYTE StaticBuffer[1024];

    /* Read the sectors */
    Result = SeekDisk(DiskImage, Cylinder, Head, Sector);
    if (Result != 0x00)
        return Result;

    BytesToRead = (DWORD)NumSectors * DiskImage->DiskInfo.SectorSize;

    // FIXME: Consider just looping around filling each time the buffer...

    if (BytesToRead <= sizeof(StaticBuffer))
    {
        LocalBuffer = StaticBuffer;
    }
    else
    {
        LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, BytesToRead);
        ASSERT(LocalBuffer != NULL);
    }

    if (ReadFile(DiskImage->hDisk, LocalBuffer, BytesToRead, &BytesToRead, NULL))
    {
        /* Write to the memory */
        EmulatorWriteMemory(&EmulatorContext,
                            TO_LINEAR(getES(), getBX()),
                            LocalBuffer,
                            BytesToRead);

        Result = 0x00;
    }
    else
    {
        Result = 0x04;
    }

    if (LocalBuffer != StaticBuffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);

    /* Return success or error */
    return Result;
}

BYTE
WriteDisk(IN PDISK_IMAGE DiskImage,
          IN WORD Cylinder,
          IN BYTE Head,
          IN BYTE Sector,
          IN BYTE NumSectors)
{
    BYTE Result;
    DWORD BytesToWrite;

    PVOID LocalBuffer;
    BYTE StaticBuffer[1024];

    /* Check for write protection */
    if (DiskImage->ReadOnly)
    {
        /* Return error */
        return 0x03;
    }

    /* Write the sectors */
    Result = SeekDisk(DiskImage, Cylinder, Head, Sector);
    if (Result != 0x00)
        return Result;

    BytesToWrite = (DWORD)NumSectors * DiskImage->DiskInfo.SectorSize;

    // FIXME: Consider just looping around filling each time the buffer...

    if (BytesToWrite <= sizeof(StaticBuffer))
    {
        LocalBuffer = StaticBuffer;
    }
    else
    {
        LocalBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, BytesToWrite);
        ASSERT(LocalBuffer != NULL);
    }

    /* Read from the memory */
    EmulatorReadMemory(&EmulatorContext,
                       TO_LINEAR(getES(), getBX()),
                       LocalBuffer,
                       BytesToWrite);

    if (WriteFile(DiskImage->hDisk, LocalBuffer, BytesToWrite, &BytesToWrite, NULL))
        Result = 0x00;
    else
        Result = 0x04;

    if (LocalBuffer != StaticBuffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalBuffer);

    /* Return success or error */
    return Result;
}

typedef BOOLEAN (*MOUNT_DISK_HANDLER)(IN PDISK_IMAGE DiskImage, IN HANDLE hFile);

typedef struct _DISK_MOUNT_INFO
{
    PDISK_IMAGE DiskArray;
    ULONG NumDisks;
    MOUNT_DISK_HANDLER MountDiskHelper;
} DISK_MOUNT_INFO, *PDISK_MOUNT_INFO;

static DISK_MOUNT_INFO DiskMountInfo[MAX_DISK_TYPE] =
{
    {XDCFloppyDrive, _ARRAYSIZE(XDCFloppyDrive), MountFDI},
    {XDCHardDrive  , _ARRAYSIZE(XDCHardDrive)  , MountHDD},
};

PDISK_IMAGE
RetrieveDisk(IN DISK_TYPE DiskType,
             IN ULONG DiskNumber)
{
    ASSERT(DiskType < MAX_DISK_TYPE);

    if (DiskNumber >= DiskMountInfo[DiskType].NumDisks)
    {
        DisplayMessage(L"RetrieveDisk: Disk number %d:%d invalid.", DiskType, DiskNumber);
        return NULL;
    }

    return &DiskMountInfo[DiskType].DiskArray[DiskNumber];
}

BOOLEAN
MountDisk(IN DISK_TYPE DiskType,
          IN ULONG DiskNumber,
          IN PCWSTR FileName,
          IN BOOLEAN ReadOnly)
{
    BOOLEAN Success = FALSE;
    PDISK_IMAGE DiskImage;
    HANDLE hFile;

    BY_HANDLE_FILE_INFORMATION FileInformation;

    ASSERT(DiskType < MAX_DISK_TYPE);

    if (DiskNumber >= DiskMountInfo[DiskType].NumDisks)
    {
        DisplayMessage(L"MountDisk: Disk number %d:%d invalid.", DiskType, DiskNumber);
        return FALSE;
    }

    DiskImage = &DiskMountInfo[DiskType].DiskArray[DiskNumber];
    if (IsDiskPresent(DiskImage))
    {
        DPRINT1("MountDisk: Disk %d:%d:0x%p already in use, recycling...\n", DiskType, DiskNumber, DiskImage);
        UnmountDisk(DiskType, DiskNumber);
    }

    /* Try to open the file */
    SetLastError(0); // For debugging purposes
    hFile = CreateFileW(FileName,
                        GENERIC_READ | (ReadOnly ? 0 : GENERIC_WRITE),
                        (ReadOnly ? FILE_SHARE_READ : 0),
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    DPRINT1("File '%S' opening %s ; GetLastError() = %u\n",
            FileName, hFile != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

    /* If we failed, bail out */
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DisplayMessage(L"MountDisk: Error when opening disk file '%s' (Error: %u).", FileName, GetLastError());
        return FALSE;
    }

    /* OK, we have a handle to the file */

    /*
     * Check that it is really a file, and not a physical drive.
     * For obvious security reasons, we do not want to be able to
     * write directly to physical drives.
     *
     * Redundant checks
     */
    SetLastError(0);
    if (!GetFileInformationByHandle(hFile, &FileInformation) &&
        GetLastError() == ERROR_INVALID_FUNCTION)
    {
        /* Objects other than real files are not supported */
        DisplayMessage(L"MountDisk: '%s' is not a valid disk file.", FileName);
        goto Quit;
    }
    SetLastError(0);
    if (GetFileSize(hFile, NULL) == INVALID_FILE_SIZE &&
        GetLastError() == ERROR_INVALID_FUNCTION)
    {
        /* Objects other than real files are not supported */
        DisplayMessage(L"MountDisk: '%s' is not a valid disk file.", FileName);
        goto Quit;
    }

    /* Success, mount the image */
    if (!DiskMountInfo[DiskType].MountDiskHelper(DiskImage, hFile))
    {
        DisplayMessage(L"MountDisk: Failed to mount disk file '%s' in 0x%p.", FileName, DiskImage);
        goto Quit;
    }

    /* Update its read/write state */
    DiskImage->ReadOnly = ReadOnly;

    Success = TRUE;

Quit:
    if (!Success) FileClose(hFile);
    return Success;
}

BOOLEAN
UnmountDisk(IN DISK_TYPE DiskType,
            IN ULONG DiskNumber)
{
    PDISK_IMAGE DiskImage;

    ASSERT(DiskType < MAX_DISK_TYPE);

    if (DiskNumber >= DiskMountInfo[DiskType].NumDisks)
    {
        DisplayMessage(L"UnmountDisk: Disk number %d:%d invalid.", DiskType, DiskNumber);
        return FALSE;
    }

    DiskImage = &DiskMountInfo[DiskType].DiskArray[DiskNumber];
    if (!IsDiskPresent(DiskImage))
    {
        DPRINT1("UnmountDisk: Disk %d:%d:0x%p is already unmounted\n", DiskType, DiskNumber, DiskImage);
        return FALSE;
    }

    /* Flush the image and unmount it */
    FlushFileBuffers(DiskImage->hDisk);
    FileClose(DiskImage->hDisk);
    DiskImage->hDisk = NULL;
    return TRUE;
}


/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN DiskCtrlInitialize(VOID)
{
    return TRUE;
}

VOID DiskCtrlCleanup(VOID)
{
    ULONG DiskNumber;

    /* Unmount all the present floppy disk drives */
    for (DiskNumber = 0; DiskNumber < DiskMountInfo[FLOPPY_DISK].NumDisks; ++DiskNumber)
    {
        if (IsDiskPresent(&DiskMountInfo[FLOPPY_DISK].DiskArray[DiskNumber]))
            UnmountDisk(FLOPPY_DISK, DiskNumber);
    }

    /* Unmount all the present hard disk drives */
    for (DiskNumber = 0; DiskNumber < DiskMountInfo[HARD_DISK].NumDisks; ++DiskNumber)
    {
        if (IsDiskPresent(&DiskMountInfo[HARD_DISK].DiskArray[DiskNumber]))
            UnmountDisk(HARD_DISK, DiskNumber);
    }
}

/* EOF */
