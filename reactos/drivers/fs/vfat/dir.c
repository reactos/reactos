/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/dir.c
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



unsigned long
vfat_wstrlen (PWSTR s)
{
  WCHAR c = ' ';
  unsigned int len = 0;

  while (c != 0)
    {
      c = *s;
      s++;
      len++;
    };
  s -= len;

  return len - 1;
}

#define DWORD_ROUND_UP(x) ( (((ULONG)(x))%32) ? ((((ULONG)x)&(~0x1f))+0x20) : ((ULONG)x) )

NTSTATUS
VfatGetFileNameInformation (PVFATFCB pFcb,
			   PFILE_NAMES_INFORMATION pInfo, ULONG BufferLength)
{
  ULONG Length;
  Length = vfat_wstrlen (pFcb->ObjectName);
  if ((sizeof (FILE_DIRECTORY_INFORMATION) + Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_DIRECTORY_INFORMATION) + Length);
  memcpy (pInfo->FileName, pFcb->ObjectName,
	  sizeof (WCHAR) * (pInfo->FileNameLength));
  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileDirectoryInformation (PVFATFCB pFcb,
				PDEVICE_EXTENSION DeviceExt,
				PFILE_DIRECTORY_INFORMATION pInfo,
				ULONG BufferLength)
{
  unsigned long long AllocSize;
  ULONG Length;
  Length = vfat_wstrlen (pFcb->ObjectName);
  if ((sizeof (FILE_DIRECTORY_INFORMATION) + Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_DIRECTORY_INFORMATION) + Length);
  memcpy (pInfo->FileName, pFcb->ObjectName,
	  sizeof (WCHAR) * (pInfo->FileNameLength));
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (pFcb->entry.CreationDate,
			    pFcb->entry.CreationTime, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (pFcb->entry.AccessDate, 0,
			    &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->LastWriteTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->ChangeTime);
  pInfo->EndOfFile = RtlConvertUlongToLargeInteger (pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize + DeviceExt->BytesPerCluster - 1) /
	       DeviceExt->BytesPerCluster) * DeviceExt->BytesPerCluster;
  pInfo->AllocationSize.QuadPart = AllocSize;
  pInfo->FileAttributes = pFcb->entry.Attrib;

  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileFullDirectoryInformation (PVFATFCB pFcb,
				    PDEVICE_EXTENSION DeviceExt,
				    PFILE_FULL_DIRECTORY_INFORMATION pInfo,
				    ULONG BufferLength)
{
  unsigned long long AllocSize;
  ULONG Length;
  Length = vfat_wstrlen (pFcb->ObjectName);
  if ((sizeof (FILE_FULL_DIRECTORY_INFORMATION) + Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_FULL_DIRECTORY_INFORMATION) + Length);
  memcpy (pInfo->FileName, pFcb->ObjectName,
	  sizeof (WCHAR) * (pInfo->FileNameLength));
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (pFcb->entry.CreationDate,
			    pFcb->entry.CreationTime, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (pFcb->entry.AccessDate, 0,
			    &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->LastWriteTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->ChangeTime);
  pInfo->EndOfFile = RtlConvertUlongToLargeInteger (pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize + DeviceExt->BytesPerCluster - 1) /
	       DeviceExt->BytesPerCluster) * DeviceExt->BytesPerCluster;
  pInfo->AllocationSize.QuadPart = AllocSize;
  pInfo->FileAttributes = pFcb->entry.Attrib;
//      pInfo->EaSize=;
  return STATUS_SUCCESS;
}

NTSTATUS
VfatGetFileBothInformation (PVFATFCB pFcb,
			   PDEVICE_EXTENSION DeviceExt,
			   PFILE_BOTH_DIRECTORY_INFORMATION pInfo,
			   ULONG BufferLength)
{
  short i;
  unsigned long long AllocSize;
  ULONG Length;
  Length = vfat_wstrlen (pFcb->ObjectName);
  if ((sizeof (FILE_BOTH_DIRECTORY_INFORMATION) + Length) > BufferLength)
    return STATUS_BUFFER_OVERFLOW;
  pInfo->FileNameLength = Length;
  pInfo->NextEntryOffset =
    DWORD_ROUND_UP (sizeof (FILE_BOTH_DIRECTORY_INFORMATION) + Length);
  memcpy (pInfo->FileName, pFcb->ObjectName,
	  sizeof (WCHAR) * (pInfo->FileNameLength));
//      pInfo->FileIndex=;
  FsdDosDateTimeToFileTime (pFcb->entry.CreationDate,
			    pFcb->entry.CreationTime, &pInfo->CreationTime);
  FsdDosDateTimeToFileTime (pFcb->entry.AccessDate, 0,
			    &pInfo->LastAccessTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->LastWriteTime);
  FsdDosDateTimeToFileTime (pFcb->entry.UpdateDate, pFcb->entry.UpdateTime,
			    &pInfo->ChangeTime);
  pInfo->EndOfFile = RtlConvertUlongToLargeInteger (pFcb->entry.FileSize);
  /* Make allocsize a rounded up multiple of BytesPerCluster */
  AllocSize = ((pFcb->entry.FileSize + DeviceExt->BytesPerCluster - 1) /
	       DeviceExt->BytesPerCluster) * DeviceExt->BytesPerCluster;
  pInfo->AllocationSize.QuadPart = AllocSize;
  pInfo->FileAttributes = pFcb->entry.Attrib;
//      pInfo->EaSize=;
  for (i = 0; i < 8 && (pFcb->entry.Filename[i] != ' '); i++)
    pInfo->ShortName[i] = pFcb->entry.Filename[i];
  pInfo->ShortNameLength = i;
  pInfo->ShortName[i] = '.';
  for (i = 0; i < 3 && (pFcb->entry.Ext[i] != ' '); i++)
    pInfo->ShortName[i + 1 + pInfo->ShortNameLength] = pFcb->entry.Ext[i];
  if (i)
    pInfo->ShortNameLength += (i + 1);
  return STATUS_SUCCESS;
}

NTSTATUS
DoQuery (PDEVICE_OBJECT DeviceObject, PIRP Irp, PIO_STACK_LOCATION Stack)
{
  NTSTATUS RC = STATUS_SUCCESS;
  long BufferLength = 0;
  PUNICODE_STRING pSearchPattern = NULL;
  FILE_INFORMATION_CLASS FileInformationClass;
  unsigned long FileIndex = 0;
  unsigned char *Buffer = NULL;
  PFILE_NAMES_INFORMATION Buffer0 = NULL;
  PFILE_OBJECT pFileObject = NULL;
  PVFATFCB pFcb;
  VFATFCB tmpFcb;
  PVFATCCB pCcb;
  PDEVICE_EXTENSION DeviceExt;
  WCHAR star[5], *pCharPattern;
  unsigned long OldEntry, OldSector;
  DeviceExt = DeviceObject->DeviceExtension;
  // Obtain the callers parameters
  BufferLength = Stack->Parameters.QueryDirectory.Length;
  pSearchPattern = Stack->Parameters.QueryDirectory.FileName;
  FileInformationClass =
    Stack->Parameters.QueryDirectory.FileInformationClass;
  FileIndex = Stack->Parameters.QueryDirectory.FileIndex;
  pFileObject = Stack->FileObject;
  pCcb = (PVFATCCB) pFileObject->FsContext2;
  pFcb = pCcb->pFcb;
  if (Stack->Flags & SL_RESTART_SCAN)
    {				//FIXME : what is really use of RestartScan ?
      pCcb->StartEntry = pCcb->StartSector = 0;
    }
  // determine Buffer for result :
  if (Irp->MdlAddress)
    Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
  else
    Buffer = Irp->UserBuffer;
  DPRINT ("Buffer=%x tofind=%S\n", Buffer, pSearchPattern->Buffer);
  if (pSearchPattern == NULL)
    {
      star[0] = '*';
      star[1] = 0;
      pCharPattern = star;
    }
  else
    pCharPattern = pSearchPattern->Buffer;
  tmpFcb.ObjectName = tmpFcb.PathName;
  while (RC == STATUS_SUCCESS && BufferLength > 0)
    {
      OldSector = pCcb->StartSector;
      OldEntry = pCcb->StartEntry;
      if (OldSector)
	pCcb->StartEntry++;
      RC =
	FindFile (DeviceExt, &tmpFcb, pFcb, pCharPattern, &pCcb->StartSector,
		  &pCcb->StartEntry);
      DPRINT ("Found %S,RC=%x, sector %x entry %x\n", tmpFcb.ObjectName, RC,
	      pCcb->StartSector, pCcb->StartEntry);
      if (NT_SUCCESS (RC))
	{
	  switch (FileInformationClass)
	    {
	    case FileNameInformation:
	      RC =
		VfatGetFileNameInformation (&tmpFcb,
					   (PFILE_NAMES_INFORMATION) Buffer,
					   BufferLength);
	      break;
	    case FileDirectoryInformation:
	      RC =
		VfatGetFileDirectoryInformation (&tmpFcb, DeviceExt,
						(PFILE_DIRECTORY_INFORMATION)
						Buffer, BufferLength);
	      break;
	    case FileFullDirectoryInformation:
	      RC =
		VfatGetFileFullDirectoryInformation (&tmpFcb, DeviceExt,
						    (PFILE_FULL_DIRECTORY_INFORMATION)
						    Buffer, BufferLength);
	      break;
	    case FileBothDirectoryInformation:
	      RC =
		VfatGetFileBothInformation (&tmpFcb, DeviceExt,
					   (PFILE_BOTH_DIRECTORY_INFORMATION)
					   Buffer, BufferLength);
	      break;
	    default:
	      RC = STATUS_INVALID_INFO_CLASS;
	    }
	}
      else
	{
	  if (Buffer0)
	    Buffer0->NextEntryOffset = 0;
	  break;
	}
      if (RC == STATUS_BUFFER_OVERFLOW)
	{
	  if (Buffer0)
	    Buffer0->NextEntryOffset = 0;
	  pCcb->StartSector = OldSector;
	  pCcb->StartEntry = OldEntry;
	  break;
	}
      Buffer0 = (PFILE_NAMES_INFORMATION) Buffer;
      Buffer0->FileIndex = FileIndex++;
      if (Stack->Flags & SL_RETURN_SINGLE_ENTRY)
	break;
      BufferLength -= Buffer0->NextEntryOffset;
      Buffer += Buffer0->NextEntryOffset;
    }
  if (Buffer0)
    Buffer0->NextEntryOffset = 0;
  if (FileIndex > 0)
    return STATUS_SUCCESS;
  return RC;
}


NTSTATUS STDCALL
VfatDirectoryControl (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: directory control : read/write directory informations
 */
{
  NTSTATUS RC = STATUS_SUCCESS;
  PFILE_OBJECT FileObject = NULL;
  PIO_STACK_LOCATION Stack;
  Stack = IoGetCurrentIrpStackLocation (Irp);
  CHECKPOINT;
  FileObject = Stack->FileObject;
  switch (Stack->MinorFunction)
    {
    case IRP_MN_QUERY_DIRECTORY:
      RC = DoQuery (DeviceObject, Irp, Stack);
      break;
    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
      DPRINT (" vfat, dir : change\n");
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      // error
      DbgPrint ("unexpected minor function %x in VFAT driver\n",
		Stack->MinorFunction);
      RC = STATUS_INVALID_DEVICE_REQUEST;
      break;
    }
  Irp->IoStatus.Status = RC;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest (Irp, IO_NO_INCREMENT);
  return RC;
}


