/*
 *  FreeLoader NTFS support
 *  Copyright (C) 2004  Filip Navara  <xnavara@volny.cz>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Limitations:
 * - No support for compressed files.
 * - May crash on corrupted filesystem.
 */

#include <freeldr.h>
#include <debug.h>

PNTFS_BOOTSECTOR NtfsBootSector;
ULONG NtfsClusterSize;
ULONG NtfsMftRecordSize;
ULONG NtfsIndexRecordSize;
ULONG NtfsDriveNumber;
ULONG NtfsSectorOfClusterZero;
PNTFS_MFT_RECORD NtfsMasterFileTable;
/* FIXME: NtfsMFTContext is never freed. */
PNTFS_ATTR_CONTEXT NtfsMFTContext;

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
        *DataRunLength += *DataRun << (i << 3);
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
            *DataRunOffset += *DataRun << (i << 3);
            DataRun++;
        }
        /* The last byte contains sign so we must process it different way. */
        *DataRunOffset = ((CHAR)(*(DataRun++)) << (i << 3)) + *DataRunOffset;
    }

    DbgPrint((DPRINT_FILESYSTEM, "DataRunOffsetSize: %x\n", DataRunOffsetSize));
    DbgPrint((DPRINT_FILESYSTEM, "DataRunLengthSize: %x\n", DataRunLengthSize));
    DbgPrint((DPRINT_FILESYSTEM, "DataRunOffset: %x\n", *DataRunOffset));
    DbgPrint((DPRINT_FILESYSTEM, "DataRunLength: %x\n", *DataRunLength));

    return DataRun;
}

static PNTFS_ATTR_CONTEXT NtfsPrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord)
{
    PNTFS_ATTR_CONTEXT Context;

    Context = MmAllocateMemory(FIELD_OFFSET(NTFS_ATTR_CONTEXT, Record) + AttrRecord->Length);
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
    MmFreeMemory(Context);
}

/* FIXME: Optimize for multisector reads. */
static BOOLEAN NtfsDiskRead(ULONGLONG Offset, ULONGLONG Length, PCHAR Buffer)
{
    USHORT ReadLength;

    DbgPrint((DPRINT_FILESYSTEM, "NtfsDiskRead - Offset: %I64d Length: %I64d\n", Offset, Length));
    RtlZeroMemory((PCHAR)DISKREADBUFFER, 0x1000);

    /* I. Read partial first sector if needed */
    if (Offset % NtfsBootSector->BytesPerSector)
    {
        if (!MachDiskReadLogicalSectors(NtfsDriveNumber, NtfsSectorOfClusterZero + (Offset / NtfsBootSector->BytesPerSector), 1, (PCHAR)DISKREADBUFFER))
            return FALSE;
        ReadLength = min(Length, NtfsBootSector->BytesPerSector - (Offset % NtfsBootSector->BytesPerSector));
        RtlCopyMemory(Buffer, (PCHAR)DISKREADBUFFER + (Offset % NtfsBootSector->BytesPerSector), ReadLength);
        Buffer += ReadLength;
        Length -= ReadLength;
        Offset += ReadLength;
    }

    /* II. Read all complete 64-sector blocks. */
    while (Length >= (ULONGLONG)64 * (ULONGLONG)NtfsBootSector->BytesPerSector)
    {
        if (!MachDiskReadLogicalSectors(NtfsDriveNumber, NtfsSectorOfClusterZero + (Offset / NtfsBootSector->BytesPerSector), 64, (PCHAR)DISKREADBUFFER))
            return FALSE;
        RtlCopyMemory(Buffer, (PCHAR)DISKREADBUFFER, 64 * NtfsBootSector->BytesPerSector);
        Buffer += 64 * NtfsBootSector->BytesPerSector;
        Length -= 64 * NtfsBootSector->BytesPerSector;
        Offset += 64 * NtfsBootSector->BytesPerSector;
    }

    /* III. Read the rest of data */
    if (Length)
    {
        ReadLength = ((Length + NtfsBootSector->BytesPerSector - 1) / NtfsBootSector->BytesPerSector);
        if (!MachDiskReadLogicalSectors(NtfsDriveNumber, NtfsSectorOfClusterZero + (Offset / NtfsBootSector->BytesPerSector), ReadLength, (PCHAR)DISKREADBUFFER))
            return FALSE;
        RtlCopyMemory(Buffer, (PCHAR)DISKREADBUFFER, Length);
    }

    return TRUE;
}

