/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Paging file functions
 * COPYRIGHT:   Copyright 1998-2003 David Welch <welch@mcmail.com>
 *              Copyright 2010-2018 Pierre Schweitzer <pierre@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Minimum pagefile size is 256 pages (1 MB) */
#define MINIMUM_PAGEFILE_SIZE       (256ULL * PAGE_SIZE)

/* Maximum pagefile sizes for different architectures */
#if defined(_M_IX86) && !defined(_X86PAE_)
/* Around 4 GB */
    #define MAXIMUM_PAGEFILE_SIZE   ((1ULL * 1024 * 1024 - 1) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(4ULL * GIGABYTE - 1)
/* PAE uses the same size as x64 */
#elif (defined(_M_IX86) && defined(_X86PAE_)) || defined (_M_AMD64) || defined(_M_ARM64)
/* Around 16 TB */
    #if (NTDDI_VERSION >= NTDDI_WIN10)
    #define MAXIMUM_PAGEFILE_SIZE   ((4ULL * 1024 * 1024 * 1024 - 2) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(16ULL * TERABYTE - PAGE_SIZE - 1)
    #else
    #define MAXIMUM_PAGEFILE_SIZE   ((4ULL * 1024 * 1024 * 1024 - 1) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(16ULL * TERABYTE - 1)
    #endif
#elif defined (_M_IA64)
/* Around 32 TB */
    #define MAXIMUM_PAGEFILE_SIZE   ((8ULL * 1024 * 1024 * 1024 - 1) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(32ULL * TERABYTE - 1)
#elif defined(_M_ARM)
/* Around 2 GB */
    #if (NTDDI_VERSION >= NTDDI_WIN10)
    #define MAXIMUM_PAGEFILE_SIZE   ((512ULL * 1024 - 2) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(2ULL * GIGABYTE - PAGE_SIZE - 1)
    #elif (NTDDI_VERSION >= NTDDI_WINBLUE) // NTDDI_WIN81
    #define MAXIMUM_PAGEFILE_SIZE   ((512ULL * 1024 - 1) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(2ULL * GIGABYTE - 1)
    #else
/* Around 4 GB */
    #define MAXIMUM_PAGEFILE_SIZE   ((1ULL * 1024 * 1024 - 1) * PAGE_SIZE)
                                 // PAGE_ROUND_DOWN(4ULL * GIGABYTE - 1)
    #endif
#else
#error Unknown architecture
#endif

/* List of paging files, both used and free */
PMMPAGING_FILE MmPagingFile[MAX_PAGING_FILES];

/* Lock for examining the list of paging files */
KGUARDED_MUTEX MmPageFileCreationLock;

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

