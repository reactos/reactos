/* $Id: atapi.c,v 1.4 2002/01/27 01:24:44 ekohl Exp $
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
 *   - Use scsiport driver.
 */

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>

#include "../include/srb.h"
#include "../include/scsi.h"
#include "../include/ntddscsi.h"

#include "atapi.h"
#include "partitio.h"

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
  IDE_DRIVE_IDENTIFY DeviceParms[2];
  BOOLEAN DevicePresent[2];
  BOOLEAN DeviceAtapi[2];
} ATAPI_MINIPORT_EXTENSION, *PATAPI_MINIPORT_EXTENSION;


typedef struct _UNIT_EXTENSION
{
  ULONG Dummy;
} UNIT_EXTENSION, *PUNIT_EXTENSION;


typedef struct _IDE_CONTROLLER_PARAMETERS
{
  int              CommandPortBase;
  int              CommandPortSpan;
  int              ControlPortBase;
  int              ControlPortSpan;
  int              Vector;
  int              IrqL;
  int              SynchronizeIrqL;
  KINTERRUPT_MODE  InterruptMode;
  KAFFINITY        Affinity;
} IDE_CONTROLLER_PARAMETERS, *PIDE_CONTROLLER_PARAMETERS;

//  NOTE: Do not increase max drives above 2

#define  IDE_MAX_DRIVES       2

#define  IDE_MAX_CONTROLLERS  2
IDE_CONTROLLER_PARAMETERS Controllers[IDE_MAX_CONTROLLERS] = 
{
  {0x01f0, 8, 0x03f6, 1, 14, 14, 15, LevelSensitive, 0xffff},
  {0x0170, 8, 0x0376, 1, 15, 15, 15, LevelSensitive, 0xffff}
  /*{0x01E8, 8, 0x03ee, 1, 11, 11, 15, LevelSensitive, 0xffff},
  {0x0168, 8, 0x036e, 1, 10, 10, 15, LevelSensitive, 0xffff}*/
};

static BOOLEAN IDEInitialized = FALSE;

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
AtapiIdentifyATADevice(IN ULONG CommandPort,
		       IN ULONG DriveNum,
		       OUT PIDE_DRIVE_IDENTIFY DrvParms);

static BOOLEAN
AtapiIdentifyATAPIDevice(IN ULONG CommandPort,
		         IN ULONG DriveNum,
		         OUT PIDE_DRIVE_IDENTIFY DrvParms);

static BOOLEAN
IDEResetController(IN WORD CommandPort,
		   IN WORD ControlPort);

static int
IDEPolledRead(IN WORD Address,
	      IN BYTE PreComp,
	      IN BYTE SectorCnt,
	      IN BYTE SectorNum,
	      IN BYTE CylinderLow,
	      IN BYTE CylinderHigh,
	      IN BYTE DrvHead,
	      IN BYTE Command,
	      OUT BYTE *Buffer);



static NTSTATUS STDCALL
IDEDispatchReadWrite(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static VOID STDCALL
IDEStartIo(IN PDEVICE_OBJECT DeviceObject,
	   IN PIRP Irp);

static IO_ALLOCATION_ACTION STDCALL
IDEAllocateController(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp,
		      IN PVOID MapRegisterBase,
		      IN PVOID Ccontext);

static BOOLEAN STDCALL
IDEStartController(IN OUT PVOID Context);

VOID
IDEBeginControllerReset(PIDE_CONTROLLER_EXTENSION ControllerExtension);

static BOOLEAN STDCALL
IDEIsr(IN PKINTERRUPT Interrupt,
       IN PVOID ServiceContext);

static VOID
IDEDpcForIsr(IN PKDPC Dpc,
	     IN PDEVICE_OBJECT DpcDeviceObject,
	     IN PIRP DpcIrp,
	     IN PVOID DpcContext);

static VOID
IDEFinishOperation(PIDE_CONTROLLER_EXTENSION ControllerExtension);

static VOID STDCALL
IDEIoTimer(PDEVICE_OBJECT DeviceObject,
	   PVOID Context);


static ULONG
AtapiExecuteScsi(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
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
	      /* both channels unclaimed: claim primary channel */
	      DPRINT1("Primary channel!\n");

	      ConfigInfo->BusInterruptLevel = 14;
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
	      /* primary channel claimed: claim secondary channel */
	      DPRINT1("Secondary channel!\n");

	      ConfigInfo->BusInterruptLevel = 15;
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
  PATAPI_MINIPORT_EXTENSION DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  ULONG Result;

  DPRINT1("AtapiStartIo() called\n");

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:

	Result = AtapiExecuteScsi(DevExt,
				  Srb);

	break;

    }

  Srb->SrbStatus = Result;

  return(TRUE);
}


