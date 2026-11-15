/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        boot/freeldr/freeldr/lib/fs/ntfs.c
 * PURPOSE:     FreeLoader NTFS support
 * COPYRIGHT:   Copyright (C) 2004 Filip Navara <xnavara@volny.cz>
 *              Copyright (C) 2009-2010 Hervé Poussineau
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * Limitations:
 * - No support for compressed files.
 * - May crash on corrupted filesystem.
 */

#ifndef _M_ARM
#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(FILESYSTEM);

#define TAG_NTFS_CONTEXT 'CftN'
#define TAG_NTFS_LIST 'LftN'
#define TAG_NTFS_DIR 'RftN'
#define TAG_NTFS_DIR 'RftN'
#define TAG_NTFS_MFT 'MftN'
#define TAG_NTFS_INDEX_REC 'IftN'
#define TAG_NTFS_BITMAP 'BftN'
#define TAG_NTFS_FILE 'FftN'
#define TAG_NTFS_VOLUME 'VftN'
#define TAG_NTFS_DATA 'DftN'

#define NTFS_MAX_ATTRIBUTE_LIST_RECURSION 8

typedef struct _NTFS_CACHED_NAME
{
    ULONGLONG MftIndex;
    ULONG Attributes;
    UCHAR NameLength;
    CHAR Name[255];
} NTFS_CACHED_NAME, *PNTFS_CACHED_NAME;

typedef struct _NTFS_VOLUME_INFO
{
    NTFS_BOOTSECTOR BootSector;
    ULONG ClusterSize;
    ULONG MftRecordSize;
    ULONG IndexRecordSize;
    PNTFS_MFT_RECORD MasterFileTable;
    /* Freed with the volume when this device is mounted again. */
    PNTFS_ATTR_CONTEXT MFTContext;
    ULONG DeviceId;
    PUCHAR TemporarySector;
    /* Last directory index, reused for the next file in that directory. */
    BOOLEAN DirectoryCached;
    ULONGLONG CachedDirectory;
    ULONG CachedPathLength;
    CHAR CachedPath[261];
    ULONG CachedNameCount;
    ULONG CachedNameCapacity;
    PNTFS_CACHED_NAME CachedNames;
} NTFS_VOLUME_INFO;

PNTFS_VOLUME_INFO NtfsVolumes[MAX_FDS];

static ULONGLONG NtfsGetAttributeSize(PNTFS_ATTR_RECORD AttrRecord)
{
    if (AttrRecord->IsNonResident)
        return AttrRecord->NonResident.DataSize;
    else
        return AttrRecord->Resident.ValueLength;
}

static PUCHAR NtfsDecodeRun(PUCHAR DataRun, LONGLONG *DataRunOffset, ULONGLONG *DataRunLength)
{
    UCHAR DataRunOffsetSize;
    UCHAR DataRunLengthSize;
    CHAR i;

    DataRunOffsetSize = (*DataRun >> 4) & 0xF;
    DataRunLengthSize = *DataRun & 0xF;
    *DataRunOffset = 0;
    *DataRunLength = 0;
    DataRun++;
    for (i = 0; i < DataRunLengthSize; i++)
    {
        *DataRunLength += ((ULONG64)*DataRun) << (i * 8);
        DataRun++;
    }

    /* NTFS 3+ sparse files */
    if (DataRunOffsetSize == 0)
    {
        *DataRunOffset = -1;
    }
    else
    {
        for (i = 0; i < DataRunOffsetSize - 1; i++)
        {
            *DataRunOffset += ((ULONG64)*DataRun) << (i * 8);
            DataRun++;
        }
        /* The last byte contains sign so we must process it different way. */
        *DataRunOffset = ((LONG64)(CHAR)(*(DataRun++)) << (i * 8)) + *DataRunOffset;
    }

    TRACE("DataRunOffsetSize: %x\n", DataRunOffsetSize);
    TRACE("DataRunLengthSize: %x\n", DataRunLengthSize);
    TRACE("DataRunOffset: %x\n", *DataRunOffset);
    TRACE("DataRunLength: %x\n", *DataRunLength);

    return DataRun;
}

static PNTFS_ATTR_CONTEXT NtfsPrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord)
{
    PNTFS_ATTR_CONTEXT Context;

    Context = FrLdrTempAlloc(FIELD_OFFSET(NTFS_ATTR_CONTEXT, Record) + AttrRecord->Length,
                             TAG_NTFS_CONTEXT);
    RtlCopyMemory(&Context->Record, AttrRecord, AttrRecord->Length);
    if (AttrRecord->IsNonResident)
    {
        /* Length 0 means the run cache has not been filled yet. Decoding the
         * first run here left CacheRun pointing at the next run, so a later
         * hit treated the second run as the first. */
        Context->CacheRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
        Context->CacheRunOffset = 0;
        Context->CacheRunLength = 0;
        Context->CacheRunStartLCN = 0;
        Context->CacheRunLastLCN = 0;
        Context->CacheRunCurrentOffset = 0;
    }

    return Context;
}

static VOID NtfsReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context)
{
    FrLdrTempFree(Context, TAG_NTFS_CONTEXT);
}

static BOOLEAN NtfsDiskRead(PNTFS_VOLUME_INFO Volume, ULONGLONG Offset, ULONGLONG Length, PCHAR Buffer)
{
    LARGE_INTEGER Position;
    ULONG Count;
    ULONG ReadLength;
    ARC_STATUS Status;

    TRACE("NtfsDiskRead - Offset: %I64u Length: %I64u\n", Offset, Length);

    //
    // I. Read partial first sector if needed
    //
    if (Offset % Volume->BootSector.BytesPerSector)
    {
        Position.QuadPart = Offset & ~(Volume->BootSector.BytesPerSector - 1);
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcRead(Volume->DeviceId, Volume->TemporarySector, Volume->BootSector.BytesPerSector, &Count);
        if (Status != ESUCCESS || Count != Volume->BootSector.BytesPerSector)
            return FALSE;
        ReadLength = (USHORT)min(Length, Volume->BootSector.BytesPerSector - (Offset % Volume->BootSector.BytesPerSector));

        //
        // Copy interesting data
        //
        RtlCopyMemory(Buffer,
                      &Volume->TemporarySector[Offset % Volume->BootSector.BytesPerSector],
                      ReadLength);

        //
        // Move to unfilled buffer part
        //
        Buffer += ReadLength;
        Length -= ReadLength;
        Offset += ReadLength;
    }

    //
    // II. Read all complete blocks
    //
    if (Length >= Volume->BootSector.BytesPerSector)
    {
        Position.QuadPart = Offset;
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        ReadLength = Length & ~(Volume->BootSector.BytesPerSector - 1);
        Status = ArcRead(Volume->DeviceId, Buffer, ReadLength, &Count);
        if (Status != ESUCCESS || Count != ReadLength)
            return FALSE;

        //
        // Move to unfilled buffer part
        //
        Buffer += ReadLength;
        Length -= ReadLength;
        Offset += ReadLength;
    }

    //
    // III. Read the rest of data
    //
    if (Length)
    {
        Position.QuadPart = Offset;
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcRead(Volume->DeviceId, Buffer, (ULONG)Length, &Count);
        if (Status != ESUCCESS || Count != Length)
            return FALSE;
    }

    return TRUE;
}

static BOOLEAN NtfsDiskWrite(PNTFS_VOLUME_INFO Volume, ULONGLONG Offset, ULONGLONG Length, PCHAR Buffer)
{
    LARGE_INTEGER Position;
    ULONG Count;
    ULONG WriteLength;
    ARC_STATUS Status;
    USHORT SectorSize = Volume->BootSector.BytesPerSector;

    if (Offset % SectorSize)
    {
        Position.QuadPart = Offset & ~(SectorSize - 1);
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcRead(Volume->DeviceId, Volume->TemporarySector, SectorSize, &Count);
        if (Status != ESUCCESS || Count != SectorSize)
            return FALSE;
        WriteLength = (USHORT)min(Length, SectorSize - (Offset % SectorSize));
        RtlCopyMemory(&Volume->TemporarySector[Offset % SectorSize], Buffer, WriteLength);
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcWrite(Volume->DeviceId, Volume->TemporarySector, SectorSize, &Count);
        if (Status != ESUCCESS || Count != SectorSize)
            return FALSE;
        Buffer += WriteLength;
        Length -= WriteLength;
        Offset += WriteLength;
    }

    if (Length >= SectorSize)
    {
        Position.QuadPart = Offset;
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        WriteLength = Length & ~(SectorSize - 1);
        Status = ArcWrite(Volume->DeviceId, Buffer, WriteLength, &Count);
        if (Status != ESUCCESS || Count != WriteLength)
            return FALSE;
        Buffer += WriteLength;
        Length -= WriteLength;
        Offset += WriteLength;
    }

    if (Length)
    {
        Position.QuadPart = Offset;
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcRead(Volume->DeviceId, Volume->TemporarySector, SectorSize, &Count);
        if (Status != ESUCCESS || Count != SectorSize)
            return FALSE;
        RtlCopyMemory(Volume->TemporarySector, Buffer, (ULONG)Length);
        Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (Status != ESUCCESS)
            return FALSE;
        Status = ArcWrite(Volume->DeviceId, Volume->TemporarySector, SectorSize, &Count);
        if (Status != ESUCCESS || Count != SectorSize)
            return FALSE;
    }

    return TRUE;
}

static ULONG NtfsReadAttribute(PNTFS_VOLUME_INFO Volume, PNTFS_ATTR_CONTEXT Context, ULONGLONG Offset, PCHAR Buffer, ULONG Length)
{
    ULONGLONG LastLCN;
    PUCHAR DataRun;
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG DataRunStartLCN;
    ULONGLONG CurrentOffset;
    ULONG ReadLength;
    ULONG AlreadyRead;

    if (!Context->Record.IsNonResident)
    {
        if (Offset > Context->Record.Resident.ValueLength)
            return 0;
        if (Offset + Length > Context->Record.Resident.ValueLength)
            Length = (ULONG)(Context->Record.Resident.ValueLength - Offset);
        RtlCopyMemory(Buffer, (PCHAR)&Context->Record + Context->Record.Resident.ValueOffset + Offset, Length);
        return Length;
    }

    /*
     * Non-resident attribute
     */

    /*
     * I. Find the corresponding start data run.
     */

    AlreadyRead = 0;

    /* CacheRunOffset is the file offset where the cached run begins, and
     * CacheRun points at the encoded run that follows it. */
    if (Context->CacheRunLength != 0 &&
        Context->CacheRunOffset <= Offset &&
        Offset < Context->CacheRunOffset + Context->CacheRunLength * Volume->ClusterSize)
    {
        DataRun = Context->CacheRun;
        LastLCN = Context->CacheRunLastLCN;
        DataRunStartLCN = Context->CacheRunStartLCN;
        DataRunLength = Context->CacheRunLength;
        CurrentOffset = Context->CacheRunCurrentOffset;
    }
    else
    {
        LastLCN = 0;
        DataRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
        CurrentOffset = 0;

        while (1)
        {
            DataRun = NtfsDecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunOffset != -1)
            {
                /* Normal data run. */
                DataRunStartLCN = LastLCN + DataRunOffset;
                LastLCN = DataRunStartLCN;
            }
            else
            {
                /* Sparse data run. */
                DataRunStartLCN = -1;
            }

            if (Offset >= CurrentOffset &&
                Offset < CurrentOffset + (DataRunLength * Volume->ClusterSize))
            {
                /* Remember this run, not the position after the read. */
                Context->CacheRun = DataRun;
                Context->CacheRunOffset = CurrentOffset;
                Context->CacheRunLength = DataRunLength;
                Context->CacheRunStartLCN = DataRunStartLCN;
                Context->CacheRunLastLCN = LastLCN;
                Context->CacheRunCurrentOffset = CurrentOffset;
                break;
            }

            if (*DataRun == 0)
            {
                return AlreadyRead;
            }

            CurrentOffset += DataRunLength * Volume->ClusterSize;
        }
    }

    /*
     * II. Go through the run list and read the data
     */

    ReadLength = (ULONG)min(DataRunLength * Volume->ClusterSize - (Offset - CurrentOffset), Length);
    if (DataRunStartLCN == -1)
        RtlZeroMemory(Buffer, ReadLength);
    if (DataRunStartLCN == -1 || NtfsDiskRead(Volume, DataRunStartLCN * Volume->ClusterSize + Offset - CurrentOffset, ReadLength, Buffer))
    {
        Length -= ReadLength;
        Buffer += ReadLength;
        AlreadyRead += ReadLength;

        if (ReadLength == DataRunLength * Volume->ClusterSize - (Offset - CurrentOffset))
        {
            CurrentOffset += DataRunLength * Volume->ClusterSize;
            DataRun = NtfsDecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunOffset != (ULONGLONG)-1)
            {
                DataRunStartLCN = LastLCN + DataRunOffset;
                LastLCN = DataRunStartLCN;
            }
            else
                DataRunStartLCN = -1;
        }

        while (Length > 0)
        {
            ReadLength = (ULONG)min(DataRunLength * Volume->ClusterSize, Length);
            if (DataRunStartLCN == -1)
                RtlZeroMemory(Buffer, ReadLength);
            else if (!NtfsDiskRead(Volume, DataRunStartLCN * Volume->ClusterSize, ReadLength, Buffer))
                break;

            Length -= ReadLength;
            Buffer += ReadLength;
            AlreadyRead += ReadLength;

            /* We finished this request, but there still data in this data run. */
            if (Length == 0 && ReadLength != DataRunLength * Volume->ClusterSize)
                break;

            /*
             * Go to next run in the list.
             */

            if (*DataRun == 0)
                break;
            CurrentOffset += DataRunLength * Volume->ClusterSize;
            DataRun = NtfsDecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunOffset != -1)
            {
                /* Normal data run. */
                DataRunStartLCN = LastLCN + DataRunOffset;
                LastLCN = DataRunStartLCN;
            }
            else
            {
                /* Sparse data run. */
                DataRunStartLCN = -1;
            }
        } /* while */

    } /* if Disk */

    return AlreadyRead;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttributeHelper(
    PNTFS_VOLUME_INFO Volume,
    ULONGLONG CurrentMftIndex,
    PNTFS_ATTR_RECORD AttrRecord,
    PNTFS_ATTR_RECORD AttrRecordEnd,
    ULONG Type,
    const WCHAR *Name,
    ULONG NameLength,
    ULONG RecursionLimit,
    ULONG Instance);
