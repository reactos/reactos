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

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PNTFS_ATTR_CONTEXT
PrepareAttributeContext(PNTFS_ATTR_RECORD AttrRecord)
{
    PNTFS_ATTR_CONTEXT Context;

    Context = ExAllocatePoolWithTag(NonPagedPool,
                                    FIELD_OFFSET(NTFS_ATTR_CONTEXT, Record) + AttrRecord->Length,
                                    TAG_NTFS);
    RtlCopyMemory(&Context->Record, AttrRecord, AttrRecord->Length);
    if (AttrRecord->IsNonResident)
    {
        LONGLONG DataRunOffset;
        ULONGLONG DataRunLength;
        ULONGLONG NextVBN = 0;
        PUCHAR DataRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;

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
            ExFreePoolWithTag(Context, TAG_NTFS);
            return NULL;
        }
    }

    return Context;
}


VOID
ReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context)
{
    if (Context->Record.IsNonResident)
    {
        FsRtlUninitializeLargeMcb(&Context->DataRunsMCB);
    }

    ExFreePoolWithTag(Context, TAG_NTFS);
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

    DPRINT("FindAttribute(%p, %p, 0x%x, %S, %u, %p)\n", Vcb, MftRecord, Type, Name, NameLength, AttrCtx);

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
                if (RtlCompareMemory(AttrName, Name, NameLength << 1) == (NameLength << 1))
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

    FindCloseAttribute(&Context);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


ULONGLONG
AttributeAllocatedLength(PNTFS_ATTR_RECORD AttrRecord)
{
    if (AttrRecord->IsNonResident)
        return AttrRecord->NonResident.AllocatedSize;
    else
        return AttrRecord->Resident.ValueLength;
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
* Increases the size of the Master File Table by 8 records. Bitmap entries for the new records are cleared,
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
    ULONG DataSizeDifference = Vcb->NtfsInfo.BytesPerFileRecord * 8;
    ULONG BitmapOffset;
    PUCHAR BitmapBuffer;
    ULONGLONG BitmapBytes;
    ULONGLONG NewBitmapSize;
    ULONG BytesRead;
    ULONG LengthWritten;
    NTSTATUS Status;

    DPRINT1("IncreaseMftSize(%p, %s)\n", Vcb, CanWait ? "TRUE" : "FALSE");

    // We need exclusive access to the mft while we change its size
    if (!ExAcquireResourceExclusiveLite(&(Vcb->DirResource), CanWait))
    {
        return STATUS_CANT_WAIT;
    }

    // Find the bitmap attribute of master file table
    Status = FindAttribute(Vcb, Vcb->MasterFileTable, AttributeBitmap, L"", 0, &BitmapContext, &BitmapOffset);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $BITMAP attribute of Mft!\n");
        ExReleaseResourceLite(&(Vcb->DirResource));
        return Status;
    }

    // Get size of Bitmap Attribute
    BitmapSize.QuadPart = AttributeDataLength(&BitmapContext->Record);

    // Calculate the new mft size
    DataSize.QuadPart = AttributeDataLength(&(Vcb->MFTContext->Record)) + DataSizeDifference;

    // Determine how many bytes will make up the bitmap
    BitmapBytes = DataSize.QuadPart / Vcb->NtfsInfo.BytesPerFileRecord / 8;
    
    // Determine how much we need to adjust the bitmap size (it's possible we don't)
    BitmapSizeDifference = BitmapBytes - BitmapSize.QuadPart;
    NewBitmapSize = max(BitmapSize.QuadPart + BitmapSizeDifference, BitmapSize.QuadPart);

    // Allocate memory for the bitmap
    BitmapBuffer = ExAllocatePoolWithTag(NonPagedPool, NewBitmapSize, TAG_NTFS);
    if (!BitmapBuffer)
    {
        DPRINT1("ERROR: Unable to allocate memory for bitmap attribute!\n");
        ExReleaseResourceLite(&(Vcb->DirResource));
        ReleaseAttributeContext(BitmapContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Zero the bytes we'll be adding
    RtlZeroMemory((PUCHAR)((ULONG_PTR)BitmapBuffer), NewBitmapSize);

    // Read the bitmap attribute
    BytesRead = ReadAttribute(Vcb,
                              BitmapContext,
                              0,
                              (PCHAR)BitmapBuffer,
                              BitmapSize.LowPart);
    if (BytesRead != BitmapSize.LowPart)
    {
        DPRINT1("ERROR: Bytes read != Bitmap size!\n");
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
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // If the bitmap grew
    if (BitmapSizeDifference > 0)
    {
        // Set the new bitmap size
        BitmapSize.QuadPart += BitmapSizeDifference;
        if (BitmapContext->Record.IsNonResident)
            Status = SetNonResidentAttributeDataLength(Vcb, BitmapContext, BitmapOffset, Vcb->MasterFileTable, &BitmapSize);
        else
            Status = SetResidentAttributeDataLength(Vcb, BitmapContext, BitmapOffset, Vcb->MasterFileTable, &BitmapSize);
    
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Failed to set size of bitmap attribute!\n");
            ExReleaseResourceLite(&(Vcb->DirResource));
            ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
            ReleaseAttributeContext(BitmapContext);
            return Status;
        }
    }

    //NtfsDumpFileAttributes(Vcb, FileRecord);

    // Update the file record with the new attribute sizes
    Status = UpdateFileRecord(Vcb, Vcb->VolumeFcb->MFTIndex, Vcb->MasterFileTable);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Failed to update $MFT file record!\n");
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // Write out the new bitmap
    Status = WriteAttribute(Vcb, BitmapContext, BitmapOffset, BitmapBuffer, BitmapSize.LowPart, &LengthWritten);
    if (!NT_SUCCESS(Status))
    {
        ExReleaseResourceLite(&(Vcb->DirResource));
        ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        DPRINT1("ERROR: Couldn't write to bitmap attribute of $MFT!\n");
    }

    // Cleanup
    ExReleaseResourceLite(&(Vcb->DirResource));
    ExFreePoolWithTag(BitmapBuffer, TAG_NTFS);
    ReleaseAttributeContext(BitmapContext);

    return STATUS_SUCCESS;
}

VOID
InternalSetResidentAttributeLength(PNTFS_ATTR_CONTEXT AttrContext,
                                   PFILE_RECORD_HEADER FileRecord,
                                   ULONG AttrOffset,
                                   ULONG DataSize)
{
    PNTFS_ATTR_RECORD Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
    ULONG NextAttributeOffset;

    DPRINT("InternalSetResidentAttributeLength( %p, %p, %lu, %lu )\n", AttrContext, FileRecord, AttrOffset, DataSize);

    // update ValueLength Field
    AttrContext->Record.Resident.ValueLength =
    Destination->Resident.ValueLength = DataSize;

    // calculate the record length and end marker offset
    AttrContext->Record.Length =
    Destination->Length = DataSize + AttrContext->Record.Resident.ValueOffset;
    NextAttributeOffset = AttrOffset + AttrContext->Record.Length;

    // Ensure NextAttributeOffset is aligned to an 8-byte boundary
    if (NextAttributeOffset % 8 != 0)
    {
        USHORT Padding = 8 - (NextAttributeOffset % 8);
        NextAttributeOffset += Padding;
        AttrContext->Record.Length += Padding;
        Destination->Length += Padding;
    }
    
    // advance Destination to the final "attribute" and set the file record end
    Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)Destination + Destination->Length);
    SetFileRecordEnd(FileRecord, Destination, FILE_RECORD_END);
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

    // are we truncating the file?
    if (DataSize->QuadPart < AttributeDataLength(&AttrContext->Record))
    {
        if (!MmCanFileBeTruncated(FileObject->SectionObjectPointer, DataSize))
        {
            DPRINT1("Can't truncate a memory-mapped file!\n");
            return STATUS_USER_MAPPED_FILE;
        }
    }

    if (AttrContext->Record.IsNonResident)
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
        if(AttrContext->Record.IsNonResident)
            Fcb->RFCB.AllocationSize.QuadPart = AttrContext->Record.NonResident.AllocatedSize;
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
    ULONG ExistingClusters = AttrContext->Record.NonResident.AllocatedSize / BytesPerCluster;

    if (!AttrContext->Record.IsNonResident)
    {
        DPRINT1("ERROR: SetNonResidentAttributeDataLength() called for resident attribute!\n");
        return STATUS_INVALID_PARAMETER;
    }

    // do we need to increase the allocation size?
    if (AttrContext->Record.NonResident.AllocatedSize < AllocationSize)
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
                                          (LONGLONG)AttrContext->Record.NonResident.HighestVCN,
                                          (PLONGLONG)&LastClusterInDataRun.QuadPart,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL))
            {
                DPRINT1("Error looking up final large MCB entry!\n");

                // Most likely, HighestVCN went above the largest mapping
                DPRINT1("Highest VCN of record: %I64u\n", AttrContext->Record.NonResident.HighestVCN);
                return STATUS_INVALID_PARAMETER;
            }
        }

        DPRINT("LastClusterInDataRun: %I64u\n", LastClusterInDataRun.QuadPart);
        DPRINT("Highest VCN of record: %I64u\n", AttrContext->Record.NonResident.HighestVCN);

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
    else if (AttrContext->Record.NonResident.AllocatedSize > AllocationSize)
    {
        // shrink allocation size
        ULONG ClustersToFree = ExistingClusters - (AllocationSize / BytesPerCluster);
        Status = FreeClusters(Vcb, AttrContext, AttrOffset, FileRecord, ClustersToFree);
    }

    // TODO: is the file compressed, encrypted, or sparse?

    AttrContext->Record.NonResident.AllocatedSize = AllocationSize;
    AttrContext->Record.NonResident.DataSize = DataSize->QuadPart;
    AttrContext->Record.NonResident.InitializedSize = DataSize->QuadPart;

    DestinationAttribute->NonResident.AllocatedSize = AllocationSize;
    DestinationAttribute->NonResident.DataSize = DataSize->QuadPart;
    DestinationAttribute->NonResident.InitializedSize = DataSize->QuadPart;

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
    ULONG NextAttributeOffset = AttrOffset + AttrContext->Record.Length;
    PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((PCHAR)FileRecord + NextAttributeOffset);

    if (AttrContext->Record.IsNonResident)
    {
        DPRINT1("ERROR: SetResidentAttributeDataLength() called for non-resident attribute!\n");
        return STATUS_INVALID_PARAMETER;
    }

    //NtfsDumpFileAttributes(Vcb, FileRecord);

    // Do we need to increase the data length?
    if (DataSize->QuadPart > AttrContext->Record.Resident.ValueLength)
    {
        // There's usually padding at the end of a record. Do we need to extend past it?
        ULONG MaxValueLength = AttrContext->Record.Length - AttrContext->Record.Resident.ValueOffset;
        if (MaxValueLength < DataSize->LowPart)
        {
            // If this is the last attribute, we could move the end marker to the very end of the file record
            MaxValueLength += Vcb->NtfsInfo.BytesPerFileRecord - NextAttributeOffset - (sizeof(ULONG) * 2);

            if (MaxValueLength < DataSize->LowPart || NextAttribute->Type != AttributeEnd)
            {
                // convert attribute to non-resident
                PNTFS_ATTR_RECORD Destination = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + AttrOffset);
                LARGE_INTEGER AttribDataSize;
                PVOID AttribData;
                ULONG EndAttributeOffset;
                ULONG LengthWritten;

                DPRINT1("Converting attribute to non-resident.\n");

                AttribDataSize.QuadPart = AttrContext->Record.Resident.ValueLength;

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

                // Zero out the NonResident structure
                RtlZeroMemory(&AttrContext->Record.NonResident.LowestVCN,
                              FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.CompressedSize) - FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.LowestVCN));
                RtlZeroMemory(&Destination->NonResident.LowestVCN,
                              FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.CompressedSize) - FIELD_OFFSET(NTFS_ATTR_RECORD, NonResident.LowestVCN));

                // update the mapping pairs offset, which will be 0x40 + length in bytes of the name
                AttrContext->Record.NonResident.MappingPairsOffset = Destination->NonResident.MappingPairsOffset = 0x40 + (Destination->NameLength * 2);

                // mark the attribute as non-resident
                AttrContext->Record.IsNonResident = Destination->IsNonResident = 1;

                // update the end of the file record
                // calculate position of end markers (1 byte for empty data run)
                EndAttributeOffset = AttrOffset + AttrContext->Record.NonResident.MappingPairsOffset + 1;
                EndAttributeOffset = ALIGN_UP_BY(EndAttributeOffset, 8);

                // Update the length
                Destination->Length = EndAttributeOffset - AttrOffset;
                AttrContext->Record.Length = Destination->Length;

                // Update the file record end
                SetFileRecordEnd(FileRecord,
                                 (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + EndAttributeOffset),
                                 FILE_RECORD_END);

                // update file record on disk
                Status = UpdateFileRecord(Vcb, AttrContext->FileMFTIndex, FileRecord);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("ERROR: Couldn't update file record to continue migration!\n");
                    if (AttribDataSize.QuadPart > 0)
                        ExFreePoolWithTag(AttribData, TAG_NTFS);
                    return Status;
                }

                // Initialize the MCB, potentially catch an exception
                _SEH2_TRY{
                    FsRtlInitializeLargeMcb(&AttrContext->DataRunsMCB, NonPagedPool);
                } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                } _SEH2_END;

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
                    Status = WriteAttribute(Vcb, AttrContext, 0, AttribData, AttribDataSize.QuadPart, &LengthWritten);
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
    else if (DataSize->LowPart < AttrContext->Record.Resident.ValueLength)
    {
        // we need to decrease the length
        if (NextAttribute->Type != AttributeEnd)
        {
            DPRINT1("FIXME: Don't know how to decrease length of resident attribute unless it's the final attribute!\n");
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    // set the new length of the resident attribute (if we didn't migrate it)
    if (!AttrContext->Record.IsNonResident)
        InternalSetResidentAttributeLength(AttrContext, FileRecord, AttrOffset, DataSize->LowPart);

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
        //TEMPTEMP
        ULONG UsedBufferSize;
        TempBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);

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
    if (Context->Record.IsNonResident)
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
               PULONG RealLengthWritten)
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
    
    //TEMPTEMP
    PUCHAR TempBuffer;
        

    DPRINT("WriteAttribute(%p, %p, %I64u, %p, %lu, %p)\n", Vcb, Context, Offset, Buffer, Length, RealLengthWritten);

    *RealLengthWritten = 0;

    // is this a resident attribute?
    if (!Context->Record.IsNonResident)
    {
        ULONG AttributeOffset;
        PNTFS_ATTR_CONTEXT FoundContext;
        PFILE_RECORD_HEADER FileRecord;

        if (Offset + Length > Context->Record.Resident.ValueLength)
        {
            DPRINT1("DRIVER ERROR: Attribute is too small!\n");
            return STATUS_INVALID_PARAMETER;
        }

        FileRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);

        if (!FileRecord)
        {
            DPRINT1("Error: Couldn't allocate file record!\n");
            return STATUS_NO_MEMORY;
        }

        // read the file record
        ReadFileRecord(Vcb, Context->FileMFTIndex, FileRecord);

        // find where to write the attribute data to
        Status = FindAttribute(Vcb, FileRecord,
                               Context->Record.Type,
                               (PCWSTR)((PCHAR)&Context->Record + Context->Record.NameOffset),
                               Context->Record.NameLength,
                               &FoundContext,
                               &AttributeOffset);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ERROR: Couldn't find matching attribute!\n");
            ExFreePoolWithTag(FileRecord, TAG_NTFS);
            return Status;
        }

        DPRINT("Offset: %I64u, AttributeOffset: %u, ValueOffset: %u\n", Offset, AttributeOffset, Context->Record.Resident.ValueLength);
        Offset += AttributeOffset + Context->Record.Resident.ValueOffset;
        
        if (Offset + Length > Vcb->NtfsInfo.BytesPerFileRecord)
        {
            DPRINT1("DRIVER ERROR: Data being written extends past end of file record!\n");
            ReleaseAttributeContext(FoundContext);
            ExFreePoolWithTag(FileRecord, TAG_NTFS);
            return STATUS_INVALID_PARAMETER;
        }

        // copy the data being written into the file record
        RtlCopyMemory((PCHAR)FileRecord + Offset, Buffer, Length);

        Status = UpdateFileRecord(Vcb, Context->FileMFTIndex, FileRecord);

        ReleaseAttributeContext(FoundContext);
        ExFreePoolWithTag(FileRecord, TAG_NTFS);

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
                return STATUS_NOT_IMPLEMENTED;
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
                return STATUS_END_OF_FILE;
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

        return Status;
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
            return STATUS_NOT_IMPLEMENTED;
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
                return STATUS_END_OF_FILE;
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

    // TEMPTEMP
    if(Context->Record.IsNonResident)
        ExFreePoolWithTag(TempBuffer, TAG_NTFS);

    Context->CacheRun = DataRun;
    Context->CacheRunOffset = Offset + *RealLengthWritten;
    Context->CacheRunStartLCN = DataRunStartLCN;
    Context->CacheRunLength = DataRunLength;
    Context->CacheRunLastLCN = LastLCN;
    Context->CacheRunCurrentOffset = CurrentOffset;

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
        DPRINT1("ReadFileRecord failed: %I64u read, %u expected\n", BytesRead, Vcb->NtfsInfo.BytesPerFileRecord);
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

    DPRINT("UpdateFileNameRecord(%p, %I64d, %wZ, %u, %I64u, %I64u, %s)\n",
           Vcb,
           ParentMFTIndex,
           FileName,
           DirSearch,
           NewDataSize,
           NewAllocationSize,
           CaseSensitive ? "TRUE" : "FALSE");

    MftRecord = ExAllocatePoolWithTag(NonPagedPool,
                                      Vcb->NtfsInfo.BytesPerFileRecord,
                                      TAG_NTFS);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, ParentMFTIndex, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return Status;
    }

    ASSERT(MftRecord->Ntfs.Type == NRH_FILE_TYPE);
    Status = FindAttribute(Vcb, MftRecord, AttributeIndexRoot, L"$I30", 4, &IndexRootCtx, NULL);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return Status;
    }

    IndexRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerIndexRecord, TAG_NTFS);
    if (IndexRecord == NULL)
    {
        ReleaseAttributeContext(IndexRootCtx);
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReadAttribute(Vcb, IndexRootCtx, 0, IndexRecord, Vcb->NtfsInfo.BytesPerIndexRecord);
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

    ReleaseAttributeContext(IndexRootCtx);
    ExFreePoolWithTag(IndexRecord, TAG_NTFS);
    ExFreePoolWithTag(MftRecord, TAG_NTFS);

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

    DPRINT("UpdateIndexEntrySize(%p, %p, %p, %u, %p, %p, %wZ, %u, %u, %u, %I64u, %I64u)\n", Vcb, MftRecord, IndexRecord, IndexBlockSize, FirstEntry, LastEntry, FileName, *StartEntry, *CurrentEntry, DirSearch, NewDataSize, NewAllocatedSize);

    // find the index entry responsible for the file we're trying to update
    IndexEntry = FirstEntry;
    while (IndexEntry < LastEntry &&
           !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
    {
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) > 0x10 &&
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

    IndexAllocationSize = AttributeDataLength(&IndexAllocationCtx->Record);
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

            Status = WriteAttribute(Vcb, IndexAllocationCtx, RecordOffset, (const PUCHAR)IndexRecord, IndexBlockSize, &Written);
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
    Status = WriteAttribute(Vcb, Vcb->MFTContext, MftIndex * Vcb->NtfsInfo.BytesPerFileRecord, (const PUCHAR)FileRecord, Vcb->NtfsInfo.BytesPerFileRecord, &BytesWritten);

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
    PVOID BitmapData;
    ULONG LengthWritten;
    PNTFS_ATTR_CONTEXT BitmapContext;
    LARGE_INTEGER BitmapBits;
    UCHAR SystemReservedBits;

    DPRINT1("AddNewMftEntry(%p, %p, %p, %s)\n", FileRecord, DeviceExt, DestinationIndex, CanWait ? "TRUE" : "FALSE");

    // First, we have to read the mft's $Bitmap attribute
    Status = FindAttribute(DeviceExt, DeviceExt->MasterFileTable, AttributeBitmap, L"", 0, &BitmapContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Couldn't find $Bitmap attribute of master file table!\n");
        return Status;
    }

    // allocate a buffer for the $Bitmap attribute
    BitmapDataSize = AttributeDataLength(&BitmapContext->Record);
    BitmapData = ExAllocatePoolWithTag(NonPagedPool, BitmapDataSize, TAG_NTFS);
    if (!BitmapData)
    {
        ReleaseAttributeContext(BitmapContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // read $Bitmap attribute
    AttrBytesRead = ReadAttribute(DeviceExt, BitmapContext, 0, BitmapData, BitmapDataSize);

    if (AttrBytesRead == 0)
    {
        DPRINT1("ERROR: Unable to read $Bitmap attribute of master file table!\n");
        ExFreePoolWithTag(BitmapData, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    // we need to backup the bits for records 0x10 - 0x17 and leave them unassigned if they aren't assigned
    RtlCopyMemory(&SystemReservedBits, (PVOID)((ULONG_PTR)BitmapData + 2), 1);
    RtlFillMemory((PVOID)((ULONG_PTR)BitmapData + 2), 1, (UCHAR)0xFF);

    // Calculate bit count
    BitmapBits.QuadPart = AttributeDataLength(&(DeviceExt->MFTContext->Record)) /
                          DeviceExt->NtfsInfo.BytesPerFileRecord;
    if (BitmapBits.HighPart != 0)
    {
        DPRINT1("\tFIXME: bitmap sizes beyond 32bits are not yet supported!\n");
        BitmapBits.LowPart = 0xFFFFFFFF;
    }

    // convert buffer into bitmap
    RtlInitializeBitMap(&Bitmap, (PULONG)BitmapData, BitmapBits.LowPart);

    // set next available bit, preferrably after 23rd bit
    MftIndex = RtlFindClearBitsAndSet(&Bitmap, 1, 24);
    if ((LONG)MftIndex == -1)
    {
        DPRINT1("Couldn't find free space in MFT for file record, increasing MFT size.\n");

        ExFreePoolWithTag(BitmapData, TAG_NTFS);
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
    RtlCopyMemory((PVOID)((ULONG_PTR)BitmapData + 2), &SystemReservedBits, 1);

    // write the bitmap back to the MFT's $Bitmap attribute
    Status = WriteAttribute(DeviceExt, BitmapContext, 0, BitmapData, BitmapDataSize, &LengthWritten);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR encountered when writing $Bitmap attribute!\n");
        ExFreePoolWithTag(BitmapData, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    // update the file record (write it to disk)
    Status = UpdateFileRecord(DeviceExt, MftIndex, FileRecord);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Unable to write file record!\n");
        ExFreePoolWithTag(BitmapData, TAG_NTFS);
        ReleaseAttributeContext(BitmapContext);
        return Status;
    }

    *DestinationIndex = MftIndex;

    ExFreePoolWithTag(BitmapData, TAG_NTFS);
    ReleaseAttributeContext(BitmapContext);

    return Status;
}

NTSTATUS
AddFixupArray(PDEVICE_EXTENSION Vcb,
              PNTFS_RECORD_HEADER Record)
{
    USHORT *pShortToFixUp;
    unsigned int ArrayEntryCount = Record->UsaCount - 1;
    unsigned int Offset = Vcb->NtfsInfo.BytesPerSector - 2;
    int i;

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
BrowseIndexEntries(PDEVICE_EXTENSION Vcb,
                   PFILE_RECORD_HEADER MftRecord,
                   PCHAR IndexRecord,
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
    ULONG RecordOffset;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;
    PNTFS_ATTR_CONTEXT IndexAllocationCtx;
    ULONGLONG IndexAllocationSize;
    PINDEX_BUFFER IndexBuffer;

    DPRINT("BrowseIndexEntries(%p, %p, %p, %u, %p, %p, %wZ, %u, %u, %s, %s, %p)\n",
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

    IndexEntry = FirstEntry;
    while (IndexEntry < LastEntry &&
           !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
    {
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) >= 0x10 &&
            *CurrentEntry >= *StartEntry &&
            IndexEntry->FileName.NameType != NTFS_FILE_NAME_DOS &&
            CompareFileName(FileName, IndexEntry, DirSearch, CaseSensitive))
        {
            *StartEntry = *CurrentEntry;
            *OutMFTIndex = (IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK);
            return STATUS_SUCCESS;
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

    IndexAllocationSize = AttributeDataLength(&IndexAllocationCtx->Record);
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

        Status = BrowseIndexEntries(NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    FirstEntry,
                                    LastEntry,
                                    FileName,
                                    StartEntry,
                                    CurrentEntry,
                                    DirSearch,
                                    CaseSensitive,
                                    OutMFTIndex);
        if (NT_SUCCESS(Status))
        {
            break;
        }
    }

    ReleaseAttributeContext(IndexAllocationCtx);
    return Status;    
}

NTSTATUS
NtfsFindMftRecord(PDEVICE_EXTENSION Vcb,
                  ULONGLONG MFTIndex,
                  PUNICODE_STRING FileName,
                  PULONG FirstEntry,
                  BOOLEAN DirSearch,
                  ULONGLONG *OutMFTIndex,
                  BOOLEAN CaseSensitive)
{
    PFILE_RECORD_HEADER MftRecord;
    PNTFS_ATTR_CONTEXT IndexRootCtx;
    PINDEX_ROOT_ATTRIBUTE IndexRoot;
    PCHAR IndexRecord;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry, IndexEntryEnd;
    NTSTATUS Status;
    ULONG CurrentEntry = 0;

    DPRINT("NtfsFindMftRecord(%p, %I64d, %wZ, %u, %u, %p)\n", Vcb, MFTIndex, FileName, *FirstEntry, DirSearch, OutMFTIndex);

    MftRecord = ExAllocatePoolWithTag(NonPagedPool,
                                      Vcb->NtfsInfo.BytesPerFileRecord,
                                      TAG_NTFS);
    if (MftRecord == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, MFTIndex, MftRecord);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return Status;
    }

    ASSERT(MftRecord->Ntfs.Type == NRH_FILE_TYPE);
    Status = FindAttribute(Vcb, MftRecord, AttributeIndexRoot, L"$I30", 4, &IndexRootCtx, NULL);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
        return Status;
    }

    IndexRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerIndexRecord, TAG_NTFS);
    if (IndexRecord == NULL)
    {
        ReleaseAttributeContext(IndexRootCtx);
        ExFreePoolWithTag(MftRecord, TAG_NTFS);
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
                                IndexRecord,
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
    ExFreePoolWithTag(MftRecord, TAG_NTFS);

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

        Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, &Current, &FirstEntry, FALSE, &CurrentMFTIndex, CaseSensitive);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if (Remaining.Length == 0)
            break;

        FsRtlDissectName(Current, &Current, &Remaining);
    }

    *FileRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (*FileRecord == NULL)
    {
        DPRINT("NtfsLookupFileAt: Can't allocate MFT record\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, CurrentMFTIndex, *FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsLookupFileAt: Can't read MFT record\n");
        ExFreePoolWithTag(*FileRecord, TAG_NTFS);
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

    DPRINT("NtfsFindFileAt(%p, %wZ, %u, %p, %p, %I64x, %s)\n",
           Vcb,
           SearchPattern,
           *FirstEntry,
           FileRecord,
           MFTIndex,
           CurrentMFTIndex,
           (CaseSensitive ? "TRUE" : "FALSE"));

    Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, SearchPattern, FirstEntry, TRUE, &CurrentMFTIndex, CaseSensitive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsFindFileAt: NtfsFindMftRecord() failed with status 0x%08lx\n", Status);
        return Status;
    }

    *FileRecord = ExAllocatePoolWithTag(NonPagedPool, Vcb->NtfsInfo.BytesPerFileRecord, TAG_NTFS);
    if (*FileRecord == NULL)
    {
        DPRINT("NtfsFindFileAt: Can't allocate MFT record\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ReadFileRecord(Vcb, CurrentMFTIndex, *FileRecord);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtfsFindFileAt: Can't read MFT record\n");
        ExFreePoolWithTag(*FileRecord, TAG_NTFS);
        return Status;
    }

    *MFTIndex = CurrentMFTIndex;

    return STATUS_SUCCESS;
}

/* EOF */