static BOOLEAN STDCALL
AtapiInterrupt(IN PVOID DeviceExtension)
{
  return(TRUE);
}






//  ----------------------------------------------------  Discardable statics


static BOOLEAN
AtapiFindDevices(PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
  BOOLEAN DeviceFound = FALSE;
  ULONG CommandPortBase;
  ULONG UnitNumber;
  ULONG Retries;
  BYTE High, Low;

  DPRINT("AtapiFindDevices() called\n");

//  CommandPortBase = ScsiPortConvertPhysicalAddressToUlong((*ConfigInfo->AccessRanges)[0].RangeStart);
  CommandPortBase = ScsiPortConvertPhysicalAddressToUlong(ConfigInfo->AccessRanges[0].RangeStart);

  DPRINT("  CommandPortBase: %x\n", CommandPortBase);

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
	  if (AtapiIdentifyATAPIDevice(CommandPortBase,
				       UnitNumber,
				       &DeviceExtension->DeviceParms[UnitNumber]))
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
	  if (AtapiIdentifyATADevice(CommandPortBase,
				     UnitNumber,
				     &DeviceExtension->DeviceParms[UnitNumber]))
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


//    IDEResetController
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
IDEResetController(IN WORD CommandPort,
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


//    AtapiIdentifyATADevice
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
AtapiIdentifyATADevice(IN ULONG CommandPort,
                       IN ULONG DriveNum,
                       OUT PIDE_DRIVE_IDENTIFY DrvParms)
{
  /*  Get the Drive Identify block from drive or die  */
  if (IDEPolledRead((WORD)CommandPort, 0, 1, 0, 0, 0, (DriveNum ? IDE_DH_DRV1 : 0),
                    IDE_CMD_IDENT_ATA_DRV, (BYTE *)DrvParms) != 0) 
    {
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
    DrvParms->BytesPerSector = 512; /* FIXME !!!*/
  return TRUE;
}


static BOOLEAN
AtapiIdentifyATAPIDevice(IN ULONG CommandPort,
			 IN ULONG DriveNum,
			 OUT PIDE_DRIVE_IDENTIFY DrvParms)
{
  /*  Get the Drive Identify block from drive or die  */
  if (IDEPolledRead((WORD)CommandPort, 0, 1, 0, 0, 0, (DriveNum ? IDE_DH_DRV1 : 0),
                    IDE_CMD_IDENT_ATAPI_DRV, (BYTE *)DrvParms) != 0) /* atapi_identify */
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
//  DrvParms->BytesPerSector = 512; /* FIXME !!!*/
  return TRUE;
}


//    IDEPolledRead
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
IDEPolledRead(IN WORD Address,
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
	      return(IDE_ER_ABRT);
	    }
	  if (Status & IDE_SR_DRQ)
	    {
	      break;
	    }
	  else
	    {
	      return(IDE_ER_ABRT);
	    }
	}
      ScsiPortStallExecution(10);
    }

  /*  timed out  */
  if (RetryCount >= IDE_MAX_POLL_RETRIES)
    {
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
		  return(0);
		}
	    }
	}
    }
}


//  -------------------------------------------  Nondiscardable statics

