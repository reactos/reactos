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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: pagefile.c,v 1.43 2004/04/10 22:35:25 gdalsnes Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pagefile.c
 * PURPOSE:         Paging file functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/mm.h>
#include <napi/core.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct _PAGINGFILE
{
   LIST_ENTRY PagingFileListEntry;
   PFILE_OBJECT FileObject;
   LARGE_INTEGER MaximumSize;
   LARGE_INTEGER CurrentSize;
   ULONG FreePages;
   ULONG UsedPages;
   PULONG AllocMap;
   KSPIN_LOCK AllocMapLock;
   ULONG AllocMapSize;
   PGET_RETRIEVAL_DESCRIPTOR RetrievalPointers;
}
PAGINGFILE, *PPAGINGFILE;

typedef struct _RETRIEVEL_DESCRIPTOR_LIST
{
   struct _RETRIEVEL_DESCRIPTOR_LIST* Next;
   GET_RETRIEVAL_DESCRIPTOR RetrievalPointers;
}
RETRIEVEL_DESCRIPTOR_LIST, *PRETRIEVEL_DESCRIPTOR_LIST;

/* GLOBALS *******************************************************************/

#define PAIRS_PER_RUN (1024)

#define MAX_PAGING_FILES  (32)

/* List of paging files, both used and free */
static PPAGINGFILE PagingFileList[MAX_PAGING_FILES];

/* Lock for examining the list of paging files */
static KSPIN_LOCK PagingFileListLock;

/* Number of paging files */
static ULONG MiPagingFileCount;

/* Number of pages that are available for swapping */
static ULONG MiFreeSwapPages;

/* Number of pages that have been allocated for swapping */
static ULONG MiUsedSwapPages;

/*
 * Number of pages that have been reserved for swapping but not yet allocated 
 */
static ULONG MiReservedSwapPages;

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

static PVOID MmCoreDumpPageFrame = NULL;
static ULONG MmCoreDumpSize;
static DUMP_POINTERS MmCoreDumpPointers;
static PMM_CORE_DUMP_FUNCTIONS MmCoreDumpFunctions;
static ULONG MmCoreDumpPageFile = 0xFFFFFFFF;
static ROS_QUERY_LCN_MAPPING MmCoreDumpLcnMapping;

ULONG MmCoreDumpType = MM_CORE_DUMP_TYPE_NONE;

/*
 * Translate between a swap entry and a file and offset pair.
 */
#define FILE_FROM_ENTRY(i) ((i) >> 24)
#define OFFSET_FROM_ENTRY(i) (((i) & 0xffffff) - 1)
#define ENTRY_FROM_FILE_OFFSET(i, j) (((i) << 24) | ((j) + 1))

static BOOLEAN MmSwapSpaceMessage = FALSE;

/* FUNCTIONS *****************************************************************/

VOID
MmShowOutOfSpaceMessagePagingFile(VOID)
{
   if (!MmSwapSpaceMessage)
   {
      DPRINT1("MM: Out of swap space.\n");
      MmSwapSpaceMessage = TRUE;
   }
}

