/* $Id: atapi.c,v 1.6 2002/02/03 20:21:13 ekohl Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS ATAPI miniport driver
 * FILE:        services/storage/atapi/atapi.c
 * PURPOSE:     ATAPI miniport driver
 * PROGRAMMERS: Eric Kohl (ekohl@rz-online.de)
 * REVISIONS:
 *              09-09-2001 Created
 */

/*
 * Note:
 *   This driver is derived from Rex Jolliff's ide driver. Lots of his
 *   routines are still in here although they belong into the higher level
 *   drivers. They will be moved away as soon as possible.
 */

/*
 * TODO:
 *	- implement sending of atapi commands
 *	- handle removable atapi non-cdrom drives
 */

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>

#include "../include/srb.h"
#include "../include/scsi.h"
#include "../include/ntddscsi.h"

#include "atapi.h"

#define NDEBUG
#include <debug.h>

#define  VERSION  "V0.0.1"


//  -------------------------------------------------------  File Static Data

//    ATAPI_MINIPORT_EXTENSION
//
//  DESCRIPTION:
//    Extension to be placed in each port device object
//
//  ACCESS:
//    Allocated from NON-PAGED POOL
//    Available at any IRQL
//

typedef struct _ATAPI_MINIPORT_EXTENSION
{
  IDE_DRIVE_IDENTIFY DeviceParams[2];
  BOOLEAN DevicePresent[2];
  BOOLEAN DeviceAtapi[2];

  ULONG CommandPortBase;
  ULONG ControlPortBase;

  BOOLEAN ExpectingInterrupt;
  PSCSI_REQUEST_BLOCK CurrentSrb;

  PUSHORT DataBuffer;
} ATAPI_MINIPORT_EXTENSION, *PATAPI_MINIPORT_EXTENSION;


typedef struct _UNIT_EXTENSION
{
  ULONG Dummy;
} UNIT_EXTENSION, *PUNIT_EXTENSION;


//  -----------------------------------------------  Discardable Declarations

#ifdef  ALLOC_PRAGMA

//  make the initialization routines discardable, so that they 
//  don't waste space

#pragma  alloc_text(init, DriverEntry)
#pragma  alloc_text(init, IDECreateController)
#pragma  alloc_text(init, IDEPolledRead)

//  make the PASSIVE_LEVEL routines pageable, so that they don't
//  waste nonpaged memory

#pragma  alloc_text(page, IDEShutdown)
#pragma  alloc_text(page, IDEDispatchOpenClose)
#pragma  alloc_text(page, IDEDispatchRead)
#pragma  alloc_text(page, IDEDispatchWrite)

#endif  /*  ALLOC_PRAGMA  */

//  ---------------------------------------------------- Forward Declarations

static ULONG STDCALL
AtapiFindCompatiblePciController(PVOID DeviceExtension,
				 PVOID HwContext,
				 PVOID BusInformation,
				 PCHAR ArgumentString,
				 PPORT_CONFIGURATION_INFORMATION ConfigInfo,
				 PBOOLEAN Again);

static ULONG STDCALL
AtapiFindIsaBusController(PVOID DeviceExtension,
			  PVOID HwContext,
			  PVOID BusInformation,
			  PCHAR ArgumentString,
			  PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			  PBOOLEAN Again);

static ULONG STDCALL
AtapiFindNativePciController(PVOID DeviceExtension,
			     PVOID HwContext,
			     PVOID BusInformation,
			     PCHAR ArgumentString,
			     PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     PBOOLEAN Again);

static BOOLEAN STDCALL
AtapiInitialize(IN PVOID DeviceExtension);

static BOOLEAN STDCALL
AtapiResetBus(IN PVOID DeviceExtension,
	      IN ULONG PathId);

static BOOLEAN STDCALL
AtapiStartIo(IN PVOID DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb);

static BOOLEAN STDCALL
AtapiInterrupt(IN PVOID DeviceExtension);

static BOOLEAN
AtapiFindDevices(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 PPORT_CONFIGURATION_INFORMATION ConfigInfo);

static BOOLEAN
AtapiIdentifyDevice(IN ULONG CommandPort,
		    IN ULONG ControlPort,
		    IN ULONG DriveNum,
		    IN BOOLEAN Atapi,
		    OUT PIDE_DRIVE_IDENTIFY DrvParms);

static BOOLEAN
IDEResetController(IN WORD CommandPort,
		   IN WORD ControlPort);

static int
AtapiPolledRead(IN WORD CommandPort,
		IN WORD ControlPort,
		IN BYTE PreComp,
		IN BYTE SectorCnt,
		IN BYTE SectorNum,
		IN BYTE CylinderLow,
		IN BYTE CylinderHigh,
		IN BYTE DrvHead,
		IN BYTE Command,
		OUT BYTE *Buffer);



static ULONG
AtapiSendAtapiCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiSendIdeCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		    IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiInquiry(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiReadCapacity(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		  IN PSCSI_REQUEST_BLOCK Srb);

static ULONG
AtapiReadWrite(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
	       IN PSCSI_REQUEST_BLOCK Srb);

//  ----------------------------------------------------------------  Inlines

void
IDESwapBytePairs(char *Buf,
                 int Cnt)
{
  char  t;
  int   i;

  for (i = 0; i < Cnt; i += 2)
    {
      t = Buf[i];
      Buf[i] = Buf[i+1];
      Buf[i+1] = t;
    }
}


static BOOLEAN
IdeFindDrive(int Address,
	     int DriveIdx)
{
  ULONG Cyl;

  DPRINT1("IdeFindDrive(Address %lx DriveIdx %lu) called!\n", Address, DriveIdx);

  IDEWriteDriveHead(Address, IDE_DH_FIXED | (DriveIdx ? IDE_DH_DRV1 : 0));
  IDEWriteCylinderLow(Address, 0x30);

  Cyl = IDEReadCylinderLow(Address);
  DPRINT1("Cylinder %lx\n", Cyl);


  DPRINT1("IdeFindDrive() done!\n");
//  for(;;);
  return(Cyl == 0x30);
}


//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver, locates and claims 
//    hardware resources, and creates various NT objects needed
//    to process I/O requests.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS

STDCALL NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
  HW_INITIALIZATION_DATA InitData;
  NTSTATUS Status;

  DbgPrint("ATAPI Driver %s\n", VERSION);

  /* Initialize data structure */
  RtlZeroMemory(&InitData,
		sizeof(HW_INITIALIZATION_DATA));
  InitData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);
  InitData.HwInitialize = AtapiInitialize;
  InitData.HwResetBus = AtapiResetBus;
  InitData.HwStartIo = AtapiStartIo;
  InitData.HwInterrupt = AtapiInterrupt;

  InitData.DeviceExtensionSize = sizeof(ATAPI_MINIPORT_EXTENSION);
  InitData.SpecificLuExtensionSize = sizeof(UNIT_EXTENSION);

  InitData.MapBuffers = TRUE;

  /* Search the PCI bus for compatibility mode ide controllers */
  InitData.HwFindAdapter = AtapiFindCompatiblePciController;
  InitData.NumberOfAccessRanges = 2;
  InitData.AdapterInterfaceType = PCIBus;

  InitData.VendorId = NULL;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = NULL;
  InitData.DeviceIdLength = 0;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      NULL);
