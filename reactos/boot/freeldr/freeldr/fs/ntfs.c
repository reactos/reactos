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

    DPRINTM(DPRINT_FILESYSTEM, "DataRunOffsetSize: %x\n", DataRunOffsetSize);
    DPRINTM(DPRINT_FILESYSTEM, "DataRunLengthSize: %x\n", DataRunLengthSize);
    DPRINTM(DPRINT_FILESYSTEM, "DataRunOffset: %x\n", *DataRunOffset);
    DPRINTM(DPRINT_FILESYSTEM, "DataRunLength: %x\n", *DataRunLength);

    return DataRun;
}

static PNTFS_ATTR_CONTEXT NtfsPrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord)
{
    PNTFS_ATTR_CONTEXT Context;

    Context = MmHeapAlloc(FIELD_OFFSET(NTFS_ATTR_CONTEXT, Record) + AttrRecord->Length);
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
    MmHeapFree(Context);
}

static BOOLEAN NtfsDiskRead(PNTFS_VOLUME_INFO Volume, ULONGLONG Offset, ULONGLONG Length, PCHAR Buffer)
{
    LARGE_INTEGER Position;
    ULONG Count;
    USHORT ReadLength;
    LONG ret;

    DPRINTM(DPRINT_FILESYSTEM, "NtfsDiskRead - Offset: %I64d Length: %I64d\n", Offset, Length);

    //
    // I. Read partial first sector if needed
    //
    if (Offset % Volume->BootSector.BytesPerSector)
    {
        Position.HighPart = 0;
        Position.LowPart = Offset & ~(Volume->BootSector.BytesPerSector - 1);
        ret = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (ret != ESUCCESS)
            return FALSE;
        ReadLength = min(Length, Volume->BootSector.BytesPerSector - (Offset % Volume->BootSector.BytesPerSector));
        ret = ArcRead(Volume->DeviceId, Buffer, ReadLength, &Count);
        if (ret != ESUCCESS || Count != ReadLength)
            return FALSE;

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
        Position.HighPart = 0;
        Position.LowPart = Offset;
        ret = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (ret != ESUCCESS)
            return FALSE;
        ReadLength = Length & ~(Volume->BootSector.BytesPerSector - 1);
        ret = ArcRead(Volume->DeviceId, Buffer, ReadLength, &Count);
        if (ret != ESUCCESS || Count != ReadLength)
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
        Position.HighPart = 0;
        Position.LowPart = Offset;
        ret = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
        if (ret != ESUCCESS)
            return FALSE;
        ret = ArcRead(Volume->DeviceId, Buffer, Length, &Count);
        if (ret != ESUCCESS || Count != Length)
            return FALSE;
    }

    return TRUE;
}

