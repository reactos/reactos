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
#define BYTES_PER_DIRENT_LOG 0x05

typedef struct _FAT_ENUM_DIR_CONTEXT *PFAT_ENUM_DIR_CONTEXT;

typedef ULONG (*PFAT_COPY_DIRENT_ROUTINE) (struct _FAT_ENUM_DIR_CONTEXT *, PDIR_ENTRY, PVOID);

typedef struct _FAT_ENUM_DIR_CONTEXT
{
    PFILE_OBJECT FileObject;
    LARGE_INTEGER PageOffset;
    LONGLONG BeyoundLastEntryOffset;
    PVOID PageBuffer;
    PBCB PageBcb;

   /*
    * We should always cache short file name
    * because we never know if there is a long
    * name for the dirent and when we find out
    * that our base dirent might become unpinned.
    */
    UCHAR DirentFileName[RTL_FIELD_SIZE(DIR_ENTRY, FileName) + 1];
    PFAT_COPY_DIRENT_ROUTINE CopyDirent;
    LONGLONG BytesPerClusterMask;
   /*
    * The following fields are
    * set by the copy routine
    */
    PULONG NextEntryOffset;
    PULONG FileNameLength;
    PWCHAR FileName;
} FAT_ENUM_DIR_CONTEXT;

typedef enum _FILE_TIME_INDEX {
    FileCreationTime = 0,
    FileLastAccessTime,
    FileLastWriteTime,
    FileChangeTime
} FILE_TIME_INDEX;

VOID
FatQueryFileTimes(
    OUT PLARGE_INTEGER FileTimes,
    IN PDIR_ENTRY Dirent);

VOID
Fat8dot3ToUnicodeString(
    OUT PUNICODE_STRING FileName,
    IN PUCHAR ShortName,
    IN UCHAR Flags);

ULONG
FatDirentToDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToFullDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToIdFullDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToBothDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToIdBothDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToNamesInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

ULONG
FatDirentToObjectIdInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer);

/* FUNCTIONS *****************************************************************/

FORCEINLINE
VOID
FatDateTimeToSystemTime(
    OUT PLARGE_INTEGER SystemTime,
    IN PFAT_DATETIME FatDateTime,
    IN UCHAR TenMs OPTIONAL)
{
    TIME_FIELDS TimeFields;

   /*
    * Setup time fields.
    */
    TimeFields.Year = FatDateTime->Date.Year + 1980;
    TimeFields.Month = FatDateTime->Date.Month;
    TimeFields.Day = FatDateTime->Date.Day;
    TimeFields.Hour = FatDateTime->Time.Hour;
    TimeFields.Minute = FatDateTime->Time.Minute;
    TimeFields.Second = (FatDateTime->Time.DoubleSeconds << 1);
   /*
    * Adjust up to 10 milliseconds
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
   /*
    * Fix seconds value that might get beyoud the bound.
    */
    if (TimeFields.Second > 59)
        TimeFields.Second = 0;
   /*
    * Perform ceonversion to system time if possible.
    */
    if (RtlTimeFieldsToTime(&TimeFields, SystemTime)) {
        /* Convert to system time */
        ExLocalTimeToSystemTime( SystemTime, SystemTime );
    }
    else
    {
        /* Set to default time if conversion failed */
        *SystemTime = FatGlobalData.DefaultFileTime;
    }
}

VOID
FatQueryFileTimes(
    OUT PLARGE_INTEGER FileTimes,
    IN PDIR_ENTRY Dirent)
{
   /*
    * Convert LastWriteTime
    */
    FatDateTimeToSystemTime(
        &FileTimes[FileLastWriteTime],
        &Dirent->LastWriteDateTime, 0);
   /*
    * All other time fileds are valid (according to MS)
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
            FatDateTimeToSystemTime(
                &FileTimes[FileCreationTime],
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
            FatDateTimeToSystemTime(
                &FileTimes[FileLastAccessTime],
                &LastAccessDateTime, 0);
        }
    }
}

VOID
Fat8dot3ToUnicodeString(
	OUT PUNICODE_STRING FileName,
	IN PUCHAR ShortName,
	IN UCHAR Flags)
{
    PCHAR Name;
    UCHAR Index, Ext = 0;
    OEM_STRING Oem;

    Name = Add2Ptr(FileName->Buffer, 0x0c, PCHAR);
    RtlCopyMemory(Name, ShortName, 0x0b);

    /* Restore the name byte used to mark deleted entries. */
    if (Name[0] == 0x05)
        Name[0] |= 0xe0;

    /* Locate the end of name part. */
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
                Oem.Buffer = Add2Ptr(
                    FileName->Buffer,
                    FileName->Length - Oem.Length,
                    PSTR);
            }
            else
            {
                /* Set base name for downcase */
                Oem.Buffer = (PSTR) FileName->Buffer;
                Oem.Length = FileName->Length
                    - Ext * sizeof(WCHAR);
            }
            Oem.MaximumLength = Oem.Length;
            RtlUpcaseUnicodeString(
                (PUNICODE_STRING)&Oem,
                (PUNICODE_STRING)&Oem,
                FALSE);
        }
    }
}

