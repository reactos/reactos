#include <ddk/ntddk.h>
#include "ramdrv.h"
#include <debug.h>
#include "../../lib/bzip2/bzlib.h"

NTSTATUS STDCALL RamdrvDispatchDeviceControl(PDEVICE_OBJECT DeviceObject,
					     PIRP Irp)
{
   PIO_STACK_LOCATION IrpStack;
   ULONG ControlCode, InputLength, OutputLength;
   NTSTATUS Status;

   DPRINT("RamdrvDispatchDeviceControl\n");

   IrpStack = IoGetCurrentIrpStackLocation(Irp);
   ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
   InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
   OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

   switch (ControlCode)
   {
      case IOCTL_DISK_GET_DRIVE_GEOMETRY:
         if (OutputLength < sizeof(DISK_GEOMETRY))
         {
            Status = STATUS_INVALID_PARAMETER;
	 }
	 else
	 {
            PDISK_GEOMETRY Geometry = Irp->AssociatedIrp.SystemBuffer;
            Geometry->MediaType = F3_1Pt44_512;
            Geometry->Cylinders.QuadPart = 80;
            Geometry->TracksPerCylinder = 2 * 18;
            Geometry->SectorsPerTrack = 18;
            Geometry->BytesPerSector = 512;
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        }
        break;
     default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
   }
   Irp->IoStatus.Status = Status;
   IoCompleteRequest(Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);	
   return Status;
}

NTSTATUS STDCALL RamdrvDispatchReadWrite(PDEVICE_OBJECT DeviceObject,
					 PIRP Irp)
{
  PRAMDRV_DEVICE_EXTENSION devext = (PRAMDRV_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION Stk = IoGetCurrentIrpStackLocation( Irp );

  if( Stk->Parameters.Read.ByteOffset.u.HighPart ||
      Stk->Parameters.Read.ByteOffset.u.LowPart >= devext->Size )
    {
      Irp->IoStatus.Status = STATUS_END_OF_FILE;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, 0 );
      return STATUS_END_OF_FILE;
    }
  if( (Stk->Parameters.Read.ByteOffset.u.LowPart + Stk->Parameters.Read.Length) > devext->Size )
    Stk->Parameters.Read.Length = devext->Size - Stk->Parameters.Read.ByteOffset.u.LowPart;
  if( Stk->MajorFunction == IRP_MJ_READ )
    RtlCopyMemory( MmGetSystemAddressForMdl( Irp->MdlAddress ),
		   devext->Buffer + Stk->Parameters.Read.ByteOffset.u.LowPart,
		   Stk->Parameters.Read.Length );
  else RtlCopyMemory( devext->Buffer + Stk->Parameters.Read.ByteOffset.u.LowPart,
		      MmGetSystemAddressForMdl( Irp->MdlAddress ),
		      Stk->Parameters.Read.Length );
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = Stk->Parameters.Read.Length;
  IoCompleteRequest( Irp, 0 );
  return STATUS_SUCCESS;
}

NTSTATUS STDCALL RamdrvDispatchOpenClose(PDEVICE_OBJECT DeviceObject,
					 PIRP Irp)
{
   DPRINT("RamdrvDispatchOpenClose\n");
   return STATUS_SUCCESS;
}