LARGE_INTEGER STATIC
MmGetOffsetPageFile(PGET_RETRIEVAL_DESCRIPTOR RetrievalPointers, LARGE_INTEGER Offset)
{
   /* Simple binary search */
   ULONG first, last, mid;
   first = 0;
   last = RetrievalPointers->NumberOfPairs - 1;
   while (first <= last)
   {
      mid = (last - first) / 2 + first;
      if ((ULONGLONG) Offset.QuadPart < RetrievalPointers->Pair[mid].Vcn)
      {
         if (mid == 0)
         {
            Offset.QuadPart += RetrievalPointers->Pair[0].Lcn - RetrievalPointers->StartVcn;
            return Offset;
         }
         else
         {
            if ((ULONGLONG) Offset.QuadPart >= RetrievalPointers->Pair[mid-1].Vcn)
            {
               Offset.QuadPart += RetrievalPointers->Pair[mid].Lcn  - RetrievalPointers->Pair[mid-1].Vcn;
               return Offset;
            }
            last = mid - 1;
         }
      }
      else
      {
         if (mid == RetrievalPointers->NumberOfPairs - 1)
         {
            break;
         }
         if ((ULONGLONG) Offset.QuadPart < RetrievalPointers->Pair[mid+1].Vcn)
         {
            Offset.QuadPart += RetrievalPointers->Pair[mid+1].Lcn  - RetrievalPointers->Pair[mid].Vcn;
            return Offset;
         }
         first = mid + 1;
      }
   }
   KEBUGCHECK(0);
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

NTSTATUS MmWriteToSwapPage(SWAPENTRY SwapEntry, PMDL Mdl)
{
   ULONG i, offset;
   LARGE_INTEGER file_offset;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   KEVENT Event;

   DPRINT("MmWriteToSwapPage\n");

   if (SwapEntry == 0)
   {
      KEBUGCHECK(0);
      return(STATUS_UNSUCCESSFUL);
   }

   i = FILE_FROM_ENTRY(SwapEntry);
   offset = OFFSET_FROM_ENTRY(SwapEntry);

   if (i >= MAX_PAGING_FILES)
   {
      DPRINT1("Bad swap entry 0x%.8X\n", SwapEntry);
      KEBUGCHECK(0);
   }
   if (PagingFileList[i]->FileObject == NULL ||
         PagingFileList[i]->FileObject->DeviceObject == NULL)
   {
      DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
      KEBUGCHECK(0);
   }

   file_offset.QuadPart = offset * PAGE_SIZE;
   file_offset = MmGetOffsetPageFile(PagingFileList[i]->RetrievalPointers, file_offset);

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoPageWrite(PagingFileList[i]->FileObject,
                        Mdl,
                        &file_offset,
                        &Event,
                        &Iosb);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      return(Iosb.Status);
   }
   return(Status);
}

NTSTATUS MmReadFromSwapPage(SWAPENTRY SwapEntry, PMDL Mdl)
{
   ULONG i, offset;
   LARGE_INTEGER file_offset;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   KEVENT Event;

   DPRINT("MmReadFromSwapPage\n");

   if (SwapEntry == 0)
   {
      KEBUGCHECK(0);
      return(STATUS_UNSUCCESSFUL);
   }

   i = FILE_FROM_ENTRY(SwapEntry);
   offset = OFFSET_FROM_ENTRY(SwapEntry);

   if (i >= MAX_PAGING_FILES)
   {
      DPRINT1("Bad swap entry 0x%.8X\n", SwapEntry);
      KEBUGCHECK(0);
   }
   if (PagingFileList[i]->FileObject == NULL ||
         PagingFileList[i]->FileObject->DeviceObject == NULL)
   {
      DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
      KEBUGCHECK(0);
   }

   file_offset.QuadPart = offset * PAGE_SIZE;
   file_offset = MmGetOffsetPageFile(PagingFileList[i]->RetrievalPointers, file_offset);

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoPageRead(PagingFileList[i]->FileObject,
                       Mdl,
                       &file_offset,
                       &Event,
                       &Iosb);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      return(Iosb.Status);
   }
   return(Status);
}

VOID INIT_FUNCTION
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
   MiPagingFileCount = 0;

   /*
    * Initialize the crash dump support.
    */
   if (MmCoreDumpType != MM_CORE_DUMP_TYPE_NONE)
   {
      MmCoreDumpPageFrame = MmAllocateSection(PAGE_SIZE);
      if (MmCoreDumpType == MM_CORE_DUMP_TYPE_FULL)
      {
         MmCoreDumpSize = MmStats.NrTotalPages * 4096 + 1024 * 1024;
      }
      else
      {
         MmCoreDumpSize = 1024 * 1024;
      }
   }
}