//  if (newStatus < statusToReturn)
//    statusToReturn = newStatus;

  /* Search the ISA bus for ide controllers */
#if 0
  InitData.HwFindAdapter = AtapiFindIsaBusController;
  InitData.NumberOfAccessRanges = 2;
  InitData.AdapterInterfaceType = Isa;

  InitData.VendorId = NULL;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = NULL;
  InitData.DeviceIdLength = 0;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      NULL);
//      if (newStatus < statusToReturn)
//        statusToReturn = newStatus;
#endif

  /* Search the PCI bus for native mode ide controllers */
#if 0
  InitData.HwFindAdapter = AtapiFindNativePciController;
  InitData.NumberOfAccessRanges = 2;
  InitData.AdapterInterfaceType = PCIBus;

  InitData.VendorId = NULL;
  InitData.VendorIdLength = 0;
  InitData.DeviceId = NULL;
  InitData.DeviceIdLength = 0;

  Status = ScsiPortInitialize(DriverObject,
			      RegistryPath,
			      &InitData,
			      (PVOID)i);
//  if (newStatus < statusToReturn)
//    statusToReturn = newStatus;
#endif

  DPRINT1( "Returning from DriverEntry\n" );

  return(Status);
}


static ULONG STDCALL
AtapiFindCompatiblePciController(PVOID DeviceExtension,
				 PVOID HwContext,
				 PVOID BusInformation,
				 PCHAR ArgumentString,
				 PPORT_CONFIGURATION_INFORMATION ConfigInfo,
				 PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  PCI_SLOT_NUMBER SlotNumber;
  PCI_COMMON_CONFIG PciConfig;
  ULONG DataSize;
  ULONG FunctionNumber;
  BOOLEAN ChannelFound;
  BOOLEAN DeviceFound;

  DPRINT("AtapiFindCompatiblePciController() Bus: %lu  Slot: %lu\n",
	  ConfigInfo->SystemIoBusNumber,
	  ConfigInfo->SlotNumber);

  *Again = FALSE;

  /* both channels were claimed: exit */
  if (ConfigInfo->AtdiskPrimaryClaimed == TRUE &&
      ConfigInfo->AtdiskSecondaryClaimed == TRUE)
    return(SP_RETURN_NOT_FOUND);


  SlotNumber.u.AsULONG = 0;
  for (FunctionNumber = 0 /*ConfigInfo->SlotNumber*/; FunctionNumber < 256; FunctionNumber++)
    {
//      SlotNumber.u.bits.FunctionNumber = FunctionNumber;
//      SlotNumber.u.AsULONG = (FunctionNumber & 0x07);
      SlotNumber.u.AsULONG = FunctionNumber;

      ChannelFound = FALSE;
      DeviceFound = FALSE;

      DataSize = ScsiPortGetBusData(DeviceExtension,
				    PCIConfiguration,
				    0,
				    SlotNumber.u.AsULONG,
				    &PciConfig,
				    sizeof(PCI_COMMON_CONFIG));
//      if (DataSize != sizeof(PCI_COMMON_CONFIG) ||
//	  PciConfig.VendorID == PCI_INVALID_VENDORID)
      if (DataSize == 0)
	{
//	  if ((SlotNumber.u.AsULONG & 0x07) == 0)
//	    return(SP_RETURN_ERROR); /* No bus found */

	  continue;
//	  return(SP_RETURN_ERROR);
	}

      if (PciConfig.BaseClass == 0x01 &&
	  PciConfig.SubClass == 0x01) // &&
//	  (PciConfig.ProgIf & 0x05) == 0)
	{
	  /* both channels are in compatibility mode */
	  DPRINT1("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
		 ConfigInfo->SystemIoBusNumber,
//		 SlotNumber.u.AsULONG >> 3,
//		 SlotNumber.u.AsULONG & 0x07,
		 SlotNumber.u.bits.DeviceNumber,
		 SlotNumber.u.bits.FunctionNumber,
		 PciConfig.VendorID,
		 PciConfig.DeviceID);
	  DPRINT("ProgIF 0x%02hx\n", PciConfig.ProgIf);

	  DPRINT1("Found IDE controller in compatibility mode!\n");

	  ConfigInfo->NumberOfBuses = 1;
	  ConfigInfo->MaximumNumberOfTargets = 2;
	  ConfigInfo->MaximumTransferLength = 0x10000; /* max 64Kbyte */

	  if (ConfigInfo->AtdiskPrimaryClaimed == FALSE)
	    {
	      /* Both channels unclaimed: Claim primary channel */
	      DPRINT1("Primary channel!\n");

	      DevExt->CommandPortBase = 0x01F0;
	      DevExt->ControlPortBase = 0x03F6;

	      ConfigInfo->BusInterruptLevel = 14;
	      ConfigInfo->BusInterruptVector = 14;
	      ConfigInfo->InterruptMode = LevelSensitive;

	      ConfigInfo->AccessRanges[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0x01F0);
	      ConfigInfo->AccessRanges[0].RangeLength = 8;
	      ConfigInfo->AccessRanges[0].RangeInMemory = FALSE;

	      ConfigInfo->AccessRanges[1].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0x03F6);
	      ConfigInfo->AccessRanges[1].RangeLength = 1;
	      ConfigInfo->AccessRanges[1].RangeInMemory = FALSE;

	      ConfigInfo->AtdiskPrimaryClaimed = TRUE;
	      ChannelFound = TRUE;
	      *Again = TRUE;
	    }
	  else if (ConfigInfo->AtdiskSecondaryClaimed == FALSE)
	    {
	      /* Primary channel already claimed: claim secondary channel */
	      DPRINT1("Secondary channel!\n");

	      DevExt->CommandPortBase = 0x0170;
	      DevExt->ControlPortBase = 0x0376;

	      ConfigInfo->BusInterruptLevel = 15;
	      ConfigInfo->BusInterruptVector = 15;
	      ConfigInfo->InterruptMode = LevelSensitive;

	      ConfigInfo->AccessRanges[0].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0x0170);
	      ConfigInfo->AccessRanges[0].RangeLength = 8;
	      ConfigInfo->AccessRanges[0].RangeInMemory = FALSE;

	      ConfigInfo->AccessRanges[1].RangeStart = ScsiPortConvertUlongToPhysicalAddress(0x0376);
	      ConfigInfo->AccessRanges[1].RangeLength = 1;
	      ConfigInfo->AccessRanges[1].RangeInMemory = FALSE;

	      ConfigInfo->AtdiskSecondaryClaimed = TRUE;
	      ChannelFound = TRUE;
	      *Again = FALSE;
	    }

	  /* Find attached devices */
	  if (ChannelFound == TRUE)
	    {
	      DeviceFound = AtapiFindDevices(DevExt,
					     ConfigInfo);
	    }
	  DPRINT("AtapiFindCompatiblePciController() returns: SP_RETURN_FOUND\n");
	  return(SP_RETURN_FOUND);
        }
    }

  DPRINT("AtapiFindCompatiblePciController() returns: SP_RETURN_NOT_FOUND\n");

  return(SP_RETURN_NOT_FOUND);
}


