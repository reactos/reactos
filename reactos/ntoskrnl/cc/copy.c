/* $Id: copy.c,v 1.22 2004/06/06 07:52:22 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/copy.c
 * PURPOSE:         Implements cache managers copy interface
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 *                  Created 05.10.2001
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

#if defined(__GNUC__)
static PHYSICAL_ADDRESS CcZeroPage = (PHYSICAL_ADDRESS)0LL;
#else
static PHYSICAL_ADDRESS CcZeroPage = { 0 };
#endif

#define MAX_ZERO_LENGTH	(256 * 1024)

/* FUNCTIONS *****************************************************************/

VOID 
CcInitCacheZeroPage(VOID)
{
   NTSTATUS Status;

   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &CcZeroPage);
   if (!NT_SUCCESS(Status))
   {
       DbgPrint("Can't allocate CcZeroPage.\n");
       KEBUGCHECK(0);
   }
   Status = MiZeroPage(CcZeroPage);
   if (!NT_SUCCESS(Status))
   {
       DbgPrint("Can't zero out CcZeroPage.\n");
       KEBUGCHECK(0);
   }
}

NTSTATUS
ReadCacheSegmentChain(PBCB Bcb, ULONG ReadOffset, ULONG Length,
		      PVOID Buffer)
{
  PCACHE_SEGMENT head;
  PCACHE_SEGMENT current;
  PCACHE_SEGMENT previous;
  IO_STATUS_BLOCK Iosb;
  LARGE_INTEGER SegOffset;
  NTSTATUS Status;
  ULONG TempLength;
  KEVENT Event;

  Status = CcRosGetCacheSegmentChain(Bcb, ReadOffset, Length, &head);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  current = head;
  while (current != NULL)
    {
      /*
       * If the current segment is valid then copy it into the
       * user buffer.
       */
      if (current->Valid)
	{
	  TempLength = min(Bcb->CacheSegmentSize, Length);
	  memcpy(Buffer, current->BaseAddress, TempLength);
#if defined(__GNUC__)
	  Buffer += TempLength;
#else
	  {
	    char* pTemp = Buffer;
		pTemp += TempLength;
		Buffer = pTemp;
	  }
#endif
	  Length = Length - TempLength; 
	  previous = current;
	  current = current->NextInChain;
	  CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
	}
      /*
       * Otherwise read in as much as we can.
       */
      else
	{
	  PCACHE_SEGMENT current2;
	  ULONG current_size;
	  PMDL Mdl;
	  ULONG i;
	  ULONG offset;

	  /*
	   * Count the maximum number of bytes we could read starting
	   * from the current segment.
	   */
	  current2 = current;
	  current_size = 0;
	  while (current2 != NULL && !current2->Valid)
	    {
	      current2 = current2->NextInChain;
	      current_size += Bcb->CacheSegmentSize;
	    }
	  
	  /*
	   * Create an MDL which contains all their pages.
	   */
	  Mdl = MmCreateMdl(NULL, NULL, current_size);
	  Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
	  current2 = current;
	  offset = 0;
	  while (current2 != NULL && !current2->Valid)
	    {
	      for (i = 0; i < (Bcb->CacheSegmentSize / PAGE_SIZE); i++)
		{
		  PVOID address;
		  PHYSICAL_ADDRESS page;
		  address = (char*)current2->BaseAddress + (i * PAGE_SIZE);
		  page = MmGetPhysicalAddressForProcess(NULL, address);
		  ((PULONG)(Mdl + 1))[offset] = page.QuadPart >> PAGE_SHIFT;
		  offset++;
		}
	      current2 = current2->NextInChain;
	    }

	  /*
	   * Read in the information.
	   */
	  SegOffset.QuadPart = current->FileOffset;
	  KeInitializeEvent(&Event, NotificationEvent, FALSE);
	  Status = IoPageRead(Bcb->FileObject,
			      Mdl,
			      &SegOffset,
			      &Event,
			      &Iosb);
	  if (Status == STATUS_PENDING)
	  {
	     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
	     Status = Iosb.Status;
	  }
	  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
	    {
	      while (current != NULL)
		{
		  previous = current;
		  current = current->NextInChain;
		  CcRosReleaseCacheSegment(Bcb, previous, FALSE, FALSE, FALSE);
		}
	      return(Status);
	    }
	  while (current != NULL && !current->Valid)
	    {
	      previous = current;
	      current = current->NextInChain;
	      TempLength = min(Bcb->CacheSegmentSize, Length);
	      memcpy(Buffer, previous->BaseAddress, TempLength);
#if defined(__GNUC__)
	      Buffer += TempLength;
#else
		  {
			char* pTemp = Buffer;
			pTemp += TempLength;
			Buffer = pTemp;
		  }
#endif
	      Length = Length - TempLength; 
	      CcRosReleaseCacheSegment(Bcb, previous, TRUE, FALSE, FALSE);
	    }
	}
    }
  return(STATUS_SUCCESS);
}

