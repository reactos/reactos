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
 * FILE:             services/fs/cdfs/fcb.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "cdfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


/* FUNCTIONS ****************************************************************/

static PWCHAR
CdfsGetNextPathElement(PWCHAR FileName)
{
    if (*FileName == L'\0')
    {
        return(NULL);
    }

    while (*FileName != L'\0' && *FileName != L'\\')
    {
        FileName++;
    }

    return(FileName);
}


static VOID
CdfsWSubString(LPWSTR pTarget, LPCWSTR pSource, size_t pLength)
{
    wcsncpy (pTarget, pSource, pLength);
    pTarget [pLength] = L'\0';
}


PFCB
CdfsCreateFCB(PCWSTR FileName)
{
    PFCB Fcb;

    Fcb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(FCB),
                                CDFS_NONPAGED_FCB_TAG);
    if(!Fcb) return NULL;

    RtlZeroMemory(Fcb, sizeof(FCB));
    RtlInitEmptyUnicodeString(&Fcb->PathName, Fcb->PathNameBuffer, sizeof(Fcb->PathNameBuffer));

    if (FileName)
    {
        RtlAppendUnicodeToString(&Fcb->PathName, FileName);
        if (wcsrchr(Fcb->PathName.Buffer, '\\') != 0)
        {
            Fcb->ObjectName = wcsrchr(Fcb->PathName.Buffer, '\\');
        }
        else
        {
            Fcb->ObjectName = Fcb->PathName.Buffer;
        }
    }

    ExInitializeResourceLite(&Fcb->PagingIoResource);
    ExInitializeResourceLite(&Fcb->MainResource);
    ExInitializeResourceLite(&Fcb->NameListResource);
    Fcb->RFCB.PagingIoResource = &Fcb->PagingIoResource;
    Fcb->RFCB.Resource = &Fcb->MainResource;
    Fcb->RFCB.IsFastIoPossible = FastIoIsNotPossible;
    InitializeListHead(&Fcb->ShortNameList);

    return(Fcb);
}


VOID
CdfsDestroyFCB(PFCB Fcb)
{
    PLIST_ENTRY Entry;

    ExDeleteResourceLite(&Fcb->PagingIoResource);
    ExDeleteResourceLite(&Fcb->MainResource);

    while (!IsListEmpty(&Fcb->ShortNameList))
    {
        Entry = Fcb->ShortNameList.Flink;
        RemoveEntryList(Entry);
        ExFreePoolWithTag(Entry, CDFS_SHORT_NAME_TAG);
    }

    ExDeleteResourceLite(&Fcb->NameListResource);
    ExFreePoolWithTag(Fcb, CDFS_NONPAGED_FCB_TAG);
}


BOOLEAN
CdfsFCBIsDirectory(PFCB Fcb)
{
    return(Fcb->Entry.FileFlags & FILE_FLAG_DIRECTORY);
}


BOOLEAN
CdfsFCBIsRoot(PFCB Fcb)
{
    return (Fcb->PathName.Length == sizeof(WCHAR) && Fcb->PathName.Buffer[0] == L'\\');
}