static ULONG STDCALL
AtapiFindIsaBusController(PVOID DeviceExtension,
			  PVOID HwContext,
			  PVOID BusInformation,
			  PCHAR ArgumentString,
			  PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			  PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  DPRINT("AtapiFindIsaBusController() called!\n");

  *Again = FALSE;

  if (ConfigInfo->AtdiskPrimaryClaimed == TRUE)
    {
       DPRINT("Primary IDE controller already claimed!\n");

    }

  if (ConfigInfo->AtdiskSecondaryClaimed == TRUE)
    {
       DPRINT("Secondary IDE controller already claimed!\n");

    }

  DPRINT("AtapiFindIsaBusController() done!\n");

  return(SP_RETURN_NOT_FOUND);
}


static ULONG STDCALL
AtapiFindNativePciController(PVOID DeviceExtension,
			     PVOID HwContext,
			     PVOID BusInformation,
			     PCHAR ArgumentString,
			     PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     PBOOLEAN Again)
{
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  DPRINT("AtapiFindNativePciController() called!\n");

  *Again = FALSE;

  DPRINT("AtapiFindNativePciController() done!\n");

  return(SP_RETURN_NOT_FOUND);
}


static BOOLEAN STDCALL
AtapiInitialize(IN PVOID DeviceExtension)
{
  return(TRUE);
}


static BOOLEAN STDCALL
AtapiResetBus(IN PVOID DeviceExtension,
	      IN ULONG PathId)
{
  return(TRUE);
}


static BOOLEAN STDCALL
AtapiStartIo(IN PVOID DeviceExtension,
	     IN PSCSI_REQUEST_BLOCK Srb)
{
  PATAPI_MINIPORT_EXTENSION DevExt;
  ULONG Result;

  DPRINT1("AtapiStartIo() called\n");

  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:
	DevExt->CurrentSrb = Srb;
	if (DevExt->DeviceAtapi[Srb->TargetId] == TRUE)
	  {
	    Result = AtapiSendAtapiCommand(DevExt,
					   Srb);
	  }
	else
	  {
	    Result = AtapiSendIdeCommand(DevExt,
					 Srb);
	  }
	break;

    }

  Srb->SrbStatus = Result;


  if (Result != SRB_STATUS_PENDING)
    {
      DevExt->CurrentSrb = NULL;
      Srb->SrbStatus = (UCHAR)Result;
#if 0
      ScsiPortNotification(RequestComplete,
			   DeviceExtension,
			   Srb);

      ScsiPortNotification(NextRequest,
			   DeviceExtension,
			   NULL);
#endif
    }

  return(TRUE);
}