NTSTATUS 
ReadCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  LARGE_INTEGER SegOffset;
  IO_STATUS_BLOCK IoStatus;
  KEVENT Event;

  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = (ULONG)(CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset);
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
    {
      Size = CacheSeg->Bcb->CacheSegmentSize;
    }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Status = IoPageRead(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, & Event, &IoStatus); 
  if (Status == STATUS_PENDING)
  {
     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
     Status = IoStatus.Status;
  }

  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
    {
      DPRINT1("IoPageRead failed, Status %x\n", Status);
      return Status;
    }
  if (CacheSeg->Bcb->CacheSegmentSize > Size)
    {
      memset ((char*)CacheSeg->BaseAddress + Size, 0, 
	      CacheSeg->Bcb->CacheSegmentSize - Size);
    }
  return STATUS_SUCCESS;
}

NTSTATUS 
WriteCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatus;
  LARGE_INTEGER SegOffset;
  KEVENT Event;

  CacheSeg->Dirty = FALSE;
  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = (ULONG)(CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset);
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
    {
      Size = CacheSeg->Bcb->CacheSegmentSize;
    }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  Status = IoPageWrite(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &Event, &IoStatus);
  if (Status == STATUS_PENDING)
  {
     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
     Status = IoStatus.Status;
  }
  if (!NT_SUCCESS(Status) && (Status != STATUS_END_OF_FILE))
    {
      DPRINT1("IoPageWrite failed, Status %x\n", Status);
      CacheSeg->Dirty = TRUE;
      return(Status);
    }
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
CcCopyRead (IN PFILE_OBJECT FileObject,
	    IN PLARGE_INTEGER FileOffset,
	    IN ULONG Length,
	    IN BOOLEAN Wait,
	    OUT PVOID Buffer,
	    OUT PIO_STATUS_BLOCK IoStatus)
{
  ULONG ReadOffset;
  ULONG TempLength;
  NTSTATUS Status = STATUS_SUCCESS;
  PVOID BaseAddress;
  PCACHE_SEGMENT CacheSeg;
  BOOLEAN Valid;
  ULONG ReadLength = 0;
  PBCB Bcb;
  KIRQL oldirql;
  PLIST_ENTRY current_entry;
  PCACHE_SEGMENT current;
  
  DPRINT("CcCopyRead(FileObject %x, FileOffset %x, "
	 "Length %d, Wait %d, Buffer %x, IoStatus %x)\n",
	 FileObject, (ULONG)FileOffset->QuadPart, Length, Wait,
	 Buffer, IoStatus);

  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
  ReadOffset = (ULONG)FileOffset->QuadPart;
  
  DPRINT("AllocationSize %d, FileSize %d\n",
         (ULONG)Bcb->AllocationSize.QuadPart,
         (ULONG)Bcb->FileSize.QuadPart);

  /*
   * Check for the nowait case that all the cache segments that would
   * cover this read are in memory.
   */
  if (!Wait)
    {
      KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
      current_entry = Bcb->BcbSegmentListHead.Flink;
      while (current_entry != &Bcb->BcbSegmentListHead)
	{
	  current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
				      BcbSegmentListEntry);
	  if (!current->Valid && current->FileOffset < ReadOffset + Length
	      && current->FileOffset + Bcb->CacheSegmentSize > ReadOffset)
	    {
	      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	      IoStatus->Status = STATUS_UNSUCCESSFUL;
	      IoStatus->Information = 0;
	      return FALSE;
	    }
	  current_entry = current_entry->Flink;
	}
      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
    }

  TempLength = ReadOffset % Bcb->CacheSegmentSize;
  if (TempLength != 0)
    {
      TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
      Status = CcRosRequestCacheSegment(Bcb,
					ROUND_DOWN(ReadOffset, 
						   Bcb->CacheSegmentSize),
					&BaseAddress, &Valid, &CacheSeg);
      if (!NT_SUCCESS(Status))
	{
	  IoStatus->Information = 0;
	  IoStatus->Status = Status;
	  DPRINT("CcRosRequestCacheSegment faild, Status %x\n", Status);
	  return FALSE;
	}
      if (!Valid)
	{
	  Status = ReadCacheSegment(CacheSeg);
	  if (!NT_SUCCESS(Status))
	    {
	      IoStatus->Information = 0;
	      IoStatus->Status = Status;
              CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	      return FALSE;
	    }
	}
      memcpy (Buffer, (char*)BaseAddress + ReadOffset % Bcb->CacheSegmentSize, 
	      TempLength);
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
      ReadLength += TempLength;
      Length -= TempLength;
      ReadOffset += TempLength;
      Buffer = (PVOID)((char*)Buffer + TempLength);
    }  
  while (Length > 0)
    {
      TempLength = min(max(Bcb->CacheSegmentSize, 65536), Length);
      ReadCacheSegmentChain(Bcb, ReadOffset, TempLength, Buffer);
      ReadLength += TempLength;
      Length -= TempLength;
      ReadOffset += TempLength;
#if defined(__GNUC__)
      Buffer += TempLength;
#else
	  {
		char* pTemp = Buffer;
		pTemp += TempLength;
		Buffer = pTemp;
	  }
#endif
    }
  IoStatus->Status = STATUS_SUCCESS;
  IoStatus->Information = ReadLength;
  DPRINT("CcCopyRead O.K.\n");
  return TRUE;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
