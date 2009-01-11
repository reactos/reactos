/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/finfo.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Herve Poussineau (reactos@poussine.freesurf.fr)
 *
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

const char* FileInformationClassNames[] =
{
  "??????",
  "FileDirectoryInformation",
  "FileFullDirectoryInformation",
  "FileBothDirectoryInformation",
  "FileBasicInformation",
  "FileStandardInformation",
  "FileInternalInformation",
  "FileEaInformation",
  "FileAccessInformation",
  "FileNameInformation",
  "FileRenameInformation",
  "FileLinkInformation",
  "FileNamesInformation",
  "FileDispositionInformation",
  "FilePositionInformation",
  "FileFullEaInformation",
  "FileModeInformation",
  "FileAlignmentInformation",
  "FileAllInformation",
  "FileAllocationInformation",
  "FileEndOfFileInformation",
  "FileAlternateNameInformation",
  "FileStreamInformation",
  "FilePipeInformation",
  "FilePipeLocalInformation",
  "FilePipeRemoteInformation",
  "FileMailslotQueryInformation",
  "FileMailslotSetInformation",
  "FileCompressionInformation",
  "FileObjectIdInformation",
  "FileCompletionInformation",
  "FileMoveClusterInformation",
  "FileQuotaInformation",
  "FileReparsePointInformation",
  "FileNetworkOpenInformation",
  "FileAttributeTagInformation",
  "FileTrackingInformation",
  "FileIdBothDirectoryInformation",
  "FileIdFullDirectoryInformation",
  "FileValidDataLengthInformation",
  "FileShortNameInformation",
  "FileMaximumInformation"
};

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatGetStandardInformation(PVFATFCB FCB,
			   PFILE_STANDARD_INFORMATION StandardInfo,
			   PULONG BufferLength)