VOID
CdfsGrabFCB(PDEVICE_EXTENSION Vcb,
            PFCB Fcb)
{
    KIRQL  oldIrql;

    DPRINT("grabbing FCB at %p: %S, refCount:%d\n",
        Fcb,
        Fcb->PathName,
        Fcb->RefCount);

    KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
    Fcb->RefCount++;
    KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


VOID
CdfsReleaseFCB(PDEVICE_EXTENSION Vcb,
               PFCB Fcb)
{
    KIRQL  oldIrql;

    DPRINT("releasing FCB at %p: %wZ, refCount:%d\n",
        Fcb,
        &Fcb->PathName,
        Fcb->RefCount);

    KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
    Fcb->RefCount--;
    if (Fcb->RefCount <= 0 && !CdfsFCBIsDirectory(Fcb))
    {
        RemoveEntryList(&Fcb->FcbListEntry);
        CdfsDestroyFCB(Fcb);
    }
    KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


VOID
CdfsAddFCBToTable(PDEVICE_EXTENSION Vcb,
                  PFCB Fcb)
{
    KIRQL  oldIrql;

    KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
    Fcb->DevExt = Vcb;
    InsertTailList(&Vcb->FcbListHead, &Fcb->FcbListEntry);
    KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


PFCB
CdfsGrabFCBFromTable(PDEVICE_EXTENSION Vcb,
                     PUNICODE_STRING FileName)
{
    KIRQL  oldIrql;
    PFCB Fcb;
    PLIST_ENTRY  current_entry;

    KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);

    if (FileName == NULL || FileName->Length == 0 || FileName->Buffer[0] == 0)
    {
        DPRINT("Return FCB for stream file object\n");
        Fcb = Vcb->StreamFileObject->FsContext;
        Fcb->RefCount++;
        KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
        return(Fcb);
    }

    current_entry = Vcb->FcbListHead.Flink;
    while (current_entry != &Vcb->FcbListHead)
    {
        Fcb = CONTAINING_RECORD(current_entry, FCB, FcbListEntry);

        DPRINT("Comparing '%wZ' and '%wZ'\n", FileName, &Fcb->PathName);
        if (RtlCompareUnicodeString(FileName, &Fcb->PathName, TRUE) == 0)
        {
            Fcb->RefCount++;
            KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
            return(Fcb);
        }

        //FIXME: need to compare against short name in FCB here

        current_entry = current_entry->Flink;
    }
    KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);

    return(NULL);
}


NTSTATUS
CdfsFCBInitializeCache(PVCB Vcb,
                       PFCB Fcb)
{
    PFILE_OBJECT FileObject;
    PCCB  newCCB;

    FileObject = IoCreateStreamFileObject(NULL, Vcb->StorageDevice);

    newCCB = ExAllocatePoolWithTag(NonPagedPool, sizeof(CCB), CDFS_CCB_TAG);
    if (newCCB == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(newCCB,
        sizeof(CCB));

    FileObject->ReadAccess = TRUE;
    FileObject->WriteAccess = FALSE;
    FileObject->DeleteAccess = FALSE;
    FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = newCCB;
    newCCB->PtrFileObject = FileObject;
    Fcb->FileObject = FileObject;
    Fcb->DevExt = Vcb;

    _SEH2_TRY
    {
        CcInitializeCacheMap(FileObject,
            (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
            FALSE,
            &(CdfsGlobalData->CacheMgrCallbacks),
            Fcb);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        FileObject->FsContext2 = NULL;
        ExFreePoolWithTag(newCCB, CDFS_CCB_TAG);
        ObDereferenceObject(FileObject);
        Fcb->FileObject = NULL;
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(FileObject);
    Fcb->Flags |= FCB_CACHE_INITIALIZED;

    return STATUS_SUCCESS;
}


PFCB
CdfsMakeRootFCB(PDEVICE_EXTENSION Vcb)
{
    PFCB Fcb;

    Fcb = CdfsCreateFCB(L"\\");

    Fcb->Entry.DataLengthL = Vcb->CdInfo.RootSize;
    Fcb->Entry.ExtentLocationL = Vcb->CdInfo.RootStart;
    Fcb->Entry.FileFlags = FILE_FLAG_DIRECTORY;
    Fcb->IndexNumber.QuadPart = 0LL;
    Fcb->RefCount = 1;
    Fcb->DirIndex = 0;
    Fcb->RFCB.FileSize.QuadPart = Vcb->CdInfo.RootSize;
    Fcb->RFCB.ValidDataLength.QuadPart = Vcb->CdInfo.RootSize;
    Fcb->RFCB.AllocationSize.QuadPart = Vcb->CdInfo.RootSize;

    CdfsFCBInitializeCache(Vcb, Fcb);
    CdfsAddFCBToTable(Vcb, Fcb);
    CdfsGrabFCB(Vcb, Fcb);

    return(Fcb);
}


PFCB
CdfsOpenRootFCB(PDEVICE_EXTENSION Vcb)
{
    UNICODE_STRING FileName;
    PFCB Fcb;

    RtlInitUnicodeString(&FileName, L"\\");

    Fcb = CdfsGrabFCBFromTable(Vcb,
        &FileName);
    if (Fcb == NULL)
    {
        Fcb = CdfsMakeRootFCB(Vcb);
    }

    return(Fcb);
}


static VOID
CdfsGetDirEntryName(PDEVICE_EXTENSION DeviceExt,
                    PDIR_RECORD Record,
                    PWSTR Name)
                    /*
                    * FUNCTION: Retrieves the file name from a directory record.
                    */
{
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
            CdfsSwapString(Name,
                Record->FileId,
                Record->FileIdLength);
        }
    }

    DPRINT("Name '%S'\n", Name);
}


NTSTATUS
CdfsMakeFCBFromDirEntry(PVCB Vcb,
                        PFCB DirectoryFCB,
                        PWSTR LongName,
                        PWSTR ShortName,
                        PDIR_RECORD Record,
                        ULONG DirectorySector,
                        ULONG DirectoryOffset,
                        PFCB * fileFCB)
{
    WCHAR pathName[MAX_PATH];
    PFCB rcFCB;
    ULONG Size;

    /* Check if the full string would overflow the pathName buffer (the additional characters are for '\\' and '\0') */
    if ((LongName[0] != 0) &&
        ((DirectoryFCB->PathName.Length / sizeof(WCHAR)) + 1 + wcslen(LongName) + 1 > MAX_PATH))
    {
        return(STATUS_OBJECT_NAME_INVALID);
    }

    wcscpy(pathName, DirectoryFCB->PathName.Buffer);
    if (!CdfsFCBIsRoot(DirectoryFCB))
    {
        wcscat(pathName, L"\\");
    }

    if (LongName[0] != 0)
    {
        wcscat(pathName, LongName);
    }
    else
    {
        WCHAR entryName[MAX_PATH];

        CdfsGetDirEntryName(Vcb, Record, entryName);
        wcscat(pathName, entryName);
    }

    rcFCB = CdfsCreateFCB(pathName);
    memcpy(&rcFCB->Entry, Record, sizeof(DIR_RECORD));

    /* Copy short name into FCB */
    rcFCB->ShortNameU.Length = wcslen(ShortName) * sizeof(WCHAR);
    rcFCB->ShortNameU.MaximumLength = rcFCB->ShortNameU.Length;
    rcFCB->ShortNameU.Buffer = rcFCB->ShortNameBuffer;
    wcscpy(rcFCB->ShortNameBuffer, ShortName);

    Size = rcFCB->Entry.DataLengthL;

    rcFCB->RFCB.FileSize.QuadPart = Size;
    rcFCB->RFCB.ValidDataLength.QuadPart = Size;
    rcFCB->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, BLOCKSIZE);
    if (CdfsFCBIsDirectory(rcFCB))
    {
        CdfsFCBInitializeCache(Vcb, rcFCB);
    }
    rcFCB->IndexNumber.u.HighPart = DirectorySector;
    rcFCB->IndexNumber.u.LowPart = DirectoryOffset;
    rcFCB->RefCount++;
    CdfsAddFCBToTable(Vcb, rcFCB);
    *fileFCB = rcFCB;

    DPRINT("%S %u %I64d\n", LongName, Size, rcFCB->RFCB.AllocationSize.QuadPart);

    return(STATUS_SUCCESS);
}


NTSTATUS
CdfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
                          PFCB Fcb,
                          PFILE_OBJECT FileObject)
{
    PCCB  newCCB;

    newCCB = ExAllocatePoolWithTag(NonPagedPool, sizeof(CCB), CDFS_CCB_TAG);
    if (newCCB == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    memset(newCCB, 0, sizeof(CCB));

    FileObject->ReadAccess = TRUE;
    FileObject->WriteAccess = FALSE;
    FileObject->DeleteAccess = FALSE;
    FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = newCCB;
    newCCB->PtrFileObject = FileObject;
    Fcb->DevExt = Vcb;

    if (CdfsFCBIsDirectory(Fcb))
    {
        _SEH2_TRY
        {
            CcInitializeCacheMap(FileObject,
                (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                FALSE,
                &(CdfsGlobalData->CacheMgrCallbacks),
                Fcb);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            FileObject->FsContext2 = NULL;
            ExFreePoolWithTag(newCCB, CDFS_CCB_TAG);
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        Fcb->Flags |= FCB_CACHE_INITIALIZED;
    }

    DPRINT("file open: fcb:%p file size: %u\n", Fcb, Fcb->Entry.DataLengthL);

    return(STATUS_SUCCESS);
}


NTSTATUS
CdfsDirFindFile(PDEVICE_EXTENSION DeviceExt,
                PFCB DirectoryFcb,
                PUNICODE_STRING FileToFind,
                PFCB *FoundFCB)
{
    UNICODE_STRING TempName;
    WCHAR Name[256];
    PVOID Block;
    ULONG DirSize;
    PDIR_RECORD Record;
    ULONG Offset;
    ULONG BlockOffset;
    NTSTATUS Status;

    LARGE_INTEGER StreamOffset, OffsetOfEntry;
    PVOID Context;

    WCHAR ShortNameBuffer[13];
    UNICODE_STRING ShortName;
    UNICODE_STRING LongName;
    UNICODE_STRING FileToFindUpcase;

    ASSERT(DeviceExt);
    ASSERT(DirectoryFcb);
    ASSERT(FileToFind);

    DPRINT("CdfsDirFindFile(VCB:%p, dirFCB:%p, File:%wZ)\n",
        DeviceExt,
        DirectoryFcb,
        FileToFind);
    DPRINT("Dir Path:%wZ\n", &DirectoryFcb->PathName);

    /* default to '.' if no filename specified */
    if (FileToFind->Length == 0)
    {
        RtlInitUnicodeString(&TempName, L".");
        FileToFind = &TempName;
    }

    DirSize = DirectoryFcb->Entry.DataLengthL;
    StreamOffset.QuadPart = (LONGLONG)DirectoryFcb->Entry.ExtentLocationL * (LONGLONG)BLOCKSIZE;

    if (!CcMapData(DeviceExt->StreamFileObject,
        &StreamOffset,
        BLOCKSIZE,
        TRUE,
        &Context,
        &Block))
    {
        DPRINT("CcMapData() failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    Offset = 0;
    BlockOffset = 0;
    Record = (PDIR_RECORD)Block;

    /* Upper case the expression for FsRtlIsNameInExpression */
    Status = RtlUpcaseUnicodeString(&FileToFindUpcase, FileToFind, TRUE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    while(TRUE)
    {
        if (Record->RecordLength == 0)
        {
            DPRINT("RecordLength == 0  Stopped!\n");
            break;
        }

        DPRINT("RecordLength %u  ExtAttrRecordLength %u  NameLength %u\n",
            Record->RecordLength, Record->ExtAttrRecordLength, Record->FileIdLength);

        CdfsGetDirEntryName(DeviceExt, Record, Name);
        DPRINT ("Name '%S'\n", Name);
        DPRINT ("Sector %lu\n", DirectoryFcb->Entry.ExtentLocationL);
        DPRINT ("Offset %lu\n", Offset);

        RtlInitUnicodeString(&LongName, Name);
        RtlInitEmptyUnicodeString(&ShortName, ShortNameBuffer, sizeof(ShortNameBuffer));
        RtlZeroMemory(ShortNameBuffer, sizeof(ShortNameBuffer));

        OffsetOfEntry.QuadPart = StreamOffset.QuadPart + Offset;
        CdfsShortNameCacheGet(DirectoryFcb, &OffsetOfEntry, &LongName, &ShortName);

        DPRINT("ShortName '%wZ'\n", &ShortName);

        ASSERT(LongName.Length >= sizeof(WCHAR));
        ASSERT(ShortName.Length >= sizeof(WCHAR));
        if (FsRtlIsNameInExpression(&FileToFindUpcase, &LongName, TRUE, NULL) ||
            FsRtlIsNameInExpression(&FileToFindUpcase, &ShortName, TRUE, NULL))
        {
            DPRINT("Match found, %S\n", Name);
            Status = CdfsMakeFCBFromDirEntry(DeviceExt,
                DirectoryFcb,
                Name,
                ShortNameBuffer,
                Record,
                DirectoryFcb->Entry.ExtentLocationL,
                Offset,
                FoundFCB);

            RtlFreeUnicodeString(&FileToFindUpcase);
            CcUnpinData(Context);

            return(Status);
        }

        Offset += Record->RecordLength;
        BlockOffset += Record->RecordLength;
        Record = (PDIR_RECORD)((ULONG_PTR)Block + BlockOffset);
        if (BlockOffset >= BLOCKSIZE || Record->RecordLength == 0)
        {
            DPRINT("Map next sector\n");
            CcUnpinData(Context);
            StreamOffset.QuadPart += BLOCKSIZE;
            Offset = ROUND_UP(Offset, BLOCKSIZE);
            BlockOffset = 0;

            if (!CcMapData(DeviceExt->StreamFileObject,
                &StreamOffset,
                BLOCKSIZE, TRUE,
                &Context, &Block))
            {
                DPRINT("CcMapData() failed\n");
                RtlFreeUnicodeString(&FileToFindUpcase);
                return(STATUS_UNSUCCESSFUL);
            }
            Record = (PDIR_RECORD)((ULONG_PTR)Block + BlockOffset);
        }

        if (Offset >= DirSize)
            break;
    }

    RtlFreeUnicodeString(&FileToFindUpcase);
    CcUnpinData(Context);

    return(STATUS_OBJECT_NAME_NOT_FOUND);
}


NTSTATUS
CdfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
                  PFCB *pParentFCB,
                  PFCB *pFCB,
                  PUNICODE_STRING FileName)
{
    UNICODE_STRING PathName;
    UNICODE_STRING ElementName;
    NTSTATUS Status;
    WCHAR  pathName [MAX_PATH];
    WCHAR  elementName [MAX_PATH];
    PWCHAR  currentElement;
    PFCB  FCB;
    PFCB  parentFCB;

    DPRINT("CdfsGetFCBForFile(%p, %p, %p, '%wZ')\n",
        Vcb,
        pParentFCB,
        pFCB,
        FileName);

    /* Trivial case, open of the root directory on volume */
    if (FileName->Buffer[0] == L'\0' || wcscmp(FileName->Buffer, L"\\") == 0)
    {
        DPRINT("returning root FCB\n");

        FCB = CdfsOpenRootFCB(Vcb);
        *pFCB = FCB;
        *pParentFCB = NULL;

        return((FCB != NULL) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND);
    }
    else
    {
        currentElement = &FileName->Buffer[1];
        wcscpy (pathName, L"\\");
        FCB = CdfsOpenRootFCB (Vcb);
    }
    parentFCB = NULL;

    /* Parse filename and check each path element for existance and access */
    while (CdfsGetNextPathElement(currentElement) != 0)
    {
        /*  Skip blank directory levels */
        if ((CdfsGetNextPathElement(currentElement) - currentElement) == 0)
        {
            currentElement++;
            continue;
        }

        DPRINT("Parsing, currentElement:%S\n", currentElement);
        DPRINT("  parentFCB:%p FCB:%p\n", parentFCB, FCB);

        /* Descend to next directory level */
        if (parentFCB)
        {
            CdfsReleaseFCB(Vcb, parentFCB);
            parentFCB = NULL;
        }

        /* fail if element in FCB is not a directory */
        if (!CdfsFCBIsDirectory(FCB))
        {
            DPRINT("Element in requested path is not a directory\n");

            CdfsReleaseFCB(Vcb, FCB);
            FCB = 0;
            *pParentFCB = NULL;
            *pFCB = NULL;

            return(STATUS_OBJECT_PATH_NOT_FOUND);
        }
        parentFCB = FCB;

        /* Extract next directory level into dirName */
        CdfsWSubString(pathName,
            FileName->Buffer,
            CdfsGetNextPathElement(currentElement) - FileName->Buffer);
        DPRINT("  pathName:%S\n", pathName);

        RtlInitUnicodeString(&PathName, pathName);

        FCB = CdfsGrabFCBFromTable(Vcb, &PathName);
        if (FCB == NULL)
        {
            CdfsWSubString(elementName,
                currentElement,
                CdfsGetNextPathElement(currentElement) - currentElement);
            DPRINT("  elementName:%S\n", elementName);

            RtlInitUnicodeString(&ElementName, elementName);
            Status = CdfsDirFindFile(Vcb,
                parentFCB,
                &ElementName,
                &FCB);
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                *pParentFCB = parentFCB;
                *pFCB = NULL;
                currentElement = CdfsGetNextPathElement(currentElement);
                if (*currentElement == L'\0' || CdfsGetNextPathElement(currentElement + 1) == 0)
                {
                    return(STATUS_OBJECT_NAME_NOT_FOUND);
                }
                else
                {
                    return(STATUS_OBJECT_PATH_NOT_FOUND);
                }
            }
            else if (!NT_SUCCESS(Status))
            {
                CdfsReleaseFCB(Vcb, parentFCB);
                *pParentFCB = NULL;
                *pFCB = NULL;

                return(Status);
            }
        }
        currentElement = CdfsGetNextPathElement(currentElement);
    }

    *pParentFCB = parentFCB;
    *pFCB = FCB;

    return STATUS_SUCCESS;
}

/* EOF */