ULONG
FatDirentToDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_DIRECTORY_INFORMATION Info;
    //UNICODE_STRING FileName;

    Info = (PFILE_DIRECTORY_INFORMATION) Buffer;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    //FileName.Buffer = Info->ShortName;
    //FileName.MaximumLength = sizeof(Info->ShortName);
    // FatQueryShortName(&FileName, Dirent);
    // Info->ShortNameLength = (CCHAR) FileName.Length;
    Info->NextEntryOffset = sizeof(*Info);
   /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToFullDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_FULL_DIR_INFORMATION Info;
    //UNICODE_STRING FileName;

    Info = (PFILE_FULL_DIR_INFORMATION) Buffer;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    //FileName.Buffer = Info->ShortName;
    //FileName.MaximumLength = sizeof(Info->ShortName);
    // FatQueryShortName(&FileName, Dirent);
    // Info->ShortNameLength = (CCHAR) FileName.Length;
    Info->NextEntryOffset = sizeof(*Info);
   /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToIdFullDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_ID_FULL_DIR_INFORMATION Info;
    //UNICODE_STRING FileName;

    Info = (PFILE_ID_FULL_DIR_INFORMATION) Buffer;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    //FileName.Buffer = Info->ShortName;
    //FileName.MaximumLength = sizeof(Info->ShortName);
    // FatQueryShortName(&FileName, Dirent);
    // Info->ShortNameLength = (CCHAR) FileName.Length;
    Info->NextEntryOffset = sizeof(*Info);
    /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToBothDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_BOTH_DIR_INFORMATION Info;
    UNICODE_STRING FileName;

    Info = (PFILE_BOTH_DIR_INFORMATION) Buffer;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FileName.Buffer = Info->ShortName;
    FileName.MaximumLength = sizeof(Info->ShortName);
    Fat8dot3ToUnicodeString(&FileName, Dirent->FileName, Dirent->Case);
    Info->ShortNameLength = (CCHAR) FileName.Length;
    Info->NextEntryOffset = sizeof(*Info);
   /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToIdBothDirInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_ID_BOTH_DIR_INFORMATION Info;
    UNICODE_STRING FileName;

    Info = (PFILE_ID_BOTH_DIR_INFORMATION) Buffer;
    /* Setup Attributes */
    Info->FileAttributes = Dirent->Attributes;
    /* Setup times */
    FatQueryFileTimes(&Info->CreationTime, Dirent);
    /* Setup sizes */
    Info->EndOfFile.QuadPart = Dirent->FileSize;
    Info->AllocationSize.QuadPart =
        (Context->BytesPerClusterMask + Dirent->FileSize)
            & ~(Context->BytesPerClusterMask);
    FileName.Buffer = Info->ShortName;
    FileName.MaximumLength = sizeof(Info->ShortName);
    Fat8dot3ToUnicodeString(&FileName, Dirent->FileName, Dirent->Case);
    Info->ShortNameLength = (CCHAR) FileName.Length;
    Info->NextEntryOffset = sizeof(*Info);
   /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToNamesInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_NAMES_INFORMATION Info;

    Info = (PFILE_NAMES_INFORMATION) Buffer;
//    FatQueryShortName(&FileName, Dirent);
    Info->NextEntryOffset = sizeof(*Info);
   /*
    * Associate LFN buffer and length pointers
    * of this entry with the context.
    */
    Context->NextEntryOffset = &Info->NextEntryOffset;
    Context->FileName = Info->FileName;
    Context->FileNameLength = &Info->FileNameLength;
    return Info->NextEntryOffset;
}