static BOOLEAN STDCALL
AtapiInterrupt(IN PVOID DeviceExtension)
{
  PATAPI_MINIPORT_EXTENSION DevExt;
  PSCSI_REQUEST_BLOCK Srb;
  ULONG CommandPortBase;
  ULONG ControlPortBase;

  UCHAR DeviceStatus;
  BOOLEAN IsLastBlock, AnErrorOccured, RequestIsComplete;
  ULONG Retries;
  NTSTATUS ErrorStatus;
  ULONG ErrorInformation;
  PUCHAR TargetAddress;



  DPRINT1("AtapiInterrupt() called!\n");

  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  Srb = DevExt->CurrentSrb;

  DPRINT1("Srb: %p\n", Srb);

  CommandPortBase = DevExt->CommandPortBase;
  ControlPortBase = DevExt->ControlPortBase;

  DPRINT("CommandPortBase: %lx  ControlPortBase: %lx\n", CommandPortBase, ControlPortBase);


  IsLastBlock = FALSE;
  AnErrorOccured = FALSE;
  RequestIsComplete = FALSE;
  ErrorStatus = STATUS_SUCCESS;
  ErrorInformation = 0;

  DeviceStatus = IDEReadStatus(CommandPortBase);

  /*  Handle error condition if it exists */
  if (DeviceStatus & IDE_SR_ERR)
    {
      BYTE ErrorReg, SectorCount, SectorNum, CylinderLow, CylinderHigh;
      BYTE DriveHead;

      /* Log the error */
      ErrorReg = IDEReadError(CommandPortBase);
      CylinderLow = IDEReadCylinderLow(CommandPortBase);
      CylinderHigh = IDEReadCylinderHigh(CommandPortBase);
      DriveHead = IDEReadDriveHead(CommandPortBase);
      SectorCount = IDEReadSectorCount(CommandPortBase);
      SectorNum = IDEReadSectorNum(CommandPortBase);

      /* FIXME: should use the NT error logging facility */
      DbgPrint("ATAPT Error: OP:%02x STAT:%02x ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n",
	       0, //DeviceExtension->Operation,
	       DeviceStatus,
	       ErrorReg,
	       CylinderLow,
	       CylinderHigh,
	       SectorCount,
	       SectorNum);

      /* FIXME: should retry the command and perhaps recalibrate the drive */

        //  Set error status information
      AnErrorOccured = TRUE;
      ErrorStatus = STATUS_DISK_OPERATION_FAILED;
#if 0
      ErrorInformation = 
          (((((((CylinderHigh << 8) + CylinderLow) * 
              DeviceExtension->LogicalHeads) + 
              (DriveHead % DeviceExtension->LogicalHeads)) * 
              DeviceExtension->SectorsPerLogTrk) + SectorNum - 1) -
          DeviceExtension->StartingSector) * DeviceExtension->BytesPerSector;
#endif
    }
  else
    {
      switch (Srb->Cdb[0])
	{
	  case SCSIOP_READ:
	    DPRINT1("SCSIOP_READ\n");

	    /* Update controller/device state variables */
	    TargetAddress = Srb->DataBuffer;
	    Srb->DataBuffer += DevExt->DeviceParams[Srb->TargetId].BytesPerSector;
	    Srb->DataTransferLength -= DevExt->DeviceParams[Srb->TargetId].BytesPerSector;
//	    DevExt->SectorsTransferred++;

	    /* Remember whether DRQ should be low at end (last block read) */
	    IsLastBlock = Srb->DataTransferLength == 0;

	    /* Wait for DRQ assertion */
	    for (Retries = 0; Retries < IDE_MAX_DRQ_RETRIES &&
		 !(IDEReadStatus(CommandPortBase) & IDE_SR_DRQ);
		 Retries++)
	      {
		KeStallExecutionProcessor(10);
	      }

	    /* Copy the block of data */
	    IDEReadBlock(CommandPortBase,
			 TargetAddress,
			 IDE_SECTOR_BUF_SZ);

	    /* check DRQ */
	    if (IsLastBlock)
	      {
		for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES &&
		     (IDEReadStatus(CommandPortBase) & IDE_SR_BUSY);
		     Retries++)
		  {
		    KeStallExecutionProcessor(10);
		  }

		/* Check for data overrun */
		if (IDEReadStatus(CommandPortBase) & IDE_SR_DRQ)
		  {
		    AnErrorOccured = TRUE;
		    ErrorStatus = STATUS_DATA_OVERRUN;
		    ErrorInformation = 0;
		  }
		else
		  {

#if 0
                    //  Setup next transfer or set RequestIsComplete
                  if (DeviceExtension->BytesRequested > 
                      DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = 
                      DeviceExtension->BytesPerSector * IDE_MAX_SECTORS_PER_XFER;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else if (DeviceExtension->BytesRequested > 0) 
                    {
                      DeviceExtension->StartingSector += DeviceExtension->SectorsTransferred;
                      DeviceExtension->SectorsTransferred = 0;
                      DeviceExtension->BytesToTransfer = DeviceExtension->BytesRequested;
                      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
                    } 
                  else
                    {
                      RequestIsComplete = TRUE;
                    }
#endif
		    RequestIsComplete = TRUE;
		  }
	      }
	    break;

	  case SCSIOP_WRITE:
	    DPRINT1("AtapiInterrupt(): SCSIOP_WRITE not implemented yet!\n");
	    RequestIsComplete = TRUE;
	    break;
      }
    }


  /* If there was an error or the request is done, complete the packet */
  if (AnErrorOccured || RequestIsComplete)
    {
#if 0
      /* Set the return status and info values */
      Irp = ControllerExtension->CurrentIrp;
      Irp->IoStatus.Status = ErrorStatus;
      Irp->IoStatus.Information = ErrorInformation;

      /* Clear out controller fields */
      ControllerExtension->OperationInProgress = FALSE;
      ControllerExtension->DeviceStatus = 0;

      /* Queue the Dpc to finish up */
      IoRequestDpc(DeviceExtension->DeviceObject,
		   Irp,
		   ControllerExtension);
#endif
    }
  else if (IsLastBlock)
    {
#if 0
      /* Else more data is needed, setup next device I/O */
      IDEStartController((PVOID)DeviceExtension);
#endif
    }

  DPRINT1("AtapiInterrupt() done!\n");

  return(TRUE);
}






//  ----------------------------------------------------  Discardable statics