BOOLEAN
MmReserveSwapPages(ULONG Nr)
{
   KIRQL oldIrql;
   ULONG MiAvailSwapPages;

   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   MiAvailSwapPages =
      (MiFreeSwapPages * MM_PAGEFILE_COMMIT_RATIO) + MM_PAGEFILE_COMMIT_GRACE;
   MiReservedSwapPages = MiReservedSwapPages + Nr;
   if (MM_PAGEFILE_COMMIT_RATIO != 0 && MiAvailSwapPages < MiReservedSwapPages)
   {
      KeReleaseSpinLock(&PagingFileListLock, oldIrql);
      return(FALSE);
   }
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
   return(TRUE);
}

VOID
MmDereserveSwapPages(ULONG Nr)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   MiReservedSwapPages = MiReservedSwapPages - Nr;
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
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
            break;
         }
      }
      if (j == 32)
      {
         continue;
      }
      PagingFile->AllocMap[i] |= (1 << j);
      PagingFile->UsedPages++;
      PagingFile->FreePages--;
      KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
      return((i * 32) + j);
   }

   KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
   return(0xFFFFFFFF);
}

VOID
MmFreeSwapPage(SWAPENTRY Entry)
{
   ULONG i;
   ULONG off;
   KIRQL oldIrql;

   i = FILE_FROM_ENTRY(Entry);
   off = OFFSET_FROM_ENTRY(Entry);

   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   if (PagingFileList[i] == NULL)
   {
      KEBUGCHECK(0);
   }
   KeAcquireSpinLockAtDpcLevel(&PagingFileList[i]->AllocMapLock);

   PagingFileList[i]->AllocMap[off / 32] &= (~(1 << (off % 32)));

   PagingFileList[i]->FreePages++;
   PagingFileList[i]->UsedPages--;

   MiFreeSwapPages++;
   MiUsedSwapPages--;

   KeReleaseSpinLockFromDpcLevel(&PagingFileList[i]->AllocMapLock);
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
}

BOOLEAN
MmIsAvailableSwapPage(VOID)
{
   return(MiFreeSwapPages > 0);
}

SWAPENTRY
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
            KEBUGCHECK(0);
            KeReleaseSpinLock(&PagingFileListLock, oldIrql);
            return(STATUS_UNSUCCESSFUL);
         }
         MiUsedSwapPages++;
         MiFreeSwapPages--;
         KeReleaseSpinLock(&PagingFileListLock, oldIrql);

         entry = ENTRY_FROM_FILE_OFFSET(i, off);
         return(entry);
      }
   }

   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
   KEBUGCHECK(0);
   return(0);
}

STATIC PRETRIEVEL_DESCRIPTOR_LIST FASTCALL
MmAllocRetrievelDescriptorList(ULONG Pairs)
{
   ULONG Size;
   PRETRIEVEL_DESCRIPTOR_LIST RetDescList;

   Size = sizeof(RETRIEVEL_DESCRIPTOR_LIST) + Pairs * sizeof(MAPPING_PAIR);
   RetDescList = ExAllocatePool(NonPagedPool, Size);
   if (RetDescList)
   {
      RtlZeroMemory(RetDescList, Size);
   }

   return RetDescList;
}

