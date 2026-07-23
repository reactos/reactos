/*
 * PROJECT:     ReactOS exFAT filesystem driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Directory enumeration
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "exfat.h"

#define NDEBUG
#include <debug.h>

#define EXFAT_ALIGN_DIRECTORY_ENTRY(Size) ALIGN_UP_BY(Size, sizeof(LONGLONG))

static NTSTATUS
ExFatSetSearchPattern(
    PEXFAT_CCB Ccb,
    PUNICODE_STRING Pattern)
{
    static const WCHAR MatchAll[] = L"*";
    UNICODE_STRING Source;
    UNICODE_STRING Upcased;
    NTSTATUS Status;

    if (Pattern && Pattern->Length)
    {
        Source = *Pattern;
    }
    else
    {
        RtlInitUnicodeString(&Source, MatchAll);
    }

    /* FsRtlIsNameInExpression(IgnoreCase) requires a pre-upcased pattern. */
    Status = RtlUpcaseUnicodeString(&Upcased, &Source, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlFreeUnicodeString(&Ccb->SearchPattern);
    Ccb->SearchPattern = Upcased;
    return STATUS_SUCCESS;
}

static ULONG
ExFatDirectoryEntrySize(
    FILE_INFORMATION_CLASS InformationClass,
    ULONG NameLength)
{
    switch (InformationClass)
    {
        case FileDirectoryInformation:
            return FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName) + NameLength;
        case FileFullDirectoryInformation:
            return FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + NameLength;
        case FileBothDirectoryInformation:
            return FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName) + NameLength;
        case FileNamesInformation:
            return FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName) + NameLength;
        case FileIdBothDirectoryInformation:
            return FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName) + NameLength;
        case FileIdFullDirectoryInformation:
            return FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName) + NameLength;
        default:
            return 0;
    }
}

static VOID
ExFatFillDirectoryEntryValues(
    FILE_INFORMATION_CLASS InformationClass,
    PVOID Buffer,
    ULONG FileIndex,
    LARGE_INTEGER CreationTime,
    LARGE_INTEGER WriteTime,
    LARGE_INTEGER FileSize,
    LARGE_INTEGER AllocationSize,
    ULONG Attributes,
    ULONGLONG FileId,
    PUNICODE_STRING Name)
{
#define FILL_COMMON(Entry) \
    do { \
        (Entry)->NextEntryOffset = 0; \
        (Entry)->FileIndex = FileIndex; \
        (Entry)->CreationTime = CreationTime; \
        (Entry)->LastAccessTime = WriteTime; \
        (Entry)->LastWriteTime = WriteTime; \
        (Entry)->ChangeTime = WriteTime; \
        (Entry)->EndOfFile = FileSize; \
        (Entry)->AllocationSize = AllocationSize; \
        (Entry)->FileAttributes = Attributes; \
        (Entry)->FileNameLength = Name->Length; \
        RtlCopyMemory((Entry)->FileName, Name->Buffer, Name->Length); \
    } while (0)

    switch (InformationClass)
    {
        case FileDirectoryInformation:
            FILL_COMMON((PFILE_DIRECTORY_INFORMATION)Buffer);
            break;
        case FileFullDirectoryInformation:
            FILL_COMMON((PFILE_FULL_DIR_INFORMATION)Buffer);
            ((PFILE_FULL_DIR_INFORMATION)Buffer)->EaSize = 0;
            break;
        case FileBothDirectoryInformation:
            FILL_COMMON((PFILE_BOTH_DIR_INFORMATION)Buffer);
            ((PFILE_BOTH_DIR_INFORMATION)Buffer)->EaSize = 0;
            ((PFILE_BOTH_DIR_INFORMATION)Buffer)->ShortNameLength = 0;
            RtlZeroMemory(((PFILE_BOTH_DIR_INFORMATION)Buffer)->ShortName,
                          sizeof(((PFILE_BOTH_DIR_INFORMATION)Buffer)->ShortName));
            break;
        case FileNamesInformation:
        {
            PFILE_NAMES_INFORMATION Entry = Buffer;
            Entry->NextEntryOffset = 0;
            Entry->FileIndex = FileIndex;
            Entry->FileNameLength = Name->Length;
            RtlCopyMemory(Entry->FileName, Name->Buffer, Name->Length);
            break;
        }
        case FileIdBothDirectoryInformation:
            FILL_COMMON((PFILE_ID_BOTH_DIR_INFORMATION)Buffer);
            ((PFILE_ID_BOTH_DIR_INFORMATION)Buffer)->EaSize = 0;
            ((PFILE_ID_BOTH_DIR_INFORMATION)Buffer)->ShortNameLength = 0;
            RtlZeroMemory(((PFILE_ID_BOTH_DIR_INFORMATION)Buffer)->ShortName,
                          sizeof(((PFILE_ID_BOTH_DIR_INFORMATION)Buffer)->ShortName));
            ((PFILE_ID_BOTH_DIR_INFORMATION)Buffer)->FileId.QuadPart = FileId;
            break;
        case FileIdFullDirectoryInformation:
            FILL_COMMON((PFILE_ID_FULL_DIR_INFORMATION)Buffer);
            ((PFILE_ID_FULL_DIR_INFORMATION)Buffer)->EaSize = 0;
            ((PFILE_ID_FULL_DIR_INFORMATION)Buffer)->FileId.QuadPart = FileId;
            break;
        default:
            break;
    }

#undef FILL_COMMON
}