//    IDEDispatchReadWrite
//
//  DESCRIPTION:
//    Answer requests for reads and writes
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static NTSTATUS STDCALL
IDEDispatchReadWrite(IN PDEVICE_OBJECT pDO,
                     IN PIRP Irp)
{
  ULONG                  IrpInsertKey;
  LARGE_INTEGER          AdjustedOffset, AdjustedExtent, PartitionExtent, InsertKeyLI;
  PIO_STACK_LOCATION     IrpStack;
  PIDE_DEVICE_EXTENSION  DeviceExtension;

  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = (PIDE_DEVICE_EXTENSION)pDO->DeviceExtension;

    //  Validate operation parameters
  AdjustedOffset = RtlEnlargedIntegerMultiply(DeviceExtension->Offset, 
                                              DeviceExtension->BytesPerSector);
DPRINT("Offset:%ld * BytesPerSector:%ld  = AdjOffset:%ld:%ld\n",
       DeviceExtension->Offset, 
       DeviceExtension->BytesPerSector,
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart);
DPRINT("AdjOffset:%ld:%ld + ByteOffset:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.HighPart,
       (unsigned long) IrpStack->Parameters.Read.ByteOffset.u.LowPart);
  AdjustedOffset = RtlLargeIntegerAdd(AdjustedOffset, 
                                      IrpStack->Parameters.Read.ByteOffset);
DPRINT(" = AdjOffset:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart);
  AdjustedExtent = RtlLargeIntegerAdd(AdjustedOffset, 
                                      RtlConvertLongToLargeInteger(IrpStack->Parameters.Read.Length));
DPRINT("AdjOffset:%ld:%ld + Length:%ld = AdjExtent:%ld:%ld\n",
       (unsigned long) AdjustedOffset.u.HighPart,
       (unsigned long) AdjustedOffset.u.LowPart,
       IrpStack->Parameters.Read.Length,
       (unsigned long) AdjustedExtent.u.HighPart,
       (unsigned long) AdjustedExtent.u.LowPart);
    /*FIXME: this assumption will fail on drives bigger than 1TB */
  PartitionExtent.QuadPart = DeviceExtension->Offset + DeviceExtension->Size;
  PartitionExtent = RtlExtendedIntegerMultiply(PartitionExtent, 
                                               DeviceExtension->BytesPerSector);
  if ((AdjustedExtent.QuadPart > PartitionExtent.QuadPart) ||
      (IrpStack->Parameters.Read.Length & (DeviceExtension->BytesPerSector - 1))) 
    {
      DPRINT("Request failed on bad parameters\n",0);
      DPRINT("AdjustedExtent=%d:%d PartitionExtent=%d:%d ReadLength=%d\n", 
             (unsigned int) AdjustedExtent.u.HighPart,
             (unsigned int) AdjustedExtent.u.LowPart,
             (unsigned int) PartitionExtent.u.HighPart,
             (unsigned int) PartitionExtent.u.LowPart,
             IrpStack->Parameters.Read.Length);
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return  STATUS_INVALID_PARAMETER;
    }

    //  Adjust operation to absolute sector offset
  IrpStack->Parameters.Read.ByteOffset = AdjustedOffset;

    //  Start the packet and insert the request in order of sector offset
  assert(DeviceExtension->BytesPerSector == 512);
  InsertKeyLI = RtlLargeIntegerShiftRight(IrpStack->Parameters.Read.ByteOffset, 9); 
  IrpInsertKey = InsertKeyLI.u.LowPart;
  IoStartPacket(DeviceExtension->DeviceObject, Irp, &IrpInsertKey, NULL);
   
  DPRINT("Returning STATUS_PENDING\n");
  return  STATUS_PENDING;
}


//    IDEStartIo
//
//  DESCRIPTION:
//    Get the next requested I/O packet started
//
//  RUN LEVEL:
//    DISPATCH_LEVEL
//
//  ARGUMENTS:
//    Dispatch routine standard arguments
//
//  RETURNS:
//    NTSTATUS
//

static VOID STDCALL
IDEStartIo(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
  LARGE_INTEGER              SectorLI;
  PIO_STACK_LOCATION         IrpStack;
  PIDE_DEVICE_EXTENSION      DeviceExtension;
  KIRQL                      OldIrql;
  
  DPRINT("IDEStartIo() called!\n");
  
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // FIXME: implement the supported functions

  switch (IrpStack->MajorFunction) 
    {
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
      DeviceExtension->Operation = IrpStack->MajorFunction;
      DeviceExtension->BytesRequested = IrpStack->Parameters.Read.Length;
      assert(DeviceExtension->BytesPerSector == 512);
      SectorLI = RtlLargeIntegerShiftRight(IrpStack->Parameters.Read.ByteOffset, 9);
      DeviceExtension->StartingSector = SectorLI.u.LowPart;
      if (DeviceExtension->BytesRequested > DeviceExtension->BytesPerSector * 
          IDE_MAX_SECTORS_PER_XFER) 
        {
          DeviceExtension->BytesToTransfer = DeviceExtension->BytesPerSector * 
              IDE_MAX_SECTORS_PER_XFER;
        } 
      else 
        {
          DeviceExtension->BytesToTransfer = DeviceExtension->BytesRequested;
        }
      DeviceExtension->BytesRequested -= DeviceExtension->BytesToTransfer;
      DeviceExtension->SectorsTransferred = 0;
      DeviceExtension->TargetAddress = (BYTE *)MmGetSystemAddressForMdl(Irp->MdlAddress);
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
      IoAllocateController(DeviceExtension->ControllerObject,
                           DeviceObject, 
                           IDEAllocateController, 
                           NULL);
      KeLowerIrql(OldIrql);
      break;

    default:
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Information = 0;
      KeBugCheck((ULONG)Irp);
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      IoStartNextPacket(DeviceObject, FALSE);
      break;
    }
  DPRINT("IDEStartIo() finished!\n");
}

//    IDEAllocateController

static IO_ALLOCATION_ACTION STDCALL
IDEAllocateController(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp,
                      IN PVOID MapRegisterBase,
                      IN PVOID Ccontext) 
{
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

  DeviceExtension = (PIDE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) 
      DeviceExtension->ControllerObject->ControllerExtension;
  ControllerExtension->CurrentIrp = Irp;
  ControllerExtension->Retries = 0;
  return KeSynchronizeExecution(ControllerExtension->Interrupt,
                                IDEStartController,
                                DeviceExtension) ? KeepObject : 
                                  DeallocateObject;
}

//    IDEStartController

static BOOLEAN STDCALL
IDEStartController(IN OUT PVOID Context)
{
  BYTE  SectorCnt, SectorNum, CylinderLow, CylinderHigh;
  BYTE  DrvHead, Command;
  BYTE  Status;
  int  Retries;
  ULONG  StartingSector;
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;
  PIRP  Irp;

  DeviceExtension = (PIDE_DEVICE_EXTENSION) Context;
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION)
      DeviceExtension->ControllerObject->ControllerExtension;
  ControllerExtension->OperationInProgress = TRUE;
  ControllerExtension->DeviceForOperation = DeviceExtension;

    //  Write controller registers to start opteration
  StartingSector = DeviceExtension->StartingSector;
  SectorCnt = DeviceExtension->BytesToTransfer / 
      DeviceExtension->BytesPerSector;
  if (DeviceExtension->LBASupported) 
    {
      SectorNum = StartingSector & 0xff;
      CylinderLow = (StartingSector >> 8) & 0xff;
      CylinderHigh = (StartingSector >> 16) & 0xff;
      DrvHead = ((StartingSector >> 24) & 0x0f) | 
          (DeviceExtension->UnitNumber ? IDE_DH_DRV1 : 0) |
          IDE_DH_LBA;
    } 
  else 
    {
      SectorNum = (StartingSector % DeviceExtension->SectorsPerLogTrk) + 1;
      StartingSector /= DeviceExtension->SectorsPerLogTrk;
      DrvHead = (StartingSector % DeviceExtension->LogicalHeads) | 
          (DeviceExtension->UnitNumber ? IDE_DH_DRV1 : 0);
      StartingSector /= DeviceExtension->LogicalHeads;
      CylinderLow = StartingSector & 0xff;
      CylinderHigh = StartingSector >> 8;
    }
  Command = DeviceExtension->Operation == IRP_MJ_READ ? 
     IDE_CMD_READ : IDE_CMD_WRITE;
  if (DrvHead & IDE_DH_LBA) 
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
             ControllerExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             ((DrvHead & 0x0f) << 24) +
             (CylinderHigh << 16) + (CylinderLow << 8) + SectorNum,
             SectorCnt, 
             Command);
    } 
  else 
    {
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
             DeviceExtension->Operation == IRP_MJ_READ ? "READ" : "WRITE",
             ControllerExtension->CommandPortBase,
             DrvHead & IDE_DH_DRV1 ? 1 : 0, 
             CylinderHigh, 
             CylinderLow, 
             DrvHead & 0x0f, 
             SectorNum, 
             SectorCnt, 
             Command);
    }

  /*  wait for BUSY to clear  */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(ControllerExtension->CommandPortBase);
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
    }

  /*  Select the desired drive  */
  IDEWriteDriveHead(ControllerExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  wait for BUSY to clear and DRDY to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(ControllerExtension->CommandPortBase);
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
    }

  /*  Setup command parameters  */
  IDEWritePrecomp(ControllerExtension->CommandPortBase, 0);
  IDEWriteSectorCount(ControllerExtension->CommandPortBase, SectorCnt);
  IDEWriteSectorNum(ControllerExtension->CommandPortBase, SectorNum);
  IDEWriteCylinderHigh(ControllerExtension->CommandPortBase, CylinderHigh);
  IDEWriteCylinderLow(ControllerExtension->CommandPortBase, CylinderLow);
  IDEWriteDriveHead(ControllerExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /*  Issue command to drive  */
  IDEWriteCommand(ControllerExtension->CommandPortBase, Command);
  ControllerExtension->TimerState = IDETimerCmdWait;
  ControllerExtension->TimerCount = IDE_CMD_TIMEOUT;
  
  if (DeviceExtension->Operation == IRP_MJ_WRITE) 
    {

        //  Wait for controller ready
      for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++) 
        {
          BYTE  Status = IDEReadStatus(ControllerExtension->CommandPortBase);
          if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR)) 
            {
              break;
            }
          ScsiPortStallExecution(10);
        }
      if (Retries >= IDE_MAX_BUSY_RETRIES)
        {
          if (ControllerExtension->Retries++ > IDE_MAX_CMD_RETRIES)
            {
              Irp = ControllerExtension->CurrentIrp;
              Irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
              Irp->IoStatus.Information = 0;

              return FALSE;
            }
          else
            {
              IDEBeginControllerReset(ControllerExtension);

              return TRUE;
            }
        }

        //  Load the first sector of data into the controller
      IDEWriteBlock(ControllerExtension->CommandPortBase, 
                    DeviceExtension->TargetAddress,
                    IDE_SECTOR_BUF_SZ);
      DeviceExtension->TargetAddress += IDE_SECTOR_BUF_SZ;
      DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
      DeviceExtension->SectorsTransferred++;
    }
  DPRINT ("Command issued to drive, IDEStartController done\n");

  return  TRUE;
}

