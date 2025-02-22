/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2014 ReactOS Team
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/mft.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Valentin Verkhovsky
 *                   Pierre Schweitzer (pierre@reactos.org)
 *                   Hervé Poussineau (hpoussin@reactos.org)
 *                   Trevor Thompson
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"
#include <ntintsafe.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PNTFS_ATTR_CONTEXT
PrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord)
{
    PNTFS_ATTR_CONTEXT Context;

    Context = ExAllocateFromNPagedLookasideList(&NtfsGlobalData->AttrCtxtLookasideList);
    if(!Context)
    {
        DPRINT1("Error: Unable to allocate memory for context!\n");
        return NULL;
    }

    // Allocate memory for a copy of the attribute
    Context->pRecord = ExAllocatePoolWithTag(NonPagedPool, AttrRecord->Length, TAG_NTFS);
    if(!Context->pRecord)
    {
        DPRINT1("Error: Unable to allocate memory for attribute record!\n");
        ExFreeToNPagedLookasideList(&NtfsGlobalData->AttrCtxtLookasideList, Context);
        return NULL;
    }

    // Copy the attribute
    RtlCopyMemory(Context->pRecord, AttrRecord, AttrRecord->Length);

    if (AttrRecord->IsNonResident)
    {
        LONGLONG DataRunOffset;
        ULONGLONG DataRunLength;
        ULONGLONG NextVBN = 0;
        PUCHAR DataRun = (PUCHAR)((ULONG_PTR)Context->pRecord + Context->pRecord->NonResident.MappingPairsOffset);

        Context->CacheRun = DataRun;
        Context->CacheRunOffset = 0;
        Context->CacheRun = DecodeRun(Context->CacheRun, &DataRunOffset, &DataRunLength);
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

        // Convert the data runs to a map control block
        if (!NT_SUCCESS(ConvertDataRunsToLargeMCB(DataRun, &Context->DataRunsMCB, &NextVBN)))
        {
            DPRINT1("Unable to convert data runs to MCB!\n");
            ExFreePoolWithTag(Context->pRecord, TAG_NTFS);
            ExFreeToNPagedLookasideList(&NtfsGlobalData->AttrCtxtLookasideList, Context);
            return NULL;
        }
    }

    return Context;
}


VOID
ReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context)
{
    if (Context->pRecord)
    {
        if (Context->pRecord->IsNonResident)
        {
            FsRtlUninitializeLargeMcb(&Context->DataRunsMCB);
        }

        ExFreePoolWithTag(Context->pRecord, TAG_NTFS);
    }

    ExFreeToNPagedLookasideList(&NtfsGlobalData->AttrCtxtLookasideList, Context);
}


/**
* @name FindAttribute
* @implemented
*
* Searches a file record for an attribute matching the given type and name.
*
* @param Offset
* Optional pointer to a ULONG that will receive the offset of the found attribute
* from the beginning of the record. Can be set to NULL.
*/
NTSTATUS
FindAttribute(PDEVICE_EXTENSION Vcb,
              PFILE_RECORD_HEADER MftRecord,
              ULONG Type,
              PCWSTR Name,
              ULONG NameLength,
              PNTFS_ATTR_CONTEXT * AttrCtx,
              PULONG Offset)
{
    BOOLEAN Found;
    NTSTATUS Status;
    FIND_ATTR_CONTXT Context;
    PNTFS_ATTR_RECORD Attribute;
    PNTFS_ATTRIBUTE_LIST_ITEM AttrListItem;

    DPRINT("FindAttribute(%p, %p, 0x%x, %S, %lu, %p, %p)\n", Vcb, MftRecord, Type, Name, NameLength, AttrCtx, Offset);

    Found = FALSE;
    Status = FindFirstAttribute(&Context, Vcb, MftRecord, FALSE, &Attribute);
    while (NT_SUCCESS(Status))
    {
        if (Attribute->Type == Type && Attribute->NameLength == NameLength)
        {
            if (NameLength != 0)
            {
                PWCHAR AttrName;

                AttrName = (PWCHAR)((PCHAR)Attribute + Attribute->NameOffset);
                DPRINT("%.*S, %.*S\n", Attribute->NameLength, AttrName, NameLength, Name);
                if (RtlCompareMemory(AttrName, Name, NameLength * sizeof(WCHAR)) == (NameLength  * sizeof(WCHAR)))
                {
                    Found = TRUE;
                }
            }
            else
            {
                Found = TRUE;
            }

            if (Found)
            {
                /* Found it, fill up the context and return. */
                DPRINT("Found context\n");
                *AttrCtx = PrepareAttributeContext(Attribute);

                (*AttrCtx)->FileMFTIndex = MftRecord->MFTRecordNumber;

                if (Offset != NULL)
                    *Offset = Context.Offset;

                FindCloseAttribute(&Context);
                return STATUS_SUCCESS;
            }
        }

        Status = FindNextAttribute(&Context, &Attribute);
    }

    /* No attribute found, check if it is referenced in another file record */
    Status = FindFirstAttributeListItem(&Context, &AttrListItem);
    while (NT_SUCCESS(Status))
    {
        if (AttrListItem->Type == Type && AttrListItem->NameLength == NameLength)
        {
            if (NameLength != 0)
            {
                PWCHAR AttrName;

                AttrName = (PWCHAR)((PCHAR)AttrListItem + AttrListItem->NameOffset);
                DPRINT("%.*S, %.*S\n", AttrListItem->NameLength, AttrName, NameLength, Name);
                if (RtlCompareMemory(AttrName, Name, NameLength * sizeof(WCHAR)) == (NameLength  * sizeof(WCHAR)))
                {
                    Found = TRUE;
                }
            }
            else
            {
                Found = TRUE;
            }

            if (Found == TRUE)
            {
                /* Get the MFT Index of attribute */
                ULONGLONG MftIndex;
                PFILE_RECORD_HEADER RemoteHdr;

                MftIndex = AttrListItem->MFTIndex & NTFS_MFT_MASK;
                RemoteHdr = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);

                if (RemoteHdr == NULL)
                {
                    FindCloseAttribute(&Context);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                /* Check we are not reading ourselves */
                if (MftRecord->MFTRecordNumber == MftIndex)
                {
                    DPRINT1("Attribute list references missing attribute to this file entry !");
                    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, RemoteHdr);
                    FindCloseAttribute(&Context);
                    return STATUS_OBJECT_NAME_NOT_FOUND;
                }
                /* Read the new file record */
                ReadFileRecord(Vcb, MftIndex, RemoteHdr);
                Status = FindAttribute(Vcb, RemoteHdr, Type, Name, NameLength, AttrCtx, Offset);
                ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, RemoteHdr);
                FindCloseAttribute(&Context);
                return Status;
            }
        }
        Status = FindNextAttributeListItem(&Context, &AttrListItem);
    }
    FindCloseAttribute(&Context);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


ULONGLONG
AttributeAllocatedLength(PNTFS_ATTR_RECORD AttrRecord)
{
    if (AttrRecord->IsNonResident)
        return AttrRecord->NonResident.AllocatedSize;
    else
        return ALIGN_UP_BY(AttrRecord->Resident.ValueLength, ATTR_RECORD_ALIGNMENT);
}


ULONGLONG
AttributeDataLength(PNTFS_ATTR_RECORD AttrRecord)
{
    if (AttrRecord->IsNonResident)
        return AttrRecord->NonResident.DataSize;
    else
        return AttrRecord->Resident.ValueLength;
}

/**
* @name IncreaseMftSize
* @implemented
*
* Increases the size of the master file table on a volume, increasing the space available for file records.
*
* @param Vcb
* Pointer to the VCB (DEVICE_EXTENSION) of the target volume.
*
*
* @param CanWait
* Boolean indicating if the function is allowed to wait for exclusive access to the master file table.
* This will only be relevant if the MFT doesn't have any free file records and needs to be enlarged.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_INVALID_PARAMETER if there was an error reading the Mft's bitmap.
* STATUS_CANT_WAIT if CanWait was FALSE and the function could not get immediate, exclusive access to the MFT.
*
* @remarks
* Increases the size of the Master File Table by 64 records. Bitmap entries for the new records are cleared,
* and the bitmap is also enlarged if needed. Mimicking Windows' behavior when enlarging the mft is still TODO.
* This function will wait for exlusive access to the volume fcb.
*/
NTSTATUS
IncreaseMftSize(PDEVICE_EXTENSION Vcb, BOOLEAN CanWait)
{
    PNTFS_ATTR_CONTEXT BitmapContext;
    LARGE_INTEGER BitmapSize;
    LARGE_INTEGER DataSize;
    LONGLONG BitmapSizeDifference;
    ULONG NewRecords = ATTR_RECORD_ALIGNMENT * 8;  // Allocate one new record for every bit of every byte we'll be adding to the bitmap
    ULONG DataSizeDifference = Vcb->NtfsInfo.BytesPerFileRecord * NewRecords;
    ULONG BitmapOffset;
    PUCHAR BitmapBuffer;
    ULONGLONG BitmapBytes;
    ULONGLONG NewBitmapSize;
    ULONGLONG FirstNewMftIndex;
    ULONG BytesRead;
    ULONG LengthWritten;
    PFILE_RECORD_HEADER BlankFileRecord;
    ULONG i;
    NTSTATUS Status;

    DPRINT1("IncreaseMftSize(%p, %s)\n", Vcb, CanWait ? "TRUE" : "FALSE");

    // We need exclusive access to the mft while we change its size
    if (!ExAcquireResourceExclusiveLite(&(Vcb->DirResource), CanWait))
    {
        return STATUS_CANT_WAIT;
    }

    // Create a blank file record that will be used later
    BlankFileRecord = NtfsCreateEmptyFileRecord(Vcb);
    if (!BlankFileRecord)
    {
        DPRINT1("Error: Unable to create empty file record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Clear the flags (file record is not in use)
    BlankFileRecord->Flags = 0;

    // Find the bitmap attribute of master file table
    Status = FindAttribute(Vcb, Vcb->MasterFileTable, AttributeBitmap, L"", 0, &BitmapContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $BITMAP attribute of Mft!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        return Status;
    }

    // Get size of Bitmap Attribute
    BitmapSize.QuadPart = AttributeDataLength(BitmapContext->pRecord);

    // Calculate the new mft size
    DataSize.QuadPart = AttributeDataLength(Vcb->MFTContext->pRecord) + DataSizeDifference;

    // Find the index of the first Mft entry that will be created
    FirstNewMftIndex = AttributeDataLength(Vcb->MFTContext->pRecord) / Vcb->NtfsInfo.BytesPerFileRecord;

    // Determine how many bytes will make up the bitmap
    BitmapBytes = DataSize.QuadPart / Vcb->NtfsInfo.BytesPerFileRecord / 8;
    if ((DataSize.QuadPart / Vcb->NtfsInfo.BytesPerFileRecord) % 8 != 0)
        BitmapBytes++;

    // Windows will always keep the number of bytes in a bitmap as a multiple of 8, so no bytes are wasted on slack
    BitmapBytes = ALIGN_UP_BY(BitmapBytes, ATTR_RECORD_ALIGNMENT);

    // Determine how much we need to adjust the bitmap size (it's possible we don't)
    BitmapSizeDifference = BitmapBytes - BitmapSize.QuadPart;
    NewBitmapSize = max(BitmapSize.QuadPart + BitmapSizeDifference, BitmapSize.QuadPart);

    // Allocate memory for the bitmap
    BitmapBuffer = ExAllocatePoolWithTag(NonPagedPool, NewBitmapSize, TAG_NTFS);
    if (!BitmapBuffer)
    {
        DPRINT1("ERROR: Unable to allocate memory for bitmap attribute!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        ReleaseAttributeContext(BitmapContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Zero the bytes we'll be adding
    RtlZeroMemory(BitmapBuffer, NewBitmapSize);

    // Read the bitmap attribute
    BytesRead = ReadAttribute(Vcb,
                              BitmapContext,
                              0,
                              (PCHAR)BitmapBuffer,
                              BitmapSize.LowPart);
    if (BytesRead != BitmapSize.LowPart)
    {
        DPRINT1("ERROR: Bytes read != Bitmap size!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return STATUS_INVALID_PARAMETER;
    }

    // Increase the mft size
    Status = SetNonResidentAttributeDataLength(Vcb, Vcb->MFTContext, Vcb->MftDataOffset, Vcb->MasterFileTable, &DataSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to set size of $MFT data attribute!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // We'll need to find the bitmap again, because its offset will have changed after resizing the data attribute
    ReleaseAttributeContext(BitmapContext);
    Status = FindAttribute(Vcb, Vcb->MasterFileTable, AttributeBitmap, L"", 0, &BitmapContext, &BitmapOffset);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $BITMAP attribute of Mft!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        return Status;
    }

    // If the bitmap grew
    if (BitmapSizeDifference > 0)
    {
        // Set the new bitmap size
        BitmapSize.QuadPart = NewBitmapSize;
        if (BitmapContext->pRecord->IsNonResident)
            Status = SetNonResidentAttributeDataLength(Vcb, BitmapContext, BitmapOffset, Vcb->MasterFileTable, &BitmapSize);
        else
            Status = SetResidentAttributeDataLength(Vcb, BitmapContext, BitmapOffset, Vcb->MasterFileTable, &BitmapSize);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to set size of bitmap attribute!\n");
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
            ExReleaseResourceLite(&(Vcb->DirResource));
            ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
            ReleaseAttributeContext(BitmapContext);
            return Status;
        }
    }

    NtfsDumpFileAttributes(Vcb, Vcb->MasterFileTable);

    // Update the file record with the new attribute sizes
    Status = UpdateFileRecord(Vcb, Vcb->VolumeFcb->MFTIndex, Vcb->MasterFileTable);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to update $MFT file record!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // Write out the new bitmap
    Status = WriteAttribute(Vcb, BitmapContext, 0, BitmapBuffer, NewBitmapSize, &LengthWritten, Vcb->MasterFileTable);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        DPRINT1("ERROR: Couldn't write to bitmap attribute of $MFT!\n");
        return Status;
    }

    // Create blank records for the new file record entries.
    for (i = 0; i < NewRecords; i++)
    {
        Status = UpdateFileRecord(Vcb, FirstNewMftIndex + i, BlankFileRecord);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to write blank file record!\n");
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
            ExReleaseResourceLite(&(Vcb->DirResource));
            ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
            ReleaseAttributeContext(BitmapContext);
            return Status;
        }
    }

    // Update the mft mirror
    Status = UpdateMftMirror(Vcb);

    // Cleanup
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, BlankFileRecord);
    ExReleaseResourceLite(&(Vcb->DirResource));
    ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
    ReleaseAttributeContext(BitmapContext);

    return Status;
}

/**
* @name MoveAttributes
* @implemented
*
* Moves a block of attributes to a new location in the file Record. The attribute at FirstAttributeToMove
* and every attribute after that will be moved to MoveTo.
*
* @param DeviceExt
* Pointer to the DEVICE_EXTENSION (VCB) of the target volume.
*
* @param FirstAttributeToMove
* Pointer to the first NTFS_ATTR_RECORD that needs to be moved. This pointer must reside within a file record.
*
* @param FirstAttributeOffset
* Offset of FirstAttributeToMove relative to the beginning of the file record.
*
* @param MoveTo
* ULONG_PTR with the memory location that will be the new location of the first attribute being moved.
*
* @return
* The new location of the final attribute (i.e. AttributeEnd marker).
*/
PNTFS_ATTR_RECORD
MoveAttributes(PDEVICE_EXTENSION DeviceExt,
               PNTFS_ATTR_RECORD FirstAttributeToMove,
               ULONG FirstAttributeOffset,
               ULONG_PTR MoveTo)
{
    // Get the size of all attributes after this one
    ULONG MemBlockSize = 0;
    PNTFS_ATTR_RECORD CurrentAttribute = FirstAttributeToMove;
    ULONG CurrentOffset = FirstAttributeOffset;
    PNTFS_ATTR_RECORD FinalAttribute;

    while (CurrentAttribute->Type != AttributeEnd && CurrentOffset < DeviceExt->NtfsInfo.BytesPerFileRecord)
    {
        CurrentOffset += CurrentAttribute->Length;
        MemBlockSize += CurrentAttribute->Length;
        CurrentAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)CurrentAttribute + CurrentAttribute->Length);
    }

    FinalAttribute = (PNTFS_ATTR_RECORD)(MoveTo + MemBlockSize);
    MemBlockSize += sizeof(ULONG) * 2;  // Add the AttributeEnd and file record end

    ASSERT(MemBlockSize % ATTR_RECORD_ALIGNMENT == 0);

    // Move the attributes after this one
    RtlMoveMemory((PCHAR)MoveTo, FirstAttributeToMove, MemBlockSize);

    return FinalAttribute;
}

