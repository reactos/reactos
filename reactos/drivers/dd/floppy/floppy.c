/*
 *  FLOPPY.C - NEC-765/8272A floppy device driver
 *   written by Rex Jolliff
 *    with help from various other sources, including but not limited to:
 *    Art Baker's NT Device Driver Book, Linux Source, and the internet.
 *
 *  Modification History:
 *    08/19/98  RJJ  Created.
 *    01/31/01  PJS  Heavy rewrite, most of code thrown out
 *
 *  To do:
 * FIXME: get it working
 * FIXME: add support for DMA hardware
 * FIXME: should add support for floppy tape/zip devices
 */

#include <ddk/ntddk.h>

#include "floppy.h"
#define NDEBUG
#include <debug.h>


FLOPPY_CONTROLLER_PARAMETERS ControllerParameters[FLOPPY_MAX_CONTROLLERS] = 
{
  {0x03f0, 6, 6, 2, 6, LevelSensitive, 0xffff}
  //  {0x0370, 6, 6, 6, LevelSensitive, 0xffff},
};

const FLOPPY_MEDIA_TYPE MediaTypes[] = {
   { 0x02, 80, 2, 18, 512 },
   { 0, 0, 0, 0, 0 } };


static BOOLEAN
FloppyCreateController(PDRIVER_OBJECT DriverObject,
                       PFLOPPY_CONTROLLER_PARAMETERS ControllerParameters,
                       int Index)
{
   PCONTROLLER_OBJECT ControllerObject;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension;
   PFLOPPY_DEVICE_EXTENSION DeviceExtension;
   UNICODE_STRING DeviceName;
   UNICODE_STRING arcname;
   UNICODE_STRING SymlinkName;
   NTSTATUS Status;
   PDEVICE_OBJECT DeviceObject;
   PCONFIGURATION_INFORMATION ConfigInfo;
   LARGE_INTEGER Timeout;
   BYTE Byte;
   int c;
   PCONFIGURATION_INFORMATION Config;
   DEVICE_DESCRIPTION DeviceDescription;
   ULONG MaxMapRegs;
   
   /* FIXME: Register port ranges and interrupts with HAL */

   /*  Create controller object for FDC  */
   ControllerObject = IoCreateController(sizeof(FLOPPY_CONTROLLER_EXTENSION));
   if (ControllerObject == NULL)
     {
	DPRINT("Could not create controller object for controller %d\n",
	       Index);
	return FALSE;
     }
   
   /* FIXME: fill out controller data */
   ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)
     ControllerObject->ControllerExtension;
   ControllerExtension->Number = Index;
   ControllerExtension->PortBase = ControllerParameters->PortBase;
   ControllerExtension->Vector = ControllerParameters->Vector;
   KeInitializeEvent( &ControllerExtension->Event, SynchronizationEvent, FALSE );
   ControllerExtension->Device = 0;  // no active device
   ControllerExtension->Irp = 0;     // no active IRP
   /*  Initialize the spin lock in the controller extension  */
   KeInitializeSpinLock(&ControllerExtension->SpinLock);
   ControllerExtension->IsrState = FloppyIsrDetect;
   ControllerExtension->DpcState = FloppyDpcDetect;
   
   /*  Register an interrupt handler for this controller  */
   Status = IoConnectInterrupt(&ControllerExtension->Interrupt,
			       FloppyIsr, 
			       ControllerObject, 
			       &ControllerExtension->SpinLock, 
			       ControllerExtension->Vector, 
			       ControllerParameters->IrqL, 
			       ControllerParameters->SynchronizeIrqL, 
			       ControllerParameters->InterruptMode, 
			       FALSE, 
			       ControllerParameters->Affinity, 
			       FALSE);
   if (!NT_SUCCESS(Status)) 
     {
	DPRINT("Could not Connect Interrupt %d\n", 
	       ControllerExtension->Vector);
	goto controllercleanup;
     }

   
   /* setup DMA stuff for controller */
   DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
   DeviceDescription.Master = FALSE;
   DeviceDescription.ScatterGather = FALSE;
   DeviceDescription.AutoInitialize = FALSE;
   DeviceDescription.Dma32BitAddress = FALSE;
   DeviceDescription.DmaChannel = ControllerParameters->DmaChannel;
   DeviceDescription.InterfaceType = Isa;
   //   DeviceDescription.DmaWidth = Width8Bits;
   ControllerExtension->AdapterObject = HalGetAdapter( &DeviceDescription, &MaxMapRegs );
   if( ControllerExtension->AdapterObject == NULL )
      {
	 DPRINT1( "Could not get adapter object\n" );
	 goto interruptcleanup;
      }
			     
