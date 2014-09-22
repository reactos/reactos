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
   ULONG_PTR MaximumSize;
   ULONG_PTR CurrentSize;
   PFN_NUMBER FreePages;
   PFN_NUMBER UsedPages;
   PVOID AllocMap;
   RTL_BITMAP AllocBitMap;
   ULONG_PTR AllocBitMapHint;
   KSPIN_LOCK AllocMapLock;
   PRETRIEVAL_POINTERS_BUFFER RetrievalPointers;
}
PAGINGFILE, *PPAGINGFILE;

typedef struct _RETRIEVEL_DESCRIPTOR_LIST
{
   struct _RETRIEVEL_DESCRIPTOR_LIST* Next;
   RETRIEVAL_POINTERS_BUFFER RetrievalPointers;
}
RETRIEVEL_DESCRIPTOR_LIST, *PRETRIEVEL_DESCRIPTOR_LIST;

/* GLOBALS *******************************************************************/

#define PAIRS_PER_RUN (1024)

#define MAX_PAGING_FILES  (16)

/* List of paging files, both used and free */
static PPAGINGFILE PagingFileList[MAX_PAGING_FILES];

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

static LARGE_INTEGER
MmGetOffsetPageFile(PRETRIEVAL_POINTERS_BUFFER RetrievalPointers, LARGE_INTEGER Offset)
{
   /* Simple binary search */
   ULONG first, last, mid;
   first = 0;
   last = RetrievalPointers->ExtentCount - 1;
   while (first <= last)
   {
      mid = (last - first) / 2 + first;
      if (Offset.QuadPart < RetrievalPointers->Extents[mid].NextVcn.QuadPart)
      {
         if (mid == 0)
         {
            Offset.QuadPart += RetrievalPointers->Extents[0].Lcn.QuadPart - RetrievalPointers->StartingVcn.QuadPart;
            return Offset;
         }
         else
         {
            if (Offset.QuadPart >= RetrievalPointers->Extents[mid-1].NextVcn.QuadPart)
            {
               Offset.QuadPart += RetrievalPointers->Extents[mid].Lcn.QuadPart - RetrievalPointers->Extents[mid-1].NextVcn.QuadPart;
               return Offset;
            }
            last = mid - 1;
         }
      }
      else
      {
         if (mid == RetrievalPointers->ExtentCount - 1)
         {
            break;
         }
         if (Offset.QuadPart < RetrievalPointers->Extents[mid+1].NextVcn.QuadPart)
         {
            Offset.QuadPart += RetrievalPointers->Extents[mid+1].Lcn.QuadPart  - RetrievalPointers->Extents[mid].NextVcn.QuadPart;
            return Offset;
         }
         first = mid + 1;
      }
   }
   KeBugCheck(MEMORY_MANAGEMENT);
#if defined(__GNUC__)

   return (LARGE_INTEGER)0LL;
#else

   {
      const LARGE_INTEGER dummy =
         {
            0
         };
      return dummy;
   }
#endif
}

NTSTATUS
NTAPI
MmWriteToSwapPage(SWAPENTRY SwapEntry, PFN_NUMBER Page)
{
    MMPTE TempPte;

    TempPte.u.Long = 0;
    TempPte.u.Soft.PageFileHigh = OFFSET_FROM_ENTRY(SwapEntry);
    TempPte.u.Soft.PageFileLow = FILE_FROM_ENTRY(SwapEntry);
    return MiWritePageFile(Page, &TempPte);
}


NTSTATUS
NTAPI
MmReadFromSwapPage(SWAPENTRY SwapEntry, PFN_NUMBER Page)
{
    MMPTE TempPte;

    TempPte.u.Long = 0;
    TempPte.u.Soft.PageFileHigh = OFFSET_FROM_ENTRY(SwapEntry);
    TempPte.u.Soft.PageFileLow = FILE_FROM_ENTRY(SwapEntry);
    return MiReadPageFile(Page, &TempPte);
}