/*
 * FUNCTION: Retrieve the standard file information
 */
{

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  /* PRECONDITION */
  ASSERT(StandardInfo != NULL);
  ASSERT(FCB != NULL);

  if (vfatFCBIsDirectory(FCB))
    {
      StandardInfo->AllocationSize.QuadPart = 0;
      StandardInfo->EndOfFile.QuadPart = 0;
      StandardInfo->Directory = TRUE;
    }
  else
    {
      StandardInfo->AllocationSize = FCB->RFCB.AllocationSize;
      StandardInfo->EndOfFile = FCB->RFCB.FileSize;
      StandardInfo->Directory = FALSE;
    }
  StandardInfo->NumberOfLinks = 1;
  StandardInfo->DeletePending = FCB->Flags & FCB_DELETE_PENDING ? TRUE : FALSE;

  *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatSetPositionInformation(PFILE_OBJECT FileObject,
			   PFILE_POSITION_INFORMATION PositionInfo)
{
  DPRINT ("FsdSetPositionInformation()\n");

  DPRINT ("PositionInfo %p\n", PositionInfo);
  DPRINT ("Setting position %d\n", PositionInfo->CurrentByteOffset.u.LowPart);

  FileObject->CurrentByteOffset.QuadPart =
    PositionInfo->CurrentByteOffset.QuadPart;

  return (STATUS_SUCCESS);
}

static NTSTATUS
VfatGetPositionInformation(PFILE_OBJECT FileObject,
			   PVFATFCB FCB,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_POSITION_INFORMATION PositionInfo,
			   PULONG BufferLength)
{
  DPRINT ("VfatGetPositionInformation()\n");

  if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  PositionInfo->CurrentByteOffset.QuadPart =
    FileObject->CurrentByteOffset.QuadPart;

  DPRINT("Getting position %I64x\n",
	 PositionInfo->CurrentByteOffset.QuadPart);

  *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatSetBasicInformation(PFILE_OBJECT FileObject,
			PVFATFCB FCB,
			PDEVICE_EXTENSION DeviceExt,
			PFILE_BASIC_INFORMATION BasicInfo)
{
  DPRINT("VfatSetBasicInformation()\n");

  ASSERT(NULL != FileObject);
  ASSERT(NULL != FCB);
  ASSERT(NULL != DeviceExt);
  ASSERT(NULL != BasicInfo);
  /* Check volume label bit */
  ASSERT(0 == (*FCB->Attributes & 0x08));

  if (FCB->Flags & FCB_IS_FATX_ENTRY)
  {
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->CreationTime,
                             &FCB->entry.FatX.CreationDate,
                             &FCB->entry.FatX.CreationTime);
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->LastAccessTime,
                             &FCB->entry.FatX.AccessDate,
                             &FCB->entry.FatX.AccessTime);
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->LastWriteTime,
                             &FCB->entry.FatX.UpdateDate,
                             &FCB->entry.FatX.UpdateTime);
  }
  else
  {
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->CreationTime,
                             &FCB->entry.Fat.CreationDate,
                             &FCB->entry.Fat.CreationTime);
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->LastAccessTime,
                             &FCB->entry.Fat.AccessDate,
                             NULL);
    FsdSystemTimeToDosDateTime(DeviceExt,
                             &BasicInfo->LastWriteTime,
                             &FCB->entry.Fat.UpdateDate,
                             &FCB->entry.Fat.UpdateTime);
  }

  *FCB->Attributes = (unsigned char)((*FCB->Attributes &
                       (FILE_ATTRIBUTE_DIRECTORY | 0x48)) |
                      (BasicInfo->FileAttributes &
                       (FILE_ATTRIBUTE_ARCHIVE |
                        FILE_ATTRIBUTE_SYSTEM |
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_READONLY)));
  DPRINT("Setting attributes 0x%02x\n", *FCB->Attributes);

  VfatUpdateEntry(FCB);

  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatGetBasicInformation(PFILE_OBJECT FileObject,
			PVFATFCB FCB,
			PDEVICE_OBJECT DeviceObject,
			PFILE_BASIC_INFORMATION BasicInfo,
			PULONG BufferLength)
{
  PDEVICE_EXTENSION DeviceExt;
  DPRINT("VfatGetBasicInformation()\n");

  DeviceExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  if (FCB->Flags & FCB_IS_FATX_ENTRY)
  {
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.FatX.CreationDate,
			     FCB->entry.FatX.CreationTime,
			     &BasicInfo->CreationTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.FatX.AccessDate,
			     FCB->entry.FatX.AccessTime,
			     &BasicInfo->LastAccessTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.FatX.UpdateDate,
			     FCB->entry.FatX.UpdateTime,
			     &BasicInfo->LastWriteTime);
    BasicInfo->ChangeTime = BasicInfo->LastWriteTime;
  }
  else
  {
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.Fat.CreationDate,
			     FCB->entry.Fat.CreationTime,
			     &BasicInfo->CreationTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.Fat.AccessDate,
			     0,
			     &BasicInfo->LastAccessTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     FCB->entry.Fat.UpdateDate,
			     FCB->entry.Fat.UpdateTime,
			     &BasicInfo->LastWriteTime);
    BasicInfo->ChangeTime = BasicInfo->LastWriteTime;
  }

  BasicInfo->FileAttributes = *FCB->Attributes & 0x3f;
  /* Synthesize FILE_ATTRIBUTE_NORMAL */
  if (0 == (BasicInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_ARCHIVE |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_READONLY)))
  {
    DPRINT("Synthesizing FILE_ATTRIBUTE_NORMAL\n");
    BasicInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
  }
  DPRINT("Getting attributes 0x%02x\n", BasicInfo->FileAttributes);

  *BufferLength -= sizeof(FILE_BASIC_INFORMATION);
  return(STATUS_SUCCESS);
}