static BOOLEAN NtfsReadMftRecord(PNTFS_VOLUME_INFO Volume, ULONGLONG MFTIndex, PNTFS_MFT_RECORD Buffer);

static PNTFS_ATTR_CONTEXT NtfsFindAttributeHelperList(
    PNTFS_VOLUME_INFO Volume,
    ULONGLONG ParentMftIndex,
    PNTFS_ATTR_LIST_ATTR AttrListRecord,
    PNTFS_ATTR_LIST_ATTR AttrListRecordEnd,
    ULONG Type,
    const WCHAR *Name,
    ULONG NameLength,
    ULONG RecursionLimit)
{
    ULONGLONG PrevMftIndex = -1;
    PNTFS_ATTR_CONTEXT Context = NULL;
    PNTFS_MFT_RECORD MftRecord;
    if (RecursionLimit < 1)
        return NULL;

    MftRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (!MftRecord)
        return NULL;

    while (AttrListRecord < AttrListRecordEnd)
    {
        ULONGLONG MftIndex = AttrListRecord->BaseFileRef & NTFS_MFT_MASK;
        ULONG AttrType = AttrListRecord->Type;
        ULONG AttrId = AttrListRecord->AttrId;

        if (AttrType == NTFS_ATTR_TYPE_END)
            break;

        TRACE("RecursionLimit = %u, AttrType = 0x%x, MftIndex = %I64u\n", RecursionLimit, AttrType, MftIndex);

        if (MftIndex == ParentMftIndex)
        {
            TRACE("Skipping unnecessary recursion level!\n");
            goto skip;
        }

        if (AttrType == Type &&
            AttrListRecord->NameLength == NameLength)
        {
            PWCHAR AttrListName;

            AttrListName = (PWCHAR)((PCHAR)AttrListRecord + AttrListRecord->NameOffset);
            if (RtlEqualMemory(AttrListName, Name, NameLength * sizeof(WCHAR)))
            {
                PNTFS_ATTR_RECORD AttrRecord;
                PNTFS_ATTR_RECORD AttrRecordEnd;

                if (PrevMftIndex != MftIndex)
                {
                    PrevMftIndex = MftIndex;
                    if (!NtfsReadMftRecord(Volume, MftIndex, MftRecord))
                        goto skip;
                }

                AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + MftRecord->AttributesOffset);
                AttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + Volume->MftRecordSize);

                Context = NtfsFindAttributeHelper(Volume, MftIndex,
                                                  AttrRecord, AttrRecordEnd,
                                                  Type, Name, NameLength,
                                                  RecursionLimit - 1, AttrId);
                if (Context)
                    break;
            }
        }

skip:
        if (AttrListRecord->RecLength == 0)
            break;
        AttrListRecord = (PNTFS_ATTR_LIST_ATTR)((PCHAR)AttrListRecord + AttrListRecord->RecLength);
    }

    if (MftRecord)
        FrLdrTempFree(MftRecord, TAG_NTFS_MFT);

    return Context;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttributeHelper(
    PNTFS_VOLUME_INFO Volume,
    ULONGLONG CurrentMftIndex,
    PNTFS_ATTR_RECORD AttrRecord,
    PNTFS_ATTR_RECORD AttrRecordEnd,
    ULONG Type,
    const WCHAR *Name,
    ULONG NameLength,
    ULONG RecursionLimit,
    ULONG Instance)
{
    PNTFS_ATTR_CONTEXT Context = NULL;
    PNTFS_ATTR_CONTEXT ListContext = NULL;
    PVOID ListBuffer = NULL;
    ULONGLONG ListSize = 0;
    if (RecursionLimit < 1)
        return NULL;

    while (AttrRecord < AttrRecordEnd)
    {
        ULONG AttrType = AttrRecord->Type;
        ULONG AttrInstance = AttrRecord->Instance;

        if (AttrType == NTFS_ATTR_TYPE_END)
            break;

        TRACE("RecursionLimit = %u, AttrType = 0x%x\n", RecursionLimit, AttrType);

        if (ListContext)
            NtfsReleaseAttributeContext(ListContext);
        if (ListBuffer)
            FrLdrTempFree(ListBuffer, TAG_NTFS_LIST);

        ListContext = ListBuffer = NULL;

        /* Limit the $ATTRIBUTE_LIST recursion or else infinity loop */
        if (AttrType != Type && AttrType == NTFS_ATTR_TYPE_ATTRIBUTE_LIST && RecursionLimit >= 2)
        {
            PNTFS_ATTR_LIST_ATTR ListAttrRecord;
            PNTFS_ATTR_LIST_ATTR ListAttrRecordEnd;

            ListContext = NtfsPrepareAttributeContext(AttrRecord);

            ListSize = NtfsGetAttributeSize(&ListContext->Record);
            if (ListSize <= 0xFFFFFFFF)
                ListBuffer = FrLdrTempAlloc((ULONG)ListSize, TAG_NTFS_LIST);
            else
                ListBuffer = NULL;

            if (!ListBuffer)
            {
                TRACE("Failed to allocate memory: %x\n", (ULONG)ListSize);
                goto skip;
            }

            ListAttrRecord = (PNTFS_ATTR_LIST_ATTR)ListBuffer;
            ListAttrRecordEnd = (PNTFS_ATTR_LIST_ATTR)((PCHAR)ListBuffer + ListSize);

            if (NtfsReadAttribute(Volume, ListContext, 0, ListBuffer, (ULONG)ListSize) == ListSize)
            {
                Context = NtfsFindAttributeHelperList(Volume, CurrentMftIndex,
                                                      ListAttrRecord, ListAttrRecordEnd,
                                                      Type, Name, NameLength,
                                                      RecursionLimit - 1);

                if (Context != NULL)
                    break;
            }
        }

        if (AttrType == Type &&
            AttrRecord->NameLength == NameLength &&
            /* HACK for ntfs3 driver on linux! Because the ntfs3 likes to generate invalid
             * attribute size when mft record increases and I don't know how I will handle this */
            NtfsGetAttributeSize(AttrRecord) != 0 &&
            (Instance == (ULONG)-1 || AttrInstance == Instance))
        {
            PWCHAR AttrName;

            AttrName = (PWCHAR)((PCHAR)AttrRecord + AttrRecord->NameOffset);
            if (RtlEqualMemory(AttrName, Name, NameLength * sizeof(WCHAR)))
            {
                /* Found it, fill up the context and return */
                Context = NtfsPrepareAttributeContext(AttrRecord);
                break;
            }
        }

skip:
        if (AttrRecord->Length == 0)
            break;
        AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)AttrRecord + AttrRecord->Length);
    }

    if (ListContext)
        NtfsReleaseAttributeContext(ListContext);
    if (ListBuffer)
        FrLdrTempFree(ListBuffer, TAG_NTFS_LIST);

    return Context;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttribute(PNTFS_VOLUME_INFO Volume, PNTFS_MFT_RECORD MftRecord, ULONGLONG MftIndex, ULONG Type, const WCHAR *Name)
{
    PNTFS_ATTR_RECORD AttrRecord;
    PNTFS_ATTR_RECORD AttrRecordEnd;
    ULONG NameLength;

    AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + MftRecord->AttributesOffset);
    AttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + Volume->MftRecordSize);
    for (NameLength = 0; Name[NameLength] != 0; NameLength++)
        ;

    return NtfsFindAttributeHelper(Volume, MftIndex, AttrRecord, AttrRecordEnd, Type, Name, NameLength, NTFS_MAX_ATTRIBUTE_LIST_RECURSION, -1);
}

static BOOLEAN NtfsFixupRecord(PNTFS_VOLUME_INFO Volume, PNTFS_RECORD Record)
{
    USHORT *USA;
    USHORT USANumber;
    USHORT USACount;
    USHORT *Block;

    USA = (USHORT*)((PCHAR)Record + Record->USAOffset);
    USANumber = *(USA++);
    USACount = Record->USACount - 1; /* Exclude the USA Number. */
    Block = (USHORT*)((PCHAR)Record + Volume->BootSector.BytesPerSector - 2);

    while (USACount)
    {
        if (*Block != USANumber)
            return FALSE;
        *Block = *(USA++);
        Block = (USHORT*)((PCHAR)Block + Volume->BootSector.BytesPerSector);
        USACount--;
    }

    return TRUE;
}

static ULONG NtfsExchangeCode;

static BOOLEAN NtfsReadMftRecord(PNTFS_VOLUME_INFO Volume, ULONGLONG MFTIndex, PNTFS_MFT_RECORD Buffer)
{
    ULONGLONG BytesRead;

    /* The run cache is keyed by file offset. A stale entry makes a later
     * record read return short, so decode the MFT run list every time. */
    Volume->MFTContext->CacheRunLength = 0;
    BytesRead = NtfsReadAttribute(Volume, Volume->MFTContext, MFTIndex * Volume->MftRecordSize, (PCHAR)Buffer, Volume->MftRecordSize);
    if (BytesRead != Volume->MftRecordSize)
    {
        /* 8: short read. Bits 23-12 are the record, bits 11-0 are the bytes. */
        NtfsExchangeCode = 0x08000000 |
                           (((ULONG)MFTIndex & 0xFFF) << 12) |
                           ((ULONG)BytesRead & 0xFFF);
        return FALSE;
    }

    /* Apply update sequence array fixups. */
    if (!NtfsFixupRecord(Volume, (PNTFS_RECORD)Buffer))
    {
        /* 9: the record was the right length and its update sequence did not match. */
        NtfsExchangeCode = 0x09000000 | ((ULONG)MFTIndex & 0xFFFFFF);
        return FALSE;
    }
    return TRUE;
}

#if DBG
VOID NtfsPrintFile(PNTFS_INDEX_ENTRY IndexEntry)
{
    PWCHAR FileName;
    UCHAR FileNameLength;
    CHAR AnsiFileName[256];
    UCHAR i;

    FileName = IndexEntry->FileName.FileName;
    FileNameLength = min(IndexEntry->FileName.FileNameLength, 255);

    for (i = 0; i < FileNameLength; i++)
        AnsiFileName[i] = (CHAR)FileName[i];
    AnsiFileName[i] = 0;

    TRACE("- %s (%x)\n", AnsiFileName, (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK));
}
#endif

