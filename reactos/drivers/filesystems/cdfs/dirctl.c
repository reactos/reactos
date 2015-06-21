/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2004 ReactOS Team
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
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/cdfs/dirctl.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 *                   Eric Kohl
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

#define ROUND_DOWN(N, S) (((N) / (S)) * (S))
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

/* FUNCTIONS ****************************************************************/

/*
* FUNCTION: Retrieves the file name, be it in short or long file name format
*/
static NTSTATUS
CdfsGetEntryName(PDEVICE_EXTENSION DeviceExt,
                 PVOID *Context,
                 PVOID *Block,
                 PLARGE_INTEGER StreamOffset,
                 ULONG DirLength,
                 PVOID *Ptr,
                 PWSTR Name,
                 PULONG pIndex,
                 PULONG CurrentOffset)
{
    PDIR_RECORD Record = *Ptr;
    ULONG Index;

    if (*CurrentOffset >= DirLength)
        return(STATUS_NO_MORE_ENTRIES);

    if (*CurrentOffset == 0)
    {
        Index = 0;
        Record = (PDIR_RECORD)*Block;
        while (Index < *pIndex)
        {
            (*Ptr) = (PVOID)((ULONG_PTR)(*Ptr) + Record->RecordLength);
            (*CurrentOffset) += Record->RecordLength;
            Record = *Ptr;
            if ((ULONG_PTR)(*Ptr) - (ULONG_PTR)(*Block) >= BLOCKSIZE || Record->RecordLength == 0)
            {
                DPRINT("Map next sector\n");
                CcUnpinData(*Context);
                StreamOffset->QuadPart += BLOCKSIZE;
                *CurrentOffset = ROUND_UP(*CurrentOffset, BLOCKSIZE);
                if (!CcMapData(DeviceExt->StreamFileObject,
                    StreamOffset,
                    BLOCKSIZE, TRUE,
                    Context, Block))
                {
                    DPRINT("CcMapData() failed\n");
                    return(STATUS_UNSUCCESSFUL);
                }
                *Ptr = *Block;
                Record = (PDIR_RECORD)*Ptr;
            }
            if (*CurrentOffset >= DirLength)
                return(STATUS_NO_MORE_ENTRIES);

            Index++;
        }
    }

    if ((ULONG_PTR)(*Ptr) - (ULONG_PTR)(*Block) >= BLOCKSIZE || Record->RecordLength == 0)
    {
        DPRINT("Map next sector\n");
        CcUnpinData(*Context);
        StreamOffset->QuadPart += BLOCKSIZE;
        *CurrentOffset = ROUND_UP(*CurrentOffset, BLOCKSIZE);
        if (!CcMapData(DeviceExt->StreamFileObject,
            StreamOffset,
            BLOCKSIZE, TRUE,
            Context, Block))
        {
            DPRINT("CcMapData() failed\n");
            return(STATUS_UNSUCCESSFUL);
        }
        *Ptr = *Block;
        Record = (PDIR_RECORD)*Ptr;
    }

    if (*CurrentOffset >= DirLength)
        return STATUS_NO_MORE_ENTRIES;

    DPRINT("Index %lu  RecordLength %lu  Offset %lu\n",
        *pIndex, Record->RecordLength, *CurrentOffset);

    if (Record->FileIdLength == 1 && Record->FileId[0] == 0)
    {
        wcscpy(Name, L".");
    }
    else if (Record->FileIdLength == 1 && Record->FileId[0] == 1)
    {
        wcscpy(Name, L"..");
    }
    else
    {
        if (DeviceExt->CdInfo.JolietLevel == 0)
        {
            ULONG i;

            for (i = 0; i < Record->FileIdLength && Record->FileId[i] != ';'; i++)
                Name[i] = (WCHAR)Record->FileId[i];
            Name[i] = 0;
        }
        else
        {
            CdfsSwapString(Name, Record->FileId, Record->FileIdLength);
        }
    }

    DPRINT("Name '%S'\n", Name);

    *Ptr = Record;

    return(STATUS_SUCCESS);
}


