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
/* $Id: pagefile.c,v 1.22 2002/08/14 20:58:36 dwelch Exp $
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
} PAGINGFILE, *PPAGINGFILE;

/* GLOBALS *******************************************************************/

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

static PVOID MmCoreDumpPageFrame;
static PULONG MmCoreDumpBlockMap;
static ULONG MmCoreDumpSize;
static PULONG MmCoreDumpBlockMap = NULL;
static MM_DUMP_POINTERS MmCoreDumpDeviceFuncs;
ULONG MmCoreDumpType;

/*
 * Translate between a swap entry and a file and offset pair.
 */
#define FILE_FROM_ENTRY(i) ((i) >> 24)
#define OFFSET_FROM_ENTRY(i) (((i) & 0xffffff) - 1)
#define ENTRY_FROM_FILE_OFFSET(i, j) (((i) << 24) | ((j) + 1))

/* FUNCTIONS *****************************************************************/

NTSTATUS MmWriteToSwapPage(SWAPENTRY SwapEntry, PMDL Mdl)
{
   ULONG i, offset;
   LARGE_INTEGER file_offset;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   
   if (SwapEntry == 0)
     {
	KeBugCheck(0);
	return(STATUS_UNSUCCESSFUL);
     }
   
   i = FILE_FROM_ENTRY(SwapEntry);
   offset = OFFSET_FROM_ENTRY(SwapEntry);

   if (i > MAX_PAGING_FILES)
     {
       DPRINT1("Bad swap entry 0x%.8X\n", SwapEntry);
       KeBugCheck(0);
     }
   if (PagingFileList[i]->FileObject == NULL ||
       PagingFileList[i]->FileObject->DeviceObject == NULL)
     {
       DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
       KeBugCheck(0);
     }
   
   file_offset.QuadPart = offset * 4096;
     
   Status = IoPageWrite(PagingFileList[i]->FileObject,
			Mdl,
			&file_offset,
			&Iosb,
			TRUE);
   return(Status);
}

NTSTATUS MmReadFromSwapPage(SWAPENTRY SwapEntry, PMDL Mdl)
{
   ULONG i, offset;
   LARGE_INTEGER file_offset;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   
   if (SwapEntry == 0)
     {
	KeBugCheck(0);
	return(STATUS_UNSUCCESSFUL);
     }
   
   i = FILE_FROM_ENTRY(SwapEntry);
   offset = OFFSET_FROM_ENTRY(SwapEntry);

   if (i > MAX_PAGING_FILES)
     {
       DPRINT1("Bad swap entry 0x%.8X\n", SwapEntry);
       KeBugCheck(0);
     }
   if (PagingFileList[i]->FileObject == NULL ||
       PagingFileList[i]->FileObject->DeviceObject == NULL)
     {
       DPRINT1("Bad paging file 0x%.8X\n", SwapEntry);
       KeBugCheck(0);
     }
   
   file_offset.QuadPart = offset * 4096;
     
   Status = IoPageRead(PagingFileList[i]->FileObject,
		       Mdl,
		       &file_offset,
		       &Iosb,
		       TRUE);
   return(Status);
}