NTSTATUS
InternalSetResidentAttributeLength(PDEVICE_EXTENSION DeviceExt,
                                   PNTFS_ATTR_CONTEXT AttrContext,
                                   PFILE_RECORD_HEADER FileRecord,
                                   ULONG AttrOffset,
                                   ULONG DataSize)
{
    PNTFS_ATTR_RECORD Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
    PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Destination + Destination->Length);
    PNTFS_ATTR_RECORD FinalAttribute;
    ULONG OldAttributeLength = Destination->Length;
    ULONG NextAttributeOffset;

    DPRINT1("InternalSetResidentAttributeLength( %p, %p, %p, %lu, %lu )\n", DeviceExt, AttrContext, FileRecord, AttrOffset, DataSize);

    ASSERT(!AttrContext->pRecord->IsNonResident);

    // Update ValueLength Field
    Destination->Resident.ValueLength = DataSize;

    // Calculate the record length and end marker offset
    Destination->Length = ALIGN_UP_BY(DataSize + AttrContext->pRecord->Resident.ValueOffset, ATTR_RECORD_ALIGNMENT);
    NextAttributeOffset = AttrOffset + Destination->Length;

    // Ensure NextAttributeOffset is aligned to an 8-byte boundary
    ASSERT(NextAttributeOffset % ATTR_RECORD_ALIGNMENT == 0);

    // Will the new attribute be larger than the old one?
    if (Destination->Length > OldAttributeLength)
    {
        // Free the old copy of the attribute in the context, as it will be the wrong length
        ExFreePoolWithTag(AttrContext->pRecord, TAG_NTFS);

        // Create a new copy of the attribute record for the context
        AttrContext->pRecord = ExAllocatePoolWithTag(NonPagedPool, Destination->Length, TAG_NTFS);
        if (!AttrContext->pRecord)
        {
            DPRINT1("Unable to allocate memory for attribute!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory((PVOID)((ULONG_PTR)AttrContext->pRecord + OldAttributeLength), Destination->Length - OldAttributeLength);
        RtlCopyMemory(AttrContext->pRecord, Destination, OldAttributeLength);
    }

    // Are there attributes after this one that need to be moved?
    if (NextAttribute->Type != AttributeEnd)
    {
        // Move the attributes after this one
        FinalAttribute = MoveAttributes(DeviceExt, NextAttribute, NextAttributeOffset, (ULONG_PTR)Destination + Destination->Length);
    }
    else
    {
        // advance to the final "attribute," adjust for the changed length of the attribute we're resizing
        FinalAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)NextAttribute - OldAttributeLength + Destination->Length);
    }

    // Update pRecord's length
    AttrContext->pRecord->Length = Destination->Length;
    AttrContext->pRecord->Resident.ValueLength = DataSize;

    // set the file record end
    SetFileRecordEnd(FileRecord, FinalAttribute, FILE_RECORD_END);

    //NtfsDumpFileRecord(DeviceExt, FileRecord);

    return STATUS_SUCCESS;
}

/**
*   @parameter FileRecord
*   Pointer to a file record. Must be a full record at least
*   Fcb->Vcb->NtfsInfo.BytesPerFileRecord bytes large, not just the header.
*/
NTSTATUS
SetAttributeDataLength(PFILE_OBJECT FileObject,
                       PNTFS_FCB Fcb,
                       PNTFS_ATTR_CONTEXT AttrContext,
                       ULONG AttrOffset,
                       PFILE_RECORD_HEADER FileRecord,
                       PLARGE_INTEGER DataSize)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT1("SetAttributeDataLength(%p, %p, %p, %lu, %p, %I64u)\n",
            FileObject,
            Fcb,
            AttrContext,
            AttrOffset,
            FileRecord,
            DataSize->QuadPart);

    // are we truncating the file?
    if (DataSize->QuadPart < AttributeDataLength(AttrContext->pRecord))
    {
        if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer, DataSize))
        {
            DPRINT1("Can't truncate a memory-mapped file!\n");
            return STATUS_USER_MAPPED_FILE;
        }
    }

    if (AttrContext->pRecord->IsNonResident)
    {
        Status = SetNonResidentAttributeDataLength(Fcb->Vcb,
                                                   AttrContext,
                                                   AttrOffset,
                                                   FileRecord,
                                                   DataSize);
    }
    else
    {
        // resident attribute
        Status = SetResidentAttributeDataLength(Fcb->Vcb,
                                                AttrContext,
                                                AttrOffset,
                                                FileRecord,
                                                DataSize);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to set size of attribute!\n");
        return Status;
    }

    //NtfsDumpFileAttributes(Fcb->Vcb, FileRecord);

    // write the updated file record back to disk
    Status = UpdateFileRecord(Fcb->Vcb, Fcb->MFTIndex, FileRecord);

    if (NT_SUCCESS(Status))
    {
        if (AttrContext->pRecord->IsNonResident)
            Fcb->RFCB.AllocationSize.QuadPart = AttrContext->pRecord->NonResident.AllocatedSize;
        else
            Fcb->RFCB.AllocationSize = *DataSize;
        Fcb->RFCB.FileSize = *DataSize;
        Fcb->RFCB.ValidDataLength = *DataSize;
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
    }

    return STATUS_SUCCESS;
}

/**
* @name SetFileRecordEnd
* @implemented
*
* This small function sets a new endpoint for the file record. It set's the final
* AttrEnd->Type to AttributeEnd and recalculates the bytes used by the file record.
*
* @param FileRecord
* Pointer to the file record whose endpoint (length) will be set.
*
* @param AttrEnd
* Pointer to section of memory that will receive the AttributeEnd marker. This must point
* to memory allocated for the FileRecord. Must be aligned to an 8-byte boundary (relative to FileRecord).
*
* @param EndMarker
* This value will be written after AttributeEnd but isn't critical at all. When Windows resizes
* a file record, it preserves the final ULONG that previously ended the record, even though this
* value is (to my knowledge) never used. We emulate this behavior.
*
*/
VOID
SetFileRecordEnd(PFILE_RECORD_HEADER FileRecord,
                 PNTFS_ATTR_RECORD AttrEnd,
                 ULONG EndMarker)
{
    // Ensure AttrEnd is aligned on an 8-byte boundary, relative to FileRecord
    ASSERT(((ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord) % ATTR_RECORD_ALIGNMENT == 0);

    // mark the end of attributes
    AttrEnd->Type = AttributeEnd;

    // Restore the "file-record-end marker." The value is never checked but this behavior is consistent with Win2k3.
    AttrEnd->Length = EndMarker;

    // recalculate bytes in use
    FileRecord->BytesInUse = (ULONG_PTR)AttrEnd - (ULONG_PTR)FileRecord + sizeof(ULONG) * 2;
}

/**
* @name SetNonResidentAttributeDataLength
* @implemented
*
* Called by SetAttributeDataLength() to set the size of a non-resident attribute. Doesn't update the file record.
*
* @param Vcb
* Pointer to a DEVICE_EXTENSION describing the target disk.
*
* @param AttrContext
* PNTFS_ATTR_CONTEXT describing the location of the attribute whose size is being set.
*
* @param AttrOffset
* Offset, from the beginning of the record, of the attribute being sized.
*
* @param FileRecord
* Pointer to a file record containing the attribute to be resized. Must be a complete file record,
* not just the header.
*
* @param DataSize
* Pointer to a LARGE_INTEGER describing the new size of the attribute's data.
*
* @return
* STATUS_SUCCESS on success;
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_INVALID_PARAMETER if we can't find the last cluster in the data run.
*
* @remarks
* Called by SetAttributeDataLength() and IncreaseMftSize(). Use SetAttributeDataLength() unless you have a good
* reason to use this. Doesn't update the file record on disk. Doesn't inform the cache controller of changes with
* any associated files. Synchronization is the callers responsibility.
*/
NTSTATUS
SetNonResidentAttributeDataLength(PDEVICE_EXTENSION Vcb,
                                  PNTFS_ATTR_CONTEXT AttrContext,
                                  ULONG AttrOffset,
                                  PFILE_RECORD_HEADER FileRecord,
                                  PLARGE_INTEGER DataSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesPerCluster = Vcb->NtfsInfo.BytesPerCluster;
    ULONGLONG AllocationSize = ROUND_UP(DataSize->QuadPart, BytesPerCluster);
    PNTFS_ATTR_RECORD DestinationAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
    ULONG ExistingClusters = AttrContext->pRecord->NonResident.AllocatedSize / BytesPerCluster;

    ASSERT(AttrContext->pRecord->IsNonResident);

    // do we need to increase the allocation size?
    if (AttrContext->pRecord->NonResident.AllocatedSize < AllocationSize)
    {
        ULONG ClustersNeeded = (AllocationSize / BytesPerCluster) - ExistingClusters;
        LARGE_INTEGER LastClusterInDataRun;
        ULONG NextAssignedCluster;
        ULONG AssignedClusters;

        if (ExistingClusters == 0)
        {
            LastClusterInDataRun.QuadPart = 0;
        }
        else
        {
            if (!FsRtlLookupLargeMcbEntry(&AttrContext->DataRunsMCB,
                                          (LONGLONG)AttrContext->pRecord->NonResident.HighestVCN,
                                          (PLONGLONG)&LastClusterInDataRun.QuadPart,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL))
            {
                DPRINT1("Error looking up final large MCB entry!\n");

                // Most likely, HighestVCN went above the largest mapping
                DPRINT1("Highest VCN of record: %I64u\n", AttrContext->pRecord->NonResident.HighestVCN);
                return STATUS_INVALID_PARAMETER;
            }
        }

        DPRINT("LastClusterInDataRun: %I64u\n", LastClusterInDataRun.QuadPart);
        DPRINT("Highest VCN of record: %I64u\n", AttrContext->pRecord->NonResident.HighestVCN);

        while (ClustersNeeded > 0)
        {
            Status = NtfsAllocateClusters(Vcb,
                                          LastClusterInDataRun.LowPart + 1,
                                          ClustersNeeded,
                                          &NextAssignedCluster,
                                          &AssignedClusters);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Error: Unable to allocate requested clusters!\n");
                return Status;
            }

            // now we need to add the clusters we allocated to the data run
            Status = AddRun(Vcb, AttrContext, AttrOffset, FileRecord, NextAssignedCluster, AssignedClusters);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Error: Unable to add data run!\n");
                return Status;
            }

            ClustersNeeded -= AssignedClusters;
            LastClusterInDataRun.LowPart = NextAssignedCluster + AssignedClusters - 1;
        }
    }
    else if (AttrContext->pRecord->NonResident.AllocatedSize > AllocationSize)
    {
        // shrink allocation size
        ULONG ClustersToFree = ExistingClusters - (AllocationSize / BytesPerCluster);
        Status = FreeClusters(Vcb, AttrContext, AttrOffset, FileRecord, ClustersToFree);
    }

    // TODO: is the file compressed, encrypted, or sparse?

    AttrContext->pRecord->NonResident.AllocatedSize = AllocationSize;
    AttrContext->pRecord->NonResident.DataSize = DataSize->QuadPart;
    AttrContext->pRecord->NonResident.InitializedSize = DataSize->QuadPart;

    DestinationAttribute->NonResident.AllocatedSize = AllocationSize;
    DestinationAttribute->NonResident.DataSize = DataSize->QuadPart;
    DestinationAttribute->NonResident.InitializedSize = DataSize->QuadPart;

    // HighestVCN seems to be set incorrectly somewhere. Apply a hack-fix to reset it.
    // HACKHACK FIXME: Fix for sparse files; this math won't work in that case.
    AttrContext->pRecord->NonResident.HighestVCN = ((ULONGLONG)AllocationSize / Vcb->NtfsInfo.BytesPerCluster) - 1;
    DestinationAttribute->NonResident.HighestVCN = AttrContext->pRecord->NonResident.HighestVCN;

    DPRINT("Allocated Size: %I64u\n", DestinationAttribute->NonResident.AllocatedSize);

    return Status;
}

