/*
 * $Id: dir.c,v 1.34 2004/05/23 13:34:32 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/dir.c
 * PURPOSE:          VFAT Filesystem : directory control
 * UPDATE HISTORY:
     19-12-1998 : created

*/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"


// function like DosDateTimeToFileTime
BOOL FsdDosDateTimeToFileTime (WORD wDosDate, WORD wDosTime, TIME * FileTime)
{
  PDOSTIME pdtime = (PDOSTIME) & wDosTime;
  PDOSDATE pddate = (PDOSDATE) & wDosDate;
  TIME_FIELDS TimeFields;

  if (FileTime == NULL)
    return FALSE;

  TimeFields.Milliseconds = 0;
  TimeFields.Second = pdtime->Second * 2;
  TimeFields.Minute = pdtime->Minute;
  TimeFields.Hour = pdtime->Hour;

  TimeFields.Day = pddate->Day;
  TimeFields.Month = pddate->Month;
  TimeFields.Year = 1980 + pddate->Year;

  RtlTimeFieldsToTime (&TimeFields, (PLARGE_INTEGER) FileTime);

  return TRUE;
}


// function like FileTimeToDosDateTime
BOOL
FsdFileTimeToDosDateTime (TIME * FileTime, WORD * pwDosDate, WORD * pwDosTime)
{
  PDOSTIME pdtime = (PDOSTIME) pwDosTime;
  PDOSDATE pddate = (PDOSDATE) pwDosDate;
  TIME_FIELDS TimeFields;

  if (FileTime == NULL)
    return FALSE;

  RtlTimeToTimeFields ((PLARGE_INTEGER) FileTime, &TimeFields);

  if (pdtime)
    {
      pdtime->Second = TimeFields.Second / 2;
      pdtime->Minute = TimeFields.Minute;
      pdtime->Hour = TimeFields.Hour;
    }

  if (pddate)
    {
      pddate->Day = TimeFields.Day;
      pddate->Month = TimeFields.Month;
      pddate->Year = TimeFields.Year - 1980;
    }

  return TRUE;
}


#define DWORD_ROUND_UP(x)   ROUND_UP((x), (sizeof(DWORD)))

