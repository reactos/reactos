/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/etfs.c
 * PURPOSE:         Boot Library El Torito File System Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include "../drivers/filesystems/cdfs_new/cd.h"
typedef struct _RAW_ET_VD
{
    UCHAR BootIndicator;
    UCHAR StandardId[5];
    UCHAR Version;
    UCHAR SystemId[32];
    UCHAR Reserved[32];
    ULONG BootCatalogOffset;
    UCHAR Padding[1973];
} RAW_ET_VD, *PRAW_ET_VD;

/* DATA VARIABLES ************************************************************/

typedef struct _BL_ETFS_CONTEXT
{
    ULONG RootDirOffset;
    ULONG RootDirSize;
    ULONG BlockSize;
    ULONG VolumeSize;
    BOOLEAN IsIso;
    PRAW_ISO_VD MemoryBlock;
    ULONG Offset;
} BL_ETFS_CONTEXT, *PBL_ETFS_CONTEXT;

typedef struct _BL_ETFS_FILE
{
    ULONG Flags;
    ULONG DeviceId;
    ULONG Offset;
    ULONG Unknown;
    ULONGLONG Size;
    PWCHAR FsName;
} BL_ETFS_FILE, *PBL_ETFS_FILE;

ULONG EtfsDeviceTableEntries;
PVOID* EtfsDeviceTable;

NTSTATUS
EtfsOpen (
    _In_ PBL_FILE_ENTRY Directory,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PBL_FILE_ENTRY *FileEntry
    );

