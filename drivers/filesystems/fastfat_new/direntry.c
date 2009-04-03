/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/direntry.c
 * PURPOSE:         Directory entries
 * PROGRAMMERS:     Alexey Vlasov
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* PROTOTYPES ***************************************************************/

typedef enum _FILE_TIME_INDEX
{
    FileCreationTime = 0,
    FileLastAccessTime,
    FileLastWriteTime,
    FileChangeTime
} FILE_TIME_INDEX;
 

VOID
Fat8dot3ToUnicodeString(OUT PUNICODE_STRING FileName,
                        IN PUCHAR ShortName,
                        IN UCHAR Flags);

ULONG
FatDirentToDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                   IN PDIR_ENTRY Dirent,
                   IN PVOID Buffer);

ULONG
FatDirentToFullDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                       IN PDIR_ENTRY Dirent,
                       IN PVOID Buffer);

ULONG
FatDirentToIdFullDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                         IN PDIR_ENTRY Dirent,
                         IN PVOID Buffer);

ULONG
FatDirentToBothDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                       IN PDIR_ENTRY Dirent,
                       IN PVOID Buffer);

ULONG
FatDirentToIdBothDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                         IN PDIR_ENTRY Dirent,
                         IN PVOID Buffer);

ULONG
FatDirentToNamesInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                     IN PDIR_ENTRY Dirent,
                     IN PVOID Buffer);

ULONG
FatDirentToObjectIdInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                        IN PDIR_ENTRY Dirent,
                        IN PVOID Buffer);

/* FUNCTIONS *****************************************************************/

#define FatQueryFileName(xInfo, xDirent)                    \
{                                                           \
    UNICODE_STRING FileName;                                \
    if (Info->FileNameLength == 0)                          \
    {                                                       \
        FileName.Buffer = Info->FileName;                   \
        FileName.MaximumLength = 0x18;                      \
        Fat8dot3ToUnicodeString(&FileName,                  \
            (xDirent)->FileName, (xDirent)->Case);          \
        Info->FileNameLength = FileName.Length;             \
    }                                                       \
}

#define FatQueryBothFileName(xInfo, xDirent)                \
{                                                           \
    UNICODE_STRING FileName;                                \
    FileName.MaximumLength = 0x18;                          \
    if (Info->FileNameLength == 0)                          \
    {                                                       \
        FileName.Buffer = Info->FileName;                   \
        Fat8dot3ToUnicodeString(&FileName,                  \
            (xDirent)->FileName, (xDirent)->Case);          \
        Info->FileNameLength = FileName.Length;             \
        Info->ShortNameLength = 0;                          \
    }                                                       \
    else                                                    \
    {                                                       \
        FileName.Buffer = Info->ShortName;                  \
        Fat8dot3ToUnicodeString(&FileName,                  \
            (xDirent)->FileName, (xDirent)->Case);          \
        Info->ShortNameLength = (CCHAR) FileName.Length;    \
    }                                                       \
}

FORCEINLINE
UCHAR
FatLfnChecksum(
    PUCHAR Buffer
)
{
    UCHAR Index, Chksum;

    for (Index = 0x1, Chksum = *Buffer;
        Index < RTL_FIELD_SIZE(DIR_ENTRY, FileName);
        Index ++)
    {
        Chksum = (((Chksum & 0x1) << 0x7) | (Chksum >> 0x1))
            + Buffer[Index];
    }
    return Chksum;
}

