/****************************************************************************
 * Defered procedure calls for floppy disk driver, reactos project, created *
 * by Phillip Susi on 2/25/2001.  This software is published under the GNU  *
 * general public license, see the README file for more details             *
 ***************************************************************************/

#include <ddk/ntddk.h>
#define NDEBUG
#include <debug.h>
#include "floppy.h"


VOID FloppyDpc( PKDPC Dpc,
		PDEVICE_OBJECT DeviceObject,
		PIRP Irp,
		PVOID Context )
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   ControllerExtension->DpcState( Dpc,
				  DeviceObject,
				  Irp,
				  Context );
}


VOID FloppyDpcDetect( PKDPC Dpc,
		      PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PVOID Context )
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   KeSetEvent( &ControllerExtension->Event, 0, FALSE );
}

VOID FloppyDpcFailIrp( PKDPC Dpc,
		       PDEVICE_OBJECT DeviceObject,
		       PIRP Irp,
		       PVOID Context )
{
   Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
   CHECKPOINT;
   IoCompleteRequest( Irp, 0 );
}

VOID FloppyMotorSpindownDpc( PKDPC Dpc,
			     PVOID Context,
			     PVOID Arg1,
			     PVOID Arg2 )
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;

   // queue call to turn off motor
   IoAllocateController( Controller,
			 ControllerExtension->Device,
			 FloppyExecuteSpindown,
			 ControllerExtension );
}

VOID FloppyMotorSpinupDpc( PKDPC Dpc,
			   PVOID Context,
			   PVOID Arg1,
			   PVOID Arg2 )
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;
   LARGE_INTEGER Timeout;

   Timeout.QuadPart = FLOPPY_MOTOR_SPINDOWN_TIME;
   // Motor has had time to spin up, mark motor as spun up and restart IRP
   // don't forget to set the spindown timer
   KeSetTimer( &ControllerExtension->SpinupTimer,
	       Timeout,
	       &ControllerExtension->MotorSpindownDpc );
   DPRINT( "Motor spun up, retrying operation\n" );
   ControllerExtension->MotorOn = DeviceExtension->DriveSelect;
   IoFreeController( Controller );
   IoAllocateController( Controller,
			 ControllerExtension->Device,
			 FloppyExecuteReadWrite,
			 ControllerExtension->Irp );
}

VOID FloppySeekDpc( PKDPC Dpc,
		    PDEVICE_OBJECT DeviceObject,
		    PIRP Irp,
		    PVOID Context )
{
  PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)DeviceExtension->Controller->ControllerExtension;

  // if the seek failed, fail the IRP
  if( ControllerExtension->St0 & FLOPPY_ST0_GDMASK )
    {
      ControllerExtension->Irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
      ControllerExtension->Irp->IoStatus.Information = 0;
      DPRINT( "Failing IRP: St0 = %2x, St1 = %2x, St2 = %2x\n",
	      ControllerExtension->St0,
	      ControllerExtension->St1,
	      ControllerExtension->St2 );
      for(;;);
      IoCompleteRequest( ControllerExtension->Irp, 0 );
      IoFreeController( DeviceExtension->Controller );
      return;
    }
  KeStallExecutionProcessor( 10000 );
  DPRINT( "Seek completed, now on cyl %2x\n", DeviceExtension->Cyl );
  // now that we are on the right cyl, restart the read
  if( FloppyExecuteReadWrite( DeviceObject,
			      ControllerExtension->Irp,
			      ControllerExtension->MapRegisterBase,
			      ControllerExtension->Irp ) == DeallocateObject )
    IoFreeController( DeviceExtension->Controller );
}

VOID FloppyDpcReadWrite( PKDPC Dpc,
			 PDEVICE_OBJECT DeviceObject,
			 PIRP Irp,
			 PVOID Context )
{
  PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;
  DWORD SectorSize = 128 << ControllerExtension->SectorSizeCode;
  PIO_STACK_LOCATION Stk = IoGetCurrentIrpStackLocation( ControllerExtension->Irp );
  BOOLEAN WriteToDevice = Stk->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE;

  Irp = ControllerExtension->Irp;
  // if the IO failed, fail the IRP
  if( ControllerExtension->St0 & FLOPPY_ST0_GDMASK )
    {
      Irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
      Irp->IoStatus.Information = 0;
      DPRINT( "Failing IRP: St0 = %2x, St1 = %2x, St2 = %2x\n",
	      ControllerExtension->St0,
	      ControllerExtension->St1,
	      ControllerExtension->St2 );
      for(;;);
      IoCompleteRequest( Irp, 0 );
      IoFreeController( Controller );
      return;
    }
  // don't forget to flush the buffers
  IoFlushAdapterBuffers( ControllerExtension->AdapterObject,
			 ControllerExtension->Irp->MdlAddress,
			 ControllerExtension->MapRegisterBase,
			 ControllerExtension->Irp->Tail.Overlay.DriverContext[0],
			 ControllerExtension->TransferLength,
			 WriteToDevice );
  DPRINT( "St0 = %2x, St1  %2x, St2 = %2x\n",
	  ControllerExtension->St0,
	  ControllerExtension->St1,
	  ControllerExtension->St2 );
  // update buffer info
  Stk->Parameters.Read.ByteOffset.u.LowPart += ControllerExtension->TransferLength;
  Stk->Parameters.Read.Length -= ControllerExtension->TransferLength;
  // drivercontext used for current va
  (DWORD)ControllerExtension->Irp->Tail.Overlay.DriverContext[0] += ControllerExtension->TransferLength;
			 
  DPRINT( "First dword: %x\n", *((DWORD *)ControllerExtension->MapRegisterBase) )

  // if there is more IO to be done, restart execute routine to issue next read
  if( Stk->Parameters.Read.Length )
    {
      if( FloppyExecuteReadWrite( DeviceObject,
				  Irp,
				  ControllerExtension->MapRegisterBase,
				  Irp ) == DeallocateObject )
	IoFreeController( Controller );
    }
  else {
    IoFreeController( Controller );
    // otherwise, complete the Irp
    IoCompleteRequest( Irp, 0 );
  }
}
VOID FloppyDpcDetectMedia( PKDPC Dpc,
			   PDEVICE_OBJECT DeviceObject,
			   PIRP Irp,
			   PVOID Context )
{
  PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  // If the read ID failed, fail the irp
  if( ControllerExtension->St1 != 0 )
    {
      DPRINT1( "Read ID failed: ST1 = %2x\n", ControllerExtension->St1 );
      Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
      IoCompleteRequest( Irp, 0 );
      return;
    }
  // set media type, and restart the IRP from the beginning
  ((PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension)->MediaType = 0;
  DPRINT( "Media detected, restarting IRP\n" );
  // don't forget to free the controller so that the now queued routine may execute
  IoFreeController( Controller );

  IoAllocateController( Controller,
			DeviceObject,
			FloppyExecuteReadWrite,
			Irp );
}