static BOOLEAN
AtapiFindDevices(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
  BOOLEAN DeviceFound = FALSE;
  ULONG CommandPortBase;
  ULONG ControlPortBase;
  ULONG UnitNumber;
  ULONG Retries;
  BYTE High, Low;

  DPRINT("AtapiFindDevices() called\n");

//  CommandPortBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
  CommandPortBase = ScsiPortConvertPhysicalAddressToUlong(ConfigInfo->AccessRanges[0].RangeStart);
  DPRINT("  CommandPortBase: %x\n", CommandPortBase);

//  ControlPortBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[1].RangeStart);
  ControlPortBase = ScsiPortConvertPhysicalAddressToUlong(ConfigInfo->AccessRanges[1].RangeStart);
  DPRINT("  ControlPortBase: %x\n", ControlPortBase);

  for (UnitNumber = 0; UnitNumber < 2; UnitNumber++)
    {
      /* disable interrupts */
      IDEWriteDriveControl(CommandPortBase,
			   IDE_DC_nIEN);

      /* select drive */
      IDEWriteDriveHead(CommandPortBase,
			IDE_DH_FIXED | (UnitNumber ? IDE_DH_DRV1 : 0));
      ScsiPortStallExecution(500);
      IDEWriteCylinderHigh(CommandPortBase, 0);
      IDEWriteCylinderLow(CommandPortBase, 0);
      IDEWriteCommand(CommandPortBase, 0x08); /* IDE_COMMAND_ATAPI_RESET */
//      ScsiPortStallExecution(1000*1000);
//      IDEWriteDriveHead(CommandPortBase,
//			IDE_DH_FIXED | (UnitNumber ? IDE_DH_DRV1 : 0));
//			IDE_DH_FIXED);

      for (Retries = 0; Retries < 20000; Retries++)
	{
	  if (!(IDEReadStatus(CommandPortBase) & IDE_SR_BUSY))
	    {
	      break;
	    }
	  ScsiPortStallExecution(150);
	}
      if (Retries >= IDE_RESET_BUSY_TIMEOUT * 1000)
	{
	  DbgPrint("Timeout on drive %lu\n", UnitNumber);
	  return(DeviceFound);
	}

      High = IDEReadCylinderHigh(CommandPortBase);
      Low = IDEReadCylinderLow(CommandPortBase);

      DPRINT("  Check drive %lu: High 0x%x Low 0x%x\n",
	     UnitNumber,
	     High,
	     Low);

      if (High == 0xEB && Low == 0x14)
	{
	  if (AtapiIdentifyDevice(CommandPortBase,
				  ControlPortBase,
				  UnitNumber,
				  TRUE,
				  &DeviceExtension->DeviceParams[UnitNumber]))
	    {
	      DPRINT("  ATAPI drive found!\n");
	      DeviceExtension->DevicePresent[UnitNumber] = TRUE;
	      DeviceExtension->DeviceAtapi[UnitNumber] = TRUE;
	      DeviceFound = TRUE;
	    }
	  else
	    {
	      DPRINT("  No ATAPI drive found!\n");
	    }
	}
      else
	{
	  if (AtapiIdentifyDevice(CommandPortBase,
				  ControlPortBase,
				  UnitNumber,
				  FALSE,
				  &DeviceExtension->DeviceParams[UnitNumber]))
	    {
	      DPRINT("  IDE drive found!\n");
	      DeviceExtension->DevicePresent[UnitNumber] = TRUE;
	      DeviceExtension->DeviceAtapi[UnitNumber] = FALSE;
	      DeviceFound = TRUE;
	    }
	  else
	    {
	      DPRINT("  No IDE drive found!\n");
	    }
	}
    }

  DPRINT("AtapiFindDrives() done\n");

  return(DeviceFound);
}


//    AtapiResetController
//
//  DESCRIPTION:
//    Reset the controller and report completion status
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  WORD  CommandPort  The address of the command port
//    IN  WORD  ControlPort  The address of the control port
//
//  RETURNS:
//

static BOOLEAN
AtapiResetController(IN WORD CommandPort,
		     IN WORD ControlPort)
{
  int Retries;

  /* Assert drive reset line */
  IDEWriteDriveControl(ControlPort, IDE_DC_SRST);

  /* Wait for min. 25 microseconds */
  ScsiPortStallExecution(IDE_RESET_PULSE_LENGTH);

  /* Negate drive reset line */
  IDEWriteDriveControl(ControlPort, 0);

  /* Wait for BUSY negation */
  for (Retries = 0; Retries < IDE_RESET_BUSY_TIMEOUT * 1000; Retries++)
    {
      if (!(IDEReadStatus(CommandPort) & IDE_SR_BUSY))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }

  CHECKPOINT;
  if (Retries >= IDE_RESET_BUSY_TIMEOUT * 1000)
    {
      return(FALSE);
    }

  CHECKPOINT;

    //  return TRUE if controller came back to life. and
    //  the registers are initialized correctly
  return(IDEReadError(CommandPort) == 1);
}


//    AtapiIdentifyDevice
//
//  DESCRIPTION:
//    Get the identification block from the drive
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   int                  CommandPort  Address of the command port
//    IN   int                  DriveNum     The drive index (0,1)
//    OUT  PIDE_DRIVE_IDENTIFY  DrvParms     Address to write drive ident block
//
//  RETURNS:
//    TRUE  The drive identification block was retrieved successfully
//

static BOOLEAN
AtapiIdentifyDevice(IN ULONG CommandPort,
		    IN ULONG ControlPort,
		    IN ULONG DriveNum,
		    IN BOOLEAN Atapi,
		    OUT PIDE_DRIVE_IDENTIFY DrvParms)
{
  /*  Get the Drive Identify block from drive or die  */
  if (AtapiPolledRead((WORD)CommandPort,
		      (WORD)ControlPort,
		      0,
		      1,
		      0,
		      0,
		      0,
		      (DriveNum ? IDE_DH_DRV1 : 0),
		      (Atapi ? IDE_CMD_IDENT_ATAPI_DRV : IDE_CMD_IDENT_ATA_DRV),
		      (BYTE *)DrvParms) != 0) /* atapi_identify */
    {
      DPRINT1("IDEPolledRead() failed\n");
      return FALSE;
    }

  /*  Report on drive parameters if debug mode  */
  IDESwapBytePairs(DrvParms->SerialNumber, 20);
  IDESwapBytePairs(DrvParms->FirmwareRev, 8);
  IDESwapBytePairs(DrvParms->ModelNumber, 40);
  DPRINT("Config:%04x  Cyls:%5d  Heads:%2d  Sectors/Track:%3d  Gaps:%02d %02d\n", 
         DrvParms->ConfigBits, 
         DrvParms->LogicalCyls, 
         DrvParms->LogicalHeads, 
         DrvParms->SectorsPerTrack, 
         DrvParms->InterSectorGap, 
         DrvParms->InterSectorGapSize);
  DPRINT("Bytes/PLO:%3d  Vendor Cnt:%2d  Serial number:[%.20s]\n", 
         DrvParms->BytesInPLO, 
         DrvParms->VendorUniqueCnt, 
         DrvParms->SerialNumber);
  DPRINT("Cntlr type:%2d  BufSiz:%5d  ECC bytes:%3d  Firmware Rev:[%.8s]\n", 
         DrvParms->ControllerType, 
         DrvParms->BufferSize * IDE_SECTOR_BUF_SZ, 
         DrvParms->ECCByteCnt, 
         DrvParms->FirmwareRev);
  DPRINT("Model:[%.40s]\n", DrvParms->ModelNumber);
  DPRINT("RWMult?:%02x  LBA:%d  DMA:%d  MinPIO:%d ns  MinDMA:%d ns\n", 
         (DrvParms->RWMultImplemented) & 0xff, 
         (DrvParms->Capabilities & IDE_DRID_LBA_SUPPORTED) ? 1 : 0,
         (DrvParms->Capabilities & IDE_DRID_DMA_SUPPORTED) ? 1 : 0, 
         DrvParms->MinPIOTransTime,
         DrvParms->MinDMATransTime);
  DPRINT("TM:Cyls:%d  Heads:%d  Sectors/Trk:%d Capacity:%ld\n",
         DrvParms->TMCylinders, 
         DrvParms->TMHeads, 
         DrvParms->TMSectorsPerTrk,
         (ULONG)(DrvParms->TMCapacityLo + (DrvParms->TMCapacityHi << 16)));
  DPRINT("TM:SectorCount: 0x%04x%04x = %lu\n",
         DrvParms->TMSectorCountHi,
         DrvParms->TMSectorCountLo,
         (ULONG)((DrvParms->TMSectorCountHi << 16) + DrvParms->TMSectorCountLo));

  DPRINT("BytesPerSector %d\n", DrvParms->BytesPerSector);
  if (DrvParms->BytesPerSector == 0)
    DrvParms->BytesPerSector = 512;
  return TRUE;
}