static ULONGLONG NtfsReadAttribute(PNTFS_ATTR_CONTEXT Context, ULONGLONG Offset, PCHAR Buffer, ULONGLONG Length)
{
    ULONGLONG LastLCN;
    PUCHAR DataRun;
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG DataRunStartLCN;
    ULONGLONG CurrentOffset;
    ULONGLONG ReadLength;
    ULONGLONG AlreadyRead;

    if (!Context->Record.IsNonResident)
    {
        if (Offset > Context->Record.Resident.ValueLength)
            return 0;
        if (Offset + Length > Context->Record.Resident.ValueLength)
            Length = Context->Record.Resident.ValueLength - Offset;
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

    if(Context->CacheRunOffset <= Offset && Offset < Context->CacheRunOffset + Context->CacheRunLength * NtfsClusterSize)
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
                Offset < CurrentOffset + (DataRunLength * NtfsClusterSize))
            {
                break;
            }

            if (*DataRun == 0)
            {
                return AlreadyRead;
            }

            CurrentOffset += DataRunLength * NtfsClusterSize;
        }
    }

    /*
     * II. Go through the run list and read the data
     */

    ReadLength = min(DataRunLength * NtfsClusterSize - (Offset - CurrentOffset), Length);
    if (DataRunStartLCN == -1)
    RtlZeroMemory(Buffer, ReadLength);
    if (NtfsDiskRead(DataRunStartLCN * NtfsClusterSize + Offset - CurrentOffset, ReadLength, Buffer))
    {
        Length -= ReadLength;
        Buffer += ReadLength;
        AlreadyRead += ReadLength;

        if (ReadLength == DataRunLength * NtfsClusterSize - (Offset - CurrentOffset))
        {
            CurrentOffset += DataRunLength * NtfsClusterSize;
            DataRun = NtfsDecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunLength != -1)
            {
                DataRunStartLCN = LastLCN + DataRunOffset;
                LastLCN = DataRunStartLCN;
            }
            else
                DataRunStartLCN = -1;

            if (*DataRun == 0)
                return AlreadyRead;
        }

        while (Length > 0)
        {
            ReadLength = min(DataRunLength * NtfsClusterSize, Length);
            if (DataRunStartLCN == -1)
                RtlZeroMemory(Buffer, ReadLength);
            else if (!NtfsDiskRead(DataRunStartLCN * NtfsClusterSize, ReadLength, Buffer))
                break;

            Length -= ReadLength;
            Buffer += ReadLength;
            AlreadyRead += ReadLength;

            /* We finished this request, but there still data in this data run. */
            if (Length == 0 && ReadLength != DataRunLength * NtfsClusterSize)
                break;

            /*
             * Go to next run in the list.
             */

            if (*DataRun == 0)
                break;
            CurrentOffset += DataRunLength * NtfsClusterSize;
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

static PNTFS_ATTR_CONTEXT NtfsFindAttributeHelper(PNTFS_ATTR_RECORD AttrRecord, PNTFS_ATTR_RECORD AttrRecordEnd, ULONG Type, const WCHAR *Name, ULONG NameLength)
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
            ListBuffer = MmAllocateMemory(ListSize);

            ListAttrRecord = (PNTFS_ATTR_RECORD)ListBuffer;
            ListAttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)ListBuffer + ListSize);

            if (NtfsReadAttribute(ListContext, 0, ListBuffer, ListSize) == ListSize)
            {
                Context = NtfsFindAttributeHelper(ListAttrRecord, ListAttrRecordEnd,
                                                  Type, Name, NameLength);

                NtfsReleaseAttributeContext(ListContext);
                MmFreeMemory(ListBuffer);

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

        AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)AttrRecord + AttrRecord->Length);
    }

    return NULL;
}