VOID
FatFindDirent(IN OUT PFAT_FIND_DIRENT_CONTEXT Context,
              OUT PDIR_ENTRY* Dirent,
              OUT PUNICODE_STRING LongFileName OPTIONAL)
{
    PDIR_ENTRY Entry, EndOfPage;
    UCHAR SeqNum = 0, Checksum = 0;
    PUNICODE_STRING FileName;

    /* Pin first page. */
    Entry = (PDIR_ENTRY) FatPinPage(&Context->Page, 0);
    EndOfPage = FatPinEndOfPage(&Context->Page, PDIR_ENTRY);

    /* Run dirents. */
    FileName = NULL;
    while (TRUE)
    {
        /* Check if we have entered the area of never used dirents */
        if (Entry->FileName[0] == FAT_DIRENT_NEVER_USED)
            ExRaiseStatus(STATUS_OBJECT_NAME_NOT_FOUND);

        /* Just ignore entries marked as deleted */
        if (Entry->FileName[0] == FAT_DIRENT_DELETED)
            goto FatFindDirentNext;

        /* Check if it's an lfn */
        if (Entry->Attributes == FAT_DIRENT_ATTR_LFN)
        {
            PLONG_FILE_NAME_ENTRY LfnEntry;
       
            FileName = LongFileName;
            LfnEntry = (PLONG_FILE_NAME_ENTRY) Entry;

            /* Run lfns and collect file name if required */
            do {
                PWSTR Lfn;

                /* Check if we just running lfn */
                if (FileName == NULL)
                    goto FatFindDirentRunLfn;

                /* Check for cluster index to be zero. */
                if (LfnEntry->Reserved != 0)
                {
                    FileName = NULL;
                    goto FatFindDirentRunLfn;
                }

                /* Check if this is the last lfn entry. */
                if (FlagOn(LfnEntry->SeqNum, FAT_FN_DIR_ENTRY_LAST))
                {
                    SeqNum = (LfnEntry->SeqNum & ~FAT_FN_DIR_ENTRY_LAST);
                    Checksum = LfnEntry->Checksum;

                    /* Check if we exceed max number of lfns */
                    if (SeqNum > (FAT_FN_MAX_DIR_ENTIES + 1))
                    {
                        FileName = NULL;
                        goto FatFindDirentRunLfn;
                    }

                    /* Setup maximal expected lfn length */
                    FileName->Length = (SeqNum * FAT_LFN_NAME_LENGTH);

                    /* Extend lfn buffer if needed */
                    if (FileName->Length > FileName->MaximumLength)
                    {
                        Lfn = ExAllocatePoolWithTag(PagedPool,
                            LongFileName->Length, TAG_VFAT);
                        if (Lfn == NULL)
                            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                        if (FileName->Buffer != NULL)
                            ExFreePool(FileName->Buffer);
                        FileName->Buffer = Lfn;
                        FileName->MaximumLength = FileName->Length;
                    }
                }
                else if (!(LfnEntry->SeqNum == SeqNum
                    && LfnEntry->Checksum == Checksum))
                {
                   /* Wrong SeqNum or CheckSum. */
                    FileName = NULL;
                    goto FatFindDirentRunLfn;
                }
                /* Gather file name */
                Lfn = Add2Ptr(FileName->Buffer, (SeqNum * FAT_LFN_NAME_LENGTH)
                        - sizeof(LfnEntry->NameC), PWSTR);
                RtlCopyMemory(Lfn, LfnEntry->NameC, sizeof(LfnEntry->NameC));
                Lfn -= (sizeof(LfnEntry->NameB) / sizeof(WCHAR));
                RtlCopyMemory(Lfn, LfnEntry->NameB, sizeof(LfnEntry->NameB));
                Lfn -= (sizeof(LfnEntry->NameA) / sizeof(WCHAR));
                RtlCopyMemory(Lfn, LfnEntry->NameA, sizeof(LfnEntry->NameA));

                /* If last lfn determine exact lfn length. */
                if (FlagOn(LfnEntry->SeqNum, FAT_FN_DIR_ENTRY_LAST))
                {
                    PWSTR LfnEnd = Add2Ptr(FileName->Buffer,
                        (FileName->Length - sizeof(WCHAR)), PWSTR);

                    /* Trim trailing 0xffff's */
                    while (LfnEnd > Lfn && *LfnEnd == 0xffff) --LfnEnd;

                    /* Trim 0 terminator is the one is there. */
                    if (*LfnEnd == 0x0) --LfnEnd;

                    /* Set correct lfn size */
                    FileName->Length = (USHORT)PtrOffset(FileName->Buffer, LfnEnd);
                }
                /* Setup validation for the next iteration */
                SeqNum = LfnEntry->SeqNum - 0x1;
                Checksum = LfnEntry->Checksum;

FatFindDirentRunLfn:
                /* Get next dirent */
                LfnEntry ++;
                if (LfnEntry > (PLONG_FILE_NAME_ENTRY) EndOfPage)
                {
                    if (FatPinIsLastPage(&Context->Page))
                        ExRaiseStatus(STATUS_OBJECT_NAME_NOT_FOUND);
                    LfnEntry = (PLONG_FILE_NAME_ENTRY) FatPinNextPage(&Context->Page);
                    EndOfPage = FatPinEndOfPage(&Context->Page, PDIR_ENTRY);
                }
            }
            while (LfnEntry->Attributes == FAT_DIRENT_ATTR_LFN);
            Entry = (PDIR_ENTRY) LfnEntry;
            continue;
        }

        /* If we've got here then this is a normal dirent */
        if (FileName != NULL && FileName->Length > 0)
        {
            /* Check if we have a correct lfn collected. */
            if (FatLfnChecksum(Entry->FileName) != Checksum)
            {
                FileName = NULL;
            }
            else
            {
                /* See if we were looking for this dirent. */
                if (!Context->Valid8dot3Name &&
                    FsRtlAreNamesEqual(FileName, Context->FileName, TRUE, NULL))
                {
                    Fat8dot3ToUnicodeString(&Context->ShortName, Entry->FileName, Entry->Case);
                    *Dirent = Entry;
                    return;
                }
            }
        }

       /* We surely have a short name, check if we were looking for that. */
        if (Context->Valid8dot3Name)
        {
            Fat8dot3ToUnicodeString(&Context->ShortName,
                Entry->FileName, Entry->Case);
            if (FsRtlAreNamesEqual(&Context->ShortName, Context->FileName, TRUE, NULL))
            {
                if (ARGUMENT_PRESENT(LongFileName) && FileName == NULL)
                    LongFileName->Length = 0;
                *Dirent = Entry;
                return;
            }
        }
        FileName = NULL;

FatFindDirentNext:
        Entry ++;
        if (Entry > EndOfPage)
        {
            if (FatPinIsLastPage(&Context->Page))
                ExRaiseStatus(STATUS_OBJECT_NAME_NOT_FOUND);
            Entry = (PDIR_ENTRY) FatPinNextPage(&Context->Page);
            EndOfPage = FatPinEndOfPage(&Context->Page, PDIR_ENTRY);
        }
    }
    /* Should never get here! */
    ASSERT(TRUE);
}