ULONG
FatDirentToObjectIdInfo(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN PDIR_ENTRY Dirent,
    IN PVOID Buffer)
{
    PFILE_OBJECTID_INFORMATION Info;

    Info = (PFILE_OBJECTID_INFORMATION) Buffer;
    return sizeof(*Info);
}

ULONG
FatEnumerateDirents(
    IN OUT PFAT_ENUM_DIR_CONTEXT Context,
    IN ULONG Index,
    IN BOOLEAN CanWait)
{
    LONGLONG PageOffset;
    SIZE_T OffsetWithinPage, PageValidLength;
    PUCHAR Entry, BeyoudLastEntry;
   /*
    * Determine page offset and the offset within page
    * for the first cluster.
    */
    PageValidLength = PAGE_SIZE;
    PageOffset = ((LONGLONG) Index) << BYTES_PER_DIRENT_LOG;
    OffsetWithinPage = (SIZE_T) (PageOffset & (PAGE_SIZE - 1));
    PageOffset -= OffsetWithinPage;
   /*
    * Check if the context already has the required page mapped.
    * Map the first page is necessary.
    */
    if (PageOffset != Context->PageOffset.QuadPart)
    {
        Context->PageOffset.QuadPart = PageOffset;
        if (Context->PageBcb != NULL)
        {
            CcUnpinData(Context->PageBcb);
            Context->PageBcb = NULL;
        }
        if (!CcMapData(Context->FileObject, &Context->PageOffset,
            PAGE_SIZE, CanWait, &Context->PageBcb, &Context->PageBuffer))
        {
            Context->PageOffset.QuadPart = 0LL;
            ExRaiseStatus(STATUS_CANT_WAIT);
        }
    }
    Entry = Add2Ptr(Context->PageBuffer, OffsetWithinPage, PUCHAR);
   /*
    * Next Page Offset.
    */
    PageOffset = Context->PageOffset.QuadPart + PAGE_SIZE;
    if (PageOffset > Context->BeyoundLastEntryOffset)
        PageValidLength = (SIZE_T) (Context->BeyoundLastEntryOffset
            - Context->PageOffset.QuadPart);
    BeyoudLastEntry = Add2Ptr(Context->PageBuffer, PageValidLength, PUCHAR);
    while (TRUE)
	{
        do
        {
            if (*Entry == FAT_DIRENT_NEVER_USED)
                return 0; // TODO: return something reasonable.
            if (*Entry == FAT_DIRENT_DELETED)
            {
                continue;
            }
            if (Entry[0x0a] == FAT_DIRENT_ATTR_LFN)
            {
                PLONG_FILE_NAME_ENTRY Lfnent;
                Lfnent = (PLONG_FILE_NAME_ENTRY) Entry;
            }
            else
            {
                PDIR_ENTRY Dirent;
                Dirent = (PDIR_ENTRY) Entry;
                RtlCopyMemory(Context->DirentFileName,
                    Dirent->FileName,
                    sizeof(Dirent->FileName));
            }
        }
		while (++Entry < BeyoudLastEntry);
       /*
        * Check if this is the last available entry.
        */
        if (PageValidLength < PAGE_SIZE)
            break;
       /*
        * We are getting beyound current page and
        * are still in the continous run, map the next page.
        */
        Context->PageOffset.QuadPart = PageOffset;
        CcUnpinData(Context->PageBcb);
        if (!CcMapData(Context->FileObject,
            &Context->PageOffset, PAGE_SIZE, CanWait,
            &Context->PageBcb, &Context->PageBuffer))
        {
            Context->PageBcb = NULL;
            Context->PageOffset.QuadPart = 0LL;
            ExRaiseStatus(STATUS_CANT_WAIT);
        }
        Entry = (PUCHAR) Context->PageBuffer;
       /*
        * Next Page Offset.
        */
        PageOffset = Context->PageOffset.QuadPart + PAGE_SIZE;
        if (PageOffset > Context->BeyoundLastEntryOffset)
            PageValidLength = (SIZE_T) (Context->BeyoundLastEntryOffset
                - Context->PageOffset.QuadPart);
            BeyoudLastEntry = Add2Ptr(Context->PageBuffer, PageValidLength, PUCHAR);
    }
    return 0;
}

/* EOF */