static PNTFS_ATTR_CONTEXT NtfsFindAttribute(PNTFS_MFT_RECORD MftRecord, ULONG Type, const WCHAR *Name)
{
    PNTFS_ATTR_RECORD AttrRecord;
    PNTFS_ATTR_RECORD AttrRecordEnd;
    ULONG NameLength;

    AttrRecord = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + MftRecord->AttributesOffset);
    AttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)MftRecord + NtfsMftRecordSize);
    for (NameLength = 0; Name[NameLength] != 0; NameLength++)
        ;

    return NtfsFindAttributeHelper(AttrRecord, AttrRecordEnd, Type, Name, NameLength);
}

static BOOLEAN NtfsFixupRecord(PNTFS_RECORD Record)
{
    USHORT *USA;
    USHORT USANumber;
    USHORT USACount;
    USHORT *Block;

    USA = (USHORT*)((PCHAR)Record + Record->USAOffset);
    USANumber = *(USA++);
    USACount = Record->USACount - 1; /* Exclude the USA Number. */
    Block = (USHORT*)((PCHAR)Record + NtfsBootSector->BytesPerSector - 2);

    while (USACount)
    {
        if (*Block != USANumber)
            return FALSE;
        *Block = *(USA++);
        Block = (USHORT*)((PCHAR)Block + NtfsBootSector->BytesPerSector);
        USACount--;
    }

    return TRUE;
}

static BOOLEAN NtfsReadMftRecord(ULONG MFTIndex, PNTFS_MFT_RECORD Buffer)
{
    ULONGLONG BytesRead;

    BytesRead = NtfsReadAttribute(NtfsMFTContext, MFTIndex * NtfsMftRecordSize, (PCHAR)Buffer, NtfsMftRecordSize);
    if (BytesRead != NtfsMftRecordSize)
        return FALSE;

    /* Apply update sequence array fixups. */
    return NtfsFixupRecord((PNTFS_RECORD)Buffer);
}

#ifdef DBG
VOID NtfsPrintFile(PNTFS_INDEX_ENTRY IndexEntry)
{
    PWCHAR FileName;
    UCHAR FileNameLength;
    CHAR AnsiFileName[256];
    UCHAR i;

    FileName = IndexEntry->FileName.FileName;
    FileNameLength = IndexEntry->FileName.FileNameLength;

    for (i = 0; i < FileNameLength; i++)
        AnsiFileName[i] = FileName[i];
    AnsiFileName[i] = 0;

    DbgPrint((DPRINT_FILESYSTEM, "- %s (%x)\n", AnsiFileName, IndexEntry->Data.Directory.IndexedFile));
}
#endif

