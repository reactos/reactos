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
/* $Id: pagefile.c,v 1.19 2002/05/13 18:10:40 chorns Exp $
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

#if 0
static PVOID MmCoreDumpPageFrame;
static BYTE MmCoreDumpHeader[PAGESIZE];
#endif

/*
 * Translate between a swap entry and a file and offset pair.
 */
#define PTE_SWAP_FILE_MASK  0x0f000000
#define PTE_SWAP_FILE_BIT   24
#define FILE_FROM_ENTRY(i) (((i) & PTE_SWAP_FILE_MASK) >> PTE_SWAP_FILE_BIT)
#define OFFSET_FROM_ENTRY(i) (((i) & 0xffffff) - 1)
#define ENTRY_FROM_FILE_OFFSET(i, j) (((i) << PTE_SWAP_FILE_BIT) | ((j) + 1))

/* FUNCTIONS *****************************************************************/

#ifdef DBG

VOID
MiValidateSwapEntry(SWAPENTRY Entry)
{
	ULONG i;
	ULONG off;

  if (Entry != 0)
    {
	    DPRINT("MiValidateSwapEntry(SwapEntry 0x%.08x)\n", Entry);

			i = FILE_FROM_ENTRY(Entry);
		
			assertmsg(i < MAX_PAGING_FILES,
		    ("Bad SwapEntry (0x%.08x). Wrong paging file number (%d, 0x%.08x)\n", Entry, i, off));
		
			off = OFFSET_FROM_ENTRY(Entry);
		
			assertmsg(off / 32 <= PagingFileList[i]->AllocMapSize,
			  ("Bad SwapEntry (0x%.08x). Wrong paging file offset (%d, 0x%.08x)\n", Entry, i, off));
    }
}
	
#endif

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
     
  if (file_offset.QuadPart > PagingFileList[i]->MaximumSize.QuadPart)
		{
      DPRINT1("Bad swap file offset 0x%.08x\n", file_offset.u.LowPart);
      KeBugCheck(0);
		}

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
   
   DPRINT("MmReadFromSwapPage(SwapEntry 0x%.08x)\n", SwapEntry);

   VALIDATE_SWAP_ENTRY(SwapEntry);

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

  if (file_offset.QuadPart > PagingFileList[i]->MaximumSize.QuadPart)
		{
      DPRINT1("Bad swap file offset 0x%.08x\n", file_offset.u.LowPart);
      KeBugCheck(0);
		}
     
   Status = IoPageRead(PagingFileList[i]->FileObject,
		       Mdl,
		       &file_offset,
		       &Iosb,
		       TRUE);
   DPRINT("MmReadFromSwapPage() Status 0x%.8X\n", Status);
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
}

BOOLEAN
MmReserveSwapPages(ULONG Nr)
{
   KIRQL oldIrql;
   ULONG MiAvailSwapPages;
   
   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   MiAvailSwapPages =
     (MiFreeSwapPages * MM_PAGEFILE_COMMIT_RATIO) + MM_PAGEFILE_COMMIT_GRACE;
   if (MM_PAGEFILE_COMMIT_RATIO != 0 && MiAvailSwapPages < MiReservedSwapPages)
     {
       KeReleaseSpinLock(&PagingFileListLock, oldIrql);
       return(FALSE);
     }
   MiReservedSwapPages = MiReservedSwapPages + Nr;
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

   assertmsg(i < MAX_PAGING_FILES,
     ("Bad SwapEntry (0x%.08x). Wrong paging file number (%d, 0x%.08x)\n", Entry, i, off));

   off = OFFSET_FROM_ENTRY(Entry);

   assertmsg(off / 32 <= PagingFileList[i]->AllocMapSize,
     ("Bad SwapEntry (0x%.08x). Wrong paging file offset (%d, 0x%.08x)\n", Entry, i, off));
   
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

#if 0
NTSTATUS STDCALL MmDumpToPagingFile(PCONTEXT Context,
				    ULONG BugCode,
				    ULONG ExceptionCode,
				    ULONG Cr2)
{
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->Magic = 
     MM_CORE_DUMP_HEADER_MAGIC;
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->Version = 
     MM_CORE_DUMP_HEADER_VERSION;
   memcpy(&((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->Context,
	  Context,
	  sizeof(CONTEXT));
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->DumpLength = 0;
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->BugCode = BugCode;
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->ExceptionCode = 
     ExceptionCode;
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->Cr2 = Cr2;
   ((PMM_CORE_DUMP_HEADER)MmCoreDumpHeader)->Cr3 = 0;
}
#endif

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
   PVOID Buffer;

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
  DPRINT1("Failed to open swap file (Status 0x%.08x)\n", Status);
	return(Status);
     }

   Buffer = ExAllocatePool(NonPagedPool, 4096);
   memset(Buffer, 0, 4096);
   Status = NtWriteFile(FileHandle,
			NULL,
			NULL,
			NULL,
			&IoStatus,
			Buffer,
			4096,
			InitialSize,
			NULL);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("Failed to write to swap file (Status 0x%.08x)\n", Status);
       NtClose(FileHandle);
       return(Status);
     }
   ExFreePool(Buffer);

   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_ALL_ACCESS,
				      IoFileObjectType,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
  DPRINT1("Failed to reference swap file (Status 0x%.08x)\n", Status);
	NtClose(FileHandle);
	return(Status);
     }
   
   NtClose(FileHandle);
   
   PagingFile = ExAllocatePool(NonPagedPool, sizeof(*PagingFile));
   if (PagingFile == NULL)
     {
	ObDereferenceObject(FileObject);
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

   DPRINT("Successfully opened swap file\n");
   
   return(STATUS_SUCCESS);
}


/* EOF */
