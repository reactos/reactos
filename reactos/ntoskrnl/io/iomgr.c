/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/io/iomgr.c
 * PURPOSE:              Initializes the io manager
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE IoDeviceType = NULL;
POBJECT_TYPE IoFileType = NULL;
                           
/* FUNCTIONS ****************************************************************/

VOID IopCloseFile(PVOID ObjectBody, ULONG HandleCount)
{
   PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IopCloseFile()\n");
   
   if (HandleCount > 0)
     {
	return;
     }
   
   ObReferenceObjectByPointer(FileObject,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileType,
			      UserMode);
   
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_CLEANUP,
				      FileObject->DeviceObject,
				      NULL,
				      0,
				      NULL,
				      NULL,
				      NULL);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   
   Status = IoCallDriver(FileObject->DeviceObject, Irp);
}

VOID IopDeleteFile(PVOID ObjectBody)
{
   PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   NTSTATUS Status;
   
   DPRINT("IopDeleteFile()\n");
   
   ObReferenceObjectByPointer(ObjectBody,
			      STANDARD_RIGHTS_REQUIRED,
			      IoFileType,
			      UserMode);
   
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_CLOSE,
				      FileObject->DeviceObject,
				      NULL,
				      0,
				      NULL,
				      NULL,
				      NULL);
   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   
   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   
   if (FileObject->FileName.Buffer != NULL)
     {
	ExFreePool(FileObject->FileName.Buffer);
     }
}

VOID IoShutdownIoManager(VOID)
{
}

VOID IoInit(VOID)
{
   OBJECT_ATTRIBUTES attr;
   HANDLE handle;
   UNICODE_STRING UnicodeString;
   ANSI_STRING AnsiString;
   
   /*
    * Register iomgr types
    */
   IoDeviceType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   IoDeviceType->TotalObjects = 0;
   IoDeviceType->TotalHandles = 0;
   IoDeviceType->MaxObjects = ULONG_MAX;
   IoDeviceType->MaxHandles = ULONG_MAX;
   IoDeviceType->PagedPoolCharge = 0;
   IoDeviceType->NonpagedPoolCharge = sizeof(DEVICE_OBJECT);
   IoDeviceType->Dump = NULL;
   IoDeviceType->Open = NULL;
   IoDeviceType->Close = NULL;   
   IoDeviceType->Delete = NULL;
   IoDeviceType->Parse = NULL;
   IoDeviceType->Security = NULL;
   IoDeviceType->QueryName = NULL;
   IoDeviceType->OkayToClose = NULL;
   IoDeviceType->Create = IopCreateDevice;
   
   RtlInitAnsiString(&AnsiString,"Device");
   RtlAnsiStringToUnicodeString(&IoDeviceType->TypeName,&AnsiString,TRUE);
   
   IoFileType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   IoFileType->TotalObjects = 0;
   IoFileType->TotalHandles = 0;
   IoFileType->MaxObjects = ULONG_MAX;
   IoFileType->MaxHandles = ULONG_MAX;
   IoFileType->PagedPoolCharge = 0;
   IoFileType->NonpagedPoolCharge = sizeof(FILE_OBJECT);
   IoFileType->Dump = NULL;
   IoFileType->Open = NULL;
   IoFileType->Close = IopCloseFile;
   IoFileType->Delete = IopDeleteFile;
   IoFileType->Parse = NULL;
   IoFileType->Security = NULL;
   IoFileType->QueryName = NULL;
   IoFileType->OkayToClose = NULL;
   IoFileType->Create = IopCreateFile;
   
   RtlInitAnsiString(&AnsiString,"File");
   RtlAnsiStringToUnicodeString(&IoFileType->TypeName,&AnsiString,TRUE);

   /*
    * Create the device directory
    */
   RtlInitAnsiString(&AnsiString,"\\Device");
   RtlAnsiStringToUnicodeString(&UnicodeString,&AnsiString,TRUE);
   InitializeObjectAttributes(&attr,&UnicodeString,0,NULL,NULL);
   ZwCreateDirectoryObject(&handle,0,&attr);
   
   RtlInitAnsiString(&AnsiString,"\\??");
   RtlAnsiStringToUnicodeString(&UnicodeString,&AnsiString,TRUE);
   InitializeObjectAttributes(&attr,&UnicodeString,0,NULL,NULL);
   ZwCreateDirectoryObject(&handle,0,&attr);

   IoInitCancelHandling();
   IoInitSymbolicLinkImplementation();
   IoInitFileSystemImplementation();
}