static NTSTATUS
VfatSetDispositionInformation(PFILE_OBJECT FileObject,
			      PVFATFCB FCB,
			      PDEVICE_OBJECT DeviceObject,
			      PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
#ifdef DBG
   PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
#endif

   DPRINT ("FsdSetDispositionInformation(<%wZ>, Delete %d)\n", &FCB->PathNameU, DispositionInfo->DeleteFile);

   ASSERT(DeviceExt != NULL);
   ASSERT(DeviceExt->FatInfo.BytesPerCluster != 0);
   ASSERT(FCB != NULL);

   if (!DispositionInfo->DeleteFile)
   {
      /* undelete the file */
      FCB->Flags &= ~FCB_DELETE_PENDING;
      FileObject->DeletePending = FALSE;
      return STATUS_SUCCESS;
   }

   if (FCB->Flags & FCB_DELETE_PENDING)
   {
      /* stream already marked for deletion. just update the file object */
      FileObject->DeletePending = TRUE;
      return STATUS_SUCCESS;
   }

   if (*FCB->Attributes & FILE_ATTRIBUTE_READONLY)
   {
      return STATUS_CANNOT_DELETE;
   }

   if (vfatFCBIsRoot(FCB) ||
     (FCB->LongNameU.Length == sizeof(WCHAR) && FCB->LongNameU.Buffer[0] == L'.') ||
     (FCB->LongNameU.Length == 2 * sizeof(WCHAR) && FCB->LongNameU.Buffer[0] == L'.' && FCB->LongNameU.Buffer[1] == L'.'))
   {
      // we cannot delete a '.', '..' or the root directory
      return STATUS_ACCESS_DENIED;
   }


   if (!MmFlushImageSection (FileObject->SectionObjectPointer, MmFlushForDelete))
   {
      /* can't delete a file if its mapped into a process */

      DPRINT("MmFlushImageSection returned FALSE\n");
      return STATUS_CANNOT_DELETE;
   }

   if (vfatFCBIsDirectory(FCB) && !VfatIsDirectoryEmpty(FCB))
   {
      /* can't delete a non-empty directory */

      return STATUS_DIRECTORY_NOT_EMPTY;
   }

   /* all good */
   FCB->Flags |= FCB_DELETE_PENDING;
   FileObject->DeletePending = TRUE;

   return STATUS_SUCCESS;
}

static NTSTATUS
VfatGetNameInformation(PFILE_OBJECT FileObject,
		       PVFATFCB FCB,
		       PDEVICE_OBJECT DeviceObject,
		       PFILE_NAME_INFORMATION NameInfo,
		       PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file name information
 */
{
  ULONG BytesToCopy;
  ASSERT(NameInfo != NULL);
  ASSERT(FCB != NULL);

  /* If buffer can't hold at least the file name length, bail out */
  if (*BufferLength < FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
    return STATUS_BUFFER_OVERFLOW;

  /* Save file name length, and as much file len, as buffer length allows */
  NameInfo->FileNameLength = FCB->PathNameU.Length;

  /* Calculate amount of bytes to copy not to overflow the buffer */
  BytesToCopy = min(FCB->PathNameU.Length,
                    *BufferLength - FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]));

  /* Fill in the bytes */
  RtlCopyMemory(NameInfo->FileName, FCB->PathNameU.Buffer, BytesToCopy);

  /* Check if we could write more but are not able to */
  if (*BufferLength < FCB->PathNameU.Length + (ULONG)FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]))
  {
    /* Return number of bytes written */
    *BufferLength -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + BytesToCopy;
    return STATUS_BUFFER_OVERFLOW;
  }

  /* We filled up as many bytes, as needed */
  *BufferLength -= (FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]) + FCB->PathNameU.Length);

  return STATUS_SUCCESS;
}

static NTSTATUS
VfatGetInternalInformation(PVFATFCB Fcb,
			   PFILE_INTERNAL_INFORMATION InternalInfo,
			   PULONG BufferLength)
{
  ASSERT(InternalInfo);
  ASSERT(Fcb);

  if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;
  // FIXME: get a real index, that can be used in a create operation
  InternalInfo->IndexNumber.QuadPart = 0;
  *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);
  return STATUS_SUCCESS;
}


