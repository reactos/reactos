/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/cdfs.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "cdfs.h"

typedef struct
{
  PDEVICE_OBJECT StorageDevice;
  PFILE_OBJECT StreamFileObject;
  struct _fcb_system *fss;

  PFCB RootFcb;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* GLOBALS *****************************************************************/

static PDRIVER_OBJECT DriverObject;

/* FUNCTIONS ****************************************************************/

/* DIRECTORY FUNCTIONS ******************************************************/
/* Hacked until working directory code...                                   */

NTSTATUS
FsdGetRootDirectoryData(PDEVICE_OBJECT DeviceObject,
			PFCB RootFcb)
{
  PPVD Buffer;
  NTSTATUS Status;

  Buffer = ExAllocatePool( NonPagedPool, CDFS_BASIC_SECTOR );
  if (Buffer == NULL)
    return(STATUS_INSUFFICIENT_RESOURCES);

  Status = CdfsReadSectors(DeviceObject,
			   CDFS_PRIMARY_DESCRIPTOR_LOCATION,
			   1,
			   Buffer);
  if (!NT_SUCCESS(Status))
    return Status;

  RootFcb->extent_start = Buffer->RootDirRecord.ExtentLocationL;
  RootFcb->byte_count = Buffer->RootDirRecord.DataLengthL;

  ExFreePool(Buffer);

  DPRINT1("RootFcb->extent_start %lu\n", RootFcb->extent_start);
  DPRINT1("RootFcb->byte_count %lu\n", RootFcb->byte_count);

  return(STATUS_SUCCESS);
}



/* HACK -- NEEDS FIXING */

int
FsdExtractDirectoryEntry(PDEVICE_EXTENSION DeviceExt,
			 FsdFcbEntry *parent,
			 FsdFcbEntry *fill_in,
			 int entry_to_get)
{
  switch( entry_to_get )
    {
      case 0:
	wcscpy( fill_in->name, L"." );
	fill_in->extent_start = parent->extent_start;
	fill_in->byte_count = parent->byte_count;
	break;

      case 1:
	wcscpy( fill_in->name, L".." );
	fill_in->extent_start = parent->extent_start;
	fill_in->byte_count = parent->byte_count;
	break;

      case 2:
	wcscpy( fill_in->name, L"readme.txt" );
	fill_in->extent_start = 0x190;
	fill_in->byte_count = 0x800;
	break;

      default:
	return 1;
    }

  return 0;
}


FsdFcbEntry *FsdSearchDirectoryAt( PDEVICE_EXTENSION DeviceExt,
				   FsdFcbEntry *parent,
				   wchar_t *look_for )
{
  FsdFcbEntry *ent;
  int i;

  ent = FsdCreateFcb( DeviceExt->fss, parent, look_for );
  if( ent )
    return ent;

  for( i = 0; i < parent->byte_count; i++ ) {
    FsdFcbEntry new_ent;
    if( FsdExtractDirectoryEntry( DeviceExt, parent, &new_ent, i ) )
      return NULL;

    if( !_wcsicmp( new_ent.name, look_for ) ) {
      ent->extent_start = new_ent.extent_start;
      ent->byte_count = new_ent.byte_count;
      return ent;
    }
  }

  /* Not found */
  FsdDelete( ent );

  return NULL;
}

NTSTATUS
FsdCloseFile(PDEVICE_EXTENSION DeviceExt,
	     PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
   return(STATUS_SUCCESS);
}

/* Should go into a basic library */
PFCB
FsdSearchDirectory(PDEVICE_EXTENSION DeviceExt,
		   PFCB ParentFcb,
		   PWSTR filename)
{
  wchar_t *string_imed = NULL;
  wchar_t *string_end = NULL;
  FsdFcbEntry *this_elt = NULL;

  DPRINT1("FsdSearchDirectory(%S) called\n", filename);

  if (ParentFcb == NULL && *filename == 0)
    return(DeviceExt->RootFcb);

CHECKPOINT1;
  string_imed = ExAllocatePool(NonPagedPool,
		    (wcslen( filename ) + 1) * sizeof( wchar_t ) );

  if (string_imed == NULL)
    return NULL;

  wcscpy( string_imed, filename );

  
  /* Chop off a component and try it */
  string_end = wcschr( string_imed, L'\\' );
  if (string_end != NULL)
    {
      *string_end = 0;
      string_end++;
    }
CHECKPOINT1;

  if (ParentFcb == NULL)
    this_elt = FsdGetFcbEntry(DeviceExt->fss, NULL, L"\\");
  else
    this_elt = FsdGetFcbEntry(DeviceExt->fss, ParentFcb, string_imed);
DPRINT1("this_elt %p\n", this_elt);

  if (this_elt)
    {
      ExFreePool( string_imed );
CHECKPOINT1;
      return this_elt;
    }

CHECKPOINT1;
  this_elt = FsdSearchDirectoryAt( DeviceExt, ParentFcb, string_imed );
CHECKPOINT1;

  /* It's the end if we have nothing more to do */
  if (string_end != NULL)
    this_elt = FsdSearchDirectory( DeviceExt, this_elt, string_end );
CHECKPOINT1;

  ExFreePool( string_imed );

  DPRINT1("FsdSearchDirectory() done");

  return this_elt;
}


NTSTATUS
FsdOpenFile(PDEVICE_EXTENSION DeviceExt,
	    PFILE_OBJECT FileObject,
	    PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
  PFCB Fcb;

  DPRINT1("FsdOpenFile(FileName %S) called\n", FileName);

  /* Just skip leading backslashes... */
  while (*FileName == L'\\')
    FileName++;
CHECKPOINT1;

  Fcb = FsdSearchDirectory(DeviceExt->fss,
			   NULL,
			   FileName);
CHECKPOINT1;
  if (Fcb == NULL)
    {
      DPRINT1("FsdSearchDirectory() failed\n");
      return(STATUS_OBJECT_PATH_NOT_FOUND);
    }
CHECKPOINT1;

  FileObject->Flags = FileObject->Flags | FO_FCB_IS_VALID |
    FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  FileObject->FsContext = Fcb;
  FileObject->FsContext2 = DeviceExt->fss;

  DPRINT1("FsdOpenFile() done\n");

  return(STATUS_SUCCESS);
}


BOOLEAN
FsdHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
  PUCHAR bytebuf; // [CDFS_BASIC_SECTOR];
  NTSTATUS readstatus;
  int ret;

  bytebuf = ExAllocatePool( NonPagedPool, CDFS_BASIC_SECTOR );
  if( !bytebuf ) return FALSE;

  DPRINT1( "CDFS: Checking on mount of device %08x\n", DeviceToMount );

  readstatus = CdfsReadSectors(DeviceToMount,
			       CDFS_PRIMARY_DESCRIPTOR_LOCATION,
			       1,
			       bytebuf);
  bytebuf[6] = 0;
  DPRINT1( "CD-identifier: [%.5s]\n", bytebuf + 1 );

  ret = 
    readstatus == STATUS_SUCCESS &&
    bytebuf[0] == 1 &&
    bytebuf[1] == 'C' &&
    bytebuf[2] == 'D' &&
    bytebuf[3] == '0' &&
    bytebuf[4] == '0' &&
    bytebuf[5] == '1';

  ExFreePool( bytebuf );

  return ret;
}

