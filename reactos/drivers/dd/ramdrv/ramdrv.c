#include <ntddk.h>
#include "ramdrv.h"
#include <debug.h>
#include "../../../lib/bzip2/bzlib.h"

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
  UNICODE_STRING DeviceName;
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
  
  DbgPrint("Ramdisk driver\n");
  
  /* Export other driver entry points... */
  DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdrvDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdrvDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = RamdrvDispatchReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdrvDispatchReadWrite;
  //   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
  //   RamdrvDispatchDeviceControl;
  
  
  // create device and symbolic link
  RtlInitUnicodeString( &DeviceName, L"\\Device\\Ramdisk" );
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
  RtlInitUnicodeString( &LinkName, L"\\ArcName\\virt(0)disk(0)ram(0)" );
  IoAssignArcName( &LinkName, &DeviceName );
  RtlInitUnicodeString( &LinkName, L"\\??\\Z:" );
  IoCreateSymbolicLink( &LinkName, &DeviceName );

  RtlInitUnicodeString( &LinkName, L"\\Device\\Floppy0\\ramdisk.bz2" );
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
  DPRINT( "Read in %d bytes, decompressing now\n", iosb.Information );
  asm( "int3" );
  BZ2_bzBuffToBuffDecompress( devext->Buffer,
			      &dstlen,
			      tbuff,
			      iosb.Information,
			      1,
			      0 );
  DPRINT( "Decompressed\n" );
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

