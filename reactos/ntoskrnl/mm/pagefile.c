/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/pagefile.c
 * PURPOSE:         Paging file functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/bitops.h>
#include <internal/io.h>

#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef ULONG SWAPENTRY;

typedef struct _PPAGINGFILE
{
   LIST_ENTRY PagingFileListEntry;
   PFILE_OBJECT FileObject;
   ULONG MaximumSize;
   ULONG CurrentSize;
   ULONG FreePages;
   ULONG UsedPages;
   PULONG AllocMap;
   KSPIN_LOCK AllocMapLock;
   ULONG AllocMapSize;
} PAGINGFILE, *PPAGINGFILE;

/* GLOBALS *******************************************************************/

#define MAX_PAGING_FILES  (32)

static PPAGINGFILE PagingFileList[MAX_PAGING_FILES];
static KSPIN_LOCK PagingFileListLock;

static ULONG MiFreeSwapPages;
static ULONG MiUsedSwapPages;
static ULONG MiReservedSwapPages;

/* FUNCTIONS *****************************************************************/

VOID MmInitPagingFile(VOID)
{
   KeInitializeSpinLock(&PagingFileListLock);
   
   MiFreeSwapPages = 0;
   MiUsedSwapPages = 0;
   MiReservedSwapPages = 0;
}

VOID MmReserveSwapPages(ULONG Nr)
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   MiReservedSwapPages = MiReservedSwapPages + Nr;
   MiFreeSwapPages = MiFreeSwapPages - Nr;
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
}

VOID MmDereserveSwapPages(ULONG Nr)
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   MiReservedSwapPages = MiReservedSwapPages - Nr;
   MiFreeSwapPages = MiFreeSwapPages - Nr;
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
}

ULONG MiAllocPageFromPagingFile(PPAGINGFILE PagingFile)
{
   KIRQL oldIrql;
   ULONG i;
   ULONG off;
   
   KeAcquireSpinLock(&PagingFile->AllocMapLock, &oldIrql);
   
   for (i = 0; i < PagingFile->AllocMapSize; i++)
     {
	off = find_first_zero_bit(PagingFile->AllocMap, 
				  PagingFile->AllocMapSize * 32);
	clear_bit(off % 32, &PagingFile->AllocMap[off / 32]);
	PagingFile->UsedPages--;
	PagingFile->FreePages++;
	KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
	return(off + 1);
     }
   
   KeReleaseSpinLock(&PagingFile->AllocMapLock, oldIrql);
   return(0);
}

VOID MmFreeSwapPage(SWAPENTRY Entry)
{
   ULONG i;
   ULONG off;
   KIRQL oldIrql;
   
   i = (Entry >> 24) - 1;
   off = Entry & 0xffffff;
   
   KeAcquireSpinLock(&PagingFileListLock, &oldIrql);
   KeAcquireSpinLockAtDpcLevel(&PagingFileList[i]->AllocMapLock);
   
   set_bit(off % 32, &PagingFileList[i]->AllocMap[off / 32]);
   
   PagingFileList[i]->FreePages++;
   PagingFileList[i]->UsedPages--;
   
   MiFreeSwapPages++;
   MiUsedSwapPages--;
   
   KeReleaseSpinLockFromDpcLevel(&PagingFileList[i]->AllocMapLock);
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
}

SWAPENTRY MmAllocSwapPage(VOID)
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
	     MiUsedSwapPages++;
	     MiFreeSwapPages--;
	     KeReleaseSpinLock(&PagingFileListLock, oldIrql);
	     
	     entry = ((i+1) << 24) || off;
	     return(entry);
	  }
     }
   
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
   return(0);
}

NTSTATUS STDCALL NtCreatePagingFile(IN	PUNICODE_STRING	PageFileName,
				    IN	ULONG		MinimumSize,
				    IN	ULONG		MaximumSize,
				    OUT	PULONG		ActualSize)
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
   
   InitializeObjectAttributes(&ObjectAttributes,
			      PageFileName,
			      0,
			      NULL,
			      NULL);			      
   Status = NtCreateFile(&FileHandle,
			 FILE_ALL_ACCESS,
			 &ObjectAttributes,
			 &IoStatus,
			 NULL,
			 0,
			 0,
			 FILE_OPEN,
			 FILE_SYNCHRONOUS_IO_NONALERT,
			 NULL,
			 0);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_ALL_ACCESS,
				      IoFileType,
				      UserMode,
				      (PVOID*)&FileObject,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
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
   PagingFile->MaximumSize = PagingFile->CurrentSize = MinimumSize;
   PagingFile->FreePages = MinimumSize;
   PagingFile->UsedPages = 0;
   KeInitializeSpinLock(&PagingFile->AllocMapLock);
   
   AllocMapSize = (MinimumSize / 32) + 1;
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
   MiFreeSwapPages = MiFreeSwapPages + MinimumSize;
   KeReleaseSpinLock(&PagingFileListLock, oldIrql);
   
   return(STATUS_SUCCESS);
}