#if 0
   /*  Check for each possible drive and create devices for them */
   for (DriveIdx = 0; DriveIdx < FLOPPY_MAX_DRIVES; DriveIdx++)
     {
	/* FIXME: try to identify the drive */
	/* FIXME: create a device if it's there */
     }
#endif
    
   /* FIXME: Let's assume one drive and one controller for the moment */
   RtlInitUnicodeString(&DeviceName, L"\\Device\\Floppy0");
   Status = IoCreateDevice(DriverObject,
			   sizeof(FLOPPY_DEVICE_EXTENSION),
			   &DeviceName,
			   FILE_DEVICE_DISK,
			   FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	goto interruptcleanup;
     }
   DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   DeviceExtension->DriveSelect = 0;
   DeviceExtension->Controller = ControllerObject;
   DeviceExtension->MediaType = ~0;
   ControllerExtension->MotorOn = ~0;
   // set up DPC
   ControllerExtension->Device = DeviceObject;
   KeInitializeDpc( &ControllerExtension->MotorSpinupDpc,
		    FloppyMotorSpinupDpc,
		    ControllerObject );
   KeInitializeDpc( &ControllerExtension->MotorSpindownDpc,
		    FloppyMotorSpindownDpc,
		    ControllerObject );
   KeInitializeTimer( &ControllerExtension->SpinupTimer );
   IoInitializeDpcRequest( DeviceObject, FloppyDpc );
   // reset controller and wait for interrupt
   DPRINT( "Controller Off\n" );
   FloppyWriteDOR( ControllerExtension->PortBase, 0 );
   // let controller reset for at least FLOPPY_RESET_TIME
   KeStallExecutionProcessor( FLOPPY_RESET_TIME );
   DPRINT( "Controller On\n" ); 
   FloppyWriteDOR( ControllerExtension->PortBase, FLOPPY_DOR_ENABLE | FLOPPY_DOR_DMA );
   // wait for interrupt now
   Timeout.QuadPart = -10000000;
   Status = KeWaitForSingleObject( &ControllerExtension->Event,
				   Executive,
				   KernelMode,
				   FALSE,
				   &Timeout );
   if( Status != STATUS_WAIT_0 )
      {
	 DPRINT1( "Error: KeWaitForSingleObject returned: %x\n", Status );
	 goto devicecleanup;
      }
   // set for high speed mode
   //   FloppyWriteCCNTL( ControllerExtension->PortBase, FLOPPY_CCNTL_1MBIT );
   
   // ok, so we have an FDC, now check for drives
   // aparently the sense drive status command does not work on any FDC I can find
   // so instead we will just have to assume a 1.44 meg 3.5 inch floppy.  At some
   // point we should get the bios disk parameters passed in to the kernel at boot
   // and stored in the HARDWARE registry key for us to pick up here.

   // turn on motor, wait for spinup time, and recalibrate the drive
   FloppyWriteDOR( ControllerExtension->PortBase, FLOPPY_DRIVE0_ON );
   Timeout.QuadPart = FLOPPY_MOTOR_SPINUP_TIME;
   KeDelayExecutionThread( KernelMode, FALSE, &Timeout );
   DPRINT( "MSTAT: %2x\n", FloppyReadMSTAT( ControllerExtension->PortBase ) );
   FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_RECAL );
   DPRINT( "MSTAT: %2x\n", FloppyReadMSTAT( ControllerExtension->PortBase ) );
   KeStallExecutionProcessor( 10000 );
   FloppyWriteDATA( ControllerExtension->PortBase, 0 ); // drive select
   Timeout.QuadPart = FLOPPY_RECAL_TIMEOUT;
   Status = KeWaitForSingleObject( &ControllerExtension->Event,
				   Executive,
				   KernelMode,
				   FALSE,
				   &Timeout );
   if( Status != STATUS_WAIT_0 )
      {
	 DPRINT1( "Error: KeWaitForSingleObject returned: %x\n", Status );
	 goto devicecleanup;
      }
   if( ControllerExtension->St0 != FLOPPY_ST0_SEEKGD )
     {
       DbgPrint( "Floppy: error recalibrating drive, ST0: %2x\n", (DWORD)ControllerExtension->St0 );
       goto interruptcleanup;
     }
   DeviceExtension->Cyl = 0;
   // drive is good, and it is now on track 0, turn off the motor
   FloppyWriteDOR( ControllerExtension->PortBase, FLOPPY_DOR_ENABLE | FLOPPY_DOR_DMA );
   /* Initialize the device */
   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceObject->AlignmentRequirement = FILE_512_BYTE_ALIGNMENT;
   // change state machine, no interrupt expected
   ControllerExtension->IsrState = FloppyIsrUnexpected;
   Config = IoGetConfigurationInformation();
   Config->FloppyCount++;
   // call IoAllocateAdapterChannel, and wait for execution routine to be given the channel
   CHECKPOINT;
   Status = IoAllocateAdapterChannel( ControllerExtension->AdapterObject,
				      DeviceObject,
				      0x3000/PAGESIZE,  // max track size is 12k
				      FloppyAdapterControl,
				      ControllerExtension );
   if( !NT_SUCCESS( Status ) )
     {
       DPRINT1( "Error: IoAllocateAdapterChannel returned %x\n", Status );
       goto interruptcleanup;
     }
   CHECKPOINT;
   Status = KeWaitForSingleObject( &ControllerExtension->Event,
				   Executive,
				   KernelMode,
				   FALSE,
				   &Timeout );
   CHECKPOINT;
   if( Status != STATUS_WAIT_0 )
      {
	 DPRINT1( "Error: KeWaitForSingleObject returned: %x\n", Status );
	 goto interruptcleanup;
      }
   // Ok, we own the adapter object, from now on we can just IoMapTransfer, and not
   // bother releasing the adapter ever.

   RtlInitUnicodeString( &arcname, L"\\ArcName\\multi(0)disk(0)fdisk(0)" );
   Status = IoAssignArcName( &arcname, &DeviceName );
   DbgPrint( "Floppy drive initialized\n" );
   return TRUE;

 devicecleanup:
   IoDeleteDevice( DeviceObject );
 interruptcleanup:
   IoDisconnectInterrupt(ControllerExtension->Interrupt);
 controllercleanup:
   // turn off controller
   FloppyWriteDOR( ControllerExtension->PortBase, 0 );
   IoDeleteController(ControllerObject);
   for(;;);
   return FALSE;
}