static NTSTATUS
VfatGetNetworkOpenInformation(PVFATFCB Fcb,
			      PDEVICE_EXTENSION DeviceExt,
			      PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
			      PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file network open information
 */
{
  ASSERT(NetworkInfo);
  ASSERT(Fcb);

  if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  if (Fcb->Flags & FCB_IS_FATX_ENTRY)
  {
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.FatX.CreationDate,
			     Fcb->entry.FatX.CreationTime,
			     &NetworkInfo->CreationTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.FatX.AccessDate,
			     Fcb->entry.FatX.AccessTime,
			     &NetworkInfo->LastAccessTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.FatX.UpdateDate,
			     Fcb->entry.FatX.UpdateTime,
			     &NetworkInfo->LastWriteTime);
    NetworkInfo->ChangeTime.QuadPart = NetworkInfo->LastWriteTime.QuadPart;
  }
  else
  {
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.Fat.CreationDate,
			     Fcb->entry.Fat.CreationTime,
			     &NetworkInfo->CreationTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.Fat.AccessDate,
			     0,
			     &NetworkInfo->LastAccessTime);
    FsdDosDateTimeToSystemTime(DeviceExt,
			     Fcb->entry.Fat.UpdateDate,
			     Fcb->entry.Fat.UpdateTime,
			     &NetworkInfo->LastWriteTime);
    NetworkInfo->ChangeTime.QuadPart = NetworkInfo->LastWriteTime.QuadPart;
  }
  if (vfatFCBIsDirectory(Fcb))
    {
      NetworkInfo->EndOfFile.QuadPart = 0L;
      NetworkInfo->AllocationSize.QuadPart = 0L;
    }
  else
    {
      NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;
      NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    }
  NetworkInfo->FileAttributes = *Fcb->Attributes & 0x3f;
  /* Synthesize FILE_ATTRIBUTE_NORMAL */
  if (0 == (NetworkInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                           FILE_ATTRIBUTE_ARCHIVE |
                                           FILE_ATTRIBUTE_SYSTEM |
                                           FILE_ATTRIBUTE_HIDDEN |
                                           FILE_ATTRIBUTE_READONLY)))
  {
    DPRINT("Synthesizing FILE_ATTRIBUTE_NORMAL\n");
    NetworkInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
  }

  *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
  return STATUS_SUCCESS;
}


static NTSTATUS
VfatGetEaInformation(PFILE_OBJECT FileObject,
		     PVFATFCB Fcb,
		     PDEVICE_OBJECT DeviceObject,
		     PFILE_EA_INFORMATION Info,
		     PULONG BufferLength)
{
    PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

    /* FIXME - use SEH to access the buffer! */
    Info->EaSize = 0;
    *BufferLength -= sizeof(*Info);
    if (DeviceExt->FatInfo.FatType == FAT12 ||
        DeviceExt->FatInfo.FatType == FAT16)
    {
        /* FIXME */
        DPRINT1("VFAT: FileEaInformation not implemented!\n");
    }
    return STATUS_SUCCESS;
}


static NTSTATUS
VfatGetAllInformation(PFILE_OBJECT FileObject,
		      PVFATFCB Fcb,
		      PDEVICE_OBJECT DeviceObject,
		      PFILE_ALL_INFORMATION Info,
		      PULONG BufferLength)
/*
 * FUNCTION: Retrieve the all file information
 */
{
  NTSTATUS Status;
  ULONG InitialBufferLength = *BufferLength;

  ASSERT(Info);
  ASSERT(Fcb);

  if (*BufferLength < sizeof(FILE_ALL_INFORMATION) + Fcb->PathNameU.Length + sizeof(WCHAR))
    return(STATUS_BUFFER_OVERFLOW);

  /* Basic Information */
  Status = VfatGetBasicInformation(FileObject, Fcb, DeviceObject, &Info->BasicInformation, BufferLength);
  if (!NT_SUCCESS(Status)) return Status;
  /* Standard Information */
  Status = VfatGetStandardInformation(Fcb, &Info->StandardInformation, BufferLength);
  if (!NT_SUCCESS(Status)) return Status;
  /* Internal Information */
  Status = VfatGetInternalInformation(Fcb, &Info->InternalInformation, BufferLength);
  if (!NT_SUCCESS(Status)) return Status;
  /* EA Information */
  Info->EaInformation.EaSize = 0;
  /* Access Information: The IO-Manager adds this information */
  /* Position Information */
  Status = VfatGetPositionInformation(FileObject, Fcb, DeviceObject, &Info->PositionInformation, BufferLength);
  if (!NT_SUCCESS(Status)) return Status;
  /* Mode Information: The IO-Manager adds this information */
  /* Alignment Information: The IO-Manager adds this information */
  /* Name Information */
  Status = VfatGetNameInformation(FileObject, Fcb, DeviceObject, &Info->NameInformation, BufferLength);
  if (!NT_SUCCESS(Status)) return Status;

  *BufferLength = InitialBufferLength - (sizeof(FILE_ALL_INFORMATION) + Fcb->PathNameU.Length + sizeof(WCHAR));

  return STATUS_SUCCESS;
}