//    AtapiPolledRead
//
//  DESCRIPTION:
//    Read a sector of data from the drive in a polled fashion.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN   WORD  Address       Address of command port for drive
//    IN   BYTE  PreComp       Value to write to precomp register
//    IN   BYTE  SectorCnt     Value to write to sectorCnt register
//    IN   BYTE  SectorNum     Value to write to sectorNum register
//    IN   BYTE  CylinderLow   Value to write to CylinderLow register
//    IN   BYTE  CylinderHigh  Value to write to CylinderHigh register
//    IN   BYTE  DrvHead       Value to write to Drive/Head register
//    IN   BYTE  Command       Value to write to Command register
//    OUT  BYTE  *Buffer       Buffer for output data
//
//  RETURNS:
//    int  0 is success, non 0 is an error code
//

static int
AtapiPolledRead(IN WORD Address,
		IN WORD ControlPort,
		IN BYTE PreComp,
		IN BYTE SectorCnt,
		IN BYTE SectorNum,
		IN BYTE CylinderLow,
		IN BYTE CylinderHigh,
		IN BYTE DrvHead,
		IN BYTE Command,
		OUT BYTE *Buffer)
{
  ULONG SectorCount = 0;
  ULONG RetryCount;
  BOOLEAN Junk = FALSE;
  UCHAR Status;
  UCHAR Control;

  /* Disable interrupts */
  Control = IDEReadAltStatus(ControlPort);
  IDEWriteDriveControl(ControlPort, Control | IDE_DC_nIEN);

  /*  Wait for STATUS.BUSY and STATUS.DRQ to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(Address);
      if (!(Status & IDE_SR_BUSY) && !(Status & IDE_SR_DRQ))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  if (RetryCount == IDE_MAX_BUSY_RETRIES)
    {
      return(IDE_ER_ABRT);
    }

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(Address, IDE_DH_FIXED | DrvHead);

  /*  Wait for STATUS.BUSY and STATUS.DRQ to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(Address);
      if (!(Status & IDE_SR_BUSY) && !(Status & IDE_SR_DRQ))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  if (RetryCount >= IDE_MAX_BUSY_RETRIES)
    {
      return IDE_ER_ABRT;
    }

  /*  Issue command to drive  */
  if (DrvHead & IDE_DH_LBA)
    {
      DPRINT("READ:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     ((DrvHead & 0x0f) << 24) + (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
	     SectorCnt,
	     Command);
    }
  else
    {
      DPRINT("READ:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     CylinderHigh,
	     CylinderLow,
	     DrvHead & 0x0f,
	     SectorNum,
	     SectorCnt,
	     Command);
    }

  /*  Setup command parameters  */
  IDEWritePrecomp(Address, PreComp);
  IDEWriteSectorCount(Address, SectorCnt);
  IDEWriteSectorNum(Address, SectorNum);
  IDEWriteCylinderHigh(Address, CylinderHigh);
  IDEWriteCylinderLow(Address, CylinderLow);
  IDEWriteDriveHead(Address, IDE_DH_FIXED | DrvHead);

  /*  Issue the command  */
  IDEWriteCommand(Address, Command);
  ScsiPortStallExecution(50);

  /*  wait for DRQ or error  */
  for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(Address);
      if (!(Status & IDE_SR_BUSY))
	{
	  if (Status & IDE_SR_ERR)
	    {
	      IDEWriteDriveControl(ControlPort, Control & ~IDE_DC_nIEN);
	      return(IDE_ER_ABRT);
	    }
	  if (Status & IDE_SR_DRQ)
	    {
	      break;
	    }
	  else
	    {
	      IDEWriteDriveControl(ControlPort, Control & ~IDE_DC_nIEN);
	      return(IDE_ER_ABRT);
	    }
	}
      ScsiPortStallExecution(10);
    }

  /*  timed out  */
  if (RetryCount >= IDE_MAX_POLL_RETRIES)
    {
      IDEWriteDriveControl(ControlPort, Control & ~IDE_DC_nIEN);
      return(IDE_ER_ABRT);
    }

  while (1)
    {
      /*  Read data into buffer  */
      if (Junk == FALSE)
	{
	  IDEReadBlock(Address, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer += IDE_SECTOR_BUF_SZ;
	}
      else
	{
	  UCHAR JunkBuffer[IDE_SECTOR_BUF_SZ];
	  IDEReadBlock(Address, JunkBuffer, IDE_SECTOR_BUF_SZ);
	}
      SectorCount++;

      /*  Check for error or more sectors to read  */
      for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
	{
	  Status = IDEReadStatus(Address);
	  if (!(Status & IDE_SR_BUSY))
	    {
	      if (Status & IDE_SR_ERR)
		{
		  IDEWriteDriveControl(ControlPort, Control & ~IDE_DC_nIEN);
		  return(IDE_ER_ABRT);
		}
	      if (Status & IDE_SR_DRQ)
		{
		  if (SectorCount >= SectorCnt)
		    {
		      DPRINT("Buffer size exceeded!\n");
		      Junk = TRUE;
		    }
		  break;
		}
	      else
		{
		  if (SectorCount > SectorCnt)
		    {
		      DPRINT("Read %lu sectors of junk!\n",
			     SectorCount - SectorCnt);
		    }
		  IDEWriteDriveControl(ControlPort, Control & ~IDE_DC_nIEN);
		  return(0);
		}
	    }
	}
    }
}