VOID
FatEnumerateDirents(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                    IN SIZE_T Offset)
{
    PUCHAR Entry, EndOfPage;
    PWSTR FileName;
    USHORT FileNameLength;
    USHORT FileNameMaximumLength;
    PVOID InfoEntry;
    UCHAR SeqNum = 0;
    UCHAR Checksum = 0;

    /* Pin first page. */
    Entry = (PUCHAR)FatPinPage(&Context->Page, Offset);
    EndOfPage = FatPinEndOfPage(&Context->Page, PUCHAR);

    /* Iterate dirents. */
    while (TRUE)
    {
        /* Check if we have entered the area of never used dirents */
        if (*Entry == FAT_DIRENT_NEVER_USED)
            ExRaiseStatus(STATUS_NO_MORE_FILES);

        /* Just ignore entries marked as deleted */
        if (*Entry == FAT_DIRENT_DELETED)
            goto FatEnumerateDirentsNext;

        /* Get info pointer. */
        InfoEntry = Add2Ptr(Context->Buffer, Context->Offset, PVOID);

        /* Check if info class has file name */
        if (Context->NameOffset == Context->LengthOffset)
        {
            FileName = NULL;
            FileNameMaximumLength = 0;
        }
        else
        {
            FileName = Add2Ptr(InfoEntry, Context->NameOffset, PWSTR);
            FileNameMaximumLength = (USHORT) (Context->Length
                - Context->Offset + Context->NameOffset);
        }
        FileNameLength = 0;

        /* Check if it's an lfn */
        while (Entry[0xb] == FAT_DIRENT_ATTR_LFN)
        {
            PWSTR Lfn;
            PLONG_FILE_NAME_ENTRY LfnEntry;

            /* Check if we just running lfn */
            if (FileNameMaximumLength == 0)
                goto FatEnumerateDirentsRunLfn;

            LfnEntry = (PLONG_FILE_NAME_ENTRY) Entry;

            /* Check for cluster index to be zero. */
            if (LfnEntry->Reserved != 0)
            {
                FileNameMaximumLength = 0;
                goto FatEnumerateDirentsRunLfn;
            }

            /* Check if this is the last lfn entry. */
            if (FlagOn(LfnEntry->SeqNum, FAT_FN_DIR_ENTRY_LAST))
            {
                SeqNum = (LfnEntry->SeqNum & ~FAT_FN_DIR_ENTRY_LAST);
                Checksum = LfnEntry->Checksum;

                /* Check if we exceed max number of lfns */
                if (SeqNum > (FAT_FN_MAX_DIR_ENTIES + 1))
                {
                    FileNameMaximumLength = 0;
                    goto FatEnumerateDirentsRunLfn;
                }

                /* Setup maximal expected lfn length */
                FileNameLength = SeqNum * FAT_LFN_NAME_LENGTH;

                /* Validate the maximal expected lfn length */
                if (FileNameLength > FileNameMaximumLength)
                    goto FatEnumerateDirentsRunLfn;
            }
            else if (!(LfnEntry->SeqNum == SeqNum
                && LfnEntry->Checksum == Checksum))
            {
               /* Wrong SeqNum or CheckSum. */
                FileNameMaximumLength = 0;
                goto FatEnumerateDirentsRunLfn;
            }
            /* Gather file name */
            Lfn = Add2Ptr(FileName, (SeqNum * FAT_LFN_NAME_LENGTH)
                    - sizeof(LfnEntry->NameC), PWSTR);
            RtlCopyMemory(Lfn, LfnEntry->NameC, sizeof(LfnEntry->NameC));
            Lfn -= (sizeof(LfnEntry->NameB) / sizeof(WCHAR));
            RtlCopyMemory(Lfn, LfnEntry->NameB, sizeof(LfnEntry->NameB));
            Lfn -= (sizeof(LfnEntry->NameA) / sizeof(WCHAR));
            RtlCopyMemory(Lfn, LfnEntry->NameA, sizeof(LfnEntry->NameA));

            /* If last lfn determine exact lfn length. */
            if (FlagOn(LfnEntry->SeqNum, FAT_FN_DIR_ENTRY_LAST))
            {
                PWSTR LfnEnd = Add2Ptr(FileName, FileNameLength
                    - sizeof(WCHAR), PWSTR);

                /* Trim trailing 0xffff's */
                while (LfnEnd > Lfn && *LfnEnd == 0xffff) --LfnEnd;

                /* Trim 0 terminator is the one is there. */
                if (*LfnEnd == 0x0) --LfnEnd;

                /* Set correct lfn size */
                FileNameLength = (USHORT)PtrOffset(FileName, LfnEnd);
            }
            /* Setup vaidation for the next iteration */
            SeqNum = LfnEntry->SeqNum - 0x1;
            Checksum = LfnEntry->Checksum;
FatEnumerateDirentsRunLfn:
            Entry = Add2Ptr(Entry, sizeof(DIR_ENTRY), PUCHAR);
            if (Entry > EndOfPage)
            {
                if (FatPinIsLastPage(&Context->Page))
                    ExRaiseStatus(STATUS_NO_MORE_FILES);
                Entry = (PUCHAR) FatPinNextPage(&Context->Page);
                EndOfPage = FatPinEndOfPage(&Context->Page, PUCHAR);
            }
        }

        /* if lfn was found, validate and commit. */
        if (FileNameLength > 0 && FatLfnChecksum(Entry) == Checksum)
        {
            *Add2Ptr(InfoEntry, Context->LengthOffset, PULONG) = FileNameLength;
        }
        else
        {
            *Add2Ptr(InfoEntry, Context->LengthOffset, PULONG) = 0;
        }
        /* TODO: Implement Filtering using Context->FileName & Context->CcbFlags. */

        /* Copy the entry values. */
        Context->Offset += Context->CopyDirent((PFAT_ENUM_DIR_CONTEXT)Context, (PDIR_ENTRY) Entry, InfoEntry);

FatEnumerateDirentsNext:
        /* Get next entry */
        Entry = Add2Ptr(Entry, sizeof(DIR_ENTRY), PUCHAR);
        if (Entry > EndOfPage)
        {
            if (FatPinIsLastPage(&Context->Page))
                ExRaiseStatus(STATUS_NO_MORE_FILES);
            Entry = (PUCHAR) FatPinNextPage(&Context->Page);
            EndOfPage = FatPinEndOfPage(&Context->Page, PUCHAR);
        }
    }
}