//    IDEBeginControllerReset

VOID 
IDEBeginControllerReset(PIDE_CONTROLLER_EXTENSION ControllerExtension)
{
  int Retries;

  DPRINT("Controller Reset initiated on %04x\n", 
         ControllerExtension->ControlPortBase);

    /*  Assert drive reset line  */
  DPRINT("Asserting reset line\n");
  IDEWriteDriveControl(ControllerExtension->ControlPortBase, IDE_DC_SRST);

    /*  Wait for BSY assertion  */
  DPRINT("Waiting for BSY assertion\n");
  for (Retries = 0; Retries < IDE_MAX_RESET_RETRIES; Retries++) 
    {
      BYTE Status = IDEReadStatus(ControllerExtension->CommandPortBase);
      if ((Status & IDE_SR_BUSY)) 
        {
          break;
        }
      ScsiPortStallExecution(10);
    }
  if (Retries == IDE_MAX_RESET_RETRIES)
    {
      DPRINT("Timeout on BSY assertion\n");
    }

    /*  Negate drive reset line  */
  DPRINT("Negating reset line\n");
  IDEWriteDriveControl(ControllerExtension->ControlPortBase, 0);

  // FIXME: handle case of no device 0

    /*  Set timer to check for end of reset  */
  ControllerExtension->TimerState = IDETimerResetWaitForBusyNegate;
  ControllerExtension->TimerCount = IDE_RESET_BUSY_TIMEOUT;
}