//  -------------------------------------------  Nondiscardable statics

static ULONG
AtapiSendAtapiCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb);
{
  DPRINT1("AtapiSendAtapiComamnd() called!\n");
  DPRINT1("Not implemented yet!\n");
  return(SRB_STATUS_SELECTION_TIMEOUT);
}


static ULONG
AtapiSendIdeCommand(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		    IN PSCSI_REQUEST_BLOCK Srb)
{
  ULONG SrbStatus = SRB_STATUS_SUCCESS;

  DPRINT("AtapiSendIdeCommand() called!\n");

  DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n",
	 Srb->PathId,
	 Srb->TargetId,
	 Srb->Lun);

  switch (Srb->Cdb[0])
    {
      case SCSIOP_INQUIRY:
	SrbStatus = AtapiInquiry(DeviceExtension,
				 Srb);
	break;

      case SCSIOP_READ_CAPACITY:
	SrbStatus = AtapiReadCapacity(DeviceExtension,
				      Srb);
	break;

      case SCSIOP_READ:
      case SCSIOP_WRITE:
	SrbStatus = AtapiReadWrite(DeviceExtension,
				   Srb);
	break;

      case SCSIOP_MODE_SENSE:
      case SCSIOP_TEST_UNIT_READY:
      case SCSIOP_VERIFY:
      case SCSIOP_START_STOP_UNIT:
      case SCSIOP_REQUEST_SENSE:
	break;

     default:
	DPRINT1("AtapiSendIdeCommand():unknown command %x\n",
		Srb->Cdb[0]);
	SrbStatus = SRB_STATUS_INVALID_REQUEST;
	break;
    }

  DPRINT("AtapiSendIdeCommand() done!\n");

  return(SrbStatus);
}


static ULONG
AtapiInquiry(PATAPI_MINIPORT_EXTENSION DeviceExtension,
	     PSCSI_REQUEST_BLOCK Srb)
{
  PIDE_DRIVE_IDENTIFY DeviceParams;
  PINQUIRYDATA InquiryData;
  ULONG i;

  DPRINT("SCSIOP_INQUIRY: TargetId: %lu\n", Srb->TargetId);

  if ((Srb->PathId != 0) ||
      (Srb->TargetId > 1) ||
      (Srb->Lun != 0) ||
      (DeviceExtension->DevicePresent[Srb->TargetId] == FALSE))
    {
      return(SRB_STATUS_SELECTION_TIMEOUT);
    }

  InquiryData = Srb->DataBuffer;
  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* clear buffer */
  for (i = 0; i < Srb->DataTransferLength; i++)
    {
      ((PUCHAR)Srb->DataBuffer)[i] = 0;
    }

  /* set device class */
  if (DeviceExtension->DeviceAtapi[Srb->TargetId] == FALSE)
    {
      /* hard-disk */
      InquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    }
  else
    {
      /* FIXME: this is incorrect use SCSI-INQUIRY command!! */
      /* cdrom drive */
      InquiryData->DeviceType = READ_ONLY_DIRECT_ACCESS_DEVICE;
    }

  DPRINT("ConfigBits: 0x%x\n", DeviceParams->ConfigBits);
  if (DeviceParams->ConfigBits & 0x80)
    {
      DPRINT1("Removable media!\n");
      InquiryData->RemovableMedia = 1;
    }

  for (i = 0; i < 20; i += 2)
    {
      InquiryData->VendorId[i] =
	((PUCHAR)DeviceParams->ModelNumber)[i];
      InquiryData->VendorId[i+1] =
	((PUCHAR)DeviceParams->ModelNumber)[i+1];
    }

  for (i = 0; i < 4; i++)
    {
      InquiryData->ProductId[12+i] = ' ';
    }

  for (i = 0; i < 4; i += 2)
    {
      InquiryData->ProductRevisionLevel[i] =
	((PUCHAR)DeviceParams->FirmwareRev)[i];
      InquiryData->ProductRevisionLevel[i+1] =
	((PUCHAR)DeviceParams->FirmwareRev)[i+1];
    }

  return(SRB_STATUS_SUCCESS);
}


static ULONG
AtapiReadCapacity(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		  PSCSI_REQUEST_BLOCK Srb)
{
  PREAD_CAPACITY_DATA CapacityData;
  PIDE_DRIVE_IDENTIFY DeviceParams;
  ULONG LastSector;

  DPRINT("SCSIOP_READ_CAPACITY: TargetId: %lu\n", Srb->TargetId);

  if ((Srb->PathId != 0) ||
      (Srb->TargetId > 1) ||
      (Srb->Lun != 0) ||
      (DeviceExtension->DevicePresent[Srb->TargetId] == FALSE))
    {
      return(SRB_STATUS_SELECTION_TIMEOUT);
    }


  CapacityData = (PREAD_CAPACITY_DATA)Srb->DataBuffer;
  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* Set sector (block) size to 512 bytes (big-endian). */
  CapacityData->BytesPerBlock = 0x20000;

  /* Calculate last sector (big-endian). */
  if (DeviceParams->Capabilities & IDE_DRID_LBA_SUPPORTED)
    {
      LastSector = (ULONG)((DeviceParams->TMSectorCountHi << 16) +
			    DeviceParams->TMSectorCountLo) - 1;
    }
  else
    {
      LastSector = (ULONG)(DeviceParams->LogicalCyls *
			   DeviceParams->LogicalHeads *
			   DeviceParams->SectorsPerTrack)-1;
    }

  CapacityData->LogicalBlockAddress = (((PUCHAR)&LastSector)[0] << 24) |
				      (((PUCHAR)&LastSector)[1] << 16) |
				      (((PUCHAR)&LastSector)[2] << 8) |
				      ((PUCHAR)&LastSector)[3];

  DPRINT("LastCount: %lu (%08lx / %08lx)\n",
	 LastSector,
	 LastSector,
	 CapacityData->LogicalBlockAddress);

  return(SRB_STATUS_SUCCESS);
}