static BOOLEAN NtfsCompareFileName(PCHAR FileName, PNTFS_INDEX_ENTRY IndexEntry)
{
    PWCHAR EntryFileName;
    UCHAR EntryFileNameLength;
    UCHAR i;

    EntryFileName = IndexEntry->FileName.FileName;
    EntryFileNameLength = IndexEntry->FileName.FileNameLength;

#ifdef DBG
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

static BOOLEAN NtfsFindMftRecord(ULONG MFTIndex, PCHAR FileName, ULONG *OutMFTIndex)
{
    PNTFS_MFT_RECORD MftRecord;
    ULONG Magic;
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

    MftRecord = MmAllocateMemory(NtfsMftRecordSize);
    if (MftRecord == NULL)
    {
        return FALSE;
    }

    if (NtfsReadMftRecord(MFTIndex, MftRecord))
    {
        Magic = MftRecord->Magic;

        IndexRootCtx = NtfsFindAttribute(MftRecord, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
        if (IndexRootCtx == NULL)
        {
            MmFreeMemory(MftRecord);
            return FALSE;
        }

        IndexRecord = MmAllocateMemory(NtfsIndexRecordSize);
        if (IndexRecord == NULL)
        {
            MmFreeMemory(MftRecord);
            return FALSE;
        }

        NtfsReadAttribute(IndexRootCtx, 0, IndexRecord, NtfsIndexRecordSize);
        IndexRoot = (PNTFS_INDEX_ROOT)IndexRecord;
        IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)&IndexRoot->IndexHeader + IndexRoot->IndexHeader.EntriesOffset);
        /* Index root is always resident. */
        IndexEntryEnd = (PNTFS_INDEX_ENTRY)(IndexRecord + IndexRootCtx->Record.Resident.ValueLength);
        NtfsReleaseAttributeContext(IndexRootCtx);

        DbgPrint((DPRINT_FILESYSTEM, "NtfsIndexRecordSize: %x IndexBlockSize: %x\n", NtfsIndexRecordSize, IndexRoot->IndexBlockSize));

        while (IndexEntry < IndexEntryEnd &&
               !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
        {
            if (NtfsCompareFileName(FileName, IndexEntry))
            {
                *OutMFTIndex = IndexEntry->Data.Directory.IndexedFile;
                MmFreeMemory(IndexRecord);
                MmFreeMemory(MftRecord);
                return TRUE;
            }
	    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
        }

        if (IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX)
        {
            DbgPrint((DPRINT_FILESYSTEM, "Large Index!\n"));

            IndexBlockSize = IndexRoot->IndexBlockSize;

            IndexBitmapCtx = NtfsFindAttribute(MftRecord, NTFS_ATTR_TYPE_BITMAP, L"$I30");
            if (IndexBitmapCtx == NULL)
            {
                DbgPrint((DPRINT_FILESYSTEM, "Corrupted filesystem!\n"));
                MmFreeMemory(MftRecord);
                return FALSE;
            }
            BitmapDataSize = NtfsGetAttributeSize(&IndexBitmapCtx->Record);
            DbgPrint((DPRINT_FILESYSTEM, "BitmapDataSize: %x\n", BitmapDataSize));
            BitmapData = MmAllocateMemory(BitmapDataSize);
            if (BitmapData == NULL)
            {
                MmFreeMemory(IndexRecord);
                MmFreeMemory(MftRecord);
                return FALSE;
            }
            NtfsReadAttribute(IndexBitmapCtx, 0, BitmapData, BitmapDataSize);
            NtfsReleaseAttributeContext(IndexBitmapCtx);

            IndexAllocationCtx = NtfsFindAttribute(MftRecord, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
            if (IndexAllocationCtx == NULL)
            {
                DbgPrint((DPRINT_FILESYSTEM, "Corrupted filesystem!\n"));
                MmFreeMemory(BitmapData);
                MmFreeMemory(IndexRecord);
                MmFreeMemory(MftRecord);
                return FALSE;
            }
            IndexAllocationSize = NtfsGetAttributeSize(&IndexAllocationCtx->Record);

            RecordOffset = 0;

            for (;;)
            {
                DbgPrint((DPRINT_FILESYSTEM, "RecordOffset: %x IndexAllocationSize: %x\n", RecordOffset, IndexAllocationSize));
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

                NtfsReadAttribute(IndexAllocationCtx, RecordOffset, IndexRecord, IndexBlockSize);

                if (!NtfsFixupRecord((PNTFS_RECORD)IndexRecord))
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
                        DbgPrint((DPRINT_FILESYSTEM, "File found\n"));
                        *OutMFTIndex = IndexEntry->Data.Directory.IndexedFile;
                        MmFreeMemory(BitmapData);
                        MmFreeMemory(IndexRecord);
                        MmFreeMemory(MftRecord);
                        NtfsReleaseAttributeContext(IndexAllocationCtx);
                        return TRUE;
                    }
                    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
                }

                RecordOffset += IndexBlockSize;
            }

            NtfsReleaseAttributeContext(IndexAllocationCtx);
            MmFreeMemory(BitmapData);
        }

        MmFreeMemory(IndexRecord);
    }
    else
    {
        DbgPrint((DPRINT_FILESYSTEM, "Can't read MFT record\n"));
    }
    MmFreeMemory(MftRecord);

    return FALSE;
}