static VOID UpdateFileSize(PFILE_OBJECT FileObject, PVFATFCB Fcb, ULONG Size, ULONG ClusterSize)
{
   if (Size > 0)
   {
      Fcb->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, ClusterSize);
   }
   else
   {
      Fcb->RFCB.AllocationSize.QuadPart = (LONGLONG)0;
   }
   if (!vfatFCBIsDirectory(Fcb))
   {
      if (Fcb->Flags & FCB_IS_FATX_ENTRY)
         Fcb->entry.FatX.FileSize = Size;
      else
         Fcb->entry.Fat.FileSize = Size;
   }
   Fcb->RFCB.FileSize.QuadPart = Size;
   Fcb->RFCB.ValidDataLength.QuadPart = Size;

   if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
   {
      CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
   }
}

NTSTATUS
VfatSetAllocationSizeInformation(PFILE_OBJECT FileObject,
				 PVFATFCB Fcb,
				 PDEVICE_EXTENSION DeviceExt,
				 PLARGE_INTEGER AllocationSize)
{
  ULONG OldSize;
  ULONG Cluster, FirstCluster;
  NTSTATUS Status;

  ULONG ClusterSize = DeviceExt->FatInfo.BytesPerCluster;
  ULONG NewSize = AllocationSize->u.LowPart;
  ULONG NCluster;
  BOOLEAN AllocSizeChanged = FALSE;

  DPRINT("VfatSetAllocationSizeInformation(File <%wZ>, AllocationSize %d %d)\n", &Fcb->PathNameU,
      AllocationSize->HighPart, AllocationSize->LowPart);

  if (Fcb->Flags & FCB_IS_FATX_ENTRY)
    OldSize = Fcb->entry.FatX.FileSize;
  else
    OldSize = Fcb->entry.Fat.FileSize;
  if (AllocationSize->u.HighPart > 0)
  {
    return STATUS_INVALID_PARAMETER;
  }
  if (OldSize == NewSize)
    {
      return(STATUS_SUCCESS);
    }

  FirstCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

  if (NewSize > Fcb->RFCB.AllocationSize.u.LowPart)
  {
    AllocSizeChanged = TRUE;
    if (FirstCluster == 0)
    {
      Fcb->LastCluster = Fcb->LastOffset = 0;
      Status = NextCluster (DeviceExt, FirstCluster, &FirstCluster, TRUE);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("NextCluster failed. Status = %x\n", Status);
	return Status;
      }
      if (FirstCluster == 0xffffffff)
      {
         return STATUS_DISK_FULL;
      }
      Status = OffsetToCluster(DeviceExt, FirstCluster,
                               ROUND_DOWN(NewSize - 1, ClusterSize),
                               &NCluster, TRUE);
      if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
      {
         /* disk is full */
         NCluster = Cluster = FirstCluster;
         Status = STATUS_SUCCESS;
         while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
         {
            Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster (DeviceExt, Cluster, 0);
            Cluster = NCluster;
         }
         return STATUS_DISK_FULL;
      }
      if (Fcb->Flags & FCB_IS_FATX_ENTRY)
      {
         Fcb->entry.FatX.FirstCluster = FirstCluster;
      }
      else
      {
        if (DeviceExt->FatInfo.FatType == FAT32)
        {
          Fcb->entry.Fat.FirstCluster = (unsigned short)(FirstCluster & 0x0000FFFF);
          Fcb->entry.Fat.FirstClusterHigh = FirstCluster >> 16;
        }
        else
        {
            ASSERT((FirstCluster >> 16) == 0);
            Fcb->entry.Fat.FirstCluster = (unsigned short)(FirstCluster & 0x0000FFFF);
        }
      }
    }
    else
    {
       if (Fcb->LastCluster > 0)
       {
          if (Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize == Fcb->LastOffset)
          {
             Cluster = Fcb->LastCluster;
             Status = STATUS_SUCCESS;
          }
          else
          {
             Status = OffsetToCluster(DeviceExt, Fcb->LastCluster,
                                      Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize - Fcb->LastOffset,
                                      &Cluster, FALSE);
          }
       }
       else
       {
          Status = OffsetToCluster(DeviceExt, FirstCluster,
                                   Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize,
                                   &Cluster, FALSE);
       }
       if (!NT_SUCCESS(Status))
       {
          return Status;
       }

       Fcb->LastCluster = Cluster;
       Fcb->LastOffset = Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize;

       /* FIXME: Check status */
       /* Cluster points now to the last cluster within the chain */
       Status = OffsetToCluster(DeviceExt, Cluster,
	                        ROUND_DOWN(NewSize - 1, ClusterSize) - Fcb->LastOffset,
                                &NCluster, TRUE);
       if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
       {
	  /* disk is full */
	  NCluster = Cluster;
          Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
	  WriteCluster(DeviceExt, Cluster, 0xffffffff);
	  Cluster = NCluster;
          while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
	  {
	    Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster (DeviceExt, Cluster, 0);
	    Cluster = NCluster;
	  }
	  return STATUS_DISK_FULL;
       }
    }
    UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
  }
  else if (NewSize + ClusterSize <= Fcb->RFCB.AllocationSize.u.LowPart)
  {
    AllocSizeChanged = TRUE;
    /* FIXME: Use the cached cluster/offset better way. */
    Fcb->LastCluster = Fcb->LastOffset = 0;
    UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
    if (NewSize > 0)
    {
      Status = OffsetToCluster(DeviceExt, FirstCluster,
	          ROUND_DOWN(NewSize - 1, ClusterSize),
		  &Cluster, FALSE);

      NCluster = Cluster;
      Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
      WriteCluster(DeviceExt, Cluster, 0xffffffff);
      Cluster = NCluster;
    }
    else
    {
      if (Fcb->Flags & FCB_IS_FATX_ENTRY)
      {
         Fcb->entry.FatX.FirstCluster = 0;
      }
      else
      {
        if (DeviceExt->FatInfo.FatType == FAT32)
        {
          Fcb->entry.Fat.FirstCluster = 0;
          Fcb->entry.Fat.FirstClusterHigh = 0;
        }
        else
        {
            Fcb->entry.Fat.FirstCluster = 0;
        }
      }

      NCluster = Cluster = FirstCluster;
      Status = STATUS_SUCCESS;
    }
    while (NT_SUCCESS(Status) && 0xffffffff != Cluster && Cluster > 1)
    {
       Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
       WriteCluster (DeviceExt, Cluster, 0);
       Cluster = NCluster;
    }
  }
  else
  {
     UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
  }
  /* Update the on-disk directory entry */
  Fcb->Flags |= FCB_IS_DIRTY;
  if (AllocSizeChanged)
    {
      VfatUpdateEntry(Fcb);
    }
  return STATUS_SUCCESS;
}