NTSTATUS STDCALL DriverEntry(IN PDRIVER_OBJECT DriverObject,
			     IN PUNICODE_STRING RegistryPath)
{
  UNICODE_STRING DeviceName = UNICODE_STRING_INITIALIZER(L"\\Device\\Ramdisk");
  NTSTATUS Status;
  PDEVICE_OBJECT DeviceObject;
  PRAMDRV_DEVICE_EXTENSION devext;
  UNICODE_STRING LinkName;
  HANDLE file;
  OBJECT_ATTRIBUTES objattr;
  IO_STATUS_BLOCK iosb;
  LARGE_INTEGER allocsize;
  HANDLE event;
  void *tbuff;
  unsigned int dstlen = 1024 * 1440;
  FILE_STANDARD_INFORMATION finfo;
  DWORD err;
  
  DPRINT("Ramdisk driver\n");
  
  /* Export other driver entry points... */
  DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdrvDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdrvDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = RamdrvDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdrvDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdrvDispatchDeviceControl;
  
  
  // create device and symbolic link
  Status = IoCreateDevice( DriverObject,
			   sizeof( RAMDRV_DEVICE_EXTENSION ),
			   &DeviceName,
			   FILE_DEVICE_DISK,
			   0,
			   FALSE,
			   &DeviceObject );
  if( !NT_SUCCESS( Status ) )
    return Status;
  DeviceObject->Flags |= DO_DIRECT_IO;
  devext = (PRAMDRV_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  devext->Size = 1440 * 1024;
  devext->Buffer = ExAllocatePool( PagedPool, devext->Size );
  if( !devext->Buffer )
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto cleandevice;
    }
  RtlInitUnicodeStringFromLiteral( &LinkName, L"\\??\\Z:" );
  IoCreateSymbolicLink( &LinkName, &DeviceName );

  RtlInitUnicodeStringFromLiteral( &LinkName, L"\\Device\\Floppy0\\ramdisk.bz2" );
  InitializeObjectAttributes( &objattr,
			      &LinkName,
			      0,
			      0,
			      0 );
  allocsize.u.LowPart = allocsize.u.HighPart = 0;

  Status = NtOpenFile( &file,
			 GENERIC_READ,
			 &objattr,
			 &iosb,
		         FILE_SHARE_READ,
			 FILE_NO_INTERMEDIATE_BUFFERING );

  if( !NT_SUCCESS( Status ) )
    {
      DPRINT( "Failed to open floppy\n" );
      goto cleanbuffer;
    }

  InitializeObjectAttributes( &objattr,
			      0,
			      0,
			      0,
			      0 );
  Status = NtCreateEvent( &event,
			  0,
			  &objattr,
			  TRUE,
			  FALSE );
  if( !NT_SUCCESS( Status ) )
    {
      DPRINT( "Failed to create event\n" );
      goto cleanfile;
    }

  Status = NtQueryInformationFile( file,
				   &iosb,
				   &finfo,
				   sizeof( finfo ),
				   FileStandardInformation );

  if( !NT_SUCCESS( Status ) )
    {
      DPRINT1( "Failed to query file information\n" );
      goto cleanevent;
    }
  tbuff = ExAllocatePool( PagedPool, finfo.EndOfFile.u.LowPart );
  if( !tbuff )
    {
      DPRINT1( "Failed to allocate buffer of size %d\n", finfo.EndOfFile.u.LowPart );
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto cleanevent;
    }

  Status = NtReadFile( file,
		       event,
		       0,
		       0,
		       &iosb,
		       tbuff,
		       finfo.EndOfFile.u.LowPart,
		       &allocsize,
		       0 );

  if( !NT_SUCCESS( Status ) )
    {
      DPRINT( "Failed to read floppy\n" );
      goto cleantbuff;
    }
  Status = NtWaitForSingleObject( event, FALSE, 0 );
  if( Status != STATUS_WAIT_0 || !NT_SUCCESS( iosb.Status ) )
    {
      DPRINT( "Failed to read floppy\n" );
      goto cleantbuff;
    }
  DPRINT( "RAMDRV: Read in %d bytes, decompressing now\n", iosb.Information );
  err = BZ2_bzBuffToBuffDecompress( devext->Buffer,
				    &dstlen,
				    tbuff,
				    iosb.Information,
				    1,
				    0 );
  if( err == 0 )
  {
    DPRINT( "RAMDRV: Image Decompressed\n");
  }
  else DbgPrint( "RAMDRV: Failed to decomparess image, error: %d\n", err );
  ExFreePool( tbuff );
  NtClose( file );
  NtClose( event );
  return STATUS_SUCCESS;

 cleantbuff:
  ExFreePool( tbuff );
 cleanevent:
  NtClose( event );
 cleanfile:
  NtClose( file );
 cleanbuffer:
  ExFreePool( devext->Buffer );

 cleandevice:
  IoDeleteDevice( DeviceObject );
  for(;;);

  return Status;
}