CcCopyWrite (IN PFILE_OBJECT FileObject,
	     IN PLARGE_INTEGER FileOffset,
	     IN ULONG Length,
	     IN BOOLEAN Wait,
	     IN PVOID Buffer)
{
   NTSTATUS Status;
   ULONG WriteOffset;
   KIRQL oldirql;
   PBCB Bcb;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT CacheSeg;
   ULONG TempLength;
   PVOID BaseAddress;
   BOOLEAN Valid;

   DPRINT("CcCopyWrite(FileObject %x, FileOffset %x, "
	  "Length %d, Wait %d, Buffer %x)\n",
          FileObject, (ULONG)FileOffset->QuadPart, Length, Wait, Buffer);

   Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
   WriteOffset = (ULONG)FileOffset->QuadPart;

   if (!Wait)
     {
       /* testing, if the requested datas are available */
       KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
       current_entry = Bcb->BcbSegmentListHead.Flink;
       while (current_entry != &Bcb->BcbSegmentListHead)
	 {
	   CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
					BcbSegmentListEntry);
	   if (!CacheSeg->Valid)
	     {
	       if ((WriteOffset >= CacheSeg->FileOffset && 
		    WriteOffset < CacheSeg->FileOffset + Bcb->CacheSegmentSize)
		   || (WriteOffset + Length > CacheSeg->FileOffset && 
		       WriteOffset + Length <= CacheSeg->FileOffset + 
		       Bcb->CacheSegmentSize))
		 {
		   KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
		   /* datas not available */
		   return(FALSE);
		 }
	     }
	   current_entry = current_entry->Flink;
	 }
       KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
     }

   TempLength = WriteOffset % Bcb->CacheSegmentSize;
   if (TempLength != 0)
     {
       ULONG ROffset;
       ROffset = ROUND_DOWN(WriteOffset, Bcb->CacheSegmentSize);
       TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
       Status = CcRosRequestCacheSegment(Bcb, ROffset,
					 &BaseAddress, &Valid, &CacheSeg);
       if (!NT_SUCCESS(Status))
	 {
	   return(FALSE);
	 }
       if (!Valid)
	 {
	   if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
	     {
	       return(FALSE);
	     }
	 }
       memcpy ((char*)BaseAddress + WriteOffset % Bcb->CacheSegmentSize, 
	       Buffer, TempLength);
       CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);
       
       Length -= TempLength;
       WriteOffset += TempLength;
#if defined(__GNUC__)
       Buffer += TempLength;
#else
	  {
		char* pTemp = Buffer;
		pTemp += TempLength;
		Buffer = pTemp;
	  }
#endif
     }
   
   while (Length > 0)
     {
       TempLength = min (Bcb->CacheSegmentSize, Length);
       Status = CcRosRequestCacheSegment(Bcb, WriteOffset,
					 &BaseAddress, &Valid, &CacheSeg);
       if (!NT_SUCCESS(Status))
	 {
	   return(FALSE);
	 }
       if (!Valid && TempLength < Bcb->CacheSegmentSize)
	 {
	   if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
	     {
               CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	       return FALSE;
	     }
	 }
       memcpy (BaseAddress, Buffer, TempLength);
       CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, TRUE, FALSE);
       Length -= TempLength;
       WriteOffset += TempLength;
#if defined(__GNUC__)
       Buffer += TempLength;
#else
	  {
		char* pTemp = Buffer;
		pTemp += TempLength;
		Buffer = pTemp;
	  }
#endif
     }
   return(TRUE);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