static ULONGLONG NtfsReadAttribute(PNTFS_VOLUME_INFO Volume, PNTFS_ATTR_CONTEXT Context, ULONGLONG Offset, PCHAR Buffer, ULONGLONG Length)
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

    ReadLength = min(DataRunLength * Volume->ClusterSize - (Offset - CurrentOffset), Length);
    if (DataRunStartLCN == -1)
    RtlZeroMemory(Buffer, ReadLength);
    if (NtfsDiskRead(Volume, DataRunStartLCN * Volume->ClusterSize + Offset - CurrentOffset, ReadLength, Buffer))
    {
        Length -= ReadLength;
        Buffer += ReadLength;
        AlreadyRead += ReadLength;

        if (ReadLength == DataRunLength * Volume->ClusterSize - (Offset - CurrentOffset))
        {
            CurrentOffset += DataRunLength * Volume->ClusterSize;
            DataRun = NtfsDecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunLength != (ULONGLONG)-1)
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
            ReadLength = min(DataRunLength * Volume->ClusterSize, Length);
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
            ListBuffer = MmHeapAlloc(ListSize);

            ListAttrRecord = (PNTFS_ATTR_RECORD)ListBuffer;
            ListAttrRecordEnd = (PNTFS_ATTR_RECORD)((PCHAR)ListBuffer + ListSize);

            if (NtfsReadAttribute(Volume, ListContext, 0, ListBuffer, ListSize) == ListSize)
            {
                Context = NtfsFindAttributeHelper(Volume, ListAttrRecord, ListAttrRecordEnd,
                                                  Type, Name, NameLength);

                NtfsReleaseAttributeContext(ListContext);
                MmHeapFree(ListBuffer);

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

static BOOLEAN NtfsReadMftRecord(PNTFS_VOLUME_INFO Volume, ULONG MFTIndex, PNTFS_MFT_RECORD Buffer)
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
    FileNameLength = IndexEntry->FileName.FileNameLength;

    for (i = 0; i < FileNameLength; i++)
        AnsiFileName[i] = FileName[i];
    AnsiFileName[i] = 0;

    DPRINTM(DPRINT_FILESYSTEM, "- %s (%x)\n", AnsiFileName, IndexEntry->Data.Directory.IndexedFile);
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

static BOOLEAN NtfsFindMftRecord(PNTFS_VOLUME_INFO Volume, ULONG MFTIndex, PCHAR FileName, ULONG *OutMFTIndex)
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

    MftRecord = MmHeapAlloc(Volume->MftRecordSize);
    if (MftRecord == NULL)
    {
        return FALSE;
    }

    if (NtfsReadMftRecord(Volume, MFTIndex, MftRecord))
    {
        Magic = MftRecord->Magic;

        IndexRootCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_INDEX_ROOT, L"$I30");
        if (IndexRootCtx == NULL)
        {
            MmHeapFree(MftRecord);
            return FALSE;
        }

        IndexRecord = MmHeapAlloc(Volume->IndexRecordSize);
        if (IndexRecord == NULL)
        {
            MmHeapFree(MftRecord);
            return FALSE;
        }

        NtfsReadAttribute(Volume, IndexRootCtx, 0, IndexRecord, Volume->IndexRecordSize);
        IndexRoot = (PNTFS_INDEX_ROOT)IndexRecord;
        IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)&IndexRoot->IndexHeader + IndexRoot->IndexHeader.EntriesOffset);
        /* Index root is always resident. */
        IndexEntryEnd = (PNTFS_INDEX_ENTRY)(IndexRecord + IndexRootCtx->Record.Resident.ValueLength);
        NtfsReleaseAttributeContext(IndexRootCtx);

        DPRINTM(DPRINT_FILESYSTEM, "IndexRecordSize: %x IndexBlockSize: %x\n", Volume->IndexRecordSize, IndexRoot->IndexBlockSize);

        while (IndexEntry < IndexEntryEnd &&
               !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
        {
            if (NtfsCompareFileName(FileName, IndexEntry))
            {
                *OutMFTIndex = IndexEntry->Data.Directory.IndexedFile;
                MmHeapFree(IndexRecord);
                MmHeapFree(MftRecord);
                return TRUE;
            }
	    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
        }

        if (IndexRoot->IndexHeader.Flags & NTFS_LARGE_INDEX)
        {
            DPRINTM(DPRINT_FILESYSTEM, "Large Index!\n");

            IndexBlockSize = IndexRoot->IndexBlockSize;

            IndexBitmapCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_BITMAP, L"$I30");
            if (IndexBitmapCtx == NULL)
            {
                DPRINTM(DPRINT_FILESYSTEM, "Corrupted filesystem!\n");
                MmHeapFree(MftRecord);
                return FALSE;
            }
            BitmapDataSize = NtfsGetAttributeSize(&IndexBitmapCtx->Record);
            DPRINTM(DPRINT_FILESYSTEM, "BitmapDataSize: %x\n", BitmapDataSize);
            BitmapData = MmHeapAlloc(BitmapDataSize);
            if (BitmapData == NULL)
            {
                MmHeapFree(IndexRecord);
                MmHeapFree(MftRecord);
                return FALSE;
            }
            NtfsReadAttribute(Volume, IndexBitmapCtx, 0, BitmapData, BitmapDataSize);
            NtfsReleaseAttributeContext(IndexBitmapCtx);

            IndexAllocationCtx = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_INDEX_ALLOCATION, L"$I30");
            if (IndexAllocationCtx == NULL)
            {
                DPRINTM(DPRINT_FILESYSTEM, "Corrupted filesystem!\n");
                MmHeapFree(BitmapData);
                MmHeapFree(IndexRecord);
                MmHeapFree(MftRecord);
                return FALSE;
            }
            IndexAllocationSize = NtfsGetAttributeSize(&IndexAllocationCtx->Record);

            RecordOffset = 0;

            for (;;)
            {
                DPRINTM(DPRINT_FILESYSTEM, "RecordOffset: %x IndexAllocationSize: %x\n", RecordOffset, IndexAllocationSize);
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
                        DPRINTM(DPRINT_FILESYSTEM, "File found\n");
                        *OutMFTIndex = IndexEntry->Data.Directory.IndexedFile;
                        MmHeapFree(BitmapData);
                        MmHeapFree(IndexRecord);
                        MmHeapFree(MftRecord);
                        NtfsReleaseAttributeContext(IndexAllocationCtx);
                        return TRUE;
                    }
                    IndexEntry = (PNTFS_INDEX_ENTRY)((PCHAR)IndexEntry + IndexEntry->Length);
                }

                RecordOffset += IndexBlockSize;
            }

            NtfsReleaseAttributeContext(IndexAllocationCtx);
            MmHeapFree(BitmapData);
        }

        MmHeapFree(IndexRecord);
    }
    else
    {
        DPRINTM(DPRINT_FILESYSTEM, "Can't read MFT record\n");
    }
    MmHeapFree(MftRecord);

    return FALSE;
}