NTSTATUS VfatQueryInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;

  NTSTATUS Status = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  ASSERT(IrpContext);

  /* INITIALIZATION */
  FileInformationClass = IrpContext->Stack->Parameters.QueryFile.FileInformationClass;
  FCB = (PVFATFCB) IrpContext->FileObject->FsContext;

  DPRINT("VfatQueryInformation is called for '%s'\n",
         FileInformationClass >= FileMaximumInformation - 1 ? "????" : FileInformationClassNames[FileInformationClass]);


  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;
  BufferLength = IrpContext->Stack->Parameters.QueryFile.Length;

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     if (!ExAcquireResourceSharedLite(&FCB->MainResource,
                                      (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
     {
        return VfatQueueRequest (IrpContext);
     }
  }


  switch (FileInformationClass)
    {
    case FileStandardInformation:
      Status = VfatGetStandardInformation(FCB,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FilePositionInformation:
      Status = VfatGetPositionInformation(IrpContext->FileObject,
				      FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileBasicInformation:
      Status = VfatGetBasicInformation(IrpContext->FileObject,
				   FCB,
				   IrpContext->DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;
    case FileNameInformation:
      Status = VfatGetNameInformation(IrpContext->FileObject,
				  FCB,
				  IrpContext->DeviceObject,
				  SystemBuffer,
				  &BufferLength);
      break;
    case FileInternalInformation:
      Status = VfatGetInternalInformation(FCB,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileNetworkOpenInformation:
      Status = VfatGetNetworkOpenInformation(FCB,
					 IrpContext->DeviceExt,
					 SystemBuffer,
					 &BufferLength);
      break;
    case FileAllInformation:
      Status = VfatGetAllInformation(IrpContext->FileObject,
				 FCB,
				 IrpContext->DeviceObject,
				 SystemBuffer,
				 &BufferLength);
      break;

    case FileEaInformation:
      Status = VfatGetEaInformation(IrpContext->FileObject,
				FCB,
				IrpContext->DeviceObject,
				SystemBuffer,
				&BufferLength);
      break;

    case FileAlternateNameInformation:
      Status = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      Status = STATUS_INVALID_PARAMETER;
    }

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }
  IrpContext->Irp->IoStatus.Status = Status;
  if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
    IrpContext->Irp->IoStatus.Information =
      IrpContext->Stack->Parameters.QueryFile.Length - BufferLength;
  else
    IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return Status;
}

NTSTATUS VfatSetInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  BOOLEAN CanWait = (IrpContext->Flags & IRPCONTEXT_CANWAIT) != 0;

  /* PRECONDITION */
  ASSERT(IrpContext);

  DPRINT("VfatSetInformation(IrpContext %p)\n", IrpContext);

  /* INITIALIZATION */
  FileInformationClass =
    IrpContext->Stack->Parameters.SetFile.FileInformationClass;
  FCB = (PVFATFCB) IrpContext->FileObject->FsContext;
  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;

  DPRINT("VfatSetInformation is called for '%s'\n",
         FileInformationClass >= FileMaximumInformation - 1 ? "????" : FileInformationClassNames[ FileInformationClass]);

  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %p\n", SystemBuffer);

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
      if (!ExAcquireResourceExclusiveLite(&FCB->MainResource,
                                          (BOOLEAN)CanWait))
	{
	  return(VfatQueueRequest (IrpContext));
	}
    }

  switch (FileInformationClass)
    {
    case FilePositionInformation:
      RC = VfatSetPositionInformation(IrpContext->FileObject,
				      SystemBuffer);
      break;
    case FileDispositionInformation:
      RC = VfatSetDispositionInformation(IrpContext->FileObject,
					 FCB,
					 IrpContext->DeviceObject,
					 SystemBuffer);
      break;
    case FileAllocationInformation:
    case FileEndOfFileInformation:
      RC = VfatSetAllocationSizeInformation(IrpContext->FileObject,
					    FCB,
					    IrpContext->DeviceExt,
					    (PLARGE_INTEGER)SystemBuffer);
      break;
    case FileBasicInformation:
      RC = VfatSetBasicInformation(IrpContext->FileObject,
				   FCB,
				   IrpContext->DeviceExt,
				   SystemBuffer);
      break;
    case FileRenameInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }

  IrpContext->Irp->IoStatus.Status = RC;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}

/* EOF */
