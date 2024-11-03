/*
 *  FreeLoader NTFS support
 *  Copyright (C) 2004  Filip Navara  <xnavara@volny.cz>
 *  Copyright (C) 2009-2010  Hervé Poussineau
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
#define TAG_NTFS_MFT 'MftN'
#define TAG_NTFS_INDEX_REC 'IftN'
#define TAG_NTFS_BITMAP 'BftN'
#define TAG_NTFS_FILE 'FftN'
#define TAG_NTFS_VOLUME 'VftN'
#define TAG_NTFS_DATA 'DftN'

typedef struct _NTFS_VOLUME_INFO
{
    NTFS_BOOTSECTOR BootSector;
    ULONG ClusterSize;
    ULONG MftRecordSize;
    ULONG IndexRecordSize;
    PNTFS_MFT_RECORD MasterFileTable;
    /* FIXME: MFTContext is never freed. */
    PNTFS_ATTR_CONTEXT MFTContext;
    ULONG DeviceId;
    PUCHAR TemporarySector;
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
        LONGLONG DataRunOffset;
        ULONGLONG DataRunLength;

        Context->CacheRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
        Context->CacheRunOffset = 0;
        Context->CacheRun = NtfsDecodeRun(Context->CacheRun, &DataRunOffset, &DataRunLength);
        Context->CacheRunLength = DataRunLength;
        if (DataRunOffset != -1)
        {
            /* Normal run. */
            Context->CacheRunStartLCN =
            Context->CacheRunLastLCN = DataRunOffset;
        }
        else
        {
            /* Sparse run. */
            Context->CacheRunStartLCN = -1;
            Context->CacheRunLastLCN = 0;
        }
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

    TRACE("NtfsDiskRead - Offset: %I64d Length: %I64d\n", Offset, Length);

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

    // FIXME: Cache seems to be non-working. Disable it for now
    //if(Context->CacheRunOffset <= Offset && Offset < Context->CacheRunOffset + Context->CacheRunLength * Volume->ClusterSize)
    if (0)
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

    Context->CacheRun = DataRun;
    Context->CacheRunOffset = Offset + AlreadyRead;
    Context->CacheRunStartLCN = DataRunStartLCN;
    Context->CacheRunLength = DataRunLength;
    Context->CacheRunLastLCN = LastLCN;
    Context->CacheRunCurrentOffset = CurrentOffset;

    return AlreadyRead;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttributeHelper(PNTFS_VOLUME_INFO Volume, PNTFS_ATTR_RECORD AttrRecord, PNTFS_ATTR_RECORD AttrRecordEnd, ULONG Type, const WCHAR *Name, ULONG NameLength)
{
    while (AttrRecord < AttrRecordEnd)
    {
        if (AttrRecord->Type == NTFS_ATTR_TYPE_END)
            break;

        if (AttrRecord->Type == NTFS_ATTR_TYPE_ATTRIBUTE_LIST)
        {
            PNTFS_ATTR_CONTEXT Context;
            PNTFS_ATTR_CONTEXT ListContext;
            PVOID ListBuffer;
            ULONGLONG ListSize;
            PNTFS_ATTR_RECORD ListAttrRecord;
            PNTFS_ATTR_RECORD ListAttrRecordEnd;

            ListContext = NtfsPrepareAttributeContext(AttrRecord);

            ListSize = NtfsGetAttributeSize(&ListContext->Record);
            if(ListSize <= 0xFFFFFFFF)
                ListBuffer = FrLdrTempAlloc((ULONG)ListSize, TAG_NTFS_LIST);
            else
                ListBuffer = NULL;

            if(!ListBuffer)
            {
                TRACE("Failed to allocate memory: %x\n", (ULONG)ListSize);
                continue;
            }

            ListAttrRecord = (PNTFS_ATTR_RECORD)ListBuffer;
            ListAttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)ListBuffer + ListSize);

            if (NtfsReadAttribute(Volume, ListContext, 0, ListBuffer, (ULONG)ListSize) == ListSize)
            {
                Context = NtfsFindAttributeHelper(Volume, ListAttrRecord, ListAttrRecordEnd,
                                                  Type, Name, NameLength);

                NtfsReleaseAttributeContext(ListContext);
                FrLdrTempFree(ListBuffer, TAG_NTFS_LIST);

                if (Context != NULL)
                    return Context;
            }
        }

        if (AttrRecord->Type == Type)
        {
            if (AttrRecord->NameLength == NameLength)
            {
                PWCHAR AttrName;

                AttrName = (PWCHAR)((PCHAR)AttrRecord + AttrRecord->NameOffset);
                if (RtlEqualMemory(AttrName, Name, NameLength << 1))
                {
                    /* Found it, fill up the context and return. */
                    return NtfsPrepareAttributeContext(AttrRecord);
                }
            }
        }

        if (AttrRecord->Length == 0)
            return NULL;
        AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)AttrRecord + AttrRecord->Length);
    }

    return NULL;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttribute(PNTFS_VOLUME_INFO Volume, PNTFS_MFT_RECORD MftRecord, ULONG Type, const WCHAR *Name)
{
    PNTFS_ATTR_RECORD AttrRecord;
    PNTFS_ATTR_RECORD AttrRecordEnd;
    ULONG NameLength;

    AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + MftRecord->AttributesOffset);
    AttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + Volume->MftRecordSize);
    for (NameLength = 0; Name[NameLength] != 0; NameLength++)
        ;

    return NtfsFindAttributeHelper(Volume, AttrRecord, AttrRecordEnd, Type, Name, NameLength);
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