static BOOLEAN NtfsLookupFile(PNTFS_VOLUME_INFO Volume, PCSTR FileName, PNTFS_MFT_RECORD MftRecord, PNTFS_ATTR_CONTEXT *DataContext)
{
    ULONG NumberOfPathParts;
    CHAR PathPart[261];
    ULONG CurrentMFTIndex;
    UCHAR i;

    DPRINTM(DPRINT_FILESYSTEM, "NtfsLookupFile() FileName = %s\n", FileName);

    CurrentMFTIndex = NTFS_FILE_ROOT;
    NumberOfPathParts = FsGetNumPathParts(FileName);
    for (i = 0; i < NumberOfPathParts; i++)
    {
        FsGetFirstNameFromPath(PathPart, FileName);

        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
            ;
        FileName++;

        DPRINTM(DPRINT_FILESYSTEM, "- Lookup: %s\n", PathPart);
        if (!NtfsFindMftRecord(Volume, CurrentMFTIndex, PathPart, &CurrentMFTIndex))
        {
            DPRINTM(DPRINT_FILESYSTEM, "- Failed\n");
            return FALSE;
        }
        DPRINTM(DPRINT_FILESYSTEM, "- Lookup: %x\n", CurrentMFTIndex);
    }

    if (!NtfsReadMftRecord(Volume, CurrentMFTIndex, MftRecord))
    {
        DPRINTM(DPRINT_FILESYSTEM, "NtfsLookupFile: Can't read MFT record\n");
        return FALSE;
    }

    *DataContext = NtfsFindAttribute(Volume, MftRecord, NTFS_ATTR_TYPE_DATA, L"");
    if (*DataContext == NULL)
    {
        DPRINTM(DPRINT_FILESYSTEM, "NtfsLookupFile: Can't find data attribute\n");
        return FALSE;
    }

    return TRUE;
}

LONG NtfsClose(ULONG FileId)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);

    NtfsReleaseAttributeContext(FileHandle->DataContext);
    MmHeapFree(FileHandle);

    return ESUCCESS;
}

LONG NtfsGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    Information->EndingAddress.LowPart = (ULONG)NtfsGetAttributeSize(&FileHandle->DataContext->Record);
    Information->CurrentAddress.LowPart = FileHandle->Offset;

    DPRINTM(DPRINT_FILESYSTEM, "NtfsGetFileInformation() FileSize = %d\n",
        Information->EndingAddress.LowPart);
    DPRINTM(DPRINT_FILESYSTEM, "NtfsGetFileInformation() FilePointer = %d\n",
        Information->CurrentAddress.LowPart);

    return ESUCCESS;
}

LONG NtfsOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
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

    DPRINTM(DPRINT_FILESYSTEM, "NtfsOpen() FileName = %s\n", Path);

    //
    // Allocate file structure
    //
    FileHandle = MmHeapAlloc(sizeof(NTFS_FILE_HANDLE) + Volume->MftRecordSize);
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
        MmHeapFree(FileHandle);
        return ENOENT;
    }

    return ESUCCESS;
}