FORCEINLINE
VOID
FatDateTimeToSystemTime(OUT PLARGE_INTEGER SystemTime,
                        IN PFAT_DATETIME FatDateTime,
                        IN UCHAR TenMs OPTIONAL)
{
    TIME_FIELDS TimeFields;

    /* Setup time fields */
    TimeFields.Year = FatDateTime->Date.Year + 1980;
    TimeFields.Month = FatDateTime->Date.Month;
    TimeFields.Day = FatDateTime->Date.Day;
    TimeFields.Hour = FatDateTime->Time.Hour;
    TimeFields.Minute = FatDateTime->Time.Minute;
    TimeFields.Second = (FatDateTime->Time.DoubleSeconds << 1);

    /* Adjust up to 10 milliseconds
     * if the parameter was supplied
     */
    if (ARGUMENT_PRESENT(TenMs))
    {
        TimeFields.Second += TenMs / 100;
        TimeFields.Milliseconds = (TenMs % 100) * 10;
    }
    else
    {
        TimeFields.Milliseconds = 0;
    }

    /* Fix seconds value that might get beyoud the bound */
    if (TimeFields.Second > 59) TimeFields.Second = 0;

    /* Perform ceonversion to system time if possible */
    if (RtlTimeFieldsToTime(&TimeFields, SystemTime))
    {
        /* Convert to system time */
        ExLocalTimeToSystemTime(SystemTime, SystemTime);
    }
    else
    {
        /* Set to default time if conversion failed */
        *SystemTime = FatGlobalData.DefaultFileTime;
    }
}