BL_FILE_CALLBACKS EtfsFunctionTable =
{
    EtfsOpen,
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
EtfsOpen (
    _In_ PBL_FILE_ENTRY Directory,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags, 
    _Out_ PBL_FILE_ENTRY *FileEntry
    )
{
    EfiPrintf(L"Attempting to open file %s in directory %s. Not yet supported\r\n", FileName, Directory->FilePath);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
EtfspCheckCdfs (
    _In_ PBL_ETFS_CONTEXT EtfsContext,
    _In_ ULONG DeviceId,
    _Out_ PRAW_ISO_VD *VolumeDescriptor,
    _Out_ PBOOLEAN VolumeIsIso
    )
{
    EfiPrintf(L"Raw Cdfs not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
EtfspCheckEtfs (
    _In_ PBL_ETFS_CONTEXT EtfsContext,
    _In_ ULONG DeviceId,
    _Out_ PRAW_ISO_VD *VolumeDescriptor,
    _Out_ PBOOLEAN VolumeIsIso
    )
{
    PRAW_ISO_VD IsoVd;
    PRAW_ET_VD EtVd;
    NTSTATUS Status;
    BOOLEAN IsIso;
    BL_DEVICE_INFORMATION DeviceInformation;
    ULONG Unknown, BytesRead;
    ANSI_STRING CompareString, String;

    /* Save our static buffer pointer */
    IsoVd = EtfsContext->MemoryBlock;
    EtVd = (PRAW_ET_VD)IsoVd;

    /* First, read the El Torito Volume Descriptor */
    BlDeviceGetInformation(DeviceId, &DeviceInformation);
    Unknown = DeviceInformation.BlockDeviceInfo.Unknown;
    DeviceInformation.BlockDeviceInfo.Unknown |= 1;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    Status = BlDeviceReadAtOffset(DeviceId,
                                  CD_SECTOR_SIZE,
                                  (FIRST_VD_SECTOR + 1) * CD_SECTOR_SIZE,
                                  EtfsContext->MemoryBlock,
                                  &BytesRead);
    DeviceInformation.BlockDeviceInfo.Unknown = Unknown;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L" read failed\r\n");
        return Status;
    }

    /* Remember that's where we last read */
    EtfsContext->Offset = (FIRST_VD_SECTOR + 1) * CD_SECTOR_SIZE;

    /* Check if it's EL TORITO! */
    RtlInitString(&String, "EL TORITO SPECIFICATION");
    CompareString.Buffer = (PCHAR)EtVd->SystemId;
    CompareString.Length = 23;
    CompareString.MaximumLength = 23;
    if (!RtlEqualString(&CompareString, &String, TRUE))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Check the version and boot indicator */
    if ((EtVd->Version != 1) || (EtVd->BootIndicator))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Check if it has the CD0001 identifier */
    RtlInitString(&String, ISO_VOL_ID);
    CompareString.Buffer = (PCHAR)EtVd->StandardId;
    CompareString.Length = 5;
    CompareString.MaximumLength = 5;
    if (!RtlEqualString(&CompareString, &String, TRUE))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Step two, we now want to read the ISO Volume Descriptor */
    DeviceInformation.BlockDeviceInfo.Unknown |= 1u;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    Status = BlDeviceReadAtOffset(DeviceId,
                                  CD_SECTOR_SIZE,
                                  FIRST_VD_SECTOR * CD_SECTOR_SIZE,
                                  EtfsContext->MemoryBlock,
                                  &BytesRead);
    DeviceInformation.BlockDeviceInfo.Unknown = Unknown;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Remember where we left off */
    EtfsContext->Offset = FIRST_VD_SECTOR  * CD_SECTOR_SIZE;

    /* This should also say CD0001 */
    CompareString.Buffer = (PCHAR)IsoVd->StandardId;
    CompareString.Length = 5;
    CompareString.MaximumLength = 5;
    IsIso = RtlEqualString(&CompareString, &String, TRUE);
    if (!IsIso)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* And should be a version we support */
    if ((IsoVd->Version != VERSION_1) || (IsoVd->DescType != VD_PRIMARY))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Return back to the caller */
    *VolumeDescriptor = IsoVd;
    *VolumeIsIso = IsIso;
    EfiPrintf(L"Recognized!!!\r\n");
    return STATUS_SUCCESS;
}

VOID
EtfspGetDirectoryInfo (
    _In_ PBL_ETFS_CONTEXT EtfsContext,
    _In_ PRAW_DIR_REC DirEntry,
    _Out_ PULONG FileOffset,
    _Out_ PULONG FileSize,
    _Out_opt_ PBOOLEAN IsDirectory
    )
{
    ULONG SectorOffset;
    BOOLEAN IsDir;

    *FileOffset = *(PULONG)DirEntry->FileLoc * EtfsContext->BlockSize;
    *FileOffset += (DirEntry->XarLen * EtfsContext->BlockSize);

    SectorOffset = ALIGN_DOWN_BY(*FileOffset, CD_SECTOR_SIZE);

    *FileSize = *(PULONG)DirEntry->DataLen;

    IsDir = DE_FILE_FLAGS(EtfsContext->IsIso, DirEntry) & ISO_ATTR_DIRECTORY;
    if (IsDir)
    {
        *FileSize += ALIGN_UP_BY(SectorOffset, CD_SECTOR_SIZE) - SectorOffset;
    }

    if (IsDirectory)
    {
        *IsDirectory = IsDir;
    }
}

NTSTATUS
EtfspDeviceContextDestroy (
    _In_ PBL_ETFS_CONTEXT EtfsContext
    )
{
    if (EtfsContext->MemoryBlock)
    {
        BlMmFreeHeap(EtfsContext->MemoryBlock);
    }
    BlMmFreeHeap(EtfsContext);
    return 0;
}

NTSTATUS
EtfspCreateContext (
    _In_ ULONG DeviceId,
    _Out_ PBL_ETFS_CONTEXT *EtfsContext
    )
{
    PBL_ETFS_CONTEXT NewContext;
    PVOID MemoryBlock;
    NTSTATUS Status;
    BOOLEAN IsIso;
    PRAW_ISO_VD RawVd;

    NewContext = (PBL_ETFS_CONTEXT)BlMmAllocateHeap(sizeof(*NewContext));
    if (!NewContext)
    {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(NewContext, sizeof(*NewContext));

    MemoryBlock = BlMmAllocateHeap(CD_SECTOR_SIZE);
    NewContext->MemoryBlock = MemoryBlock;
    if (!MemoryBlock)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    Status = EtfspCheckEtfs(NewContext, DeviceId, &RawVd, &IsIso);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Drive not EDFS. Checking for CDFS: %lx\r\n");
        Status = EtfspCheckCdfs(NewContext, DeviceId, &RawVd, &IsIso);
    }

    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Drive not CDFS. Failing: %lx\r\n");
        goto Quickie;
    }

    NewContext->IsIso = IsIso;
    NewContext->BlockSize = RVD_LB_SIZE(RawVd, IsIso);
    NewContext->VolumeSize = RVD_VOL_SIZE(RawVd, IsIso);

    EtfspGetDirectoryInfo(NewContext,
                          (PRAW_DIR_REC)RVD_ROOT_DE(RawVd, IsIso),
                          &NewContext->RootDirOffset,
                          &NewContext->RootDirSize,
                          0);

Quickie:
    EtfspDeviceContextDestroy(NewContext);
    NewContext = NULL;

    *EtfsContext = NewContext;
    return Status;
}

NTSTATUS
EtfspDeviceTableDestroyEntry (
    _In_ PBL_ETFS_CONTEXT EtfsContext,
    _In_ ULONG Index
    )
{
    EtfspDeviceContextDestroy(EtfsContext);
    EtfsDeviceTable[Index] = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
EtfsMount (
    _In_ ULONG DeviceId,
    _In_ ULONG Unknown,
    _Out_ PBL_FILE_ENTRY* FileEntry
    )
{
    PBL_ETFS_CONTEXT EtfsContext = NULL;
    PBL_FILE_ENTRY RootEntry;
    NTSTATUS Status;
    PBL_ETFS_FILE EtfsFile;

    EfiPrintf(L"Trying to mount as ETFS...\r\n");

    Status = EtfspCreateContext(DeviceId, &EtfsContext);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"ETFS context failed: %lx\r\n");
        return Status;
    }

    Status = BlTblSetEntry(&EtfsDeviceTable,
                           &EtfsDeviceTableEntries,
                           EtfsContext,
                           &DeviceId,
                           TblDoNotPurgeEntry);
    if (!NT_SUCCESS(Status))
    {
        EtfspDeviceContextDestroy(EtfsContext);
        return Status;
    }

    RootEntry = BlMmAllocateHeap(sizeof(*RootEntry));
    if (!RootEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    RtlZeroMemory(RootEntry, sizeof(*RootEntry));

    RootEntry->FilePath = BlMmAllocateHeap(4);
    if (!RootEntry->FilePath)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    wcsncpy(RootEntry->FilePath, L"\\", 1);

    RootEntry->DeviceId = DeviceId;
    RtlCopyMemory(&RootEntry->Callbacks,
                  &EtfsFunctionTable,
                  sizeof(RootEntry->Callbacks));

    EtfsFile = (PBL_ETFS_FILE)BlMmAllocateHeap(sizeof(*EtfsFile));
    if (!EtfsFile)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    RootEntry->Flags |= 0x10000;

    RtlZeroMemory(EtfsFile, sizeof(*EtfsFile));
    RootEntry->FsSpecificData = EtfsFile;
    EtfsFile->DeviceId = DeviceId;
    EtfsFile->Flags |= 1;
    EtfsFile->Offset = EtfsContext->RootDirOffset;
    EtfsFile->Unknown = 0;
    EtfsFile->Size = EtfsContext->RootDirSize;
    EtfsFile->FsName = L"cdfs";
    *FileEntry = RootEntry;

    return STATUS_SUCCESS;

Quickie:
    if (RootEntry->FilePath)
    {
        BlMmFreeHeap(RootEntry->FilePath);
    }
    if (RootEntry->FsSpecificData)
    {
        BlMmFreeHeap(RootEntry->FsSpecificData);
    }
    if (RootEntry)
    {
        BlMmFreeHeap(RootEntry);
    }

    EtfspDeviceTableDestroyEntry(EtfsContext, DeviceId);

    return Status;
}

NTSTATUS
EtfsInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Allocate the device table with 2 entries*/
    EtfsDeviceTableEntries = 2;
    EtfsDeviceTable = BlMmAllocateHeap(sizeof(PBL_FILE_ENTRY) *
                                       EtfsDeviceTableEntries);
    if (EtfsDeviceTable)
    {
        /* Zero it out */
        RtlZeroMemory(EtfsDeviceTable,
                      sizeof(PBL_FILE_ENTRY) * EtfsDeviceTableEntries);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No memory, fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return back to caller */
    return Status;
}