/**
* @name SetResidentAttributeDataLength
* @implemented
*
* Called by SetAttributeDataLength() to set the size of a non-resident attribute. Doesn't update the file record.
*
* @param Vcb
* Pointer to a DEVICE_EXTENSION describing the target disk.
*
* @param AttrContext
* PNTFS_ATTR_CONTEXT describing the location of the attribute whose size is being set.
*
* @param AttrOffset
* Offset, from the beginning of the record, of the attribute being sized.
*
* @param FileRecord
* Pointer to a file record containing the attribute to be resized. Must be a complete file record,
* not just the header.
*
* @param DataSize
* Pointer to a LARGE_INTEGER describing the new size of the attribute's data.
*
* @return
* STATUS_SUCCESS on success;
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_INVALID_PARAMETER if AttrContext describes a non-resident attribute.
* STATUS_NOT_IMPLEMENTED if requested to decrease the size of an attribute that isn't the
* last attribute listed in the file record.
*
* @remarks
* Called by SetAttributeDataLength() and IncreaseMftSize(). Use SetAttributeDataLength() unless you have a good
* reason to use this. Doesn't update the file record on disk. Doesn't inform the cache controller of changes with
* any associated files. Synchronization is the callers responsibility.
*/
NTSTATUS
SetResidentAttributeDataLength(PDEVICE_EXTENSION Vcb,
                               PNTFS_ATTR_CONTEXT AttrContext,
                               ULONG AttrOffset,
                               PFILE_RECORD_HEADER FileRecord,
                               PLARGE_INTEGER DataSize)
{
    NTSTATUS Status;

    // find the next attribute
    ULONG NextAttributeOffset = AttrOffset + AttrContext->pRecord->Length;
    PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((PCHAR)FileRecord + NextAttributeOffset);

    ASSERT(!AttrContext->pRecord->IsNonResident);

    //NtfsDumpFileAttributes(Vcb, FileRecord);

    // Do we need to increase the data length?
    if (DataSize->QuadPart > AttrContext->pRecord->Resident.ValueLength)
    {
        // There's usually padding at the end of a record. Do we need to extend past it?
        ULONG MaxValueLength = AttrContext->pRecord->Length - AttrContext->pRecord->Resident.ValueOffset;
        if (MaxValueLength < DataSize->LowPart)
        {
            // If this is the last attribute, we could move the end marker to the very end of the file record
            MaxValueLength += Vcb->NtfsInfo.BytesPerFileRecord - NextAttributeOffset - (sizeof(ULONG) * 2);

            if (MaxValueLength < DataSize->LowPart || NextAttribute->Type != AttributeEnd)
            {
                // convert attribute to non-resident
                PNTFS_ATTR_RECORD Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
                PNTFS_ATTR_RECORD NewRecord;
                LARGE_INTEGER AttribDataSize;
                PVOID AttribData;
                ULONG NewRecordLength;
                ULONG EndAttributeOffset;
                ULONG LengthWritten;

                DPRINT1("Converting attribute to non-resident.\n");

                AttribDataSize.QuadPart = AttrContext->pRecord->Resident.ValueLength;

                // Is there existing data we need to back-up?
                if (AttribDataSize.QuadPart > 0)
                {
                    AttribData = ExAllocatePoolWithTag(NonPagedPool, AttribDataSize.QuadPart, TAG_NTFS);
                    if (AttribData == NULL)
                    {
                        DPRINT1("ERROR: Couldn't allocate memory for attribute data. Can't migrate to non-resident!\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    // read data to temp buffer
                    Status = ReadAttribute(Vcb, AttrContext, 0, AttribData, AttribDataSize.QuadPart);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("ERROR: Unable to read attribute before migrating!\n");
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                        return Status;
                    }
                }

                // Start by turning this attribute into a 0-length, non-resident attribute, then enlarge it.

                // The size of a 0-length, non-resident attribute will be 0x41 + the size of the attribute name, aligned to an 8-byte boundary
                NewRecordLength = ALIGN_UP_BY(0x41 + (AttrContext->pRecord->NameLength * sizeof(WCHAR)), ATTR_RECORD_ALIGNMENT);

                // Create a new attribute record that will store the 0-length, non-resident attribute
                NewRecord = ExAllocatePoolWithTag(NonPagedPool, NewRecordLength, TAG_NTFS);

                // Zero out the NonResident structure
                RtlZeroMemory(NewRecord, NewRecordLength);

                // Copy the data that's common to both non-resident and resident attributes
                RtlCopyMemory(NewRecord, AttrContext->pRecord, FIELD_OFFSET(NTFS_ATTR_RECORD, Resident.ValueLength));

                // if there's a name
                if (AttrContext->pRecord->NameLength != 0)
                {
                    // copy the name
                    // An attribute name will be located at offset 0x18 for a resident attribute, 0x40 for non-resident
                    RtlCopyMemory((PCHAR)((ULONG_PTR)NewRecord + 0x40),
                                  (PCHAR)((ULONG_PTR)AttrContext->pRecord + 0x18),
                                  AttrContext->pRecord->NameLength * sizeof(WCHAR));
                }

                // update the mapping pairs offset, which will be 0x40 (size of a non-resident header) + length in bytes of the name
                NewRecord->NonResident.MappingPairsOffset = 0x40 + (AttrContext->pRecord->NameLength * sizeof(WCHAR));

                // update the end of the file record
                // calculate position of end markers (1 byte for empty data run)
                EndAttributeOffset = AttrOffset + NewRecord->NonResident.MappingPairsOffset + 1;
                EndAttributeOffset = ALIGN_UP_BY(EndAttributeOffset, ATTR_RECORD_ALIGNMENT);

                // Update the length
                NewRecord->Length = EndAttributeOffset - AttrOffset;

                ASSERT(NewRecord->Length == NewRecordLength);

                // Copy the new attribute record into the file record
                RtlCopyMemory(Destination, NewRecord, NewRecord->Length);

                // Update the file record end
                SetFileRecordEnd(FileRecord,
                                 (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + EndAttributeOffset),
                                 FILE_RECORD_END);

                // Initialize the MCB, potentially catch an exception
                _SEH2_TRY
                {
                    FsRtlInitializeLargeMcb(&AttrContext->DataRunsMCB, NonPagedPool);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    DPRINT1("Unable to create LargeMcb!\n");
                    if (AttribDataSize.QuadPart > 0)
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                    ExFreePoolWithTag(NewRecord, TAG_NTFS);
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                } _SEH2_END;

                // Mark the attribute as non-resident (we wait until after we know the LargeMcb was initialized)
                NewRecord->IsNonResident = Destination->IsNonResident = 1;

                // Update file record on disk
                Status = UpdateFileRecord(Vcb, AttrContext->FileMFTIndex, FileRecord);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ERROR: Couldn't update file record to continue migration!\n");
                    if (AttribDataSize.QuadPart > 0)
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                    ExFreePoolWithTag(NewRecord, TAG_NTFS);
                    return Status;
                }

                // Now we need to free the old copy of the attribute record in the context and replace it with the new one
                ExFreePoolWithTag(AttrContext->pRecord, TAG_NTFS);
                AttrContext->pRecord = NewRecord;

                // Now we can treat the attribute as non-resident and enlarge it normally
                Status = SetNonResidentAttributeDataLength(Vcb, AttrContext, AttrOffset, FileRecord, DataSize);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ERROR: Unable to migrate resident attribute!\n");
                    if (AttribDataSize.QuadPart > 0)
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                    return Status;
                }

                // restore the back-up attribute, if we made one
                if (AttribDataSize.QuadPart > 0)
                {
                    Status = WriteAttribute(Vcb, AttrContext, 0, AttribData, AttribDataSize.QuadPart, &LengthWritten, FileRecord);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("ERROR: Unable to write attribute data to non-resident clusters during migration!\n");
                        // TODO: Reverse migration so no data is lost
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                        return Status;
                    }

                    ExFreePoolWithTag(AttribData, TAG_NTFS);
                }
            }
        }
    }

    // set the new length of the resident attribute (if we didn't migrate it)
    if (!AttrContext->pRecord->IsNonResident)
        return InternalSetResidentAttributeLength(Vcb, AttrContext, FileRecord, AttrOffset, DataSize->LowPart);

    return STATUS_SUCCESS;
}

ULONG
ReadAttribute(PDEVICE_EXTENSION Vcb,
              PNTFS_ATTR_CONTEXT Context,
              ULONGLONG Offset,
              PCHAR Buffer,
              ULONG Length)
{
    ULONGLONG LastLCN;
    PUCHAR DataRun;
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG DataRunStartLCN;
    ULONGLONG CurrentOffset;
    ULONG ReadLength;
    ULONG AlreadyRead;
    NTSTATUS Status;

    //TEMPTEMP
    PUCHAR TempBuffer;

    if (!Context->pRecord->IsNonResident)
    {
        // We need to truncate Offset to a ULONG for pointer arithmetic
        // The check below should ensure that Offset is well within the range of 32 bits
        ULONG LittleOffset = (ULONG)Offset;

        // Ensure that offset isn't beyond the end of the attribute
        if (Offset > Context->pRecord->Resident.ValueLength)
            return 0;
        if (Offset + Length > Context->pRecord->Resident.ValueLength)
            Length = (ULONG)(Context->pRecord->Resident.ValueLength - Offset);

        RtlCopyMemory(Buffer, (PVOID)((ULONG_PTR)Context->pRecord + Context->pRecord->Resident.ValueOffset + LittleOffset), Length);
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
        //TEMPTEMP
        ULONG UsedBufferSize;
        TempBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
        if (TempBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        LastLCN = 0;
        CurrentOffset = 0;

        // This will be rewritten in the next iteration to just use the DataRuns MCB directly
        ConvertLargeMCBToDataRuns(&Context->DataRunsMCB,
                                  TempBuffer,
                                  Vcb->NtfsInfo.BytesPerFileRecord,
                                  &UsedBufferSize);

        DataRun = TempBuffer;

        while (1)
        {
            DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);
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
                Offset < CurrentOffset + (DataRunLength * Vcb->NtfsInfo.BytesPerCluster))
            {
                break;
            }

            if (*DataRun == 0)
            {
                ExFreePoolWithTag(TempBuffer, TAG_NTFS);
                return AlreadyRead;
            }

            CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
        }
    }

    /*
     * II. Go through the run list and read the data
     */

    ReadLength = (ULONG)min(DataRunLength * Vcb->NtfsInfo.BytesPerCluster - (Offset - CurrentOffset), Length);
    if (DataRunStartLCN == -1)
    {
        RtlZeroMemory(Buffer, ReadLength);
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = NtfsReadDisk(Vcb->StorageDevice,
                              DataRunStartLCN * Vcb->NtfsInfo.BytesPerCluster + Offset - CurrentOffset,
                              ReadLength,
                              Vcb->NtfsInfo.BytesPerSector,
                              (PVOID)Buffer,
                              FALSE);
    }
    if (NT_SUCCESS(Status))
    {
        Length -= ReadLength;
        Buffer += ReadLength;
        AlreadyRead += ReadLength;

        if (ReadLength == DataRunLength * Vcb->NtfsInfo.BytesPerCluster - (Offset - CurrentOffset))
        {
            CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
            DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);
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
            ReadLength = (ULONG)min(DataRunLength * Vcb->NtfsInfo.BytesPerCluster, Length);
            if (DataRunStartLCN == -1)
                RtlZeroMemory(Buffer, ReadLength);
            else
            {
                Status = NtfsReadDisk(Vcb->StorageDevice,
                                      DataRunStartLCN * Vcb->NtfsInfo.BytesPerCluster,
                                      ReadLength,
                                      Vcb->NtfsInfo.BytesPerSector,
                                      (PVOID)Buffer,
                                      FALSE);
                if (!NT_SUCCESS(Status))
                    break;
            }

            Length -= ReadLength;
            Buffer += ReadLength;
            AlreadyRead += ReadLength;

            /* We finished this request, but there still data in this data run. */
            if (Length == 0 && ReadLength != DataRunLength * Vcb->NtfsInfo.BytesPerCluster)
                break;

            /*
             * Go to next run in the list.
             */

            if (*DataRun == 0)
                break;
            CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
            DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);
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

    // TEMPTEMP
    if (Context->pRecord->IsNonResident)
        ExFreePoolWithTag(TempBuffer, TAG_NTFS);

    Context->CacheRun = DataRun;
    Context->CacheRunOffset = Offset + AlreadyRead;
    Context->CacheRunStartLCN = DataRunStartLCN;
    Context->CacheRunLength = DataRunLength;
    Context->CacheRunLastLCN = LastLCN;
    Context->CacheRunCurrentOffset = CurrentOffset;

    return AlreadyRead;
}


/**
* @name WriteAttribute
* @implemented
*
* Writes an NTFS attribute to the disk. It presently borrows a lot of code from ReadAttribute(),
* and it still needs more documentation / cleaning up.
*
* @param Vcb
* Volume Control Block indicating which volume to write the attribute to
*
* @param Context
* Pointer to an NTFS_ATTR_CONTEXT that has information about the attribute
*
* @param Offset
* Offset, in bytes, from the beginning of the attribute indicating where to start
* writing data
*
* @param Buffer
* The data that's being written to the device
*
* @param Length
* How much data will be written, in bytes
*
* @param RealLengthWritten
* Pointer to a ULONG which will receive how much data was written, in bytes
*
* @param FileRecord
* Optional pointer to a FILE_RECORD_HEADER that contains a copy of the file record
* being written to. Can be NULL, in which case the file record will be read from disk.
* If not-null, WriteAttribute() will skip reading from disk, and FileRecord
* will be updated with the newly-written attribute before the function returns.
*
* @return
* STATUS_SUCCESS if successful, an error code otherwise. STATUS_NOT_IMPLEMENTED if
* writing to a sparse file.
*
* @remarks Note that in this context the word "attribute" isn't referring read-only, hidden,
* etc. - the file's data is actually stored in an attribute in NTFS parlance.
*
*/