//    IDEIsr
//
//  DESCIPTION:
//    Handle interrupts for IDE devices
//
//  RUN LEVEL:
//    DIRQL
//
//  ARGUMENTS:
//    IN  PKINTERRUPT  Interrupt       The interrupt level in effect
//    IN  PVOID        ServiceContext  The driver supplied context
//                                     (the controller extension)
//  RETURNS:
//    TRUE   This ISR handled the interrupt
//    FALSE  Another ISR must handle this interrupt

static BOOLEAN STDCALL
IDEIsr(IN PKINTERRUPT Interrupt,
       IN PVOID ServiceContext)
{
  BOOLEAN   IsLastBlock, AnErrorOccured, RequestIsComplete;
  BYTE     *TargetAddress;
  int       Retries;
  NTSTATUS  ErrorStatus;
  ULONG     ErrorInformation;
  PIRP  Irp;
  PIDE_DEVICE_EXTENSION  DeviceExtension;
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

  if (IDEInitialized == FALSE)
    {
      return FALSE;
    }
  DPRINT ("IDEIsr called\n");

  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) ServiceContext;

    //  Read the status port to clear the interrtupt (even if it's not ours).
  ControllerExtension->DeviceStatus = IDEReadStatus(ControllerExtension->CommandPortBase);

    //  If the interrupt is not ours, get the heck outta dodge.
  if (!ControllerExtension->OperationInProgress)
    {
      return FALSE;
    }

  DeviceExtension = ControllerExtension->DeviceForOperation;
  IsLastBlock = FALSE;
  AnErrorOccured = FALSE;
  RequestIsComplete = FALSE;
  ErrorStatus = STATUS_SUCCESS;
  ErrorInformation = 0;

    //  Handle error condition if it exists
  if (ControllerExtension->DeviceStatus & IDE_SR_ERR) 
    {
      BYTE ErrorReg, SectorCount, SectorNum, CylinderLow, CylinderHigh;
      BYTE DriveHead;

        //  Log the error
      ErrorReg = IDEReadError(ControllerExtension->CommandPortBase);
      CylinderLow = IDEReadCylinderLow(ControllerExtension->CommandPortBase);
      CylinderHigh = IDEReadCylinderHigh(ControllerExtension->CommandPortBase);
      DriveHead = IDEReadDriveHead(ControllerExtension->CommandPortBase);
      SectorCount = IDEReadSectorCount(ControllerExtension->CommandPortBase);
      SectorNum = IDEReadSectorNum(ControllerExtension->CommandPortBase);
        // FIXME: should use the NT error logging facility
      DbgPrint ("IDE Error: OP:%02x STAT:%02x ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n", 
                DeviceExtension->Operation, 
                ControllerExtension->DeviceStatus, 
                ErrorReg, 
                CylinderLow,
                CylinderHigh, 
                SectorCount, 
                SectorNum);

        // FIXME: should retry the command and perhaps recalibrate the drive

        //  Set error status information
      AnErrorOccured = TRUE;
      ErrorStatus = STATUS_DISK_OPERATION_FAILED;
      ErrorInformation = 
          (((((((CylinderHigh << 8) + CylinderLow) * 
              DeviceExtension->LogicalHeads) + 
              (DriveHead % DeviceExtension->LogicalHeads)) * 
              DeviceExtension->SectorsPerLogTrk) + SectorNum - 1) -
          DeviceExtension->StartingSector) * DeviceExtension->BytesPerSector;
    } 
  else 
    {

        // Check controller and setup for next transfer
      switch (DeviceExtension->Operation) 
        {
        case  IRP_MJ_READ:

            //  Update controller/device state variables
          TargetAddress = DeviceExtension->TargetAddress;
          DeviceExtension->TargetAddress += DeviceExtension->BytesPerSector;
          DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
          DeviceExtension->SectorsTransferred++;

            //  Remember whether DRQ should be low at end (last block read)
          IsLastBlock = DeviceExtension->BytesToTransfer == 0;

            //  Wait for DRQ assertion
          for (Retries = 0; Retries < IDE_MAX_DRQ_RETRIES &&
              !(IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ);
              Retries++) 
            {
              ScsiPortStallExecution(10);
            }

            //  Copy the block of data
          IDEReadBlock(ControllerExtension->CommandPortBase, 
                       TargetAddress,
                       IDE_SECTOR_BUF_SZ);

            //  check DRQ
          if (IsLastBlock) 
            {
              for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES &&
                  (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_BUSY);
                  Retries++) 
                {
                  ScsiPortStallExecution(10);
                }

                //  Check for data overrun
              if (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ) 
                {
                  AnErrorOccured = TRUE;
                  ErrorStatus = STATUS_DATA_OVERRUN;
                  ErrorInformation = 0;
                } 
              else 
                {

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
                }
            }
          break;

        case IRP_MJ_WRITE:

            //  check DRQ
          if (DeviceExtension->BytesToTransfer == 0) 
            {
              for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES &&
                  (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_BUSY);
                   Retries++) 
                {
                  ScsiPortStallExecution(10);
                }

                //  Check for data overrun
              if (IDEReadStatus(ControllerExtension->CommandPortBase) & IDE_SR_DRQ) 
                {
                  AnErrorOccured = TRUE;
                  ErrorStatus = STATUS_DATA_OVERRUN;
                  ErrorInformation = 0;
                } 
              else 
                {

                    //  Setup next transfer or set RequestIsComplete
                  IsLastBlock = TRUE;
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
                }
            } 
          else 
            {

                //  Update controller/device state variables
              TargetAddress = DeviceExtension->TargetAddress;
              DeviceExtension->TargetAddress += DeviceExtension->BytesPerSector;
              DeviceExtension->BytesToTransfer -= DeviceExtension->BytesPerSector;
              DeviceExtension->SectorsTransferred++;

                //  Write block to controller
              IDEWriteBlock(ControllerExtension->CommandPortBase, 
                            TargetAddress,
                            IDE_SECTOR_BUF_SZ);
            }
          break;
        }
    }

  //  If there was an error or the request is done, complete the packet
  if (AnErrorOccured || RequestIsComplete) 
    {
      //  Set the return status and info values
      Irp = ControllerExtension->CurrentIrp;
      Irp->IoStatus.Status = ErrorStatus;
      Irp->IoStatus.Information = ErrorInformation;

      //  Clear out controller fields
      ControllerExtension->OperationInProgress = FALSE;
      ControllerExtension->DeviceStatus = 0;

      //  Queue the Dpc to finish up
      IoRequestDpc(DeviceExtension->DeviceObject, 
                   Irp, 
                   ControllerExtension);
    } 
  else if (IsLastBlock)
    {
      //  Else more data is needed, setup next device I/O
      IDEStartController((PVOID)DeviceExtension);
    }

  return TRUE;
}

