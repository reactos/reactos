/****************************************************************************
 * Defered procedure calls for floppy disk driver, reactos project, created *
 * by Phillip Susi on 2/25/2001.  This software is published under the GNU  *
 * general public license, see the README file for more details             *
 ***************************************************************************/

#include <ddk/ntddk.h>
#include <debug.h>
#include "floppy.h"


VOID FloppyDpc( PKDPC Dpc,
		PDEVICE_OBJECT DeviceObject,
		PIRP Irp,
		PVOID Context )
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   CHECKPOINT1;
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
   CHECKPOINT1;
   KeSetEvent( &ControllerExtension->Event, 0, FALSE );
}

VOID FloppyDpcFailIrp( PKDPC Dpc,
		       PDEVICE_OBJECT DeviceObject,
		       PIRP Irp,
		       PVOID Context )
{
   CHECKPOINT1;
   Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
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

   CHECKPOINT1;
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

   CHECKPOINT1;
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

VOID FloppyDpcReadWrite( PKDPC Dpc,
			 PDEVICE_OBJECT DeviceObject,
			 PIRP Irp,
			 PVOID Context )
{
  PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)Context;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  DWORD SectorSize = 128 << ControllerExtension->SectorSizeCode;

  CHECKPOINT1;
  Irp = ControllerExtension->Irp;
  // if the IO failed, fail the IRP
  if( ControllerExtension->St0 & FLOPPY_ST0_GDMASK )
    {
      Irp->IoStatus.Status = STATUS_DISK_CORRUPT_ERROR;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, 0 );
      CHECKPOINT1;
      return;
    }
  ControllerExtension->CurrentOffset += SectorSize;
  ControllerExtension->CurrentLength -= SectorSize;
  (char *)ControllerExtension->CurrentVa += SectorSize;
  // if there is more IO to be done, restart execute routine to issue next read
  if( ControllerExtension->CurrentLength )
    {
      CHECKPOINT1;
      if( FloppyExecuteReadWrite( DeviceObject,
				  Irp,
				  ControllerExtension->MapRegisterBase,
				  Irp ) == DeallocateObject )
	IoFreeController( Controller );
    }
  else {
    CHECKPOINT1;
    // oetherwise, complete the Irp
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
  CHECKPOINT1;
  // If the read ID failed, fail the irp
  if( ControllerExtension->St1 != 0 )
    {
      DPRINT( "Read ID failed: ST1 = %2x\n", ControllerExtension->St1 );
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