static BOOLEAN
NtfsCompareFileName(
    _In_ PCCH FileName,
    _In_ SIZE_T FileNameLen,
    _In_ PNTFS_INDEX_ENTRY IndexEntry)
{
    PWCHAR EntryFileName;
    UCHAR EntryFileNameLength;
    UCHAR i;

    EntryFileName = IndexEntry->FileName.FileName;
    EntryFileNameLength = IndexEntry->FileName.FileNameLength;

#if DBG
    TRACE("%s ", FileName);
    NtfsPrintFile(IndexEntry);
#endif

    if (FileNameLen != EntryFileNameLength)
        return FALSE;

    /*
     * Always perform case-insensitive comparison for file names.
     * This is necessary, because when modifying e.g. on Linux a Windows NTFS
     * partition formatted with Windows itself, the NTLDR/BOOTMGR will boot
     * normally ignoring the case of the paths.
     */
    for (i = 0; i < EntryFileNameLength; i++)
    {
        if (tolower(EntryFileName[i]) != tolower(FileName[i]))
            return FALSE;
    }

    return TRUE;
}

static VOID
NtfsDiscardDirectoryCache(PNTFS_VOLUME_INFO Volume)
{
    if (Volume->CachedNames)
        FrLdrTempFree(Volume->CachedNames, TAG_NTFS_DIR);
    Volume->CachedNames = NULL;
    Volume->CachedNameCount = 0;
    Volume->CachedNameCapacity = 0;
    Volume->DirectoryCached = FALSE;
    Volume->CachedPathLength = 0;
}

static VOID
NtfsRememberDirectoryEntry(PNTFS_VOLUME_INFO Volume, PNTFS_INDEX_ENTRY IndexEntry)
{
    PNTFS_CACHED_NAME Entry;
    ULONG i;
    ULONG Length;

    Length = IndexEntry->FileName.FileNameLength;
    if (Length == 0 || Length >= 255)
        return;

    if (Volume->CachedNameCount == Volume->CachedNameCapacity)
    {
        ULONG Capacity = Volume->CachedNameCapacity ? Volume->CachedNameCapacity * 2 : 64;
        PNTFS_CACHED_NAME Names;

        Names = FrLdrTempAlloc(Capacity * sizeof(NTFS_CACHED_NAME), TAG_NTFS_DIR);
        if (Names == NULL)
            return;
        if (Volume->CachedNames && Volume->CachedNameCount != 0)
        {
            RtlCopyMemory(Names,
                          Volume->CachedNames,
                          Volume->CachedNameCount * sizeof(NTFS_CACHED_NAME));
            FrLdrTempFree(Volume->CachedNames, TAG_NTFS_DIR);
        }
        Volume->CachedNames = Names;
        Volume->CachedNameCapacity = Capacity;
    }

    Entry = &Volume->CachedNames[Volume->CachedNameCount];
    Entry->MftIndex = IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK;
    Entry->Attributes = IndexEntry->FileName.FileAttributes;
    Entry->NameLength = (UCHAR)Length;
    for (i = 0; i < Length; i++)
        Entry->Name[i] = (CHAR)IndexEntry->FileName.FileName[i];
    Volume->CachedNameCount++;
}

static BOOLEAN
NtfsFindCachedName(
    PNTFS_VOLUME_INFO Volume,
    PCSTR FileName,
    SIZE_T FileNameLen,
    PULONGLONG OutMFTIndex,
    PULONG FileAttributes)
{
    ULONG i;

    for (i = 0; i < Volume->CachedNameCount; i++)
    {
        PNTFS_CACHED_NAME Entry = &Volume->CachedNames[i];
        ULONG n;

        if (Entry->NameLength != FileNameLen)
            continue;
        for (n = 0; n < FileNameLen; n++)
        {
            if (tolower((UCHAR)Entry->Name[n]) != tolower((UCHAR)FileName[n]))
                break;
        }
        if (n != FileNameLen)
            continue;

        *OutMFTIndex = Entry->MftIndex;
        *FileAttributes = Entry->Attributes;
        return TRUE;
    }

    return FALSE;
}

static BOOLEAN
NtfsFindMftRecord(
    _In_ PNTFS_VOLUME_INFO Volume,
    _In_ ULONGLONG MFTIndex,
    _In_ PCSTR FileName,
    _Out_ PULONGLONG OutMFTIndex,
    _Out_ PULONG FileAttributes)
{
    PNTFS_MFT_RECORD MftRecord;
    //ULONG Magic;
    PNTFS_ATTR_CONTEXT IndexRootCtx;
    PNTFS_ATTR_CONTEXT IndexBitmapCtx;
    PNTFS_ATTR_CONTEXT IndexAllocationCtx;
    PNTFS_INDEX_ROOT IndexRoot;
    ULONGLONG BitmapDataSize;
    ULONGLONG IndexAllocationSize;
    PCHAR BitmapData;
    PCHAR IndexRecord;
    PNTFS_INDEX_ENTRY IndexEntry, IndexEntryEnd;
    ULONG RecordOffset;
    ULONG IndexBlockSize;
    SIZE_T FileNameLen;
    BOOLEAN Found = FALSE;
    BOOLEAN ScanComplete = FALSE;
    BOOLEAN FixupFailed = FALSE;
    BOOLEAN ReadFailed = FALSE;

    FileNameLen = strlen(FileName);

    if (Volume->DirectoryCached && Volume->CachedDirectory == MFTIndex &&
        NtfsFindCachedName(Volume, FileName, FileNameLen, OutMFTIndex, FileAttributes))
    {
        return TRUE;
    }

    NtfsDiscardDirectoryCache(Volume);

    MftRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (MftRecord == NULL)
    {
        NtfsExchangeCode = 1;
        return FALSE;
    }

    if (NtfsReadMftRecord(Volume, MFTIndex, MftRecord))
    {
        //Magic = MftRecord->Magic;

        IndexRootCtx = NtfsFindAttribute(Volume, MftRecord, MFTIndex, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
        if (IndexRootCtx == NULL)
        {
            NtfsExchangeCode = 2;
            FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
            return FALSE;
        }

        IndexRecord = FrLdrTempAlloc(Volume->IndexRecordSize, TAG_NTFS_INDEX_REC);
        if (IndexRecord == NULL)
        {
            NtfsExchangeCode = 3;
            FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
            return FALSE;
        }

        NtfsReadAttribute(Volume, IndexRootCtx, 0, IndexRecord, Volume->IndexRecordSize);
        IndexRoot = (PNTFS_INDEX_ROOT)IndexRecord;
        IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)&IndexRoot->IndexHeader + IndexRoot->IndexHeader.EntriesOffset);
        /* Index root is always resident. */
        IndexEntryEnd = (PNTFS_INDEX_ENTRY)(IndexRecord + IndexRootCtx->Record.Resident.ValueLength);
        NtfsReleaseAttributeContext(IndexRootCtx);

        TRACE("IndexRecordSize: %x IndexBlockSize: %x\n", Volume->IndexRecordSize, IndexRoot->IndexBlockSize);

        while (IndexEntry < IndexEntryEnd &&
               !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
        {
            NtfsRememberDirectoryEntry(Volume, IndexEntry);
            if (!Found && NtfsCompareFileName(FileName, FileNameLen, IndexEntry))
            {
                *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
                *FileAttributes = IndexEntry->FileName.FileAttributes;
                Found = TRUE;
            }
            IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
        }

        if (IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX)
        {
            TRACE("Large Index!\n");

            IndexBlockSize = IndexRoot->IndexBlockSize;

            IndexBitmapCtx = NtfsFindAttribute(Volume, MftRecord, MFTIndex, NTFS_ATTR_TYPE_BITMAP, L"$I30");
            if (IndexBitmapCtx == NULL)
            {
                NtfsExchangeCode = 4;
                TRACE("Corrupted filesystem!\n");
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return FALSE;
            }
            BitmapDataSize = NtfsGetAttributeSize(&IndexBitmapCtx->Record);
            TRACE("BitmapDataSize: %x\n", (ULONG)BitmapDataSize);
            if(BitmapDataSize <= 0xFFFFFFFF)
                BitmapData = FrLdrTempAlloc((ULONG)BitmapDataSize, TAG_NTFS_BITMAP);
            else
                BitmapData = NULL;

            if (BitmapData == NULL)
            {
                NtfsExchangeCode = 5;
                FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return FALSE;
            }
            NtfsReadAttribute(Volume, IndexBitmapCtx, 0, BitmapData, (ULONG)BitmapDataSize);
            NtfsReleaseAttributeContext(IndexBitmapCtx);

            IndexAllocationCtx = NtfsFindAttribute(Volume, MftRecord, MFTIndex, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
            if (IndexAllocationCtx == NULL)
            {
                NtfsExchangeCode = 6;
                TRACE("Corrupted filesystem!\n");
                FrLdrTempFree(BitmapData, TAG_NTFS_BITMAP);
                FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return FALSE;
            }
            IndexAllocationSize = NtfsGetAttributeSize(&IndexAllocationCtx->Record);

            RecordOffset = 0;
            ScanComplete = TRUE;

            for (;;)
            {
                TRACE("RecordOffset: %x IndexAllocationSize: %x\n", RecordOffset, IndexAllocationSize);
                for (; RecordOffset < IndexAllocationSize;)
                {
                    UCHAR Bit = 1 << ((RecordOffset / IndexBlockSize) & 7);
                    ULONG Byte = (RecordOffset / IndexBlockSize) >> 3;
                    if ((BitmapData[Byte] & Bit))
                        break;
                    RecordOffset += IndexBlockSize;
                }

                if (RecordOffset >= IndexAllocationSize)
                {
                    break;
                }

                NtfsReadAttribute(Volume, IndexAllocationCtx, RecordOffset, IndexRecord, IndexBlockSize);

                if (!NtfsFixupRecord(Volume, (PNTFS_RECORD)IndexRecord))
                {
                    FixupFailed = TRUE;
                    ScanComplete = FALSE;
                    break;
                }

                /* FIXME */
                IndexEntry = (PNTFS_INDEX_ENTRY)(IndexRecord + 0x18 + *(USHORT *)(IndexRecord + 0x18));
                IndexEntryEnd = (PNTFS_INDEX_ENTRY)(IndexRecord + IndexBlockSize);

                while (IndexEntry < IndexEntryEnd &&
                       !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
                {
                    NtfsRememberDirectoryEntry(Volume, IndexEntry);
                    if (!Found && NtfsCompareFileName(FileName, FileNameLen, IndexEntry))
                    {
                        TRACE("File found\n");
                        *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
                        *FileAttributes = IndexEntry->FileName.FileAttributes;
                        Found = TRUE;
                    }
                    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
                }

                RecordOffset += IndexBlockSize;
            }

            NtfsReleaseAttributeContext(IndexAllocationCtx);
            FrLdrTempFree(BitmapData, TAG_NTFS_BITMAP);
        }
        else
        {
            ScanComplete = TRUE;
        }

        FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
    }
    else
    {
        ReadFailed = TRUE;
        TRACE("Can't read MFT record\n");
    }
    FrLdrTempFree(MftRecord, TAG_NTFS_MFT);

    if (ScanComplete)
    {
        Volume->CachedDirectory = MFTIndex;
        Volume->DirectoryCached = TRUE;
    }
    else
    {
        NtfsDiscardDirectoryCache(Volume);
    }

    if (!Found && FixupFailed)
        NtfsExchangeCode = 7;
    else if (!Found && !ReadFailed)
    {
        /* High byte is the name we failed to find. Low word is how many
         * directory entries the scan kept. */
        NtfsExchangeCode = ((ULONG)(UCHAR)FileName[0] << 24) |
                           ((ULONG)FileNameLen << 16) |
                           (Volume->CachedNameCount & 0xFFFF);
    }
    return Found;
}

static BOOLEAN NtfsLookupFile(PNTFS_VOLUME_INFO Volume, PCSTR FileName, PNTFS_MFT_RECORD MftRecord, PNTFS_FILE_HANDLE FileHandle)
{
    ULONG NumberOfPathParts;
    ULONG i;
    ULONGLONG CurrentMFTIndex;
    ULONG FileAttributes;
    CHAR PathPart[261];
    PCSTR OriginalName = FileName;

    TRACE("NtfsLookupFile() FileName = %s\n", FileName);

    CurrentMFTIndex = NTFS_FILE_ROOT;

    /* Skip leading path separator, if any */
    if (*FileName == '\\' || *FileName == '/')
        ++FileName;
    PathPart[0] = ANSI_NULL;

    /*
     * The previous file left its parent directory in memory. Another file in
     * that directory only needs its own name resolved.
     */
    if (Volume->DirectoryCached && Volume->CachedPathLength != 0)
    {
        ULONG Prefix = Volume->CachedPathLength;
        PCSTR Rest;

        if (_strnicmp(OriginalName, Volume->CachedPath, Prefix) == 0 &&
            (OriginalName[Prefix] == '\\' || OriginalName[Prefix] == '/'))
        {
            Rest = OriginalName + Prefix + 1;
            if (*Rest != '\0' && strchr(Rest, '\\') == NULL && strchr(Rest, '/') == NULL)
            {
            FsGetFirstNameFromPath(PathPart, Rest);
            if (!NtfsFindMftRecord(Volume, Volume->CachedDirectory, PathPart, &CurrentMFTIndex, &FileAttributes))
            {
                TRACE("- Failed\n");
                return FALSE;
            }
            goto ReadFileRecord;
            }
        }
    }

    /* Figure out how many sub-directories we are nested in and loop once for each part */
    NumberOfPathParts = FsGetNumPathParts(FileName);
    for (i = 0; i < NumberOfPathParts; i++)
    {
        FsGetFirstNameFromPath(PathPart, FileName);

        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
            ;
        FileName++;

        TRACE("- Lookup: %s\n", PathPart);
        if (!NtfsFindMftRecord(Volume, CurrentMFTIndex, PathPart, &CurrentMFTIndex, &FileAttributes))
        {
            TRACE("- Failed\n");
            return FALSE;
        }
        TRACE("- Lookup: %x\n", CurrentMFTIndex);
    }

    if (NumberOfPathParts > 1 && Volume->DirectoryCached)
    {
        PCSTR LastSep = strrchr(OriginalName, '\\');
        ULONG Length;

        if (LastSep == NULL)
            LastSep = strrchr(OriginalName, '/');
        if (LastSep != NULL)
        {
            Length = (ULONG)(LastSep - OriginalName);
            if (Length < sizeof(Volume->CachedPath))
            {
                RtlCopyMemory(Volume->CachedPath, OriginalName, Length);
                Volume->CachedPath[Length] = ANSI_NULL;
                Volume->CachedPathLength = Length;
            }
        }
    }

ReadFileRecord:
    if (!NtfsReadMftRecord(Volume, CurrentMFTIndex, MftRecord))
    {
        TRACE("NtfsLookupFile: Can't read MFT record\n");
        return FALSE;
    }

    FileHandle->DataContext = NtfsFindAttribute(Volume, MftRecord, CurrentMFTIndex, NTFS_ATTR_TYPE_DATA, L"");
    if (FileHandle->DataContext == NULL)
    {
        TRACE("NtfsLookupFile: Can't find data attribute\n");
        return FALSE;
    }

    /* Map the attributes to ARC file attributes */
    FileHandle->Attributes = 0;
    if (FileAttributes & NTFS_FILE_ATTR_READONLY)
        FileHandle->Attributes |= ReadOnlyFile;
    if (FileAttributes & NTFS_FILE_ATTR_HIDDEN)
        FileHandle->Attributes |= HiddenFile;
    if (FileAttributes & NTFS_FILE_ATTR_SYSTEM)
        FileHandle->Attributes |= SystemFile;
    if (FileAttributes & NTFS_FILE_ATTR_ARCHIVE)
        FileHandle->Attributes |= ArchiveFile;
    if (FileAttributes & NTFS_FILE_ATTR_DIRECTORY)
        FileHandle->Attributes |= DirectoryFile;

    /* Copy the file name, perhaps truncated */
    FileHandle->FileNameLength = (ULONG)strlen(PathPart);
    FileHandle->FileNameLength = min(FileHandle->FileNameLength, sizeof(FileHandle->FileName) - 1);
    RtlCopyMemory(FileHandle->FileName, PathPart, FileHandle->FileNameLength);

    return TRUE;
}

ARC_STATUS NtfsClose(ULONG FileId)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);

    NtfsReleaseAttributeContext(FileHandle->DataContext);
    FrLdrTempFree(FileHandle, TAG_NTFS_FILE);

    return ESUCCESS;
}