NTSTATUS
WriteAttribute(PDEVICE_EXTENSION Vcb,
               PNTFS_ATTR_CONTEXT Context,
               ULONGLONG Offset,
               const PUCHAR Buffer,
               ULONG Length,
               PULONG RealLengthWritten,
               PFILE_RECORD_HEADER FileRecord)
{
    ULONGLONG LastLCN;
    PUCHAR DataRun;
    LONGLONG DataRunOffset;
    ULONGLONG DataRunLength;
    LONGLONG DataRunStartLCN;
    ULONGLONG CurrentOffset;
    ULONG WriteLength;
    NTSTATUS Status;
    PUCHAR SourceBuffer = Buffer;
    LONGLONG StartingOffset;
    BOOLEAN FileRecordAllocated = FALSE;

    //TEMPTEMP
    PUCHAR TempBuffer;


    DPRINT("WriteAttribute(%p, %p, %I64u, %p, %lu, %p, %p)\n", Vcb, Context, Offset, Buffer, Length, RealLengthWritten, FileRecord);

    *RealLengthWritten = 0;

    // is this a resident attribute?
    if (!Context->pRecord->IsNonResident)
    {
        ULONG AttributeOffset;
        PNTFS_ATTR_CONTEXT FoundContext;
        PNTFS_ATTR_RECORD Destination;

        // Ensure requested data is within the bounds of the attribute
        ASSERT(Offset + Length <= Context->pRecord->Resident.ValueLength);

        if (Offset + Length > Context->pRecord->Resident.ValueLength)
        {
            DPRINT1("DRIVER ERROR: Attribute is too small!\n");
            return STATUS_INVALID_PARAMETER;
        }

        // Do we need to read the file record?
        if (FileRecord == NULL)
        {
            FileRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
            if (!FileRecord)
            {
                DPRINT1("Error: Couldn't allocate file record!\n");
                return STATUS_NO_MEMORY;
            }

            FileRecordAllocated = TRUE;

            // read the file record
            ReadFileRecord(Vcb, Context->FileMFTIndex, FileRecord);
        }

        // find where to write the attribute data to
        Status = FindAttribute(Vcb, FileRecord,
                               Context->pRecord->Type,
                               (PCWSTR)((ULONG_PTR)Context->pRecord + Context->pRecord->NameOffset),
                               Context->pRecord->NameLength,
                               &FoundContext,
                               &AttributeOffset);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Couldn't find matching attribute!\n");
            if(FileRecordAllocated)
                ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, FileRecord);
            return Status;
        }

        Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttributeOffset);

        DPRINT("Offset: %I64u, AttributeOffset: %u, ValueOffset: %u\n", Offset, AttributeOffset, Context->pRecord->Resident.ValueLength);

        // Will we be writing past the end of the allocated file record?
        if (Offset + Length + AttributeOffset + Context->pRecord->Resident.ValueOffset > Vcb->NtfsInfo.BytesPerFileRecord)
        {
            DPRINT1("DRIVER ERROR: Data being written extends past end of file record!\n");
            ReleaseAttributeContext(FoundContext);
            if (FileRecordAllocated)
                ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, FileRecord);
            return STATUS_INVALID_PARAMETER;
        }

        // copy the data being written into the file record. We cast Offset to ULONG, which is safe because it's range has been verified.
        RtlCopyMemory((PCHAR)((ULONG_PTR)Destination + Context->pRecord->Resident.ValueOffset + (ULONG)Offset), Buffer, Length);

        Status = UpdateFileRecord(Vcb, Context->FileMFTIndex, FileRecord);

        // Update the context's copy of the resident attribute
        ASSERT(Context->pRecord->Length == Destination->Length);
        RtlCopyMemory((PVOID)Context->pRecord, Destination, Context->pRecord->Length);

        ReleaseAttributeContext(FoundContext);
        if (FileRecordAllocated)
            ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, FileRecord);

        if (NT_SUCCESS(Status))
            *RealLengthWritten = Length;

        return Status;
    }

    // This is a non-resident attribute.

    // I. Find the corresponding start data run.

    // FIXME: Cache seems to be non-working. Disable it for now
    //if(Context->CacheRunOffset <= Offset && Offset < Context->CacheRunOffset + Context->CacheRunLength * Volume->ClusterSize)
    /*if (0)
    {
    DataRun = Context->CacheRun;
    LastLCN = Context->CacheRunLastLCN;
    DataRunStartLCN = Context->CacheRunStartLCN;
    DataRunLength = Context->CacheRunLength;
    CurrentOffset = Context->CacheRunCurrentOffset;
    }
    else*/
    {
        ULONG UsedBufferSize;
        LastLCN = 0;
        CurrentOffset = 0;

        // This will be rewritten in the next iteration to just use the DataRuns MCB directly
        TempBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
        if (TempBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ConvertLargeMCBToDataRuns(&Context->DataRunsMCB,
                                  TempBuffer,
                                  Vcb->NtfsInfo.BytesPerFileRecord,
                                  &UsedBufferSize);

        DataRun = TempBuffer;

        while (1)
        {
            DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);
            if (DataRunOffset != -1)
            {
                // Normal data run.
                // DPRINT1("Writing to normal data run, LastLCN %I64u DataRunOffset %I64d\n", LastLCN, DataRunOffset);
                DataRunStartLCN = LastLCN + DataRunOffset;
                LastLCN = DataRunStartLCN;
            }
            else
            {
                // Sparse data run. We can't support writing to sparse files yet
                // (it may require increasing the allocation size).
                DataRunStartLCN = -1;
                DPRINT1("FIXME: Writing to sparse files is not supported yet!\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto Cleanup;
            }

            // Have we reached the data run we're trying to write to?
            if (Offset >= CurrentOffset &&
                Offset < CurrentOffset + (DataRunLength * Vcb->NtfsInfo.BytesPerCluster))
            {
                break;
            }

            if (*DataRun == 0)
            {
                // We reached the last assigned cluster
                // TODO: assign new clusters to the end of the file.
                // (Presently, this code will rarely be reached, the write will usually have already failed by now)
                // [We can reach here by creating a new file record when the MFT isn't large enough]
                DPRINT1("FIXME: Master File Table needs to be enlarged.\n");
                Status = STATUS_END_OF_FILE;
                goto Cleanup;
            }

            CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
        }
    }

    // II. Go through the run list and write the data

    /* REVIEWME -- As adapted from NtfsReadAttribute():
    We seem to be making a special case for the first applicable data run, but I'm not sure why.
    Does it have something to do with (not) caching? Is this strategy equally applicable to writing? */

    WriteLength = (ULONG)min(DataRunLength * Vcb->NtfsInfo.BytesPerCluster - (Offset - CurrentOffset), Length);

    StartingOffset = DataRunStartLCN * Vcb->NtfsInfo.BytesPerCluster + Offset - CurrentOffset;

    // Write the data to the disk
    Status = NtfsWriteDisk(Vcb->StorageDevice,
                           StartingOffset,
                           WriteLength,
                           Vcb->NtfsInfo.BytesPerSector,
                           (PVOID)SourceBuffer);

    // Did the write fail?
    if (!NT_SUCCESS(Status))
    {
        Context->CacheRun = DataRun;
        Context->CacheRunOffset = Offset;
        Context->CacheRunStartLCN = DataRunStartLCN;
        Context->CacheRunLength = DataRunLength;
        Context->CacheRunLastLCN = LastLCN;
        Context->CacheRunCurrentOffset = CurrentOffset;

        goto Cleanup;
    }

    Length -= WriteLength;
    SourceBuffer += WriteLength;
    *RealLengthWritten += WriteLength;

    // Did we write to the end of the data run?
    if (WriteLength == DataRunLength * Vcb->NtfsInfo.BytesPerCluster - (Offset - CurrentOffset))
    {
        // Advance to the next data run
        CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
        DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);

        if (DataRunOffset != (ULONGLONG)-1)
        {
            DataRunStartLCN = LastLCN + DataRunOffset;
            LastLCN = DataRunStartLCN;
        }
        else
            DataRunStartLCN = -1;
    }

    // Do we have more data to write?
    while (Length > 0)
    {
        // Make sure we don't write past the end of the current data run
        WriteLength = (ULONG)min(DataRunLength * Vcb->NtfsInfo.BytesPerCluster, Length);

        // Are we dealing with a sparse data run?
        if (DataRunStartLCN == -1)
        {
            DPRINT1("FIXME: Don't know how to write to sparse files yet! (DataRunStartLCN == -1)\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto Cleanup;
        }
        else
        {
            // write the data to the disk
            Status = NtfsWriteDisk(Vcb->StorageDevice,
                                   DataRunStartLCN * Vcb->NtfsInfo.BytesPerCluster,
                                   WriteLength,
                                   Vcb->NtfsInfo.BytesPerSector,
                                   (PVOID)SourceBuffer);
            if (!NT_SUCCESS(Status))
                break;
        }

        Length -= WriteLength;
        SourceBuffer += WriteLength;
        *RealLengthWritten += WriteLength;

        // We finished this request, but there's still data in this data run.
        if (Length == 0 && WriteLength != DataRunLength * Vcb->NtfsInfo.BytesPerCluster)
            break;

        // Go to next run in the list.

        if (*DataRun == 0)
        {
            // that was the last run
            if (Length > 0)
            {
                // Failed sanity check.
                DPRINT1("Encountered EOF before expected!\n");
                Status = STATUS_END_OF_FILE;
                goto Cleanup;
            }

            break;
        }

        // Advance to the next data run
        CurrentOffset += DataRunLength * Vcb->NtfsInfo.BytesPerCluster;
        DataRun = DecodeRun(DataRun, &DataRunOffset, &DataRunLength);
        if (DataRunOffset != -1)
        {
            // Normal data run.
            DataRunStartLCN = LastLCN + DataRunOffset;
            LastLCN = DataRunStartLCN;
        }
        else
        {
            // Sparse data run.
            DataRunStartLCN = -1;
        }
    } // end while (Length > 0) [more data to write]

    Context->CacheRun = DataRun;
    Context->CacheRunOffset = Offset + *RealLengthWritten;
    Context->CacheRunStartLCN = DataRunStartLCN;
    Context->CacheRunLength = DataRunLength;
    Context->CacheRunLastLCN = LastLCN;
    Context->CacheRunCurrentOffset = CurrentOffset;

Cleanup:
    // TEMPTEMP
    if (Context->pRecord->IsNonResident)
        ExFreePoolWithTag(TempBuffer, TAG_NTFS);

    return Status;
}

NTSTATUS
ReadFileRecord(PDEVICE_EXTENSION Vcb,
               ULONGLONG index,
               PFILE_RECORD_HEADER file)
{
    ULONGLONG BytesRead;

    DPRINT("ReadFileRecord(%p, %I64x, %p)\n", Vcb, index, file);

    BytesRead = ReadAttribute(Vcb, Vcb->MFTContext, index * Vcb->NtfsInfo.BytesPerFileRecord, (PCHAR)file, Vcb->NtfsInfo.BytesPerFileRecord);
    if (BytesRead != Vcb->NtfsInfo.BytesPerFileRecord)
    {
        DPRINT1("ReadFileRecord failed: %I64u read, %lu expected\n", BytesRead, Vcb->NtfsInfo.BytesPerFileRecord);
        return STATUS_PARTIAL_COPY;
    }

    /* Apply update sequence array fixups. */
    DPRINT("Sequence number: %u\n", file->SequenceNumber);
    return FixupUpdateSequenceArray(Vcb, &file->Ntfs);
}


/**
* Searches a file's parent directory (given the parent's index in the mft)
* for the given file. Upon finding an index entry for that file, updates
* Data Size and Allocated Size values in the $FILE_NAME attribute of that entry.
*
* (Most of this code was copied from NtfsFindMftRecord)
*/
NTSTATUS
UpdateFileNameRecord(PDEVICE_EXTENSION Vcb,
                     ULONGLONG ParentMFTIndex,
                     PUNICODE_STRING FileName,
                     BOOLEAN DirSearch,
                     ULONGLONG NewDataSize,
                     ULONGLONG NewAllocationSize,
                     BOOLEAN CaseSensitive)
{
    PFILE_RECORD_HEADER MftRecord;
    PNTFS_ATTR_CONTEXT IndexRootCtx;
    PINDEX_ROOT_ATTRIBUTE IndexRoot;
    PCHAR IndexRecord;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry, IndexEntryEnd;
    NTSTATUS Status;
    ULONG CurrentEntry = 0;

    DPRINT("UpdateFileNameRecord(%p, %I64d, %wZ, %s, %I64u, %I64u, %s)\n",
           Vcb,
           ParentMFTIndex,
           FileName,
           DirSearch ? "TRUE" : "FALSE",
           NewDataSize,
           NewAllocationSize,
           CaseSensitive ? "TRUE" : "FALSE");

    MftRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, ParentMFTIndex, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return Status;
    }

    ASSERT(MftRecord->Ntfs.Type == NRH_FILE_TYPE);
    Status = FindAttribute(Vcb, MftRecord, AttributeIndexRoot, L"$I30", 4, &IndexRootCtx, NULL);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return Status;
    }

    IndexRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerIndexRecord, TAG_NTFS);
    if (IndexRecord == NULL)
    {
        ReleaseAttributeContext(IndexRootCtx);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadAttribute(Vcb, IndexRootCtx, 0, IndexRecord, AttributeDataLength(IndexRootCtx->pRecord));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to read Index Root!\n");
        ExFreePoolWithTag(IndexRecord, TAG_NTFS);
        ReleaseAttributeContext(IndexRootCtx);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return Status;
    }

    IndexRoot = (PINDEX_ROOT_ATTRIBUTE)IndexRecord;
    IndexEntry = (PINDEX_ENTRY_ATTRIBUTE)((PCHAR)&IndexRoot->Header + IndexRoot->Header.FirstEntryOffset);
    // Index root is always resident.
    IndexEntryEnd = (PINDEX_ENTRY_ATTRIBUTE)(IndexRecord + IndexRoot->Header.TotalSizeOfEntries);

    DPRINT("IndexRecordSize: %x IndexBlockSize: %x\n", Vcb->NtfsInfo.BytesPerIndexRecord, IndexRoot->SizeOfEntry);

    Status = UpdateIndexEntryFileNameSize(Vcb,
                                          MftRecord,
                                          IndexRecord,
                                          IndexRoot->SizeOfEntry,
                                          IndexEntry,
                                          IndexEntryEnd,
                                          FileName,
                                          &CurrentEntry,
                                          &CurrentEntry,
                                          DirSearch,
                                          NewDataSize,
                                          NewAllocationSize,
                                          CaseSensitive);

    if (Status == STATUS_PENDING)
    {
        // we need to write the index root attribute back to disk
        ULONG LengthWritten;
        Status = WriteAttribute(Vcb, IndexRootCtx, 0, (PUCHAR)IndexRecord, AttributeDataLength(IndexRootCtx->pRecord), &LengthWritten, MftRecord);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Couldn't update Index Root!\n");
        }

    }

    ReleaseAttributeContext(IndexRootCtx);
    ExFreePoolWithTag(IndexRecord, TAG_NTFS);
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);

    return Status;
}