//    IDEDpcForIsr
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID
IDEDpcForIsr(IN PKDPC Dpc,
             IN PDEVICE_OBJECT DpcDeviceObject,
             IN PIRP DpcIrp,
             IN PVOID DpcContext)
{
  DPRINT("IDEDpcForIsr()\n");
  IDEFinishOperation((PIDE_CONTROLLER_EXTENSION) DpcContext);
}

//    IDEFinishOperation

static VOID
IDEFinishOperation(PIDE_CONTROLLER_EXTENSION ControllerExtension)
{
  PIDE_DEVICE_EXTENSION DeviceExtension;
  PIRP Irp;
  ULONG Operation;

  DeviceExtension = ControllerExtension->DeviceForOperation;
  Irp = ControllerExtension->CurrentIrp;
  Operation = DeviceExtension->Operation;
  ControllerExtension->OperationInProgress = FALSE;
  ControllerExtension->DeviceForOperation = 0;
  ControllerExtension->CurrentIrp = 0;

  //  Deallocate the controller
  IoFreeController(DeviceExtension->ControllerObject);

  //  Start the next packet
  IoStartNextPacketByKey(DeviceExtension->DeviceObject, 
                         FALSE, 
                         DeviceExtension->StartingSector);

  //  Issue completion of the current packet
  IoCompleteRequest(Irp, IO_DISK_INCREMENT);

  //  Flush cache if necessary
  if (Operation == IRP_MJ_READ) 
    {
      KeFlushIoBuffers(Irp->MdlAddress, TRUE, FALSE);
    }
}

