#include <ntddk.h>
#include "ramdrv.h"
#include <debug.h>

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
      IoDeleteDevice( DeviceObject );
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  RtlInitUnicodeString( &LinkName, L"\\ArcName\\virt(0)disk(0)ram(0)" );
  IoAssignArcName( &LinkName, &DeviceName );
  RtlInitUnicodeString( &LinkName, L"\\??\\Z:" );
  IoCreateSymbolicLink( &LinkName, &DeviceName );

  RtlInitUnicodeString( &LinkName, L"\\Device\\Floppy0" );
  InitializeObjectAttributes( &objattr,
			      &LinkName,
			      0,
			      0,
			      0 );
  allocsize.u.LowPart = allocsize.u.HighPart = 0;
  
  Status = NtCreateFile( &file,
			 GENERIC_READ | GENERIC_WRITE,
			 &objattr,
			 &iosb,
			 &allocsize,
			 0,
			 0,
			 OPEN_EXISTING,
			 0,
			 0,
			 0 );
  if( !NT_SUCCESS( Status ) )
    {
      DPRINT( "Failed to open floppy\n" );
      return STATUS_SUCCESS;
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
      NtClose( file );
      return STATUS_SUCCESS;
    }
  
  Status = NtReadFile( file,
		       event,
		       0,
		       0,
		       &iosb,
		       devext->Buffer,
		       1440 * 1024,
		       &allocsize,
		       0 );
  if( !NT_SUCCESS( Status ) )
    {
      NtClose( file );
      NtClose( event );
      DPRINT( "Failed to read floppy\n" );
      return STATUS_SUCCESS;
    }
  Status = NtWaitForSingleObject( event, FALSE, 0 );
  if( Status != STATUS_WAIT_0 || !NT_SUCCESS( iosb.Status ) )
    {
      DPRINT( "Failed to read floppy\n" );
      NtClose( file );
      NtClose( event );
      return STATUS_SUCCESS;
    }
  NtClose( file );
  NtClose( event );
  return STATUS_SUCCESS;
}