ARC_STATUS NtfsGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(*Information));
    Information->EndingAddress.QuadPart = NtfsGetAttributeSize(&FileHandle->DataContext->Record);
    Information->CurrentAddress.QuadPart = FileHandle->Offset;

    /* Set the ARC file attributes */
    Information->Attributes = FileHandle->Attributes;

    /* Copy the file name, perhaps truncated, and NUL-terminated */
    Information->FileNameLength = min(FileHandle->FileNameLength, sizeof(Information->FileName) - 1);
    RtlCopyMemory(Information->FileName, FileHandle->FileName, Information->FileNameLength);
    Information->FileName[Information->FileNameLength] = ANSI_NULL;

    TRACE("NtfsGetFileInformation(%lu) -> FileSize = %llu, FilePointer = 0x%llx\n",
          FileId, Information->EndingAddress.QuadPart, Information->CurrentAddress.QuadPart);

    return ESUCCESS;
}

ARC_STATUS NtfsOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PNTFS_VOLUME_INFO Volume;
    PNTFS_FILE_HANDLE FileHandle;
    PNTFS_MFT_RECORD MftRecord;
    ULONG DeviceId;

    //
    // Check parameters
    //
    if (OpenMode != OpenReadOnly)
        return EACCES;

    //
    // Get underlying device
    //
    DeviceId = FsGetDeviceId(*FileId);
    Volume = NtfsVolumes[DeviceId];

    TRACE("NtfsOpen() FileName = %s\n", Path);

    //
    // Allocate file structure
    //
    FileHandle = FrLdrTempAlloc(sizeof(NTFS_FILE_HANDLE) + Volume->MftRecordSize,
                                TAG_NTFS_FILE);
    if (!FileHandle)
    {
        return ENOMEM;
    }
    RtlZeroMemory(FileHandle, sizeof(NTFS_FILE_HANDLE) + Volume->MftRecordSize);
    FileHandle->Volume = Volume;

    //
    // Search file entry
    //
    MftRecord = (PNTFS_MFT_RECORD)(FileHandle + 1);
    if (!NtfsLookupFile(Volume, Path, MftRecord, FileHandle))
    {
        FrLdrTempFree(FileHandle, TAG_NTFS_FILE);
        return ENOENT;
    }

    FsSetDeviceSpecific(*FileId, FileHandle);
    return ESUCCESS;
}

ARC_STATUS NtfsRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);
    ULONGLONG BytesRead64;

    /* Read data */
    BytesRead64 = NtfsReadAttribute(FileHandle->Volume, FileHandle->DataContext, FileHandle->Offset, Buffer, N);
    FileHandle->Offset += BytesRead64;
    *Count = (ULONG)BytesRead64;
    if (BytesRead64 > 0)
        return ESUCCESS;
    else
        return EIO;
}

ARC_STATUS NtfsSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += FileHandle->Offset;
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart >= NtfsGetAttributeSize(&FileHandle->DataContext->Record))
        return EINVAL;

    FileHandle->Offset = NewPosition.QuadPart;
    return ESUCCESS;
}


/**
 * @brief
 * Returns the size of the NTFS volume laid on the storage media device
 * opened via @p DeviceId.
 **/
ULONGLONG
NtfsGetVolumeSize(
    _In_ ULONG DeviceId)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumes[DeviceId];
    ASSERT(Volume);
    return Volume->BootSector.VolumeSectorCount * Volume->BootSector.BytesPerSector;
}

#include <pshpack1.h>
typedef struct
{
    NTFS_RECORD Record;
    ULONGLONG Lsn;
    ULONGLONG Vcn;
    NTFS_INDEX_HEADER Header;
} NTFS_INDEX_BLOCK, *PNTFS_INDEX_BLOCK;
#include <poppack.h>

/* Index entries are 8-byte aligned. drivers/filesystems/ntfs/btree.c uses the same alignment. */
#define NTFS_ENTRY_ALIGN 8
#define NTFS_ENTRY_CAP 512

static PNTFS_VOLUME_INFO
NtfsVolumeFromId(ULONG DeviceId)
{
    if (DeviceId >= MAX_FDS)
        return NULL;
    return NtfsVolumes[DeviceId];
}

static VOID
NtfsPrepareFixup(PNTFS_VOLUME_INFO Volume, PNTFS_RECORD Record)
{
    USHORT *USA;
    USHORT USANumber;
    USHORT Count;
    USHORT *Block;
    USHORT Index;

    USA = (USHORT*)((PCHAR)Record + Record->USAOffset);
    USANumber = USA[0];
    Count = Record->USACount - 1;
    Block = (USHORT*)((PCHAR)Record + Volume->BootSector.BytesPerSector - 2);
    for (Index = 0; Index < Count; Index++)
    {
        USA[Index + 1] = *Block;
        *Block = USANumber;
        Block = (USHORT*)((PCHAR)Block + Volume->BootSector.BytesPerSector);
    }
}

static BOOLEAN
NtfsWriteAttribute(
    PNTFS_VOLUME_INFO Volume,
    PNTFS_ATTR_CONTEXT Context,
    ULONGLONG Offset,
    PCHAR Buffer,
    ULONG Length)
{
    ULONG Done = 0;

    if (Context->Record.IsNonResident == FALSE)
        return FALSE;
    Context->CacheRunLength = 0;
    while (Done < Length)
    {
        PUCHAR DataRun;
        LONGLONG LastLCN = 0;
        ULONGLONG Current = 0;
        ULONGLONG Target = Offset + Done;
        BOOLEAN Found = FALSE;

        DataRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
        while (*DataRun != 0)
        {
            LONGLONG Relative;
            ULONGLONG RunLength;
            LONGLONG Lcn;
            ULONGLONG RunEnd;

            DataRun = NtfsDecodeRun(DataRun, &Relative, &RunLength);
            if (Relative != -1)
            {
                Lcn = LastLCN + Relative;
                LastLCN = Lcn;
            }
            else
            {
                Lcn = -1;
            }
            RunEnd = Current + RunLength * Volume->ClusterSize;
            if (Target >= Current && Target < RunEnd)
            {
                ULONG Chunk = (ULONG)min(RunEnd - Target, (ULONGLONG)(Length - Done));
                if (Lcn == -1)
                    return FALSE;
                if (!NtfsDiskWrite(Volume,
                                   Lcn * Volume->ClusterSize + (Target - Current),
                                   Chunk,
                                   Buffer + Done))
                    return FALSE;
                Done += Chunk;
                Found = TRUE;
                break;
            }
            Current = RunEnd;
        }
        if (!Found)
            return FALSE;
    }
    return TRUE;
}

static BOOLEAN
NtfsWriteMftRecord(PNTFS_VOLUME_INFO Volume, ULONGLONG Index, PNTFS_MFT_RECORD Record)
{
    PNTFS_MFT_RECORD Copy;
    BOOLEAN Ok;

    Copy = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (Copy == NULL)
        return FALSE;
    RtlCopyMemory(Copy, Record, Volume->MftRecordSize);
    NtfsPrepareFixup(Volume, (PNTFS_RECORD)Copy);
    Ok = NtfsWriteAttribute(Volume,
                            Volume->MFTContext,
                            Index * Volume->MftRecordSize,
                            (PCHAR)Copy,
                            Volume->MftRecordSize);
    FrLdrTempFree(Copy, TAG_NTFS_MFT);
    return Ok;
}

static BOOLEAN
NtfsLookupMft(PNTFS_VOLUME_INFO Volume, PCSTR Path, PULONGLONG IndexOut)
{
    ULONGLONG Current = NTFS_FILE_ROOT;
    ULONG Attributes;
    CHAR Part[261];
    ULONG Parts;
    ULONG Index;

    if (Path == NULL)
        return FALSE;
    while (*Path == '\\' || *Path == '/')
        Path++;
    if (*Path == '\0')
    {
        *IndexOut = Current;
        return TRUE;
    }

    NtfsDiscardDirectoryCache(Volume);
    Parts = FsGetNumPathParts(Path);
    for (Index = 0; Index < Parts; Index++)
    {
        FsGetFirstNameFromPath(Part, Path);
        while (*Path != '\0' && *Path != '\\' && *Path != '/')
            Path++;
        if (*Path != '\0')
            Path++;
        if (!NtfsFindMftRecord(Volume, Current, Part, &Current, &Attributes))
            return FALSE;
    }
    *IndexOut = Current;
    return TRUE;
}