NTSTATUS
FsdMountDevice(PDEVICE_EXTENSION DeviceExt,
	       PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
   return(STATUS_SUCCESS);
}


NTSTATUS
FsdReadFile(PDEVICE_EXTENSION DeviceExt,
	    PFILE_OBJECT FileObject,
	    PVOID Buffer,
	    ULONG Length,
	    ULONG Offset)
/*
 * FUNCTION: Reads data from a file
 */
{
  PUCHAR bytebuf; // [CDFS_BASIC_SECTOR];
  NTSTATUS Status;
  int ret;
  int sofar = 0;
  FsdFcbEntry *ffe = FileObject->FsContext;

  if( Length ) return STATUS_SUCCESS;

  bytebuf = ExAllocatePool( NonPagedPool, CDFS_BASIC_SECTOR );
  if (!bytebuf)
    return FALSE;

  if (Offset + Length > ffe->byte_count)
    Length = ffe->byte_count - Offset;

  DPRINT1( "Reading %d bytes at %d\n", Offset, Length );

  if( Length == 0 ) return STATUS_UNSUCCESSFUL;

  while( sofar < Length )
    {
      int remains = 0;

      Status = CdfsReadSectors(DeviceExt->StorageDevice,
			       ffe->extent_start + (sofar >> 11),
			       1,
			       bytebuf);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool( bytebuf );
	  return(Status);
	}

      remains = Length - sofar;
      if (remains > BLOCKSIZE)
	remains = BLOCKSIZE;

      memcpy( ((char *)Buffer) + sofar, bytebuf, remains );
      sofar += remains;
    }

  ExFreePool( bytebuf );

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
FsdClose(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
   PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_OBJECT FileObject = Stack->FileObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   NTSTATUS Status;
   
   DPRINT1( "Closing\n" );

   Status = FsdCloseFile(DeviceExtension,FileObject);

   Irp->IoStatus.Status = Status;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(Status);
}