/**
* Recursively searches directory index and applies the size update to the $FILE_NAME attribute of the
* proper index entry.
* (Heavily based on BrowseIndexEntries)
*/
NTSTATUS
UpdateIndexEntryFileNameSize(PDEVICE_EXTENSION Vcb,
                             PFILE_RECORD_HEADER MftRecord,
                             PCHAR IndexRecord,
                             ULONG IndexBlockSize,
                             PINDEX_ENTRY_ATTRIBUTE FirstEntry,
                             PINDEX_ENTRY_ATTRIBUTE LastEntry,
                             PUNICODE_STRING FileName,
                             PULONG StartEntry,
                             PULONG CurrentEntry,
                             BOOLEAN DirSearch,
                             ULONGLONG NewDataSize,
                             ULONGLONG NewAllocatedSize,
                             BOOLEAN CaseSensitive)
{
    NTSTATUS Status;
    ULONG RecordOffset;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;
    PNTFS_ATTR_CONTEXT IndexAllocationCtx;
    ULONGLONG IndexAllocationSize;
    PINDEX_BUFFER IndexBuffer;

    DPRINT("UpdateIndexEntrySize(%p, %p, %p, %lu, %p, %p, %wZ, %lu, %lu, %s, %I64u, %I64u, %s)\n",
           Vcb,
           MftRecord,
           IndexRecord,
           IndexBlockSize,
           FirstEntry,
           LastEntry,
           FileName,
           *StartEntry,
           *CurrentEntry,
           DirSearch ? "TRUE" : "FALSE",
           NewDataSize,
           NewAllocatedSize,
           CaseSensitive ? "TRUE" : "FALSE");

    // find the index entry responsible for the file we're trying to update
    IndexEntry = FirstEntry;
    while (IndexEntry < LastEntry &&
           !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
    {
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) > NTFS_FILE_FIRST_USER_FILE &&
            *CurrentEntry >= *StartEntry &&
            IndexEntry->FileName.NameType != NTFS_FILE_NAME_DOS &&
            CompareFileName(FileName, IndexEntry, DirSearch, CaseSensitive))
        {
            *StartEntry = *CurrentEntry;
            IndexEntry->FileName.DataSize = NewDataSize;
            IndexEntry->FileName.AllocatedSize = NewAllocatedSize;
            // indicate that the caller will still need to write the structure to the disk
            return STATUS_PENDING;
        }

        (*CurrentEntry) += 1;
        ASSERT(IndexEntry->Length >= sizeof(INDEX_ENTRY_ATTRIBUTE));
        IndexEntry = (PINDEX_ENTRY_ATTRIBUTE)((PCHAR)IndexEntry + IndexEntry->Length);
    }

    /* If we're already browsing a subnode */
    if (IndexRecord == NULL)
    {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    /* If there's no subnode */
    if (!(IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE))
    {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    Status = FindAttribute(Vcb, MftRecord, AttributeIndexAllocation, L"$I30", 4, &IndexAllocationCtx, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Corrupted filesystem!\n");
        return Status;
    }

    IndexAllocationSize = AttributeDataLength(IndexAllocationCtx->pRecord);
    Status = STATUS_OBJECT_PATH_NOT_FOUND;
    for (RecordOffset = 0; RecordOffset < IndexAllocationSize; RecordOffset += IndexBlockSize)
    {
        ReadAttribute(Vcb, IndexAllocationCtx, RecordOffset, IndexRecord, IndexBlockSize);
        Status = FixupUpdateSequenceArray(Vcb, &((PFILE_RECORD_HEADER)IndexRecord)->Ntfs);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        IndexBuffer = (PINDEX_BUFFER)IndexRecord;
        ASSERT(IndexBuffer->Ntfs.Type == NRH_INDX_TYPE);
        ASSERT(IndexBuffer->Header.AllocatedSize + FIELD_OFFSET(INDEX_BUFFER, Header) == IndexBlockSize);
        FirstEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)&IndexBuffer->Header + IndexBuffer->Header.FirstEntryOffset);
        LastEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)&IndexBuffer->Header + IndexBuffer->Header.TotalSizeOfEntries);
        ASSERT(LastEntry <= (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexBuffer + IndexBlockSize));

        Status = UpdateIndexEntryFileNameSize(NULL,
                                              NULL,
                                              NULL,
                                              0,
                                              FirstEntry,
                                              LastEntry,
                                              FileName,
                                              StartEntry,
                                              CurrentEntry,
                                              DirSearch,
                                              NewDataSize,
                                              NewAllocatedSize,
                                              CaseSensitive);
        if (Status == STATUS_PENDING)
        {
            // write the index record back to disk
            ULONG Written;

            // first we need to update the fixup values for the index block
            Status = AddFixupArray(Vcb, &((PFILE_RECORD_HEADER)IndexRecord)->Ntfs);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Error: Failed to update fixup sequence array!\n");
                break;
            }

            Status = WriteAttribute(Vcb, IndexAllocationCtx, RecordOffset, (const PUCHAR)IndexRecord, IndexBlockSize, &Written, MftRecord);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ERROR Performing write!\n");
                break;
            }

            Status = STATUS_SUCCESS;
            break;
        }
        if (NT_SUCCESS(Status))
        {
            break;
        }
    }

    ReleaseAttributeContext(IndexAllocationCtx);
    return Status;
}

/**
* @name UpdateFileRecord
* @implemented
*
* Writes a file record to the master file table, at a given index.
*
* @param Vcb
* Pointer to the DEVICE_EXTENSION of the target drive being written to.
*
* @param MftIndex
* Target index in the master file table to store the file record.
*
* @param FileRecord
* Pointer to the complete file record which will be written to the master file table.
*
* @return
* STATUS_SUCCESSFUL on success. An error passed from WriteAttribute() otherwise.
*
*/
NTSTATUS
UpdateFileRecord(PDEVICE_EXTENSION Vcb,
                 ULONGLONG MftIndex,
                 PFILE_RECORD_HEADER FileRecord)
{
    ULONG BytesWritten;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UpdateFileRecord(%p, 0x%I64x, %p)\n", Vcb, MftIndex, FileRecord);

    // Add the fixup array to prepare the data for writing to disk
    AddFixupArray(Vcb, &FileRecord->Ntfs);

    // write the file record to the master file table
    Status = WriteAttribute(Vcb,
                            Vcb->MFTContext,
                            MftIndex * Vcb->NtfsInfo.BytesPerFileRecord,
                            (const PUCHAR)FileRecord,
                            Vcb->NtfsInfo.BytesPerFileRecord,
                            &BytesWritten,
                            FileRecord);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UpdateFileRecord failed: %lu written, %lu expected\n", BytesWritten, Vcb->NtfsInfo.BytesPerFileRecord);
    }

    // remove the fixup array (so the file record pointer can still be used)
    FixupUpdateSequenceArray(Vcb, &FileRecord->Ntfs);

    return Status;
}


NTSTATUS
FixupUpdateSequenceArray(PDEVICE_EXTENSION Vcb,
                         PNTFS_RECORD_HEADER Record)
{
    USHORT *USA;
    USHORT USANumber;
    USHORT USACount;
    USHORT *Block;

    USA = (USHORT*)((PCHAR)Record + Record->UsaOffset);
    USANumber = *(USA++);
    USACount = Record->UsaCount - 1; /* Exclude the USA Number. */
    Block = (USHORT*)((PCHAR)Record + Vcb->NtfsInfo.BytesPerSector - 2);

    DPRINT("FixupUpdateSequenceArray(%p, %p)\nUSANumber: %u\tUSACount: %u\n", Vcb, Record, USANumber, USACount);

    while (USACount)
    {
        if (*Block != USANumber)
        {
            DPRINT1("Mismatch with USA: %u read, %u expected\n" , *Block, USANumber);
            return STATUS_UNSUCCESSFUL;
        }
        *Block = *(USA++);
        Block = (USHORT*)((PCHAR)Block + Vcb->NtfsInfo.BytesPerSector);
        USACount--;
    }

    return STATUS_SUCCESS;
}