static BOOLEAN MmSystemPageFileLocated = FALSE;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MmBuildMdlFromPages(PMDL Mdl, PPFN_NUMBER Pages)
{
    memcpy(Mdl + 1, Pages, sizeof(PFN_NUMBER) * (PAGE_ROUND_UP(Mdl->ByteOffset+Mdl->ByteCount)/PAGE_SIZE));
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
        if (MmPagingFile[i]->FileObject == FileObject) return TRUE;
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

    if (MmPagingFile[i]->FileObject == NULL ||
            MmPagingFile[i]->FileObject->DeviceObject == NULL)
    {
        DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, &Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    file_offset.QuadPart = offset * PAGE_SIZE;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoSynchronousPageWrite(MmPagingFile[i]->FileObject,
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
    return MiReadPageFile(Page, FILE_FROM_ENTRY(SwapEntry), OFFSET_FROM_ENTRY(SwapEntry));
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
    PMMPAGING_FILE PagingFile;

    DPRINT("MiReadSwapFile\n");

    if (PageFileOffset == 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
        return(STATUS_UNSUCCESSFUL);
    }

    /* Normalize offset. */
    PageFileOffset--;

    ASSERT(PageFileIndex < MAX_PAGING_FILES);

    PagingFile = MmPagingFile[PageFileIndex];

    if (PagingFile->FileObject == NULL || PagingFile->FileObject->DeviceObject == NULL)
    {
        DPRINT1("Bad paging file %u\n", PageFileIndex);
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    MmBuildMdlFromPages(Mdl, &Page);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED | MDL_IO_PAGE_READ;

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

CODE_SEG("INIT")
VOID
NTAPI
MmInitPagingFile(VOID)
{
    ULONG i;

    KeInitializeGuardedMutex(&MmPageFileCreationLock);

    MiFreeSwapPages = 0;
    MiUsedSwapPages = 0;
    MiReservedSwapPages = 0;

    for (i = 0; i < MAX_PAGING_FILES; i++)
    {
        MmPagingFile[i] = NULL;
    }
    MmNumberOfPagingFiles = 0;
}

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry)
{
    ULONG i;
    ULONG_PTR off;
    PMMPAGING_FILE PagingFile;

    i = FILE_FROM_ENTRY(Entry);
    off = OFFSET_FROM_ENTRY(Entry) - 1;

    KeAcquireGuardedMutex(&MmPageFileCreationLock);

    PagingFile = MmPagingFile[i];
    if (PagingFile == NULL)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    RtlClearBit(PagingFile->Bitmap, off >> 5);

    PagingFile->FreeSpace++;
    PagingFile->CurrentUsage--;

    MiFreeSwapPages++;
    MiUsedSwapPages--;
    UpdateTotalCommittedPages(-1);

    KeReleaseGuardedMutex(&MmPageFileCreationLock);
}

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID)
{
    ULONG i;
    ULONG off;
    SWAPENTRY entry;

    KeAcquireGuardedMutex(&MmPageFileCreationLock);

    if (MiFreeSwapPages == 0)
    {
        KeReleaseGuardedMutex(&MmPageFileCreationLock);
        return(0);
    }

    for (i = 0; i < MAX_PAGING_FILES; i++)
    {
        if (MmPagingFile[i] != NULL &&
                MmPagingFile[i]->FreeSpace >= 1)
        {
            off = RtlFindClearBitsAndSet(MmPagingFile[i]->Bitmap, 1, 0);
            if (off == 0xFFFFFFFF)
            {
                KeBugCheck(MEMORY_MANAGEMENT);
                KeReleaseGuardedMutex(&MmPageFileCreationLock);
                return(STATUS_UNSUCCESSFUL);
            }
            MiUsedSwapPages++;
            MiFreeSwapPages--;
            UpdateTotalCommittedPages(1);

            KeReleaseGuardedMutex(&MmPageFileCreationLock);

            entry = ENTRY_FROM_FILE_OFFSET(i, off + 1);
            return(entry);
        }
    }

    KeReleaseGuardedMutex(&MmPageFileCreationLock);
    KeBugCheck(MEMORY_MANAGEMENT);
    return(0);
}

NTSTATUS
NTAPI
NtCreatePagingFile(
    _In_ PUNICODE_STRING FileName,
    _In_ PLARGE_INTEGER MinimumSize,
    _In_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG Reserved)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatus;
    PFILE_OBJECT FileObject;
    PMMPAGING_FILE PagingFile;
    SIZE_T AllocMapSize;
    ULONG Count;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING PageFileName;
    LARGE_INTEGER SafeMinimumSize, SafeMaximumSize, AllocationSize;
    FILE_FS_DEVICE_INFORMATION FsDeviceInfo;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;
    PWSTR Buffer;
    DEVICE_TYPE DeviceType;

    PAGED_CODE();

    DPRINT("NtCreatePagingFile(FileName: '%wZ', MinimumSize: %I64d, MaximumSize: %I64d)\n",
           FileName, MinimumSize->QuadPart, MaximumSize->QuadPart);

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
            SafeMinimumSize = ProbeForReadLargeInteger(MinimumSize);
            SafeMaximumSize = ProbeForReadLargeInteger(MaximumSize);
            PageFileName = ProbeForReadUnicodeString(FileName);
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
        SafeMinimumSize = *MinimumSize;
        SafeMaximumSize = *MaximumSize;
        PageFileName = *FileName;
    }

    /*
     * Pagefiles cannot be larger than the platform-specific memory addressable
     * limits, and of course the minimum should be smaller than the maximum.
     */
    if (SafeMinimumSize.QuadPart < MINIMUM_PAGEFILE_SIZE ||
        SafeMinimumSize.QuadPart > MAXIMUM_PAGEFILE_SIZE)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
    if (SafeMaximumSize.QuadPart < SafeMinimumSize.QuadPart ||
        SafeMaximumSize.QuadPart > MAXIMUM_PAGEFILE_SIZE)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Validate the name length */
    if ((PageFileName.Length == 0) ||
        (PageFileName.Length > MAXIMUM_FILENAME_LENGTH))
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    /* Allocate a buffer to keep the name copy. Note that it is kept only
     * for information purposes, so it gets allocated in the paged pool,
     * even if it will be stored in the PagingFile structure, that is
     * allocated from non-paged pool (see below). */
    PageFileName.MaximumLength = PageFileName.Length;
    Buffer = ExAllocatePoolWithTag(PagedPool, PageFileName.Length, TAG_MM);
    if (Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the name */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(PageFileName.Buffer, PageFileName.Length, sizeof(WCHAR));
            RtlCopyMemory(Buffer, PageFileName.Buffer, PageFileName.Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ExFreePoolWithTag(Buffer, TAG_MM);

            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        RtlCopyMemory(Buffer, PageFileName.Buffer, PageFileName.Length);
    }

    /* Replace caller's buffer with ours */
    PageFileName.Buffer = Buffer;

    /* Create the security descriptor for the page file */
    Status = RtlCreateSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Buffer, TAG_MM);
        return Status;
    }

    /* Create the DACL: we will only allow two SIDs */
    Count = sizeof(ACL) + (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
                          (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));
    Dacl = ExAllocatePoolWithTag(PagedPool, Count, TAG_DACL);
    if (Dacl == NULL)
    {
        ExFreePoolWithTag(Buffer, TAG_MM);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize the DACL */
    Status = RtlCreateAcl(Dacl, Count, ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto EarlyQuit;

    /* Grant full access to admins */
    Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeAliasAdminsSid);
    if (!NT_SUCCESS(Status))
        goto EarlyQuit;

    /* Grant full access to SYSTEM */
    Status = RtlAddAccessAllowedAce(Dacl, ACL_REVISION, FILE_ALL_ACCESS, SeLocalSystemSid);
    if (!NT_SUCCESS(Status))
        goto EarlyQuit;

    /* Attach the DACL to the security descriptor */
    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl, FALSE);
    if (!NT_SUCCESS(Status))
        goto EarlyQuit;

    InitializeObjectAttributes(&ObjectAttributes,
                               &PageFileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               &SecurityDescriptor);

    /* Make sure we can at least store a complete page:
     * If we have 2048 BytesPerAllocationUnit (FAT16 < 128MB) there is
     * a problem if the paging file is fragmented. Suppose the first cluster
     * of the paging file is cluster 3042 but cluster 3043 is NOT part of the
     * paging file but of another file. We can't write a complete page (4096
     * bytes) to the physical location of cluster 3042 then. */
    AllocationSize.QuadPart = SafeMinimumSize.QuadPart + PAGE_SIZE;

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
                          IO_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING);
    /* If we failed, relax a bit constraints, someone may be already holding the
     * the file, so share write, don't attempt to replace and don't delete on close
     * (basically, don't do anything conflicting).
     * This can happen if the caller attempts to extend a page file.
     */
    if (!NT_SUCCESS(Status))
    {
        ULONG i;

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
                              IO_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING);
        if (!NT_SUCCESS(Status))
            goto EarlyQuit;

        /* We opened it! Check we are that "someone" ;-)
         * First, get the opened file object.
         */
        Status = ObReferenceObjectByHandle(FileHandle,
                                           FILE_READ_DATA | FILE_WRITE_DATA,
                                           IoFileObjectType,
                                           KernelMode,
                                           (PVOID*)&FileObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ZwClose(FileHandle);
            goto EarlyQuit;
        }

        /* Find if it matches a previous page file */
        PagingFile = NULL;

        KeAcquireGuardedMutex(&MmPageFileCreationLock);

        for (i = 0; i < MmNumberOfPagingFiles; ++i)
        {
            if (MmPagingFile[i]->FileObject->SectionObjectPointer == FileObject->SectionObjectPointer)
            {
                /* Same object pointer: this is the matching page file */
                PagingFile = MmPagingFile[i];
                break;
            }
        }

        /* If we didn't find the page file, fail */
        if (PagingFile == NULL)
        {
            KeReleaseGuardedMutex(&MmPageFileCreationLock);
            ObDereferenceObject(FileObject);
            ZwClose(FileHandle);
            Status = STATUS_NOT_FOUND;
            goto EarlyQuit;
        }

        /* Don't allow page file shrinking */
        if (PagingFile->MinimumSize > (SafeMinimumSize.QuadPart >> PAGE_SHIFT))
        {
            KeReleaseGuardedMutex(&MmPageFileCreationLock);
            ObDereferenceObject(FileObject);
            ZwClose(FileHandle);
            Status = STATUS_INVALID_PARAMETER_2;
            goto EarlyQuit;
        }

        if ((SafeMaximumSize.QuadPart >> PAGE_SHIFT) < PagingFile->MaximumSize)
        {
            KeReleaseGuardedMutex(&MmPageFileCreationLock);
            ObDereferenceObject(FileObject);
            ZwClose(FileHandle);
            Status = STATUS_INVALID_PARAMETER_3;
            goto EarlyQuit;
        }

        /* FIXME: implement parameters checking and page file extension */
        UNIMPLEMENTED;

        KeReleaseGuardedMutex(&MmPageFileCreationLock);
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        Status = STATUS_NOT_IMPLEMENTED;
        goto EarlyQuit;
    }

    if (!NT_SUCCESS(Status))
    {
EarlyQuit:
        DPRINT1("Failed creating page file: %lx\n", Status);
        ExFreePoolWithTag(Dacl, TAG_DACL);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return Status;
    }

    /* Set the security descriptor */
    if (NT_SUCCESS(IoStatus.Status))
    {
        Status = ZwSetSecurityObject(FileHandle, DACL_SECURITY_INFORMATION, &SecurityDescriptor);
        if (!NT_SUCCESS(Status))
        {
            ZwClose(FileHandle);
            ExFreePoolWithTag(Dacl, TAG_DACL);
            ExFreePoolWithTag(Buffer, TAG_MM);
            return Status;
        }
    }

    /* DACL is no longer needed, free it */
    ExFreePoolWithTag(Dacl, TAG_DACL);

    /* FIXME: To enable once page file management is moved to ARM3 */