static BOOLEAN NtfsReadMftRecord(PNTFS_VOLUME_INFO Volume, ULONGLONG MFTIndex, PNTFS_MFT_RECORD Buffer)
{
    ULONGLONG BytesRead;

    BytesRead = NtfsReadAttribute(Volume, Volume->MFTContext, MFTIndex * Volume->MftRecordSize, (PCHAR)Buffer, Volume->MftRecordSize);
    if (BytesRead != Volume->MftRecordSize)
        return FALSE;

    /* Apply update sequence array fixups. */
    return NtfsFixupRecord(Volume, (PNTFS_RECORD)Buffer);
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

static BOOLEAN NtfsCompareFileName(PCHAR FileName, PNTFS_INDEX_ENTRY IndexEntry)
{
    PWCHAR EntryFileName;
    UCHAR EntryFileNameLength;
    UCHAR i;

    EntryFileName = IndexEntry->FileName.FileName;
    EntryFileNameLength = IndexEntry->FileName.FileNameLength;

#if DBG
    NtfsPrintFile(IndexEntry);
#endif

    if (strlen(FileName) != EntryFileNameLength)
        return FALSE;

    /* Do case-sensitive compares for Posix file names. */
    if (IndexEntry->FileName.FileNameType == NTFS_FILE_NAME_POSIX)
    {
        for (i = 0; i < EntryFileNameLength; i++)
            if (EntryFileName[i] != FileName[i])
                return FALSE;
    }
    else
    {
        for (i = 0; i < EntryFileNameLength; i++)
            if (tolower(EntryFileName[i]) != tolower(FileName[i]))
                return FALSE;
    }

    return TRUE;
}

static BOOLEAN NtfsFindMftRecord(PNTFS_VOLUME_INFO Volume, ULONGLONG MFTIndex, PCHAR FileName, ULONGLONG *OutMFTIndex)
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

    MftRecord = FrLdrTempAlloc(Volume->MftRecordSize, TAG_NTFS_MFT);
    if (MftRecord == NULL)
    {
        return FALSE;
    }

    if (NtfsReadMftRecord(Volume, MFTIndex, MftRecord))
    {
        //Magic = MftRecord->Magic;

        IndexRootCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
        if (IndexRootCtx == NULL)
        {
            FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
            return FALSE;
        }

        IndexRecord = FrLdrTempAlloc(Volume->IndexRecordSize, TAG_NTFS_INDEX_REC);
        if (IndexRecord == NULL)
        {
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
            if (NtfsCompareFileName(FileName, IndexEntry))
            {
                *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
                FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return TRUE;
            }
        IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
        }

        if (IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX)
        {
            TRACE("Large Index!\n");

            IndexBlockSize = IndexRoot->IndexBlockSize;

            IndexBitmapCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_BITMAP, L"$I30");
            if (IndexBitmapCtx == NULL)
            {
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
                FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return FALSE;
            }
            NtfsReadAttribute(Volume, IndexBitmapCtx, 0, BitmapData, (ULONG)BitmapDataSize);
            NtfsReleaseAttributeContext(IndexBitmapCtx);

            IndexAllocationCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
            if (IndexAllocationCtx == NULL)
            {
                TRACE("Corrupted filesystem!\n");
                FrLdrTempFree(BitmapData, TAG_NTFS_BITMAP);
                FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                return FALSE;
            }
            IndexAllocationSize = NtfsGetAttributeSize(&IndexAllocationCtx->Record);

            RecordOffset = 0;

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
                    break;
                }

                /* FIXME */
                IndexEntry = (PNTFS_INDEX_ENTRY)(IndexRecord + 0x18 + *(USHORT *)(IndexRecord + 0x18));
            IndexEntryEnd = (PNTFS_INDEX_ENTRY)(IndexRecord + IndexBlockSize);

                while (IndexEntry < IndexEntryEnd &&
                       !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
                {
                    if (NtfsCompareFileName(FileName, IndexEntry))
                    {
                        TRACE("File found\n");
                        *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
                        FrLdrTempFree(BitmapData, TAG_NTFS_BITMAP);
                        FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
                        FrLdrTempFree(MftRecord, TAG_NTFS_MFT);
                        NtfsReleaseAttributeContext(IndexAllocationCtx);
                        return TRUE;
                    }
                    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
                }

                RecordOffset += IndexBlockSize;
            }

            NtfsReleaseAttributeContext(IndexAllocationCtx);
            FrLdrTempFree(BitmapData, TAG_NTFS_BITMAP);
        }

        FrLdrTempFree(IndexRecord, TAG_NTFS_INDEX_REC);
    }
    else
    {
        TRACE("Can't read MFT record\n");
    }
    FrLdrTempFree(MftRecord, TAG_NTFS_MFT);

    return FALSE;
}

