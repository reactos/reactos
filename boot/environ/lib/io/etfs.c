/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/etfs.c
 * PURPOSE:         Boot Library El Torito File System Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <bl.h>
#include <cdfs/cd.h>
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

typedef struct _BL_ETFS_DEVICE
{
    ULONG RootDirOffset;
    ULONG RootDirSize;
    ULONG BlockSize;
    ULONG VolumeSize;
    BOOLEAN IsIso;
    PUCHAR MemoryBlock;
    ULONG Offset;
} BL_ETFS_DEVICE, *PBL_ETFS_DEVICE;

typedef struct _BL_ETFS_FILE
{
    ULONG DiskOffset;
    ULONG DirOffset;
    ULONG DirEntOffset;

    BL_FILE_INFORMATION;

    ULONG DeviceId;
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

NTSTATUS
EtfsGetInformation (
    _In_ PBL_FILE_ENTRY FileEntry,
    _Out_ PBL_FILE_INFORMATION FileInfo
    );

NTSTATUS
EtfsSetInformation (
    _In_ PBL_FILE_ENTRY FileEntry,
    _In_ PBL_FILE_INFORMATION FileInfo
    );

NTSTATUS
EtfsRead (
    _In_ PBL_FILE_ENTRY FileEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_opt_ PULONG BytesReturned
    );

BL_FILE_CALLBACKS EtfsFunctionTable =
{
    EtfsOpen,
    NULL,
    EtfsRead,
    NULL,
    NULL,
    EtfsGetInformation,
    EtfsSetInformation
};

/* FUNCTIONS *****************************************************************/

VOID
EtfspGetDirectoryInfo (
    _In_ PBL_ETFS_DEVICE EtfsDevice,
    _In_ PRAW_DIR_REC DirEntry,
    _Out_ PULONG FileOffset,
    _Out_ PULONG FileSize,
    _Out_opt_ PBOOLEAN IsDirectory
    )
{
    ULONG SectorOffset;
    BOOLEAN IsDir;

    *FileOffset = *(PULONG)DirEntry->FileLoc * EtfsDevice->BlockSize;
    *FileOffset += (DirEntry->XarLen * EtfsDevice->BlockSize);

    SectorOffset = ALIGN_DOWN_BY(*FileOffset, CD_SECTOR_SIZE);

    *FileSize = *(PULONG)DirEntry->DataLen;

    IsDir = DE_FILE_FLAGS(EtfsDevice->IsIso, DirEntry) & ISO_ATTR_DIRECTORY;
    if (IsDir)
    {
        *FileSize += ALIGN_UP_BY(SectorOffset, CD_SECTOR_SIZE) - SectorOffset;
    }

    if (IsDirectory)
    {
        *IsDirectory = IsDir;
    }
}

USHORT
EtfspGetDirentNameLength (
    _In_ PRAW_DIR_REC DirEntry
    )
{
    USHORT Length, RealLength;
    PUCHAR Pos;

    RealLength = Length = DirEntry->FileIdLen;
    for (Pos = DirEntry->FileId + Length - 1; Length; --Pos)
    {
        --Length;

        if (*Pos == ';')
        {
            RealLength = Length;
            break;
        }
    }

    Length = RealLength;
    for (Pos = DirEntry->FileId + Length - 1; Length; --Pos)
    {
        --Length;

        if (*Pos != '.')
        {
            break;
        }

        RealLength = Length;
    }

    return RealLength;
}

LONG
EtfspCompareNames (
    __in PSTRING Name1,
    __in PUNICODE_STRING Name2
    )
{
    ULONG i, l1, l2, l;

    l1 = Name1->Length;
    l2 = Name2->Length / sizeof(WCHAR);
    l = min(l1, l2);

    for (i = 0; i < l; i++)
    {
        if (toupper(Name1->Buffer[i]) != toupper(Name2->Buffer[i]))
        {
            return toupper(Name1->Buffer[i]) - toupper(Name2->Buffer[i]);
        }
    }

    if (l2 <= l1)
    {
        return l2 < l1;
    }
    else
    {
        return -1;
    }
}

BOOLEAN
EtfspFileMatch (
    _In_ PRAW_DIR_REC DirEntry,
    _In_ PUNICODE_STRING FileName
    )
{
    BOOLEAN Match;
    USHORT Length;
    ANSI_STRING DirName;

    if ((DirEntry->FileIdLen != 1) ||
        ((DirEntry->FileId[0] != 0) && (DirEntry->FileId[0] != 1)))
    {
        Length = EtfspGetDirentNameLength(DirEntry);
        DirName.Length = Length;
        DirName.MaximumLength = Length;
        DirName.Buffer = (PCHAR)DirEntry->FileId;

        Match = EtfspCompareNames(&DirName, FileName);
    }
    else
    {
        Match = -1;
    }
    return Match;
}

NTSTATUS
EtfspGetDirent (
    _In_ PBL_FILE_ENTRY DirectoryEntry,
    _Out_ PRAW_DIR_REC *DirEntry,
    _Inout_ PULONG DirentOffset
    )
{
    PBL_ETFS_FILE EtfsFile;
    ULONG FileOffset, DirectoryOffset, AlignedOffset, RemainderOffset;
    ULONG DeviceId, ReadSize, DirLen;
    PBL_ETFS_DEVICE EtfsDevice;
    BOOLEAN NeedRead, IsMulti;
    NTSTATUS result;
    PRAW_DIR_REC DirEnt;
    PUCHAR MemoryBlock;

    EtfsFile = DirectoryEntry->FsSpecificData;
    DeviceId = EtfsFile->DeviceId;
    FileOffset = EtfsFile->DiskOffset;
    EtfsDevice = EtfsDeviceTable[DeviceId];

    DirectoryOffset = *DirentOffset;
    MemoryBlock = EtfsDevice->MemoryBlock;

    IsMulti = 0;

    AlignedOffset = (FileOffset + *DirentOffset) & ~CD_SECTOR_SIZE;
    RemainderOffset = *DirentOffset + FileOffset - AlignedOffset;

    ReadSize = 2048 - RemainderOffset;
    NeedRead = AlignedOffset == EtfsDevice->Offset ? 0 : 1;

ReadAgain:
    if (DirectoryOffset >= EtfsFile->Size)
    {
        return STATUS_NO_SUCH_FILE;
    }

    while (ReadSize < MIN_DIR_REC_SIZE)
    {
        DirectoryOffset += ReadSize;
        AlignedOffset += 2048;
        ReadSize = 2048;
        RemainderOffset = 0;
        NeedRead = 1;
        if (DirectoryOffset >= EtfsFile->Size)
        {
            return STATUS_NO_SUCH_FILE;
        }
    }

    if (NeedRead)
    {
        result = BlDeviceReadAtOffset(DirectoryEntry->DeviceId,
                                      CD_SECTOR_SIZE,
                                      AlignedOffset,
                                      MemoryBlock,
                                      NULL);
        if (!NT_SUCCESS(result))
        {
            EfiPrintf(L"Device read failed %lx\r\n", result);
            return result;
        }

        NeedRead = FALSE;
        EtfsDevice->Offset = AlignedOffset;
    }

    if (!*(MemoryBlock + RemainderOffset))
    {
        AlignedOffset += 2048;
        NeedRead = TRUE;

        RemainderOffset = 0;
        DirectoryOffset += ReadSize;
        ReadSize = 2048;
        goto ReadAgain;
    }

    DirEnt = (PRAW_DIR_REC)(MemoryBlock + RemainderOffset);
    DirLen = DirEnt->DirLen;
    if (DirLen > ReadSize)
    {
        EfiPrintf(L"Dir won't fit %lx %lx\r\n", DirLen, ReadSize);
        return STATUS_NO_SUCH_FILE;
    }

    if (IsMulti)
    {
        if (!(DE_FILE_FLAGS(EtfsDevice->IsIso, DirEnt) & ISO_ATTR_MULTI))
        {
            IsMulti = TRUE;
        }
    }
    else if (DE_FILE_FLAGS(EtfsDevice->IsIso, DirEnt) & ISO_ATTR_MULTI)
    {
        IsMulti = TRUE;
    }
    else
    {
        if ((DirEnt->FileIdLen != 1) ||
            ((DirEnt->FileId[0] != 0) && (DirEnt->FileId[0] != 1)))
        {
            goto Quickie;
        }
    }

    RemainderOffset += DirLen;
    DirectoryOffset += DirLen;
    ReadSize -= DirLen;
    goto ReadAgain;

Quickie:
    *DirEntry = DirEnt;
    *DirentOffset = DirectoryOffset;
    return STATUS_SUCCESS;
}

NTSTATUS
EtfspSearchForDirent (
    _In_ PBL_FILE_ENTRY DirectoryEntry,
    _In_ PWCHAR FileName,
    _Out_ PRAW_DIR_REC *DirEntry,
    _Out_ PULONG DirentOffset
    )
{
    UNICODE_STRING Name;
    ULONG NextOffset;
    PRAW_DIR_REC DirEnt;
    NTSTATUS Status;

    RtlInitUnicodeString(&Name, FileName);
    for (NextOffset = *DirentOffset;
         ;
         NextOffset = NextOffset + DirEnt->DirLen)
    {
        Status = EtfspGetDirent(DirectoryEntry, &DirEnt, &NextOffset);
        if (!NT_SUCCESS(Status))
        {
            return STATUS_NO_SUCH_FILE;
        }

        if (!EtfspFileMatch(DirEnt, &Name))
        {
            break;
        }
    }

    *DirEntry = DirEnt;
    *DirentOffset = NextOffset;
    return 0;
}

NTSTATUS
EtfspCachedSearchForDirent (
    _In_ PBL_FILE_ENTRY DirectoryEntry,
    _In_ PWCHAR FileName,
    _Out_ PRAW_DIR_REC *DirEntry,
    _Out_ PULONG DirOffset,
    _In_ BOOLEAN KeepOffset
    )
{
    PBL_ETFS_FILE EtfsFile;
    PBL_ETFS_DEVICE EtfsDevice;
    NTSTATUS Status;
    ULONG DirentOffset;
    PRAW_DIR_REC Dirent;
    UNICODE_STRING Name;

    EtfsFile = DirectoryEntry->FsSpecificData;
    EtfsDevice = EtfsDeviceTable[EtfsFile->DeviceId];
    RtlInitUnicodeString(&Name, FileName);
    DirentOffset = EtfsFile->DirEntOffset;

    if ((KeepOffset) ||
        (ALIGN_DOWN_BY((DirentOffset + EtfsFile->DiskOffset), CD_SECTOR_SIZE) ==
         EtfsDevice->Offset))
    {
        Status = EtfspGetDirent(DirectoryEntry, &Dirent, &DirentOffset);
        if (NT_SUCCESS(Status))
        {
            if (!EtfspFileMatch(Dirent, &Name))
            {
                *DirEntry = Dirent;
                *DirOffset = DirentOffset;
                return STATUS_SUCCESS;
            }
        }
        else
        {
            DirentOffset = 0;
        }
    }
    else
    {
        DirentOffset = 0;
    }

    Status = EtfspSearchForDirent(DirectoryEntry,
                                  FileName,
                                  DirEntry,
                                  &DirentOffset);
    if (!(NT_SUCCESS(Status)) && (DirentOffset))
    {
        DirentOffset = 0;
        Status = EtfspSearchForDirent(DirectoryEntry,
                                      FileName,
                                      DirEntry,
                                      &DirentOffset);
    }

    if (NT_SUCCESS(Status))
    {
        *DirOffset = DirentOffset;
    }

    return Status;
}

NTSTATUS
EtfsRead (
    _In_ PBL_FILE_ENTRY FileEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_opt_ PULONG BytesReturned
    )
{
    ULONG BytesRead;
    PBL_ETFS_FILE EtfsFile;
    NTSTATUS Status;

    /* Assume failure for now */
    BytesRead = 0;

    /* Make sure that the read is within the file's boundaries */
    EtfsFile = FileEntry->FsSpecificData;
    if ((Size + EtfsFile->Offset) > EtfsFile->Size)
    {
        /* Bail out otherwise */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Read the offset that matches this file's offset, on the disk */
        Status = BlDeviceReadAtOffset(FileEntry->DeviceId,
                                      Size,
                                      EtfsFile->Offset + EtfsFile->DiskOffset,
                                      Buffer,
                                      &BytesRead);
        if (NT_SUCCESS(Status))
        {
            /* Update the file offset and return the size as having been read */
            EtfsFile->Offset += Size;
            BytesRead = Size;
        }
    }

    /* Check if caller wanted to know how many bytes were read */
    if (BytesReturned)
    {
        /* Return the value */
        *BytesReturned = BytesRead;
    }

    /* All done */
    return Status;
}

NTSTATUS
EtfsSetInformation (
    _In_ PBL_FILE_ENTRY FileEntry,
    _In_ PBL_FILE_INFORMATION FileInfo
    )
{
    PBL_ETFS_FILE EtfsFile;
    BL_FILE_INFORMATION LocalFileInfo;

    /* Get the underlying ETFS file data structure */
    EtfsFile = (PBL_ETFS_FILE)FileEntry->FsSpecificData;

    /* Make a copy of the incoming attributes, but ignore the new offset */
    LocalFileInfo = *FileInfo;
    LocalFileInfo.Offset = EtfsFile->Offset;

    /* Check if these match exactly the current file */
    if (!RtlEqualMemory(&LocalFileInfo, &EtfsFile->Size, sizeof(*FileInfo)))
    {
        /* Nope -- which means caller is trying to change an immutable */
        EfiPrintf(L"Incorrect information change\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Is the offset past the end of the file? */
    if (FileInfo->Offset >= EtfsFile->Size)
    {
        /* Don't allow EOF */
        EfiPrintf(L"Offset too large: %lx vs %lx\r\n", FileInfo->Offset, EtfsFile->Size);
        return STATUS_INVALID_PARAMETER;
    }

    /* Update the offset */
    EtfsFile->Offset = FileInfo->Offset;
    return STATUS_SUCCESS;
}

NTSTATUS
EtfsGetInformation (
    _In_ PBL_FILE_ENTRY FileEntry,
    _Out_ PBL_FILE_INFORMATION FileInfo
    )
{
    PBL_ETFS_FILE EtfsFile;

    /* Get the underlying ETFS file data structure */
    EtfsFile = (PBL_ETFS_FILE)FileEntry->FsSpecificData;

    /* Copy the cached information structure within it */
    RtlCopyMemory(FileInfo, &EtfsFile->Size, sizeof(*FileInfo));
    return STATUS_SUCCESS;
}

NTSTATUS
EtfsOpen (
    _In_ PBL_FILE_ENTRY Directory,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PBL_FILE_ENTRY *FileEntry
    )
{
    PBL_ETFS_DEVICE EtfsDevice;
    NTSTATUS Status;
    PBL_FILE_ENTRY NewFile;
    PWCHAR FilePath, FormatString;
    PBL_ETFS_FILE EtfsFile;
    ULONG DeviceId, FileSize, DirOffset, FileOffset;
    SIZE_T Size;
    PRAW_DIR_REC DirEntry;
    BOOLEAN IsDirectory;

    EtfsFile = Directory->FsSpecificData;
    DeviceId = EtfsFile->DeviceId;
    EtfsDevice = EtfsDeviceTable[DeviceId];

    /* Find the given file (or directory) in the given directory */
    Status = EtfspCachedSearchForDirent(Directory,
                                        FileName,
                                        &DirEntry,
                                        &DirOffset,
                                        FALSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Find out information about the file (or directory) we found */
    EtfspGetDirectoryInfo(EtfsDevice,
                          DirEntry,
                          &FileOffset,
                          &FileSize,
                          &IsDirectory);

    /* Allocate a file entry */
    NewFile = BlMmAllocateHeap(sizeof(*NewFile));
    if (!NewFile)
    {
        return STATUS_NO_MEMORY;
    }

    /* Zero it out */
    RtlZeroMemory(NewFile, sizeof(*NewFile));

    /* Figure out the size of the path and filename plus a slash and NUL */
    Size = wcslen(Directory->FilePath) + wcslen(FileName) + 2;
    FilePath = BlMmAllocateHeap(Size * sizeof(WCHAR));
    if (!FilePath)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Allocate an ETFS file entry */
    EtfsFile = (PBL_ETFS_FILE)BlMmAllocateHeap(sizeof(*EtfsFile));
    if (!EtfsFile)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(EtfsFile, sizeof(*EtfsFile));

    /* Capture the device ID of the directory */
    NewFile->DeviceId = Directory->DeviceId;

    /* Check if this is the root or a filename\directory under */
    FormatString = L"%ls%ls";
    if (Directory->FilePath[1])
    {
        FormatString = L"%ls\\%ls";
    }

    /* Combine the paths, and save the final path in the file entry */
    _snwprintf(FilePath, Size, FormatString, Directory->FilePath, FileName);
    NewFile->FilePath = FilePath;

    /* Copy the ETFS function callbacks into the file netry */
    RtlCopyMemory(&NewFile->Callbacks,
                  &EtfsFunctionTable,
                  sizeof(NewFile->Callbacks));

    /* Fill out the rest of the details */
    EtfsFile->DiskOffset = FileOffset;
    EtfsFile->DirOffset = DirOffset;
    EtfsFile->Size = FileSize;
    EtfsFile->DeviceId = DeviceId;

    /* Check if this is a directory */
    if (IsDirectory)
    {
        EtfsFile->Flags |= BL_ETFS_FILE_ENTRY_DIRECTORY;
        NewFile->Flags |= BL_FILE_ENTRY_DIRECTORY;
    }

    /* Write down the name of the filesystem */
    EtfsFile->FsName = L"cdfs";

    /* All done, return the file entry, and save the ETFS side */
    NewFile->FsSpecificData = EtfsFile;
    *FileEntry = NewFile;
    return Status;

Quickie:
    /* Failure path -- free the file path if we had one */
    if (NewFile->FilePath)
    {
        BlMmFreeHeap(NewFile->FilePath);
    }

    /* Free the ETFS file entry if we had one */
    if (NewFile->FsSpecificData)
    {
        BlMmFreeHeap(NewFile->FsSpecificData);
    }

    /* Free the file entry itself, and return the error code */
    BlMmFreeHeap(NewFile);
    return Status;
}

NTSTATUS
EtfspCheckCdfs (
    _In_ PBL_ETFS_DEVICE EtfsDevice,
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
    _In_ PBL_ETFS_DEVICE EtfsDevice,
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
    IsoVd = (PRAW_ISO_VD)EtfsDevice->MemoryBlock;
    EtVd = (PRAW_ET_VD)IsoVd;

    /* First, read the El Torito Volume Descriptor */
    BlDeviceGetInformation(DeviceId, &DeviceInformation);
    Unknown = DeviceInformation.BlockDeviceInfo.Unknown;
    DeviceInformation.BlockDeviceInfo.Unknown |= 1;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    Status = BlDeviceReadAtOffset(DeviceId,
                                  CD_SECTOR_SIZE,
                                  (FIRST_VD_SECTOR + 1) * CD_SECTOR_SIZE,
                                  EtfsDevice->MemoryBlock,
                                  &BytesRead);
    DeviceInformation.BlockDeviceInfo.Unknown = Unknown;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L" read failed\r\n");
        return Status;
    }

    /* Remember that's where we last read */
    EtfsDevice->Offset = (FIRST_VD_SECTOR + 1) * CD_SECTOR_SIZE;

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
                                  EtfsDevice->MemoryBlock,
                                  &BytesRead);
    DeviceInformation.BlockDeviceInfo.Unknown = Unknown;
    BlDeviceSetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Remember where we left off */
    EtfsDevice->Offset = FIRST_VD_SECTOR  * CD_SECTOR_SIZE;

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
    return STATUS_SUCCESS;
}

NTSTATUS
EtfspDeviceContextDestroy (
    _In_ PBL_ETFS_DEVICE EtfsDevice
    )
{
    if (EtfsDevice->MemoryBlock)
    {
        BlMmFreeHeap(EtfsDevice->MemoryBlock);
    }

    BlMmFreeHeap(EtfsDevice);

    return STATUS_SUCCESS;
}

NTSTATUS
EtfspCreateContext (
    _In_ ULONG DeviceId,
    _Out_ PBL_ETFS_DEVICE *EtfsDevice
    )
{
    PBL_ETFS_DEVICE NewContext;
    PVOID MemoryBlock;
    NTSTATUS Status;
    BOOLEAN IsIso;
    PRAW_ISO_VD RawVd;

    NewContext = (PBL_ETFS_DEVICE)BlMmAllocateHeap(sizeof(*NewContext));
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
    Status = STATUS_SUCCESS;

Quickie:
    if (!NT_SUCCESS(Status))
    {
        EtfspDeviceContextDestroy(NewContext);
        NewContext = NULL;
    }

    *EtfsDevice = NewContext;
    return Status;
}

NTSTATUS
EtfspDeviceTableDestroyEntry (
    _In_ PBL_ETFS_DEVICE EtfsDevice,
    _In_ ULONG Index
    )
{
    EtfspDeviceContextDestroy(EtfsDevice);
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
    PBL_ETFS_DEVICE EtfsDevice = NULL;
    PBL_FILE_ENTRY RootEntry;
    NTSTATUS Status;
    PBL_ETFS_FILE EtfsFile;

    EfiPrintf(L"Trying to mount as ETFS...\r\n");

    Status = EtfspCreateContext(DeviceId, &EtfsDevice);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"ETFS context failed: %lx\r\n");
        return Status;
    }

    Status = BlTblSetEntry(&EtfsDeviceTable,
                           &EtfsDeviceTableEntries,
                           EtfsDevice,
                           &DeviceId,
                           TblDoNotPurgeEntry);
    if (!NT_SUCCESS(Status))
    {
        EtfspDeviceContextDestroy(EtfsDevice);
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
    EtfsFile->DiskOffset = EtfsDevice->RootDirOffset;
    EtfsFile->DirOffset = 0;
    EtfsFile->Size = EtfsDevice->RootDirSize;
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

    EtfspDeviceTableDestroyEntry(EtfsDevice, DeviceId);

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