static BOOLEAN NtfsLookupFile(PCSTR FileName, PNTFS_MFT_RECORD MftRecord, PNTFS_ATTR_CONTEXT *DataContext)
{
    ULONG NumberOfPathParts;
    CHAR PathPart[261];
    ULONG CurrentMFTIndex;
    UCHAR i;

    DbgPrint((DPRINT_FILESYSTEM, "NtfsLookupFile() FileName = %s\n", FileName));

    CurrentMFTIndex = NTFS_FILE_ROOT;
    NumberOfPathParts = FsGetNumPathParts(FileName);
    for (i = 0; i < NumberOfPathParts; i++)
    {
        FsGetFirstNameFromPath(PathPart, FileName);

        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
            ;
        FileName++;

        DbgPrint((DPRINT_FILESYSTEM, "- Lookup: %s\n", PathPart));
        if (!NtfsFindMftRecord(CurrentMFTIndex, PathPart, &CurrentMFTIndex))
        {
            DbgPrint((DPRINT_FILESYSTEM, "- Failed\n"));
            return FALSE;
        }
        DbgPrint((DPRINT_FILESYSTEM, "- Lookup: %x\n", CurrentMFTIndex));
    }

    if (!NtfsReadMftRecord(CurrentMFTIndex, MftRecord))
    {
        DbgPrint((DPRINT_FILESYSTEM, "NtfsLookupFile: Can't read MFT record\n"));
        return FALSE;
    }

    *DataContext = NtfsFindAttribute(MftRecord, NTFS_ATTR_TYPE_DATA, L"");
    if (*DataContext == NULL)
    {
        DbgPrint((DPRINT_FILESYSTEM, "NtfsLookupFile: Can't find data attribute\n"));
        return FALSE;
    }

    return TRUE;
}

BOOLEAN NtfsOpenVolume(ULONG DriveNumber, ULONG VolumeStartSector)
{
    NtfsBootSector = (PNTFS_BOOTSECTOR)DISKREADBUFFER;

    DbgPrint((DPRINT_FILESYSTEM, "NtfsOpenVolume() DriveNumber = 0x%x VolumeStartSector = 0x%x\n", DriveNumber, VolumeStartSector));

    if (!MachDiskReadLogicalSectors(DriveNumber, VolumeStartSector, 1, (PCHAR)DISKREADBUFFER))
    {
        FileSystemError("Failed to read the boot sector.");
        return FALSE;
    }

    if (!RtlEqualMemory(NtfsBootSector->SystemId, "NTFS", 4))
    {
        FileSystemError("Invalid NTFS signature.");
        return FALSE;
    }

    NtfsBootSector = MmAllocateMemory(NtfsBootSector->BytesPerSector);
    if (NtfsBootSector == NULL)
    {
        return FALSE;
    }

    RtlCopyMemory(NtfsBootSector, (PCHAR)DISKREADBUFFER, ((PNTFS_BOOTSECTOR)DISKREADBUFFER)->BytesPerSector);

    NtfsClusterSize = NtfsBootSector->SectorsPerCluster * NtfsBootSector->BytesPerSector;
    if (NtfsBootSector->ClustersPerMftRecord > 0)
        NtfsMftRecordSize = NtfsBootSector->ClustersPerMftRecord * NtfsClusterSize;
    else
        NtfsMftRecordSize = 1 << (-NtfsBootSector->ClustersPerMftRecord);
    if (NtfsBootSector->ClustersPerIndexRecord > 0)
        NtfsIndexRecordSize = NtfsBootSector->ClustersPerIndexRecord * NtfsClusterSize;
    else
        NtfsIndexRecordSize = 1 << (-NtfsBootSector->ClustersPerIndexRecord);

    DbgPrint((DPRINT_FILESYSTEM, "NtfsClusterSize: 0x%x\n", NtfsClusterSize));
    DbgPrint((DPRINT_FILESYSTEM, "ClustersPerMftRecord: %d\n", NtfsBootSector->ClustersPerMftRecord));
    DbgPrint((DPRINT_FILESYSTEM, "ClustersPerIndexRecord: %d\n", NtfsBootSector->ClustersPerIndexRecord));
    DbgPrint((DPRINT_FILESYSTEM, "NtfsMftRecordSize: 0x%x\n", NtfsMftRecordSize));
    DbgPrint((DPRINT_FILESYSTEM, "NtfsIndexRecordSize: 0x%x\n", NtfsIndexRecordSize));

    NtfsDriveNumber = DriveNumber;
    NtfsSectorOfClusterZero = VolumeStartSector;

    DbgPrint((DPRINT_FILESYSTEM, "Reading MFT index...\n"));
    if (!MachDiskReadLogicalSectors(DriveNumber,
                                NtfsSectorOfClusterZero +
                                (NtfsBootSector->MftLocation * NtfsBootSector->SectorsPerCluster),
                                NtfsMftRecordSize / NtfsBootSector->BytesPerSector, (PCHAR)DISKREADBUFFER))
    {
        FileSystemError("Failed to read the Master File Table record.");
        return FALSE;
    }

    NtfsMasterFileTable = MmAllocateMemory(NtfsMftRecordSize);
    if (NtfsMasterFileTable == NULL)
    {
        MmFreeMemory(NtfsBootSector);
        return FALSE;
    }

    RtlCopyMemory(NtfsMasterFileTable, (PCHAR)DISKREADBUFFER, NtfsMftRecordSize);

    DbgPrint((DPRINT_FILESYSTEM, "Searching for DATA attribute...\n"));
    NtfsMFTContext = NtfsFindAttribute(NtfsMasterFileTable, NTFS_ATTR_TYPE_DATA, L"");
    if (NtfsMFTContext == NULL)
    {
        FileSystemError("Can't find data attribute for Master File Table.");
        return FALSE;
    }

    return TRUE;
}