NTSTATUS STDCALL
MmDumpToPagingFile(ULONG BugCode,
                   ULONG BugCodeParameter1,
                   ULONG BugCodeParameter2,
                   ULONG BugCodeParameter3,
                   ULONG BugCodeParameter4,
                   PKTRAP_FRAME TrapFrame)
{
   PMM_CORE_DUMP_HEADER Headers;
   NTSTATUS Status;
   UCHAR MdlBase[sizeof(MDL) + sizeof(PVOID)];
   PMDL Mdl = (PMDL)MdlBase;
   PETHREAD Thread = PsGetCurrentThread();
   ULONG StackSize;
   PULONG MdlMap;
   LONGLONG NextOffset = 0;
   ULONG i;
   PGET_RETRIEVAL_DESCRIPTOR RetrievalPointers;
   LARGE_INTEGER DiskOffset;

   if (MmCoreDumpPageFile == 0xFFFFFFFF)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   DbgPrint("\nMM: Dumping core: ");

   /* Prepare the dump headers. */
   Headers = (PMM_CORE_DUMP_HEADER)MmCoreDumpPageFrame;
   Headers->Magic = MM_CORE_DUMP_HEADER_MAGIC;
   Headers->Version = MM_CORE_DUMP_HEADER_VERSION;
   Headers->Type = MmCoreDumpType;
   if (TrapFrame != NULL)
   {
      if (!(TrapFrame->Eflags & (1 << 17)))
      {
         memcpy(&Headers->TrapFrame, TrapFrame,
                sizeof(KTRAP_FRAME) - (4 * sizeof(DWORD)));
      }
      else
      {
         memcpy(&Headers->TrapFrame, TrapFrame, sizeof(KTRAP_FRAME));
      }
   }
   Headers->BugCheckCode = BugCode;
   Headers->BugCheckParameters[0] = BugCodeParameter1;
   Headers->BugCheckParameters[1] = BugCodeParameter2;
   Headers->BugCheckParameters[2] = BugCodeParameter3;
   Headers->BugCheckParameters[3] = BugCodeParameter4;
   Headers->FaultingStackBase = (PVOID)Thread->Tcb.StackLimit;
   Headers->FaultingStackSize = StackSize =
                                   (ULONG)((char*)Thread->Tcb.StackBase - Thread->Tcb.StackLimit);
   Headers->PhysicalMemorySize = MmStats.NrTotalPages * PAGE_SIZE;

   /* Initialize the dump device. */
   Status = MmCoreDumpFunctions->DumpInit();
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to initialize core dump device.\n");
      return(Status);
   }

   /* Initialize the MDL. */
   Mdl->Next = NULL;
   Mdl->Size = sizeof(MDL) + sizeof(PVOID);
   Mdl->MdlFlags = MDL_SOURCE_IS_NONPAGED_POOL;
   Mdl->Process = NULL;
   Mdl->MappedSystemVa = MmCoreDumpPageFrame;
   Mdl->StartVa = NULL;
   Mdl->ByteCount = PAGE_SIZE;
   Mdl->ByteOffset = 0;
   MdlMap = (PULONG)(Mdl + 1);


   /* Initialize the retrieval offsets. */
   RetrievalPointers = PagingFileList[MmCoreDumpPageFile]->RetrievalPointers;

   /* Dump the header. */
   MdlMap[0] = MmGetPhysicalAddress(MmCoreDumpPageFrame).u.LowPart;
#if defined(__GNUC__)

   DiskOffset = MmGetOffsetPageFile(RetrievalPointers, (LARGE_INTEGER)0LL);
#else

   {
      const LARGE_INTEGER dummy =
         {
            0
         };
      DiskOffset = MmGetOffsetPageFile(RetrievalPointers, dummy);
   }