NTSTATUS
NTAPI
MiReadPageFile(
	_In_ PFN_NUMBER Page,
	_In_ const MMPTE* PointerPte
)
{
   LARGE_INTEGER FileOffset;
   IO_STATUS_BLOCK Iosb;
   ULONG PageFileIndex = PointerPte->u.Soft.PageFileLow;
   ULONG_PTR PageFileOffset = PointerPte->u.Soft.PageFileHigh;
   NTSTATUS Status;
   KEVENT Event;
   UCHAR MdlBase[sizeof(MDL) + sizeof(PFN_NUMBER)];
   PMDL Mdl = (PMDL)MdlBase;
   PPAGINGFILE PagingFile;

   DPRINT("MiReadPageFile\n");

   if (PageFileOffset == 0)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
      return(STATUS_UNSUCCESSFUL);
   }

   /* Normalize it */
   PageFileOffset--;

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

   FileOffset.QuadPart = PageFileOffset * PAGE_SIZE;
   FileOffset = MmGetOffsetPageFile(PagingFile->RetrievalPointers, FileOffset);

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoPageRead(PagingFile->FileObject,
                       Mdl,
                       &FileOffset,
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
MiWritePageFile(
    _In_ PFN_NUMBER Page,
    _In_ const MMPTE* PointerPte
)
{
    ULONG PageFileIndex = PointerPte->u.Soft.PageFileLow;
    ULONG_PTR PageFileOffset = PointerPte->u.Soft.PageFileHigh;
    LARGE_INTEGER FileOffset;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    KEVENT Event;
    UCHAR MdlBase[sizeof(MDL) + sizeof(PFN_NUMBER)];
    PMDL Mdl = (PMDL) MdlBase;
    PPAGINGFILE PagingFile;

    /* The PTE must already be setup to point to a pagefile entry */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(PointerPte->u.Soft.Transition == 0);
    ASSERT(PointerPte->u.Soft.Prototype == 0);

    DPRINT("MiWriteSwapFile\n");

    if (PageFileOffset == 0)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
        return (STATUS_UNSUCCESSFUL);
    }

    /* Normalize it */
    PageFileOffset--;

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

    FileOffset.QuadPart = PageFileOffset * PAGE_SIZE;
    FileOffset = MmGetOffsetPageFile(PagingFile->RetrievalPointers, FileOffset);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Status = IoSynchronousPageWrite(PagingFile->FileObject,
                                    Mdl,
                                    &FileOffset,
                                    &Event,
                                    &Iosb);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Iosb.Status;
    }
    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
    }
    return Status;
}

VOID
NTAPI
MiFreePageFileEntry(
    _In_ PMMPTE PointerPte
)
{
    ULONG PageFileIndex;
    ULONG_PTR PageFileOffset;
    PPAGINGFILE PageFile;

    /* Some sanity checks */
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(PointerPte->u.Soft.Transition == 0);
    ASSERT(PointerPte->u.Soft.Prototype == 0);

    /* We must hold the PFN lock */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    PageFileIndex = PointerPte->u.Soft.PageFileLow;
    PageFileOffset = PointerPte->u.Soft.PageFileHigh;

    /* Fix up the value */
    ASSERT(PageFileOffset != 0);
    PageFileOffset--;

    DPRINT1("Releasing page file entry %u, %lu\n", PageFileIndex, PageFileOffset);

    ASSERT(PageFileIndex < MAX_PAGING_FILES);

    PageFile = PagingFileList[PageFileIndex];

    /* And also lock this pagefile */
    KeAcquireSpinLockAtDpcLevel(&PageFile->AllocMapLock);

    /* It must be already in use */
    ASSERT(RtlTestBit(&PageFile->AllocBitMap, PageFileOffset));
    RtlClearBit(&PageFile->AllocBitMap, PageFileOffset);

    /* One more */
    PageFile->FreePages++;
    PageFile->UsedPages--;

    MiFreeSwapPages++;
    MiUsedSwapPages--;

    /* Done */
    KeReleaseSpinLockFromDpcLevel(&PageFile->AllocMapLock);
}