IO_ALLOCATION_ACTION STDCALL
FloppyExecuteSpindown(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PVOID MapRegisterbase,
		      PVOID Context)
{
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension= (PFLOPPY_CONTROLLER_EXTENSION)Context;

   // turn off motor, and return
   DPRINT( "Spinning down motor\n" );
   ControllerExtension->MotorOn = ~0;
   FloppyWriteDOR( ControllerExtension->PortBase,
		   FLOPPY_DOR_ENABLE | FLOPPY_DOR_DMA );
   return DeallocateObject;
}

IO_ALLOCATION_ACTION STDCALL
FloppyExecuteReadWrite(PDEVICE_OBJECT DeviceObject,
		       PIRP Irp,
		       PVOID MapRegisterbase,
		       PVOID Context)
{
   PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)DeviceExtension->Controller->ControllerExtension;
   LARGE_INTEGER Timeout;
   BOOLEAN WriteToDevice;
   DWORD Cyl, Sector, Head;
   PIO_STACK_LOCATION Stk;
   DWORD Length;

   ControllerExtension->Irp = Irp = (PIRP)Context;
   Stk = IoGetCurrentIrpStackLocation( Irp );
   ControllerExtension->Device = DeviceObject;
   Timeout.QuadPart = FLOPPY_MOTOR_SPINUP_TIME;
   DPRINT( "FloppyExecuteReadWrite()\n" );
   CHECKPOINT;
   WriteToDevice = Stk->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE;
   // verify drive is spun up and selected
   if( ControllerExtension->MotorOn != DeviceExtension->DriveSelect )
      {
	 // turn on and select drive, and allow it to spin up
	 // FloppyMotorSpinupDpc will restart this operation once motor is spun up
	 DPRINT( "Motor not on, turning it on now\n" );
	 FloppyWriteDOR( ControllerExtension->PortBase,
			 DeviceExtension->DriveSelect ? FLOPPY_DRIVE1_ON : FLOPPY_DRIVE0_ON );
	 // cancel possible spindown timer first
	 KeCancelTimer( &ControllerExtension->SpinupTimer );
	 KeSetTimerEx( &ControllerExtension->SpinupTimer,
		       Timeout,
		       0,
		       &ControllerExtension->MotorSpinupDpc );
	 return KeepObject;
      }
   else {
     Timeout.QuadPart = FLOPPY_MOTOR_SPINDOWN_TIME;
     // motor is already spinning, so reset the spindown timer
     KeCancelTimer( &ControllerExtension->SpinupTimer );
     KeSetTimer( &ControllerExtension->SpinupTimer,
		 Timeout,
		 &ControllerExtension->MotorSpindownDpc );
   }
   // verify media content
   if( FloppyReadDIR( ControllerExtension->PortBase ) & FLOPPY_DI_DSKCHNG )
      {
	 // No disk is in the drive
	 DPRINT( "No disk is in the drive\n" );
	 Irp->IoStatus.Status = STATUS_NO_MEDIA;
	 Irp->IoStatus.Information = 0;
	 IoCompleteRequest( Irp, 0 );
	 return DeallocateObject;
      }
   if( DeviceExtension->MediaType == ~0 )
      {
	 // media is in disk, but we have not yet detected what kind it is,
	 // so detect it now
	 // First, we need to recalibrate the drive though
	 ControllerExtension->IsrState = FloppyIsrRecal;
	 DPRINT( "Recalibrating drive\n" );
	 KeStallExecutionProcessor( 1000 );
	 FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_RECAL );
	 KeStallExecutionProcessor( 1000 );
	 FloppyWriteDATA( ControllerExtension->PortBase, DeviceExtension->DriveSelect );
	 return KeepObject;
      }
   // looks like we have media in the drive.... do the read
   // first, calculate geometry for read
   Sector = Stk->Parameters.Read.ByteOffset.u.LowPart / MediaTypes[DeviceExtension->MediaType].BytesPerSector;
   // absolute sector right now
   Cyl = Sector / MediaTypes[DeviceExtension->MediaType].SectorsPerTrack;
   DPRINT( "Sector = %x, Offset = %x, Cyl = %x, Heads = %x MediaType = %x\n", Sector, Stk->Parameters.Read.ByteOffset.u.LowPart, (DWORD)Cyl, (DWORD)MediaTypes[DeviceExtension->MediaType].Heads, (DWORD)DeviceExtension->MediaType );
   Head = Cyl % MediaTypes[DeviceExtension->MediaType].Heads;
   DPRINT( "Head = %2x\n", Head );
   // convert absolute cyl to relative
   Cyl /= MediaTypes[DeviceExtension->MediaType].Heads;
   // convert absolute sector to relative
   Sector %= MediaTypes[DeviceExtension->MediaType].SectorsPerTrack;
   Sector++;  // track relative sector numbers are 1 based, not 0 based
   DPRINT( "Cyl = %2x, Head = %2x, Sector = %2x\n", Cyl, Head, Sector );

   // seek if we need to seek
   if( DeviceExtension->Cyl != Cyl )
     {
       DPRINT( "Seeking...\n" );
       ControllerExtension->IsrState = FloppyIsrDetect;
       ControllerExtension->DpcState = FloppySeekDpc;
       FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_SEEK );
       KeStallExecutionProcessor( 100 );
       FloppyWriteDATA( ControllerExtension->PortBase, DeviceExtension->DriveSelect );
       KeStallExecutionProcessor( 100 );
       FloppyWriteDATA( ControllerExtension->PortBase, Cyl );
       return KeepObject;
     }
   //set up DMA and issue read command
   Length = MediaTypes[DeviceExtension->MediaType].SectorsPerTrack - Sector + 1;
   // number of sectors untill end of track
   Length *= 512;   // convert to bytes
   if( Length > Stk->Parameters.Read.Length )
     Length = Stk->Parameters.Read.Length;
   DPRINT( "Sector: %d, Length: %d\n", Sector, Length );
   ControllerExtension->TransferLength = Length;
   IoMapTransfer( ControllerExtension->AdapterObject,
		  Irp->MdlAddress,
		  ControllerExtension->MapRegisterBase,
		  Irp->Tail.Overlay.DriverContext[0], // current va
		  &Length,
		  WriteToDevice );
   ControllerExtension->IsrState = FloppyIsrReadWrite;
   ControllerExtension->DpcState = FloppyDpcReadWrite;
   CHECKPOINT;
   FloppyWriteDATA( ControllerExtension->PortBase, WriteToDevice ? FLOPPY_CMD_WRITE : FLOPPY_CMD_READ );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, ( Head << 2 ) | DeviceExtension->DriveSelect );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, Cyl );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, Head );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, Sector );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, MediaTypes[DeviceExtension->MediaType].SectorSizeCode );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, MediaTypes[DeviceExtension->MediaType].SectorsPerTrack );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, 0 );
   KeStallExecutionProcessor( 100 );
   FloppyWriteDATA( ControllerExtension->PortBase, 0xFF );
   CHECKPOINT;
   // eventually, the FDC will interrupt and we will read results then
   return KeepObject;
}

