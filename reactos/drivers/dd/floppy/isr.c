/************************************************************************
 * Interrupt handlers for floppy disk driver, reactos project, created  *
 * by Phillip Susi on 2/25/2001.  This software is publised under the   *
 * GNU General public license.  See the README file for more details    *
 ***********************************************************************/

#include <ddk/ntddk.h>
#define NDEBUG
#include <debug.h>
#include "floppy.h"


// ISR state machine function called when expecting an interrupt due to reset
// During reset, there is nothing ready to read, just issue sense interrupt status

BOOLEAN FloppyIsrDetect( PCONTROLLER_OBJECT Controller )
{
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;
   // Issue read interrupt status, and store the results
   FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_SNS_INTR );
   KeStallExecutionProcessor( 100 );
   ControllerExtension->St0 = FloppyReadDATA( ControllerExtension->PortBase );
   KeStallExecutionProcessor( 100 );
   DeviceExtension->Cyl = FloppyReadDATA( ControllerExtension->PortBase );
   // now queue DPC to set the event
   IoRequestDpc( ControllerExtension->Device,
		 0,
		 Controller );
   return TRUE;
}

// ISR state machine handler for unexpected interrupt
BOOLEAN FloppyIsrUnexpected( PCONTROLLER_OBJECT Controller )
{
   DPRINT( "Unexpected interrupt!\n" );
   return FALSE;
}

BOOLEAN FloppyIsrDetectMedia( PCONTROLLER_OBJECT Controller )
{
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  BYTE SectorSize;
  // media detect in progress, read ID command already issued
  // first, read result registers
  KeStallExecutionProcessor( 1000 );
  ControllerExtension->St0 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 1000 );
  ControllerExtension->St1 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 1000 );
  ControllerExtension->St2 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 1000 );
  FloppyReadDATA( ControllerExtension->PortBase ); // ignore cyl
  KeStallExecutionProcessor( 1000 );
  FloppyReadDATA( ControllerExtension->PortBase ); // ignore head
  KeStallExecutionProcessor( 1000 );
  FloppyReadDATA( ControllerExtension->PortBase ); // ignore sector
  KeStallExecutionProcessor( 1000 );
  SectorSize = FloppyReadDATA( ControllerExtension->PortBase );
  DPRINT( "Sector Size Code: %2x\n", SectorSize );
  DPRINT( "St0 = %2x, St1 = %2x, St2 = %2x\n", ControllerExtension->St0, ControllerExtension->St1, ControllerExtension->St2 );
  DPRINT( "ControllerExtension->Device = %x, ControllerExtension->Irp = %x\n", ControllerExtension->Device, ControllerExtension->Irp );
  // queue DPC
  ControllerExtension->DpcState = FloppyDpcDetectMedia;
  IoRequestDpc( ControllerExtension->Device,
		ControllerExtension->Irp,
		Controller );
  return TRUE;
} 
    
BOOLEAN FloppyIsrRecal( PCONTROLLER_OBJECT Controller )
{
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   // issue sense interrupt status, and read St0 and cyl

   FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_SNS_INTR );
   KeStallExecutionProcessor( 1000 );
   ControllerExtension->St0 = FloppyReadDATA( ControllerExtension->PortBase );
   KeStallExecutionProcessor( 1000 );
   FloppyReadDATA( ControllerExtension->PortBase );  // ignore cyl number
   DPRINT( "Recal St0: %2x\n", ControllerExtension->St0 );

   // If recalibrate worked, issue read ID for each media type untill one works
   if( ControllerExtension->St0 != FLOPPY_ST0_SEEKGD )
      {
	 DPRINT( "Recalibrate failed, ST0 = %2x\n", ControllerExtension->St0 );
	 // queue DPC to fail IRP
	 ControllerExtension->DpcState = FloppyDpcFailIrp;
	 IoRequestDpc( ControllerExtension->Device,
		       ControllerExtension->Irp,
		       Controller );
      }
   else {
     // issue first read id, FloppyIsrDetectMedia will handle
     DPRINT( "Recalibrate worked, issuing read ID mark command\n" );
     ControllerExtension->IsrState = FloppyIsrDetectMedia;
     KeStallExecutionProcessor( 1000 );
     FloppyWriteDATA( ControllerExtension->PortBase, FLOPPY_CMD_RD_ID | FLOPPY_C0M_MFM );
     KeStallExecutionProcessor( 1000 );
     FloppyWriteDATA( ControllerExtension->PortBase, ((PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension)->DriveSelect );
   }
   
   return TRUE;
}