VOID
FatQueryFileTimes(OUT PLARGE_INTEGER FileTimes,
                  IN PDIR_ENTRY Dirent)
{
    /* Convert LastWriteTime */
    FatDateTimeToSystemTime(&FileTimes[FileLastWriteTime],
                            &Dirent->LastWriteDateTime,
                            0);
    /* All other time fileds are valid (according to MS)
     * only if Win31 compatability mode is set.
     */
    if (FatGlobalData.Win31FileSystem)
    {
       /* We can avoid calling conversion routine
        * if time in dirent is 0 or equals to already
        * known time (LastWriteTime).
        */
        if (Dirent->CreationDateTime.Value == 0)
        {
            /* Set it to default time */
            FileTimes[FileCreationTime] = FatGlobalData.DefaultFileTime;
        }
        else if (Dirent->CreationDateTime.Value
            == Dirent->LastWriteDateTime.Value)
        {
            /* Assign the already known time */
            FileTimes[FileCreationTime] = FileTimes[FileLastWriteTime];
            /* Adjust milliseconds from extra dirent field */
            FileTimes[FileCreationTime].QuadPart
                += (ULONG) Dirent->CreationTimeTenMs * 100000;
        }
        else
        {
            /* Perform conversion */
            FatDateTimeToSystemTime(&FileTimes[FileCreationTime],
                                    &Dirent->CreationDateTime,
                                    Dirent->CreationTimeTenMs);
        }
        if (Dirent->LastAccessDate.Value == 0)
        {
            /* Set it to default time */
            FileTimes[FileLastAccessTime] = FatGlobalData.DefaultFileTime;
        }
        else if (Dirent->LastAccessDate.Value
                == Dirent->LastWriteDateTime.Date.Value)
        {
            /* Assign the already known time */
            FileTimes[FileLastAccessTime] = FileTimes[FileLastWriteTime];
        }
        else
        {
            /* Perform conversion */
            FAT_DATETIME LastAccessDateTime;

            LastAccessDateTime.Date.Value = Dirent->LastAccessDate.Value;
            LastAccessDateTime.Time.Value = 0;
            FatDateTimeToSystemTime(&FileTimes[FileLastAccessTime],
                                    &LastAccessDateTime,
                                    0);
        }
    }
}