LONG NtfsRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);
    ULONGLONG BytesRead64;

    //
    // Read file
    //
    BytesRead64 = NtfsReadAttribute(FileHandle->Volume, FileHandle->DataContext, FileHandle->Offset, Buffer, N);
    *Count = (ULONG)BytesRead64;

    //
    // Check for success
    //
    if (BytesRead64 > 0)
        return ESUCCESS;
    else
        return EIO;
}

LONG NtfsSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PNTFS_FILE_HANDLE FileHandle = FsGetDeviceSpecific(FileId);

    DPRINTM(DPRINT_FILESYSTEM, "NtfsSeek() NewFilePointer = %lu\n", Position->LowPart);

    if (SeekMode != SeekAbsolute)
        return EINVAL;
    if (Position->HighPart != 0)
        return EINVAL;
    if (Position->LowPart >= (ULONG)NtfsGetAttributeSize(&FileHandle->DataContext->Record))
        return EINVAL;

    FileHandle->Offset = Position->LowPart;
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
    LONG ret;

    //
    // Allocate data for volume information
    //
    Volume = MmHeapAlloc(sizeof(NTFS_VOLUME_INFO));
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(NTFS_VOLUME_INFO));

    //
    // Read the BootSector
    //
    Position.HighPart = 0;
    Position.LowPart = 0;
    ret = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (ret != ESUCCESS)
    {
        MmHeapFree(Volume);
        return NULL;
    }
    ret = ArcRead(DeviceId, &Volume->BootSector, sizeof(Volume->BootSector), &Count);
    if (ret != ESUCCESS || Count != sizeof(Volume->BootSector))
    {
        MmHeapFree(Volume);
        return NULL;
    }

    //
    // Check if BootSector is valid. If no, return early
    //
    if (!RtlEqualMemory(Volume->BootSector.SystemId, "NTFS", 4))
    {
        MmHeapFree(Volume);
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

    DPRINTM(DPRINT_FILESYSTEM, "ClusterSize: 0x%x\n", Volume->ClusterSize);
    DPRINTM(DPRINT_FILESYSTEM, "ClustersPerMftRecord: %d\n", Volume->BootSector.ClustersPerMftRecord);
    DPRINTM(DPRINT_FILESYSTEM, "ClustersPerIndexRecord: %d\n", Volume->BootSector.ClustersPerIndexRecord);
    DPRINTM(DPRINT_FILESYSTEM, "MftRecordSize: 0x%x\n", Volume->MftRecordSize);
    DPRINTM(DPRINT_FILESYSTEM, "IndexRecordSize: 0x%x\n", Volume->IndexRecordSize);

    //
    // Read MFT index
    //
    DPRINTM(DPRINT_FILESYSTEM, "Reading MFT index...\n");
    Volume->MasterFileTable = MmHeapAlloc(Volume->MftRecordSize);
    if (!Volume->MasterFileTable)
    {
        MmHeapFree(Volume);
        return NULL;
    }
    Position.HighPart = 0;
    Position.LowPart = Volume->BootSector.MftLocation * Volume->ClusterSize;
    ret = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (ret != ESUCCESS)
    {
        FileSystemError("Failed to seek to Master File Table record.");
        MmHeapFree(Volume->MasterFileTable);
        MmHeapFree(Volume);
        return NULL;
    }
    ret = ArcRead(DeviceId, Volume->MasterFileTable, Volume->MftRecordSize, &Count);
    if (ret != ESUCCESS || Count != Volume->MftRecordSize)
    {
        FileSystemError("Failed to read the Master File Table record.");
        MmHeapFree(Volume->MasterFileTable);
        MmHeapFree(Volume);
        return NULL;
    }

    //
    // Keep device id
    //
    Volume->DeviceId = DeviceId;

    //
    // Search DATA attribute
    //
    DPRINTM(DPRINT_FILESYSTEM, "Searching for DATA attribute...\n");
    Volume->MFTContext = NtfsFindAttribute(Volume, Volume->MasterFileTable, NTFS_ATTR_TYPE_DATA, L"");
    if (!Volume->MFTContext)
    {
        FileSystemError("Can't find data attribute for Master File Table.");
        MmHeapFree(Volume->MasterFileTable);
        MmHeapFree(Volume);
        return NULL;
    }

    //
    // Remember NTFS volume information
    //
    NtfsVolumes[DeviceId] = Volume;

    //
    // Return success
    //
    return &NtfsFuncTable;
}

#endif