#if 0
    /* Check we won't overflow commit limit with the page file */
    if (MmTotalCommitLimitMaximum + (SafeMaximumSize.QuadPart >> PAGE_SHIFT) <= MmTotalCommitLimitMaximum)
    {
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return STATUS_INVALID_PARAMETER_3;
    }
#endif

    /* Set its end of file to minimal size */
    Status = ZwSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &SafeMinimumSize,
                                  sizeof(LARGE_INTEGER),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(IoStatus.Status))
    {
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return Status;
    }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_READ_DATA | FILE_WRITE_DATA,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return Status;
    }

    /* Only allow page file on a few device types */
    DeviceType = IoGetRelatedDeviceObject(FileObject)->DeviceType;
    if (DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM &&
        DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM &&
        DeviceType != FILE_DEVICE_DFS_VOLUME &&
        DeviceType != FILE_DEVICE_DFS_FILE_SYSTEM)
    {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return Status;
    }

    /* Deny page file creation on a floppy disk */
    FsDeviceInfo.Characteristics = 0;
    IoQueryVolumeInformation(FileObject, FileFsDeviceInformation,
                             sizeof(FsDeviceInfo), &FsDeviceInfo, &Count);
    if (BooleanFlagOn(FsDeviceInfo.Characteristics, FILE_FLOPPY_DISKETTE))
    {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return STATUS_FLOPPY_VOLUME;
    }

    /*
     * Missing validation steps TODO:
     * (see https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/mm/modwrite/create.htm )
     *
     * - Verify that no file system driver or any filter driver has done file
     *   I/O while opening the file.
     *   Verify that nothing of the paging file is yet in memory. Specifically,
     *   the file object must either have no SectionObjectPointer or the latter
     *   must have neither a DataSectionObject nor an ImageSectionObject.
     *   Otherwise, we should fail, returning STATUS_INCOMPATIBLE_FILE_MAP.
     *
     * - Inform all the applicable drivers to prepare for the possibility of
     *   paging I/O. Much of the point to paging I/O is to resolve page faults.
     *   Especially important is that drivers that handle paging I/O do not
     *   cause more page faults. All the code and data that each driver might
     *   ever use for access to the paging file must be locked into physical
     *   memory. This can’t be left until paging I/O actually occurs.
     *   It must be done in advance.
     */

    PagingFile = ExAllocatePoolZero(NonPagedPool, sizeof(*PagingFile), TAG_MM);
    if (PagingFile == NULL)
    {
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PagingFile->FileHandle = FileHandle;
    PagingFile->FileObject = FileObject;
    PagingFile->Size = (SafeMinimumSize.QuadPart >> PAGE_SHIFT);
    PagingFile->MinimumSize = PagingFile->Size;
    PagingFile->MaximumSize = (SafeMaximumSize.QuadPart >> PAGE_SHIFT);
    /* First page is never used: it's the header
     * TODO: write it
     */
    PagingFile->FreeSpace = PagingFile->Size - 1;
    PagingFile->CurrentUsage = 0;
    PagingFile->PageFileName = PageFileName;
    ASSERT(PagingFile->Size == PagingFile->FreeSpace + PagingFile->CurrentUsage + 1);

    AllocMapSize = sizeof(RTL_BITMAP) + (((PagingFile->MaximumSize + 31) / 32) * sizeof(ULONG));
    PagingFile->Bitmap = ExAllocatePoolWithTag(NonPagedPool,
                                               AllocMapSize,
                                               TAG_MM);
    if (PagingFile->Bitmap == NULL)
    {
        ExFreePoolWithTag(PagingFile, TAG_MM);
        ObDereferenceObject(FileObject);
        ZwClose(FileHandle);
        ExFreePoolWithTag(Buffer, TAG_MM);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(PagingFile->Bitmap,
                        (PULONG)(PagingFile->Bitmap + 1),
                        (ULONG)(PagingFile->MaximumSize));
    RtlClearAllBits(PagingFile->Bitmap);

    /* Insert the new paging file information into the list */
    KeAcquireGuardedMutex(&MmPageFileCreationLock);
    /* Ensure the corresponding slot is empty yet */
    ASSERT(MmPagingFile[MmNumberOfPagingFiles] == NULL);
    MmPagingFile[MmNumberOfPagingFiles] = PagingFile;
    MmNumberOfPagingFiles++;
    MiFreeSwapPages = MiFreeSwapPages + PagingFile->FreeSpace;
    KeReleaseGuardedMutex(&MmPageFileCreationLock);

    MmSwapSpaceMessage = FALSE;

    if (!MmSystemPageFileLocated && BooleanFlagOn(FileObject->DeviceObject->Flags, DO_SYSTEM_BOOT_PARTITION))
    {
        MmSystemPageFileLocated = IoInitializeCrashDump(FileHandle);
    }

    return STATUS_SUCCESS;
}

/* EOF */