NTSTATUS
NTAPI
MiReservePageFileEntry(
    _Out_ PMMPTE PointerPte
)
{
    ULONG PageFileIndex;
    ULONG_PTR PageFileOffset;
    PPAGINGFILE PageFile;

    /* We must hold the PFN lock */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    for (PageFileIndex = 0; PageFileIndex < MAX_PAGING_FILES; PageFileIndex++)
    {
        if (PagingFileList[PageFileIndex] == NULL)
            continue;
        if (PagingFileList[PageFileIndex]->FreePages != 0)
            break;
    }

    if (PageFileIndex == MAX_PAGING_FILES)
    {
        DPRINT1("Pagefiles are full!\n");
        return STATUS_PAGEFILE_QUOTA;
    }

    PointerPte->u.Long = 0;

    PageFile = PagingFileList[PageFileIndex];

    /* And also lock this page file */
    KeAcquireSpinLockAtDpcLevel(&PageFile->AllocMapLock);

    /* Find a free entry in this page file */
    PageFileOffset = RtlFindClearBitsAndSet(&PageFile->AllocBitMap, 1, PageFile->AllocBitMapHint);

    /* We must have found something as the file was marked as having free entries */
    ASSERT(PageFileOffset != ~((ULONG_PTR)0));
    PageFile->AllocBitMapHint = PageFileOffset & ~((PageFile->MaximumSize / PAGE_SIZE) - 1);

    DPRINT1("Reserving page file entry %lu, %Iu\n", PageFileIndex, PageFileOffset);

    /* Done */
    KeReleaseSpinLockFromDpcLevel(&PageFile->AllocMapLock);

    /* We rely on PageFileHigh being != 0 */
    PointerPte->u.Soft.PageFileHigh = PageFileOffset + 1;
    PointerPte->u.Soft.PageFileLow = PageFileIndex;

    return STATUS_SUCCESS;
}

VOID
INIT_FUNCTION
NTAPI
MmInitPagingFile(VOID)
{
   ULONG i;

   MiFreeSwapPages = 0;
   MiUsedSwapPages = 0;
   MiReservedSwapPages = 0;

   for (i = 0; i < MAX_PAGING_FILES; i++)
   {
      PagingFileList[i] = NULL;
   }
   MmNumberOfPagingFiles = 0;
}