NTSTATUS STDCALL
FsdCreate(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  NTSTATUS Status;
  PDEVICE_EXTENSION DeviceExt;
  
  DeviceExt = DeviceObject->DeviceExtension;
  Status = FsdOpenFile(DeviceExt,FileObject,FileObject->FileName.Buffer);
  
  Irp->IoStatus.Status = Status;
  if (Status == STATUS_SUCCESS)
    Irp->IoStatus.Information = FILE_OPENED;
  else
    Irp->IoStatus.Information = 0;
  
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return(Status);
}


NTSTATUS STDCALL
FsdWrite(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
   DPRINT("FsdWrite(DeviceObject %x Irp %x)\n",DeviceObject,Irp);
   
   Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
   Irp->IoStatus.Information = 0;
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL
FsdRead(PDEVICE_OBJECT DeviceObject,
	PIRP Irp)
{
  ULONG Length;
  PVOID Buffer;
  ULONG Offset;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  PFILE_OBJECT FileObject = Stack->FileObject;
  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
  NTSTATUS Status;

  DPRINT1("FsdRead(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

  Length = Stack->Parameters.Read.Length;
  Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
  Offset = Stack->Parameters.Read.ByteOffset.u.LowPart;

  Status = FsdReadFile(DeviceExt,FileObject,Buffer,Length,Offset);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = Length;
  IoCompleteRequest(Irp,IO_NO_INCREMENT);
  return(Status);
}

// XXX The VPB in devicetomount may be null for some reason...
NTSTATUS
FsdMountVolume(PIRP Irp)
{
  PDEVICE_OBJECT DeviceObject = NULL;
  PDEVICE_OBJECT DeviceToMount;
  PDEVICE_EXTENSION DeviceExt = NULL;
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT1("FsdMountVolume() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;

  if (FsdHasFileSystem(DeviceToMount) == FALSE)
      return(STATUS_UNRECOGNIZED_VOLUME);

  Status = IoCreateDevice(DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  NULL,
			  FILE_DEVICE_CD_ROM_FILE_SYSTEM,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    return(Status);

  DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
  DeviceExt = (PVOID)DeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceExt,sizeof(DEVICE_EXTENSION));

  DeviceExt->fss = FsdFcbInit();
  if (!DeviceExt->fss)
    return(STATUS_UNSUCCESSFUL);

  DeviceExt->RootFcb = FsdCreateFcb(DeviceExt->fss, NULL, L"\\");
  Status = FsdGetRootDirectoryData(DeviceToMount, DeviceExt->RootFcb);

  DeviceObject->Vpb = DeviceToMount->Vpb;
  if (DeviceObject->Vpb != NULL)
    DeviceObject->Vpb->Flags |= VPB_MOUNTED;

#if 0
  Status = FsdMountDevice(DeviceExt,
			  DeviceToMount);
  if (!NT_SUCCESS(Status))
    return(Status);
#endif

  DeviceExt->StorageDevice = IoAttachDeviceToDeviceStack(DeviceObject,
							 DeviceToMount);

  DeviceExt->StreamFileObject = IoCreateStreamFileObject(NULL,
							 DeviceExt->StorageDevice);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
FsdFileSystemControl(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT1("CDFS FsdFileSystemControl() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_USER_FS_REQUEST:
	DPRINT("CDFS: IRP_MN_USER_FS_REQUEST\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;

      case IRP_MN_MOUNT_VOLUME:
	DPRINT("CDFS: IRP_MN_MOUNT_VOLUME\n");
	Status = FsdMountVolume(Irp);
	break;

      case IRP_MN_VERIFY_VOLUME:
	DPRINT("CDFS: IRP_MN_VERIFY_VOLUME\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;

      default:
	DPRINT("CDFS FSC: MinorFunction %d\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


NTSTATUS STDCALL
FsdDirectoryControl(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT1("FsdDirectoryControl() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
#if 0
      case IRP_MN_USER_FS_REQUEST:
	DPRINT("CDFS: IRP_MN_USER_FS_REQUEST\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;

      case IRP_MN_MOUNT_VOLUME:
	DPRINT("CDFS: IRP_MN_MOUNT_VOLUME\n");
	Status = FsdMountVolume(Irp);
	break;

      case IRP_MN_VERIFY_VOLUME:
	DPRINT("CDFS: IRP_MN_VERIFY_VOLUME\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
#endif

      default:
	DPRINT("CDFS FSC: MinorFunction %d\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


static NTSTATUS
FsdQueryNameInformation(PFILE_OBJECT FileObject,
			PFCB Fcb,
			PDEVICE_OBJECT DeviceObject,
			PFILE_NAME_INFORMATION NameInfo,
			PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file name information
 */
{
  ULONG NameLength;

  assert (NameInfo != NULL);
  assert (Fcb != NULL);

#if 0
  NameLength = wcslen(FCB->PathName) * sizeof(WCHAR);
  if (*BufferLength < sizeof(FILE_NAME_INFORMATION) + NameLength)
    return STATUS_BUFFER_OVERFLOW;

  NameInfo->FileNameLength = NameLength;
  memcpy(NameInfo->FileName,
	 FCB->PathName,
	 NameLength + sizeof(WCHAR));
#endif

  /* Fake name */
  NameLength = 2;
  wcscpy(NameInfo->FileName, L"\\");

  *BufferLength -=
    (sizeof(FILE_NAME_INFORMATION) + NameLength + sizeof(WCHAR));

  return STATUS_SUCCESS;
}

NTSTATUS STDCALL
FsdQueryInformation(PDEVICE_OBJECT DeviceObject,
		    PIRP Irp)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  PFCB Fcb;
  PVOID SystemBuffer;
  ULONG BufferLength;

  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT1("FsdQueryInformation() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileInformationClass = Stack->Parameters.QueryFile.FileInformationClass;
  FileObject = Stack->FileObject;
  Fcb = FileObject->FsContext;

  SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
  BufferLength = Stack->Parameters.QueryFile.Length;

  switch (FileInformationClass)
    {
#if 0
      case FileStandardInformation:
	Status = VfatGetStandardInformation(Fcb,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FilePositionInformation:
      RC = VfatGetPositionInformation(IrpContext->FileObject,
				      FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileBasicInformation:
      RC = VfatGetBasicInformation(FileObject,
				   FCB,
				   DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;
#endif

      case FileNameInformation:
	Status = FsdQueryNameInformation(FileObject,
					 Fcb,
					 DeviceObject,
					 SystemBuffer,
					 &BufferLength);
	break;

#if 0
      case FileInternalInformation:
	Status = FsdGetInternalInformation(Fcb,
					   SystemBuffer,
					   &BufferLength);
	break;

      case FileAlternateNameInformation:
      case FileAllInformation:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
#endif
      default:
	DPRINT("Unimplemented information class %u\n", FileInformationClass);
	Status = STATUS_NOT_SUPPORTED;
    }

  Irp->IoStatus.Status = Status;
  if (NT_SUCCESS(Status))
    Irp->IoStatus.Information =
      Stack->Parameters.QueryFile.Length - BufferLength;
  else
    Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT _DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
  PDEVICE_OBJECT DeviceObject;
  NTSTATUS Status;
  UNICODE_STRING DeviceName;

  DbgPrint("CDFS 0.0.1\n");

  DriverObject = _DriverObject;

  RtlInitUnicodeString(&DeviceName,
		       L"\\Device\\cdfs");
  Status = IoCreateDevice(DriverObject,
			  0,
			  &DeviceName,
			  FILE_DEVICE_CD_ROM_FILE_SYSTEM,
			  0,
			  FALSE,
			  &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DeviceObject->Flags = DO_DIRECT_IO;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsdClose;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = FsdCreate;
  DriverObject->MajorFunction[IRP_MJ_READ] = FsdRead;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = FsdWrite;
  DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
    FsdFileSystemControl;
  DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] =
    FsdDirectoryControl;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
    FsdQueryInformation;
  DriverObject->DriverUnload = NULL;

  IoRegisterFileSystem(DeviceObject);

  return(STATUS_SUCCESS);
}

