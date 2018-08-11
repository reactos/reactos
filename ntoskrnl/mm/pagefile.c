/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pagefile.c
 * PURPOSE:         Paging file functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitPagingFile)
#endif

PVOID
NTAPI
MiFindExportedRoutineByName(IN PVOID DllBase,
                            IN PANSI_STRING ExportName);

/* TYPES *********************************************************************/

typedef struct _PAGINGFILE
{
    LIST_ENTRY PagingFileListEntry;
    PFILE_OBJECT FileObject;
    HANDLE FileHandle;
    LARGE_INTEGER MaximumSize;
    LARGE_INTEGER CurrentSize;
    PFN_NUMBER FreePages;
    PFN_NUMBER UsedPages;
    PULONG AllocMap;
    KSPIN_LOCK AllocMapLock;
    ULONG AllocMapSize;
}
PAGINGFILE, *PPAGINGFILE;

/* GLOBALS *******************************************************************/

#define PAIRS_PER_RUN (1024)

#define MAX_PAGING_FILES  (16)

/* List of paging files, both used and free */
static PPAGINGFILE PagingFileList[MAX_PAGING_FILES];

/* Lock for examining the list of paging files */
static KSPIN_LOCK PagingFileListLock;

/* Number of paging files */
ULONG MmNumberOfPagingFiles;

/* Number of pages that are available for swapping */
PFN_COUNT MiFreeSwapPages;

/* Number of pages that have been allocated for swapping */
PFN_COUNT MiUsedSwapPages;

BOOLEAN MmZeroPageFile;

/*
 * Number of pages that have been reserved for swapping but not yet allocated
 */
static PFN_COUNT MiReservedSwapPages;

/*
 * Ratio between reserved and available swap pages, e.g. setting this to five
 * forces one swap page to be available for every five swap pages that are
 * reserved. Setting this to zero turns off commit checking altogether.
 */
#define MM_PAGEFILE_COMMIT_RATIO      (1)

/*
 * Number of pages that can be used for potentially swapable memory without
 * pagefile space being reserved. The intention is that this allows smss
 * to start up and create page files while ordinarily having a commit
 * ratio of one.
 */
#define MM_PAGEFILE_COMMIT_GRACE      (256)

/*
 * Translate between a swap entry and a file and offset pair.
 */
#define FILE_FROM_ENTRY(i) ((i) & 0x0f)
#define OFFSET_FROM_ENTRY(i) ((i) >> 11)
#define ENTRY_FROM_FILE_OFFSET(i, j) ((i) | ((j) << 11) | 0x400)

/* Make sure there can be only 16 paging files */
C_ASSERT(FILE_FROM_ENTRY(0xffffffff) < MAX_PAGING_FILES);

static BOOLEAN MmSwapSpaceMessage = FALSE;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MmBuildMdlFromPages(PMDL Mdl, PPFN_NUMBER Pages)
{
    memcpy(Mdl + 1, Pages, sizeof(PFN_NUMBER) * (PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGE_SIZE));

    /* FIXME: this flag should be set by the caller perhaps? */
    Mdl->MdlFlags |= MDL_IO_PAGE_READ;
}


BOOLEAN
NTAPI
MmIsFileObjectAPagingFile(PFILE_OBJECT FileObject)
{
    ULONG i;

    /* Loop through all the paging files */
    for (i = 0; i < MmNumberOfPagingFiles; i++)
    {
        /* Check if this is one of them */
        if (PagingFileList[i]->FileObject == FileObject) return TRUE;
    }

    /* Nothing found */
    return FALSE;
}

VOID
NTAPI
MmShowOutOfSpaceMessagePagingFile(VOID)
{
    if (!MmSwapSpaceMessage)
    {
        DPRINT1("MM: Out of swap space.\n");
        MmSwapSpaceMessage = TRUE;
    }
}