static BOOLEAN
NtfsSplitPath(PCSTR Path, PCHAR Parent, ULONG ParentSize, PCHAR Name, ULONG NameSize)
{
    PCSTR Slash = strrchr(Path, '\\');
    SIZE_T ParentLen;

    if (Slash == NULL)
        Slash = strrchr(Path, '/');
    if (Slash == NULL || Slash[1] == '\0')
        return FALSE;
    ParentLen = (SIZE_T)(Slash - Path);
    if (ParentLen == 0)
    {
        if (RtlStringCbCopyA(Parent, ParentSize, "\\") != STATUS_SUCCESS)
            return FALSE;
    }
    else
    {
        if (ParentLen >= ParentSize)
            return FALSE;
        RtlCopyMemory(Parent, Path, ParentLen);
        Parent[ParentLen] = ANSI_NULL;
    }
    return RtlStringCbCopyA(Name, NameSize, Slash + 1) == STATUS_SUCCESS;
}

static INT
NtfsOrderEntries(PNTFS_INDEX_ENTRY Left, PNTFS_INDEX_ENTRY Right)
{
    UCHAR Count;
    UCHAR Index;

    Count = min(Left->FileName.FileNameLength, Right->FileName.FileNameLength);
    for (Index = 0; Index < Count; Index++)
    {
        UCHAR First = (UCHAR)tolower((UCHAR)Left->FileName.FileName[Index]);
        UCHAR Second = (UCHAR)tolower((UCHAR)Right->FileName.FileName[Index]);
        if (First != Second)
            return (First < Second) ? -1 : 1;
    }
    if (Left->FileName.FileNameLength == Right->FileName.FileNameLength)
        return 0;
    return (Left->FileName.FileNameLength < Right->FileName.FileNameLength) ? -1 : 1;
}

static PNTFS_INDEX_ENTRY
NtfsIndexEnd(PNTFS_INDEX_HEADER Header, PCHAR Limit)
{
    PNTFS_INDEX_ENTRY Entry;

    Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
    while ((PCHAR)Entry + sizeof(NTFS_INDEX_ENTRY) <= Limit &&
           !(Entry->Flags & NTFS_INDEX_ENTRY_END))
    {
        if (Entry->Length < 16)
            return NULL;
        Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
    }
    if ((PCHAR)Entry >= Limit || Entry->Length < 16)
        return NULL;
    return Entry;
}

static BOOLEAN
NtfsIndexIsLeaf(PNTFS_INDEX_HEADER Header, PCHAR Limit)
{
    PNTFS_INDEX_ENTRY Entry;
    PNTFS_INDEX_ENTRY End;

    End = NtfsIndexEnd(Header, Limit);
    if (End == NULL)
        return FALSE;
    Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
    while (Entry < End)
    {
        if (Entry->Flags & NTFS_INDEX_ENTRY_NODE)
            return FALSE;
        Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
    }
    return TRUE;
}

static BOOLEAN
NtfsIndexInsert(PNTFS_INDEX_HEADER Header, PCHAR Limit, PNTFS_INDEX_ENTRY NewEntry)
{
    PNTFS_INDEX_ENTRY Entry;
    PNTFS_INDEX_ENTRY End;
    ULONG Used;
    ULONG Need;
    ULONG Tail;

    End = NtfsIndexEnd(Header, Limit);
    if (End == NULL || NewEntry->Length < 16)
        return FALSE;
    if (!NtfsIndexIsLeaf(Header, Limit))
        return FALSE;
    Need = NewEntry->Length;
    Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
    while (Entry < End)
    {
        if (NtfsOrderEntries(NewEntry, Entry) < 0)
            break;
        Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
    }
    Used = (ULONG)((PCHAR)End + End->Length - (PCHAR)Header);
    if (Used + Need > Header->AllocatedSize)
        return FALSE;
    if ((PCHAR)Header + Used + Need > Limit)
        return FALSE;
    Tail = (ULONG)((PCHAR)End + End->Length - (PCHAR)Entry);
    RtlMoveMemory((PCHAR)Entry + Need, Entry, Tail);
    RtlCopyMemory(Entry, NewEntry, Need);
    Header->IndexLength = Used + Need;
    return TRUE;
}

static BOOLEAN
NtfsIndexRemove(PNTFS_INDEX_HEADER Header, PCHAR Limit, ULONGLONG FileIndex, PBOOLEAN Removed)
{
    PNTFS_INDEX_ENTRY Entry;
    PNTFS_INDEX_ENTRY End;

    *Removed = FALSE;
    for (;;)
    {
        End = NtfsIndexEnd(Header, Limit);
        if (End == NULL)
            return FALSE;
        Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
        while (Entry < End)
        {
            if ((Entry->Data.Directory.IndexedFile & NTFS_MFT_MASK) == (FileIndex & NTFS_MFT_MASK))
            {
                ULONG Length = Entry->Length;
                ULONG Tail = (ULONG)((PCHAR)End + End->Length - ((PCHAR)Entry + Length));
                ULONG Used;
                RtlMoveMemory(Entry, (PCHAR)Entry + Length, Tail);
                Used = (ULONG)((PCHAR)End + End->Length - (PCHAR)Header);
                if (Used < Length)
                    return FALSE;
                Header->IndexLength = Used - Length;
                *Removed = TRUE;
                break;
            }
            Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
        }
        if (Entry >= End)
            return TRUE;
    }
}

static BOOLEAN
NtfsAttrNameIs(PNTFS_ATTR_RECORD Attr, PCWSTR Name)
{
    PWCHAR Got;
    ULONG Index;

    Got = (PWCHAR)((PCHAR)Attr + Attr->NameOffset);
    for (Index = 0; Name[Index] != 0; Index++)
    {
        if (Index >= Attr->NameLength || Got[Index] != Name[Index])
            return FALSE;
    }
    return Index == Attr->NameLength;
}

static PNTFS_ATTR_RECORD
NtfsResidentNamed(PNTFS_MFT_RECORD Record, ULONG Type, PCWSTR Name)
{
    PNTFS_ATTR_RECORD Attr;
    PCHAR End;

    Attr = (PNTFS_ATTR_RECORD)((PCHAR)Record + Record->AttributesOffset);
    End = (PCHAR)Record + Record->BytesInUse;
    while ((PCHAR)Attr + sizeof(ULONG) < End && Attr->Type != NTFS_ATTR_TYPE_END)
    {
        if (Attr->Length == 0)
            return NULL;
        if (Attr->Type == Type && Attr->IsNonResident == FALSE && NtfsAttrNameIs(Attr, Name))
            return Attr;
        Attr = (PNTFS_ATTR_RECORD)((PCHAR)Attr + Attr->Length);
    }
    return NULL;
}

static BOOLEAN
NtfsCopyEntry(
    PNTFS_INDEX_ENTRY Source,
    ULONGLONG Parent,
    PCSTR NewName,
    PUCHAR Out,
    PULONG OutLength)
{
    PNTFS_INDEX_ENTRY Dest;
    ULONG Length = Source->Length;

    if (Length < 16 || Length > NTFS_ENTRY_CAP)
        return FALSE;
    if (Source->Flags & NTFS_INDEX_ENTRY_NODE)
        return FALSE;
    RtlCopyMemory(Out, Source, Length);
    Dest = (PNTFS_INDEX_ENTRY)Out;
    Dest->FileName.ParentDirectory = Parent;
    if (NewName != NULL)
    {
        SIZE_T NameLength = strlen(NewName);
        ULONG KeyLength;
        ULONG EntryLength;
        ULONG Index;

        if (NameLength == 0 || NameLength > 255)
            return FALSE;
        KeyLength = FIELD_OFFSET(NTFS_FILE_NAME_ATTR, FileName) + (ULONG)NameLength * sizeof(WCHAR);
        EntryLength = (FIELD_OFFSET(NTFS_INDEX_ENTRY, FileName) + KeyLength + (NTFS_ENTRY_ALIGN - 1)) & ~(NTFS_ENTRY_ALIGN - 1);
        if (EntryLength > NTFS_ENTRY_CAP)
            return FALSE;
        if (EntryLength > Length)
            RtlZeroMemory(Out + Length, EntryLength - Length);
        Dest->Length = (USHORT)EntryLength;
        Dest->KeyLength = (USHORT)KeyLength;
        Dest->FileName.FileNameLength = (UCHAR)NameLength;
        for (Index = 0; Index < NameLength; Index++)
            Dest->FileName.FileName[Index] = (WCHAR)(UCHAR)NewName[Index];
        Length = EntryLength;
    }
    *OutLength = Length;
    return TRUE;
}

static BOOLEAN
NtfsSameFileName(PCSTR Name, PNTFS_FILE_NAME_ATTR FileName)
{
    SIZE_T Length = strlen(Name);
    UCHAR Index;

    if (Length != FileName->FileNameLength)
        return FALSE;
    for (Index = 0; Index < FileName->FileNameLength; Index++)
    {
        if (tolower((UCHAR)FileName->FileName[Index]) != tolower((UCHAR)Name[Index]))
            return FALSE;
    }
    return TRUE;
}

static BOOLEAN
NtfsRenameInRecord(PNTFS_MFT_RECORD Record, PCSTR OldName, PCSTR NewName, ULONGLONG Parent, BOOLEAN Apply)
{
    PNTFS_ATTR_RECORD Attr;
    PCHAR End;
    SIZE_T NewLength = NewName == NULL ? 0 : strlen(NewName);

    Attr = (PNTFS_ATTR_RECORD)((PCHAR)Record + Record->AttributesOffset);
    End = (PCHAR)Record + Record->BytesInUse;
    while ((PCHAR)Attr + sizeof(ULONG) < End && Attr->Type != NTFS_ATTR_TYPE_END)
    {
        PNTFS_FILE_NAME_ATTR FileName;
        if (Attr->Length == 0)
            return FALSE;
        if (Attr->Type == NTFS_ATTR_TYPE_FILENAME && Attr->IsNonResident == FALSE)
        {
            FileName = (PNTFS_FILE_NAME_ATTR)((PCHAR)Attr + Attr->Resident.ValueOffset);
            if (NewName != NULL &&
                FileName->FileNameType != NTFS_FILE_NAME_DOS &&
                NtfsSameFileName(OldName, FileName))
            {
                ULONG NewValue = FIELD_OFFSET(NTFS_FILE_NAME_ATTR, FileName) + (ULONG)NewLength * sizeof(WCHAR);
                ULONG NewAttr = (Attr->Resident.ValueOffset + NewValue + (NTFS_ENTRY_ALIGN - 1)) & ~(NTFS_ENTRY_ALIGN - 1);
                LONG Delta = (LONG)NewAttr - (LONG)Attr->Length;
                if ((LONG)Record->BytesInUse + Delta > (LONG)Record->BytesAllocated)
                    return FALSE;
                if (Apply && Delta != 0)
                {
                    PCHAR Next = (PCHAR)Attr + Attr->Length;
                    ULONG Tail = Record->BytesInUse - (ULONG)(Next - (PCHAR)Record);
                    RtlMoveMemory(Next + Delta, Next, Tail);
                    Attr->Length = NewAttr;
                    Attr->Resident.ValueLength = NewValue;
                    Record->BytesInUse = (ULONG)((LONG)Record->BytesInUse + Delta);
                    End = (PCHAR)Record + Record->BytesInUse;
                    FileName = (PNTFS_FILE_NAME_ATTR)((PCHAR)Attr + Attr->Resident.ValueOffset);
                }
                if (Apply)
                {
                    ULONG Index;
                    FileName->FileNameLength = (UCHAR)NewLength;
                    for (Index = 0; Index < NewLength; Index++)
                        FileName->FileName[Index] = (WCHAR)(UCHAR)NewName[Index];
                }
            }
            if (Apply)
                FileName->ParentDirectory = Parent;
        }
        Attr = (PNTFS_ATTR_RECORD)((PCHAR)Attr + Attr->Length);
    }
    return TRUE;
}

typedef struct
{
    PNTFS_INDEX_ENTRY Entry;
    ULONGLONG FileIndex;
    ULONGLONG Chosen;
    BOOLEAN Insert;
    BOOLEAN Remove;
    BOOLEAN Select;
    BOOLEAN Changed;
    BOOLEAN Ok;
    BOOLEAN HaveBest;
    ULONGLONG BestOffset;
    INT BestRank;
    BOOLEAN HaveFallback;
    ULONGLONG FallbackOffset;
    INT FallbackRank;
    BOOLEAN Retarget;
    ULONGLONG FromIndex;
    ULONGLONG ToRef;
    ULONG Hits;
} NTFS_INDEX_JOB;

static PCHAR
NtfsHeaderLimit(PNTFS_INDEX_HEADER Header, PCHAR HardLimit)
{
    PCHAR Soft = (PCHAR)Header + Header->AllocatedSize;
    if (Soft > HardLimit)
        return HardLimit;
    return Soft;
}

