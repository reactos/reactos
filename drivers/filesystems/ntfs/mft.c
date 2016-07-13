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
#undef NDEBUG
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

        Context->CacheRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
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
    }

    return Context;
}


VOID
ReleaseAttributeContext(PNTFS_ATTR_CONTEXT Context)
{
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

void
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
    
    // advance Destination to the final "attribute" and write the end type
    (ULONG_PTR)Destination += Destination->Length;
    Destination->Type = AttributeEnd;

    // write the final marker (which shares the same offset and type as the Length field)
    Destination->Length = FILE_RECORD_END;

    FileRecord->BytesInUse = NextAttributeOffset + (sizeof(ULONG) * 2);
}

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
        ULONG BytesPerCluster = Fcb->Vcb->NtfsInfo.BytesPerCluster;
        ULONGLONG AllocationSize = ROUND_UP(DataSize->QuadPart, BytesPerCluster);

        // do we need to increase the allocation size?
        if (AttrContext->Record.NonResident.AllocatedSize < AllocationSize)
        {              
            ULONG ExistingClusters = AttrContext->Record.NonResident.AllocatedSize / BytesPerCluster;
            ULONG ClustersNeeded = (AllocationSize / BytesPerCluster) - ExistingClusters;
            LARGE_INTEGER LastClusterInDataRun;
            ULONG NextAssignedCluster;
            ULONG AssignedClusters;

            NTSTATUS Status = GetLastClusterInDataRun(Fcb->Vcb, &AttrContext->Record, &LastClusterInDataRun.QuadPart);

            DPRINT1("GetLastClusterInDataRun returned: %I64u\n", LastClusterInDataRun.QuadPart);
            DPRINT1("Highest VCN of record: %I64u\n", AttrContext->Record.NonResident.HighestVCN);

            while (ClustersNeeded > 0)
            {
                Status = NtfsAllocateClusters(Fcb->Vcb,
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
                Status = AddRun(AttrContext, NextAssignedCluster, AssignedClusters);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Error: Unable to add data run!\n");
                    return Status;
                }

                ClustersNeeded -= AssignedClusters;
                LastClusterInDataRun.LowPart = NextAssignedCluster + AssignedClusters - 1;
            }

            DPRINT1("FixMe: Increasing allocation size is unimplemented!\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        // TODO: is the file compressed, encrypted, or sparse?

        // NOTE: we need to have acquired the main resource exclusively, as well as(?) the PagingIoResource

        // TODO: update the allocated size on-disk
        DPRINT("Allocated Size: %I64u\n", AttrContext->Record.NonResident.AllocatedSize);

        AttrContext->Record.NonResident.DataSize = DataSize->QuadPart;
        AttrContext->Record.NonResident.InitializedSize = DataSize->QuadPart;

        // copy the attribute record back into the FileRecord
        RtlCopyMemory((PCHAR)FileRecord + AttrOffset, &AttrContext->Record, AttrContext->Record.Length);
    }
    else
    {
        // resident attribute

        // find the next attribute
        ULONG NextAttributeOffset = AttrOffset + AttrContext->Record.Length;
        PNTFS_ATTR_RECORD NextAttribute = (PNTFS_ATTR_RECORD)((PCHAR)FileRecord + NextAttributeOffset);

        //NtfsDumpFileAttributes(Fcb->Vcb, FileRecord);

        // Do we need to increase the data length?
        if (DataSize->QuadPart > AttrContext->Record.Resident.ValueLength)
        {
            // There's usually padding at the end of a record. Do we need to extend past it?
            ULONG MaxValueLength = AttrContext->Record.Length - AttrContext->Record.Resident.ValueOffset;
            if (MaxValueLength < DataSize->LowPart)
            {
                // If this is the last attribute, we could move the end marker to the very end of the file record
                MaxValueLength += Fcb->Vcb->NtfsInfo.BytesPerFileRecord - NextAttributeOffset - (sizeof(ULONG) * 2);

                if (MaxValueLength < DataSize->LowPart || NextAttribute->Type != AttributeEnd)
                {
                    DPRINT1("FIXME: Need to convert attribute to non-resident!\n");
                    return STATUS_NOT_IMPLEMENTED;
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

        InternalSetResidentAttributeLength(AttrContext, FileRecord, AttrOffset, DataSize->LowPart);
    }

    //NtfsDumpFileAttributes(Fcb->Vcb, FileRecord);

    // write the updated file record back to disk
    Status = UpdateFileRecord(Fcb->Vcb, Fcb->MFTIndex, FileRecord);

    if (NT_SUCCESS(Status))
    {
        Fcb->RFCB.FileSize = *DataSize;
        Fcb->RFCB.ValidDataLength = *DataSize;
        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
    }

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
        LastLCN = 0;
        DataRun = (PUCHAR)&Context->Record + Context->Record.NonResident.MappingPairsOffset;
        CurrentOffset = 0;

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
                // (Presently, this code will never be reached, the write should have already failed by now)
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
                     ULONGLONG NewAllocationSize)
{
    PFILE_RECORD_HEADER MftRecord;
    PNTFS_ATTR_CONTEXT IndexRootCtx;
    PINDEX_ROOT_ATTRIBUTE IndexRoot;
    PCHAR IndexRecord;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry, IndexEntryEnd;
    NTSTATUS Status;
    ULONG CurrentEntry = 0;

    DPRINT("UpdateFileNameRecord(%p, %I64d, %wZ, %u, %I64u, %I64u)\n", Vcb, ParentMFTIndex, FileName, DirSearch, NewDataSize, NewAllocationSize);

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
                                          NewAllocationSize);

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
                             ULONGLONG NewAllocatedSize)
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
            CompareFileName(FileName, IndexEntry, DirSearch))
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

        Status = UpdateIndexEntryFileNameSize(NULL, NULL, NULL, 0, FirstEntry, LastEntry, FileName, StartEntry, CurrentEntry, DirSearch, NewDataSize, NewAllocatedSize);
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
* UpdateFileRecord
* @implemented
* Writes a file record to the master file table, at a given index.
*/
NTSTATUS
UpdateFileRecord(PDEVICE_EXTENSION Vcb,
                 ULONGLONG index,
                 PFILE_RECORD_HEADER file)
{
    ULONG BytesWritten;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("UpdateFileRecord(%p, %I64x, %p)\n", Vcb, index, file);

    // Add the fixup array to prepare the data for writing to disk
    AddFixupArray(Vcb, &file->Ntfs);

    // write the file record to the master file table
    Status = WriteAttribute(Vcb, Vcb->MFTContext, index * Vcb->NtfsInfo.BytesPerFileRecord, (const PUCHAR)file, Vcb->NtfsInfo.BytesPerFileRecord, &BytesWritten);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UpdateFileRecord failed: %I64u written, %u expected\n", BytesWritten, Vcb->NtfsInfo.BytesPerFileRecord);
    }

    // remove the fixup array (so the file record pointer can still be used)
    FixupUpdateSequenceArray(Vcb, file);

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
                BOOLEAN DirSearch)
{
    BOOLEAN Ret, Alloc = FALSE;
    UNICODE_STRING EntryName;

    EntryName.Buffer = IndexEntry->FileName.Name;
    EntryName.Length = 
    EntryName.MaximumLength = IndexEntry->FileName.NameLength * sizeof(WCHAR);

    if (DirSearch)
    {
        UNICODE_STRING IntFileName;
        if (IndexEntry->FileName.NameType != NTFS_FILE_NAME_POSIX)
        {
            NT_VERIFY(NT_SUCCESS(RtlUpcaseUnicodeString(&IntFileName, FileName, TRUE)));
            Alloc = TRUE;
        }
        else
        {
            IntFileName = *FileName;
        }

        Ret = FsRtlIsNameInExpression(&IntFileName, &EntryName, (IndexEntry->FileName.NameType != NTFS_FILE_NAME_POSIX), NULL);

        if (Alloc)
        {
            RtlFreeUnicodeString(&IntFileName);
        }

        return Ret;
    }
    else
    {
        return (RtlCompareUnicodeString(FileName, &EntryName, (IndexEntry->FileName.NameType != NTFS_FILE_NAME_POSIX)) == 0);
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
                   ULONGLONG *OutMFTIndex)
{
    NTSTATUS Status;
    ULONG RecordOffset;
    PINDEX_ENTRY_ATTRIBUTE IndexEntry;
    PNTFS_ATTR_CONTEXT IndexAllocationCtx;
    ULONGLONG IndexAllocationSize;
    PINDEX_BUFFER IndexBuffer;

    DPRINT("BrowseIndexEntries(%p, %p, %p, %u, %p, %p, %wZ, %u, %u, %u, %p)\n", Vcb, MftRecord, IndexRecord, IndexBlockSize, FirstEntry, LastEntry, FileName, *StartEntry, *CurrentEntry, DirSearch, OutMFTIndex);

    IndexEntry = FirstEntry;
    while (IndexEntry < LastEntry &&
           !(IndexEntry->Flags & NTFS_INDEX_ENTRY_END))
    {
        if ((IndexEntry->Data.Directory.IndexedFile & NTFS_MFT_MASK) > 0x10 &&
            *CurrentEntry >= *StartEntry &&
            IndexEntry->FileName.NameType != NTFS_FILE_NAME_DOS &&
            CompareFileName(FileName, IndexEntry, DirSearch))
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

        Status = BrowseIndexEntries(NULL, NULL, NULL, 0, FirstEntry, LastEntry, FileName, StartEntry, CurrentEntry, DirSearch, OutMFTIndex);
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
                  ULONGLONG *OutMFTIndex)
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

    Status = BrowseIndexEntries(Vcb, MftRecord, IndexRecord, IndexRoot->SizeOfEntry, IndexEntry, IndexEntryEnd, FileName, FirstEntry, &CurrentEntry, DirSearch, OutMFTIndex);

    ExFreePoolWithTag(IndexRecord, TAG_NTFS);
    ExFreePoolWithTag(MftRecord, TAG_NTFS);

    return Status;
}

NTSTATUS
NtfsLookupFileAt(PDEVICE_EXTENSION Vcb,
                 PUNICODE_STRING PathName,
                 PFILE_RECORD_HEADER *FileRecord,
                 PULONGLONG MFTIndex,
                 ULONGLONG CurrentMFTIndex)
{
    UNICODE_STRING Current, Remaining;
    NTSTATUS Status;
    ULONG FirstEntry = 0;

    DPRINT("NtfsLookupFileAt(%p, %wZ, %p, %I64x)\n", Vcb, PathName, FileRecord, CurrentMFTIndex);

    FsRtlDissectName(*PathName, &Current, &Remaining);

    while (Current.Length != 0)
    {
        DPRINT("Current: %wZ\n", &Current);

        Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, &Current, &FirstEntry, FALSE, &CurrentMFTIndex);
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
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex)
{
    return NtfsLookupFileAt(Vcb, PathName, FileRecord, MFTIndex, NTFS_FILE_ROOT);
}

NTSTATUS
NtfsFindFileAt(PDEVICE_EXTENSION Vcb,
               PUNICODE_STRING SearchPattern,
               PULONG FirstEntry,
               PFILE_RECORD_HEADER *FileRecord,
               PULONGLONG MFTIndex,
               ULONGLONG CurrentMFTIndex)
{
    NTSTATUS Status;

    DPRINT("NtfsFindFileAt(%p, %wZ, %u, %p, %p, %I64x)\n", Vcb, SearchPattern, *FirstEntry, FileRecord, MFTIndex, CurrentMFTIndex);

    Status = NtfsFindMftRecord(Vcb, CurrentMFTIndex, SearchPattern, FirstEntry, TRUE, &CurrentMFTIndex);
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
