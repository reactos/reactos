/* $Id: iomgr.c,v 1.13 2000/07/07 02:10:18 ekohl Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/io/iomgr.c
 * PURPOSE:              Initializes the io manager
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *             29/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* DATA ********************************************************************/


POBJECT_TYPE EXPORTED IoDeviceObjectType = NULL;
POBJECT_TYPE EXPORTED IoFileObjectType = NULL;
ULONG        EXPORTED IoReadOperationCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoReadTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoWriteOperationCount = 0; /* FIXME: unknown type */
ULONG        EXPORTED IoWriteTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoStatisticsLock = 0;	/* FIXME: unknown type */

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
			      IoFileObjectType,
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
			      IoFileObjectType,
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
	FileObject->FileName.Buffer = 0;
     }
}

VOID IoShutdownIoManager(VOID)
{
}


VOID IoInit (VOID)
{
	OBJECT_ATTRIBUTES	attr;
	HANDLE			handle;
	UNICODE_STRING		UnicodeString;
	ANSI_STRING		AnsiString;
	UNICODE_STRING		DeviceName;

	/*
	 * Register iomgr types: DeviceObjectType
	 */
	IoDeviceObjectType = ExAllocatePool (
				NonPagedPool,
				sizeof (OBJECT_TYPE)
				);

	IoDeviceObjectType->TotalObjects = 0;
	IoDeviceObjectType->TotalHandles = 0;
	IoDeviceObjectType->MaxObjects = ULONG_MAX;
	IoDeviceObjectType->MaxHandles = ULONG_MAX;
	IoDeviceObjectType->PagedPoolCharge = 0;
	IoDeviceObjectType->NonpagedPoolCharge = sizeof (DEVICE_OBJECT);
	IoDeviceObjectType->Dump = NULL;
	IoDeviceObjectType->Open = NULL;
	IoDeviceObjectType->Close = NULL;
	IoDeviceObjectType->Delete = NULL;
	IoDeviceObjectType->Parse = NULL;
	IoDeviceObjectType->Security = NULL;
	IoDeviceObjectType->QueryName = NULL;
	IoDeviceObjectType->OkayToClose = NULL;
	IoDeviceObjectType->Create = IopCreateDevice;

	RtlInitAnsiString (
		& AnsiString,
		"Device"
		);
	RtlAnsiStringToUnicodeString (
		& IoDeviceObjectType->TypeName,
		& AnsiString,
		TRUE
		);
	/*
	 * Register iomgr types: FileObjectType
	 * (alias DriverObjectType)
	 */
	IoFileObjectType = ExAllocatePool (
				NonPagedPool,
				sizeof (OBJECT_TYPE)
				);

	IoFileObjectType->TotalObjects = 0;
	IoFileObjectType->TotalHandles = 0;
	IoFileObjectType->MaxObjects = ULONG_MAX;
	IoFileObjectType->MaxHandles = ULONG_MAX;
	IoFileObjectType->PagedPoolCharge = 0;
	IoFileObjectType->NonpagedPoolCharge = sizeof(FILE_OBJECT);
	IoFileObjectType->Dump = NULL;
	IoFileObjectType->Open = NULL;
	IoFileObjectType->Close = IopCloseFile;
	IoFileObjectType->Delete = IopDeleteFile;
	IoFileObjectType->Parse = NULL;
	IoFileObjectType->Security = NULL;
	IoFileObjectType->QueryName = NULL;
	IoFileObjectType->OkayToClose = NULL;
	IoFileObjectType->Create = IopCreateFile;

	RtlInitAnsiString (
		& AnsiString,
		"File"
		);
	RtlAnsiStringToUnicodeString (
		& IoFileObjectType->TypeName,
		& AnsiString,
		TRUE
		);

	/*
	 * Create the device directory
	 */
	RtlInitAnsiString (
		& AnsiString,
		"\\Device"
		);
	RtlAnsiStringToUnicodeString (
		& UnicodeString,
		& AnsiString,
		TRUE
		);
	InitializeObjectAttributes (
		& attr,
		& UnicodeString,
		0,
		NULL,
		NULL
		);
	ZwCreateDirectoryObject (
		& handle,
		0,
		& attr
		);

	/*
	 * Create the \?? directory
	 */
	RtlInitAnsiString (
		& AnsiString,
		"\\??"
		);
	RtlAnsiStringToUnicodeString (
		& UnicodeString,
		& AnsiString,
		TRUE
		);
	InitializeObjectAttributes (
		& attr,
		& UnicodeString,
		0,
		NULL,
		NULL
		);
	ZwCreateDirectoryObject (
		& handle,
		0,
		& attr
		);
	/*
	 * Initialize remaining subsubsystem
	 */
	IoInitCancelHandling ();
	IoInitSymbolicLinkImplementation ();
	IoInitFileSystemImplementation ();
	IoInitVpbImplementation ();

	/*
	 * Create link from \DosDevices to \?? directory
	 */
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\DosDevices");
	RtlInitUnicodeString (&DeviceName,
	                      L"\\??");
	IoCreateSymbolicLink (&UnicodeString,
	                      &DeviceName);
}

/* EOF */