/*
* FUNCTION: Find a file
*/
static NTSTATUS
CdfsFindFile(PDEVICE_EXTENSION DeviceExt,
             PFCB Fcb,
             PFCB Parent,
             PUNICODE_STRING FileToFind,
             PULONG pDirIndex,
             PULONG pOffset)
{
    WCHAR name[256];
    WCHAR ShortNameBuffer[13];
    UNICODE_STRING TempString;
    UNICODE_STRING ShortName;
    UNICODE_STRING LongName;
    UNICODE_STRING FileToFindUpcase;
    PVOID Block;
    NTSTATUS Status;
    ULONG len;
    ULONG DirIndex;
    ULONG Offset = 0;
    BOOLEAN IsRoot;
    PVOID Context = NULL;
    ULONG DirSize;
    PDIR_RECORD Record;
    LARGE_INTEGER StreamOffset, OffsetOfEntry;

    DPRINT("FindFile(Parent %p, FileToFind '%wZ', DirIndex: %u)\n",
        Parent, FileToFind, pDirIndex ? *pDirIndex : 0);
    DPRINT("FindFile: old Pathname %p, old Objectname %p)\n",
        Fcb->PathName, Fcb->ObjectName);

    IsRoot = FALSE;
    DirIndex = 0;

    if (FileToFind == NULL || FileToFind->Length == 0)
    {
        RtlInitUnicodeString(&TempString, L".");
        FileToFind = &TempString;
    }

    if (Parent)
    {
        if (Parent->Entry.ExtentLocationL == DeviceExt->CdInfo.RootStart)
        {
            IsRoot = TRUE;
        }
    }
    else
    {
        IsRoot = TRUE;
    }

    if (IsRoot == TRUE)
    {
        StreamOffset.QuadPart = (LONGLONG)DeviceExt->CdInfo.RootStart * (LONGLONG)BLOCKSIZE;
        DirSize = DeviceExt->CdInfo.RootSize;


        if (FileToFind->Buffer[0] == 0 ||
            (FileToFind->Buffer[0] == '\\' && FileToFind->Buffer[1] == 0) ||
            (FileToFind->Buffer[0] == '.' && FileToFind->Buffer[1] == 0))
        {
            /* it's root : complete essentials fields then return ok */
            RtlZeroMemory(Fcb, sizeof(FCB));
            RtlInitEmptyUnicodeString(&Fcb->PathName, Fcb->PathNameBuffer, sizeof(Fcb->PathNameBuffer));

            Fcb->PathNameBuffer[0] = '\\';
            Fcb->PathName.Length = sizeof(WCHAR);
            Fcb->ObjectName = &Fcb->PathNameBuffer[1];
            Fcb->Entry.ExtentLocationL = DeviceExt->CdInfo.RootStart;
            Fcb->Entry.DataLengthL = DeviceExt->CdInfo.RootSize;
            Fcb->Entry.FileFlags = 0x02; //FILE_ATTRIBUTE_DIRECTORY;

            if (pDirIndex)
                *pDirIndex = 0;
            if (pOffset)
                *pOffset = 0;
            DPRINT("CdfsFindFile: new Pathname %wZ, new Objectname %S)\n",&Fcb->PathName, Fcb->ObjectName);
            return STATUS_SUCCESS;
        }
    }
    else
    {
        StreamOffset.QuadPart = (LONGLONG)Parent->Entry.ExtentLocationL * (LONGLONG)BLOCKSIZE;
        DirSize = Parent->Entry.DataLengthL;
    }

    DPRINT("StreamOffset %I64d  DirSize %u\n", StreamOffset.QuadPart, DirSize);

    if (pDirIndex && (*pDirIndex))
        DirIndex = *pDirIndex;

    if (pOffset && (*pOffset))
    {
        Offset = *pOffset;
        StreamOffset.QuadPart += ROUND_DOWN(Offset, BLOCKSIZE);
    }

    if (!CcMapData(DeviceExt->StreamFileObject, &StreamOffset,
        BLOCKSIZE, TRUE, &Context, &Block))
    {
        DPRINT("CcMapData() failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    Record = (PDIR_RECORD) ((ULONG_PTR)Block + Offset % BLOCKSIZE);
    if (Offset)
    {
        Offset += Record->RecordLength;
        Record = (PDIR_RECORD)((ULONG_PTR)Record + Record->RecordLength);
    }

    /* Upper case the expression for FsRtlIsNameInExpression */
    Status = RtlUpcaseUnicodeString(&FileToFindUpcase, FileToFind, TRUE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    while(TRUE)
    {
        DPRINT("RecordLength %u  ExtAttrRecordLength %u  NameLength %u\n",
            Record->RecordLength, Record->ExtAttrRecordLength, Record->FileIdLength);

        Status = CdfsGetEntryName
            (DeviceExt, &Context, &Block, &StreamOffset,
            DirSize, (PVOID*)&Record, name, &DirIndex, &Offset);

        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        else if (Status == STATUS_UNSUCCESSFUL)
        {
            /* Note: the directory cache has already been unpinned */
            RtlFreeUnicodeString(&FileToFindUpcase);
            return Status;
        }

        DPRINT("Name '%S'\n", name);

        RtlInitUnicodeString(&LongName, name);
        ShortName.Length = 0;
        ShortName.MaximumLength = 26;
        ShortName.Buffer = ShortNameBuffer;

        OffsetOfEntry.QuadPart = StreamOffset.QuadPart + Offset;
        CdfsShortNameCacheGet(Parent, &OffsetOfEntry, &LongName, &ShortName);

        DPRINT("ShortName '%wZ'\n", &ShortName);

        if (FsRtlIsNameInExpression(&FileToFindUpcase, &LongName, TRUE, NULL) ||
            FsRtlIsNameInExpression(&FileToFindUpcase, &ShortName, TRUE, NULL))
        {
            if (Parent->PathName.Buffer[0])
            {
                RtlCopyUnicodeString(&Fcb->PathName, &Parent->PathName);
                len = Parent->PathName.Length / sizeof(WCHAR);
                Fcb->ObjectName=&Fcb->PathName.Buffer[len];
                if (len != 1 || Fcb->PathName.Buffer[0] != '\\')
                {
                    Fcb->ObjectName[0] = '\\';
                    Fcb->ObjectName = &Fcb->ObjectName[1];
                }
            }
            else
            {
                Fcb->ObjectName=Fcb->PathName.Buffer;
                Fcb->ObjectName[0]='\\';
                Fcb->ObjectName=&Fcb->ObjectName[1];
            }

            DPRINT("PathName '%wZ'  ObjectName '%S'\n", &Fcb->PathName, Fcb->ObjectName);

            memcpy(&Fcb->Entry, Record, sizeof(DIR_RECORD));
            wcsncpy(Fcb->ObjectName, name, min(wcslen(name) + 1,
                MAX_PATH - (Fcb->PathName.Length / sizeof(WCHAR)) + wcslen(Fcb->ObjectName)));

            /* Copy short name */
            Fcb->ShortNameU.Length = ShortName.Length;
            Fcb->ShortNameU.MaximumLength = ShortName.Length;
            Fcb->ShortNameU.Buffer = Fcb->ShortNameBuffer;
            memcpy(Fcb->ShortNameBuffer, ShortName.Buffer, ShortName.Length);

            if (pDirIndex)
                *pDirIndex = DirIndex;
            if (pOffset)
                *pOffset = Offset;

            DPRINT("FindFile: new Pathname %wZ, new Objectname %S, DirIndex %u\n",
                &Fcb->PathName, Fcb->ObjectName, DirIndex);

            RtlFreeUnicodeString(&FileToFindUpcase);
            CcUnpinData(Context);

            return STATUS_SUCCESS;
        }

        Offset += Record->RecordLength;
        Record = (PDIR_RECORD)((ULONG_PTR)Record + Record->RecordLength);
        DirIndex++;
    }

    RtlFreeUnicodeString(&FileToFindUpcase);
    CcUnpinData(Context);

    if (pDirIndex)
        *pDirIndex = DirIndex;

    if (pOffset)
        *pOffset = Offset;

    return STATUS_UNSUCCESSFUL;
}


static NTSTATUS
CdfsGetNameInformation(PFCB Fcb,
                       PDEVICE_EXTENSION DeviceExt,
                       PFILE_NAMES_INFORMATION Info,
                       ULONG BufferLength)
{
    ULONG Length;

    DPRINT("CdfsGetNameInformation() called\n");

    UNREFERENCED_PARAMETER(DeviceExt);

    Length = wcslen(Fcb->ObjectName) * sizeof(WCHAR);
    if ((sizeof(FILE_NAMES_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_NAMES_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, Fcb->ObjectName, Length);

    //  Info->FileIndex=;

    return(STATUS_SUCCESS);
}


static NTSTATUS
CdfsGetDirectoryInformation(PFCB Fcb,
                            PDEVICE_EXTENSION DeviceExt,
                            PFILE_DIRECTORY_INFORMATION Info,
                            ULONG BufferLength)
{
    ULONG Length;

    DPRINT("CdfsGetDirectoryInformation() called\n");

    UNREFERENCED_PARAMETER(DeviceExt);

    Length = wcslen(Fcb->ObjectName) * sizeof(WCHAR);
    if ((sizeof (FILE_DIRECTORY_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_DIRECTORY_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, Fcb->ObjectName, Length);

    /* Convert file times */
    CdfsDateTimeToSystemTime(Fcb,
        &Info->CreationTime);
    Info->LastWriteTime = Info->CreationTime;
    Info->ChangeTime = Info->CreationTime;

    /* Convert file flags */
    CdfsFileFlagsToAttributes(Fcb,
        &Info->FileAttributes);
    if (CdfsFCBIsDirectory(Fcb))
    {
        Info->EndOfFile.QuadPart = 0;
        Info->AllocationSize.QuadPart = 0;
    }
    else
    {
        Info->EndOfFile.QuadPart = Fcb->Entry.DataLengthL;

        /* Make AllocSize a rounded up multiple of the sector size */
        Info->AllocationSize.QuadPart = ROUND_UP(Fcb->Entry.DataLengthL, BLOCKSIZE);
    }

    //  Info->FileIndex=;

    return(STATUS_SUCCESS);
}


static NTSTATUS
CdfsGetFullDirectoryInformation(PFCB Fcb,
                                PDEVICE_EXTENSION DeviceExt,
                                PFILE_FULL_DIR_INFORMATION Info,
                                ULONG BufferLength)
{
    ULONG Length;

    DPRINT("CdfsGetFullDirectoryInformation() called\n");

    UNREFERENCED_PARAMETER(DeviceExt);

    Length = wcslen(Fcb->ObjectName) * sizeof(WCHAR);
    if ((sizeof (FILE_FULL_DIR_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_FULL_DIR_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, Fcb->ObjectName, Length);

    /* Convert file times */
    CdfsDateTimeToSystemTime(Fcb,
        &Info->CreationTime);
    Info->LastWriteTime = Info->CreationTime;
    Info->ChangeTime = Info->CreationTime;

    /* Convert file flags */
    CdfsFileFlagsToAttributes(Fcb,
        &Info->FileAttributes);

    if (CdfsFCBIsDirectory(Fcb))
    {
        Info->EndOfFile.QuadPart = 0;
        Info->AllocationSize.QuadPart = 0;
    }
    else
    {
        Info->EndOfFile.QuadPart = Fcb->Entry.DataLengthL;

        /* Make AllocSize a rounded up multiple of the sector size */
        Info->AllocationSize.QuadPart = ROUND_UP(Fcb->Entry.DataLengthL, BLOCKSIZE);
    }

    //  Info->FileIndex=;
    Info->EaSize = 0;

    return(STATUS_SUCCESS);
}


static NTSTATUS
CdfsGetBothDirectoryInformation(PFCB Fcb,
                                PDEVICE_EXTENSION DeviceExt,
                                PFILE_BOTH_DIR_INFORMATION Info,
                                ULONG BufferLength)
{
    ULONG Length;

    DPRINT("CdfsGetBothDirectoryInformation() called\n");

    UNREFERENCED_PARAMETER(DeviceExt);

    Length = wcslen(Fcb->ObjectName) * sizeof(WCHAR);
    if ((sizeof (FILE_BOTH_DIR_INFORMATION) + Length) > BufferLength)
        return(STATUS_BUFFER_OVERFLOW);

    Info->FileNameLength = Length;
    Info->NextEntryOffset =
        ROUND_UP(sizeof(FILE_BOTH_DIR_INFORMATION) + Length, sizeof(ULONG));
    RtlCopyMemory(Info->FileName, Fcb->ObjectName, Length);

    /* Convert file times */
    CdfsDateTimeToSystemTime(Fcb,
        &Info->CreationTime);
    Info->LastWriteTime = Info->CreationTime;
    Info->ChangeTime = Info->CreationTime;

    /* Convert file flags */
    CdfsFileFlagsToAttributes(Fcb,
        &Info->FileAttributes);

    if (CdfsFCBIsDirectory(Fcb))
    {
        Info->EndOfFile.QuadPart = 0;
        Info->AllocationSize.QuadPart = 0;
    }
    else
    {
        Info->EndOfFile.QuadPart = Fcb->Entry.DataLengthL;

        /* Make AllocSize a rounded up multiple of the sector size */
        Info->AllocationSize.QuadPart = ROUND_UP(Fcb->Entry.DataLengthL, BLOCKSIZE);
    }

    //  Info->FileIndex=;
    Info->EaSize = 0;

    /* Copy short name */
    ASSERT(Fcb->ShortNameU.Length / sizeof(WCHAR) <= 12);
    Info->ShortNameLength = (CCHAR)Fcb->ShortNameU.Length;
    RtlCopyMemory(Info->ShortName, Fcb->ShortNameU.Buffer, Fcb->ShortNameU.Length);

    return(STATUS_SUCCESS);
}


static NTSTATUS
CdfsQueryDirectory(PDEVICE_OBJECT DeviceObject,
                   PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    LONG BufferLength = 0;
    PUNICODE_STRING SearchPattern = NULL;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex = 0;
    PUCHAR Buffer = NULL;
    PFILE_NAMES_INFORMATION Buffer0 = NULL;
    PFCB Fcb;
    PCCB Ccb;
    FCB TempFcb;
    BOOLEAN First = FALSE;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("CdfsQueryDirectory() called\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;
    RtlInitEmptyUnicodeString(&TempFcb.PathName, TempFcb.PathNameBuffer, sizeof(TempFcb.PathNameBuffer));

    Ccb = (PCCB)FileObject->FsContext2;
    Fcb = (PFCB)FileObject->FsContext;

    /* Obtain the callers parameters */
    BufferLength = Stack->Parameters.QueryDirectory.Length;
    SearchPattern = Stack->Parameters.QueryDirectory.FileName;
    FileInformationClass =
        Stack->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = Stack->Parameters.QueryDirectory.FileIndex;

    /* Determine Buffer for result */
    if (Irp->MdlAddress)
    {
        Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    }
    else
    {
        Buffer = Irp->UserBuffer;
    }

    /* Allocate search pattern in case:
     * -> We don't have one already in context
     * -> We have been given an input pattern
     * -> The pattern length is not null
     * -> The pattern buffer is not null
     * Otherwise, we'll fall later and allocate a match all (*) pattern
     */
    if (SearchPattern != NULL &&
        SearchPattern->Length != 0 && SearchPattern->Buffer != NULL)
    {
        if (Ccb->DirectorySearchPattern.Buffer == NULL)
        {
            First = TRUE;
            Ccb->DirectorySearchPattern.Buffer =
                ExAllocatePoolWithTag(NonPagedPool, SearchPattern->Length + sizeof(WCHAR), CDFS_SEARCH_PATTERN_TAG);
            if (Ccb->DirectorySearchPattern.Buffer == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            Ccb->DirectorySearchPattern.MaximumLength = SearchPattern->Length + sizeof(WCHAR);
            RtlCopyUnicodeString(&Ccb->DirectorySearchPattern, SearchPattern);
            Ccb->DirectorySearchPattern.Buffer[SearchPattern->Length / sizeof(WCHAR)] = 0;
        }
    }
    else if (Ccb->DirectorySearchPattern.Buffer == NULL)
    {
        First = TRUE;
        Ccb->DirectorySearchPattern.Buffer = ExAllocatePoolWithTag(NonPagedPool, 2 * sizeof(WCHAR), CDFS_SEARCH_PATTERN_TAG);
        if (Ccb->DirectorySearchPattern.Buffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Ccb->DirectorySearchPattern.Length = sizeof(WCHAR);
        Ccb->DirectorySearchPattern.MaximumLength = 2 * sizeof(WCHAR);
        Ccb->DirectorySearchPattern.Buffer[0] = L'*';
        Ccb->DirectorySearchPattern.Buffer[1] = 0;
    }
    DPRINT("Search pattern '%wZ'\n", &Ccb->DirectorySearchPattern);

    /* Determine directory index */
    if (Stack->Flags & SL_INDEX_SPECIFIED)
    {
        Ccb->Entry = Stack->Parameters.QueryDirectory.FileIndex;
        Ccb->Offset = Ccb->CurrentByteOffset.u.LowPart;
    }
    else if (First || (Stack->Flags & SL_RESTART_SCAN))
    {
        Ccb->Entry = 0;
        Ccb->Offset = 0;
    }
    DPRINT("Buffer = %p  tofind = %wZ\n", Buffer, &Ccb->DirectorySearchPattern);

    TempFcb.ObjectName = TempFcb.PathName.Buffer;
    while (Status == STATUS_SUCCESS && BufferLength > 0)
    {
        Status = CdfsFindFile(DeviceExtension,
            &TempFcb,
            Fcb,
            &Ccb->DirectorySearchPattern,
            &Ccb->Entry,
            &Ccb->Offset);
        DPRINT("Found %S, Status=%x, entry %x\n", TempFcb.ObjectName, Status, Ccb->Entry);

        if (NT_SUCCESS(Status))
        {
            switch (FileInformationClass)
            {
            case FileNameInformation:
                Status = CdfsGetNameInformation(&TempFcb,
                    DeviceExtension,
                    (PFILE_NAMES_INFORMATION)Buffer,
                    BufferLength);
                break;

            case FileDirectoryInformation:
                Status = CdfsGetDirectoryInformation(&TempFcb,
                    DeviceExtension,
                    (PFILE_DIRECTORY_INFORMATION)Buffer,
                    BufferLength);
                break;

            case FileFullDirectoryInformation:
                Status = CdfsGetFullDirectoryInformation(&TempFcb,
                    DeviceExtension,
                    (PFILE_FULL_DIR_INFORMATION)Buffer,
                    BufferLength);
                break;

            case FileBothDirectoryInformation:
                Status = CdfsGetBothDirectoryInformation(&TempFcb,
                    DeviceExtension,
                    (PFILE_BOTH_DIR_INFORMATION)Buffer,
                    BufferLength);
                break;

            default:
                Status = STATUS_INVALID_INFO_CLASS;
            }

            if (Status == STATUS_BUFFER_OVERFLOW)
            {
                if (Buffer0)
                {
                    Buffer0->NextEntryOffset = 0;
                }
                break;
            }
        }
        else
        {
            if (Buffer0)
            {
                Buffer0->NextEntryOffset = 0;
            }

            if (First)
            {
                Status = STATUS_NO_SUCH_FILE;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
            break;
        }

        Buffer0 = (PFILE_NAMES_INFORMATION)Buffer;
        Buffer0->FileIndex = FileIndex++;
        Ccb->Entry++;

        if (Stack->Flags & SL_RETURN_SINGLE_ENTRY)
        {
            break;
        }
        BufferLength -= Buffer0->NextEntryOffset;
        Buffer += Buffer0->NextEntryOffset;
    }

    if (Buffer0)
    {
        Buffer0->NextEntryOffset = 0;
    }

    if (FileIndex > 0)
    {
        Status = STATUS_SUCCESS;
    }

    return(Status);
}


static NTSTATUS
CdfsNotifyChangeDirectory(PDEVICE_OBJECT DeviceObject,
                          PIRP Irp,
                          PCDFS_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExtension;
    PFCB Fcb;
    PCCB Ccb;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;

    DPRINT("CdfsNotifyChangeDirectory() called\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;

    Ccb = (PCCB)FileObject->FsContext2;
    Fcb = (PFCB)FileObject->FsContext;
 
    FsRtlNotifyFullChangeDirectory(DeviceExtension->NotifySync,
                                   &(DeviceExtension->NotifyList),
                                   Ccb,
                                   (PSTRING)&(Fcb->PathName),
                                   BooleanFlagOn(Stack->Flags, SL_WATCH_TREE),
                                   FALSE,
                                   Stack->Parameters.NotifyDirectory.CompletionFilter,
                                   Irp,
                                   NULL,
                                   NULL);

    /* We won't handle IRP completion */
    IrpContext->Flags &= ~IRPCONTEXT_COMPLETE;

    return STATUS_PENDING;
}


NTSTATUS NTAPI
CdfsDirectoryControl(
    PCDFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;

    DPRINT("CdfsDirectoryControl() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;

    switch (IrpContext->MinorFunction)
    {
    case IRP_MN_QUERY_DIRECTORY:
        Status = CdfsQueryDirectory(DeviceObject,
            Irp);
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = CdfsNotifyChangeDirectory(DeviceObject,
            Irp, IrpContext);
        break;

    default:
        DPRINT1("CDFS: MinorFunction %u\n", IrpContext->MinorFunction);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = 0;
    }

    return(Status);
}

/* EOF */
