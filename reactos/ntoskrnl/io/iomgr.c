/* $Id: iomgr.c,v 1.29 2002/10/05 10:53:37 dwelch Exp $
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
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_DEVICE_TYPE     TAG('D', 'E', 'V', 'T')
#define TAG_FILE_TYPE       TAG('F', 'I', 'L', 'E')

/* DATA ********************************************************************/


POBJECT_TYPE EXPORTED IoDeviceObjectType = NULL;
POBJECT_TYPE EXPORTED IoFileObjectType = NULL;
ULONG        EXPORTED IoReadOperationCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoReadTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoWriteOperationCount = 0; /* FIXME: unknown type */
ULONG        EXPORTED IoWriteTransferCount = 0;	/* FIXME: unknown type */
ULONG        EXPORTED IoStatisticsLock = 0;	/* FIXME: unknown type */

static GENERIC_MAPPING IopFileMapping = {FILE_GENERIC_READ,
					 FILE_GENERIC_WRITE,
					 FILE_GENERIC_EXECUTE,
					 FILE_ALL_ACCESS};

/* FUNCTIONS ****************************************************************/

VOID STDCALL
IopCloseFile(PVOID ObjectBody,
	     ULONG HandleCount)
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

VOID STDCALL
IopDeleteFile(PVOID ObjectBody)
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


VOID IoInit (VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DirName;
  UNICODE_STRING LinkName;
  HANDLE Handle;

  IopInitDriverImplementation();
  
  /*
   * Register iomgr types: DeviceObjectType
   */
  IoDeviceObjectType = ExAllocatePool (NonPagedPool, 
				       sizeof (OBJECT_TYPE));
  
  IoDeviceObjectType->Tag = TAG_DEVICE_TYPE;
  IoDeviceObjectType->TotalObjects = 0;
  IoDeviceObjectType->TotalHandles = 0;
  IoDeviceObjectType->MaxObjects = ULONG_MAX;
  IoDeviceObjectType->MaxHandles = ULONG_MAX;
  IoDeviceObjectType->PagedPoolCharge = 0;
  IoDeviceObjectType->NonpagedPoolCharge = sizeof (DEVICE_OBJECT);
  IoDeviceObjectType->Mapping = &IopFileMapping;
  IoDeviceObjectType->Dump = NULL;
  IoDeviceObjectType->Open = NULL;
  IoDeviceObjectType->Close = NULL;
  IoDeviceObjectType->Delete = NULL;
  IoDeviceObjectType->Parse = NULL;
  IoDeviceObjectType->Security = NULL;
  IoDeviceObjectType->QueryName = NULL;
  IoDeviceObjectType->OkayToClose = NULL;
  IoDeviceObjectType->Create = IopCreateDevice;
  IoDeviceObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&IoDeviceObjectType->TypeName, L"Device");

  /*
   * Register iomgr types: FileObjectType
   * (alias DriverObjectType)
   */
  IoFileObjectType = ExAllocatePool (NonPagedPool, sizeof (OBJECT_TYPE));
  
  IoFileObjectType->Tag = TAG_FILE_TYPE;
  IoFileObjectType->TotalObjects = 0;
  IoFileObjectType->TotalHandles = 0;
  IoFileObjectType->MaxObjects = ULONG_MAX;
  IoFileObjectType->MaxHandles = ULONG_MAX;
  IoFileObjectType->PagedPoolCharge = 0;
  IoFileObjectType->NonpagedPoolCharge = sizeof(FILE_OBJECT);
  IoFileObjectType->Mapping = &IopFileMapping;
  IoFileObjectType->Dump = NULL;
  IoFileObjectType->Open = NULL;
  IoFileObjectType->Close = IopCloseFile;
  IoFileObjectType->Delete = IopDeleteFile;
  IoFileObjectType->Parse = NULL;
  IoFileObjectType->Security = NULL;
  IoFileObjectType->QueryName = NULL;
  IoFileObjectType->OkayToClose = NULL;
  IoFileObjectType->Create = IopCreateFile;
  IoFileObjectType->DuplicationNotify = NULL;
  
  RtlInitUnicodeStringFromLiteral(&IoFileObjectType->TypeName, L"File");

  /*
   * Create the '\Driver' object directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName, L"\\Driver");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  NtCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\FileSystem' object directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\FileSystem");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  NtCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\Device' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\Device");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\??' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\??");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Create the '\ArcName' directory
   */
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\ArcName");
  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);
  ZwCreateDirectoryObject(&Handle,
			  0,
			  &ObjectAttributes);

  /*
   * Initialize remaining subsubsystem
   */
  IoInitCancelHandling();
  IoInitSymbolicLinkImplementation();
  IoInitFileSystemImplementation();
  IoInitVpbImplementation();
  IoInitShutdownNotification();

  /*
   * Create link from '\DosDevices' to '\??' directory
   */
  RtlInitUnicodeStringFromLiteral(&LinkName,
		       L"\\DosDevices");
  RtlInitUnicodeStringFromLiteral(&DirName,
		       L"\\??");
  IoCreateSymbolicLink(&LinkName,
		       &DirName);

  /*
   * Initialize PnP manager
   */
  PnpInit();
}


PGENERIC_MAPPING STDCALL
IoGetFileObjectGenericMapping(VOID)
{
  return(&IopFileMapping);
}

/* EOF */