static ULONG
AtapiReadWrite(PATAPI_MINIPORT_EXTENSION DeviceExtension,
	       PSCSI_REQUEST_BLOCK Srb)
{
  PIDE_DRIVE_IDENTIFY DeviceParams;

  ULONG StartingSector,i;
  ULONG SectorCount;
  UCHAR CylinderHigh;
  UCHAR CylinderLow;
  UCHAR DrvHead;
  UCHAR SectorNumber;
  UCHAR Command;
  ULONG Retries;
  UCHAR Status;


  DPRINT("AtapiReadWrite() called!\n");

  if ((Srb->PathId != 0) ||
      (Srb->TargetId > 1) ||
      (Srb->Lun != 0) ||
      (DeviceExtension->DevicePresent[Srb->TargetId] == FALSE))
    {
      return(SRB_STATUS_SELECTION_TIMEOUT);
    }

    DPRINT("SCSIOP_WRITE: TargetId: %lu\n",
	   Srb->TargetId);

  DeviceParams = &DeviceExtension->DeviceParams[Srb->TargetId];

  /* Get starting sector number from CDB. */
  StartingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
		   ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

  SectorCount = (Srb->DataTransferLength + DeviceParams->BytesPerSector - 1) /
		DeviceParams->BytesPerSector;

  DPRINT("Starting sector %lu  Number of bytes %lu  Sectors %lu\n",
	 StartingSector,
	 Srb->DataTransferLength,
	 SectorCount);

  if (DeviceParams->Capabilities & IDE_DRID_LBA_SUPPORTED)
    {
      SectorNumber = StartingSector & 0xff;
      CylinderLow = (StartingSector >> 8) & 0xff;
      CylinderHigh = (StartingSector >> 16) & 0xff;
      DrvHead = ((StartingSector >> 24) & 0x0f) | 
          (Srb->TargetId ? IDE_DH_DRV1 : 0) |
          IDE_DH_LBA;
    }
  else
    {
      SectorNumber = (StartingSector % DeviceParams->SectorsPerTrack) + 1;
      StartingSector /= DeviceParams->SectorsPerTrack;
      DrvHead = (StartingSector % DeviceParams->LogicalHeads) | 
          (Srb->TargetId ? IDE_DH_DRV1 : 0);
      StartingSector /= DeviceParams->LogicalHeads;
      CylinderLow = StartingSector & 0xff;
      CylinderHigh = StartingSector >> 8;
    }


  if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
    {
      Command = IDE_CMD_READ;
    }
  else
    {
      Command = IDE_CMD_WRITE;
    }

  if (DrvHead & IDE_DH_LBA)
    {
      DPRINT1("%s:BUS=%04x:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
	     (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? "READ" : "WRITE",
	     DeviceExtension->CommandPortBase,
	     DrvHead & IDE_DH_DRV1 ? 1 : 0,
	     ((DrvHead & 0x0f) << 24) +
	       (CylinderHigh << 16) + (CylinderLow << 8) + SectorNumber,
	     SectorCount,
	     Command);
    }
  else
    {
      DPRINT1("%s:BUS=%04x:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
             (Srb->SrbFlags & SRB_FLAGS_DATA_IN) ? "READ" : "WRITE",
             DeviceExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             CylinderHigh,
             CylinderLow,
             DrvHead & 0x0f,
             SectorNumber,
             SectorCount,
             Command);
    }

  /* Set pointer to data buffer. */
  DeviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;

  DeviceExtension->CurrentSrb = Srb;
  DeviceExtension->ExpectingInterrupt = TRUE;



  /*  wait for BUSY to clear  */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY))
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  DPRINT ("status=%02x\n", Status);
  DPRINT ("waited %ld usecs for busy to clear\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT ("Drive is BUSY for too long\n");
      return(SRB_STATUS_BUSY);
#if 0
      if (++ControllerExtension->Retries > IDE_MAX_CMD_RETRIES)
        {
          DbgPrint ("Max Retries on Drive reset reached, returning failure\n");
          Irp = ControllerExtension->CurrentIrp;
          Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
          Irp->IoStatus.Information = 0;

          return FALSE;
        }
      else
        {
          DPRINT ("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
#endif
    }

  /*  Select the desired drive  */
  IDEWriteDriveHead(DeviceExtension->CommandPortBase,
		    IDE_DH_FIXED | DrvHead);

  /*  wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if (!(Status & IDE_SR_BUSY) && (Status & IDE_SR_DRDY))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }
  DPRINT ("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT ("Drive is BUSY for too long after drive select\n");
      return(SRB_STATUS_BUSY);
#if 0
      if (ControllerExtension->Retries++ > IDE_MAX_CMD_RETRIES)
	{
          DbgPrint ("Max Retries on Drive reset reached, returning failure\n");
          Irp = ControllerExtension->CurrentIrp;
          Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
          Irp->IoStatus.Information = 0;

          return FALSE;
	}
      else
        {
          DPRINT ("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
#endif
    }

  if (Command == IDE_CMD_WRITE)
    {
      DPRINT1("Write not implemented yet!\n");
      return(SRB_STATUS_SUCCESS);
    }

  /* Indicate expecting an interrupt. */
  DeviceExtension->ExpectingInterrupt = TRUE;

  /*  Setup command parameters  */
  IDEWritePrecomp(DeviceExtension->CommandPortBase, 0);
  IDEWriteSectorCount(DeviceExtension->CommandPortBase, SectorCount);
  IDEWriteSectorNum(DeviceExtension->CommandPortBase, SectorNumber);
  IDEWriteCylinderHigh(DeviceExtension->CommandPortBase, CylinderHigh);
  IDEWriteCylinderLow(DeviceExtension->CommandPortBase, CylinderLow);
  IDEWriteDriveHead(DeviceExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  Issue command to drive  */
  IDEWriteCommand(DeviceExtension->CommandPortBase, Command);
//  ControllerExtension->TimerState = IDETimerCmdWait;
//  ControllerExtension->TimerCount = IDE_CMD_TIMEOUT;


  /* FIXME: Write data here! */


  DPRINT("AtapiReadWrite() done!\n");

  /* Wait for interrupt. */
  return(SRB_STATUS_PENDING);
}

/* EOF */
