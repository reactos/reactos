/* $Id: volume.c,v 1.3 2002/09/07 15:12:02 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/npfs/volume.c
 * PURPOSE:          Named pipe filesystem
 * PROGRAMMER:       Eric Kohl <ekohl@rz-online.de>
 */

/* INCLUDES *****************************************************************/

#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "npfs.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
NpfsQueryFsDeviceInformation(PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
			     PULONG BufferLength)
{
   DPRINT("NpfsQueryFsDeviceInformation()\n");
   DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);
   
   if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
     return(STATUS_BUFFER_OVERFLOW);
   
   FsDeviceInfo->DeviceType = FILE_DEVICE_NAMED_PIPE;
   FsDeviceInfo->Characteristics = 0;
   
   *BufferLength -= sizeof(FILE_FS_DEVICE_INFORMATION);
   
   DPRINT("NpfsQueryFsDeviceInformation() finished.\n");
   
   return(STATUS_SUCCESS);
}


static NTSTATUS
NpfsQueryFsAttributeInformation(PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
				PULONG BufferLength)
{
   DPRINT("NpfsQueryFsAttributeInformation() called.\n");
   DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);
   
   if (*BufferLength < sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8)
     return(STATUS_BUFFER_OVERFLOW);
   
   FsAttributeInfo->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;
   FsAttributeInfo->MaximumComponentNameLength = 255;
   FsAttributeInfo->FileSystemNameLength = 8;
   wcscpy(FsAttributeInfo->FileSystemName,
	  L"NPFS");
   
   DPRINT("NpfsQueryFsAttributeInformation() finished.\n");
   *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NpfsQueryVolumeInformation(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp)
{
   PIO_STACK_LOCATION Stack;
   FS_INFORMATION_CLASS FsInformationClass;
   NTSTATUS Status = STATUS_SUCCESS;
   PVOID SystemBuffer;
   ULONG BufferLength;
   
   /* PRECONDITION */
   assert(DeviceObject != NULL);
   assert(Irp != NULL);
   
   DPRINT("NpfsQueryVolumeInformation(DeviceObject %x, Irp %x)\n",
	  DeviceObject,
	  Irp);
   
   Stack = IoGetCurrentIrpStackLocation (Irp);
   FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
   BufferLength = Stack->Parameters.QueryVolume.Length;
   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
   
   DPRINT("FsInformationClass %d\n", FsInformationClass);
   DPRINT("SystemBuffer %x\n", SystemBuffer);
   
   switch (FsInformationClass)
     {
     case FileFsDeviceInformation:
       Status = NpfsQueryFsDeviceInformation(SystemBuffer,
					     &BufferLength);
       break;
   
     case FileFsAttributeInformation:
       Status = NpfsQueryFsAttributeInformation(SystemBuffer,
						&BufferLength);
       break;
   
     default:
       Status = STATUS_NOT_SUPPORTED;
     }
   
   Irp->IoStatus.Status = Status;
   if (NT_SUCCESS(Status))
     Irp->IoStatus.Information =
       Stack->Parameters.QueryVolume.Length - BufferLength;
   else
     Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp,
		     IO_NO_INCREMENT);
   
   return(Status);
}

/* EOF */