CcZeroData (IN PFILE_OBJECT     FileObject,
	    IN PLARGE_INTEGER   StartOffset,
	    IN PLARGE_INTEGER   EndOffset,
	    IN BOOLEAN          Wait)
{
  NTSTATUS Status;
  LARGE_INTEGER WriteOffset;
  ULONG Length;
  ULONG CurrentLength;
  PMDL Mdl;
  ULONG i;
  IO_STATUS_BLOCK Iosb;
  KEVENT Event;
  
  DPRINT("CcZeroData(FileObject %x, StartOffset %I64x, EndOffset %I64x, "
	 "Wait %d)\n", FileObject, StartOffset->QuadPart, EndOffset->QuadPart, 
	 Wait);
  
  Length = EndOffset->u.LowPart - StartOffset->u.LowPart;
  WriteOffset.QuadPart = StartOffset->QuadPart;

  if (FileObject->SectionObjectPointer->SharedCacheMap == NULL)
    {
      /* File is not cached */
 
      while (Length > 0)
	{
	  if (Length + WriteOffset.u.LowPart % PAGE_SIZE > MAX_ZERO_LENGTH)
	    {
	      CurrentLength = MAX_ZERO_LENGTH - WriteOffset.u.LowPart % PAGE_SIZE;
	    }
	  else
	    {
	      CurrentLength = Length;
	    }
          Mdl = MmCreateMdl(NULL, (PVOID)WriteOffset.u.LowPart, CurrentLength);

	  if (Mdl == NULL)
	    {
	      return(FALSE);
	    }
	  Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
	  for (i = 0; i < ((Mdl->Size - sizeof(MDL)) / sizeof(ULONG)); i++)
	    {
	      ((PULONG)(Mdl + 1))[i] = CcZeroPage.QuadPart >> PAGE_SHIFT;
	    }
          KeInitializeEvent(&Event, NotificationEvent, FALSE);
	  Status = IoPageWrite(FileObject, Mdl, &WriteOffset, &Event, &Iosb);
          if (Status == STATUS_PENDING)
	  {
             KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
             Status = Iosb.Status;
	  }
	  if (!NT_SUCCESS(Status))
	    {
	      return(FALSE);
	    }
	  WriteOffset.QuadPart += CurrentLength;
	  Length -= CurrentLength;
	}
    }
  else
    {
      /* File is cached */
      KIRQL oldirql;
      PBCB Bcb;
      PLIST_ENTRY current_entry;
      PCACHE_SEGMENT CacheSeg, current, previous;
      ULONG TempLength;

      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
      if (Wait)
	{
          /* testing, if the requested datas are available */
	  KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
	  current_entry = Bcb->BcbSegmentListHead.Flink;
	  while (current_entry != &Bcb->BcbSegmentListHead)
	    {
	      CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, 
					   BcbSegmentListEntry);
	      if (!CacheSeg->Valid)
		{
		  if ((WriteOffset.u.LowPart >= CacheSeg->FileOffset && 
		       WriteOffset.u.LowPart < CacheSeg->FileOffset + Bcb->CacheSegmentSize)
		      || (WriteOffset.u.LowPart + Length > CacheSeg->FileOffset && 
			  WriteOffset.u.LowPart + Length <= 
			  CacheSeg->FileOffset + Bcb->CacheSegmentSize))
		    {
		      KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
		      /* datas not available */
		      return(FALSE);
		    }
		}
	      current_entry = current_entry->Flink;
	    }
	  KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
	}
      while (Length > 0)
	{
          ULONG Offset;
	  Offset = WriteOffset.u.LowPart % Bcb->CacheSegmentSize;
	  if (Length + Offset > MAX_ZERO_LENGTH)
	    {
	      CurrentLength = MAX_ZERO_LENGTH - Offset;
	    }
	  else
	    {
	      CurrentLength = Length;
	    }
	  Status = CcRosGetCacheSegmentChain (Bcb, WriteOffset.u.LowPart - Offset,
					      Offset + CurrentLength, &CacheSeg);
	  if (!NT_SUCCESS(Status))
	    {
	      return FALSE;
	    }
	  current = CacheSeg;

	  while (current != NULL)
	    {
	      Offset = WriteOffset.u.LowPart % Bcb->CacheSegmentSize;
	      if (Offset != 0 ||
                  Offset + CurrentLength < Bcb->CacheSegmentSize)
	        {
		  if (!current->Valid)
		    {
		      /* read the segment */
		      Status = ReadCacheSegment(current);
		      if (!NT_SUCCESS(Status))
			{
			  DPRINT1("ReadCacheSegment failed, status %x\n", 
				  Status);
			}
		    }
		  TempLength = min (CurrentLength, Bcb->CacheSegmentSize - Offset);
		}
	      else
	        {
		  TempLength = Bcb->CacheSegmentSize;
		}
	      memset ((PUCHAR)current->BaseAddress + Offset, 0, TempLength);

              WriteOffset.QuadPart += TempLength;
	      CurrentLength -= TempLength;
	      Length -= TempLength;

      	      current = current->NextInChain;
	    } 

          current = CacheSeg;
	  while (current != NULL)
	    {
	      previous = current;
	      current = current->NextInChain;
	      CcRosReleaseCacheSegment(Bcb, previous, TRUE, TRUE, FALSE);
	    }
	}
    }
  return(TRUE);
}

