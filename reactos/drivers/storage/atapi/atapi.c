/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: atapi.c,v 1.13 2002/03/16 16:13:57 ekohl Exp $
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

#define ENABLE_PCI
#define ENABLE_ISA

//  -------------------------------------------------------------------------

#include <ddk/ntddk.h>

#include "../include/srb.h"
#include "../include/scsi.h"
#include "../include/ntddscsi.h"

#include "atapi.h"

#define NDEBUG
#include <debug.h>

#define VERSION  "0.0.1"


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
IDEResetController(IN ULONG CommandPort,
		   IN ULONG ControlPort);

static int
AtapiPolledRead(IN ULONG CommandPort,
		IN ULONG ControlPort,
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
#ifdef ENABLE_PCI
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
#endif

  /* Search the ISA bus for ide controllers */
#ifdef ENABLE_ISA
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

  DPRINT( "Returning from DriverEntry\n" );

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
	  DPRINT("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
		 ConfigInfo->SystemIoBusNumber,
		 SlotNumber.u.bits.DeviceNumber,
		 SlotNumber.u.bits.FunctionNumber,
		 PciConfig.VendorID,
		 PciConfig.DeviceID);
	  DPRINT("ProgIF 0x%02hx\n", PciConfig.ProgIf);

	  DPRINT("Found IDE controller in compatibility mode!\n");

	  ConfigInfo->NumberOfBuses = 1;
	  ConfigInfo->MaximumNumberOfTargets = 2;
	  ConfigInfo->MaximumTransferLength = 0x10000; /* max 64Kbyte */

	  if (ConfigInfo->AtdiskPrimaryClaimed == FALSE)
	    {
	      /* Both channels unclaimed: Claim primary channel */
	      DPRINT("Primary channel!\n");

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
	      DPRINT("Secondary channel!\n");

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
  BOOLEAN ChannelFound = FALSE;
  BOOLEAN DeviceFound = FALSE;

  DPRINT1("AtapiFindIsaBusController() called!\n");

  *Again = FALSE;

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
      *Again = FALSE/*TRUE*/;
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
  else
    {
      DPRINT1("AtapiFindIsaBusController() both channels claimed. Returns: SP_RETURN_NOT_FOUND\n");
      *Again = FALSE;
      return(SP_RETURN_NOT_FOUND);
    }

  /* Find attached devices */
  if (ChannelFound)
    {
      DeviceFound = AtapiFindDevices(DevExt,
				     ConfigInfo);
    }

  DPRINT1("AtapiFindIsaBusController() returns: SP_RETURN_FOUND\n");
  return(SP_RETURN_FOUND);
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

  DPRINT("AtapiStartIo() called\n");

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

      ScsiPortNotification(RequestComplete,
			   DeviceExtension,
			   Srb);
      ScsiPortNotification(NextRequest,
			   DeviceExtension,
			   NULL);
    }
  else
    {
      DPRINT("SrbStatus = SRB_STATUS_PENDING\n");
    }

  DPRINT("AtapiStartIo() done\n");

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
  BOOLEAN IsLastBlock;
  ULONG Retries;
  NTSTATUS ErrorStatus;
  ULONG ErrorInformation;
  PUCHAR TargetAddress;
  ULONG SectorSize;


  DPRINT("AtapiInterrupt() called!\n");

  DevExt = (PATAPI_MINIPORT_EXTENSION)DeviceExtension;
  if (DevExt->ExpectingInterrupt == FALSE)
    {
      return(FALSE);
    }

  Srb = DevExt->CurrentSrb;

  DPRINT("Srb: %p\n", Srb);

  CommandPortBase = DevExt->CommandPortBase;
  ControlPortBase = DevExt->ControlPortBase;

  DPRINT("CommandPortBase: %lx  ControlPortBase: %lx\n", CommandPortBase, ControlPortBase);

  IsLastBlock = FALSE;
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
      DbgPrint("ATAPI Error: OP:%02x STAT:%02x ERR:%02x CYLLO:%02x CYLHI:%02x SCNT:%02x SNUM:%02x\n",
	       0, //DeviceExtension->Operation,
	       DeviceStatus,
	       ErrorReg,
	       CylinderLow,
	       CylinderHigh,
	       SectorCount,
	       SectorNum);

      /* FIXME: should retry the command and perhaps recalibrate the drive */

        //  Set error status information
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
	    DPRINT("SCSIOP_READ\n");

	    /* Update controller/device state variables */
	    SectorSize = DevExt->DeviceParams[Srb->TargetId].BytesPerSector;
	    TargetAddress = Srb->DataBuffer;
	    Srb->DataBuffer += SectorSize;
	    Srb->DataTransferLength -= SectorSize;

	    /* Remember whether DRQ should be low at end (last block read) */
	    IsLastBlock = (Srb->DataTransferLength == 0);
	    DPRINT("IsLastBlock == %s\n", (IsLastBlock)?"TRUE":"FALSE");

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
			 SectorSize);

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
		    /* FIXME: Handle error! */
		  }
	      }
	    break;

	  case SCSIOP_WRITE:
	    DPRINT("SCSIOP_WRITE\n");
	    if (Srb->DataTransferLength == 0)
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
		    /* FIXME: Handle error! */
		  }

		DevExt->ExpectingInterrupt = FALSE;
		IsLastBlock = TRUE;
	      }
	    else
	      {
		/* Update SRB data */
		SectorSize = DevExt->DeviceParams[Srb->TargetId].BytesPerSector;

		TargetAddress = Srb->DataBuffer;
		Srb->DataBuffer += SectorSize;
		Srb->DataTransferLength -= SectorSize;

		/* Write the sector */
		IDEWriteBlock(CommandPortBase,
			      TargetAddress,
			      SectorSize);
	      }
	    break;
      }
    }

  /* complete this packet */
  if (IsLastBlock)
    {
      DevExt->ExpectingInterrupt = FALSE;

      ScsiPortNotification(RequestComplete,
			   DeviceExtension,
			   Srb);

      ScsiPortNotification(NextRequest,
			   DeviceExtension,
			   NULL);
    }

  DPRINT("AtapiInterrupt() done!\n");

  return(TRUE);
}