static VOID
ExFatFillDirectoryEntry(
    FILE_INFORMATION_CLASS InformationClass,
    PVOID Buffer,
    ULONG FileIndex,
    FILINFO* FatInformation,
    PUNICODE_STRING Name,
    ULONG ClusterSize)
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER WriteTime;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER AllocationSize;
    ULONG Attributes;
    ULONGLONG FileId = 0;

    CreationTime = ExFatFatTimeToSystemTime(FatInformation->crdate,
                                             FatInformation->crtime);
    WriteTime = ExFatFatTimeToSystemTime(FatInformation->fdate,
                                          FatInformation->ftime);
    FileSize.QuadPart = (FatInformation->fattrib & AM_DIR) ? 0 : FatInformation->fsize;
    AllocationSize.QuadPart = (FatInformation->fattrib & AM_DIR) ? 0 :
                              ExFatRoundUp(FatInformation->fsize, ClusterSize);
    Attributes = ExFatFatAttributesToNt(FatInformation->fattrib);
    if (InformationClass == FileIdBothDirectoryInformation ||
        InformationClass == FileIdFullDirectoryInformation)
    {
        /* Bare-name hash; Fcb->IndexNumber hashes the full path. */
        FileId = ExFatHashPath(Name);
    }

    ExFatFillDirectoryEntryValues(InformationClass,
                                  Buffer,
                                  FileIndex,
                                  CreationTime,
                                  WriteTime,
                                  FileSize,
                                  AllocationSize,
                                  Attributes,
                                  FileId,
                                  Name);
}