#endif
   DiskOffset.QuadPart += MmCoreDumpLcnMapping.LcnDiskOffset.QuadPart;
   Status = MmCoreDumpFunctions->DumpWrite(DiskOffset, Mdl);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write core dump header\n.");
      return(Status);
   }
   NextOffset += PAGE_SIZE;
   ;
   DbgPrint("00");


   /* Write out the contents of physical memory. */
   if (MmCoreDumpType == MM_CORE_DUMP_TYPE_FULL)
   {
      for (i = 0; i < MmStats.NrTotalPages; i++)
      {
         LARGE_INTEGER PhysicalAddress;
         PhysicalAddress.QuadPart = i * PAGE_SIZE;
         MdlMap[0] = i * PAGE_SIZE;
         MmCreateVirtualMappingDump(MmCoreDumpPageFrame,
                                    PAGE_READWRITE,
                                    PhysicalAddress);
#if defined(__GNUC__)

         DiskOffset = MmGetOffsetPageFile(RetrievalPointers,
                                          (LARGE_INTEGER)NextOffset);
#else

         {
            LARGE_INTEGER dummy;
            dummy.QuadPart = NextOffset;
            DiskOffset = MmGetOffsetPageFile(RetrievalPointers, dummy);
         }
#endif
         DiskOffset.QuadPart += MmCoreDumpLcnMapping.LcnDiskOffset.QuadPart;
         Status = MmCoreDumpFunctions->DumpWrite(DiskOffset, Mdl);
         if (!NT_SUCCESS(Status))
         {
            DPRINT1("MM: Failed to write page to core dump.\n");
            return(Status);
         }
         if ((i % ((1024*1024) / PAGE_SIZE)) == 0)
         {
            DbgPrint("\b\b%.2d", i / ((1024*1024)/PAGE_SIZE));
         }
         NextOffset += PAGE_SIZE;
      }
   }

   DbgPrint("\n");
   MmCoreDumpFunctions->DumpFinish();
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
MmInitializeCrashDump(HANDLE PageFileHandle, ULONG PageFileNum)
{
   PFILE_OBJECT PageFile;
   PDEVICE_OBJECT PageFileDevice;
   NTSTATUS Status;
   PIRP Irp;
   KEVENT Event;
   IO_STATUS_BLOCK Iosb;
   UNICODE_STRING DiskDumpName;
   ANSI_STRING ProcName;
   PIO_STACK_LOCATION StackPtr;
   PMODULE_OBJECT ModuleObject;

   Status = ZwFsControlFile(PageFileHandle,
                            0,
                            NULL,
                            NULL,
                            &Iosb,
                            FSCTL_ROS_QUERY_LCN_MAPPING,
                            NULL,
                            0,
                            &MmCoreDumpLcnMapping,
                            sizeof(ROS_QUERY_LCN_MAPPING));
   if (!NT_SUCCESS(Status) ||
         Iosb.Information != sizeof(ROS_QUERY_LCN_MAPPING))
   {
      return(Status);
   }

   /* Get the underlying storage device. */
   Status =
      ObReferenceObjectByHandle(PageFileHandle,
                                FILE_ALL_ACCESS,
                                NULL,
                                KernelMode,
                                (PVOID*)&PageFile,
                                NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   PageFileDevice = PageFile->Vpb->RealDevice;

   /* Get the dump pointers. */
   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_DUMP_POINTERS,
                                       PageFileDevice,
                                       NULL,
                                       0,
                                       &MmCoreDumpPointers,
                                       sizeof(MmCoreDumpPointers),
                                       FALSE,
                                       &Event,
                                       &Iosb);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = PageFile;
   StackPtr->DeviceObject = PageFileDevice;
   StackPtr->Parameters.DeviceIoControl.InputBufferLength = 0;
   StackPtr->Parameters.DeviceIoControl.OutputBufferLength = sizeof(MmCoreDumpPointers);

   Status = IoCallDriver(PageFileDevice,Irp);
   if (Status == STATUS_PENDING)
   {
      Status = KeWaitForSingleObject(&Event,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     NULL);
   }
   if (Status != STATUS_SUCCESS ||
         Iosb.Information != sizeof(MmCoreDumpPointers))
   {
      ObDereferenceObject(PageFile);
      return(Status);
   }

   /* Load the diskdump driver. */
   RtlRosInitUnicodeStringFromLiteral(&DiskDumpName, L"DiskDump");
   ModuleObject = LdrGetModuleObject(&DiskDumpName);
   if (ModuleObject == NULL)
   {
      return(STATUS_OBJECT_NAME_NOT_FOUND);
   }
   RtlInitAnsiString(&ProcName, "DiskDumpFunctions");
   Status = LdrGetProcedureAddress(ModuleObject->Base,
                                   &ProcName,
                                   0,
                                   (PVOID*)&MmCoreDumpFunctions);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(PageFile);
      return(Status);
   }

   /* Prepare for disk dumping. */
   Status = MmCoreDumpFunctions->DumpPrepare(PageFileDevice,
            &MmCoreDumpPointers);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(PageFile);
      return(Status);
   }

   MmCoreDumpPageFile = PageFileNum;
   ObDereferenceObject(PageFile);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
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
   FILE_FS_SIZE_INFORMATION FsSizeInformation;
   PRETRIEVEL_DESCRIPTOR_LIST RetDescList;
   PRETRIEVEL_DESCRIPTOR_LIST CurrentRetDescList;
   ULONG i;
   ULONG BytesPerAllocationUnit;
   LARGE_INTEGER Vcn;
   ULONG ExtentCount;
   ULONG MaxVcn;
   ULONG Count;
   ULONG Size;

   DPRINT("NtCreatePagingFile(FileName %wZ, InitialSize %I64d)\n",
          FileName, InitialSize->QuadPart);

   if (MiPagingFileCount >= MAX_PAGING_FILES)
   {
      return(STATUS_TOO_MANY_PAGING_FILES);
   }

   InitializeObjectAttributes(&ObjectAttributes,
                              FileName,
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
                         SL_OPEN_PAGING_FILE);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Status = NtQueryVolumeInformationFile(FileHandle,
                                         &IoStatus,
                                         &FsSizeInformation,
                                         sizeof(FILE_FS_SIZE_INFORMATION),
                                         FileFsSizeInformation);
   if (!NT_SUCCESS(Status))
   {
      NtClose(FileHandle);
      return Status;
   }

   BytesPerAllocationUnit = FsSizeInformation.SectorsPerAllocationUnit * FsSizeInformation.BytesPerSector;
   if (BytesPerAllocationUnit % PAGE_SIZE)
   {
      NtClose(FileHandle);
      return STATUS_UNSUCCESSFUL;
   }

   Status = NtSetInformationFile(FileHandle,
                                 &IoStatus,
                                 InitialSize,
                                 sizeof(LARGE_INTEGER),
                                 FileAllocationInformation);
   if (!NT_SUCCESS(Status))
   {
      NtClose(FileHandle);
      return(Status);
   }

   Status = ObReferenceObjectByHandle(FileHandle,
                                      FILE_ALL_ACCESS,
                                      IoFileObjectType,
                                      UserMode,
                                      (PVOID*)&FileObject,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      NtClose(FileHandle);
      return(Status);
   }

   CurrentRetDescList = RetDescList = MmAllocRetrievelDescriptorList(PAIRS_PER_RUN);

   if (CurrentRetDescList == NULL)
   {
      ObDereferenceObject(FileObject);
      NtClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

#if defined(__GNUC__)
   Vcn.QuadPart = 0LL;
#else

   Vcn.QuadPart = 0;
#endif

   ExtentCount = 0;
   MaxVcn = (ULONG)((InitialSize->QuadPart + BytesPerAllocationUnit - 1) / BytesPerAllocationUnit);
   while(1)
   {
      Status = NtFsControlFile(FileHandle,
                               0,
                               NULL,
                               NULL,
                               &IoStatus,
                               FSCTL_GET_RETRIEVAL_POINTERS,
                               &Vcn,
                               sizeof(LARGE_INTEGER),
                               &CurrentRetDescList->RetrievalPointers,
                               sizeof(GET_RETRIEVAL_DESCRIPTOR) + PAIRS_PER_RUN * sizeof(MAPPING_PAIR));
      if (!NT_SUCCESS(Status))
      {
         while (RetDescList)
         {
            CurrentRetDescList = RetDescList;
            RetDescList = RetDescList->Next;
            ExFreePool(CurrentRetDescList);
         }
         ObDereferenceObject(FileObject);
         NtClose(FileHandle);
         return(Status);
      }
      ExtentCount += CurrentRetDescList->RetrievalPointers.NumberOfPairs;
      if ((ULONG)CurrentRetDescList->RetrievalPointers.Pair[CurrentRetDescList->RetrievalPointers.NumberOfPairs-1].Vcn < MaxVcn)
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
            NtClose(FileHandle);
            return(STATUS_NO_MEMORY);
         }
         Vcn.QuadPart = CurrentRetDescList->RetrievalPointers.Pair[CurrentRetDescList->RetrievalPointers.NumberOfPairs-1].Vcn;
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
      NtClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

   RtlZeroMemory(PagingFile, sizeof(*PagingFile));

   PagingFile->FileObject = FileObject;
   PagingFile->MaximumSize.QuadPart = MaximumSize->QuadPart;
   PagingFile->CurrentSize.QuadPart = InitialSize->QuadPart;
   PagingFile->FreePages = (ULONG)(InitialSize->QuadPart / PAGE_SIZE);
   PagingFile->UsedPages = 0;
   KeInitializeSpinLock(&PagingFile->AllocMapLock);

   AllocMapSize = (PagingFile->FreePages / 32) + 1;
   PagingFile->AllocMap = ExAllocatePool(NonPagedPool,
                                         AllocMapSize * sizeof(ULONG));
   PagingFile->AllocMapSize = AllocMapSize;

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
   DPRINT("ExtentCount: %d\n", ExtentCount);
   Size = sizeof(GET_RETRIEVAL_DESCRIPTOR) + ExtentCount * sizeof(MAPPING_PAIR);
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
      NtClose(FileHandle);
      return(STATUS_NO_MEMORY);
   }

   RtlZeroMemory(PagingFile->AllocMap, AllocMapSize * sizeof(ULONG));
   RtlZeroMemory(PagingFile->RetrievalPointers, Size);

   Count = 0;
   PagingFile->RetrievalPointers->NumberOfPairs = ExtentCount;
   PagingFile->RetrievalPointers->StartVcn = RetDescList->RetrievalPointers.StartVcn;
   CurrentRetDescList = RetDescList;
   while (CurrentRetDescList)
   {
      memcpy(&PagingFile->RetrievalPointers->Pair[Count],
             CurrentRetDescList->RetrievalPointers.Pair,
             CurrentRetDescList->RetrievalPointers.NumberOfPairs * sizeof(MAPPING_PAIR));
      Count += CurrentRetDescList->RetrievalPointers.NumberOfPairs;
      RetDescList = CurrentRetDescList;
      CurrentRetDescList = CurrentRetDescList->Next;
      ExFreePool(RetDescList);
   }

   if (PagingFile->RetrievalPointers->NumberOfPairs != ExtentCount ||
         (ULONG)PagingFile->RetrievalPointers->Pair[ExtentCount - 1].Vcn != MaxVcn)
   {
      ExFreePool(PagingFile->RetrievalPointers);
      ExFreePool(PagingFile->AllocMap);
      ExFreePool(PagingFile);
      ObDereferenceObject(FileObject);
      NtClose(FileHandle);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Change the entries from lcn's to volume offset's.
    */
   PagingFile->RetrievalPointers->StartVcn *= BytesPerAllocationUnit;
   for (i = 0; i < ExtentCount; i++)
   {
      PagingFile->RetrievalPointers->Pair[i].Lcn *= BytesPerAllocationUnit;
      PagingFile->RetrievalPointers->Pair[i].Vcn *= BytesPerAllocationUnit;
   }

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
   MiPagingFileCount++;
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);

   /* Check whether this pagefile can be a crash dump target. */
   if (MmCoreDumpType != MM_CORE_DUMP_TYPE_NONE &&
         PagingFile->CurrentSize.QuadPart >= MmCoreDumpSize &&
         MmCoreDumpPageFile == 0xFFFFFFFF)
   {
      MmInitializeCrashDump(FileHandle, i);
   }
   NtClose(FileHandle);

   MmSwapSpaceMessage = FALSE;

   return(STATUS_SUCCESS);
}

/* EOF */