//  ----------------------------------------------------  Discardable statics


/**********************************************************************
 * NAME							INTERNAL
 *	AtapiFindDevices
 *
 * DESCRIPTION
 *	Searches for devices on the given port.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		Port device specific information.
 *
 *	ConfigInfo
 *		Port configuration information.
 *
 * RETURN VALUE
 *	TRUE: At least one device is attached to the port.
 *	FALSE: No device is attached to the port.
 */

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
      /* Select drive */
      IDEWriteDriveHead(CommandPortBase,
			IDE_DH_FIXED | (UnitNumber ? IDE_DH_DRV1 : 0));
      ScsiPortStallExecution(500);

      /* Disable interrupts */
      IDEWriteDriveControl(ControlPortBase,
			   IDE_DC_nIEN);
      ScsiPortStallExecution(500);

      IDEWriteCylinderHigh(CommandPortBase, 0);
      IDEWriteCylinderLow(CommandPortBase, 0);
      IDEWriteCommand(CommandPortBase, IDE_CMD_RESET);

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
AtapiResetController(IN ULONG CommandPort,
		     IN ULONG ControlPort)
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

/*
 *  AtapiIdentifyDevice
 *
 *  DESCRIPTION:
 *	Get the identification block from the drive
 *
 *  RUN LEVEL:
 *	PASSIVE_LEVEL
 *
 *  ARGUMENTS:
 *	CommandPort
 *		Address of the command port
 *	ControlPort
 *		Address of the control port
 *	DriveNum
 *		The drive index (0,1)
 *	Atapi
 *		Send an ATA(FALSE) or an ATAPI(TRUE) identify comand
 *	DrvParms
 *		Address to write drive ident block
 *
 *  RETURNS:
 *	TRUE: The drive identification block was retrieved successfully
 *	FALSE: an error ocurred
 */

