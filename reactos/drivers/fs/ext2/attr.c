/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ext2/attr.c
 * PURPOSE:          Set/Get file attributes support
 * PROGRAMMER:       David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <internal/string.h>

//#define NDEBUG
#include <internal/debug.h>

#include "ext2fs.h"

/* FUNCTIONS ****************************************************************/

NTSTATUS Ext2SetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   DPRINT("Ext2SetInformation(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS Ext2QueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   NTSTATUS Status;
   PIO_STACK_LOCATION Param;
   PFILE_OBJECT FileObject;
   PDEVICE_EXTENSION DeviceExt;
   ULONG Length;
   PFILE_BASIC_INFORMATION PFileBasicInformation;
   PFILE_STANDARD_INFORMATION PFileStandardInformation;
   PFILE_INTERNAL_INFORMATION PFileInternalInformation;
   PFILE_EA_INFORMATION PFileEaInformation;
   PFILE_ACCESS_INFORMATION PFileAccessInformation;
   PFILE_NAME_INFORMATION PFileNameInformation;
   PFILE_POSITION_INFORMATION PFilePositionInformation;
   PVOID Buffer;
   
   DPRINT("Ext2QueryInformation(DeviceObject %x Irp %x)\n", DeviceObject, Irp);
   
   Param = IoGetCurrentIrpStackLocation(Irp);
   FileObject = Param->FileObject;
   DeviceExt = DeviceObject->DeviceExtension;
   Length = Param->Parameters.QueryFile.Length;
   Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
   
   switch (Param->Parameters.QueryFile.FileInformationClass)
     {
      case FileDirectoryInformation:
      case FileFullDirectoryInformation:
      case FileBothDirectoryInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
	
      case FileBasicInformation:
	PFileBasicInformation = (PFILE_BASIC_INFORMATION)Buffer;
	memset(PFileBasicInformation, 0, sizeof(FILE_BASIC_INFORMATION));
	Status = STATUS_SUCCESS;
	break;
	
      case FileStandardInformation:
	PFileStandardInformation = (PFILE_STANDARD_INFORMATION)Buffer;
	memset(PFileStandardInformation, 0, sizeof(FILE_STANDARD_INFORMATION));
	Status = STATUS_SUCCESS;
	break;
	
      case FileInternalInformation:
	PFileInternalInformation = (PFILE_INTERNAL_INFORMATION)Buffer;
	memset(PFileInternalInformation, 0, sizeof(FILE_INTERNAL_INFORMATION));
	Status = STATUS_SUCCESS;
	break;
	
      case FileEaInformation:
	PFileEaInformation = (PFILE_EA_INFORMATION)Buffer;
	memset(PFileEaInformation, 0, sizeof(FILE_EA_INFORMATION));
	PFileEaInformation->EaSize = 0;
	Status = STATUS_SUCCESS;
	break;
	
      case FileAccessInformation:
	PFileAccessInformation = (PFILE_ACCESS_INFORMATION)Buffer;
	memset(PFileAccessInformation, 0, sizeof(FILE_ACCESS_INFORMATION));
	PFileAccessInformation->AccessFlags = 0;
	Status = STATUS_SUCCESS;
	break;
	
      case FileNameInformation:
	PFileNameInformation = (PFILE_NAME_INFORMATION)Buffer;
	memset(PFileNameInformation, 0, sizeof(FILE_NAME_INFORMATION));
	Status = STATUS_SUCCESS;
	break;
	
      case FilePositionInformation:
	PFilePositionInformation = (PFILE_POSITION_INFORMATION)Buffer;
	memcpy(PFilePositionInformation, 
	       &FileObject->CurrentByteOffset,
	       sizeof(FileObject->CurrentByteOffset));
	Status = STATUS_SUCCESS;
	break;
	
      case FileRenameInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
	
      default:
	Status = STATUS_NOT_IMPLEMENTED;
     }
   
   
   
   
   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