/**
* @name AddNewMftEntry
* @implemented
*
* Adds a file record to the master file table of a given device.
*
* @param FileRecord
* Pointer to a complete file record which will be saved to disk.
*
* @param DeviceExt
* Pointer to the DEVICE_EXTENSION of the target drive.
*
* @param DestinationIndex
* Pointer to a ULONGLONG which will receive the MFT index where the file record was stored.
*
* @param CanWait
* Boolean indicating if the function is allowed to wait for exclusive access to the master file table.
* This will only be relevant if the MFT doesn't have any free file records and needs to be enlarged.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_OBJECT_NAME_NOT_FOUND if we can't find the MFT's $Bitmap or if we weren't able
* to read the attribute.
* STATUS_INSUFFICIENT_RESOURCES if we can't allocate enough memory for a copy of $Bitmap.
* STATUS_CANT_WAIT if CanWait was FALSE and the function could not get immediate, exclusive access to the MFT.
*/
NTSTATUS
AddNewMftEntry(PFILE_RECORD_HEADER FileRecord,
               PDEVICE_EXTENSION DeviceExt,
               PULONGLONG DestinationIndex,
               BOOLEAN CanWait)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONGLONG MftIndex;
    RTL_BITMAP Bitmap;
    ULONGLONG BitmapDataSize;
    ULONGLONG AttrBytesRead;
    PUCHAR BitmapData;
    PUCHAR BitmapBuffer;
    ULONG LengthWritten;
    PNTFS_ATTR_CONTEXT BitmapContext;
    LARGE_INTEGER BitmapBits;
    UCHAR SystemReservedBits;

    DPRINT1("AddNewMftEntry(%p, %p, %p, %s)\n", FileRecord, DeviceExt, DestinationIndex, CanWait ? "TRUE" : "FALSE");

    // First, we have to read the mft's $Bitmap attribute

    // Find the attribute
    Status = FindAttribute(DeviceExt, DeviceExt->MasterFileTable, AttributeBitmap, L"", 0, &BitmapContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $Bitmap attribute of master file table!\n");
        return Status;
    }

    // Get size of bitmap
    BitmapDataSize = AttributeDataLength(BitmapContext->pRecord);

    // RtlInitializeBitmap wants a ULONG-aligned pointer, and wants the memory passed to it to be a ULONG-multiple
    // Allocate a buffer for the $Bitmap attribute plus enough to ensure we can get a ULONG-aligned pointer
    BitmapBuffer = ExAllocatePoolWithTag(NonPagedPool, BitmapDataSize + sizeof(ULONG), TAG_NTFS);
    if (!BitmapBuffer)
    {
        ReleaseAttributeContext(BitmapContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(BitmapBuffer, BitmapDataSize + sizeof(ULONG));

    // Get a ULONG-aligned pointer for the bitmap itself
    BitmapData = (PUCHAR)ALIGN_UP_BY((ULONG_PTR)BitmapBuffer, sizeof(ULONG));

    // read $Bitmap attribute
    AttrBytesRead = ReadAttribute(DeviceExt, BitmapContext, 0, (PCHAR)BitmapData, BitmapDataSize);

    if (AttrBytesRead != BitmapDataSize)
    {
        DPRINT1("ERROR: Unable to read $Bitmap attribute of master file table!\n");
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    // We need to backup the bits for records 0x10 - 0x17 (3rd byte of bitmap) and mark these records
    // as in-use so we don't assign files to those indices. They're reserved for the system (e.g. ChkDsk).
    SystemReservedBits = BitmapData[2];
    BitmapData[2] = 0xff;

    // Calculate bit count
    BitmapBits.QuadPart = AttributeDataLength(DeviceExt->MFTContext->pRecord) /
                          DeviceExt->NtfsInfo.BytesPerFileRecord;
    if (BitmapBits.HighPart != 0)
    {
        DPRINT1("\tFIXME: bitmap sizes beyond 32bits are not yet supported! (Your NTFS volume is too large)\n");
        NtfsGlobalData->EnableWriteSupport = FALSE;
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return STATUS_NOT_IMPLEMENTED;
    }

    // convert buffer into bitmap
    RtlInitializeBitMap(&Bitmap, (PULONG)BitmapData, BitmapBits.LowPart);

    // set next available bit, preferrably after 23rd bit
    MftIndex = RtlFindClearBitsAndSet(&Bitmap, 1, 24);
    if ((LONG)MftIndex == -1)
    {
        DPRINT1("Couldn't find free space in MFT for file record, increasing MFT size.\n");

        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);

        // Couldn't find a free record in the MFT, add some blank records and try again
        Status = IncreaseMftSize(DeviceExt, CanWait);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Couldn't find space in MFT for file or increase MFT size!\n");
            return Status;
        }

        return AddNewMftEntry(FileRecord, DeviceExt, DestinationIndex, CanWait);
    }

    DPRINT1("Creating file record at MFT index: %I64u\n", MftIndex);

    // update file record with index
    FileRecord->MFTRecordNumber = MftIndex;

    // [BitmapData should have been updated via RtlFindClearBitsAndSet()]

    // Restore the system reserved bits
    BitmapData[2] = SystemReservedBits;

    // write the bitmap back to the MFT's $Bitmap attribute
    Status = WriteAttribute(DeviceExt, BitmapContext, 0, BitmapData, BitmapDataSize, &LengthWritten, FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR encountered when writing $Bitmap attribute!\n");
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // update the file record (write it to disk)
    Status = UpdateFileRecord(DeviceExt, MftIndex, FileRecord);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write file record!\n");
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    *DestinationIndex = MftIndex;

    ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
    ReleaseAttributeContext(BitmapContext);

    return Status;
}

/**
* @name NtfsAddFilenameToDirectory
* @implemented
*
* Adds a $FILE_NAME attribute to a given directory index.
*
* @param DeviceExt
* Points to the target disk's DEVICE_EXTENSION.
*
* @param DirectoryMftIndex
* Mft index of the parent directory which will receive the file.
*
* @param FileReferenceNumber
* File reference of the file to be added to the directory. This is a combination of the
* Mft index and sequence number.
*
* @param FilenameAttribute
* Pointer to the FILENAME_ATTRIBUTE of the file being added to the directory.
*
* @param CaseSensitive
* Boolean indicating if the function should operate in case-sensitive mode. This will be TRUE
* if an application created the file with the FILE_FLAG_POSIX_SEMANTICS flag.
*
* @return
* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation fails.
* STATUS_NOT_IMPLEMENTED if target address isn't at the end of the given file record.
*
* @remarks
* WIP - Can only support a few files in a directory.
* One FILENAME_ATTRIBUTE is added to the directory's index for each link to that file. So, each
* file which contains one FILENAME_ATTRIBUTE for a long name and another for the 8.3 name, will
* get both attributes added to its parent directory.
*/
NTSTATUS
NtfsAddFilenameToDirectory(PDEVICE_EXTENSION DeviceExt,
                           ULONGLONG DirectoryMftIndex,
                           ULONGLONG FileReferenceNumber,
                           PFILENAME_ATTRIBUTE FilenameAttribute,
                           BOOLEAN CaseSensitive)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_RECORD_HEADER ParentFileRecord;
    PNTFS_ATTR_CONTEXT IndexRootContext;
    PINDEX_ROOT_ATTRIBUTE I30IndexRoot;
    ULONG IndexRootOffset;
    ULONGLONG I30IndexRootLength;
    ULONG LengthWritten;
    PINDEX_ROOT_ATTRIBUTE NewIndexRoot;
    ULONG AttributeLength;
    PNTFS_ATTR_RECORD NextAttribute;
    PB_TREE NewTree;
    ULONG BtreeIndexLength;
    ULONG MaxIndexRootSize;
    PB_TREE_KEY NewLeftKey;
    PB_TREE_FILENAME_NODE NewRightHandNode;
    LARGE_INTEGER MinIndexRootSize;
    ULONG NewMaxIndexRootSize;
    ULONG NodeSize;

    // Allocate memory for the parent directory
    ParentFileRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (!ParentFileRecord)
    {
        DPRINT1("ERROR: Couldn't allocate memory for file record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Open the parent directory
    Status = ReadFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        DPRINT1("ERROR: Couldn't read parent directory with index %I64u\n",
                DirectoryMftIndex);
        return Status;
    }

#ifndef NDEBUG
    DPRINT1("Dumping old parent file record:\n");
    NtfsDumpFileRecord(DeviceExt, ParentFileRecord);
#endif

    // Find the index root attribute for the directory
    Status = FindAttribute(DeviceExt,
                           ParentFileRecord,
                           AttributeIndexRoot,
                           L"$I30",
                           4,
                           &IndexRootContext,
                           &IndexRootOffset);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $I30 $INDEX_ROOT attribute for parent directory with MFT #: %I64u!\n",
                DirectoryMftIndex);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

    // Find the maximum index size given what the file record can hold
    // First, find the max index size assuming index root is the last attribute
    MaxIndexRootSize = DeviceExt->NtfsInfo.BytesPerFileRecord               // Start with the size of a file record
                       - IndexRootOffset                                    // Subtract the length of everything that comes before index root
                       - IndexRootContext->pRecord->Resident.ValueOffset    // Subtract the length of the attribute header for index root
                       - sizeof(INDEX_ROOT_ATTRIBUTE)                       // Subtract the length of the index root header
                       - (sizeof(ULONG) * 2);                               // Subtract the length of the file record end marker and padding

    // Are there attributes after this one?
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)ParentFileRecord + IndexRootOffset + IndexRootContext->pRecord->Length);
    if (NextAttribute->Type != AttributeEnd)
    {
        // Find the length of all attributes after this one, not counting the end marker
        ULONG LengthOfAttributes = 0;
        PNTFS_ATTR_RECORD CurrentAttribute = NextAttribute;
        while (CurrentAttribute->Type != AttributeEnd)
        {
            LengthOfAttributes += CurrentAttribute->Length;
            CurrentAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)CurrentAttribute + CurrentAttribute->Length);
        }

        // Leave room for the existing attributes
        MaxIndexRootSize -= LengthOfAttributes;
    }

    // Allocate memory for the index root data
    I30IndexRootLength = AttributeDataLength(IndexRootContext->pRecord);
    I30IndexRoot = ExAllocatePoolWithTag(NonPagedPool, I30IndexRootLength, TAG_NTFS);
    if (!I30IndexRoot)
    {
        DPRINT1("ERROR: Couldn't allocate memory for index root attribute!\n");
        ReleaseAttributeContext(IndexRootContext);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Read the Index Root
    Status = ReadAttribute(DeviceExt, IndexRootContext, 0, (PCHAR)I30IndexRoot, I30IndexRootLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couln't read index root attribute for Mft index #%I64u\n", DirectoryMftIndex);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

    // Convert the index to a B*Tree
    Status = CreateBTreeFromIndex(DeviceExt,
                                  ParentFileRecord,
                                  IndexRootContext,
                                  I30IndexRoot,
                                  &NewTree);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to create B-Tree from Index!\n");
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

#ifndef NDEBUG
    DumpBTree(NewTree);
#endif

    // Insert the key for the file we're adding
    Status = NtfsInsertKey(NewTree,
                           FileReferenceNumber,
                           FilenameAttribute,
                           NewTree->RootNode,
                           CaseSensitive,
                           MaxIndexRootSize,
                           I30IndexRoot->SizeOfEntry,
                           &NewLeftKey,
                           &NewRightHandNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to insert key into B-Tree!\n");
        DestroyBTree(NewTree);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

#ifndef NDEBUG
    DumpBTree(NewTree);
#endif

    // The root node can't be split
    ASSERT(NewLeftKey == NULL);
    ASSERT(NewRightHandNode == NULL);

    // Convert B*Tree back to Index

    // Updating the index allocation can change the size available for the index root,
    // And if the index root is demoted, the index allocation will need to be updated again,
    // which may change the size available for index root... etc.
    // My solution is to decrease index root to the size it would be if it was demoted,
    // then UpdateIndexAllocation will have an accurate representation of the maximum space
    // it can use in the file record. There's still a chance that the act of allocating an
    // index node after demoting the index root will increase the size of the file record beyond
    // it's limit, but if that happens, an attribute-list will most definitely be needed.
    // This a bit hacky, but it seems to be functional.

    // Calculate the minimum size of the index root attribute, considering one dummy key and one VCN
    MinIndexRootSize.QuadPart = sizeof(INDEX_ROOT_ATTRIBUTE) // size of the index root headers
                                + 0x18; // Size of dummy key with a VCN for a subnode
    ASSERT(MinIndexRootSize.QuadPart % ATTR_RECORD_ALIGNMENT == 0);

    // Temporarily shrink the index root to it's minimal size
    AttributeLength = MinIndexRootSize.LowPart;
    AttributeLength += sizeof(INDEX_ROOT_ATTRIBUTE);


    // FIXME: IndexRoot will probably be invalid until we're finished. If we fail before we finish, the directory will probably be toast.
    // The potential for catastrophic data-loss exists!!! :)

    // Update the length of the attribute in the file record of the parent directory
    Status = InternalSetResidentAttributeLength(DeviceExt,
                                                IndexRootContext,
                                                ParentFileRecord,
                                                IndexRootOffset,
                                                AttributeLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to set length of index root!\n");
        DestroyBTree(NewTree);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

    // Update the index allocation
    Status = UpdateIndexAllocation(DeviceExt, NewTree, I30IndexRoot->SizeOfEntry, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to update index allocation from B-Tree!\n");
        DestroyBTree(NewTree);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

#ifndef NDEBUG
    DPRINT1("Index Allocation updated\n");
    DumpBTree(NewTree);
#endif

    // Find the maximum index root size given what the file record can hold
    // First, find the max index size assuming index root is the last attribute
    NewMaxIndexRootSize =
       DeviceExt->NtfsInfo.BytesPerFileRecord                // Start with the size of a file record
        - IndexRootOffset                                    // Subtract the length of everything that comes before index root
        - IndexRootContext->pRecord->Resident.ValueOffset    // Subtract the length of the attribute header for index root
        - sizeof(INDEX_ROOT_ATTRIBUTE)                       // Subtract the length of the index root header
        - (sizeof(ULONG) * 2);                               // Subtract the length of the file record end marker and padding

    // Are there attributes after this one?
    NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)ParentFileRecord + IndexRootOffset + IndexRootContext->pRecord->Length);
    if (NextAttribute->Type != AttributeEnd)
    {
        // Find the length of all attributes after this one, not counting the end marker
        ULONG LengthOfAttributes = 0;
        PNTFS_ATTR_RECORD CurrentAttribute = NextAttribute;
        while (CurrentAttribute->Type != AttributeEnd)
        {
            LengthOfAttributes += CurrentAttribute->Length;
            CurrentAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)CurrentAttribute + CurrentAttribute->Length);
        }

        // Leave room for the existing attributes
        NewMaxIndexRootSize -= LengthOfAttributes;
    }

    // The index allocation and index bitmap may have grown, leaving less room for the index root,
    // so now we need to double-check that index root isn't too large
    NodeSize = GetSizeOfIndexEntries(NewTree->RootNode);
    if (NodeSize > NewMaxIndexRootSize)
    {
        DPRINT1("Demoting index root.\nNodeSize: 0x%lx\nNewMaxIndexRootSize: 0x%lx\n", NodeSize, NewMaxIndexRootSize);

        Status = DemoteBTreeRoot(NewTree);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to demote index root!\n");
            DestroyBTree(NewTree);
            ReleaseAttributeContext(IndexRootContext);
            ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
            return Status;
        }

        // We need to update the index allocation once more
        Status = UpdateIndexAllocation(DeviceExt, NewTree, I30IndexRoot->SizeOfEntry, ParentFileRecord);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to update index allocation from B-Tree!\n");
            DestroyBTree(NewTree);
            ReleaseAttributeContext(IndexRootContext);
            ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
            return Status;
        }

        // re-recalculate max size of index root
        NewMaxIndexRootSize =
            // Find the maximum index size given what the file record can hold
            // First, find the max index size assuming index root is the last attribute
            DeviceExt->NtfsInfo.BytesPerFileRecord               // Start with the size of a file record
            - IndexRootOffset                                    // Subtract the length of everything that comes before index root
            - IndexRootContext->pRecord->Resident.ValueOffset    // Subtract the length of the attribute header for index root
            - sizeof(INDEX_ROOT_ATTRIBUTE)                       // Subtract the length of the index root header
            - (sizeof(ULONG) * 2);                               // Subtract the length of the file record end marker and padding

                                                                 // Are there attributes after this one?
        NextAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)ParentFileRecord + IndexRootOffset + IndexRootContext->pRecord->Length);
        if (NextAttribute->Type != AttributeEnd)
        {
            // Find the length of all attributes after this one, not counting the end marker
            ULONG LengthOfAttributes = 0;
            PNTFS_ATTR_RECORD CurrentAttribute = NextAttribute;
            while (CurrentAttribute->Type != AttributeEnd)
            {
                LengthOfAttributes += CurrentAttribute->Length;
                CurrentAttribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)CurrentAttribute + CurrentAttribute->Length);
            }

            // Leave room for the existing attributes
            NewMaxIndexRootSize -= LengthOfAttributes;
        }


    }

    // Create the Index Root from the B*Tree
    Status = CreateIndexRootFromBTree(DeviceExt, NewTree, NewMaxIndexRootSize, &NewIndexRoot, &BtreeIndexLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to create Index root from B-Tree!\n");
        DestroyBTree(NewTree);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

    // We're done with the B-Tree now
    DestroyBTree(NewTree);

    // Write back the new index root attribute to the parent directory file record

    // First, we need to resize the attribute.
    // CreateIndexRootFromBTree() should have verified that the index root fits within MaxIndexSize.
    // We can't set the size as we normally would, because $INDEX_ROOT must always be resident.
    AttributeLength = NewIndexRoot->Header.AllocatedSize + FIELD_OFFSET(INDEX_ROOT_ATTRIBUTE, Header);

    if (AttributeLength != IndexRootContext->pRecord->Resident.ValueLength)
    {
        // Update the length of the attribute in the file record of the parent directory
        Status = InternalSetResidentAttributeLength(DeviceExt,
                                                    IndexRootContext,
                                                    ParentFileRecord,
                                                    IndexRootOffset,
                                                    AttributeLength);
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
            ReleaseAttributeContext(IndexRootContext);
            ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
            ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
            DPRINT1("ERROR: Unable to set resident attribute length!\n");
            return Status;
        }

    }

    NT_ASSERT(ParentFileRecord->BytesInUse <= DeviceExt->NtfsInfo.BytesPerFileRecord);

    Status = UpdateFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to update file record of directory with index: %llx\n", DirectoryMftIndex);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        return Status;
    }

    // Write the new index root to disk
    Status = WriteAttribute(DeviceExt,
                            IndexRootContext,
                            0,
                            (PUCHAR)NewIndexRoot,
                            AttributeLength,
                            &LengthWritten,
                            ParentFileRecord);
    if (!NT_SUCCESS(Status) || LengthWritten != AttributeLength)
    {
        DPRINT1("ERROR: Unable to write new index root attribute to parent directory!\n");
        ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
        ReleaseAttributeContext(IndexRootContext);
        ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
        ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);
        return Status;
    }

    // re-read the parent file record, so we can dump it
    Status = ReadFileRecord(DeviceExt, DirectoryMftIndex, ParentFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't read parent directory after messing with it!\n");
    }
    else
    {
#ifndef NDEBUG
        DPRINT1("Dumping new B-Tree:\n");

        Status = CreateBTreeFromIndex(DeviceExt, ParentFileRecord, IndexRootContext, NewIndexRoot, &NewTree);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Couldn't re-create b-tree\n");
            return Status;
        }

        DumpBTree(NewTree);

        DestroyBTree(NewTree);

        NtfsDumpFileRecord(DeviceExt, ParentFileRecord);