VOID
NTAPI
MmFreeSwapPage(SWAPENTRY Entry)
{
    MMPTE TempPte;
    KIRQL OldIrql;

    TempPte.u.Long = 0;
    TempPte.u.Soft.PageFileLow = FILE_FROM_ENTRY(Entry);
    TempPte.u.Soft.PageFileHigh = OFFSET_FROM_ENTRY(Entry);

    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    MiFreePageFileEntry(&TempPte);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

SWAPENTRY
NTAPI
MmAllocSwapPage(VOID)
{
    MMPTE TempPte;
    SWAPENTRY entry;
    NTSTATUS Status;
    KIRQL OldIrql;

    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Status = MiReservePageFileEntry(&TempPte);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    if (!NT_SUCCESS(Status))
        return 0;

    entry = ENTRY_FROM_FILE_OFFSET(TempPte.u.Soft.PageFileLow, TempPte.u.Soft.PageFileHigh);

    return entry;
}

static PRETRIEVEL_DESCRIPTOR_LIST FASTCALL
MmAllocRetrievelDescriptorList(ULONG Pairs)
{
   ULONG Size;
   PRETRIEVEL_DESCRIPTOR_LIST RetDescList;

   Size = sizeof(RETRIEVEL_DESCRIPTOR_LIST) + Pairs * 2 * sizeof(LARGE_INTEGER);
   RetDescList = ExAllocatePool(NonPagedPool, Size);
   if (RetDescList)
   {
      RtlZeroMemory(RetDescList, Size);
   }

   return RetDescList;
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
   KIRQL OldIrql;
   ULONG AllocMapSize;
   FILE_FS_SIZE_INFORMATION FsSizeInformation;
   PRETRIEVEL_DESCRIPTOR_LIST RetDescList;
   PRETRIEVEL_DESCRIPTOR_LIST CurrentRetDescList;
   ULONG i;
   ULONG BytesPerAllocationUnit;
   LARGE_INTEGER Vcn;
   ULONG ExtentCount;
   LARGE_INTEGER MaxVcn;
   ULONG Count;
   ULONG Size;
   KPROCESSOR_MODE PreviousMode;
   UNICODE_STRING CapturedFileName;
   LARGE_INTEGER SafeInitialSize, SafeMaximumSize;

   DPRINT("NtCreatePagingFile(FileName %wZ, InitialSize %I64d)\n",
          FileName, InitialSize->QuadPart);

   if (MmNumberOfPagingFiles >= MAX_PAGING_FILES)
   {
      return(STATUS_TOO_MANY_PAGING_FILES);
   }

   PreviousMode = ExGetPreviousMode();

   if (PreviousMode != KernelMode)
   {
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

   InitializeObjectAttributes(&ObjectAttributes,
                              &CapturedFileName,
                              0,
                              NULL,
                              NULL);

   Status = IoCreateFile(&FileHandle,
                         FILE_ALL_ACCESS,
                         &ObjectAttributes,
                         &IoStatus,
                         NULL,
                         0,
                         0,
                         FILE_OPEN_IF,
                         FILE_SYNCHRONOUS_IO_NONALERT,
                         NULL,
                         0,
                         CreateFileTypeNone,
                         NULL,
                         SL_OPEN_PAGING_FILE | IO_NO_PARAMETER_CHECKING);

   ReleaseCapturedUnicodeString(&CapturedFileName,
                                PreviousMode);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Status = ZwQueryVolumeInformationFile(FileHandle,
                                         &IoStatus,
                                         &FsSizeInformation,
                                         sizeof(FILE_FS_SIZE_INFORMATION),
                                         FileFsSizeInformation);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(FileHandle);
      return Status;
   }

   BytesPerAllocationUnit = FsSizeInformation.SectorsPerAllocationUnit *
                            FsSizeInformation.BytesPerSector;
   /* FIXME: If we have 2048 BytesPerAllocationUnit (FAT16 < 128MB) there is
    * a problem if the paging file is fragmented. Suppose the first cluster
    * of the paging file is cluster 3042 but cluster 3043 is NOT part of the
    * paging file but of another file. We can't write a complete page (4096
    * bytes) to the physical location of cluster 3042 then. */
   if (BytesPerAllocationUnit % PAGE_SIZE)
   {
      DPRINT1("BytesPerAllocationUnit %lu is not a multiple of PAGE_SIZE %d\n",
              BytesPerAllocationUnit, PAGE_SIZE);
      ZwClose(FileHandle);
      return STATUS_UNSUCCESSFUL;
   }

   Status = ZwSetInformationFile(FileHandle,
                                 &IoStatus,
                                 &SafeInitialSize,
                                 sizeof(LARGE_INTEGER),
                                 FileAllocationInformation);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(FileHandle);
      return(Status);
   }

   Status = ObReferenceObjectByHandle(FileHandle,
                                      FILE_ALL_ACCESS,
                                      IoFileObjectType,
                                      PreviousMode,
                                      (PVOID*)&FileObject,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(FileHandle);
      return(Status);
   }

   CurrentRetDescList = RetDescList = MmAllocRetrievelDescriptorList(PAIRS_PER_RUN);

   if (CurrentRetDescList == NULL)
   {
      ObDereferenceObject(FileObject);
      ZwClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

#if defined(__GNUC__)
   Vcn.QuadPart = 0LL;
#else
   Vcn.QuadPart = 0;
#endif

   ExtentCount = 0;
   MaxVcn.QuadPart = (SafeInitialSize.QuadPart + BytesPerAllocationUnit - 1) / BytesPerAllocationUnit;
   while(1)
   {
      Status = ZwFsControlFile(FileHandle,
                               0,
                               NULL,
                               NULL,
                               &IoStatus,
                               FSCTL_GET_RETRIEVAL_POINTERS,
                               &Vcn,
                               sizeof(LARGE_INTEGER),
                               &CurrentRetDescList->RetrievalPointers,
                               sizeof(RETRIEVAL_POINTERS_BUFFER) + PAIRS_PER_RUN * 2 * sizeof(LARGE_INTEGER));
      if (!NT_SUCCESS(Status))
      {
         while (RetDescList)
         {
            CurrentRetDescList = RetDescList;
            RetDescList = RetDescList->Next;
            ExFreePool(CurrentRetDescList);
         }
         ObDereferenceObject(FileObject);
         ZwClose(FileHandle);
         return(Status);
      }
      ExtentCount += CurrentRetDescList->RetrievalPointers.ExtentCount;
      if (CurrentRetDescList->RetrievalPointers.Extents[CurrentRetDescList->RetrievalPointers.ExtentCount-1].NextVcn.QuadPart < MaxVcn.QuadPart)
      {
         CurrentRetDescList->Next = MmAllocRetrievelDescriptorList(PAIRS_PER_RUN);
         if (CurrentRetDescList->Next == NULL)
         {
            while (RetDescList)
            {
               CurrentRetDescList = RetDescList;
               RetDescList = RetDescList->Next;
               ExFreePool(CurrentRetDescList);
            }
            ObDereferenceObject(FileObject);
            ZwClose(FileHandle);
            return(STATUS_NO_MEMORY);
         }
         Vcn = CurrentRetDescList->RetrievalPointers.Extents[CurrentRetDescList->RetrievalPointers.ExtentCount-1].NextVcn;
         CurrentRetDescList = CurrentRetDescList->Next;
      }
      else
      {
         break;
      }
   }

   PagingFile = ExAllocatePool(NonPagedPool, sizeof(*PagingFile));
   if (PagingFile == NULL)
   {
      while (RetDescList)
      {
         CurrentRetDescList = RetDescList;
         RetDescList = RetDescList->Next;
         ExFreePool(CurrentRetDescList);
      }
      ObDereferenceObject(FileObject);
      ZwClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

   RtlZeroMemory(PagingFile, sizeof(*PagingFile));

   PagingFile->FileObject = FileObject;
   PagingFile->MaximumSize = SafeMaximumSize.LowPart;
   PagingFile->CurrentSize = SafeInitialSize.LowPart;
   PagingFile->FreePages = (ULONG)(SafeMaximumSize.LowPart / PAGE_SIZE);
   PagingFile->UsedPages = 0;
   KeInitializeSpinLock(&PagingFile->AllocMapLock);

   AllocMapSize = (PagingFile->FreePages / 32) + 1;
   PagingFile->AllocMap = ExAllocatePool(NonPagedPool,
                                         AllocMapSize * sizeof(ULONG));

   if (PagingFile->AllocMap == NULL)
   {
      while (RetDescList)
      {
         CurrentRetDescList = RetDescList;
         RetDescList = RetDescList->Next;
         ExFreePool(CurrentRetDescList);
      }
      ExFreePool(PagingFile);
      ObDereferenceObject(FileObject);
      ZwClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }
   DPRINT("ExtentCount: %lu\n", ExtentCount);
   Size = sizeof(RETRIEVAL_POINTERS_BUFFER) + ExtentCount * 2 * sizeof(LARGE_INTEGER);
   PagingFile->RetrievalPointers = ExAllocatePool(NonPagedPool, Size);
   if (PagingFile->RetrievalPointers == NULL)
   {
      while (RetDescList)
      {
         CurrentRetDescList = RetDescList;
         RetDescList = RetDescList->Next;
         ExFreePool(CurrentRetDescList);
      }
      ExFreePool(PagingFile->AllocMap);
      ExFreePool(PagingFile);
      ObDereferenceObject(FileObject);
      ZwClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

   RtlZeroMemory(PagingFile->RetrievalPointers, Size);

   RtlInitializeBitMap(&PagingFile->AllocBitMap, PagingFile->AllocMap, AllocMapSize * 32);
   RtlClearAllBits(&PagingFile->AllocBitMap);

   Count = 0;
   PagingFile->RetrievalPointers->ExtentCount = ExtentCount;
   PagingFile->RetrievalPointers->StartingVcn = RetDescList->RetrievalPointers.StartingVcn;
   CurrentRetDescList = RetDescList;
   while (CurrentRetDescList)
   {
      memcpy(&PagingFile->RetrievalPointers->Extents[Count],
             CurrentRetDescList->RetrievalPointers.Extents,
             CurrentRetDescList->RetrievalPointers.ExtentCount * 2 * sizeof(LARGE_INTEGER));
      Count += CurrentRetDescList->RetrievalPointers.ExtentCount;
      RetDescList = CurrentRetDescList;
      CurrentRetDescList = CurrentRetDescList->Next;
      ExFreePool(RetDescList);
   }

   if (PagingFile->RetrievalPointers->ExtentCount != ExtentCount ||
         PagingFile->RetrievalPointers->Extents[ExtentCount - 1].NextVcn.QuadPart != MaxVcn.QuadPart)
   {
      ExFreePool(PagingFile->RetrievalPointers);
      ExFreePool(PagingFile->AllocMap);
      ExFreePool(PagingFile);
      ObDereferenceObject(FileObject);
      ZwClose(FileHandle);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Change the entries from lcn's to volume offset's.
    */
   PagingFile->RetrievalPointers->StartingVcn.QuadPart *= BytesPerAllocationUnit;
   for (i = 0; i < ExtentCount; i++)
   {
      PagingFile->RetrievalPointers->Extents[i].Lcn.QuadPart *= BytesPerAllocationUnit;
      PagingFile->RetrievalPointers->Extents[i].NextVcn.QuadPart *= BytesPerAllocationUnit;
   }

   OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
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
   KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

   ZwClose(FileHandle);

   MmSwapSpaceMessage = FALSE;

   return(STATUS_SUCCESS);
}

/* EOF */