static VOID
NtfsConsiderLeaf(NTFS_INDEX_JOB *Job, PNTFS_INDEX_HEADER Header, PCHAR Limit, ULONGLONG Offset)
{
    PNTFS_INDEX_ENTRY First;
    PNTFS_INDEX_ENTRY End;
    INT Order;

    if (!NtfsIndexIsLeaf(Header, Limit))
        return;
    End = NtfsIndexEnd(Header, Limit);
    if (End == NULL)
        return;
    First = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
    if (First == End)
    {
        if (!Job->HaveFallback)
        {
            Job->HaveFallback = TRUE;
            Job->FallbackOffset = Offset;
            Job->FallbackRank = 0x7FFFFFFF;
        }
        return;
    }
    Order = NtfsOrderEntries(First, Job->Entry);
    if (Order <= 0)
    {
        if (!Job->HaveBest || Order > Job->BestRank)
        {
            Job->HaveBest = TRUE;
            Job->BestOffset = Offset;
            Job->BestRank = Order;
        }
    }
    else if (!Job->HaveFallback || Order < Job->FallbackRank)
    {
        Job->HaveFallback = TRUE;
        Job->FallbackOffset = Offset;
        Job->FallbackRank = Order;
    }
}

static BOOLEAN
NtfsTouchHeader(NTFS_INDEX_JOB *Job, PNTFS_INDEX_HEADER Header, PCHAR Limit, ULONGLONG Offset)
{
    if (Job->Select)
    {
        NtfsConsiderLeaf(Job, Header, Limit, Offset);
        return TRUE;
    }
    if (Job->Retarget)
    {
        PNTFS_INDEX_ENTRY Entry;
        PNTFS_INDEX_ENTRY End;

        if (!NtfsIndexIsLeaf(Header, Limit))
            return TRUE;
        End = NtfsIndexEnd(Header, Limit);
        if (End == NULL)
            return TRUE;
        Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Header + Header->EntriesOffset);
        while (Entry < End && (Entry->Flags & NTFS_INDEX_ENTRY_END) == 0)
        {
            if ((Entry->Data.Directory.IndexedFile & NTFS_MFT_MASK) == Job->FromIndex)
            {
                Entry->Data.Directory.IndexedFile = Job->ToRef;
                Job->Changed = TRUE;
                Job->Hits++;
            }
            if (Entry->Length < 16)
            {
                Job->Ok = FALSE;
                return FALSE;
            }
            Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
        }
        return TRUE;
    }
    if (Job->Insert && Offset == Job->Chosen)
    {
        Job->Ok = NtfsIndexInsert(Header, Limit, Job->Entry);
        Job->Changed = Job->Ok;
        return Job->Ok;
    }
    if (Job->Remove)
    {
        BOOLEAN Removed = FALSE;
        if (!NtfsIndexRemove(Header, Limit, Job->FileIndex, &Removed))
        {
            Job->Ok = FALSE;
            return FALSE;
        }
        if (Removed)
            Job->Changed = TRUE;
    }
    return TRUE;
}

static BOOLEAN
NtfsGrowResidentRoot(PNTFS_MFT_RECORD Record, PNTFS_ATTR_RECORD RootAttr, ULONG Extra)
{
    PNTFS_INDEX_ROOT IndexRoot;
    PCHAR Next;
    PCHAR End;
    ULONG Tail;

    /* The new bytes stay in this file record. A full record is not split. */
    Extra = (Extra + (NTFS_ENTRY_ALIGN - 1)) & ~(NTFS_ENTRY_ALIGN - 1);
    if (Extra == 0)
        return TRUE;
    if ((ULONG)RootAttr->Resident.ValueOffset + RootAttr->Resident.ValueLength != RootAttr->Length)
        return FALSE;
    if (Record->BytesInUse > Record->BytesAllocated ||
        Extra > Record->BytesAllocated - Record->BytesInUse)
        return FALSE;
    Next = (PCHAR)RootAttr + RootAttr->Length;
    End = (PCHAR)Record + Record->BytesInUse;
    if (Next > End)
        return FALSE;
    Tail = (ULONG)(End - Next);
    RtlMoveMemory(Next + Extra, Next, Tail);
    RtlZeroMemory(Next, Extra);
    RootAttr->Length += Extra;
    RootAttr->Resident.ValueLength += Extra;
    Record->BytesInUse += Extra;
    IndexRoot = (PNTFS_INDEX_ROOT)((PCHAR)RootAttr + RootAttr->Resident.ValueOffset);
    IndexRoot->IndexHeader.AllocatedSize += Extra;
    return TRUE;
}