#endif
    }

    // Cleanup
    ExFreePoolWithTag(NewIndexRoot, TAG_NTFS);
    ReleaseAttributeContext(IndexRootContext);
    ExFreePoolWithTag(I30IndexRoot, TAG_NTFS);
    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, ParentFileRecord);

    return Status;
}

NTSTATUS
AddFixupArray(PDEVICE_EXTENSION Vcb,
              PNTFS_RECORD_HEADER Record)
{
    USHORT *pShortToFixUp;
    ULONG ArrayEntryCount = Record->UsaCount - 1;
    ULONG Offset = Vcb->NtfsInfo.BytesPerSector - 2;
    ULONG i;

    PFIXUP_ARRAY fixupArray = (PFIXUP_ARRAY)((UCHAR*)Record + Record->UsaOffset);

    DPRINT("AddFixupArray(%p, %p)\n fixupArray->USN: %u, ArrayEntryCount: %u\n", Vcb, Record, fixupArray->USN, ArrayEntryCount);

    fixupArray->USN++;

    for (i = 0; i < ArrayEntryCount; i++)
    {
        DPRINT("USN: %u\tOffset: %u\n", fixupArray->USN, Offset);

        pShortToFixUp = (USHORT*)((PCHAR)Record + Offset);
        fixupArray->Array[i] = *pShortToFixUp;
        *pShortToFixUp = fixupArray->USN;
        Offset += Vcb->NtfsInfo.BytesPerSector;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ReadLCN(PDEVICE_EXTENSION Vcb,
        ULONGLONG lcn,
        ULONG count,
        PVOID buffer)
{
    LARGE_INTEGER DiskSector;

    DiskSector.QuadPart = lcn;

    return NtfsReadSectors(Vcb->StorageDevice,
                           DiskSector.u.LowPart * Vcb->NtfsInfo.SectorsPerCluster,
                           count * Vcb->NtfsInfo.SectorsPerCluster,
                           Vcb->NtfsInfo.BytesPerSector,
                           buffer,
                           FALSE);
}


BOOLEAN
CompareFileName(PUNICODE_STRING FileName,
                PINDEX_ENTRY_ATTRIBUTE IndexEntry,
                BOOLEAN DirSearch,
                BOOLEAN CaseSensitive)
{
    BOOLEAN Ret, Alloc = FALSE;
    UNICODE_STRING EntryName;

    EntryName.Buffer = IndexEntry->FileName.Name;
    EntryName.Length =
    EntryName.MaximumLength = IndexEntry->FileName.NameLength * sizeof(WCHAR);

    if (DirSearch)
    {
        UNICODE_STRING IntFileName;
        if (!CaseSensitive)
        {
            NT_VERIFY(NT_SUCCESS(RtlUpcaseUnicodeString(&IntFileName, FileName, TRUE)));
            Alloc = TRUE;
        }
        else
        {
            IntFileName = *FileName;
        }

        Ret = FsRtlIsNameInExpression(&IntFileName, &EntryName, !CaseSensitive, NULL);

        if (Alloc)
        {
            RtlFreeUnicodeString(&IntFileName);
        }

        return Ret;
    }
    else
    {
        return (RtlCompareUnicodeString(FileName, &EntryName, !CaseSensitive) == 0);
    }
}

/**
* @name UpdateMftMirror
* @implemented
*
* Backs-up the first ~4 master file table entries to the $MFTMirr file.
*
* @param Vcb
* Pointer to an NTFS_VCB for the volume whose Mft mirror is being updated.
*
* @return

* STATUS_SUCCESS on success.
* STATUS_INSUFFICIENT_RESOURCES if an allocation failed.
* STATUS_UNSUCCESSFUL if we couldn't read the master file table.
*
* @remarks
* NTFS maintains up-to-date copies of the first several mft entries in the $MFTMirr file. Usually, the first 4 file
* records from the mft are stored. The exact number of entries is determined by the size of $MFTMirr's $DATA.
* If $MFTMirr is not up-to-date, chkdsk will reject every change it can find prior to when $MFTMirr was last updated.
* Therefore, it's recommended to call this function if the volume changes considerably. For instance, IncreaseMftSize()
* relies on this function to keep chkdsk from deleting the mft entries it creates. Note that under most instances, creating
* or deleting a file will not affect the first ~four mft entries, and so will not require updating the mft mirror.
*/
NTSTATUS
UpdateMftMirror(PNTFS_VCB Vcb)
{
    PFILE_RECORD_HEADER MirrorFileRecord;
    PNTFS_ATTR_CONTEXT MirrDataContext;
    PNTFS_ATTR_CONTEXT MftDataContext;
    PCHAR DataBuffer;
    ULONGLONG DataLength;
    NTSTATUS Status;
    ULONG BytesRead;
    ULONG LengthWritten;

    // Allocate memory for the Mft mirror file record
    MirrorFileRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (!MirrorFileRecord)
    {
        DPRINT1("Error: Failed to allocate memory for $MFTMirr!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Read the Mft Mirror file record
    Status = ReadFileRecord(Vcb, NTFS_FILE_MFTMIRR, MirrorFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to read $MFTMirr!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);
        return Status;
    }

    // Find the $DATA attribute of $MFTMirr
    Status = FindAttribute(Vcb, MirrorFileRecord, AttributeData, L"", 0, &MirrDataContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $DATA attribute!\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);
        return Status;
    }

    // Find the $DATA attribute of $MFT
    Status = FindAttribute(Vcb, Vcb->MasterFileTable, AttributeData, L"", 0, &MftDataContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $DATA attribute!\n");
        ReleaseAttributeContext(MirrDataContext);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);
        return Status;
    }

    // Get the size of the mirror's $DATA attribute
    DataLength = AttributeDataLength(MirrDataContext->pRecord);

    ASSERT(DataLength % Vcb->NtfsInfo.BytesPerFileRecord == 0);

    // Create buffer for the mirror's $DATA attribute
    DataBuffer = ExAllocatePoolWithTag(NonPagedPool, DataLength, TAG_NTFS);
    if (!DataBuffer)
    {
        DPRINT1("Error: Couldn't allocate memory for $DATA buffer!\n");
        ReleaseAttributeContext(MftDataContext);
        ReleaseAttributeContext(MirrDataContext);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(DataLength < ULONG_MAX);

    // Back up the first several entries of the Mft's $DATA Attribute
    BytesRead = ReadAttribute(Vcb, MftDataContext, 0, DataBuffer, (ULONG)DataLength);
    if (BytesRead != (ULONG)DataLength)
    {
        DPRINT1("Error: Failed to read $DATA for $MFTMirr!\n");
        ReleaseAttributeContext(MftDataContext);
        ReleaseAttributeContext(MirrDataContext);
        ExFreePoolWithTag(DataBuffer, TAG_NTFS);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);
        return STATUS_UNSUCCESSFUL;
    }

    // Write the mirror's $DATA attribute
    Status = WriteAttribute(Vcb,
                             MirrDataContext,
                             0,
                             (PUCHAR)DataBuffer,
                             DataLength,
                             &LengthWritten,
                             MirrorFileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to write $DATA attribute of $MFTMirr!\n");
    }

    // Cleanup
    ReleaseAttributeContext(MftDataContext);
    ReleaseAttributeContext(MirrDataContext);
    ExFreePoolWithTag(DataBuffer, TAG_NTFS);
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MirrorFileRecord);

    return Status;
}

#if 0
static
VOID
DumpIndexEntry(PINDEX_ENTRY_ATTRIBUTE IndexEntry)
{
    DPRINT1("Entry: %p\n", IndexEntry);
    DPRINT1("\tData.Directory.IndexedFile: %I64x\n", IndexEntry->Data.Directory.IndexedFile);
    DPRINT1("\tLength: %u\n", IndexEntry->Length);
    DPRINT1("\tKeyLength: %u\n", IndexEntry->KeyLength);
    DPRINT1("\tFlags: %x\n", IndexEntry->Flags);
    DPRINT1("\tReserved: %x\n", IndexEntry->Reserved);
    DPRINT1("\t\tDirectoryFileReferenceNumber: %I64x\n", IndexEntry->FileName.DirectoryFileReferenceNumber);
    DPRINT1("\t\tCreationTime: %I64u\n", IndexEntry->FileName.CreationTime);
    DPRINT1("\t\tChangeTime: %I64u\n", IndexEntry->FileName.ChangeTime);
    DPRINT1("\t\tLastWriteTime: %I64u\n", IndexEntry->FileName.LastWriteTime);
    DPRINT1("\t\tLastAccessTime: %I64u\n", IndexEntry->FileName.LastAccessTime);
    DPRINT1("\t\tAllocatedSize: %I64u\n", IndexEntry->FileName.AllocatedSize);
    DPRINT1("\t\tDataSize: %I64u\n", IndexEntry->FileName.DataSize);
    DPRINT1("\t\tFileAttributes: %x\n", IndexEntry->FileName.FileAttributes);
    DPRINT1("\t\tNameLength: %u\n", IndexEntry->FileName.NameLength);
    DPRINT1("\t\tNameType: %x\n", IndexEntry->FileName.NameType);
    DPRINT1("\t\tName: %.*S\n", IndexEntry->FileName.NameLength, IndexEntry->FileName.Name);
}
#endif

NTSTATUS
BrowseSubNodeIndexEntries(PNTFS_VCB Vcb,
                          PFILE_RECORD_HEADER MftRecord,
                          ULONG IndexBlockSize,
                          PUNICODE_STRING FileName,
                          PNTFS_ATTR_CONTEXT IndexAllocationContext,
                          PRTL_BITMAP Bitmap,
                          ULONGLONG VCN,
                          PULONG StartEntry,
                          PULONG CurrentEntry,
                          BOOLEAN DirSearch,
                          BOOLEAN CaseSensitive,
                          ULONGLONG *OutMFTIndex)
{
    PINDEX_BUFFER IndexRecord;
    ULONGLONG Offset;
    ULONG BytesRead;
    PINDEX_ENTRY_ATTRIBUTE FirstEntry;
    PINDEX_ENTRY_ATTRIBUTE LastEntry;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;
    ULONG NodeNumber;
    NTSTATUS Status;

    DPRINT("BrowseSubNodeIndexEntries(%p, %p, %lu, %wZ, %p, %p, %I64d, %lu, %lu, %s, %s, %p)\n",
           Vcb,
           MftRecord,
           IndexBlockSize,
           FileName,
           IndexAllocationContext,
           Bitmap,
           VCN,
           *StartEntry,
           *CurrentEntry,
           "FALSE",
           DirSearch ? "TRUE" : "FALSE",
           CaseSensitive ? "TRUE" : "FALSE",
           OutMFTIndex);

    // Calculate node number as VCN / Clusters per index record
    NodeNumber = VCN / (Vcb->NtfsInfo.BytesPerIndexRecord / Vcb->NtfsInfo.BytesPerCluster);

    // Is the bit for this node clear in the bitmap?
    if (!RtlCheckBit(Bitmap, NodeNumber))
    {
        DPRINT1("File system corruption detected, node with VCN %I64u is marked as deleted.\n", VCN);
        return STATUS_DATA_ERROR;
    }

    // Allocate memory for the index record
    IndexRecord = ExAllocatePoolWithTag(NonPagedPool, IndexBlockSize, TAG_NTFS);
    if (!IndexRecord)
    {
        DPRINT1("Unable to allocate memory for index record!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Calculate offset of index record
    Offset = VCN * Vcb->NtfsInfo.BytesPerCluster;

    // Read the index record
    BytesRead = ReadAttribute(Vcb, IndexAllocationContext, Offset, (PCHAR)IndexRecord, IndexBlockSize);
    if (BytesRead != IndexBlockSize)
    {
        DPRINT1("Unable to read index record!\n");
        ExFreePoolWithTag(IndexRecord, TAG_NTFS);
        return STATUS_UNSUCCESSFUL;
    }

    // Assert that we're dealing with an index record here
    ASSERT(IndexRecord->Ntfs.Type == NRH_INDX_TYPE);

    // Apply the fixup array to the index record
    Status = FixupUpdateSequenceArray(Vcb, &((PFILE_RECORD_HEADER)IndexRecord)->Ntfs);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(IndexRecord, TAG_NTFS);
        DPRINT1("Failed to apply fixup array!\n");
        return Status;
    }

    ASSERT(IndexRecord->Header.AllocatedSize + FIELD_OFFSET(INDEX_BUFFER, Header) == IndexBlockSize);
    FirstEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)&IndexRecord->Header + IndexRecord->Header.FirstEntryOffset);
    LastEntry = (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)&IndexRecord->Header + IndexRecord->Header.TotalSizeOfEntries);
    ASSERT(LastEntry <= (PINDEX_ENTRY_ATTRIBUTE)((ULONG_PTR)IndexRecord + IndexBlockSize));

    // Loop through all Index Entries of index, starting with FirstEntry
    IndexEntry = FirstEntry;
    while (IndexEntry <= LastEntry)
    {
        // Does IndexEntry have a sub-node?
        if (IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
        {
            if (!(IndexRecord->Header.Flags & INDEX_NODE_LARGE) || !IndexAllocationContext)
            {
                DPRINT1("Filesystem corruption detected!\n");
            }
            else
            {
                Status = BrowseSubNodeIndexEntries(Vcb,
                                                   MftRecord,
                                                   IndexBlockSize,
                                                   FileName,
                                                   IndexAllocationContext,
                                                   Bitmap,
                                                   GetIndexEntryVCN(IndexEntry),
                                                   StartEntry,
                                                   CurrentEntry,
                                                   DirSearch,
                                                   CaseSensitive,
                                                   OutMFTIndex);
                if (NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(IndexRecord, TAG_NTFS);
                    return Status;
                }
            }
        }

        // Are we done?
        if (IndexEntry->Flags & NTFS_INDEX_ENTRY_END)
            break;

        // If we've found a file whose index is greater than or equal to StartEntry that matches the search criteria
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) >= NTFS_FILE_FIRST_USER_FILE &&
            *CurrentEntry >= *StartEntry &&
            IndexEntry->FileName.NameType != NTFS_FILE_NAME_DOS &&
            CompareFileName(FileName, IndexEntry, DirSearch, CaseSensitive))
        {
            *StartEntry = *CurrentEntry;
            *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
            ExFreePoolWithTag(IndexRecord, TAG_NTFS);
            return STATUS_SUCCESS;
        }

        // Advance to the next index entry
        (*CurrentEntry) += 1;
        ASSERT(IndexEntry->Length >= sizeof(INDEX_ENTRY_ATTRIBUTE));
        IndexEntry = (PINDEX_ENTRY_ATTRIBUTE)((PCHAR)IndexEntry + IndexEntry->Length);
    }

    ExFreePoolWithTag(IndexRecord, TAG_NTFS);

    return STATUS_OBJECT_PATH_NOT_FOUND;
}