//    IDEIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID STDCALL
IDEIoTimer(PDEVICE_OBJECT DeviceObject,
	   PVOID Context)
{
  PIDE_CONTROLLER_EXTENSION  ControllerExtension;

    //  Setup Extension pointer
  ControllerExtension = (PIDE_CONTROLLER_EXTENSION) Context;
  DPRINT("Timer activated for %04lx\n", ControllerExtension->CommandPortBase);

    //  Handle state change if necessary
  switch (ControllerExtension->TimerState) 
    {
      case IDETimerResetWaitForBusyNegate:
        if (!(IDEReadStatus(ControllerExtension->CommandPortBase) & 
            IDE_SR_BUSY))
          {
            DPRINT("Busy line has negated, waiting for DRDY assert\n");
            ControllerExtension->TimerState = IDETimerResetWaitForDrdyAssert;
            ControllerExtension->TimerCount = IDE_RESET_DRDY_TIMEOUT;
            return;
          }
        break;
        
      case IDETimerResetWaitForDrdyAssert:
        if (IDEReadStatus(ControllerExtension->CommandPortBase) & 
            IDE_SR_DRQ)
          {
            DPRINT("DRDY has asserted, reset complete\n");
            ControllerExtension->TimerState = IDETimerIdle;
            ControllerExtension->TimerCount = 0;

              // FIXME: get diagnostic code from drive 0

              /*  Start current packet command again  */
            if (!KeSynchronizeExecution(ControllerExtension->Interrupt, 
                                        IDEStartController,
                                        ControllerExtension->DeviceForOperation))
              {
                IDEFinishOperation(ControllerExtension);
              }
            return;
          }
        break;

      default:
        break;
    }

    //  If we're counting down, then count.
  if (ControllerExtension->TimerCount > 0) 
    {
      ControllerExtension->TimerCount--;

      //  Else we'll check the state and process if necessary
    } 
  else 
    {
      switch (ControllerExtension->TimerState) 
        {
          case IDETimerIdle:
            break;

          case IDETimerCmdWait:
              /*  Command timed out, reset drive and try again or fail  */
            DPRINT("Timeout waiting for command completion\n");
            if (++ControllerExtension->Retries > IDE_MAX_CMD_RETRIES)
              {
		 if (ControllerExtension->CurrentIrp != NULL)
		   {
                      DbgPrint ("Max retries has been reached, IRP finished with error\n");
		      ControllerExtension->CurrentIrp->IoStatus.Status = STATUS_IO_TIMEOUT;
		      ControllerExtension->CurrentIrp->IoStatus.Information = 0;
		      IDEFinishOperation(ControllerExtension);
		   }
                ControllerExtension->TimerState = IDETimerIdle;
                ControllerExtension->TimerCount = 0;
              }
            else
              {
                IDEBeginControllerReset(ControllerExtension);
              }
            break;

          case IDETimerResetWaitForBusyNegate:
          case IDETimerResetWaitForDrdyAssert:
            if (ControllerExtension->CurrentIrp != NULL)
              {
                DbgPrint ("Timeout waiting for drive reset, giving up on IRP\n");
                ControllerExtension->CurrentIrp->IoStatus.Status = 
                  STATUS_IO_TIMEOUT;
                ControllerExtension->CurrentIrp->IoStatus.Information = 0;
                IDEFinishOperation(ControllerExtension);
              }
            ControllerExtension->TimerState = IDETimerIdle;
            ControllerExtension->TimerCount = 0;
            break;
        }
    }
}