NTSTATUS
NTAPI
MmWriteToSwapPage(SWAPENTRY SwapEntry, PFN_NUMBER Page)
{
    ULONG i;
    ULONG_PTR offset;
    LARGE_INTEGER file_offset;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    KEVENT Event;
    UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
    PMDL Mdl = (PMDL)MdlBase;

    DPRINT("MmWriteToSwapPage\n");

    if (SwapEntry == 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
        return(STATUS_UNSUCCESSFUL);
    }

    i = FILE_FROM_ENTRY(SwapEntry);
    offset = OFFSET_FROM_ENTRY(SwapEntry) - 1;

    if (PagingFileList[i]->FileObject == NULL ||
            PagingFileList[i]->FileObject->DeviceObject == NULL)
    {
        DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, &Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    file_offset.QuadPart = offset * PAGE_SIZE;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoSynchronousPageWrite(PagingFileList[i]->FileObject,
                                    Mdl,
                                    &file_offset,
                                    &Event,
                                    &Iosb);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Iosb.Status;
    }

    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    }
    return(Status);
}


NTSTATUS
NTAPI
MmReadFromSwapPage(SWAPENTRY SwapEntry, PFN_NUMBER Page)
{
    return MiReadPageFile(Page, FILE_FROM_ENTRY(SwapEntry), OFFSET_FROM_ENTRY(SwapEntry) - 1);
}

NTSTATUS
NTAPI
MiReadPageFile(
    _In_ PFN_NUMBER Page,
    _In_ ULONG PageFileIndex,
    _In_ ULONG_PTR PageFileOffset)
{
    LARGE_INTEGER file_offset;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    KEVENT Event;
    UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
    PMDL Mdl = (PMDL)MdlBase;
    PPAGINGFILE PagingFile;

    DPRINT("MiReadSwapFile\n");

    if (PageFileOffset == 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
        return(STATUS_UNSUCCESSFUL);
    }

    ASSERT(PageFileIndex < MAX_PAGING_FILES);

    PagingFile = PagingFileList[PageFileIndex];

    if (PagingFile->FileObject == NULL || PagingFile->FileObject->DeviceObject == NULL)
    {
        DPRINT1("Bad paging file %u\n", PageFileIndex);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, &Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    file_offset.QuadPart = PageFileOffset * PAGE_SIZE;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoPageRead(PagingFile->FileObject,
                        Mdl,
                        &file_offset,
                        &Event,
                        &Iosb);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Iosb.Status;
    }
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
    }
    return(Status);
}

VOID
INIT_FUNCTION
NTAPI
MmInitPagingFile(VOID)
{
    ULONG i;

    KeInitializeSpinLock(&PagingFileListLock);

    MiFreeSwapPages = 0;
    MiUsedSwapPages = 0;
    MiReservedSwapPages = 0;

    for (i = 0; i < MAX_PAGING_FILES; i++)
    {
        PagingFileList[i] = NULL;
    }
    MmNumberOfPagingFiles = 0;
}

static ULONG
MiAllocPageFromPagingFile(PPAGINGFILE PagingFile)
{
    KIRQL oldIrql;
    ULONG i, j;

    KeAcquireSpinLock(&PagingFile->AllocMapLock, &oldIrql);

    for (i = 0; i < PagingFile->AllocMapSize; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if (!(PagingFile->AllocMap[i] & (1 << j)))
            {
                PagingFile->AllocMap[i] |= (1 << j);
                PagingFile->UsedPages++;
                PagingFile->FreePages--;
                KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
                return((i * 32) + j);
            }
        }
    }

    KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
    return(0xFFFFFFFF);
}

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry)
{
    ULONG i;
    ULONG_PTR off;
    KIRQL oldIrql;

    i = FILE_FROM_ENTRY(Entry);
    off = OFFSET_FROM_ENTRY(Entry) - 1;

    KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
    if (PagingFileList[i] == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }
    KeAcquireSpinLockAtDpcLevel(&PagingFileList[i]->AllocMapLock);

    PagingFileList[i]->AllocMap[off >> 5] &= (~(1 << (off % 32)));

    PagingFileList[i]->FreePages++;
    PagingFileList[i]->UsedPages--;

    MiFreeSwapPages++;
    MiUsedSwapPages--;

    KeReleaseSpinLockFromDpcLevel(&PagingFileList[i]->AllocMapLock);
    KeReleaseSpinLock(&PagingFileListLock, oldIrql);
}

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID)
{
    KIRQL oldIrql;
    ULONG i;
    ULONG off;
    SWAPENTRY entry;

    KeAcquireSpinLock(&PagingFileListLock, &oldIrql);

    if (MiFreeSwapPages == 0)
    {
        KeReleaseSpinLock(&PagingFileListLock, oldIrql);
        return(0);
    }

    for (i = 0; i < MAX_PAGING_FILES; i++)
    {
        if (PagingFileList[i] != NULL &&
                PagingFileList[i]->FreePages >= 1)
        {
            off = MiAllocPageFromPagingFile(PagingFileList[i]);
            if (off == 0xFFFFFFFF)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
                KeReleaseSpinLock(&PagingFileListLock, oldIrql);
                return(STATUS_UNSUCCESSFUL);
            }
            MiUsedSwapPages++;
            MiFreeSwapPages--;
            KeReleaseSpinLock(&PagingFileListLock, oldIrql);

            entry = ENTRY_FROM_FILE_OFFSET(i, off + 1);
            return(entry);
        }
    }

    KeReleaseSpinLock(&PagingFileListLock, oldIrql);
    KeBugCheck(MEMORY_MANAGEMENT);
    return(0);
}