static BOOLEAN
NtfsTouchDirectory(PNTFS_VOLUME_INFO Volume, ULONGLONG DirIndex, NTFS_INDEX_JOB *Job)
{
    PNTFS_MFT_RECORD Record;
    PNTFS_ATTR_RECORD RootAttr;
    PNTFS_INDEX_ROOT IndexRoot;
    PCHAR RootLimit;
    BOOLEAN Ok = TRUE;

    Record = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (Record == NULL)
        return FALSE;
    if (!NtfsReadMftRecord(Volume, DirIndex, Record))
    {
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    RootAttr = NtfsResidentNamed(Record, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
    if (RootAttr == NULL)
    {
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    IndexRoot = (PNTFS_INDEX_ROOT)((PCHAR)RootAttr + RootAttr->Resident.ValueOffset);
    RootLimit = NtfsHeaderLimit(&IndexRoot->IndexHeader,
                                (PCHAR)RootAttr + RootAttr->Resident.ValueOffset + RootAttr->Resident.ValueLength);
    if ((IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX) == 0)
    {
        if (Job->Insert && Job->Entry != NULL &&
            Job->Entry->Length >= 16 &&
            IndexRoot->IndexHeader.IndexLength + Job->Entry->Length >
                IndexRoot->IndexHeader.AllocatedSize)
        {
            ULONG Extra = (IndexRoot->IndexHeader.IndexLength + Job->Entry->Length) -
                          IndexRoot->IndexHeader.AllocatedSize;

            if (!NtfsGrowResidentRoot(Record, RootAttr, Extra))
            {
                FrLdrTempFree(Record, TAG_NTFS_MFT);
                return FALSE;
            }
            IndexRoot = (PNTFS_INDEX_ROOT)((PCHAR)RootAttr + RootAttr->Resident.ValueOffset);
            RootLimit = NtfsHeaderLimit(&IndexRoot->IndexHeader,
                                        (PCHAR)RootAttr + RootAttr->Resident.ValueOffset +
                                        RootAttr->Resident.ValueLength);
        }
        Job->Changed = FALSE;
        Ok = NtfsTouchHeader(Job, &IndexRoot->IndexHeader, RootLimit, (ULONGLONG)-1);
        if (Ok && Job->Changed)
            Ok = NtfsWriteMftRecord(Volume, DirIndex, Record);
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return Ok;
    }

    {
        PNTFS_ATTR_CONTEXT BitmapCtx;
        PNTFS_ATTR_CONTEXT AllocCtx;
        ULONGLONG BitmapSize;
        ULONGLONG AllocSize;
        PCHAR Bitmap;
        PCHAR Block;
        ULONG BlockSize = IndexRoot->IndexBlockSize;
        ULONG Offset;

        if (BlockSize == 0 || BlockSize > Volume->IndexRecordSize)
            BlockSize = Volume->IndexRecordSize;
        BitmapCtx = NtfsFindAttribute(Volume, Record, DirIndex, NTFS_ATTR_TYPE_BITMAP, L"$I30");
        AllocCtx = NtfsFindAttribute(Volume, Record, DirIndex, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
        if (BitmapCtx == NULL || AllocCtx == NULL)
        {
            if (BitmapCtx) NtfsReleaseAttributeContext(BitmapCtx);
            if (AllocCtx) NtfsReleaseAttributeContext(AllocCtx);
            FrLdrTempFree(Record, TAG_NTFS_MFT);
            return FALSE;
        }
        BitmapSize = NtfsGetAttributeSize(&BitmapCtx->Record);
        AllocSize = NtfsGetAttributeSize(&AllocCtx->Record);
        Bitmap = FrLdrTempAlloc((ULONG)BitmapSize, TAG_NTFS_BITMAP);
        Block = FrLdrTempAlloc(BlockSize, TAG_NTFS_INDEX_REC);
        if (Bitmap == NULL || Block == NULL || BitmapSize > 0xFFFFFFFF)
        {
            if (Bitmap) FrLdrTempFree(Bitmap, TAG_NTFS_BITMAP);
            if (Block) FrLdrTempFree(Block, TAG_NTFS_INDEX_REC);
            NtfsReleaseAttributeContext(BitmapCtx);
            NtfsReleaseAttributeContext(AllocCtx);
            FrLdrTempFree(Record, TAG_NTFS_MFT);
            return FALSE;
        }
        if (NtfsReadAttribute(Volume, BitmapCtx, 0, Bitmap, (ULONG)BitmapSize) != (ULONG)BitmapSize)
            Ok = FALSE;
        for (Offset = 0; Ok && Offset < AllocSize; Offset += BlockSize)
        {
            ULONG BitIndex = Offset / BlockSize;
            PNTFS_INDEX_BLOCK IndexBlock;
            PCHAR Limit;
            BOOLEAN WriteBack;

            if ((Bitmap[BitIndex >> 3] & (1 << (BitIndex & 7))) == 0)
                continue;
            if (NtfsReadAttribute(Volume, AllocCtx, Offset, Block, BlockSize) != BlockSize ||
                !NtfsFixupRecord(Volume, (PNTFS_RECORD)Block))
            {
                Ok = FALSE;
                break;
            }
            IndexBlock = (PNTFS_INDEX_BLOCK)Block;
            Limit = NtfsHeaderLimit(&IndexBlock->Header, Block + BlockSize);
            Job->Changed = FALSE;
            if (!NtfsTouchHeader(Job, &IndexBlock->Header, Limit, Offset))
            {
                Ok = FALSE;
                break;
            }
            WriteBack = Job->Changed;
            if (WriteBack)
            {
                NtfsPrepareFixup(Volume, (PNTFS_RECORD)Block);
                if (!NtfsWriteAttribute(Volume, AllocCtx, Offset, Block, BlockSize))
                    Ok = FALSE;
            }
        }
        FrLdrTempFree(Bitmap, TAG_NTFS_BITMAP);
        FrLdrTempFree(Block, TAG_NTFS_INDEX_REC);
        NtfsReleaseAttributeContext(BitmapCtx);
        NtfsReleaseAttributeContext(AllocCtx);
    }
    FrLdrTempFree(Record, TAG_NTFS_MFT);
    NtfsDiscardDirectoryCache(Volume);
    return Ok;
}

static BOOLEAN
NtfsDirectoryInsert(PNTFS_VOLUME_INFO Volume, ULONGLONG DirIndex, PNTFS_INDEX_ENTRY Entry)
{
    NTFS_INDEX_JOB Job;
    ULONGLONG Chosen;
    RtlZeroMemory(&Job, sizeof(Job));
    Job.Entry = Entry;
    Job.Select = TRUE;
    Job.Ok = TRUE;
    if (!NtfsTouchDirectory(Volume, DirIndex, &Job))
        return FALSE;
    if (Job.HaveBest)
        Chosen = Job.BestOffset;
    else if (Job.HaveFallback)
        Chosen = Job.FallbackOffset;
    else
        return FALSE;
    RtlZeroMemory(&Job, sizeof(Job));
    Job.Entry = Entry;
    Job.Insert = TRUE;
    Job.Chosen = Chosen;
    Job.Ok = TRUE;
    return NtfsTouchDirectory(Volume, DirIndex, &Job) && Job.Ok;
}

BOOLEAN
NtfsPathExists(_In_ ULONG DeviceId, _In_ PCSTR Path)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumeFromId(DeviceId);
    ULONGLONG Index;

    if (Volume == NULL)
        return FALSE;
    return NtfsLookupMft(Volume, Path, &Index);
}

BOOLEAN
NtfsDeleteFile(_In_ ULONG DeviceId, _In_ PCSTR Path)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumeFromId(DeviceId);
    CHAR Parent[261];
    CHAR Name[261];
    ULONGLONG FileIndex;
    ULONGLONG DirIndex;
    PNTFS_MFT_RECORD Record;
    NTFS_INDEX_JOB Job;

    if (Volume == NULL)
        return FALSE;
    if (!NtfsLookupMft(Volume, Path, &FileIndex))
        return TRUE;
    if (!NtfsSplitPath(Path, Parent, sizeof(Parent), Name, sizeof(Name)))
        return FALSE;
    if (!NtfsLookupMft(Volume, Parent, &DirIndex))
        return FALSE;
    RtlZeroMemory(&Job, sizeof(Job));
    Job.Remove = TRUE;
    Job.FileIndex = FileIndex;
    Job.Ok = TRUE;
    if (!NtfsTouchDirectory(Volume, DirIndex, &Job))
        return FALSE;
    Record = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (Record == NULL)
        return FALSE;
    if (!NtfsReadMftRecord(Volume, FileIndex, Record))
    {
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    if (NtfsResidentNamed(Record, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30") != NULL)
    {
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    Record->Flags &= ~NTFS_MFT_IN_USE;
    if (!NtfsWriteMftRecord(Volume, FileIndex, Record))
    {
        FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    FrLdrTempFree(Record, TAG_NTFS_MFT);
    return TRUE;
}

static BOOLEAN
NtfsRetargetLinks(PNTFS_VOLUME_INFO Volume, ULONGLONG DirIndex, ULONGLONG FromIndex, ULONGLONG ToRef)
{
    NTFS_INDEX_JOB Job;

    RtlZeroMemory(&Job, sizeof(Job));
    Job.Retarget = TRUE;
    Job.FromIndex = FromIndex & NTFS_MFT_MASK;
    Job.ToRef = ToRef;
    Job.Ok = TRUE;
    if (!NtfsTouchDirectory(Volume, DirIndex, &Job))
        return FALSE;
    return Job.Ok && Job.Hits > 0;
}

static ULONGLONG
NtfsFileRef(PNTFS_MFT_RECORD Record, ULONGLONG Index)
{
    return ((ULONGLONG)Record->SequenceNumber << 48) | (Index & NTFS_MFT_MASK);
}

BOOLEAN
NtfsStampFile(_In_ ULONG DeviceId, _In_ PCSTR Path, _In_ PCSTR Marker)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumeFromId(DeviceId);
    ULONGLONG Index;
    PNTFS_MFT_RECORD Record;
    PNTFS_ATTR_RECORD Attr;
    PCHAR End;
    SIZE_T MarkerLen;
    BOOLEAN Stamped = FALSE;
    BOOLEAN Resident = FALSE;

    if (Volume == NULL || Marker == NULL)
        return FALSE;
    MarkerLen = strlen(Marker);
    if (MarkerLen == 0 || MarkerLen > 0xFFFFFFFF || !NtfsLookupMft(Volume, Path, &Index))
        return FALSE;
    Record = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (Record == NULL || !NtfsReadMftRecord(Volume, Index, Record))
    {
        if (Record != NULL)
            FrLdrTempFree(Record, TAG_NTFS_MFT);
        return FALSE;
    }
    Attr = (PNTFS_ATTR_RECORD)((PCHAR)Record + Record->AttributesOffset);
    End = (PCHAR)Record + Record->BytesInUse;
    while ((PCHAR)Attr + sizeof(ULONG) < End && Attr->Type != NTFS_ATTR_TYPE_END)
    {
        if (Attr->Length == 0)
            break;
        if (Attr->Type == NTFS_ATTR_TYPE_DATA && Attr->NameLength == 0)
        {
            if (Attr->IsNonResident == FALSE && Attr->Resident.ValueLength >= MarkerLen)
            {
                RtlCopyMemory((PCHAR)Attr + Attr->Resident.ValueOffset, Marker, MarkerLen);
                Resident = TRUE;
                Stamped = TRUE;
            }
            else if (Attr->IsNonResident != FALSE &&
                     (ULONGLONG)Attr->NonResident.DataSize >= MarkerLen)
            {
                PNTFS_ATTR_CONTEXT Context;

                Context = NtfsFindAttribute(Volume, Record, Index, NTFS_ATTR_TYPE_DATA, L"");
                if (Context != NULL)
                {
                    Stamped = NtfsWriteAttribute(Volume, Context, 0, (PCHAR)Marker, (ULONG)MarkerLen);
                    NtfsReleaseAttributeContext(Context);
                }
            }
            break;
        }
        Attr = (PNTFS_ATTR_RECORD)((PCHAR)Attr + Attr->Length);
    }
    if (Stamped && Resident)
        Stamped = NtfsWriteMftRecord(Volume, Index, Record);
    FrLdrTempFree(Record, TAG_NTFS_MFT);
    return Stamped;
}

ULONG
NtfsLastExchangeCode(VOID)
{
    return NtfsExchangeCode;
}

BOOLEAN
NtfsExchangeLinks(_In_ ULONG DeviceId, _In_ PCSTR StagedPath, _In_ PCSTR DestPath)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumeFromId(DeviceId);
    CHAR StagedParent[261];
    CHAR StagedName[261];
    CHAR DestParent[261];
    CHAR DestName[261];
    ULONGLONG StagedIndex;
    ULONGLONG DestIndex;
    ULONGLONG StagedDir;
    ULONGLONG DestDir;
    PNTFS_MFT_RECORD StagedRecord = NULL;
    PNTFS_MFT_RECORD DestRecord = NULL;
    PNTFS_MFT_RECORD StagedDirRecord = NULL;
    PNTFS_MFT_RECORD DestDirRecord = NULL;
    ULONGLONG StagedRef = 0;
    ULONGLONG DestRef = 0;
    BOOLEAN Linked = FALSE;

    NtfsExchangeCode = 1;
    if (Volume == NULL)
        return FALSE;
    if (!NtfsSplitPath(StagedPath, StagedParent, sizeof(StagedParent), StagedName, sizeof(StagedName)) ||
        !NtfsSplitPath(DestPath, DestParent, sizeof(DestParent), DestName, sizeof(DestName)))
    {
        NtfsExchangeCode = 2;
        return FALSE;
    }
    if (!NtfsLookupMft(Volume, StagedPath, &StagedIndex) ||
        !NtfsLookupMft(Volume, DestPath, &DestIndex) ||
        !NtfsLookupMft(Volume, StagedParent, &StagedDir) ||
        !NtfsLookupMft(Volume, DestParent, &DestDir))
    {
        NtfsExchangeCode = 3;
        return FALSE;
    }
    if ((StagedIndex & NTFS_MFT_MASK) == (DestIndex & NTFS_MFT_MASK))
    {
        NtfsExchangeCode = 4;
        return FALSE;
    }

    StagedRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    DestRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    StagedDirRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    DestDirRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (StagedRecord == NULL || DestRecord == NULL || StagedDirRecord == NULL || DestDirRecord == NULL)
    {
        NtfsExchangeCode = 5;
        goto Fail;
    }
    if (!NtfsReadMftRecord(Volume, StagedIndex, StagedRecord) ||
        !NtfsReadMftRecord(Volume, DestIndex, DestRecord) ||
        !NtfsReadMftRecord(Volume, StagedDir, StagedDirRecord) ||
        !NtfsReadMftRecord(Volume, DestDir, DestDirRecord))
    {
        NtfsExchangeCode = 6;
        goto Fail;
    }

    StagedRef = NtfsFileRef(StagedRecord, StagedIndex);
    DestRef = NtfsFileRef(DestRecord, DestIndex);
    if (!NtfsRenameInRecord(StagedRecord, StagedName, DestName, NtfsFileRef(DestDirRecord, DestDir), TRUE) ||
        !NtfsRenameInRecord(DestRecord, DestName, StagedName, NtfsFileRef(StagedDirRecord, StagedDir), TRUE))
    {
        NtfsExchangeCode = 7;
        goto Fail;
    }
    if (!NtfsRetargetLinks(Volume, DestDir, DestIndex, StagedRef))
    {
        NtfsExchangeCode = 8;
        goto Fail;
    }
    Linked = TRUE;
    if (!NtfsRetargetLinks(Volume, StagedDir, StagedIndex, DestRef))
    {
        NtfsExchangeCode = 9;
        goto Fail;
    }
    if (!NtfsWriteMftRecord(Volume, StagedIndex, StagedRecord) ||
        !NtfsWriteMftRecord(Volume, DestIndex, DestRecord))
    {
        NtfsExchangeCode = 10;
        goto Fail;
    }

    FrLdrTempFree(StagedRecord, TAG_NTFS_MFT);
    FrLdrTempFree(DestRecord, TAG_NTFS_MFT);
    FrLdrTempFree(StagedDirRecord, TAG_NTFS_MFT);
    FrLdrTempFree(DestDirRecord, TAG_NTFS_MFT);
    NtfsDiscardDirectoryCache(Volume);
    return TRUE;

Fail:
    if (Linked)
    {
        NtfsRetargetLinks(Volume, DestDir, StagedIndex, DestRef);
        NtfsRetargetLinks(Volume, StagedDir, DestIndex, StagedRef);
    }
    if (StagedRecord) FrLdrTempFree(StagedRecord, TAG_NTFS_MFT);
    if (DestRecord) FrLdrTempFree(DestRecord, TAG_NTFS_MFT);
    if (StagedDirRecord) FrLdrTempFree(StagedDirRecord, TAG_NTFS_MFT);
    if (DestDirRecord) FrLdrTempFree(DestDirRecord, TAG_NTFS_MFT);
    return FALSE;
}

BOOLEAN
NtfsMoveFile(_In_ ULONG DeviceId, _In_ PCSTR ExistingPath, _In_ PCSTR NewPath)
{
    PNTFS_VOLUME_INFO Volume = NtfsVolumeFromId(DeviceId);
    CHAR OldParent[261];
    CHAR OldName[261];
    CHAR NewParent[261];
    CHAR NewName[261];
    CHAR DestPath[261];
    ULONGLONG FileIndex;
    ULONGLONG OldDir;
    ULONGLONG NewDir;
    PNTFS_MFT_RECORD FileRecord;
    PNTFS_MFT_RECORD DirRecord;
    ULONGLONG ParentRef;
    UCHAR Entries[3][NTFS_ENTRY_CAP];
    ULONG Lengths[3];
    ULONG Count = 0;

    UNREFERENCED_PARAMETER(DestPath);
    if (Volume == NULL)
        return FALSE;
    if (!NtfsSplitPath(ExistingPath, OldParent, sizeof(OldParent), OldName, sizeof(OldName)) ||
        !NtfsSplitPath(NewPath, NewParent, sizeof(NewParent), NewName, sizeof(NewName)))
        return FALSE;
    if (!NtfsLookupMft(Volume, ExistingPath, &FileIndex) ||
        !NtfsLookupMft(Volume, OldParent, &OldDir) ||
        !NtfsLookupMft(Volume, NewParent, &NewDir))
        return FALSE;
    if (NtfsLookupMft(Volume, NewPath, &ParentRef))
        return FALSE;

    DirRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    FileRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (DirRecord == NULL || FileRecord == NULL)
    {
        if (DirRecord) FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
        if (FileRecord) FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
        return FALSE;
    }
    if (!NtfsReadMftRecord(Volume, NewDir, DirRecord) ||
        !NtfsReadMftRecord(Volume, FileIndex, FileRecord))
    {
        FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
        FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
        return FALSE;
    }
    ParentRef = ((ULONGLONG)DirRecord->SequenceNumber << 48) | (NewDir & NTFS_MFT_MASK);
    if (!NtfsRenameInRecord(FileRecord, OldName, NewName, ParentRef, FALSE))
    {
        FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
        FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
        return FALSE;
    }

    {
        PNTFS_MFT_RECORD Source = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
        PNTFS_ATTR_RECORD RootAttr;
        PNTFS_INDEX_ROOT IndexRoot;
        PNTFS_INDEX_ENTRY Entry;
        PNTFS_INDEX_ENTRY End;
        PCHAR Limit;

        if (Source == NULL || !NtfsReadMftRecord(Volume, OldDir, Source))
        {
            if (Source) FrLdrTempFree(Source, TAG_NTFS_MFT);
            FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
            FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
            return FALSE;
        }
        RootAttr = NtfsResidentNamed(Source, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
        if (RootAttr == NULL)
        {
            FrLdrTempFree(Source, TAG_NTFS_MFT);
            FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
            FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
            return FALSE;
        }
        IndexRoot = (PNTFS_INDEX_ROOT)((PCHAR)RootAttr + RootAttr->Resident.ValueOffset);
        if (IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX)
        {
            FrLdrTempFree(Source, TAG_NTFS_MFT);
        }
        else
        {
            Limit = NtfsHeaderLimit(&IndexRoot->IndexHeader,
                                    (PCHAR)RootAttr + RootAttr->Resident.ValueOffset + RootAttr->Resident.ValueLength);
            End = NtfsIndexEnd(&IndexRoot->IndexHeader, Limit);
            Entry = (PNTFS_INDEX_ENTRY)((PCHAR)&IndexRoot->IndexHeader + IndexRoot->IndexHeader.EntriesOffset);
            while (Entry != NULL && End != NULL && Entry < End && Count < 3)
            {
                if ((Entry->Data.Directory.IndexedFile & NTFS_MFT_MASK) == FileIndex)
                {
                    PCSTR Rename = NULL;
                    if (Entry->FileName.FileNameType != NTFS_FILE_NAME_DOS &&
                        NtfsCompareFileName(OldName, strlen(OldName), Entry))
                        Rename = (_stricmp(OldName, NewName) == 0) ? NULL : NewName;
                    if (!NtfsCopyEntry(Entry, ParentRef, Rename, Entries[Count], &Lengths[Count]))
                    {
                        FrLdrTempFree(Source, TAG_NTFS_MFT);
                        FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
                        FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
                        return FALSE;
                    }
                    Count++;
                }
                Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
            }
            FrLdrTempFree(Source, TAG_NTFS_MFT);
        }
    }

    if (Count == 0)
    {
        /* A large directory keeps the file in an index block. Read those blocks. */
        PNTFS_MFT_RECORD Source = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
        PNTFS_ATTR_RECORD RootAttr;
        PNTFS_INDEX_ROOT IndexRoot;
        PNTFS_ATTR_CONTEXT BitmapCtx = NULL;
        PNTFS_ATTR_CONTEXT AllocCtx = NULL;
        PCHAR Bitmap = NULL;
        PCHAR Block = NULL;
        ULONG BlockSize;
        ULONGLONG BitmapSize;
        ULONGLONG AllocSize;
        ULONG Offset;
        BOOLEAN Ok = TRUE;

        if (Source == NULL || !NtfsReadMftRecord(Volume, OldDir, Source))
            Ok = FALSE;
        if (Ok)
        {
            RootAttr = NtfsResidentNamed(Source, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
            Ok = RootAttr != NULL;
        }
        if (Ok)
        {
            IndexRoot = (PNTFS_INDEX_ROOT)((PCHAR)RootAttr + RootAttr->Resident.ValueOffset);
            BlockSize = IndexRoot->IndexBlockSize;
            if (BlockSize == 0 || BlockSize > Volume->IndexRecordSize)
                BlockSize = Volume->IndexRecordSize;
            BitmapCtx = NtfsFindAttribute(Volume, Source, OldDir, NTFS_ATTR_TYPE_BITMAP, L"$I30");
            AllocCtx = NtfsFindAttribute(Volume, Source, OldDir, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
            Ok = BitmapCtx != NULL && AllocCtx != NULL;
        }
        if (Ok)
        {
            BitmapSize = NtfsGetAttributeSize(&BitmapCtx->Record);
            AllocSize = NtfsGetAttributeSize(&AllocCtx->Record);
            Bitmap = FrLdrTempAlloc((ULONG)BitmapSize, TAG_NTFS_BITMAP);
            Block = FrLdrTempAlloc(BlockSize, TAG_NTFS_INDEX_REC);
            Ok = Bitmap != NULL && Block != NULL &&
                 NtfsReadAttribute(Volume, BitmapCtx, 0, Bitmap, (ULONG)BitmapSize) == (ULONG)BitmapSize;
        }
        if (Ok)
        {
            for (Offset = 0; Offset < AllocSize && Count < 3; Offset += BlockSize)
            {
                ULONG BitIndex = Offset / BlockSize;
                PNTFS_INDEX_BLOCK IndexBlock;
                PNTFS_INDEX_ENTRY Entry;
                PNTFS_INDEX_ENTRY End;
                PCHAR Limit;
                if ((Bitmap[BitIndex >> 3] & (1 << (BitIndex & 7))) == 0)
                    continue;
                if (NtfsReadAttribute(Volume, AllocCtx, Offset, Block, BlockSize) != BlockSize ||
                    !NtfsFixupRecord(Volume, (PNTFS_RECORD)Block))
                {
                    Ok = FALSE;
                    break;
                }
                IndexBlock = (PNTFS_INDEX_BLOCK)Block;
                Limit = NtfsHeaderLimit(&IndexBlock->Header, Block + BlockSize);
                if (!NtfsIndexIsLeaf(&IndexBlock->Header, Limit))
                    continue;
                End = NtfsIndexEnd(&IndexBlock->Header, Limit);
                Entry = (PNTFS_INDEX_ENTRY)((PCHAR)&IndexBlock->Header + IndexBlock->Header.EntriesOffset);
                while (Entry < End && Count < 3)
                {
                    if ((Entry->Data.Directory.IndexedFile & NTFS_MFT_MASK) == FileIndex)
                    {
                        PCSTR Rename = NULL;
                        if (Entry->FileName.FileNameType != NTFS_FILE_NAME_DOS &&
                            NtfsCompareFileName(OldName, strlen(OldName), Entry))
                            Rename = (_stricmp(OldName, NewName) == 0) ? NULL : NewName;
                        if (!NtfsCopyEntry(Entry, ParentRef, Rename, Entries[Count], &Lengths[Count]))
                            Ok = FALSE;
                        else
                            Count++;
                    }
                    Entry = (PNTFS_INDEX_ENTRY)((PCHAR)Entry + Entry->Length);
                }
            }
        }
        if (Bitmap) FrLdrTempFree(Bitmap, TAG_NTFS_BITMAP);
        if (Block) FrLdrTempFree(Block, TAG_NTFS_INDEX_REC);
        if (BitmapCtx) NtfsReleaseAttributeContext(BitmapCtx);
        if (AllocCtx) NtfsReleaseAttributeContext(AllocCtx);
        if (Source) FrLdrTempFree(Source, TAG_NTFS_MFT);
        if (!Ok || Count == 0)
        {
            FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
            FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
            return FALSE;
        }
    }

    {
        ULONG Index;
        NTFS_INDEX_JOB Undo;
        for (Index = 0; Index < Count; Index++)
        {
            if (!NtfsDirectoryInsert(Volume, NewDir, (PNTFS_INDEX_ENTRY)Entries[Index]))
            {
                RtlZeroMemory(&Undo, sizeof(Undo));
                Undo.Remove = TRUE;
                Undo.FileIndex = FileIndex;
                Undo.Ok = TRUE;
                NtfsTouchDirectory(Volume, NewDir, &Undo);
                FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
                FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
                return FALSE;
            }
        }
        RtlZeroMemory(&Undo, sizeof(Undo));
        Undo.Remove = TRUE;
        Undo.FileIndex = FileIndex;
        Undo.Ok = TRUE;
        if (!NtfsTouchDirectory(Volume, OldDir, &Undo))
        {
            NtfsTouchDirectory(Volume, NewDir, &Undo);
            FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
            FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
            return FALSE;
        }
    }

    if (!NtfsRenameInRecord(FileRecord, OldName, NewName, ParentRef, TRUE) ||
        !NtfsWriteMftRecord(Volume, FileIndex, FileRecord))
    {
        FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
        FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
        return FALSE;
    }
    FrLdrTempFree(DirRecord, TAG_NTFS_MFT);
    FrLdrTempFree(FileRecord, TAG_NTFS_MFT);
    NtfsDiscardDirectoryCache(Volume);
    return TRUE;
}


const DEVVTBL NtfsFuncTable =
{
    NtfsClose,
    NtfsGetFileInformation,
    NtfsOpen,
    NtfsRead,
    NtfsSeek,
    L"ntfs",
};

static VOID
NtfsFreeVolume(PNTFS_VOLUME_INFO Volume)
{
    if (Volume->MFTContext)
        NtfsReleaseAttributeContext(Volume->MFTContext);
    if (Volume->MasterFileTable)
        FrLdrTempFree(Volume->MasterFileTable, TAG_NTFS_MFT);
    if (Volume->TemporarySector)
        FrLdrTempFree(Volume->TemporarySector, TAG_NTFS_DATA);
    NtfsDiscardDirectoryCache(Volume);
    FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
}

const DEVVTBL* NtfsMount(ULONG DeviceId)
{
    PNTFS_VOLUME_INFO Volume;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("Enter NtfsMount(%lu)\n", DeviceId);

    if (DeviceId < MAX_FDS && NtfsVolumes[DeviceId])
    {
        NtfsFreeVolume(NtfsVolumes[DeviceId]);
        NtfsVolumes[DeviceId] = NULL;
    }

    //
    // Allocate data for volume information
    //
    Volume = FrLdrTempAlloc(sizeof(NTFS_VOLUME_INFO), TAG_NTFS_VOLUME);
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(NTFS_VOLUME_INFO));

    //
    // Read the BootSector
    //
    Position.QuadPart = 0;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }
    Status = ArcRead(DeviceId, &Volume->BootSector, sizeof(Volume->BootSector), &Count);
    if (Status != ESUCCESS || Count != sizeof(Volume->BootSector))
    {
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }

    //
    // Check if BootSector is valid. If no, return early
    //
    if (!RtlEqualMemory(Volume->BootSector.SystemId, "NTFS", 4))
    {
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }

    //
    // Calculate cluster size and MFT record size
    //
    Volume->ClusterSize = Volume->BootSector.SectorsPerCluster * Volume->BootSector.BytesPerSector;
    if (Volume->BootSector.ClustersPerMftRecord > 0)
        Volume->MftRecordSize = Volume->BootSector.ClustersPerMftRecord * Volume->ClusterSize;
    else
        Volume->MftRecordSize = 1 << (-Volume->BootSector.ClustersPerMftRecord);
    if (Volume->BootSector.ClustersPerIndexRecord > 0)
        Volume->IndexRecordSize = Volume->BootSector.ClustersPerIndexRecord * Volume->ClusterSize;
    else
        Volume->IndexRecordSize = 1 << (-Volume->BootSector.ClustersPerIndexRecord);

    TRACE("ClusterSize: 0x%x\n", Volume->ClusterSize);
    TRACE("ClustersPerMftRecord: %d\n", Volume->BootSector.ClustersPerMftRecord);
    TRACE("ClustersPerIndexRecord: %d\n", Volume->BootSector.ClustersPerIndexRecord);
    TRACE("MftRecordSize: 0x%x\n", Volume->MftRecordSize);
    TRACE("IndexRecordSize: 0x%x\n", Volume->IndexRecordSize);

    //
    // Read MFT index
    //
    TRACE("Reading MFT index...\n");
    Volume->MasterFileTable = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (!Volume->MasterFileTable)
    {
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }
    Position.QuadPart = Volume->BootSector.MftLocation * Volume->ClusterSize;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        FileSystemError("Failed to seek to Master File Table record.");
        FrLdrTempFree(Volume->MasterFileTable, TAG_NTFS_MFT);
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }
    Status = ArcRead(DeviceId, Volume->MasterFileTable, Volume->MftRecordSize, &Count);
    if (Status != ESUCCESS || Count != Volume->MftRecordSize)
    {
        FileSystemError("Failed to read the Master File Table record.");
        FrLdrTempFree(Volume->MasterFileTable, TAG_NTFS_MFT);
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }

    //
    // Keep room to read partial sectors
    //
    Volume->TemporarySector = FrLdrTempAlloc(Volume->BootSector.BytesPerSector, TAG_NTFS_DATA);
    if (!Volume->TemporarySector)
    {
        FileSystemError("Failed to allocate memory.");
        FrLdrTempFree(Volume->MasterFileTable, TAG_NTFS_MFT);
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }

    //
    // Keep device id
    //
    Volume->DeviceId = DeviceId;

    //
    // Search DATA attribute
    //
    TRACE("Searching for DATA attribute...\n");
    Volume->MFTContext = NtfsFindAttribute(Volume, Volume->MasterFileTable, 0, NTFS_ATTR_TYPE_DATA, L"");
    if (!Volume->MFTContext)
    {
        FileSystemError("Can't find data attribute for Master File Table.");
        FrLdrTempFree(Volume->MasterFileTable, TAG_NTFS_MFT);
        FrLdrTempFree(Volume, TAG_NTFS_VOLUME);
        return NULL;
    }

    //
    // Remember NTFS volume information
    //
    NtfsVolumes[DeviceId] = Volume;

    //
    // Return success
    //
    TRACE("NtfsMount(%lu) success\n", DeviceId);
    return &NtfsFuncTable;
}

#endif