VOID 
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
   MmCoreDumpPageFrame = MmAllocateSection(PAGESIZE);
   if (MmCoreDumpType == MM_CORE_DUMP_TYPE_FULL)
     {
       MmCoreDumpSize = MmStats.NrTotalPages * 4096 + 1024 * 1024;
     }
   else
     {
       MmCoreDumpSize = 1024 * 1024;
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
       KeBugCheck(0);
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
   static BOOLEAN SwapSpaceMessage = FALSE;
   
   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   
   if (MiFreeSwapPages == 0)
     {
	KeReleaseSpinLock(&PagingFileListLock, oldIrql);
	if (!SwapSpaceMessage)
	  {
	    DPRINT1("MM: Out of swap space.\n");
	    SwapSpaceMessage = TRUE;
	  }
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
		  KeBugCheck(0);
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
   if (!SwapSpaceMessage)
     {
       DPRINT1("MM: Out of swap space.\n");
       SwapSpaceMessage = TRUE;
     }
   return(0);
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
  PVOID Context;
  NTSTATUS Status;
  UCHAR MdlBase[sizeof(MDL) + sizeof(PVOID)];
  PMDL Mdl = (PMDL)MdlBase;
  PETHREAD Thread = PsGetCurrentThread();
  ULONG StackSize;
  PULONG MdlMap;
  ULONG NextOffset = 0;
  ULONG i;

  if (MmCoreDumpBlockMap == NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  DbgPrint("MM: Dumping core");

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
    (ULONG)(Thread->Tcb.StackBase - Thread->Tcb.StackLimit);
  Headers->PhysicalMemorySize = MmStats.NrTotalPages * PAGESIZE;

  /* Initialize the dump device. */
  Context = MmCoreDumpDeviceFuncs.Context;
  Status = MmCoreDumpDeviceFuncs.DeviceInit(Context);
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
  Mdl->ByteCount = PAGESIZE;
  Mdl->ByteOffset = 0;
  MdlMap = (PULONG)(Mdl + 1);

  /* Dump the header. */
  MdlMap[0] = MmGetPhysicalAddress(MmCoreDumpPageFrame).u.LowPart;
  Status = MmCoreDumpDeviceFuncs.DeviceWrite(Context, 
					     MmCoreDumpBlockMap[NextOffset], 
					     Mdl);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MM: Failed to write core dump header\n.");
    }
  NextOffset++;
  DbgPrint(".");

  /* Write out the kernel mode stack of the faulting thread. */
  for (i = 0; i < (StackSize / PAGESIZE); i++)
    {
      Mdl->MappedSystemVa = (PVOID)(Thread->Tcb.StackLimit + (i * PAGESIZE));
      MdlMap[0] = MmGetPhysicalAddress(Mdl->MappedSystemVa).u.LowPart;      
      Status = 
	MmCoreDumpDeviceFuncs.DeviceWrite(Context, 
					  MmCoreDumpBlockMap[NextOffset],
					  Mdl);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("MM: Failed to write page to core dump.\n");
	  return(Status);
	}
      DbgPrint(".");
      NextOffset++;
    }

  /* Write out the contents of physical memory. */
  if (MmCoreDumpType == MM_CORE_DUMP_TYPE_FULL)
    {
      for (i = 0; i < MmStats.NrTotalPages; i++)
	{
	  LARGE_INTEGER PhysicalAddress;
	  PhysicalAddress.QuadPart = i * PAGESIZE;
	  MdlMap[0] = i * PAGESIZE;
	  MmCreateVirtualMappingForKernel(MmCoreDumpPageFrame,
					  PAGE_READWRITE,
					  PhysicalAddress);
	  Status = 
	    MmCoreDumpDeviceFuncs.DeviceWrite(Context, 
					      MmCoreDumpBlockMap[NextOffset],
					      Mdl);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("MM: Failed to write page to core dump.\n");
	      return(Status);
	    }
	  DbgPrint(".");
	  NextOffset++;
	}
    }

  DbgPrint("\n");
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
   ULONG i;

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

   Status = NtSetInformationFile(FileHandle,
				 &IoStatus,
				 InitialSize,
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
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	NtClose(FileHandle);
	return(Status);
     }
   
   PagingFile = ExAllocatePool(NonPagedPool, sizeof(*PagingFile));
   if (PagingFile == NULL)
     {
	ObDereferenceObject(FileObject);
	NtClose(FileHandle);
	return(STATUS_NO_MEMORY);
     }
   
   PagingFile->FileObject = FileObject;
   PagingFile->MaximumSize.QuadPart = MaximumSize->QuadPart;
   PagingFile->CurrentSize.QuadPart = InitialSize->QuadPart;
   PagingFile->FreePages = InitialSize->QuadPart / PAGESIZE;
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
       NtClose(FileHandle);
       return(STATUS_NO_MEMORY);
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
   if (PagingFile->CurrentSize.QuadPart >= MmCoreDumpSize &&
       MmCoreDumpBlockMap != NULL)
     {
       MmCoreDumpBlockMap = 
	 ExAllocatePool(NonPagedPool, 
			(MmCoreDumpSize / PAGESIZE) * sizeof(ULONG));
       if (MmCoreDumpBlockMap == NULL)
	 {
	   DPRINT1("Failed to allocate block map.\n");
	   NtClose(FileHandle);
	   return(STATUS_SUCCESS);
	 }
       Status = ZwFsControlFile(FileHandle,
				NULL,
				NULL,
				NULL,
				&IoStatus,
				FSCTL_GET_DUMP_BLOCK_MAP,
				&MmCoreDumpSize,
				sizeof(ULONG),
				MmCoreDumpBlockMap,
				(MmCoreDumpSize / PAGESIZE) * sizeof(ULONG));
       if (!NT_SUCCESS(Status))
	 {
	   DPRINT1("Failed to get dump block map (Status %X)\n", Status);
	   NtClose(FileHandle);
	   ExFreePool(MmCoreDumpBlockMap);
	   MmCoreDumpBlockMap = NULL;
	   return(STATUS_SUCCESS);
	 }
       Status = ZwDeviceIoControlFile(FileHandle,
				      NULL,
				      NULL,
				      NULL,
				      &IoStatus,
				      IOCTL_GET_DUMP_POINTERS,
				      NULL,
				      0,
				      &MmCoreDumpDeviceFuncs,
				      sizeof(MmCoreDumpDeviceFuncs));
       if (!NT_SUCCESS(Status))
	 {
	   DPRINT1("Failed to get dump block map (Status %X)\n", Status);
	   NtClose(FileHandle);
	   ExFreePool(MmCoreDumpBlockMap);
	   MmCoreDumpBlockMap = NULL;
	   return(STATUS_SUCCESS);
	 }
     }
   NtClose(FileHandle);
   return(STATUS_SUCCESS);
}


/* EOF */