static ULONG
AtapiExecuteScsi(IN PATAPI_MINIPORT_EXTENSION DeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb)
{
  ULONG SrbStatus = SRB_STATUS_SUCCESS;

  DPRINT1("AtapiExecuteScsi() called!\n");

  DPRINT1("PathId: %lu  TargetId: %lu  Lun: %lu\n",
	  Srb->PathId,
	  Srb->TargetId,
	  Srb->Lun);

  switch (Srb->Cdb[0])
    {
      case SCSIOP_INQUIRY:
	if ((Srb->PathId == 0) &&
	    (Srb->TargetId < 2) &&
	    (Srb->Lun == 0) &&
	    (DeviceExtension->DevicePresent[Srb->TargetId] == TRUE))
	  {
	    PINQUIRYDATA InquiryData;
	    PIDE_DRIVE_IDENTIFY DeviceParams;
	    ULONG i;

	    DPRINT1("SCSIOP_INQUIRY: TargetId: %lu\n", Srb->TargetId);

	    InquiryData = Srb->DataBuffer;
	    DeviceParams = &DeviceExtension->DeviceParms[Srb->TargetId];

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

	    DPRINT1("ConfigBits: 0x%x\n", DeviceParams->ConfigBits);
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

	    SrbStatus = SRB_STATUS_SUCCESS;
	  }
	else
	  {
	    SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
	  }
	break;

     case SCSIOP_MODE_SENSE:
	break;

    }

  DPRINT1("AtapiExecuteScsi() done!\n");

  return(SrbStatus);
}


/* EOF */