VOID
Fat8dot3ToUnicodeString(OUT PUNICODE_STRING FileName,
                        IN PUCHAR ShortName,
                        IN UCHAR Flags)
{
    PCHAR Name;
    UCHAR Index, Ext = 0;
    OEM_STRING Oem;

    Name = Add2Ptr(FileName->Buffer, 0x0c, PCHAR);
    RtlCopyMemory(Name, ShortName, 0x0b);

    /* Restore the name byte used to mark deleted entries */
    if (Name[0] == 0x05)
        Name[0] |= 0xe0;

    /* Locate the end of name part */
    for (Index = 0; Index < 0x08
        && Name[Index] != 0x20; Index++);

    /* Locate the end of extension part */
    if (Name[0x08] != 0x20)
    {
        Ext = 0x2;
        Name[Index++] = 0x2e;
        Name[Index++] = Name[0x08];
        if ((Name[Index] = Name[0x09]) != 0x20)
        {
            Index ++; Ext ++;
        }
        if ((Name[Index] = Name[0x0a]) != 0x20)
        {
            Index ++; Ext ++;
        }
    }

    /* Perform Oem to Unicode conversion. */
    Oem.Buffer = Name;
    Oem.Length = Index;
    Oem.MaximumLength = Index;
    RtlOemStringToUnicodeString(FileName, &Oem, FALSE);
    Index = FlagOn(Flags, FAT_CASE_LOWER_BASE|FAT_CASE_LOWER_EXT);
    if (Index > 0)
    {
        /* Downcase the whole name */
        if (Index == (FAT_CASE_LOWER_BASE|FAT_CASE_LOWER_EXT))
        {
            RtlUpcaseUnicodeString(FileName, FileName, FALSE);
        }
        else
        {
            if (Index == FAT_CASE_LOWER_EXT)
            {
                /* Set extension for downcase */
                Oem.Length = Ext * sizeof(WCHAR);
                Oem.Buffer = Add2Ptr(FileName->Buffer,
                                     FileName->Length - Oem.Length,
                                     PSTR);
            }
            else
            {
                /* Set base name for downcase */
                Oem.Buffer = (PSTR) FileName->Buffer;
                Oem.Length = FileName->Length - Ext * sizeof(WCHAR);
            }
            Oem.MaximumLength = Oem.Length;
            RtlUpcaseUnicodeString((PUNICODE_STRING)&Oem,
                                   (PUNICODE_STRING)&Oem,
                                    FALSE);
        }
    }
}

ULONG
FatDirentToDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                   IN PDIR_ENTRY Dirent,
                   IN PVOID Buffer)
{
    PFILE_DIRECTORY_INFORMATION Info;
    Info = (PFILE_DIRECTORY_INFORMATION) Buffer;
    Info->FileIndex = 0;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FatQueryFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToFullDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                       IN PDIR_ENTRY Dirent,
                       IN PVOID Buffer)
{
    PFILE_FULL_DIR_INFORMATION Info;
    Info = (PFILE_FULL_DIR_INFORMATION) Buffer;
    Info->FileIndex = 0;
    Info->EaSize = 0;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FatQueryFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToIdFullDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                         IN PDIR_ENTRY Dirent,
                         IN PVOID Buffer)
{
    PFILE_ID_FULL_DIR_INFORMATION Info;
    Info = (PFILE_ID_FULL_DIR_INFORMATION) Buffer;
    Info->FileId.QuadPart = 0;
    Info->FileIndex = 0;
    Info->EaSize = 0;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FatQueryFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToBothDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                       IN PDIR_ENTRY Dirent,
                       IN PVOID Buffer)
{
    PFILE_BOTH_DIR_INFORMATION Info;
    Info = (PFILE_BOTH_DIR_INFORMATION) Buffer;
    Info->FileIndex = 0;
    Info->EaSize = 0;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FatQueryBothFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToIdBothDirInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                         IN PDIR_ENTRY Dirent,
                         IN PVOID Buffer)
{
    PFILE_ID_BOTH_DIR_INFORMATION Info;
    Info = (PFILE_ID_BOTH_DIR_INFORMATION) Buffer;
    Info->FileId.QuadPart = 0;
    Info->FileIndex = 0;
    Info->EaSize = 0;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FatQueryBothFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToNamesInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                     IN PDIR_ENTRY Dirent,
                     IN PVOID Buffer)
{
    PFILE_NAMES_INFORMATION Info;
    Info = (PFILE_NAMES_INFORMATION) Buffer;
    Info->FileIndex = 0;
    FatQueryFileName(Info, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
    return Info->NextEntryOffset + Info->FileNameLength;
}

ULONG
FatDirentToObjectIdInfo(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                        IN PDIR_ENTRY Dirent,
                        IN PVOID Buffer)
{
    PFILE_OBJECTID_INFORMATION Info;
    Info = (PFILE_OBJECTID_INFORMATION) Buffer;
    Info->FileReference = 0;
    ((PLONGLONG)Info->ObjectId)[0] = 0LL;
    ((PLONGLONG)Info->ObjectId)[1] = 0LL;
    return sizeof(*Info);
}

/* EOF */