NTSTATUS
BrowseIndexEntries(PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER MftRecord,
                   PINDEX_ROOT_ATTRIBUTE IndexRecord,
                   ULONG IndexBlockSize,
                   PINDEX_ENTRY_ATTRIBUTE FirstEntry,
                   PINDEX_ENTRY_ATTRIBUTE LastEntry,
                   PUNICODE_STRING FileName,
                   PULONG StartEntry,
                   PULONG CurrentEntry,
                   BOOLEAN DirSearch,
                   BOOLEAN CaseSensitive,
                   ULONGLONG *OutMFTIndex)
{
    NTSTATUS Status;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;
    PNTFS_ATTR_CONTEXT IndexAllocationContext;
    PNTFS_ATTR_CONTEXT BitmapContext;
    PCHAR *BitmapMem;
    ULONG *BitmapPtr;
    RTL_BITMAP  Bitmap;

    DPRINT("BrowseIndexEntries(%p, %p, %p, %lu, %p, %p, %wZ, %lu, %lu, %s, %s, %p)\n",
           Vcb,
           MftRecord,
           IndexRecord,
           IndexBlockSize,
           FirstEntry,
           LastEntry,
           FileName,
           *StartEntry,
           *CurrentEntry,
           DirSearch ? "TRUE" : "FALSE",
           CaseSensitive ? "TRUE" : "FALSE",
           OutMFTIndex);

    // Find the $I30 index allocation, if there is one
    Status = FindAttribute(Vcb, MftRecord, AttributeIndexAllocation, L"$I30", 4, &IndexAllocationContext, NULL);
    if (NT_SUCCESS(Status))
    {
        ULONGLONG BitmapLength;
        // Find the bitmap attribute for the index
        Status = FindAttribute(Vcb, MftRecord, AttributeBitmap, L"$I30", 4, &BitmapContext, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Potential file system corruption detected!\n");
            ReleaseAttributeContext(IndexAllocationContext);
            return Status;
        }

        // Get the length of the bitmap attribute
        BitmapLength = AttributeDataLength(BitmapContext->pRecord);

        // Allocate memory for the bitmap, including some padding; RtlInitializeBitmap() wants a pointer
        // that's ULONG-aligned, and it wants the size of the memory allocated for it to be a ULONG-multiple.
        BitmapMem = ExAllocatePoolWithTag(NonPagedPool, BitmapLength + sizeof(ULONG), TAG_NTFS);
        if (!BitmapMem)
        {
            DPRINT1("Error: failed to allocate bitmap!");
            ReleaseAttributeContext(BitmapContext);
            ReleaseAttributeContext(IndexAllocationContext);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(BitmapMem, BitmapLength + sizeof(ULONG));

        // RtlInitializeBitmap() wants a pointer that's ULONG-aligned.
        BitmapPtr = (PULONG)ALIGN_UP_BY((ULONG_PTR)BitmapMem, sizeof(ULONG));

        // Read the existing bitmap data
        Status = ReadAttribute(Vcb, BitmapContext, 0, (PCHAR)BitmapPtr, BitmapLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to read bitmap attribute!\n");
            ExFreePoolWithTag(BitmapMem, TAG_NTFS);
            ReleaseAttributeContext(BitmapContext);
            ReleaseAttributeContext(IndexAllocationContext);
            return Status;
        }

        // Initialize bitmap
        RtlInitializeBitMap(&Bitmap, BitmapPtr, BitmapLength * 8);
    }
    else
    {
        // Couldn't find an index allocation
        IndexAllocationContext = NULL;
    }


    // Loop through all Index Entries of index, starting with FirstEntry
    IndexEntry = FirstEntry;
    while (IndexEntry <= LastEntry)
    {
        // Does IndexEntry have a sub-node?
        if (IndexEntry->Flags & NTFS_INDEX_ENTRY_NODE)
        {
            if (!(IndexRecord->Header.Flags & INDEX_ROOT_LARGE) || !IndexAllocationContext)
            {
                DPRINT1("Filesystem corruption detected!\n");
            }
            else
            {
                Status = BrowseSubNodeIndexEntries(Vcb,
                                                   MftRecord,
                                                   IndexBlockSize,
                                                   FileName,
                                                   IndexAllocationContext,
                                                   &Bitmap,
                                                   GetIndexEntryVCN(IndexEntry),
                                                   StartEntry,
                                                   CurrentEntry,
                                                   DirSearch,
                                                   CaseSensitive,
                                                   OutMFTIndex);
                if (NT_SUCCESS(Status))
                {
                    ExFreePoolWithTag(BitmapMem, TAG_NTFS);
                    ReleaseAttributeContext(BitmapContext);
                    ReleaseAttributeContext(IndexAllocationContext);
                    return Status;
                }
            }
        }

        // Are we done?
        if (IndexEntry->Flags & NTFS_INDEX_ENTRY_END)
            break;

        // If we've found a file whose index is greater than or equal to StartEntry that matches the search criteria
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) >= NTFS_FILE_FIRST_USER_FILE &&
            *CurrentEntry >= *StartEntry &&
            IndexEntry->FileName.NameType != NTFS_FILE_NAME_DOS &&
            CompareFileName(FileName, IndexEntry, DirSearch, CaseSensitive))
        {
            *StartEntry = *CurrentEntry;
            *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
            if (IndexAllocationContext)
            {
                ExFreePoolWithTag(BitmapMem, TAG_NTFS);
                ReleaseAttributeContext(BitmapContext);
                ReleaseAttributeContext(IndexAllocationContext);
            }
            return STATUS_SUCCESS;
        }

        // Advance to the next index entry
        (*CurrentEntry) += 1;
        ASSERT(IndexEntry->Length >= sizeof(INDEX_ENTRY_ATTRIBUTE));
        IndexEntry = (PINDEX_ENTRY_ATTRIBUTE)((PCHAR)IndexEntry + IndexEntry->Length);
    }

    if (IndexAllocationContext)
    {
        ExFreePoolWithTag(BitmapMem, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        ReleaseAttributeContext(IndexAllocationContext);
    }

    return STATUS_OBJECT_PATH_NOT_FOUND;
}

NTSTATUS
NtfsFindMftRecord(PDEVICE_EXTENSION Vcb,
                  ULONGLONG MFTIndex,
                  PUNICODE_STRING FileName,
                  PULONG FirstEntry,
                  BOOLEAN DirSearch,
                  BOOLEAN CaseSensitive,
                  ULONGLONG *OutMFTIndex)
{
    PFILE_RECORD_HEADER MftRecord;
    PNTFS_ATTR_CONTEXT IndexRootCtx;
    PINDEX_ROOT_ATTRIBUTE IndexRoot;
    PCHAR IndexRecord;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry, IndexEntryEnd;
    NTSTATUS Status;
    ULONG CurrentEntry = 0;

    DPRINT("NtfsFindMftRecord(%p, %I64d, %wZ, %lu, %s, %s, %p)\n",
           Vcb,
           MFTIndex,
           FileName,
           *FirstEntry,
           DirSearch ? "TRUE" : "FALSE",
           CaseSensitive ? "TRUE" : "FALSE",
           OutMFTIndex);

    MftRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, MFTIndex, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return Status;
    }

    ASSERT(MftRecord->Ntfs.Type == NRH_FILE_TYPE);
    Status = FindAttribute(Vcb, MftRecord, AttributeIndexRoot, L"$I30", 4, &IndexRootCtx, NULL);
    if (!NT_SUCCESS(Status))
    {
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return Status;
    }

    IndexRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerIndexRecord, TAG_NTFS);
    if (IndexRecord == NULL)
    {
        ReleaseAttributeContext(IndexRootCtx);
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReadAttribute(Vcb, IndexRootCtx, 0, IndexRecord, Vcb->NtfsInfo.BytesPerIndexRecord);
    IndexRoot = (PINDEX_ROOT_ATTRIBUTE)IndexRecord;
    IndexEntry = (PINDEX_ENTRY_ATTRIBUTE)((PCHAR)&IndexRoot->Header + IndexRoot->Header.FirstEntryOffset);
    /* Index root is always resident. */
    IndexEntryEnd = (PINDEX_ENTRY_ATTRIBUTE)(IndexRecord + IndexRoot->Header.TotalSizeOfEntries);
    ReleaseAttributeContext(IndexRootCtx);

    DPRINT("IndexRecordSize: %x IndexBlockSize: %x\n", Vcb->NtfsInfo.BytesPerIndexRecord, IndexRoot->SizeOfEntry);

    Status = BrowseIndexEntries(Vcb,
                                MftRecord,
                                (PINDEX_ROOT_ATTRIBUTE)IndexRecord,
                                IndexRoot->SizeOfEntry,
                                IndexEntry,
                                IndexEntryEnd,
                                FileName,
                                FirstEntry,
                                &CurrentEntry,
                                DirSearch,
                                CaseSensitive,
                                OutMFTIndex);

    ExFreePoolWithTag(IndexRecord, TAG_NTFS);
    ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, MftRecord);

    return Status;
}

NTSTATUS
NtfsLookupFileAt(PDEVICE_EXTENSION Vcb,
                 PUNICODE_STRING PathName,
                 BOOLEAN CaseSensitive,
                 PFILE_RECORD_HEADER *FileRecord,
                 PULONGLONG MFTIndex,
                 ULONGLONG CurrentMFTIndex)
{
    UNICODE_STRING Current, Remaining;
    NTSTATUS Status;
    ULONG FirstEntry = 0;

    DPRINT("NtfsLookupFileAt(%p, %wZ, %s, %p, %p, %I64x)\n",
           Vcb,
           PathName,
           CaseSensitive ? "TRUE" : "FALSE",
           FileRecord,
           MFTIndex,
           CurrentMFTIndex);

    FsRtlDissectName(*PathName, &Current, &Remaining);

    while (Current.Length != 0)
    {
        DPRINT("Current: %wZ\n", &Current);

        Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, &Current, &FirstEntry, FALSE, CaseSensitive, &CurrentMFTIndex);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if (Remaining.Length == 0)
            break;

        FsRtlDissectName(Current, &Current, &Remaining);
    }

    *FileRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (*FileRecord == NULL)
    {
        DPRINT("NtfsLookupFileAt: Can't allocate MFT record\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, CurrentMFTIndex, *FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsLookupFileAt: Can't read MFT record\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, *FileRecord);
        return Status;
    }

    *MFTIndex = CurrentMFTIndex;

    return STATUS_SUCCESS;
}

NTSTATUS
NtfsLookupFile(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING PathName,
               BOOLEAN CaseSensitive,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex)
{
    return NtfsLookupFileAt(Vcb, PathName, CaseSensitive, FileRecord, MFTIndex, NTFS_FILE_ROOT);
}

void
NtfsDumpData(ULONG_PTR Buffer, ULONG Length)
{
    ULONG i, j;

    // dump binary data, 8 bytes at a time
    for (i = 0; i < Length; i += 8)
    {
        // display current offset, in hex
        DbgPrint("\t%03x\t", i);

        // display hex value of each of the next 8 bytes
        for (j = 0; j < 8; j++)
            DbgPrint("%02x ", *(PUCHAR)(Buffer + i + j));
        DbgPrint("\n");
    }
}

/**
* @name NtfsDumpFileRecord
* @implemented
*
* Provides diagnostic information about a file record. Prints a hex dump
* of the entire record (based on the size reported by FileRecord->ByesInUse),
* then prints a dump of each attribute.
*
* @param Vcb
* Pointer to a DEVICE_EXTENSION describing the volume.
*
* @param FileRecord
* Pointer to the file record to be analyzed.
*
* @remarks
* FileRecord must be a complete file record at least FileRecord->BytesAllocated
* in size, and not just the header.
*
*/
VOID
NtfsDumpFileRecord(PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER FileRecord)
{
    ULONG i, j;

    // dump binary data, 8 bytes at a time
    for (i = 0; i < FileRecord->BytesInUse; i += 8)
    {
        // display current offset, in hex
        DbgPrint("\t%03x\t", i);

        // display hex value of each of the next 8 bytes
        for (j = 0; j < 8; j++)
            DbgPrint("%02x ", *(PUCHAR)((ULONG_PTR)FileRecord + i + j));
        DbgPrint("\n");
    }

    NtfsDumpFileAttributes(Vcb, FileRecord);
}

NTSTATUS
NtfsFindFileAt(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING SearchPattern,
               PULONG FirstEntry,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex,
               ULONGLONG CurrentMFTIndex,
               BOOLEAN CaseSensitive)
{
    NTSTATUS Status;

    DPRINT("NtfsFindFileAt(%p, %wZ, %lu, %p, %p, %I64x, %s)\n",
           Vcb,
           SearchPattern,
           *FirstEntry,
           FileRecord,
           MFTIndex,
           CurrentMFTIndex,
           (CaseSensitive ? "TRUE" : "FALSE"));

    Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, SearchPattern, FirstEntry, TRUE, CaseSensitive, &CurrentMFTIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsFindFileAt: NtfsFindMftRecord() failed with status 0x%08lx\n", Status);
        return Status;
    }

    *FileRecord = ExAllocateFromNPagedLookasideList(&Vcb->FileRecLookasideList);
    if (*FileRecord == NULL)
    {
        DPRINT("NtfsFindFileAt: Can't allocate MFT record\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, CurrentMFTIndex, *FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsFindFileAt: Can't read MFT record\n");
        ExFreeToNPagedLookasideList(&Vcb->FileRecLookasideList, *FileRecord);
        return Status;
    }

    *MFTIndex = CurrentMFTIndex;

    return STATUS_SUCCESS;
}

/* EOF */