FILE* NtfsOpenFile(PCSTR FileName)
{
    PNTFS_FILE_HANDLE FileHandle;
    PNTFS_MFT_RECORD MftRecord;

    FileHandle = MmAllocateMemory(sizeof(NTFS_FILE_HANDLE) + NtfsMftRecordSize);
    if (FileHandle == NULL)
    {
        return NULL;
    }

    MftRecord = (PNTFS_MFT_RECORD)(FileHandle + 1);
    if (!NtfsLookupFile(FileName, MftRecord, &FileHandle->DataContext))
    {
        MmFreeMemory(FileHandle);
        return NULL;
    }

    FileHandle->Offset = 0;

    return (FILE*)FileHandle;
}

VOID NtfsCloseFile(FILE *File)
{
    PNTFS_FILE_HANDLE FileHandle = (PNTFS_FILE_HANDLE)File;
    NtfsReleaseAttributeContext(FileHandle->DataContext);
    MmFreeMemory(FileHandle);
}

BOOLEAN NtfsReadFile(FILE *File, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
    PNTFS_FILE_HANDLE FileHandle = (PNTFS_FILE_HANDLE)File;
    ULONGLONG BytesRead64;
    BytesRead64 = NtfsReadAttribute(FileHandle->DataContext, FileHandle->Offset, Buffer, BytesToRead);
    if (BytesRead64)
    {
        *BytesRead = (ULONG)BytesRead64;
        FileHandle->Offset += BytesRead64;
        return TRUE;
    }
    return FALSE;
}

ULONG NtfsGetFileSize(FILE *File)
{
    PNTFS_FILE_HANDLE FileHandle = (PNTFS_FILE_HANDLE)File;
    return (ULONG)NtfsGetAttributeSize(&FileHandle->DataContext->Record);
}

VOID NtfsSetFilePointer(FILE *File, ULONG NewFilePointer)
{
    PNTFS_FILE_HANDLE FileHandle = (PNTFS_FILE_HANDLE)File;
    FileHandle->Offset = NewFilePointer;
}

ULONG NtfsGetFilePointer(FILE *File)
{
    PNTFS_FILE_HANDLE FileHandle = (PNTFS_FILE_HANDLE)File;
    return FileHandle->Offset;
}