NTSTATUS NTAPI
NtCreatePagingFile(IN PUNICODE_STRING FileName,
                   IN PLARGE_INTEGER InitialSize,
                   IN PLARGE_INTEGER MaximumSize,
                   IN ULONG Reserved)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatus;
    PFILE_OBJECT FileObject;
    PPAGINGFILE PagingFile;
    KIRQL oldIrql;
    ULONG AllocMapSize;
    ULONG i;
    ULONG Count;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING CapturedFileName;
    LARGE_INTEGER SafeInitialSize, SafeMaximumSize, AllocationSize;
    FILE_FS_DEVICE_INFORMATION FsDeviceInfo;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;

    DPRINT("NtCreatePagingFile(FileName %wZ, InitialSize %I64d)\n",
           FileName, InitialSize->QuadPart);

    if (MmNumberOfPagingFiles >= MAX_PAGING_FILES)
    {
        return STATUS_TOO_MANY_PAGING_FILES;
    }

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        if (SeSinglePrivilegeCheck(SeCreatePagefilePrivilege, PreviousMode) != TRUE)
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        _SEH2_TRY
        {
            SafeInitialSize = ProbeForReadLargeInteger(InitialSize);
            SafeMaximumSize = ProbeForReadLargeInteger(MaximumSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        SafeInitialSize = *InitialSize;
        SafeMaximumSize = *MaximumSize;
    }

    /* Pagefiles can't be larger than 4GB and ofcourse the minimum should be
       smaller than the maximum */
    if (0 != SafeInitialSize.u.HighPart)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
    if (0 != SafeMaximumSize.u.HighPart)
    {
        return STATUS_INVALID_PARAMETER_3;
    }
    if (SafeMaximumSize.u.LowPart < SafeInitialSize.u.LowPart)
    {
        return STATUS_INVALID_PARAMETER_MIX;
    }

    Status = ProbeAndCaptureUnicodeString(&CapturedFileName,
                                          PreviousMode,
                                          FileName);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    /* Create the security descriptor for the page file */
    Status = RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return Status;
    }

    /* Create the DACL: we will only allow two SIDs */
    Count = sizeof(ACL) + (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
                          (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));
    Dacl = ExAllocatePoolWithTag(PagedPool, Count, 'lcaD');
    if (Dacl == NULL)
    {
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the DACL */
    Status = RtlCreateAcl(Dacl, Count, ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Dacl, 'lcaD');
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return Status;
    }

    /* Grant full access to admins */
    Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeAliasAdminsSid);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Dacl, 'lcaD');
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return Status;
    }

    /* Grant full access to SYSTEM */
    Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeLocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Dacl, 'lcaD');
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return Status;
    }

    /* Attach the DACL to the security descriptor */
    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Dacl, 'lcaD');
        ReleaseCapturedUnicodeString(&CapturedFileName,
                                     PreviousMode);
        return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &CapturedFileName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               &SecurityDescriptor);

    /* Make sure we can at least store a complete page:
     * If we have 2048 BytesPerAllocationUnit (FAT16 < 128MB) there is
     * a problem if the paging file is fragmented. Suppose the first cluster
     * of the paging file is cluster 3042 but cluster 3043 is NOT part of the
     * paging file but of another file. We can't write a complete page (4096
     * bytes) to the physical location of cluster 3042 then. */
    AllocationSize.QuadPart = SafeInitialSize.QuadPart + PAGE_SIZE;

    /* First, attempt to replace the page file, if existing */
    Status = IoCreateFile(&FileHandle,
                          SYNCHRONIZE | WRITE_DAC | FILE_READ_DATA | FILE_WRITE_DATA,
                          &ObjectAttributes,
                          &IoStatus,
                          &AllocationSize,
                          FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                          FILE_SHARE_WRITE,
                          FILE_SUPERSEDE,
                          FILE_DELETE_ON_CLOSE | FILE_NO_COMPRESSION | FILE_NO_INTERMEDIATE_BUFFERING,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          SL_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING);
    /* If we failed, relax a bit constraints, someone may be already holding the
     * the file, so share write, don't attempt to replace and don't delete on close
     * (basically, don't do anything conflicting)
     */
    if (!NT_SUCCESS(Status))
    {
        Status = IoCreateFile(&FileHandle,
                              SYNCHRONIZE | FILE_WRITE_DATA,
                              &ObjectAttributes,
                              &IoStatus,
                              &AllocationSize,
                              FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              FILE_OPEN,
                              FILE_NO_COMPRESSION | FILE_NO_INTERMEDIATE_BUFFERING,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              SL_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING);
    }

    ReleaseCapturedUnicodeString(&CapturedFileName,
                                 PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed creating page file: %lx\n", Status);
        ExFreePoolWithTag(Dacl, 'lcaD');
        return(Status);
    }

    /* Set the security descriptor */
    if (NT_SUCCESS(IoStatus.Status))
    {
        Status = ZwSetSecurityObject(FileHandle, DACL_SECURITY_INFORMATION, &SecurityDescriptor);
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Dacl, 'lcaD');
            ZwClose(FileHandle);
            return Status;
        }
    }

    /* DACL is no longer needed, free it */
    ExFreePoolWithTag(Dacl, 'lcaD');

    /* Set its end of file to initial size */
    Status = ZwSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &SafeInitialSize,
                                  sizeof(LARGE_INTEGER),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatus.Status))
    {
        ZwClose(FileHandle);
        return(Status);
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_ALL_ACCESS,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(FileHandle);
        return(Status);
    }

    /* Deny page file creation on a floppy disk */
    FsDeviceInfo.Characteristics = 0;
    IoQueryVolumeInformation(FileObject, FileFsDeviceInformation, sizeof(FsDeviceInfo), &FsDeviceInfo, &Count);
    if (BooleanFlagOn(FsDeviceInfo.Characteristics, FILE_FLOPPY_DISKETTE))
    {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        return STATUS_FLOPPY_VOLUME;
    }

    PagingFile = ExAllocatePool(NonPagedPool, sizeof(*PagingFile));
    if (PagingFile == NULL)
    {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        return(STATUS_NO_MEMORY);
    }

    RtlZeroMemory(PagingFile, sizeof(*PagingFile));

    PagingFile->FileHandle = FileHandle;
    PagingFile->FileObject = FileObject;
    PagingFile->MaximumSize.QuadPart = SafeMaximumSize.QuadPart;
    PagingFile->CurrentSize.QuadPart = SafeInitialSize.QuadPart;
    PagingFile->FreePages = (ULONG)(SafeInitialSize.QuadPart / PAGE_SIZE);
    PagingFile->UsedPages = 0;
    KeInitializeSpinLock(&PagingFile->AllocMapLock);

    AllocMapSize = (PagingFile->FreePages / 32) + 1;
    PagingFile->AllocMap = ExAllocatePool(NonPagedPool,
                                          AllocMapSize * sizeof(ULONG));
    PagingFile->AllocMapSize = AllocMapSize;

    if (PagingFile->AllocMap == NULL)
    {
        ExFreePool(PagingFile);
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        return(STATUS_NO_MEMORY);
    }

    RtlZeroMemory(PagingFile->AllocMap, AllocMapSize * sizeof(ULONG));

    KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
    for (i = 0; i < MAX_PAGING_FILES; i++)
    {
        if (PagingFileList[i] == NULL)
        {
            PagingFileList[i] = PagingFile;
            break;
        }
    }
    MiFreeSwapPages = MiFreeSwapPages + PagingFile->FreePages;
    MmNumberOfPagingFiles++;
    KeReleaseSpinLock(&PagingFileListLock, oldIrql);

    MmSwapSpaceMessage = FALSE;

    return(STATUS_SUCCESS);
}

/* EOF */