NTSTATUS STDCALL
FloppyDispatchOpenClose(PDEVICE_OBJECT DeviceObject,
			PIRP Irp)
{
   DPRINT("FloppyDispatchOpenClose\n");
   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
FloppyDispatchReadWrite(PDEVICE_OBJECT DeviceObject,
		        PIRP Irp)
{
  PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)DeviceExtension->Controller->ControllerExtension;
  PIO_STACK_LOCATION Stk = IoGetCurrentIrpStackLocation( Irp );
  KIRQL oldlvl;
  
  if( Stk->Parameters.Read.ByteOffset.u.HighPart )
    {
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, 1 );
      return STATUS_INVALID_PARAMETER;
    }
  // store currentva in drivercontext
  Irp->Tail.Overlay.DriverContext[0] = MmGetMdlVirtualAddress( Irp->MdlAddress );
  DPRINT( "FloppyDispatchReadWrite: offset = %x, length = %x, va = %x\n",
	  Stk->Parameters.Read.ByteOffset.u.LowPart,
	  Stk->Parameters.Read.Length,
	  Irp->Tail.Overlay.DriverContext[0] );
  // Queue IRP
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = Stk->Parameters.Read.Length;
  IoMarkIrpPending( Irp );
  KeRaiseIrql( DISPATCH_LEVEL, &oldlvl );
  IoAllocateController( ((PFLOPPY_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Controller,
			DeviceObject,
			FloppyExecuteReadWrite,
			Irp );
  KeLowerIrql( oldlvl );
  DPRINT( "oldlvl = %x\n", oldlvl );
  return STATUS_PENDING;
}

IO_ALLOCATION_ACTION STDCALL
FloppyAdapterControl(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp,
		     PVOID MapRegisterBase,
		     PVOID Context)
{
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Context;

  // just set the event, and return KeepObject
  CHECKPOINT;
  ControllerExtension->MapRegisterBase = MapRegisterBase;
  KeSetEvent( &ControllerExtension->Event, 0, FALSE );
  return KeepObject;
}

NTSTATUS STDCALL
FloppyDispatchDeviceControl(PDEVICE_OBJECT DeviceObject,
			    PIRP Irp)
{
   DPRINT("FloppyDispatchDeviceControl\n");
   return(STATUS_UNSUCCESSFUL);
}

/*    ModuleEntry
 *
 *  DESCRIPTION:
 *    This function initializes the driver, locates and claims 
 *    hardware resources, and creates various NT objects needed
 *    to process I/O requests.
 *
 *  RUN LEVEL:
 *    PASSIVE_LEVEL
 *
 *  ARGUMENTS:
 *    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
 *                                       for this driver
 *    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
 *                                       key
 *
 *  RETURNS:
 *    NTSTATUS  
 */
NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
   DbgPrint("Floppy driver\n");
   
   /* Export other driver entry points... */
   DriverObject->MajorFunction[IRP_MJ_CREATE] = FloppyDispatchOpenClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = FloppyDispatchOpenClose;
   DriverObject->MajorFunction[IRP_MJ_READ] = FloppyDispatchReadWrite;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = FloppyDispatchReadWrite;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
     FloppyDispatchDeviceControl;
   
   /*  Try to detect controller and abort if it fails */
   if (!FloppyCreateController(DriverObject,
			       &ControllerParameters[0],
			       0))
     {
	DPRINT("Could not find floppy controller\n");
	return STATUS_NO_SUCH_DEVICE;
     }
   
   return STATUS_SUCCESS;
}

/* EOF */