BOOLEAN FloppyIsrReadWrite( PCONTROLLER_OBJECT Controller )
{
  // read result registers from read or write command, and queue dpc to start next operation
  PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
  BYTE Cyl, Head, Sector;
  PFLOPPY_DEVICE_EXTENSION DeviceExtension = (PFLOPPY_DEVICE_EXTENSION)ControllerExtension->Device->DeviceExtension;
  PIO_STACK_LOCATION Stk = IoGetCurrentIrpStackLocation( ControllerExtension->Irp );
  BOOLEAN WriteToDevice = Stk->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE;


  ControllerExtension->St0 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 100 );
  ControllerExtension->St1 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 100 );
  ControllerExtension->St2 = FloppyReadDATA( ControllerExtension->PortBase );
  KeStallExecutionProcessor( 100 );
  Cyl = FloppyReadDATA( ControllerExtension->PortBase );    // cyl
  KeStallExecutionProcessor( 100 );
  Head = FloppyReadDATA( ControllerExtension->PortBase );    // head
  KeStallExecutionProcessor( 100 );
  Sector = FloppyReadDATA( ControllerExtension->PortBase );    // sector
  KeStallExecutionProcessor( 100 );
  ControllerExtension->SectorSizeCode = FloppyReadDATA( ControllerExtension->PortBase );
  // reprogam for next sector if we are not done reading track
  /*  if( ( ControllerExtension->TransferLength -= ( 128 << ControllerExtension->SectorSizeCode ) ) )
    {
      DPRINT1( "ISR reprogramming for next sector: %d\n", Sector );
      Sector++;
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
      FloppyWriteDATA( ControllerExtension->PortBase, ControllerExtension->SectorSizeCode );
      KeStallExecutionProcessor( 100 );
      FloppyWriteDATA( ControllerExtension->PortBase, MediaTypes[DeviceExtension->MediaType].SectorsPerTrack );
      KeStallExecutionProcessor( 100 );
      FloppyWriteDATA( ControllerExtension->PortBase, 0 );
      KeStallExecutionProcessor( 100 );
      FloppyWriteDATA( ControllerExtension->PortBase, 0xFF );
    }
    else */IoRequestDpc( ControllerExtension->Device,
		     ControllerExtension->Irp,
		     Controller );
  return TRUE;
}

// actual ISR, passes controll to handler for current state in state machine

BOOLEAN STDCALL
FloppyIsr(PKINTERRUPT Interrupt,
	  PVOID ServiceContext)
{
   PCONTROLLER_OBJECT Controller = (PCONTROLLER_OBJECT)ServiceContext;
   PFLOPPY_CONTROLLER_EXTENSION ControllerExtension = (PFLOPPY_CONTROLLER_EXTENSION)Controller->ControllerExtension;
   BYTE Byte;

   // need to make sure interrupt is for us, and add some delay for the damn FDC
   // without the delay, even though the thing has interrupted, it's still not ready
   // for us to read the data register.
   KeStallExecutionProcessor( 100 );
   Byte = FloppyReadMSTAT( ControllerExtension->PortBase );
   KeStallExecutionProcessor( 100 );
   if( Byte == 0 )
     {
       DPRINT( "Ignoring interrupt, MSTAT = 0\n" );
       return TRUE;
     }
   return ControllerExtension->IsrState( Controller );
}