NTSTATUS
VfatGetFileNameInformation (PVFAT_DIRENTRY_CONTEXT DirContext,
			    PFILE_NAMES_INFORMATION pInfo, ULONG BufferLength)
{
  if ((sizeof (FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = DirContext->LongNameU.Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length);
  memcpy (pInfo->FileName, DirContext->LongNameU.Buffer, DirContext->LongNameU.Length);
  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileDirectoryInformation (PVFAT_DIRENTRY_CONTEXT DirContext,
				 PDEVICE_EXTENSION DeviceExt,
				 PFILE_DIRECTORY_INFORMATION pInfo,
				 ULONG BufferLength)
{
  if ((sizeof (FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = DirContext->LongNameU.Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length);
  memcpy (pInfo->FileName, DirContext->LongNameU.Buffer, DirContext->LongNameU.Length);
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.CreationDate,
			    DirContext->FatDirEntry.CreationTime, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.AccessDate, 0,
			    &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.UpdateDate, 
                            DirContext->FatDirEntry.UpdateTime, &pInfo->LastWriteTime);
  pInfo->ChangeTime = pInfo->LastWriteTime;
  if (DirContext->FatDirEntry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      pInfo->EndOfFile.QuadPart = 0LL;
      pInfo->AllocationSize.QuadPart = 0LL;
    }
  else
    {
      pInfo->EndOfFile.u.HighPart = 0;
      pInfo->EndOfFile.u.LowPart = DirContext->FatDirEntry.FileSize;
      /* Make allocsize a rounded up multiple of BytesPerCluster */
      pInfo->AllocationSize.u.HighPart = 0;
      pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->FatDirEntry.FileSize, DeviceExt->FatInfo.BytesPerCluster);
    }
  pInfo->FileAttributes = DirContext->FatDirEntry.Attrib & 0x3f;

  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileFullDirectoryInformation (PVFAT_DIRENTRY_CONTEXT DirContext,
				     PDEVICE_EXTENSION DeviceExt,
				     PFILE_FULL_DIRECTORY_INFORMATION pInfo,
				     ULONG BufferLength)
{
  if ((sizeof (FILE_FULL_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = DirContext->LongNameU.Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_FULL_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length);
  memcpy (pInfo->FileName, DirContext->LongNameU.Buffer, DirContext->LongNameU.Length);
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.CreationDate,
			    DirContext->FatDirEntry.CreationTime, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.AccessDate, 
                            0, &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.UpdateDate, 
                            DirContext->FatDirEntry.UpdateTime, &pInfo->LastWriteTime);
  pInfo->ChangeTime = pInfo->LastWriteTime;
  pInfo->EndOfFile.u.HighPart = 0;
  pInfo->EndOfFile.u.LowPart = DirContext->FatDirEntry.FileSize;
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  pInfo->AllocationSize.u.HighPart = 0;
  pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->FatDirEntry.FileSize, DeviceExt->FatInfo.BytesPerCluster);
  pInfo->FileAttributes = DirContext->FatDirEntry.Attrib & 0x3f;
//      pInfo->EaSize=;
  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileBothInformation (PVFAT_DIRENTRY_CONTEXT DirContext,
			    PDEVICE_EXTENSION DeviceExt,
			    PFILE_BOTH_DIRECTORY_INFORMATION pInfo,
			    ULONG BufferLength)
{
  if ((sizeof (FILE_BOTH_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = DirContext->LongNameU.Length;
  pInfo->NextEntryOffset = 
    DWORD_ROUND_UP (sizeof (FILE_BOTH_DIRECTORY_INFORMATION) + DirContext->LongNameU.Length);
  memcpy(pInfo->ShortName, DirContext->ShortNameU.Buffer, DirContext->ShortNameU.Length);
  pInfo->ShortNameLength = DirContext->ShortNameU.Length;
  memcpy (pInfo->FileName, DirContext->LongNameU.Buffer, DirContext->LongNameU.Length);
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.CreationDate,
			    DirContext->FatDirEntry.CreationDate, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.AccessDate, 0,
			    &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (DirContext->FatDirEntry.UpdateDate, 
                            DirContext->FatDirEntry.UpdateTime, &pInfo->LastWriteTime);
  pInfo->ChangeTime = pInfo->LastWriteTime;
  if (DirContext->FatDirEntry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
    {
      pInfo->EndOfFile.QuadPart = 0LL;
      pInfo->AllocationSize.QuadPart = 0LL;
    }
  else
    {
      pInfo->EndOfFile.u.HighPart = 0;
      pInfo->EndOfFile.u.LowPart = DirContext->FatDirEntry.FileSize;
      /* Make allocsize a rounded up multiple of BytesPerCluster */
      pInfo->AllocationSize.u.HighPart = 0;
      pInfo->AllocationSize.u.LowPart = ROUND_UP(DirContext->FatDirEntry.FileSize, DeviceExt->FatInfo.BytesPerCluster);
    }
  pInfo->FileAttributes = DirContext->FatDirEntry.Attrib & 0x3f;
  pInfo->EaSize=0;
  return STATUS_SUCCESS;
}

NTSTATUS DoQuery (PVFAT_IRP_CONTEXT IrpContext)
{
  NTSTATUS RC = STATUS_SUCCESS;
  long BufferLength = 0;
  PUNICODE_STRING pSearchPattern = NULL;
  FILE_INFORMATION_CLASS FileInformationClass;
  unsigned long FileIndex = 0;
  unsigned char *Buffer = NULL;
  PFILE_NAMES_INFORMATION Buffer0 = NULL;
  PVFATFCB pFcb;
  PVFATCCB pCcb;
  BOOLEAN First = FALSE;
  BOOLEAN FirstCall;
  VFAT_DIRENTRY_CONTEXT DirContext;
  WCHAR LongNameBuffer[MAX_PATH];
  WCHAR ShortNameBuffer[13];
  
  PEXTENDED_IO_STACK_LOCATION Stack = (PEXTENDED_IO_STACK_LOCATION) IrpContext->Stack;

  pCcb = (PVFATCCB) IrpContext->FileObject->FsContext2;
  pFcb = (PVFATFCB) IrpContext->FileObject->FsContext;

  // determine Buffer for result :
  BufferLength = Stack->Parameters.QueryDirectory.Length;
#if 0
  /* Do not probe the user buffer until SEH is available */
  if (IrpContext->Irp->RequestorMode != KernelMode &&
      IrpContext->Irp->MdlAddress == NULL && 
      IrpContext->Irp->UserBuffer != NULL)
    {
      ProbeForWrite(IrpContext->Irp->UserBuffer, BufferLength, 1);
    }
#endif
  Buffer = VfatGetUserBuffer(IrpContext->Irp);

  if (!ExAcquireResourceSharedLite(&pFcb->MainResource,
                                   (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
    {
      RC = VfatLockUserBuffer(IrpContext->Irp, BufferLength, IoWriteAccess);
      if (NT_SUCCESS(RC))
        {
          RC = STATUS_PENDING;
        }
      return RC;
    }

  /* Obtain the callers parameters */
  pSearchPattern = Stack->Parameters.QueryDirectory.FileName;
  FileInformationClass =
    Stack->Parameters.QueryDirectory.FileInformationClass;
  FileIndex = Stack->Parameters.QueryDirectory.FileIndex;
  if (pSearchPattern)
    {
      if (!pCcb->SearchPattern.Buffer)
        {
          First = TRUE;
          pCcb->SearchPattern.MaximumLength = pSearchPattern->Length + sizeof(WCHAR);
          pCcb->SearchPattern.Buffer = ExAllocatePool(NonPagedPool, pCcb->SearchPattern.MaximumLength);
          if (!pCcb->SearchPattern.Buffer)
            {
              ExReleaseResourceLite(&pFcb->MainResource);
              return STATUS_INSUFFICIENT_RESOURCES;
            }
          RtlCopyUnicodeString(&pCcb->SearchPattern, pSearchPattern);
          pCcb->SearchPattern.Buffer[pCcb->SearchPattern.Length / sizeof(WCHAR)] = 0;
	}
    }
  else if (!pCcb->SearchPattern.Buffer)
    {
      First = TRUE;
      pCcb->SearchPattern.MaximumLength = 2 * sizeof(WCHAR);
      pCcb->SearchPattern.Buffer = ExAllocatePool(NonPagedPool, 2 * sizeof(WCHAR));
      if (!pCcb->SearchPattern.Buffer)
        {
          ExReleaseResourceLite(&pFcb->MainResource);
          return STATUS_INSUFFICIENT_RESOURCES;
        }
      pCcb->SearchPattern.Buffer[0] = L'*';
      pCcb->SearchPattern.Buffer[1] = 0;
      pCcb->SearchPattern.Length = sizeof(WCHAR);
    }

  if (IrpContext->Stack->Flags & SL_INDEX_SPECIFIED)
    {
      DirContext.DirIndex = pCcb->Entry = pCcb->CurrentByteOffset.u.LowPart;
      FirstCall = TRUE;
    }
  else if (First || (IrpContext->Stack->Flags & SL_RESTART_SCAN))
    {
      DirContext.DirIndex = pCcb->Entry = 0;
      FirstCall = TRUE;
    }
  else
    {
      DirContext.DirIndex = pCcb->Entry;
      FirstCall = FALSE;
    }

  DPRINT ("Buffer=%x tofind=%wZ\n", Buffer, &pCcb->SearchPattern);

  DirContext.LongNameU.Buffer = LongNameBuffer;
  DirContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
  DirContext.ShortNameU.Buffer = ShortNameBuffer;
  DirContext.ShortNameU.MaximumLength = sizeof(ShortNameBuffer);

  while (RC == STATUS_SUCCESS && BufferLength > 0)
    {
      RC = FindFile (IrpContext->DeviceExt, pFcb, 
                     &pCcb->SearchPattern, &DirContext, FirstCall);
      pCcb->Entry = DirContext.DirIndex;
      DPRINT ("Found %wZ, RC=%x, entry %x\n", &DirContext.LongNameU, RC, pCcb->Entry);
      FirstCall = FALSE;
      if (NT_SUCCESS (RC))
        {
          switch (FileInformationClass)
            {
              case FileNameInformation:
                RC = VfatGetFileNameInformation (&DirContext,
                                                 (PFILE_NAMES_INFORMATION) Buffer, 
					         BufferLength);
                break;
              case FileDirectoryInformation:
                RC = VfatGetFileDirectoryInformation (&DirContext, 
	                                              IrpContext->DeviceExt,
						      (PFILE_DIRECTORY_INFORMATION) Buffer, 
						      BufferLength);
                break;
             case FileFullDirectoryInformation:
               RC = VfatGetFileFullDirectoryInformation (&DirContext, 
	                                                 IrpContext->DeviceExt,
						         (PFILE_FULL_DIRECTORY_INFORMATION) Buffer, 
						         BufferLength);
               break;
             case FileBothDirectoryInformation:
               RC = VfatGetFileBothInformation (&DirContext, 
	                                        IrpContext->DeviceExt,
					        (PFILE_BOTH_DIRECTORY_INFORMATION) Buffer, 
					        BufferLength);
               break;
             default:
               RC = STATUS_INVALID_INFO_CLASS;
	    }
          if (RC == STATUS_BUFFER_OVERFLOW)
            {
              if (Buffer0)
                {
                  Buffer0->NextEntryOffset = 0;
                }
              break;
            }
	}
      else
        {
          if (Buffer0)
            {
              Buffer0->NextEntryOffset = 0;
            }
          if (First)
            {
              RC = STATUS_NO_SUCH_FILE;
            }
          else
            {
              RC = STATUS_NO_MORE_FILES;
            }
          break;
	}
      Buffer0 = (PFILE_NAMES_INFORMATION) Buffer;
      Buffer0->FileIndex = FileIndex++;
      pCcb->Entry = ++DirContext.DirIndex;
      if (IrpContext->Stack->Flags & SL_RETURN_SINGLE_ENTRY)
        {
          break;
        }
      BufferLength -= Buffer0->NextEntryOffset;
      Buffer += Buffer0->NextEntryOffset;
    }
  if (Buffer0)
    {
      Buffer0->NextEntryOffset = 0;
    }
  if (FileIndex > 0)
    {
      RC = STATUS_SUCCESS;
    }
  ExReleaseResourceLite(&pFcb->MainResource);
  return RC;
}


NTSTATUS VfatDirectoryControl (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: directory control : read/write directory informations
 */
{
  NTSTATUS RC = STATUS_SUCCESS;
  CHECKPOINT;
  switch (IrpContext->MinorFunction)
    {
    case IRP_MN_QUERY_DIRECTORY:
      RC = DoQuery (IrpContext);
      break;
    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
      DPRINT (" vfat, dir : change\n");
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      // error
      DbgPrint ("unexpected minor function %x in VFAT driver\n",
		IrpContext->MinorFunction);
      RC = STATUS_INVALID_DEVICE_REQUEST;
      break;
    }
  if (RC == STATUS_PENDING)
  {
     RC = VfatQueueRequest(IrpContext);
  }
  else
  {
    IrpContext->Irp->IoStatus.Status = RC;
    IrpContext->Irp->IoStatus.Information = 0;
    IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
    VfatFreeIrpContext(IrpContext);
  }
  return RC;
}


