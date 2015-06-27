/*
 *  ReactOS kernel
 *  Copyright (C) 2002,2003 ReactOS Team
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
 * FILE:             drivers/filesystem/ntfs/attrib.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMERS:      Eric Kohl
 *                   Valentin Verkhovsky
 *                   Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PUCHAR
DecodeRun(PUCHAR DataRun,
          LONGLONG *DataRunOffset,
          ULONGLONG *DataRunLength)
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

    DPRINT("DataRunOffsetSize: %x\n", DataRunOffsetSize);
    DPRINT("DataRunLengthSize: %x\n", DataRunLengthSize);
    DPRINT("DataRunOffset: %x\n", *DataRunOffset);
    DPRINT("DataRunLength: %x\n", *DataRunLength);

    return DataRun;
}

BOOLEAN
FindRun(PNTFS_ATTR_RECORD NresAttr,
        ULONGLONG vcn,
        PULONGLONG lcn,
        PULONGLONG count)
{
    if (vcn < NresAttr->NonResident.LowestVCN || vcn > NresAttr->NonResident.HighestVCN)
        return FALSE;

    DecodeRun((PUCHAR)((ULONG_PTR)NresAttr + NresAttr->NonResident.MappingPairsOffset), (PLONGLONG)lcn, count);

    return TRUE;
}


static
VOID
NtfsDumpFileNameAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PFILENAME_ATTRIBUTE FileNameAttr;

    DbgPrint("  $FILE_NAME ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    FileNameAttr = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" (%x) '%.*S' ", FileNameAttr->NameType, FileNameAttr->NameLength, FileNameAttr->Name);
    DbgPrint(" '%x' ", FileNameAttr->FileAttributes);
}


static
VOID
NtfsDumpStandardInformationAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PSTANDARD_INFORMATION StandardInfoAttr;

    DbgPrint("  $STANDARD_INFORMATION ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    StandardInfoAttr = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" '%x' ", StandardInfoAttr->FileAttribute);
}


static
VOID
NtfsDumpVolumeNameAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PWCHAR VolumeName;

    DbgPrint("  $VOLUME_NAME ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    VolumeName = (PWCHAR)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" '%.*S' ", Attribute->Resident.ValueLength / sizeof(WCHAR), VolumeName);
}


static
VOID
NtfsDumpVolumeInformationAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PVOLINFO_ATTRIBUTE VolInfoAttr;

    DbgPrint("  $VOLUME_INFORMATION ");

//    DbgPrint(" Length %lu  Offset %hu ", Attribute->Resident.ValueLength, Attribute->Resident.ValueOffset);

    VolInfoAttr = (PVOLINFO_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
    DbgPrint(" NTFS Version %u.%u  Flags 0x%04hx ",
             VolInfoAttr->MajorVersion,
             VolInfoAttr->MinorVersion,
             VolInfoAttr->Flags);
}


static
VOID
NtfsDumpIndexRootAttribute(PNTFS_ATTR_RECORD Attribute)
{
    PINDEX_ROOT_ATTRIBUTE IndexRootAttr;

    IndexRootAttr = (PINDEX_ROOT_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);

    if (IndexRootAttr->AttributeType == AttributeFileName)
        ASSERT(IndexRootAttr->CollationRule == COLLATION_FILE_NAME);

    DbgPrint("  $INDEX_ROOT (%uB, %u) ", IndexRootAttr->SizeOfEntry, IndexRootAttr->ClustersPerIndexRecord);

    if (IndexRootAttr->Header.Flags == INDEX_ROOT_SMALL)
    {
        DbgPrint(" (small) ");
    }
    else
    {
        ASSERT(IndexRootAttr->Header.Flags == INDEX_ROOT_LARGE);
        DbgPrint(" (large) ");
    }
}


static
VOID
NtfsDumpAttribute(PDEVICE_EXTENSION Vcb, PNTFS_ATTR_RECORD Attribute);


static
VOID
NtfsDumpAttributeListAttribute(PDEVICE_EXTENSION Vcb,
                               PNTFS_ATTR_RECORD Attribute)
{
    PNTFS_ATTR_CONTEXT ListContext;
    PVOID ListBuffer;
    ULONGLONG ListSize;

    ListContext = PrepareAttributeContext(Attribute);

    ListSize = AttributeDataLength(&ListContext->Record);
    if (ListSize <= 0xFFFFFFFF)
        ListBuffer = ExAllocatePoolWithTag(NonPagedPool, (ULONG)ListSize, TAG_NTFS);
    else
        ListBuffer = NULL;

    if (!ListBuffer)
    {
        DPRINT("Failed to allocate memory: %x\n", (ULONG)ListSize);
        return;
    }

    if (ReadAttribute(Vcb, ListContext, 0, ListBuffer, (ULONG)ListSize) == ListSize)
    {
        Attribute = (PNTFS_ATTR_RECORD)ListBuffer;
        while (Attribute < (PNTFS_ATTR_RECORD)((PCHAR)ListBuffer + ListSize) &&
               Attribute->Type != AttributeEnd)
        {
            NtfsDumpAttribute(Vcb, Attribute);

            Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
        }

        ReleaseAttributeContext(ListContext);
        ExFreePoolWithTag(ListBuffer, TAG_NTFS);
    }
}


static
VOID
NtfsDumpAttribute(PDEVICE_EXTENSION Vcb,
                  PNTFS_ATTR_RECORD Attribute)
{
    UNICODE_STRING Name;

    ULONGLONG lcn = 0;
    ULONGLONG runcount = 0;

    switch (Attribute->Type)
    {
        case AttributeFileName:
            NtfsDumpFileNameAttribute(Attribute);
            break;

        case AttributeStandardInformation:
            NtfsDumpStandardInformationAttribute(Attribute);
            break;

        case AttributeAttributeList:
            NtfsDumpAttributeListAttribute(Vcb, Attribute);
            break;

        case AttributeObjectId:
            DbgPrint("  $OBJECT_ID ");
            break;

        case AttributeSecurityDescriptor:
            DbgPrint("  $SECURITY_DESCRIPTOR ");
            break;

        case AttributeVolumeName:
            NtfsDumpVolumeNameAttribute(Attribute);
            break;

        case AttributeVolumeInformation:
            NtfsDumpVolumeInformationAttribute(Attribute);
            break;

        case AttributeData:
            DbgPrint("  $DATA ");
            //DataBuf = ExAllocatePool(NonPagedPool,AttributeLengthAllocated(Attribute));
            break;

        case AttributeIndexRoot:
            NtfsDumpIndexRootAttribute(Attribute);
            break;

        case AttributeIndexAllocation:
            DbgPrint("  $INDEX_ALLOCATION ");
            break;

        case AttributeBitmap:
            DbgPrint("  $BITMAP ");
            break;

        case AttributeReparsePoint:
            DbgPrint("  $REPARSE_POINT ");
            break;

        case AttributeEAInformation:
            DbgPrint("  $EA_INFORMATION ");
            break;

        case AttributeEA:
            DbgPrint("  $EA ");
            break;

        case AttributePropertySet:
            DbgPrint("  $PROPERTY_SET ");
            break;

        case AttributeLoggedUtilityStream:
            DbgPrint("  $LOGGED_UTILITY_STREAM ");
            break;

        default:
            DbgPrint("  Attribute %lx ",
                     Attribute->Type);
            break;
    }

    if (Attribute->Type != AttributeAttributeList)
    {
        if (Attribute->NameLength != 0)
        {
            Name.Length = Attribute->NameLength * sizeof(WCHAR);
            Name.MaximumLength = Name.Length;
            Name.Buffer = (PWCHAR)((ULONG_PTR)Attribute + Attribute->NameOffset);

            DbgPrint("'%wZ' ", &Name);
        }

        DbgPrint("(%s)\n",
                 Attribute->IsNonResident ? "non-resident" : "resident");

        if (Attribute->IsNonResident)
        {
            FindRun(Attribute,0,&lcn, &runcount);

            DbgPrint("  AllocatedSize %I64u  DataSize %I64u\n",
                     Attribute->NonResident.AllocatedSize, Attribute->NonResident.DataSize);
            DbgPrint("  logical clusters: %I64u - %I64u\n",
                     lcn, lcn + runcount - 1);
        }
    }
}


VOID
NtfsDumpFileAttributes(PDEVICE_EXTENSION Vcb,
                       PFILE_RECORD_HEADER FileRecord)
{
    PNTFS_ATTR_RECORD Attribute;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
           Attribute->Type != AttributeEnd)
    {
        NtfsDumpAttribute(Vcb, Attribute);

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }
}

PFILENAME_ATTRIBUTE
GetFileNameFromRecord(PFILE_RECORD_HEADER FileRecord, UCHAR NameType)
{
    PNTFS_ATTR_RECORD Attribute;
    PFILENAME_ATTRIBUTE Name;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
           Attribute->Type != AttributeEnd)
    {
        if (Attribute->Type == AttributeFileName)
        {
            Name = (PFILENAME_ATTRIBUTE)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
            if (Name->NameType == NameType ||
                (Name->NameType == NTFS_FILE_NAME_WIN32_AND_DOS && NameType == NTFS_FILE_NAME_WIN32) ||
                (Name->NameType == NTFS_FILE_NAME_WIN32_AND_DOS && NameType == NTFS_FILE_NAME_DOS))
            {
                return Name;
            }
        }

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    return NULL;
}

PSTANDARD_INFORMATION
GetStandardInformationFromRecord(PFILE_RECORD_HEADER FileRecord)
{
    PNTFS_ATTR_RECORD Attribute;
    PSTANDARD_INFORMATION StdInfo;

    Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->AttributeOffset);
    while (Attribute < (PNTFS_ATTR_RECORD)((ULONG_PTR)FileRecord + FileRecord->BytesInUse) &&
           Attribute->Type != AttributeEnd)
    {
        if (Attribute->Type == AttributeStandardInformation)
        {
            StdInfo = (PSTANDARD_INFORMATION)((ULONG_PTR)Attribute + Attribute->Resident.ValueOffset);
            return StdInfo;
        }

        Attribute = (PNTFS_ATTR_RECORD)((ULONG_PTR)Attribute + Attribute->Length);
    }

    return NULL;
}

PFILENAME_ATTRIBUTE
GetBestFileNameFromRecord(PFILE_RECORD_HEADER FileRecord)
{
    PFILENAME_ATTRIBUTE FileName;

    FileName = GetFileNameFromRecord(FileRecord, NTFS_FILE_NAME_POSIX);
    if (FileName == NULL)
    {
        FileName = GetFileNameFromRecord(FileRecord, NTFS_FILE_NAME_WIN32);
        if (FileName == NULL)
        {
            FileName = GetFileNameFromRecord(FileRecord, NTFS_FILE_NAME_DOS);
        }
    }

    return FileName;
}

/* EOF */