static NTSTATUS
ExFatQueryDirectory(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = Stack->FileObject;
    PEXFAT_VCB Vcb = DeviceObject->DeviceExtension;
    PEXFAT_FCB Fcb;
    PEXFAT_CCB Ccb;
    PEXFAT_DIR_INDEX DirIndex;
    PEXFAT_DIR_CHILD Child;
    FILE_INFORMATION_CLASS InformationClass;
    ULONG EntryBaseSize;
    ULONG BufferLength;
    PUCHAR Buffer;
    ULONG Offset = 0;
    PULONG PreviousNextOffset = NULL;
    FILINFO FatInformation;
    DIR SavedDirectory;
    ULONG SavedIndex;
    ULONG DotEntries;
    ULONG ChildPosition;
    UNICODE_STRING Name;
    ULONG Emitted;
    ULONG Required;
    ULONG AlignedRequired;
    FRESULT Result;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ReturnSingle;
    BOOLEAN IsRootDirectory;
    BOOLEAN MatchAll;
    BOOLEAN FreshScan = FALSE;
    BOOLEAN Stop = FALSE;
    BOOLEAN Found = FALSE;

    if (!FileObject)
        return STATUS_INVALID_PARAMETER;
    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;
    if (!Fcb || !Ccb || !Fcb->IsDirectory || !Ccb->HandleOpen)
        return STATUS_NOT_A_DIRECTORY;

    InformationClass = Stack->Parameters.QueryDirectory.FileInformationClass;
    EntryBaseSize = ExFatDirectoryEntrySize(InformationClass, 0);
    if (!EntryBaseSize)
        return STATUS_INVALID_INFO_CLASS;

    BufferLength = Stack->Parameters.QueryDirectory.Length;
    Status = ExFatLockUserBuffer(Irp, BufferLength, IoWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;
    Buffer = ExFatGetUserBuffer(Irp, FALSE);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    ExAcquireResourceSharedLite(&Vcb->Resource, TRUE);
    ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);

    if ((Stack->Flags & SL_RESTART_SCAN) || !Ccb->SearchPattern.Buffer)
    {
        Status = ExFatSetSearchPattern(Ccb,
                                       Stack->Parameters.QueryDirectory.FileName);
        if (!NT_SUCCESS(Status))
            goto Done;
        ExFatAcquireFatFs(Vcb);
        Result = f_rewinddir(&Ccb->Directory);
        ExFatReleaseFatFs(Vcb);
        if (Result != FR_OK)
        {
            Status = ExFatMapResult(Result);
            goto Done;
        }
        Ccb->DirectoryIndex = 0;
        Ccb->DirectoryIndexBacked = FALSE;
        FreshScan = TRUE;
    }

    ReturnSingle = !!(Stack->Flags & SL_RETURN_SINGLE_ENTRY);
    IsRootDirectory = (Fcb->PathName.Length == sizeof(WCHAR) &&
                       Fcb->PathName.Buffer[0] == L'\\');
    MatchAll = (Ccb->SearchPattern.Length == sizeof(WCHAR) &&
                Ccb->SearchPattern.Buffer[0] == L'*');
    DotEntries = IsRootDirectory ? 0 : 2;

    /*
     * exFAT stores no dot entries; synthesize "." and ".." like the Windows
     * driver so enumeration of an empty directory still returns something.
     */
    while (!Stop && !IsRootDirectory && Ccb->DirectoryIndex < 2)
    {
        UNICODE_STRING DotName;

        RtlInitUnicodeString(&DotName, Ccb->DirectoryIndex ? L".." : L".");
        if (!MatchAll &&
            !FsRtlIsNameInExpression(&Ccb->SearchPattern, &DotName, TRUE, NULL))
        {
            Ccb->DirectoryIndex++;
            continue;
        }

        Required = EntryBaseSize + DotName.Length;
        AlignedRequired = EXFAT_ALIGN_DIRECTORY_ENTRY(Required);
        if (Required > BufferLength - Offset)
        {
            Status = Found ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
            Stop = TRUE;
            break;
        }

        RtlZeroMemory(&FatInformation, sizeof(FatInformation));
        FatInformation.fattrib = AM_DIR;
        ExFatSystemTimeToFatTime(&Fcb->LastWriteTime,
                                 &FatInformation.fdate,
                                 &FatInformation.ftime);
        ExFatSystemTimeToFatTime(&Fcb->CreationTime,
                                 &FatInformation.crdate,
                                 &FatInformation.crtime);

        if (PreviousNextOffset)
            *PreviousNextOffset = Offset - ((PUCHAR)PreviousNextOffset - Buffer);
        PreviousNextOffset = (PULONG)(Buffer + Offset);
        Emitted = min(AlignedRequired, BufferLength - Offset);
        RtlZeroMemory(Buffer + Offset, Emitted);
        ExFatFillDirectoryEntry(InformationClass,
                                Buffer + Offset,
                                Ccb->DirectoryIndex,
                                &FatInformation,
                                &DotName,
                                Vcb->BytesPerCluster);
        Ccb->DirectoryIndex++;
        Found = TRUE;
        Offset += Emitted;
        if (ReturnSingle)
            Stop = TRUE;
    }

    DirIndex = ExFatLookupDirIndex(Vcb, &Fcb->PathName);
    if (!Stop && DirIndex && !DirIndex->Unindexable)
    {
        Ccb->DirectoryIndexBacked = TRUE;
        while (!Stop)
        {
            SavedIndex = Ccb->DirectoryIndex;
            ChildPosition = SavedIndex - DotEntries;
            if (ChildPosition >= DirIndex->Count)
                break;
            Child = &DirIndex->Children[ChildPosition];

            Name.Buffer = DirIndex->NamePool + Child->NameOffset;
            Name.Length = Child->NameLength;
            Name.MaximumLength = Child->NameLength;
            Ccb->DirectoryIndex++;

            if (!MatchAll &&
                !FsRtlIsNameInExpression(&Ccb->SearchPattern, &Name, TRUE, NULL))
                continue;

            Required = EntryBaseSize + Name.Length;
            AlignedRequired = EXFAT_ALIGN_DIRECTORY_ENTRY(Required);
            if (Required > BufferLength - Offset)
            {
                Ccb->DirectoryIndex = SavedIndex;
                Status = Found ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
                break;
            }

            if (PreviousNextOffset)
                *PreviousNextOffset = Offset - ((PUCHAR)PreviousNextOffset - Buffer);
            PreviousNextOffset = (PULONG)(Buffer + Offset);
            Emitted = min(AlignedRequired, BufferLength - Offset);
            RtlZeroMemory(Buffer + Offset, Emitted);
            ExFatFillDirectoryEntryValues(InformationClass,
                                          Buffer + Offset,
                                          Ccb->DirectoryIndex,
                                          Child->CreationTime,
                                          Child->LastWriteTime,
                                          Child->FileSize,
                                          Child->AllocationSize,
                                          Child->FileAttributes,
                                          Child->NameHash,
                                          &Name);
            Found = TRUE;
            Offset += Emitted;
            if (ReturnSingle)
                break;
        }
    }
    else if (!Stop)
    {
        ExFatAcquireFatFs(Vcb);
        if (Ccb->DirectoryIndexBacked)
        {
            Result = f_rewinddir(&Ccb->Directory);
            for (SavedIndex = DotEntries;
                 Result == FR_OK && SavedIndex < Ccb->DirectoryIndex;
                 SavedIndex++)
            {
                RtlZeroMemory(&FatInformation, sizeof(FatInformation));
                Result = f_readdir(&Ccb->Directory, &FatInformation);
                if (Result == FR_OK && !FatInformation.fname[0])
                    break;
            }
            Ccb->DirectoryIndexBacked = FALSE;
            if (Result != FR_OK)
            {
                Status = ExFatMapResult(Result);
                ExFatReleaseFatFs(Vcb);
                goto Done;
            }
        }

        while (!Stop)
        {
            SavedDirectory = Ccb->Directory;
            SavedIndex = Ccb->DirectoryIndex;
            RtlZeroMemory(&FatInformation, sizeof(FatInformation));
            Result = f_readdir(&Ccb->Directory, &FatInformation);
            if (Result != FR_OK)
            {
                Status = ExFatMapResult(Result);
                break;
            }
            if (!FatInformation.fname[0])
                break;
            Ccb->DirectoryIndex++;

            /* TCHAR is WCHAR here: the on-disk UTF-16 name needs no conversion. */
            RtlInitUnicodeString(&Name, FatInformation.fname);

            if (!MatchAll &&
                !FsRtlIsNameInExpression(&Ccb->SearchPattern, &Name, TRUE, NULL))
                continue;

            Required = EntryBaseSize + Name.Length;
            AlignedRequired = EXFAT_ALIGN_DIRECTORY_ENTRY(Required);
            if (Required > BufferLength - Offset)
            {
                Ccb->Directory = SavedDirectory;
                Ccb->DirectoryIndex = SavedIndex;
                Status = Found ? STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW;
                break;
            }

            if (PreviousNextOffset)
                *PreviousNextOffset = Offset - ((PUCHAR)PreviousNextOffset - Buffer);
            PreviousNextOffset = (PULONG)(Buffer + Offset);
            /* Cover struct padding and the 8-byte inter-entry gap. */
            Emitted = min(AlignedRequired, BufferLength - Offset);
            RtlZeroMemory(Buffer + Offset, Emitted);
            ExFatFillDirectoryEntry(InformationClass,
                                    Buffer + Offset,
                                    Ccb->DirectoryIndex,
                                    &FatInformation,
                                    &Name,
                                    Vcb->BytesPerCluster);
            Found = TRUE;
            Offset += Emitted;
            if (ReturnSingle)
                break;
        }
        ExFatReleaseFatFs(Vcb);
    }

    if (!Found && NT_SUCCESS(Status))
        Status = FreshScan ? STATUS_NO_SUCH_FILE : STATUS_NO_MORE_FILES;

Done:
    ExReleaseResourceLite(&Fcb->MainResource);
    ExReleaseResourceLite(&Vcb->Resource);
    Irp->IoStatus.Information = Offset;
    return Status;
}

NTSTATUS
ExFatDirectoryControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceObject == ExFatGlobalData->DeviceObject)
        return STATUS_INVALID_DEVICE_REQUEST;
    if (Stack->MinorFunction == IRP_MN_QUERY_DIRECTORY)
        return ExFatQueryDirectory(DeviceObject, Irp);
    if (Stack->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
        return STATUS_NOT_SUPPORTED;
    return STATUS_INVALID_DEVICE_REQUEST;
}