static BOOLEAN NtfsLookupFile(PNTFS_VOLUME_INFO Volume, PCSTR FileName, PNTFS_MFT_RECORD MftRecord, PNTFS_ATTR_CONTEXT *DataContext)
{
    ULONG NumberOfPathParts;
    CHAR PathPart[261];
    ULONGLONG CurrentMFTIndex;
    UCHAR i;

    TRACE("NtfsLookupFile() FileName = %s\n", FileName);

    CurrentMFTIndex = NTFS_FILE_ROOT;

    /* Skip leading path separator, if any */
    if (*FileName == '\\' || *FileName == '/')
        ++FileName;

    NumberOfPathParts = FsGetNumPathParts(FileName);
    for (i = 0; i < NumberOfPathParts; i++)
    {
        FsGetFirstNameFromPath(PathPart, FileName);

        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
            ;
        FileName++;

        TRACE("- Lookup: %s\n", PathPart);
        if (!NtfsFindMftRecord(Volume, CurrentMFTIndex, PathPart, &CurrentMFTIndex))
        {
            TRACE("- Failed\n");
            return FALSE;
        }
        TRACE("- Lookup: %x\n", CurrentMFTIndex);
    }

    if (!NtfsReadMftRecord(Volume, CurrentMFTIndex, MftRecord))
    {
        TRACE("NtfsLookupFile: Can't read MFT record\n");
        return FALSE;
    }

    *DataContext = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_DATA, L"");
    if (*DataContext == NULL)
    {
        TRACE("NtfsLookupFile: Can't find data attribute\n");
        return FALSE;
    }

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
    if (!NtfsLookupFile(Volume, Path, MftRecord, &FileHandle->DataContext))
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

    //
    // Read file
    //
    BytesRead64 = NtfsReadAttribute(FileHandle->Volume, FileHandle->DataContext, FileHandle->Offset, Buffer, N);
    FileHandle->Offset += BytesRead64;
    *Count = (ULONG)BytesRead64;

    //
    // Check for success
    //
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

const DEVVTBL NtfsFuncTable =
{
    NtfsClose,
    NtfsGetFileInformation,
    NtfsOpen,
    NtfsRead,
    NtfsSeek,
    L"ntfs",
};

const DEVVTBL* NtfsMount(ULONG DeviceId)
{
    PNTFS_VOLUME_INFO Volume;
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    TRACE("Enter NtfsMount(%lu)\n", DeviceId);

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
    Volume->MFTContext = NtfsFindAttribute(Volume, Volume->MasterFileTable, NTFS_ATTR_TYPE_DATA, L"");
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