static BOOLEAN
AtapiIdentifyDevice(IN ULONG CommandPort,
		    IN ULONG ControlPort,
		    IN ULONG DriveNum,
		    IN BOOLEAN Atapi,
		    OUT PIDE_DRIVE_IDENTIFY DrvParms)
{
  /*  Get the Drive Identify block from drive or die  */
  if (AtapiPolledRead(CommandPort,
		      ControlPort,
		      0,
		      1,
		      0,
		      0,
		      0,
		      (DriveNum ? IDE_DH_DRV1 : 0),
		      (Atapi ? IDE_CMD_IDENT_ATAPI_DRV : IDE_CMD_IDENT_ATA_DRV),
		      (BYTE *)DrvParms) != 0)
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
AtapiPolledRead(IN ULONG CommandPort,
		IN ULONG ControlPort,
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

  /*  Write Drive/Head to select drive  */
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);
  ScsiPortStallExecution(500);

  /* Disable interrupts */
  Control = IDEReadAltStatus(ControlPort);
  IDEWriteDriveControl(ControlPort, Control | IDE_DC_nIEN);
  ScsiPortStallExecution(500);

  /*  Wait for STATUS.BUSY and STATUS.DRQ to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
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
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);

  /*  Wait for STATUS.BUSY and STATUS.DRQ to clear  */
  for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
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
  IDEWritePrecomp(CommandPort, PreComp);
  IDEWriteSectorCount(CommandPort, SectorCnt);
  IDEWriteSectorNum(CommandPort, SectorNum);
  IDEWriteCylinderHigh(CommandPort, CylinderHigh);
  IDEWriteCylinderLow(CommandPort, CylinderLow);
  IDEWriteDriveHead(CommandPort, IDE_DH_FIXED | DrvHead);

  /*  Issue the command  */
  IDEWriteCommand(CommandPort, Command);
  ScsiPortStallExecution(50);

  /*  wait for DRQ or error  */
  for (RetryCount = 0; RetryCount < IDE_MAX_POLL_RETRIES; RetryCount++)
    {
      Status = IDEReadStatus(CommandPort);
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
	  IDEReadBlock(CommandPort, Buffer, IDE_SECTOR_BUF_SZ);
	  Buffer += IDE_SECTOR_BUF_SZ;
	}
      else
	{
	  UCHAR JunkBuffer[IDE_SECTOR_BUF_SZ];
	  IDEReadBlock(CommandPort, JunkBuffer, IDE_SECTOR_BUF_SZ);
	}
      SectorCount++;

      /*  Check for error or more sectors to read  */
      for (RetryCount = 0; RetryCount < IDE_MAX_BUSY_RETRIES; RetryCount++)
	{
	  Status = IDEReadStatus(CommandPort);
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
		      IN PSCSI_REQUEST_BLOCK Srb)
{
#if 0
//  PIDE_DRIVE_IDENTIFY DeviceParams;

//  ULONG StartingSector,i;
//  ULONG SectorCount;
  UCHAR ByteCountHigh;
  UCHAR ByteCountLow;
//  UCHAR DrvHead;
//  UCHAR SectorNumber;
//  UCHAR Command;
  ULONG Retries;
  UCHAR Status;

  DPRINT1("AtapiSendAtapiCommand() called!\n");

  if ((Srb->PathId != 0) ||
      (Srb->TargetId > 1) ||
      (Srb->Lun != 0) ||
      (DeviceExtension->DevicePresent[Srb->TargetId] == FALSE))
    {
      return(SRB_STATUS_SELECTION_TIMEOUT);
    }

  DPRINT1("AtapiSendAtapiCommand(): TargetId: %lu\n",
	 Srb->TargetId);


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
  DPRINT("status=%02x\n", Status);
  DPRINT("waited %ld usecs for busy to clear\n", Retries * 10);
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
		    IDE_DH_FIXED | (Srb->TargetId ? IDE_DH_DRV1 : 0));

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
  DPRINT("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long after drive select\n");
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
          DPRINT("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
#endif
    }

  ByteCountLow = (UCHAR)(Srb->DataTransferLength & 0xFF);
  ByteCountHigh = (UCHAR)(Srb->DataTransferLength >> 8);

  IDEWriteCylinderHigh(DeviceExtension->CommandPortBase, ByteCountHigh);
  IDEWriteCylinderLow(DeviceExtension->CommandPortBase, ByteCountLow);
//  IDEWriteDriveHead(DeviceExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /* Issue command to drive */
  IDEWriteCommand(DeviceExtension->CommandPortBase, 0xA0); /* Packet command */


  /*  wait for DRQ to assert */
  for (Retries = 0; Retries < IDE_MAX_BUSY_RETRIES; Retries++)
    {
      Status = IDEReadStatus(DeviceExtension->CommandPortBase);
      if ((Status & IDE_SR_DRQ))
	{
	  break;
	}
      ScsiPortStallExecution(10);
    }

  IDEWriteBlock(DeviceExtension->CommandPortBase,
		(PUSHORT)Srb->Cdb,
		12);


  DPRINT1("AtapiSendAtapiCommand() done\n");
  return(SRB_STATUS_PENDING);
#endif

//#if 0
  ULONG SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

  DPRINT("AtapiSendAtapiCommand() called!\n");

  switch (Srb->Cdb[0])
    {
      case SCSIOP_INQUIRY:
	DPRINT("  SCSIOP_INQUIRY\n");
	SrbStatus = AtapiInquiry(DeviceExtension,
				 Srb);
	break;

      case SCSIOP_READ_CAPACITY:
	DPRINT1("  SCSIOP_READ_CAPACITY\n");
	break;

      case SCSIOP_READ:
	DPRINT1("  SCSIOP_READ\n");
	break;

      case SCSIOP_WRITE:
	DPRINT1("  SCSIOP_WRITE\n");
	break;

      case SCSIOP_MODE_SENSE:
	DPRINT1("  SCSIOP_MODE_SENSE\n");
	break;

      case SCSIOP_TEST_UNIT_READY:
	DPRINT1("  SCSIOP_TEST_UNIT_READY\n");
	break;

      case SCSIOP_VERIFY:
	DPRINT1("  SCSIOP_VERIFY\n");
	break;

      case SCSIOP_START_STOP_UNIT:
	DPRINT1("  SCSIOP_START_STOP_UNIT\n");
	break;

      case SCSIOP_REQUEST_SENSE:
	DPRINT1("  SCSIOP_REQUEST_SENSE\n");
	break;

     default:
	DbgPrint("AtapiSendIdeCommand():unknown command %x\n",
		 Srb->Cdb[0]);
	break;
    }

  if (SrbStatus == SRB_STATUS_SELECTION_TIMEOUT)
    {
      DPRINT1("Not implemented yet!\n");
    }

  return(SrbStatus);
//#endif
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
	DbgPrint("AtapiSendIdeCommand():unknown command %x\n",
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
      /* get it from the ATAPI configuration word */
      InquiryData->DeviceType = (DeviceParams->ConfigBits >> 8) & 0x1F;
      DPRINT("Device class: %u\n", InquiryData->DeviceType);
    }

  DPRINT("ConfigBits: 0x%x\n", DeviceParams->ConfigBits);
  if (DeviceParams->ConfigBits & 0x80)
    {
      DPRINT("Removable media!\n");
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
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=1:BLK=%08d:SC=%02x:CM=%02x\n",
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
      DPRINT("%s:BUS=%04x:DRV=%d:LBA=0:CH=%02x:CL=%02x:HD=%01x:SN=%02x:SC=%02x:CM=%02x\n",
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
  DPRINT("status=%02x\n", Status);
  DPRINT("waited %ld usecs for busy to clear\n", Retries * 10);
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
  DPRINT("waited %ld usecs for busy to clear after drive select\n", Retries * 10);
  if (Retries >= IDE_MAX_BUSY_RETRIES)
    {
      DPRINT("Drive is BUSY for too long after drive select\n");
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
          DPRINT("Beginning drive reset sequence\n");
          IDEBeginControllerReset(ControllerExtension);

          return TRUE;
        }
#endif
    }

#if 0
  if (Command == IDE_CMD_WRITE)
    {
      DPRINT1("Write not implemented yet!\n");
      return(SRB_STATUS_SUCCESS);
    }
#endif

  /* Indicate expecting an interrupt. */
  DeviceExtension->ExpectingInterrupt = TRUE;

  /* Setup command parameters */
  IDEWritePrecomp(DeviceExtension->CommandPortBase, 0);
  IDEWriteSectorCount(DeviceExtension->CommandPortBase, SectorCount);
  IDEWriteSectorNum(DeviceExtension->CommandPortBase, SectorNumber);
  IDEWriteCylinderHigh(DeviceExtension->CommandPortBase, CylinderHigh);
  IDEWriteCylinderLow(DeviceExtension->CommandPortBase, CylinderLow);
  IDEWriteDriveHead(DeviceExtension->CommandPortBase, IDE_DH_FIXED | DrvHead);

  /* Issue command to drive */
  IDEWriteCommand(DeviceExtension->CommandPortBase, Command);

  /* Write data block */
  if (Command == IDE_CMD_WRITE)
    {
      PUCHAR TargetAddress;
      ULONG SectorSize;

      /* Wait for controller ready */
      for (Retries = 0; Retries < IDE_MAX_WRITE_RETRIES; Retries++)
	{
	  BYTE  Status = IDEReadStatus(DeviceExtension->CommandPortBase);
	  if (!(Status & IDE_SR_BUSY) || (Status & IDE_SR_ERR))
	    {
	      break;
	    }
	  KeStallExecutionProcessor(10);
	}
      if (Retries >= IDE_MAX_BUSY_RETRIES)
	{
	  DPRINT("Drive is BUSY for too long after sending write command\n");
	  return(SRB_STATUS_BUSY);
#if 0
          if (DeviceExtension->Retries++ > IDE_MAX_CMD_RETRIES)
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
#endif
	}

      /* Update SRB data */
      SectorSize = DeviceExtension->DeviceParams[Srb->TargetId].BytesPerSector;
      TargetAddress = Srb->DataBuffer;
      Srb->DataBuffer += SectorSize;
      Srb->DataTransferLength -= SectorSize;

      /* Write data block */
      IDEWriteBlock(DeviceExtension->CommandPortBase,
		    TargetAddress,
		    SectorSize);
    }


  DPRINT("AtapiReadWrite() done!\n");

  /* Wait for interrupt. */
  return(SRB_STATUS_PENDING);
}

/* EOF */
